#pragma once
#include "port_sender.h"

class EvilPort : public PortSender {
public:
    EvilPort(const std::string& ip, int port);
    bool identify(const std::string& response) const override;
    bool solve(PuzzleSession& session) override;
    bool send(const std::vector<uint8_t>& payload) override;
    using PortSender::send;

    const std::string& reply_text() const { return reply_text_; }

private:
    std::string reply_text_;
};


