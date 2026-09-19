#include "puzzle_solver.h"

#include <iostream>

namespace {
// Gives `slot` a new module for `port` if it is still empty and the
// banner matches that module type.
template <class T>
bool claim(std::unique_ptr<T>& slot, const char* name, const std::string& ip, int port,
           const std::string& response) {
    if (slot || !T::identify(response)) return false;
    slot = std::make_unique<T>(ip, port);
    std::cerr << "Port " << port << " identified as " << name << "\n";
    return true;
}

// Runs one module and logs the outcome. False if the port wasn't found
// or its exchange failed.
template <class T>
bool run_module(const std::unique_ptr<T>& module, const char* name, PuzzleSession& session) {
    if (!module) {
        std::cerr << name << ": port not identified\n";
        return false;
    }
    if (!module->solve(session)) {
        std::cerr << name << ": failed\n";
        return false;
    }
    std::cerr << name << ": done\n";
    return true;
}
}  // namespace

IdentifiedPorts identify_modules(const std::string& ip, const std::vector<int>& ports) {
    IdentifiedPorts result;

    for (int port : ports) {
        std::string response = PortSender(ip, port).probe();

        bool matched = claim(result.secret, "Secret", ip, port, response) ||
                       claim(result.evil, "Evil", ip, port, response) ||
                       claim(result.guardian, "Guardian", ip, port, response) ||
                       claim(result.dragon, "Dragon", ip, port, response);
        if (!matched) {
            std::cerr << "Port " << port << " did not match any module (response: "
                      << response << ")\n";
        }
    }
    return result;
}

int run_puzzle_chain(IdentifiedPorts& ports) {
    PuzzleSession session;

    // Secret is required by everything else. Evil and Guardian failures are
    // reported here and then caught by Dragon's own precondition check.
    if (!run_module(ports.secret, "Secret", session)) return 1;
    run_module(ports.evil, "Evil", session);
    run_module(ports.guardian, "Guardian", session);
    return run_module(ports.dragon, "Dragon", session) ? 0 : 1;
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << " <IP address> <port1> <port2> <port3> <port4>\n";
        return 1;
    }

    std::string ip = argv[1];
    std::vector<int> ports;
    for (int i = 2; i < 6; ++i) {
        int port = to_port(argv[i]);
        if (port < 0) {
            std::cerr << "Invalid port: " << argv[i] << "\n";
            return 1;
        }
        ports.push_back(port);
    }

    auto identified = identify_modules(ip, ports);
    return run_puzzle_chain(identified);
}
