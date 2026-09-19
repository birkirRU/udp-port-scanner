#pragma once
#include "port_sender.h"

// Evil port: only answers packets whose IPv4 "evil bit" (the reserved top
// bit of the flags field) is set, which the kernel never does for us.
class EvilPort : public PuzzlePort {
public:
    using PuzzlePort::PuzzlePort;
    static bool identify(const std::string& response);
    bool solve(PuzzleSession& session) override;

    // Sends via a raw socket with the evil bit set (needs root).
    bool send(const std::vector<uint8_t>& payload) override;
};
