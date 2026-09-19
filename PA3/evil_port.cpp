#include "evil_port.h"

#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {
constexpr size_t kIpHdrLen = 20;
constexpr size_t kUdpHdrLen = 8;

// Raw IPv4 sockets want ip_len / ip_off in network order on Linux, but in
// host order on macOS/BSD.
void put_ip_field(uint8_t* p, uint16_t v) {
#ifdef __APPLE__
    std::memcpy(p, &v, 2);
#else
    put_be16(p, v);
#endif
}

// Builds a complete IPv4+UDP packet with the evil bit set. We write the IP
// header ourselves because a normal UDP socket can't set that bit.
std::vector<uint8_t> build_packet(const sockaddr_in& local, const sockaddr_in& remote,
                                  const std::vector<uint8_t>& payload) {
    const uint16_t udp_len = static_cast<uint16_t>(kUdpHdrLen + payload.size());
    const uint16_t total_len = static_cast<uint16_t>(kIpHdrLen + udp_len);
    std::vector<uint8_t> pkt(kIpHdrLen + kUdpHdrLen, 0);

    // IPv4 header
    pkt[0] = 0x45;                               // version 4, header length 5 words
    put_ip_field(&pkt[2], total_len);
    put_be16(&pkt[4], 0x1234);                   // identification
    put_ip_field(&pkt[6], 0x8000);               // the evil bit
    pkt[8] = 64;                                 // TTL
    pkt[9] = IPPROTO_UDP;
    std::memcpy(&pkt[12], &local.sin_addr, 4);
    std::memcpy(&pkt[16], &remote.sin_addr, 4);
    put_be16(&pkt[10], checksum_fold(checksum_add(pkt.data(), kIpHdrLen)));

    // UDP header (checksum left 0, which is allowed over IPv4). Ports are
    // copied as-is: sockaddr already holds them in network order.
    std::memcpy(&pkt[20], &local.sin_port, 2);
    std::memcpy(&pkt[22], &remote.sin_port, 2);
    put_be16(&pkt[24], udp_len);

    pkt.insert(pkt.end(), payload.begin(), payload.end());
    return pkt;
}
}  // namespace

bool EvilPort::identify(const std::string& response) {
    return response.find("I am an evil port") != std::string::npos;
}

// We keep the ordinary UDP socket open only to own our source port (so the
// server's reply comes back to it) and send the packet on a raw socket.
bool EvilPort::send(const std::vector<uint8_t>& payload) {
    if (!open()) return false;

    sockaddr_in local{}, remote{};
    socklen_t len = sizeof(local);
    if (getsockname(sockfd_, reinterpret_cast<sockaddr*>(&local), &len) < 0) {
        perror("getsockname");
        return false;
    }
    len = sizeof(remote);
    if (getpeername(sockfd_, reinterpret_cast<sockaddr*>(&remote), &len) < 0) {
        perror("getpeername");
        return false;
    }

    int raw = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (raw < 0) {
        perror("socket(SOCK_RAW)");
        return false;
    }
    int on = 1;
    if (setsockopt(raw, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
        perror("setsockopt(IP_HDRINCL)");
        ::close(raw);
        return false;
    }

    auto pkt = build_packet(local, remote, payload);
    ssize_t n = sendto(raw, pkt.data(), pkt.size(), 0,
                       reinterpret_cast<sockaddr*>(&remote), sizeof(remote));
    ::close(raw);
    if (n != static_cast<ssize_t>(pkt.size())) {
        perror("sendto");
        return false;
    }
    return true;
}

// Send our identity (via send() above, so the evil bit is set); the reply
// ends with the secret port number.
bool EvilPort::solve(PuzzleSession& session) {
    if (!session.secret_done) {
        std::cerr << "EvilPort: needs Secret to run first\n";
        return false;
    }

    auto reply = send_and_receive(session.identity_bytes());
    if (reply.empty()) {
        std::cerr << "EvilPort: no reply (evil bit stripped, or not running as root)\n";
        return false;
    }
    std::string text(reply.begin(), reply.end());
    std::cerr << "EvilPort revealed: " << text << "\n";

    // Take the last run of digits in the text.
    size_t last = text.find_last_of("0123456789");
    if (last == std::string::npos) {
        std::cerr << "EvilPort: no port number found in reply\n";
        return false;
    }
    size_t before = text.find_last_not_of("0123456789", last);
    size_t first = (before == std::string::npos) ? 0 : before + 1;
    int port = to_port(text.substr(first, last - first + 1));
    if (port < 0) {
        std::cerr << "EvilPort: invalid port in reply\n";
        return false;
    }

    session.secret_port_2 = static_cast<uint16_t>(port);
    session.evil_done = true;
    return true;
}
