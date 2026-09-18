#pragma once
#include <string>
#include <memory>
#include <vector>
#include "port_sender.h"
#include "secret_port.h"
#include "evil_port.h"
#include "guardian_port.h"
#include "dragon_port.h"

// Result of probing the 4 given ports and matching each to a known
// module type. Concrete typed pointers instead of a homogeneous
// vector<unique_ptr<PortSender>> so run_puzzle_chain never needs
// dynamic_cast to get back to the concrete type it needs.
struct IdentifiedPorts {
    std::unique_ptr<SecretPort> secret;
    std::unique_ptr<EvilPort> evil;
    std::unique_ptr<GuardianPort> guardian;
    std::unique_ptr<DragonPort> dragon;
};

// Probes each port in `ports` and matches it to whichever module type
// recognizes its response text. The order of `ports` (i.e. CLI
// argument order) is irrelevant and never assumed -- which field of
// IdentifiedPorts a port ends up in depends entirely on identify(),
// not on its position in the vector.
IdentifiedPorts identify_modules(const std::string& ip, const std::vector<int>& ports);

// Runs the known, fixed invocation sequence -- Secret, then Evil,
// then Guardian, then Dragon -- threading a single PuzzleSession
// through all four so each module can read what earlier ones
// discovered without needing to know which module produced it.
int run_puzzle_chain(IdentifiedPorts& ports);