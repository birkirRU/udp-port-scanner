#include "port_sender.h"

#include <cstring>
#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// TODO: change parameter names such that they are prescriptive of what they are refering to
// e.g. "port" should be "recieving_port" and "ip" should be "recieving_ip"
PortSender::PortSender(const std::string& ip, int port)
    : ip_(ip), port_(port) {}

PortSender::~PortSender() {
    close();
}


// TODO: have three new parameters "domain", "type", "protocal" parameter triple into socket()
// Allows for different socket creation, for example raw sockets (helpful for Gaurdain port that most likely needs raw port).
bool PortSender::open() {
    
    if (is_open()) return true;

    sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    // Raw socket example
    // int rawsock = socket(AF_INET , SOCK_RAW , IPPROTO_RAW )
    if (sockfd_ < 0) {
        perror("Error creating host socket");
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port_));

	// inet_pton(3): converts IP text ("1.2.3.4") to binary form in d_addr.sin_addr.
	// Returns 1 on success, 0 on invalid format, -1 on invalid address family.
    if (inet_pton(AF_INET, ip_.c_str(), &addr.sin_addr) != 1) {
        // USE std::endl
        std::cerr << "Invalid IP address: " << ip_ << "\n";
        close();
        return false;
    }

    // enforces a single socket is associated with a single address. 1:1.
    // using it here elimenates the need for sendto() on udp/raw sockets.
    if (connect(sockfd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("Association of host socket and recieiving socket failed");
        close();
        return false;
    }

    return true;
}

void PortSender::close() {
    if (is_open()) {
        ::close(sockfd_);
        sockfd_ = -1;
    }
}

bool PortSender::send(const std::vector<uint8_t>& data) {
    if (!is_open() && !open()) return false;
    ssize_t sent = ::send(sockfd_, data.data(), data.size(), 0);
    return sent == static_cast<ssize_t>(data.size());
}

bool PortSender::send(const std::string& data) {
    return send(std::vector<uint8_t>(data.begin(), data.end()));
}

std::vector<uint8_t> PortSender::receive(int timeout_ms) {
    std::vector<uint8_t> result;
    if (!is_open()) return result;

    timeval tv{};
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    uint8_t buf[2048];
    ssize_t n = recv(sockfd_, buf, sizeof(buf), 0);
    if (n > 0) {
        result.assign(buf, buf + n);
    }
    // n <= 0: timeout or error, return empty vector.
    return result;
}

std::string PortSender::probe() {
    // Every puzzle port expects a message >= 6 characters long before
    // it hands out instructions. Plain spaces are a safe, content-free
    // default; override in a derived class if a module needs something
    // more specific to trigger its response.
    if (!send(std::string("      "))) return "";
    auto bytes = receive();
    return std::string(bytes.begin(), bytes.end());
}