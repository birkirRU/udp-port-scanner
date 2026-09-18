#include "secret_port.h"

#include <arpa/inet.h>
#include <cstring>
#include <sstream>
#include <iostream>
#include <iomanip>

const std::vector<std::string> SecretPort::kMemberNames = {
    "birkirsa24",
    "bjornth24",
};

SecretPort::SecretPort(const std::string& ip, int port)
    : PortSender(ip, port) {}

bool SecretPort::identify(const std::string& response) const {
    return response.find("Sacred Elder Cipher Relay") != std::string::npos; 
}

std::string SecretPort::solve(const std::string& /*input*/) {
    if (!is_open() && !open()) return "";

    // TODO: pick your own secret number (any 32-bit value works).
    uint32_t secret_number = 0x42569243;

    // Step 2: build "S.E.C.R.E.T.:name1,name2,...," + secret_number as
    // the final 4 raw bytes (network byte order).
    std::ostringstream oss;
    oss << "S.E.C.R.E.T.:";
    for (size_t i = 0; i < kMemberNames.size(); ++i) {
        if (i) oss << ", ";
        oss << kMemberNames[i];
    }
    std::string header = oss.str();

    std::vector<uint8_t> msg(header.begin(), header.end());
    uint32_t secret_be = htonl(secret_number);
    uint8_t secret_bytes[4];
    std::memcpy(secret_bytes, &secret_be, 4);
    msg.insert(msg.end(), secret_bytes, secret_bytes + 4);

    if (!send(msg)) {
        std::cerr << "SecretPort: failed to send step 2 message\n";
        return "";
    }

    // Step 3: expect a 5-byte reply: [group_id][4-byte challenge].
    auto reply = receive();
    if (reply.size() != 5) {
        std::cerr << "SecretPort: unexpected reply size " << reply.size() << "\n";
        return "";
    }
    group_id_ = reply[0];
    for (int i = 0; i < 4; ++i) {
        sigil_[i] = reply[1 + i] ^ secret_bytes[i];
    }

    // Step 5: send 5 bytes: [group_id][4-byte sigil].
    std::vector<uint8_t> knock;
    knock.push_back(group_id_);
    knock.insert(knock.end(), sigil_.begin(), sigil_.end());

    if (!send(knock)) {
        std::cerr << "SecretPort: failed to send step 5 knock\n";
        return "";
    }

    // Step 6: read the revealed secret (informational / for the README).
    auto secret_reply = receive();
    std::string secret_text(secret_reply.begin(), secret_reply.end());
    std::cerr << "SecretPort revealed: " << secret_text << "\n";

    std::ostringstream out;
    out << static_cast<int>(group_id_) << ",";
    for (uint8_t byte : sigil_) {
        out << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return out.str();

}