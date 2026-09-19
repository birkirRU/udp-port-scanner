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
    std::vector<uint8_t> msg;

    uint16_t port1 = htons(session.secret_port_1);
    uint16_t port2 = htons(session.secret_port_2);

    msg.resize(sizeof(port1) + 2 + sizeof(port2));

    std::memcpy(msg.data(), &port1, sizeof(port1));
    msg[sizeof(port1)] = ',';
    msg[sizeof(port1)] = ' ';
    std::memcpy(msg.data() + sizeof(port1) + 2, &port2, sizeof(port2));

    auto reply = send_and_receive(msg);


    return false;
}
