#pragma once
#include "port_sender.h"

// D.R.A.G.O.N. port: given the two secret ports it returns a sequence of
// ports to knock on, using the phrase the Guardian revealed.
class DragonPort : public PuzzlePort {
public:
    using PuzzlePort::PuzzlePort;
    static bool identify(const std::string& response);
    bool solve(PuzzleSession& session) override;
};
