#pragma once
#include "port_sender.h"

class SecretPort : public PortSender {
public:
    SecretPort(const std::string& ip, int port);
    bool identify(const std::string& response) const override;
    bool solve(PuzzleSession& session) override;

    const std::string& secret_text() const { return secret_text_; }

private:
    static const std::vector<std::string> kMemberNames;
    std::string secret_text_;
};
