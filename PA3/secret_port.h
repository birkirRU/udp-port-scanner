#pragma once
#include "port_sender.h"

// S.E.C.R.E.T. port: we prove who we are with a shared secret number, and
// get back our group id, a sigil, and the number of a hidden port.
class SecretPort : public PuzzlePort {
public:
    using PuzzlePort::PuzzlePort;
    static bool identify(const std::string& response);
    bool solve(PuzzleSession& session) override;
};
