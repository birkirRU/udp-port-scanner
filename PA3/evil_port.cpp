#include "evil_port.h"

#include <arpa/inet.h>
#include <cstring>
#include <iostream>

EvilPort::EvilPort(const std::string& ip, int port)
    : PortSender(ip, port) {}

bool EvilPort::identify(const std::string& response) const {
    return false;
}

bool EvilPort::solve(PuzzleSession& session) {
    if (!is_open() && !open()) return false;

    return false;
}
