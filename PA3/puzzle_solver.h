#pragma once
#include <memory>
#include <string>
#include <vector>
#include "port_sender.h"
#include "secret_port.h"
#include "evil_port.h"
#include "guardian_port.h"
#include "dragon_port.h"

// The four given ports, each matched to its module. Typed pointers (rather
// than one vector of base pointers) so no casting is needed to reach them.
struct IdentifiedPorts {
    std::unique_ptr<SecretPort> secret;
    std::unique_ptr<EvilPort> evil;
    std::unique_ptr<GuardianPort> guardian;
    std::unique_ptr<DragonPort> dragon;
};

// Probes each port and matches it to the module whose identify() recognizes
// the banner. Argument order is irrelevant: a port's module is decided only
// by what it says.
IdentifiedPorts identify_modules(const std::string& ip, const std::vector<int>& ports);

// Runs Secret, Evil, Guardian, then Dragon, passing one PuzzleSession
// through so later modules can use what earlier ones found.
// Returns 0 on success, 1 on failure.
int run_puzzle_chain(IdentifiedPorts& ports);
