#include "secret_port.h"

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <regex>

const std::vector<std::string> SecretPort::kMemberNames = {
    "birkirsa24",
    "bjornth24",
};

SecretPort::SecretPort(const std::string& ip, int port)
    : PortSender(ip, port) {}

bool SecretPort::identify(const std::string& response) const {
    return response.find("Sacred Elder Cipher Relay") != std::string::npos; 
}

bool SecretPort::solve(PuzzleSession& session) {
    if (!is_open() && !open()) return false;

    uint32_t secret_number = 0x42569243;
    uint32_t secret_be = htonl(secret_number);
    uint8_t secret_bytes[4];
    std::memcpy(secret_bytes, &secret_be, 4);

    // Step 2: "S.E.C.R.E.T.:name1,name2," + secret number as the last 4 bytes
    std::string header = "S.E.C.R.E.T.:";
    for (const auto& name : kMemberNames) header += name + ",";
    std::vector<uint8_t> msg(header.begin(), header.end());
    msg.insert(msg.end(), secret_bytes, secret_bytes + 4);

    // Step 3: reply is [group_id][4-byte challenge]
    auto reply = send_and_receive(msg);
    if (reply.size() != 5) {
        std::cerr << "SecretPort: unexpected reply size " << reply.size() << "\n";
        return false;
    }

    // Step 4: sigil = challenge XOR secret, byte by byte
    uint8_t group_id = reply[0];
    uint8_t sigil_bytes[4];
    for (int i = 0; i < 4; ++i) sigil_bytes[i] = reply[1 + i] ^ secret_bytes[i];

    // Step 5: send [group_id][sigil], read the revealed secret
    std::vector<uint8_t> knock{group_id, sigil_bytes[0], sigil_bytes[1],
    sigil_bytes[2], sigil_bytes[3]};
    auto secret_reply = send_and_receive(knock);
    if (secret_reply.empty()) {
        std::cerr << "SecretPort: no secret revealed (timeout or sigil rejected)\n";
        return false;
    }
    secret_text_.assign(secret_reply.begin(), secret_reply.end());
    std::cerr << "SecretPort revealed: " << secret_text_ << "\n";

    // hidden port:\s*  -> Matches "hidden port:" and any spaces following it
    // (\d+)            -> Capture group 1: Matches and stores the port digits (ignoring the "8" earlier)
    std::regex pattern(R"(hidden port:\s*(\d+))"); 
    std::smatch match;
    std::string matched_port;

    if (std::regex_search(secret_text_, match, pattern)) {
        // match[1] contains only the specific captured digits inside the parentheses ("4033")
        matched_port = match[1].str();
    } else {
        std::cout << "Hidden port format not found." << std::endl;
    }

    // Only publish to the session once everything succeeded.
    uint32_t sigil_be;
    std::memcpy(&sigil_be, sigil_bytes, 4);
    session.group_id = group_id;
    session.sigil = ntohl(sigil_be);
    session.hidden_port = std::stoi(matched_port);
    session.secret_done = true;
    return true;
}
