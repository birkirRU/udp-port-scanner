#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <string>
#include <iostream>


int main (int argc, const char* argv[]) {
	// . / scanner <IP address> <low port> <high port>
	// TODO: check if all arguments are provided
	const char *ipaddr = argv[1]; 

	// TODO: check if port is int, check if low is actually 
	unsigned int lowport = argv[2]#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <string>
#include <iostream>


int main (int argc, const char* argv[]) {
	// . / scanner <IP address> <low port> <high port>
	// TODO: check if all arguments are provided
	const char *ipaddr = argv[1]; 

	// TODO: check if port is int, check if low is actually 
	unsigned int lowport = std::atoi(argv[2]);
	unsigned int highport = std::atoi(argv[3]);


	// Initiate Socket and its protocal (UDP)
	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Error creating socket");
		exit(1);
	}

	// Payload to send to server.
	std::string message = "Hello World!";	



	// TODO: iterate over lowport - highport, for each send a payload, wait for recvfrom.
	//       To do this fast we need parallel.
	// Destination address
	struct sockaddr_in d_addr;
	d_addr.sin_family = AF_INET;
	d_addr.sin_port = htons(lowport); // htons(port).

		
	if (inet_pton(AF_INET, ipaddr, &d_addr.sin_addr) < 1) {
		std::cerr << "invalid ip address or address family: " << ipaddr << std::endl;
		exit(1);
	}

	// Send over to server 
	int ret;
	if ((ret = sendto(sockfd, message.c_str(), message.size(), 0 /*flags*/,
					(struct sockaddr*)&d_addr, sizeof(d_addr))) < 0) {
			perror("Error sending");
			exit(1);
	}
	
	struct sockaddr_in srcaddr;
	socklen_t srcaddrlen = sizeof(srcaddr);
	char buffer[2024];
	// Maximum Transmission Unit (max data size sent in a single physical network packet)
	// is 1500 bytes, we use 2KiB.

	// There is a bug here, we need to find it.
	// TODO: Prevent buffer overflow on recieving mesege. 
	// TODO: utalize flags and sockets option in order to timeout if nothing is being recieved.
	if ((ret = recvfrom(sockfd, buffer, sizeof(buffer), 0,
				(struct sockaddr*) &srcaddr, &srcaddrlen)) < 0) {
		perror("Error recieving");
		exit(1);
	}
	std::cout << "received: " << buffer << std::endl;

};
	unsigned int highport = argv[3];


	// Initiate Socket and its protocal (UDP)
	int sockfd;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Error creating socket");
		exit(1);
	}

	// Payload to send to server.
	std::string message = "Hello World!";	



	// TODO: iterate over lowport - highport, for each send a payload, wait for recvfrom.
	//       To do this fast we need parallel.
	// Destination address
	struct sockaddr_in d_addr;
	d_addr.sin_family = AF_INET;
	d_addr.sin_port = htons(port); // htons(port).

		
	if (inet_pton(AF_INET, ipaddr, &d_addr.sin_addr) < 1) {
		std::cerr << "invalid ip address or address family: " << ipaddr << std::endl;
		exit(1);
	}

	// Send over to server 
	int ret;
	if ((ret = sendto(sockfd, &s, sizeof(s), 0 /*flags*/,
					(stuct sockaddre*)&d_addr, sizeof(d_addr))) < 0 {
			perror("Error sending");
			exit(1);
	}
	
	struct sockaddr_in srcaddr;
	socklen_t srcaddrlen;
	char buffer[2024] 
	// Maximum Transmission Unit (max data size sent in a single physical network packet)
	// is 1500 bytes, we use 2KiB.

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
