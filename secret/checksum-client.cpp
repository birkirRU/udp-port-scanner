#include <sys/socket.h>   // socket(), sendto(), recvfrom()
#include <sys/time.h>     // struct timeval
#include <unistd.h>       // close()
#include <stdio.h>        // perror()
#include <netinet/in.h>   // struct sockaddr_in, htons(), htonl(), ntohs()
#include <arpa/inet.h>    // inet_pton()
#include <cstdlib>        // strtoul(), exit()
#include <cerrno>         // errno
#include <cstring>        // memcpy(), memset()
#include <cstdint>        // uint8_t, uint16_t, uint32_t
#include <string>
#include <iostream>

// ---------------------------------------------------------------------------
// Solves the "checksum" challenge: the server wants a reply whose payload is
// a fake IP+UDP header pair with:
//   - source address = an address the server gives us
//   - UDP checksum    = an exact target value the server gives us
//
// We're free to choose everything else in that fake header, so we reserve a
// 2-byte "adjustment word" in the payload and solve for the value that makes
// the real, correctly-computed checksum equal the required target exactly.
// This is an ordinary UDP socket exercise - no raw sockets required.
//
// Usage: ./checksum-client <IP address> <port> <signature>
// ---------------------------------------------------------------------------

#pragma pack(push, 1)
struct ip_header {
	uint8_t  ihl_version;
	uint8_t  tos;
	uint16_t tot_len;
	uint16_t id;
	uint16_t flags_frag;
	uint8_t  ttl;
	uint8_t  protocol;
	uint16_t checksum;
	uint32_t saddr;
	uint32_t daddr;
};

struct udp_header {
	uint16_t src_port;
	uint16_t dst_port;
	uint16_t length;
	uint16_t checksum;
};

struct pseudo_header {
	uint32_t src;
	uint32_t dst;
	uint8_t  zero;
	uint8_t  protocol;
	uint16_t udp_len;
};
#pragma pack(pop)

// One's-complement 16-bit addition with end-around carry.
uint32_t ones_add16(uint32_t a, uint32_t b) {
	uint32_t s = a + b;
	while (s >> 16) {
		s = (s & 0xFFFF) + (s >> 16);
	}
	return s & 0xFFFF;
}

// Folded one's-complement sum of the data, in host byte order, NOT yet
// complemented (so we can do arithmetic with it before finalizing).
uint16_t raw_sum(const void* data, size_t length) {
	const uint8_t* bytes = (const uint8_t*)data;
	uint32_t acc = 0;
	for (size_t i = 0; i + 1 < length; i += 2) {
		uint16_t word;
		std::memcpy(&word, bytes + i, 2);
		acc = ones_add16(acc, ntohs(word));
	}
	if (length & 1) {
		acc = ones_add16(acc, ((uint16_t)bytes[length - 1]) << 8);
	}
	return (uint16_t)acc;
}

uint16_t checksum(const void* data, size_t length) {
	return htons((uint16_t)(~raw_sum(data, length) & 0xFFFF));
}

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

uint32_t parse_signature(const char* str) {
	errno = 0;
	char* endptr;
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


int main(int argc, const char* argv[]) {
	if (argc != 4) {
		std::cerr << "usage: " << argv[0] << " <IP address> <port> <signature>" << std::endl;
		exit(1);
	}

	const char* ipaddr = argv[1];
	unsigned int port = parse_port(argv[2], "port");
	uint32_t signature = parse_signature(argv[3]);

	uint32_t target_addr;
	if (inet_pton(AF_INET, ipaddr, &target_addr) < 1) {
		std::cerr << "invalid ip address: " << ipaddr << std::endl;
		exit(1);
	}

	int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("Error creating socket");
		exit(1);
	}
	struct timeval tv{3, 0};
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	struct sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons((uint16_t)port);
	addr.sin_addr.s_addr = target_addr;

	// --- Step 1: send our signature to get the challenge. ---
	uint32_t signature_be = htonl(signature);
	if (sendto(sockfd, &signature_be, sizeof(signature_be), 0,
	           (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("Error sending signature");
		exit(1);
	}

	char buffer[2048];
	socklen_t addrlen = sizeof(addr);
	int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&addr, &addrlen);
	if (n < 6) {
		std::cerr << "error: challenge reply too short to contain checksum+address trailer ("
		          << n << " bytes)" << std::endl;
		close(sockfd);
		exit(1);
	}
	std::cout << "received challenge (" << n << " bytes)" << std::endl;

	// --- Step 2: parse the last 6 bytes: 2-byte checksum + 4-byte address, both network order. ---
	uint16_t target_checksum_be;
	uint32_t embedded_saddr;
	std::memcpy(&target_checksum_be, buffer + n - 6, 2);
	std::memcpy(&embedded_saddr, buffer + n - 2, 4);

	uint16_t target_checksum_host = ntohs(target_checksum_be);

	char saddr_str[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &embedded_saddr, saddr_str, sizeof(saddr_str));
	std::cout << "target checksum: 0x" << std::hex << target_checksum_host << std::dec << std::endl;
	std::cout << "required source address: " << saddr_str << std::endl;

	// --- Step 3: build the fake IP + UDP header. We choose daddr/ports freely; ---
	// --- adjust if the server expects specific values instead of these defaults. ---
	ip_header ip{};
	ip.ihl_version = (4 << 4) | 5;
	ip.tos = 0;
	ip.id = htons(0x1234);
	ip.flags_frag = 0;
	ip.ttl = 64;
	ip.protocol = IPPROTO_UDP;
	ip.checksum = 0;
	ip.saddr = embedded_saddr;      // required value from the server
	ip.daddr = target_addr;         // assumption: the challenge server's own address

	udp_header udp{};
	udp.src_port = htons(12345);    // arbitrary - free to choose
	udp.dst_port = htons((uint16_t)port); // assumption: same challenge port
	udp.length = htons((uint16_t)(sizeof(udp_header) + 2)); // header + 2-byte adjustment word
	udp.checksum = 0; // filled in below

	ip.tot_len = htons((uint16_t)(sizeof(ip_header) + ntohs(udp.length)));

	// --- Step 4: solve for the 2-byte adjustment word that makes the real ---
	// --- checksum equal the required target exactly. ---
	pseudo_header ph{};
	ph.src = ip.saddr;
	ph.dst = ip.daddr;
	ph.zero = 0;
	ph.protocol = IPPROTO_UDP;
	ph.udp_len = udp.length;

	uint16_t filler_placeholder = 0;
	uint8_t sumbuf[sizeof(pseudo_header) + sizeof(udp_header) + 2];
	std::memcpy(sumbuf, &ph, sizeof(ph));
	std::memcpy(sumbuf + sizeof(ph), &udp, sizeof(udp)); // udp.checksum is 0 here
	std::memcpy(sumbuf + sizeof(ph) + sizeof(udp), &filler_placeholder, 2);

	uint16_t s0 = raw_sum(sumbuf, sizeof(sumbuf)); // sum with filler = 0, host order
	uint32_t s_target = (~target_checksum_host) & 0xFFFF;
	uint32_t filler = ones_add16(s_target, (~(uint32_t)s0) & 0xFFFF);

	std::cout << "computed adjustment word: 0x" << std::hex << filler << std::dec << std::endl;

	// Sanity check: recompute with the real filler in place and confirm it
	// produces exactly the required checksum.
	uint16_t filler_be = htons((uint16_t)filler);
	std::memcpy(sumbuf + sizeof(ph) + sizeof(udp), &filler_be, 2);
	uint16_t check = (uint16_t)(~raw_sum(sumbuf, sizeof(sumbuf)) & 0xFFFF);
	if (check != target_checksum_host) {
		std::cerr << "warning: adjustment math didn't converge (got 0x" << std::hex << check
		          << ", wanted 0x" << target_checksum_host << std::dec << ")" << std::endl;
	} else {
		std::cout << "adjustment verified: checksum matches target" << std::endl;
	}

	udp.checksum = target_checksum_be; // use the exact bytes the server gave us
	ip.checksum = checksum(&ip, sizeof(ip)); // fake IP header's own checksum, computed normally

	// --- Step 5: assemble and send the fake header + adjustment word as our payload. ---
	uint8_t packet[sizeof(ip_header) + sizeof(udp_header) + 2];
	std::memcpy(packet, &ip, sizeof(ip));
	std::memcpy(packet + sizeof(ip), &udp, sizeof(udp));
	std::memcpy(packet + sizeof(ip) + sizeof(udp), &filler_be, 2);

	if (sendto(sockfd, packet, sizeof(packet), 0, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("Error sending forged header");
		close(sockfd);
		exit(1);
	}
	std::cout << "sent forged IP+UDP header (" << sizeof(packet) << " bytes)" << std::endl;

	// --- Step 6: receive the server's verdict. ---
	n = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr*)&addr, &addrlen);
	if (n < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK) {
			std::cerr << "timeout: no response" << std::endl;
		} else {
			perror("Error recieving");
		}
		close(sockfd);
		exit(1);
	}
	buffer[n] = '\0';
	std::cout << "\nreceived: " << buffer << std::endl;

	close(sockfd);
	return 0;
}