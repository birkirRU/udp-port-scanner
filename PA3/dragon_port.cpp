#include "dragon_port.h"

#include <sstream>
#include <iostream>

DragonPort::DragonPort(const std::string& ip, int port)
    : PortSender(ip, port) {}

bool DragonPort::identify(const std::string& response) const {
    return response.find("D.R.A.G.O.N.") != std::string::npos ||
           response.find("Dwemer") != std::string::npos;
}

std::vector<DragonPort::Knock> DragonPort::request_knock_sequence(
    int secret_port_1, int secret_port_2) {
    std::vector<Knock> knocks;

    if (!is_open() && !open()) return knocks;

    std::ostringstream oss;
    oss << secret_port_1 << "," << secret_port_2;
    if (!send(oss.str())) return knocks;

    auto reply = receive();
    std::string text(reply.begin(), reply.end());
    std::cerr << "DragonPort sequence reply: " << text << "\n";

    // TODO: parse `text` into an ordered list of {port, message} knocks
    // once you've seen the actual reply format. Placeholder below.
    (void)knocks;
    return knocks;
}

std::string DragonPort::solve(const std::string& input) {
    // Parse "<group_id>,<sigil>,<secret_port_1>,<secret_port_2>,<phrase>"
    std::istringstream iss(input);
    std::string gid_str, sigil_str, sp1_str, sp2_str, phrase;
    if (!std::getline(iss, gid_str, ',') ||
        !std::getline(iss, sigil_str, ',') ||
        !std::getline(iss, sp1_str, ',') ||
        !std::getline(iss, sp2_str, ',') ||
        !std::getline(iss, phrase)) {
        std::cerr << "DragonPort: bad input '" << input << "'\n";
        return "";
    }

    int secret_port_1 = std::stoi(sp1_str);
    int secret_port_2 = std::stoi(sp2_str);

    auto knocks = request_knock_sequence(secret_port_1, secret_port_2);

    // Trivial concrete PortSender just for firing off a one-off knock
    // and reading the reply -- it isn't itself an identifiable puzzle
    // module, so identify()/solve() are stubbed out.
    class Knocker : public PortSender {
    public:
        using PortSender::PortSender;
        bool identify(const std::string&) const override { return false; }
        std::string solve(const std::string&) override { return ""; }
    };

    std::string final_response;
    for (const auto& knock : knocks) {
        Knocker knocker(ip_, knock.port);
        if (!knocker.open()) continue;
        // TODO: build the actual knock payload -- likely group id +
        // sigil + phrase combined per the port's own instructions,
        // not just knock.message verbatim.
        knocker.send(knock.message);
        auto reply = knocker.receive();
        final_response.assign(reply.begin(), reply.end());
        std::cerr << "Knock on port " << knock.port << " -> " << final_response << "\n";
    }

    return final_response;
}