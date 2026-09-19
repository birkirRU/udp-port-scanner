#include "puzzle_solver.h"

#include <iostream>

// NOTE: you already have a puzzle_solver.cpp with content -- treat
// this file as a structural reference to merge in, not a drop-in
// replacement.

namespace {

// Trivial concrete PortSender used only to send the generic probe and
// read the response text. It isn't itself an identifiable puzzle
// module, so identify()/solve() are stubbed out and never called.
class Prober : public PortSender {
public:
    using PortSender::PortSender;
    bool identify(const std::string&) const override { return false; }
    bool solve(PuzzleSession&) override { return false; }
};

} // namespace

IdentifiedPorts identify_modules(const std::string& ip, const std::vector<int>& ports) {
    IdentifiedPorts result;

    for (int port : ports) {
        Prober prober(ip, port);
        if (!prober.open()) {
            std::cerr << "Port " << port << ": could not open socket, skipping\n";
            continue;
        }
        std::string response = prober.probe();
        std::string identify_name = "unknown";
        prober.close();

        // identify() is pure string matching -- no socket needed --
        // so a throwaway instance is enough to test each candidate
        // type against the response we already have.
        bool matched = false;
        if (!result.secret && SecretPort(ip, port).identify(response)) {
            result.secret = std::make_unique<SecretPort>(ip, port);
            matched = true;
            identify_name = "secret";
        } else if (!result.evil && EvilPort(ip, port).identify(response)) {
            result.evil = std::make_unique<EvilPort>(ip, port);
            matched = true;
            identify_name = "evil";
        } else if (!result.guardian && GuardianPort(ip, port).identify(response)) {
            result.guardian = std::make_unique<GuardianPort>(ip, port);
            matched = true;
            identify_name = "guardian";
        } else if (!result.dragon && DragonPort(ip, port).identify(response)) {
            result.dragon = std::make_unique<DragonPort>(ip, port);
            matched = true;
            identify_name = "dragon";
        }

        if (matched) {
            std::cerr << "Port " << port << " identified as " << identify_name << "\n";
        } else {
            std::cerr << "Port " << port << " did not match any known module "
                      << "(response: " << response << ")\n";
        }
    }

    return result;
}

int run_puzzle_chain(IdentifiedPorts& ports) {
    if (!ports.secret) {
        std::cerr << "Missing S.E.C.R.E.T. port, cannot continue\n";
        return 1;
    }

    PuzzleSession session;

    if (!ports.secret->solve(session)) {
        std::cerr << "S.E.C.R.E.T. handshake failed\n";
        return 1;
    }
    std::cerr << "S.E.C.R.E.T. done: group_id=" << static_cast<int>(session.group_id)
               << " sigil=" << session.sigil << "\n";

    if (ports.evil) {
        if (!ports.evil->solve(session)) {
            std::cerr << "Evil port failed, continuing without it\n";
        }
    }

    if (ports.guardian) {
        if (!ports.guardian->solve(session)) {
            std::cerr << "Guardian port failed, continuing without it\n";
        }
    }

    if (ports.dragon) {
        if (!ports.dragon->solve(session)) {
            std::cerr << "D.R.A.G.O.N. failed\n";
            return 1;
        }
        std::cerr << "D.R.A.G.O.N. sequence complete\n";
    }

    return 0;
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0]
                  << " <IP address> <port1> <port2> <port3> <port4>\n";
        return 1;
    }

    std::string ip = argv[1];
    std::vector<int> ports;
    for (int i = 2; i < 6; ++i) {
        ports.push_back(std::stoi(argv[i]));
    }

    auto identified = identify_modules(ip, ports);

    int found = (identified.secret ? 1 : 0) + (identified.evil ? 1 : 0) +
                (identified.guardian ? 1 : 0) + (identified.dragon ? 1 : 0);
    if (found != 4) {
        std::cerr << "Warning: only identified " << found
                  << "/4 ports -- continuing anyway\n";
    }

    return run_puzzle_chain(identified);
}