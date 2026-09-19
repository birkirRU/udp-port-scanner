#include "dragon_port.h"

#include <iostream>
#include <sstream>

bool DragonPort::identify(const std::string& response) {
    return response.find("I am D.R.A.G.O.N.") != std::string::npos;
}

// Exchange:
//   1. Send the two secret ports as ASCII text ("4033,4012"). The reply is
//      the ordered list of ports to knock on.
//   2. Knock on each port with [group_id][sigil] + the Guardian's phrase.
bool DragonPort::solve(PuzzleSession& session) {
    if (!session.secret_done || !session.evil_done || !session.guardian_done) {
        std::cerr << "DragonPort: needs Secret, Evil and Guardian to run first\n";
        return false;
    }

    auto reply = send_and_receive(std::to_string(session.secret_port_1) + "," +
                                  std::to_string(session.secret_port_2));
    if (reply.empty()) {
        std::cerr << "DragonPort: no reply after retries\n";
        return false;
    }
    std::string text(reply.begin(), reply.end());
    std::cerr << "DragonPort revealed: " << text << "\n";

    std::vector<int> knocks;
    std::stringstream ss(text);
    std::string token;
    while (std::getline(ss, token, ',')) {
        int port = to_port(token);
        if (port < 0) {
            std::cerr << "DragonPort: invalid port in reply: " << token << "\n";
            return false;
        }
        knocks.push_back(port);
    }

    // The Guardian's reply wraps the phrase in quotes; use only the part inside.
    std::string phrase = session.secret_phrase;
    size_t open_quote = phrase.find('"'), close_quote = phrase.rfind('"');
    if (open_quote != std::string::npos && close_quote > open_quote) {
        phrase = phrase.substr(open_quote + 1, close_quote - open_quote - 1);
    }

    auto knock_msg = session.identity_bytes();
    knock_msg.insert(knock_msg.end(), phrase.begin(), phrase.end());

    constexpr int kKnockAttempts = 10;
    for (int port : knocks) {
        PortSender knocker(remote_ip(), port);
        auto response = knocker.send_and_receive(knock_msg, kKnockAttempts);
        if (response.empty()) {
            std::cerr << "DragonPort: no reply from knock on " << port << "\n";
            return false;
        }
        std::cerr << "Knock " << port << " -> "
                  << std::string(response.begin(), response.end()) << "\n";
    }
    return true;
}
