#include "puzzle_solver.h"
#include "secret_port.h"
#include "evil_port.h"
#include "gaurdian_port.h"
#include "dragon_port.h"

#include <iostream>

// NOTE: you already have a puzzle_solver.cpp with content -- treat this
// file as a structural reference to merge in, not a drop-in replacement.

std::vector<std::unique_ptr<PortSender>> identify_modules(
    const std::string& ip, const std::vector<int>& ports) {
    // TODO: Remove "delete" keyword, use instead unuiqe pointers.
   
    // TODO: change this function to do the following:
    // TODO: Have this function return (a globaly defined std::array an ordered) (we already know the order) std::array of unuiqe pairs per port; std::pair (solve() function pointer, transition() lambda)
    // What this does is that it simplifies the run_puzzle_chain function to only a single loop
    // which connects each "output = solve(input)" with the next ports input "newoutput = solve(transition(output))" via transition lambda.
    // TODO: find a smart way of deriving a consistant structure of the transition lambda function across the different ports
    // maybe each derived port sender defines their own transition function?

    std::vector<std::unique_ptr<PortSender>> modules;

    for (int port : ports) {
        // Probe with a throwaway instance of each type until one
        // recognizes the response, since we don't know in advance
        // which port is which.
        PortSender* candidates[] = {
            new SecretPort(ip, port),
            new EvilPort(ip, port),
            new GuardianPort(ip, port),
            new DragonPort(ip, port),
        };

        std::unique_ptr<PortSender> matched;
        std::string response;
        bool probed = false;

        for (auto* candidate : candidates) {
            if (!probed) {
                if (!candidate->open()) { delete candidate; continue; }
                response = candidate->probe();
                probed = true;
            }
            if (!matched && candidate->identify(response)) {
                matched.reset(candidate);
            } else {
                delete candidate;
            }
        }

        if (matched) {
            std::cerr << "Port " << port << " identified as a known module\n";
            modules.push_back(std::move(matched));
        } else {
            std::cerr << "Port " << port << " did not match any known module "
                      << "(response: " << response << ")\n";
        }
    }

    return modules;
}

int run_puzzle_chain(const std::string& ip,
                      std::vector<std::unique_ptr<PortSender>>& modules) {
    (void)ip;
    // TODO: Remove the need for dynamic casting.

    SecretPort* secret = nullptr;
    EvilPort* evil = nullptr;
    GuardianPort* guardian = nullptr;
    DragonPort* dragon = nullptr;

    for (auto& m : modules) {
        if (auto* p = dynamic_cast<SecretPort*>(m.get())) secret = p;
        else if (auto* p = dynamic_cast<EvilPort*>(m.get())) evil = p;
        else if (auto* p = dynamic_cast<GuardianPort*>(m.get())) guardian = p;
        else if (auto* p = dynamic_cast<DragonPort*>(m.get())) dragon = p;
    }

    if (!secret) {
        std::cerr << "Missing S.E.C.R.E.T. port, cannot continue\n";
        return 1;
    }

    std::string state = secret->solve();
    if (state.empty()) {
        std::cerr << "S.E.C.R.E.T. handshake failed\n";
        return 1;
    }
    std::cerr << "S.E.C.R.E.T. state: " << state << "\n";

    if (evil) {
        state = evil->solve(state);
        std::cerr << "Evil state: " << state << "\n";
    }

    if (guardian) {
        state = guardian->solve(state);
        std::cerr << "Guardian state: " << state << "\n";
    }

    if (dragon) {
        // TODO: state must be reshaped into
        // "<group_id>,<sigil>,<secret_port_1>,<secret_port_2>,<phrase>"
        // before reaching here, once Evil/Guardian's output format is
        // finalized -- see dragon_port.cpp for the expected encoding.
        state = dragon->solve(state);
        std::cerr << "D.R.A.G.O.N. final response: " << state << "\n";
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

    auto modules = identify_modules(ip, ports);
    if (modules.size() != 4) {
        std::cerr << "Warning: only identified " << modules.size()
                  << "/4 ports -- continuing anyway\n";
    }

    return run_puzzle_chain(ip, modules);
}
