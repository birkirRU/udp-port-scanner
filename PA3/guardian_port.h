#pragma once
#include "port_sender.h"

// Guardian port: speaks a hand-built IPv6+UDP packet carried inside the
// payload of an ordinary IPv4 UDP datagram. To be accepted, our reply must
// look like a reply to the packet it sent us (see guardian_port.cpp).
class GuardianPort : public PuzzlePort {
public:
    using PuzzlePort::PuzzlePort;
    static bool identify(const std::string& response);
    bool solve(PuzzleSession& session) override;
};
