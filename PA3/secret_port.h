#pragma once
#include "port_sender.h"
#include <array>

class SecretPort : public PortSender {
public:
    SecretPort(const std::string& ip, int port);
    bool identify(const std::string& response) const override;
    std::string solve(const std::string& input = "") override;

    uint8_t group_id() const { return group_id_; }
    const std::array<uint8_t,4>& sigil() const { return sigil_; }

    const std::string& secret_text() const { return secret_text_; }

private:
    static const std::vector<std::string> kMemberNames;
    uint8_t group_id_ = 0;
    std::array<uint8_t,4> sigil_{};
    std::string secret_text_;
};