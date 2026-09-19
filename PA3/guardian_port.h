
#pragma once
#include "port_sender.h"

class GuardianPort : public PortSender {
public:
    GuardianPort(const std::string& ip, int port);
    bool identify(const std::string& response) const override;
    bool solve(PuzzleSession& session) override;

    const std::string& reply_text() const { return reply_text_; }

private:
    std::string reply_text_;
};
