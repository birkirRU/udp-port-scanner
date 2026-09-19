#pragma once
#include "port_sender.h"
#include <array>
#include <cstdint>
#include <vector>

// Guardian of the secret spell port.
//
// Real transport is plain IPv4 UDP -- the "IPv6 packet" is entirely
// inside the UDP *payload*, hand-built and hand-parsed as:
//
//   [ 40-byte ip6_hdr ][ 8-byte udphdr ][ N-byte payload ]
//
// The guardian addresses its packet from its own point of view: its
// source is itself, its destination is you. A correct reply swaps
// BOTH the addresses and the ports -- their source becomes our
// destination, their destination becomes our source, same one layer
// down for ports -- because that's what makes it a reply rather than
// an echo. The UDP checksum is computed over a 40-byte IPv6
// pseudo-header that includes both addresses, so a wrong swap breaks
// the checksum too and the packet is dropped with no error either way.
//
// No raw socket is used or needed: we only ever write bytes into a
// buffer that becomes ordinary UDP application data. The kernel still
// builds the real (outer) IPv4/UDP headers via the base class's
// normal connected socket.
class GuardianPort : public PortSender {
public:
    GuardianPort(const std::string& ip, int port);

    bool identify(const std::string& response) const override;
    bool solve(PuzzleSession& session) override;

private:
    // The IPv6 header is always exactly 40 bytes -- no IHL field, no
    // options, nothing to compute, unlike IPv4.
    static constexpr size_t kIp6HdrLen = 40;
    static constexpr size_t kUdpHdrLen = 8;
    static constexpr size_t kSrcAddrOffset = 8;
    static constexpr size_t kDstAddrOffset = 24;
    static constexpr size_t kUdpOffset = kIp6HdrLen;                  // 40
    static constexpr size_t kPayloadOffset = kIp6HdrLen + kUdpHdrLen; // 48

    // Builds [ip6_hdr][udphdr][payload] using real struct ip6_hdr /
    // struct udphdr for the two headers, with the UDP checksum
    // computed over the (never-transmitted) 40-byte IPv6
    // pseudo-header + the UDP segment.
    static std::vector<uint8_t> build_packet(
        const std::array<uint8_t, 16>& src_addr,
        const std::array<uint8_t, 16>& dst_addr,
        uint16_t src_port_host,
        uint16_t dst_port_host,
        const std::vector<uint8_t>& payload);

    // RFC 1071 one's-complement checksum helpers. `sum16`'s `sum`
    // parameter lets callers carry a running total across several
    // buffers (pseudo-header, then UDP header+payload) without
    // concatenating them into one buffer first.
    static uint32_t sum16(const uint8_t* data, size_t len, uint32_t sum = 0);
    static uint16_t fold_checksum(uint32_t sum);
};