#include "port_sender.h"

#include <arpa/inet.h>
#include <iostream>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

// ---- helpers ----

int to_port(const std::string& text) {
    try {
        int port = std::stoi(text);
        return (port >= 1 && port <= 65535) ? port : -1;
    } catch (const std::exception&) {
        return -1;
    }
}

// Adds the data as 16 bit words in network byte order. The checksum format
// treats every pair of bytes as one big endian word, so the first byte is
// shifted into the high half of the word.
uint32_t checksum_add(const uint8_t* data, size_t len, uint32_t sum) {
    size_t i = 0;
    for (; i + 1 < len; i += 2) {
        sum += (data[i] << 8) | data[i + 1];
    }

    // An odd length leaves one byte without a partner. The checksum rules
    // place that byte in the high half and use zero for the low half.
    if (i < len) sum += data[i] << 8;
    return sum;
}

// Finishes the Internet checksum after all words have been added. Carries
// above 16 bits are added back into the low 16 bits, as required by one's
// complement arithmetic, and the final sum is complemented.
uint16_t checksum_fold(uint32_t sum) {
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return static_cast<uint16_t>(~sum);
}

void put_be16(uint8_t* p, uint16_t v) {
    p[0] = static_cast<uint8_t>(v >> 8);
    p[1] = static_cast<uint8_t>(v);
}

uint16_t get_be16(const uint8_t* p) {
    return static_cast<uint16_t>((p[0] << 8) | p[1]);
}

// ---- PortSender ----

PortSender::PortSender(const std::string& remote_ip, int remote_port)
    : remote_ip_(remote_ip), remote_port_(remote_port) {}

PortSender::~PortSender() {
    close();
}

bool PortSender::open() {
    if (is_open()) return true;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(remote_port_));
    if (inet_pton(AF_INET, remote_ip_.c_str(), &addr.sin_addr) != 1) {
        std::cerr << "PortSender: invalid IPv4 address: " << remote_ip_ << "\n";
        return false;
    }

    sockfd_ = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd_ < 0) {
        perror("socket");
        return false;
    }
    // connect() on UDP just fixes the peer, so send()/recv() need no address.
    if (connect(sockfd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        perror("connect");
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
    if (!open()) return false;
    ssize_t sent = ::send(sockfd_, data.data(), data.size(), 0);
    return sent == static_cast<ssize_t>(data.size());
}

std::vector<uint8_t> PortSender::receive(int timeout_ms) {
    if (!is_open()) return {};

    timeval tv{};
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    uint8_t buf[2048];
    ssize_t n = recv(sockfd_, buf, sizeof(buf), 0);
    if (n <= 0) return {};  // timeout or error
    return std::vector<uint8_t>(buf, buf + n);
}

std::vector<uint8_t> PortSender::send_and_receive(const std::vector<uint8_t>& msg,
                                                  int max_attempts, int timeout_ms) {
    for (int attempt = 1; attempt <= max_attempts; ++attempt) {
        if (!send(msg)) {
            std::cerr << "PortSender: send failed on attempt "
                      << attempt << "/" << max_attempts << "\n";
            continue;
        }
        auto reply = receive(timeout_ms);
        if (!reply.empty()) return reply;
        std::cerr << "PortSender: no reply from port " << remote_port_
                  << " on attempt " << attempt << "/" << max_attempts << "\n";
    }
    return {};
}

std::vector<uint8_t> PortSender::send_and_receive(const std::string& msg,
                                                  int max_attempts, int timeout_ms) {
    return send_and_receive(std::vector<uint8_t>(msg.begin(), msg.end()),
                            max_attempts, timeout_ms);
}

std::string PortSender::probe() {
    auto reply = send_and_receive(std::string("      "));
    return std::string(reply.begin(), reply.end());
}
