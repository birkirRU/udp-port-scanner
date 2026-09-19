#include "evil_port.h"

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {
// One's-complement checksum over big-endian 16-bit words (RFC 1071).
uint16_t checksum16(const uint8_t* data, size_t len) {
    uint32_t sum = 0;
    for (size_t i = 0; i + 1 < len; i += 2) sum += (data[i] << 8) | data[i + 1];
    if (len % 2) sum += data[len - 1] << 8;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return static_cast<uint16_t>(~sum);
}

// Raw IPv4 sockets want ip_len / ip_off in network order on Linux,
// but in host order on macOS/BSD.
uint16_t ip_hdr_field(uint16_t v) {
#ifdef __APPLE__
    return v;
#else
    return htons(v);
#endif
}

} // namespace what does this do?


EvilPort::EvilPort(const std::string& ip, int port)
    : PortSender(ip, port) {}

bool EvilPort::identify(const std::string& response) const {
    return response.find("I am an evil port") != std::string::npos;
}

std::vector<uint8_t> EvilPort::send_with_evil_bit(const std::vector<uint8_t>& payload) {
    // Ordinary UDP socket: owns our source port and receives the reply.
    if (!is_open() && !open()) return {};

    // Which local address and port did the kernel pick for that socket?
    sockaddr_in local{};
    socklen_t local_len = sizeof(local);
    if (getsockname(sockfd_, reinterpret_cast<sockaddr*>(&local), &local_len) < 0) {
        perror("getsockname");
        return {};
    }

    sockaddr_in remote{};
    remote.sin_family = AF_INET;
    remote.sin_port = htons(static_cast<uint16_t>(remote_port_));
    if (inet_pton(AF_INET, remote_ip_.c_str(), &remote.sin_addr) != 1) {
        std::cerr << "EvilPort: bad IP " << remote_ip_ << "\n";
        return {};
    }

    // Raw socket: lets us write the IP header ourselves.
    int raw = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (raw < 0) {
        perror("socket(SOCK_RAW) failed");
        return {};
    }
    int on = 1;
    if (setsockopt(raw, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
        perror("setsockopt(IP_HDRINCL)");
        ::close(raw);
        return {};
    }

    const uint16_t udp_len = static_cast<uint16_t>(8 + payload.size());
    const uint16_t total_len = static_cast<uint16_t>(20 + udp_len);

    std::vector<uint8_t> pkt(28, 0);

    // IPv4 header (20 bytes)
    pkt[0] = 0x45;                               // version 4 and header length 5 words
    uint16_t v = ip_hdr_field(total_len);
    std::memcpy(&pkt[2], &v, 2);                 // total length
    v = htons(0x1234);
    std::memcpy(&pkt[4], &v, 2);                 // identification 
    v = ip_hdr_field(0x8000);                    // THE EVIL BIT: top bit of flags/fragment
    std::memcpy(&pkt[6], &v, 2);
    pkt[8] = 64;                                 // TTL
    pkt[9] = IPPROTO_UDP;                        // protocol = UDP
    std::memcpy(&pkt[12], &local.sin_addr, 4);   // source address
    std::memcpy(&pkt[16], &remote.sin_addr, 4);  // destination address
    uint16_t ck = checksum16(pkt.data(), 20);    
    pkt[10] = static_cast<uint8_t>(ck >> 8);
    pkt[11] = static_cast<uint8_t>(ck & 0xFF);

    // UDP header (8 bytes)
    std::memcpy(&pkt[20], &local.sin_port, 2);   // source port (already network order)
    std::memcpy(&pkt[22], &remote.sin_port, 2);  // destination port
    pkt[24] = static_cast<uint8_t>(udp_len >> 8);
    pkt[25] = static_cast<uint8_t>(udp_len & 0xFF);

    pkt.insert(pkt.end(), payload.begin(), payload.end());

    std::vector<uint8_t> reply;
    for (int attempt = 0; attempt < 3 && reply.empty(); ++attempt) {
        ssize_t n = sendto(raw, pkt.data(), pkt.size(), 0, 
                    reinterpret_cast<sockaddr*>(&remote), sizeof(remote));
        if (n != static_cast<ssize_t>(pkt.size())) {
            perror("sendto");
            break;
        }
        reply = receive();
    }
    ::close(raw);
    return reply;
}


bool EvilPort::solve(PuzzleSession& session) {
    if (!session.secret_done) {
        std::cerr << "EvilPort: needs group id and sigil from SecretPort first\n";
        return false;
    }

    // [group_id][sigil, 4 bytes big-endian], sent with the evil bit set.
    std::vector<uint8_t> id_msg{session.group_id};
    uint32_t sigil_be = htonl(session.sigil);
    uint8_t sb[4];
    std::memcpy(sb, &sigil_be, 4);
    id_msg.insert(id_msg.end(), sb, sb + 4);

    auto reply = send_with_evil_bit(id_msg);
    if (reply.empty()) {
        std::cerr << "EvilPort: no reply (evil bit stripped, or the port ignored the message)\n";
        return false;
    }
    reply_text_.assign(reply.begin(), reply.end());
    std::cerr << "EvilPort revealed: " << reply_text_ << "\n";

    // The reply ends with the port number
    // so take the last run of digits in the text.
    size_t last = reply_text_.find_last_of("0123456789");
    if (last == std::string::npos) {
        std::cerr << "EvilPort: no port number found in reply\n";
        return false;
    }
    size_t before = reply_text_.find_last_not_of("0123456789", last);
    size_t first = (before == std::string::npos) ? 0 : before + 1;
    int port = std::stoi(reply_text_.substr(first, last - first + 1));
    if (port < 1 || port > 65535) {
        std::cerr << "EvilPort: implausible port " << port << "\n";
        return false;
    }

    session.secret_port_1 = port;
    session.evil_done = true;
    return true;
}