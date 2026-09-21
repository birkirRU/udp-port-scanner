#include "secret_port.h"

#include <iostream>
#include <regex>

namespace {
const std::vector<std::string> MemberNames = {"birkirsa24", "bjornth24"};

// Our secret number 0x42569243, as the big endian bytes the server expects.
const uint8_t Secret[4] = {0x42, 0x56, 0x92, 0x43};
}  // namespace

bool SecretPort::identify(const std::string& response) {
    return response.find("Sacred Elder Cipher Relay") != std::string::npos;
}

// Exchange:
//   1. Send "S.E.C.R.E.T.:<names>," + secret. Reply is [group_id][challenge].
//   2. The sigil is the challenge XOR the secret, byte by byte.
//   3. Send [group_id][sigil]. Reply reveals the hidden port.
bool SecretPort::solve(PuzzleSession& session) {
    std::string header = "S.E.C.R.E.T.:";
    for (const auto& name : MemberNames) header += name + ",";
    std::vector<uint8_t> msg(header.begin(), header.end());
    msg.insert(msg.end(), Secret, Secret + 4);

    auto reply = send_and_receive(msg);
    if (reply.size() != 5) {
        std::cerr << "SecretPort: unexpected reply size " << reply.size() << "\n";
        return false;
    }

    PuzzleSession found;
    found.group_id = reply[0];
    for (int i = 0; i < 4; ++i) {
        found.sigil = (found.sigil << 8) | (reply[1 + i] ^ Secret[i]);
    }

    auto secret_reply = send_and_receive(found.identity_bytes());
    if (secret_reply.empty()) {
        std::cerr << "SecretPort: no reply (timeout or sigil rejected)\n";
        return false;
    }
    std::string text(secret_reply.begin(), secret_reply.end());
    std::cerr << "SecretPort revealed: " << text << "\n";

    // Capture the digits after "hidden port:"; the text has other numbers too.
    const std::regex pattern(R"(hidden port:\s*(\d+))");
    std::smatch match;
    if (!std::regex_search(text, match, pattern)) {
        std::cerr << "SecretPort: hidden port not found in reply\n";
        return false;
    }
    int port = to_port(match[1].str());
    if (port < 0) {
        std::cerr << "SecretPort: invalid hidden port " << match[1] << "\n";
        return false;
    }

    // Publish to the session only once everything succeeded.
    session.group_id = found.group_id;
    session.sigil = found.sigil;
    session.secret_port_1 = static_cast<uint16_t>(port);
    session.secret_done = true;
    return true;
}
