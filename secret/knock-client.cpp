#include <sys/socket.h>   // socket(), sendto(), recvfrom(), struct sockaddr
#include <sys/time.h>     // struct timeval
#include <unistd.h>       // close()
#include <stdio.h>        // perror()
#include <netinet/in.h>   // struct sockaddr_in, htons(), htonl()
#include <arpa/inet.h>    // inet_pton()
#include <cstdlib>        // strtoul(), exit()
#include <cerrno>         // errno
#include <cstdint>        // uint32_t
#include <string>
#include <iostream>

// ---------------------------------------------------------------------------
// Standalone client that sends a 4-byte signature to a port and prints the
// reply. Use this once you already have a signature from secret-client.
//
// Usage: ./knock-client <IP address> <port> <signature hex, e.g. 0x1a2b3c4d>
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

// Parses a hex ("0x1a2b3c4d") or decimal signature string into a uint32_t.
uint32_t parse_signature(const char* str) {
	errno = 0;
	char* endptr;
	// base 0: auto-detects "0x" prefix for hex, otherwise decimal.
	unsigned long val = std::strtoul(str, &endptr, 0);

	if (endptr == str || *endptr != '\0') {
		std::cerr << "signature is not a valid number: " << str << std::endl;
		exit(1);
	}
	if (errno == ERANGE || val > 0xFFFFFFFFUL) {
		std::cerr << "signature does not fit in 32 bits: " << str << std::endl;
		exit(1);
	}
	return (uint32_t) val;
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


int main(int argc, const char* argv[]) {
	if (argc != 4) {
		std::cerr << "usage: " << argv[0] << " <IP address> <port> <signature>" << std::endl;
		std::cerr << "  signature: hex (0x1a2b3c4d) or decimal, from secret-client's output" << std::endl;
		exit(1);
	}

	const char* ipaddr = argv[1];
	unsigned int port = parse_port(argv[2], "port");
	uint32_t signature = parse_signature(argv[3]);

	struct sockaddr_in addr;
	int sockfd = make_socket(ipaddr, port, addr);

	// Pack the signature as 4 bytes, network byte order.
	uint32_t signature_be = htonl(signature);
	std::string msg;
	msg.append(reinterpret_cast<char*>(&signature_be), sizeof(signature_be));

	int ret = sendto(sockfd, msg.data(), msg.size(), 0, (struct sockaddr*)&addr, sizeof(addr));
	if (ret < 0) {
		perror("Error sending");
		close(sockfd);
		exit(1);
	}
	std::cout << "sent signature 0x" << std::hex << signature << std::dec
	          << " (" << msg.size() << " bytes) to port " << port << std::endl;

	struct sockaddr_in srcaddr;
	socklen_t srcaddrlen = sizeof(srcaddr);
	char buffer[2048];

	ret = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
	                (struct sockaddr*)&srcaddr, &srcaddrlen);
	if (ret < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			std::cerr << "timeout: no response from port " << port << std::endl;
		} else {
			perror("Error recieving");
		}
		close(sockfd);
		exit(1);
	}
	buffer[ret] = '\0'; // null-terminate so it's safe to print as text

	std::cout << "\nreceived: " << buffer << std::endl;

	close(sockfd);
	return 0;
}