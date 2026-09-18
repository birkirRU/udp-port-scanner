#include "guardian_port.h"

#include <arpa/inet.h>
#include <cstring>
#include <iostream>

GuardianPort::GuardianPort(const std::string& ip, int port)
    : PortSender(ip, port) {}

bool GuardianPort::identify(const std::string& response) const {
    return false;
}

bool GuardianPort::solve(PuzzleSession& session) {
    if (!is_open() && !open()) return false;

    return false;
}