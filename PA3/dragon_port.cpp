#include "dragon_port.h"

#include <arpa/inet.h>
#include <cstring>
#include <iostream>

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

    // TODO: this is still just step 1 (handing over the port list).
    // Once we see what the knock-sequence reply actually looks like,
    // parse it here and drive the individual knocks against
    // secret_port_1/secret_port_2 (each knock needs group_id + sigil
    // + secret_phrase per the banner text).
    return true;
}