#include <sys/socket.h>   // socket(), sendto(), recvfrom(), struct sockaddr
#include <stdio.h>        // perror()
#include <netinet/in.h>   // struct sockaddr_in, htons()
#include <arpa/inet.h>    // inet_pton()
#include <cstdlib>        // strtoul(), exit()
#include <cerrno>         // errno
#include <climits>        // UINT_MAX
#include <string>
#include <iostream>
#include <sys/time.h>   // struct timeval

// Parses a C-string as a port number (0-65535).
// Exits with an error message if the string isn't a valid unsigned integer,
// or falls outside the valid port range.
unsigned int parse_port(const char* str, const char* label) {
	// strtoul(3): converts string to unsigned long; 
	// errors via errno and endptr.
	char* endptr;
	unsigned long val = std::strtoul(str, &endptr, 10);

	// endptr == str: no digits were parsed at all (e.g. "abc").
	// *endptr != '\0': trailing garbage after the number (e.g. "80abc").
	if (endptr == str || *endptr != '\0') {
		std::cerr << label << " is not a valid unsigned integer: " << str << std::endl;
		exit(1);
	}
	// ERANGE: value too large to fit in unsigned long.
	if (errno == ERANGE || val > 65535) {
		std::cerr << label << " is out of valid port range (0-65535): " << str << std::endl;
		exit(1);
	}

	return (unsigned int) val;
}

int main (int argc, const char* argv[]) {
	// . / scanner <IP address> <low port> <high port>
	if (argc != 4) {
		std::cerr << "usage: " << argv[0] << " <IP address> <low port> <high port>" << std::endl;
		exit(1);
	}

	const char *ipaddr = argv[1];

	unsigned int lowport = parse_port(argv[2], "low port");
	unsigned int highport = parse_port(argv[3], "high port");

	if (lowport > highport) {
		std::cerr << "low port (" << lowport << ") cannot be higher than high port (" << highport << ")" << std::endl;
		exit(1);
	}

	// socket(2): creates an endpoint for communication.
	// AF_INET = IPv4, SOCK_DGRAM = UDP, 0 = default protocol for this type.
	// Returns a file descriptor, or -1 on error.
	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Error creating socket");  // perror(3): prints errno's message to stderr.
		exit(1);
	}

	// Payload to send to server.
	std::string message = "Hello World!";	

	// TODO: iterate over lowport - highport, for each send a payload, wait for recvfrom.
	//       To do this fast we need parallel.

	// struct sockaddr_in (see ip(7)): IPv4 socket address - family, port, address.
	struct sockaddr_in d_addr;
	d_addr.sin_family = AF_INET;
	// htons(3): host-to-network short - converts port to network byte order (big-endian).
	d_addr.sin_port = htons(lowport); // htons(port).

	// inet_pton(3): converts IP text ("1.2.3.4") to binary form in d_addr.sin_addr.
	// Returns 1 on success, 0 on invalid format, -1 on invalid address family.
	if (inet_pton(AF_INET, ipaddr, &d_addr.sin_addr) < 1) {
		std::cerr << "invalid ip address or address family: " << ipaddr << std::endl;
		exit(1);
	}

	// sendto(2): sends a message on a socket, optionally to a specific address (used since
	// UDP is connectionless, no connect() call was made).
	// Args: socket fd, buffer, length, flags, destination address, address length.
	// Returns number of bytes sent, or -1 on error.
	int ret;
	if ((ret = sendto(sockfd, message.c_str(), message.size(), 0 /*flags*/,
					(struct sockaddr*)&d_addr, sizeof(d_addr))) < 0) {
			perror("Error sending");
			exit(1);
	}
	
	// Will hold the address of whoever replies to us.
	struct sockaddr_in srcaddr;
	// socklen_t must be pre-set to the size of srcaddr; recvfrom() updates it to the
	// actual size of the address written.
	socklen_t srcaddrlen = sizeof(srcaddr);
	char buffer[2024];
	// Maximum Transmission Unit (max data size sent in a single physical network packet)
	// is 1500 bytes, we use 2KiB.



	struct timeval tv;
	tv.tv_sec = 2;    // seconds
	tv.tv_usec = 0;   // microseconds


	// Apply the receive timeout (SO_RCVTIMEO) to the socket configuration.
	// This prevents blocking receive functions (like recv/recvfrom) from hanging forever.
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
		perror("Error setting SO_RCVTIMEO");
		exit(1);
	}

	
	// recvfrom(2): reads a datagram, optionally capturing the sender's address.
	// Args: socket fd, buffer, buffer length, flags, source address (out), address length (in/out).
	// Returns number of bytes received, or -1 on error.
	// There is a bug here, we need to find it.
	// TODO: Prevent buffer overflow on recieving mesege. 
	// TODO: utalize flags and sockets option in order to timeout if nothing is being recieved.
	if ((ret = recvfrom(sockfd, buffer, sizeof(buffer), 0,
				(struct sockaddr*) &srcaddr, &srcaddrlen)) < 0) {
		perror("Error recieving");
		exit(1);
	}
	std::cout << "received: " << buffer << std::endl;

}