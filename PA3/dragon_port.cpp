#include "dragon_port.h"

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <sstream>

namespace {
class Knocker : public PortSender {
public:
    using PortSender::PortSender;
    bool identify(const std::string&) const override { return false; }
    bool solve(PuzzleSession&) override { return false; }
    std::vector<uint8_t> ask(const std::vector<uint8_t>& m) { return send_and_receive(m, 10); }
};
}

DragonPort::DragonPort(const std::string& ip, int port)
    : PortSender(ip, port) {}

bool DragonPort::identify(const std::string& response) const {
    return response.find("I am D.R.A.G.O.N.") != std::string::npos;
}

bool DragonPort::solve(PuzzleSession& session) {
    if (!is_open() && !open()) return false;

    // Dragon's banner asks for "a list of secret ports, separated by
    // commas" -- that's plain ASCII text, e.g. "4033,4012", not the
    // ports packed as raw 16-bit binary values the way S.E.C.R.E.T.'s
    // XOR step wanted. Sending binary here is almost certainly why
    // nothing came back: six bytes containing two unprintable control
    // characters doesn't look like a request its parser recognizes.
    std::string msg_text = std::to_string(session.secret_port_1) + "," +
                            std::to_string(session.secret_port_2);
    std::cerr << "DragonPort: sending '" << msg_text << "'\n";

    auto reply = send_and_receive(std::vector<uint8_t>(msg_text.begin(), msg_text.end()));
    if (reply.empty()) {
        std::cerr << "DragonPort: no reply after retries\n";
        return false;
    }

    std::string reply_text(reply.begin(), reply.end());
    std::cerr << "DragonPort revealed: " << reply_text << "\n";

    // Parse "4012,4033,..." into an ordered list of ports.
    std::vector<int> knocks;
    std::stringstream ss(reply_text);
    std::string tok;
    while (std::getline(ss, tok, ',')) knocks.push_back(std::stoi(tok));

    // The guardian's reply wraps the phrase in quotes: take just that part.
    std::string phrase = session.secret_phrase;
    size_t a = phrase.find('"'), b = phrase.rfind('"');
    if (a != std::string::npos && b > a) phrase = phrase.substr(a + 1, b - a - 1);

    // Each knock: [group_id][sigil] + phrase.
    auto knock_msg = session.identity_bytes();
    knock_msg.insert(knock_msg.end(), phrase.begin(), phrase.end());


    for (int port : knocks) {
        Knocker knocker(remote_ip(), port);
        auto r = knocker.ask(knock_msg);
        if (r.empty()) {
            std::cerr << "DragonPort: no reply from knock on " << port << "\n";
            return false;
        }
        std::cerr << "Knock " << port << " -> " << std::string(r.begin(), r.end()) << "\n";
    }
    return true;

}