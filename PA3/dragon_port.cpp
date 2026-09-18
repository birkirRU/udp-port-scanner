#include "dragon_port.h"

#include <arpa/inet.h>
#include <cstring>
#include <iostream>

DragonPort::DragonPort(const std::string& ip, int port)
    : PortSender(ip, port) {}

bool DragonPort::identify(const std::string& response) const {
    return false;
}

bool DragonPort::solve(PuzzleSession& session) {
    if (!is_open() && !open()) return false;

    return false;
}
