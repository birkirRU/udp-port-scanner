// UDP port scanner: sends the S.E.C.R.E.T. message to every port in a range
// and reports the ports that answer. Used to find the four puzzle ports.
//
// Usage: ./scanner <IP address> <low port> <high port>

#include <arpa/inet.h>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>

// Parses a port number (0-65535), exiting with an error if it isn't one.
unsigned int parse_port(const char* str, const char* label) {
    char* endptr;
    errno = 0;
    unsigned long val = std::strtoul(str, &endptr, 10);

    if (endptr == str || *endptr != '\0') {
        std::cerr << label << " is not a valid unsigned integer: " << str << std::endl;
        exit(1);
    }
    if (errno == ERANGE || val > 65535) {
        std::cerr << label << " is out of valid port range (0-65535): " << str << std::endl;
        exit(1);
    }
    return static_cast<unsigned int>(val);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "usage: " << argv[0] << " <IP address> <low port> <high port>" << std::endl;
        exit(1);
    }

    const char* ipaddr = argv[1];
    unsigned int lowport = parse_port(argv[2], "low port");
    unsigned int highport = parse_port(argv[3], "high port");
    if (lowport > highport) {
        std::cerr << "low port (" << lowport << ") cannot be higher than high port ("
                  << highport << ")" << std::endl;
        exit(1);
    }

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error creating socket");
        exit(1);
    }

    // Closed ports never answer, so without a receive timeout recvfrom()
    // would block forever.
    timeval tv{};
    tv.tv_usec = 500000;  // 500 ms
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Error setting SO_RCVTIMEO");
        exit(1);
    }

    sockaddr_in d_addr{};
    d_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ipaddr, &d_addr.sin_addr) < 1) {
        std::cerr << "invalid ip address or address family: " << ipaddr << std::endl;
        exit(1);
    }

    // Message: "S.E.C.R.E.T.:<names>" followed by the secret number in network order.
    std::string message = "S.E.C.R.E.T.:birkirsa24,bjornth24";
    uint32_t secret = htonl(2189012345);
    message.append(reinterpret_cast<char*>(&secret), sizeof(secret));

    for (unsigned int port = lowport; port <= highport; port++) {
        d_addr.sin_port = htons(port);

        // The server drops about 1 in 10 requests, so try each port 3 times
        // (99.9% chance that at least one gets through).
        for (int attempt = 0; attempt < 3; attempt++) {
            if (sendto(sockfd, message.c_str(), message.size(), 0,
                       reinterpret_cast<sockaddr*>(&d_addr), sizeof(d_addr)) < 0) {
                perror("Error sending");
                exit(1);
            }

            // Leave room for the terminating '\0' added below.
            char buffer[2048];
            sockaddr_in srcaddr{};
            socklen_t srcaddrlen = sizeof(srcaddr);
            ssize_t ret = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                                   reinterpret_cast<sockaddr*>(&srcaddr), &srcaddrlen);
            if (ret < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    std::cerr << "timeout: no response " << port << std::endl;
                } else {
                    perror("Error receiving");
                }
                continue;
            }
            buffer[ret] = '\0';
            std::cout << "Port " << port << " is open" << std::endl;
            std::cout << "received: " << buffer << std::endl;
        }
    }
}
