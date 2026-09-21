#include "guardian_port.h"

#include <array>
#include <cstring>
#include <iostream>
#include <netinet/in.h>

namespace {
// Wrapped packet layout: [40 byte IPv6 header][8 byte UDP header][payload].
constexpr size_t Ip6HdrLen = 40;
constexpr size_t UdpHdrLen = 8;
constexpr size_t SrcAddrOffset = 8;
constexpr size_t DstAddrOffset = 24;
constexpr size_t UdpOffset = Ip6HdrLen;
constexpr size_t PayloadOffset = Ip6HdrLen + UdpHdrLen;

using Addr = std::array<uint8_t, 16>;

bool is_wrapped_ipv6(const std::vector<uint8_t>& pkt) {
    return pkt.size() >= PayloadOffset && (pkt[0] >> 4) == 6;
}

Addr addr_at(const std::vector<uint8_t>& pkt, size_t offset) {
    Addr addr;
    std::memcpy(addr.data(), pkt.data() + offset, addr.size());
    return addr;
}

// Builds the wrapped packet. `flow` is the version/traffic class/flow label
// word, echoed from the banner because the server rejects any other value.
std::vector<uint8_t> build_packet(const std::array<uint8_t, 4>& flow,
                                  const Addr& src, const Addr& dst,
                                  uint16_t src_port, uint16_t dst_port,
                                  const std::vector<uint8_t>& payload) {
    const uint16_t udp_len = static_cast<uint16_t>(UdpHdrLen + payload.size());
    std::vector<uint8_t> pkt(PayloadOffset, 0);

    // IPv6 header
    std::memcpy(&pkt[0], flow.data(), flow.size());
    put_be16(&pkt[4], udp_len);                  // payload length, excludes this header
    pkt[6] = IPPROTO_UDP;                        // next header
    pkt[7] = 64;                                 // hop limit (never routed)
    std::memcpy(&pkt[SrcAddrOffset], src.data(), src.size());
    std::memcpy(&pkt[DstAddrOffset], dst.data(), dst.size());

    // UDP header (checksum stays 0 until computed below)
    put_be16(&pkt[UdpOffset], src_port);
    put_be16(&pkt[UdpOffset + 2], dst_port);
    put_be16(&pkt[UdpOffset + 4], udp_len);

    pkt.insert(pkt.end(), payload.begin(), payload.end());

    // The UDP checksum covers an IPv6 pseudo header (addresses, length, next
    // header) that is never sent, followed by the UDP header and payload.
    uint8_t pseudo[40] = {};
    std::memcpy(pseudo, src.data(), src.size());
    std::memcpy(pseudo + 16, dst.data(), dst.size());
    put_be16(pseudo + 34, udp_len);
    pseudo[39] = IPPROTO_UDP;

    uint32_t sum = checksum_add(pseudo, sizeof(pseudo));
    sum = checksum_add(&pkt[UdpOffset], udp_len, sum);
    uint16_t check = checksum_fold(sum);
    put_be16(&pkt[UdpOffset + 6], check ? check : 0xFFFF);  // 0 is not allowed over IPv6
    return pkt;
}
}  // namespace

bool GuardianPort::identify(const std::string& response) {
    return response.find("I am the guardian of the secret spell") != std::string::npos;
}

// The banner is a packet from the guardian to us. Our reply must swap both
// the IPv6 addresses and the UDP ports, otherwise it counts as an echo and
// the checksum (which covers the addresses) fails too.
bool GuardianPort::solve(PuzzleSession& session) {
    if (!session.secret_done) {
        std::cerr << "GuardianPort: needs Secret to run first\n";
        return false;
    }

    auto banner = send_and_receive(std::string("knock "));
    if (!is_wrapped_ipv6(banner)) {
        std::cerr << "GuardianPort: no wrapped IPv6 banner (size=" << banner.size() << ")\n";
        return false;
    }
    std::cerr << "GuardianPort banner: "
              << std::string(banner.begin() + PayloadOffset, banner.end()) << "\n";

    std::array<uint8_t, 4> flow;
    std::memcpy(flow.data(), banner.data(), flow.size());
    Addr their_src = addr_at(banner, SrcAddrOffset);
    Addr their_dst = addr_at(banner, DstAddrOffset);
    uint16_t their_sport = get_be16(&banner[UdpOffset]);
    uint16_t their_dport = get_be16(&banner[UdpOffset + 2]);

    auto packet = build_packet(flow, their_dst, their_src, their_dport, their_sport,
                               session.identity_bytes());

    // The reply must carry the guardian's original addresses; ignore others.
    constexpr int MaxAttempts = 3;
    std::vector<uint8_t> reply;
    for (int attempt = 0; attempt < MaxAttempts && reply.empty(); ++attempt) {
        auto candidate = send_and_receive(packet);
        if (is_wrapped_ipv6(candidate) &&
            addr_at(candidate, SrcAddrOffset) == their_src &&
            addr_at(candidate, DstAddrOffset) == their_dst) {
            reply = candidate;
        }
    }
    if (reply.empty()) {
        std::cerr << "GuardianPort: no reply with the original addresses\n";
        return false;
    }

    session.secret_phrase.assign(reply.begin() + PayloadOffset, reply.end());
    std::cerr << "GuardianPort revealed: " << session.secret_phrase << "\n";
    session.guardian_done = true;
    return true;
}
