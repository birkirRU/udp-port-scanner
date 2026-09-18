#pragma once
#include "port_sender.h"

class EvilPort : public PortSender {
public:
    EvilPort(const std::string& ip, int port);
    bool identify(const std::string& response) const override;
    bool solve(PuzzleSession& session) override;

};
