#include "guardian_port.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>

GuardianPort::GuardianPort(const std::string& ip, int port)
    : PortSender(ip, port) {}

bool GuardianPort::identify(const std::string& response) const {
    return response.find("I am the guardian of the secret spell") != std::string::npos;
}

uint32_t GuardianPort::sum16(const uint8_t* data, size_t len, uint32_t sum) {
    size_t i = 0;
    for (; i + 1 < len; i += 2) {
        sum += (static_cast<uint32_t>(data[i]) << 8) | data[i + 1];
    }
    if (i < len) {
        sum += static_cast<uint32_t>(data[i]) << 8; // odd trailing byte, padded with 0
    }
    return sum;
}

uint16_t GuardianPort::fold_checksum(uint32_t sum) {
    while (sum >> 16) {
        sum = (sum & 0xffffu) + (sum >> 16); // end-around carry
    }
    uint16_t c = static_cast<uint16_t>(~sum);
    return c ? c : 0xffff; // IPv6/UDP forbids a transmitted checksum of 0
}

std::vector<uint8_t> GuardianPort::build_packet(
    const std::array<uint8_t, 4>& flow_field_raw,
    const std::array<uint8_t, 16>& src_addr,
    const std::array<uint8_t, 16>& dst_addr,
    uint16_t src_port_host,
    uint16_t dst_port_host,
    const std::vector<uint8_t>& payload) {

    static_assert(sizeof(ip6_hdr) == kIp6HdrLen,
                  "ip6_hdr isn't packed to 40 bytes on this platform");
    static_assert(sizeof(udphdr) == kUdpHdrLen,
                  "udphdr isn't packed to 8 bytes on this platform");

    const uint16_t udp_len = static_cast<uint16_t>(kUdpHdrLen + payload.size());

    // ---- real ip6_hdr, filled by hand -- never actually routed ----
    ip6_hdr v6{};
    // Copy the version/traffic-class/flow-label bytes verbatim from
    // the server's own packet rather than computing them -- the flow
    // label is checked and must match what the server issued for
    // this exchange.
    std::memcpy(&v6.ip6_flow, flow_field_raw.data(), 4);
    v6.ip6_plen = htons(udp_len);       // payload length EXCLUDES these 40 bytes
    v6.ip6_nxt = IPPROTO_UDP;           // 17
    v6.ip6_hlim = 64;                   // arbitrary -- packet is never actually routed
    std::memcpy(&v6.ip6_src, src_addr.data(), 16);
    std::memcpy(&v6.ip6_dst, dst_addr.data(), 16);

    // ---- real udphdr; checksum field must be zero while summing ----
    udphdr udp{};
    udp.source = htons(src_port_host);
    udp.dest = htons(dst_port_host);
    udp.len = htons(udp_len);
    udp.check = 0;

    // ---- IPv6 pseudo-header for the checksum: 40 bytes, never sent ----
    // layout: src(16) | dst(16) | upper-layer length as 4 bytes | zero(3) | next header(1)
    // NOTE: this is NOT the 12-byte IPv4 pseudo-header layout.
    uint8_t pseudo[40] = {};
    std::memcpy(pseudo, src_addr.data(), 16);
    std::memcpy(pseudo + 16, dst_addr.data(), 16);
    pseudo[34] = static_cast<uint8_t>(udp_len >> 8);
    pseudo[35] = static_cast<uint8_t>(udp_len);
    pseudo[39] = IPPROTO_UDP;

    uint32_t sum = sum16(pseudo, sizeof(pseudo));
    sum = sum16(reinterpret_cast<const uint8_t*>(&udp), sizeof(udp), sum);
    sum = sum16(payload.data(), payload.size(), sum);
    udp.check = htons(fold_checksum(sum));

    std::vector<uint8_t> out(kIp6HdrLen + kUdpHdrLen + payload.size());
    std::memcpy(out.data(), &v6, kIp6HdrLen);
    std::memcpy(out.data() + kIp6HdrLen, &udp, kUdpHdrLen);
    std::memcpy(out.data() + kPayloadOffset, payload.data(), payload.size());
    return out;
}

bool GuardianPort::solve(PuzzleSession& session) {
    if (!session.secret_done) {
        std::cerr << "GuardianPort: SecretPort must run first (need group_id/sigil)\n";
        return false;
    }
    if (!is_open() && !open()) return false;

    // 1. Probe (>= 6 bytes) to get the wrapped instructions.
    auto banner_bytes = send_and_receive(std::string("knock "));
    if (banner_bytes.empty()) {
        std::cerr << "GuardianPort: no banner received\n";
        return false;
    }

    // 2. Verify the shape before trusting any offset into it.
    if (banner_bytes.size() < kPayloadOffset || (banner_bytes[0] >> 4) != 6) {
        std::cerr << "GuardianPort: response isn't a wrapped IPv6 packet "
                  << "(size=" << banner_bytes.size() << ")\n";
        return false;
    }

    std::string banner_text(banner_bytes.begin() + kPayloadOffset, banner_bytes.end());
    std::cerr << "GuardianPort banner: " << banner_text << "\n";

    // 3. Swap: their source becomes our destination, their
    //    destination becomes our source. memcpy, not a cast through
    //    a misaligned pointer -- buf+8/buf+24 aren't guaranteed
    //    2- or 4-byte aligned, and reinterpret-casting would also be
    //    a strict-aliasing violation that can misbehave under -O2.
    std::array<uint8_t, 4> flow_field_raw{}; // echoed verbatim, not recomputed
    std::memcpy(flow_field_raw.data(), banner_bytes.data(), 4);

    std::array<uint8_t, 16> their_src{}, their_dst{};
    std::memcpy(their_src.data(), banner_bytes.data() + kSrcAddrOffset, 16);
    std::memcpy(their_dst.data(), banner_bytes.data() + kDstAddrOffset, 16);

    uint16_t their_sport_n, their_dport_n;
    std::memcpy(&their_sport_n, banner_bytes.data() + kUdpOffset, 2);
    std::memcpy(&their_dport_n, banner_bytes.data() + kUdpOffset + 2, 2);

    // Values from the buffer are in network order; convert to host
    // order here so build_packet() (which converts back) always
    // receives host-order ports, keeping one consistent rule:
    // variables hold host order, buffers hold network order.
    uint16_t our_src_port = ntohs(their_dport_n); // we speak from the port they addressed
    uint16_t our_dst_port = ntohs(their_sport_n); // we speak to the port they spoke from

    // 4. Payload: the identical 5 bytes S.E.C.R.E.T. wanted.
    auto payload = session.identity_bytes();

    // 5. Build and send on the ordinary connected UDP socket -- the
    //    kernel still builds the real IPv4/UDP headers; this is just
    //    53 bytes of application data.
    auto packet = build_packet(flow_field_raw, their_dst, their_src, our_src_port, our_dst_port, payload);

    std::vector<uint8_t> reply_bytes;
    constexpr int kMaxReplyAttempts = 3;
    for (int attempt = 0; attempt < kMaxReplyAttempts; ++attempt) {
        reply_bytes = send_and_receive(packet);
        if (reply_bytes.size() < kPayloadOffset || (reply_bytes[0] >> 4) != 6) {
            continue;
        }

        std::array<uint8_t, 16> reply_src{}, reply_dst{};
        std::memcpy(reply_src.data(), reply_bytes.data() + kSrcAddrOffset, 16);
        std::memcpy(reply_dst.data(), reply_bytes.data() + kDstAddrOffset, 16);
        if (reply_src == their_src && reply_dst == their_dst) {
            break;
        }
        reply_bytes.clear();
    }

    if (reply_bytes.empty()) {
        std::cerr << "GuardianPort: no reply with the original source/destination addresses\n";
        return false;
    }

    std::string reply_text(reply_bytes.begin() + kPayloadOffset, reply_bytes.end());
    session.secret_phrase = reply_text;
    std::cerr << "GuardianPort revealed: " << reply_text << "\n";

    session.guardian_done = true;
    return true;
}