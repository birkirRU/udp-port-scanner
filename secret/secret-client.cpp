#include <sys/socket.h>   // socket(), sendto(), recvfrom(), struct sockaddr
#include <sys/time.h>     // struct timeval
#include <unistd.h>       // close(), getpid()
#include <stdio.h>        // perror()
#include <netinet/in.h>   // struct sockaddr_in, htons(), htonl(), ntohl()
#include <arpa/inet.h>    // inet_pton()
#include <cstdlib>        // strtoul(), exit(), rand(), srand()
#include <cerrno>         // errno
#include <climits>        // UINT_MAX
#include <cstring>        // memcpy()
#include <cstdint>        // uint32_t, uint8_t
#include <ctime>          // time()
#include <string>
#include <iostream>

// ---------------------------------------------------------------------------
// Standalone client for the S.E.C.R.E.T protocol handshake:
//   1. Generate a 32-bit secret number.
//   2. Send 'S' + secret number (network order) + usernames.
//   3. Receive 5 bytes back: group id (1 byte) + challenge (4 bytes, network order).
//   4. signature = challenge XOR secret_number.
//   5. Send group id + signature (5 bytes, network order).
//   6. Receive the secret port as a text response.
//
// Usage: ./secret-client <IP address> <port> <usernames>
//   usernames: comma-separated RU usernames, e.g. alice,bob,carol
// ---------------------------------------------------------------------------


// Parses a C-string as a port number (0-65535).
unsigned int parse_port(const char* str, const char* label) {
	errno = 0;
	char* endptr;
	unsigned long val = std::strtoul(str, &endptr, 10);

	if (endptr == str || *endptr != '\0') {
		std::cerr << label << " is not a valid unsigned integer: " << str << std::endl;
		exit(1);
	}
	if (errno == ERANGE || val > 65535) {
		std::cerr << label << " is out of valid port range (0-65535): " << str << std::endl;
		exit(1);
	}
	return (unsigned int) val;
}

// Creates a UDP socket, fills out_addr with the destination IP/port, and sets
// a receive timeout so recvfrom() never blocks forever.
int make_socket(const char* ipaddr, unsigned int port, struct sockaddr_in& out_addr,
                 unsigned int timeout_sec = 3) {
	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("Error creating socket");
		exit(1);
	}

	out_addr.sin_family = AF_INET;
	out_addr.sin_port = htons(port);
	if (inet_pton(AF_INET, ipaddr, &out_addr.sin_addr) < 1) {
		std::cerr << "invalid ip address or address family: " << ipaddr << std::endl;
		exit(1);
	}

	struct timeval tv;
	tv.tv_sec = timeout_sec;
	tv.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
		perror("Error setting SO_RCVTIMEO");
		exit(1);
	}

	return sockfd;
}

// Sends raw bytes to addr. Returns true on success.
bool send_bytes(int sockfd, struct sockaddr_in& addr, const char* data, size_t len) {
	int ret = sendto(sockfd, data, len, 0, (struct sockaddr*)&addr, sizeof(addr));
	if (ret < 0) {
		perror("Error sending");
		return false;
	}
	return true;
}

// Receives into buf (leaving room for a null terminator). Returns number of
// bytes received, or -1 on timeout/error. Sets timed_out accordingly.
int recv_bytes(int sockfd, char* buf, size_t bufsize, bool& timed_out) {
	struct sockaddr_in srcaddr;
	socklen_t srcaddrlen = sizeof(srcaddr);
	timed_out = false;

	int ret = recvfrom(sockfd, buf, bufsize - 1, 0, (struct sockaddr*)&srcaddr, &srcaddrlen);
	if (ret < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			timed_out = true;
		} else {
			perror("Error recieving");
		}
		return -1;
	}
	buf[ret] = '\0'; // null-terminate so it's safe to print/inspect as text
	return ret;
}


int main(int argc, const char* argv[]) {
	if (argc != 4) {
		std::cerr << "usage: " << argv[0] << " <IP address> <port> <usernames>" << std::endl;
		std::cerr << "  usernames: comma-separated RU usernames, e.g. alice,bob,carol" << std::endl;
		exit(1);
	}

	const char* ipaddr = argv[1];
	unsigned int port = parse_port(argv[2], "port");
	std::string usernames = argv[3];

	struct sockaddr_in addr;
	int sockfd = make_socket(ipaddr, port, addr);

	// Step 1: generate a 32-bit secret number locally.
	std::srand((unsigned)std::time(nullptr) ^ (unsigned)getpid());
	uint32_t secret_number = ((uint32_t)std::rand() << 16) ^ (uint32_t)std::rand();
	std::cout << "generated secret number: 0x" << std::hex << secret_number << std::dec << std::endl;

	// Step 2: send 'S' + secret_number (network byte order) + usernames.
	uint32_t secret_be = htonl(secret_number);
	std::string msg;
	msg += 'S';
	msg.append(reinterpret_cast<char*>(&secret_be), sizeof(secret_be));
	msg += usernames;

	if (!send_bytes(sockfd, addr, msg.data(), msg.size())) {
		close(sockfd);
		exit(1);
	}
	std::cout << "sent secret number + usernames (" << msg.size() << " bytes)" << std::endl;

	// Step 3: receive 5-byte reply: 1 byte group id + 4 bytes challenge (network order).
	char reply[64];
	bool timed_out;
	int n = recv_bytes(sockfd, reply, sizeof(reply), timed_out);
	if (n < 5) {
		std::cerr << "error: expected at least 5 bytes for group id + challenge, got " << n
		          << (timed_out ? " (timeout)" : "") << std::endl;
		close(sockfd);
		exit(1);
	}

	uint8_t group_id = (uint8_t)reply[0];
	uint32_t challenge_be;
	std::memcpy(&challenge_be, reply + 1, sizeof(challenge_be));
	uint32_t challenge = ntohl(challenge_be);
	std::cout << "received group_id=" << (int)group_id
	          << " challenge=0x" << std::hex << challenge << std::dec << std::endl;

	// Step 4: XOR challenge with our secret number to get the signature.
	uint32_t signature = challenge ^ secret_number;
	std::cout << "computed signature=0x" << std::hex << signature << std::dec << std::endl;

	// Step 5: send back group id + signature (network byte order).
	uint32_t signature_be = htonl(signature);
	std::string signed_msg;
	signed_msg += (char)group_id;
	signed_msg.append(reinterpret_cast<char*>(&signature_be), sizeof(signature_be));

	if (!send_bytes(sockfd, addr, signed_msg.data(), signed_msg.size())) {
		close(sockfd);
		exit(1);
	}
	std::cout << "sent group_id + signature (" << signed_msg.size() << " bytes)" << std::endl;

	// Step 6: receive the secret port (text response).
	char port_reply[256];
	n = recv_bytes(sockfd, port_reply, sizeof(port_reply), timed_out);
	if (n > 0) {
		std::cout << "\nS.E.C.R.E.T says: " << port_reply << std::endl;
	} else {
		std::cerr << "error: no response to signature"
		          << (timed_out ? " (timeout)" : "") << std::endl;
		close(sockfd);
		exit(1);
	}

	close(sockfd);

	// Final summary - keep these, other ports will want them.
	std::cout << "\n--- credentials for reuse against other ports ---" << std::endl;
	std::cout << "group_id:  " << (int)group_id << std::endl;
	std::cout << "signature: 0x" << std::hex << signature << std::dec << std::endl;

	return 0;
}