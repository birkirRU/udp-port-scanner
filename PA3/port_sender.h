#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Results shared between the puzzle modules. Each module reads what earlier
// ones found and records what it discovered, so Dragon can combine the
// output of Secret, Evil and Guardian.
struct PuzzleSession {
    // From Secret
    bool secret_done = false;
    uint8_t group_id = 0;
    uint32_t sigil = 0;
    uint16_t secret_port_1 = 0;

    // From Evil
    bool evil_done = false;
    uint16_t secret_port_2 = 0;

    // From Guardian
    bool guardian_done = false;
    std::string secret_phrase;

    // The 5-byte message [group_id][sigil, big-endian] that Evil and
    // Guardian expect (and that Secret's second step sends).
    std::vector<uint8_t> identity_bytes() const {
        return {group_id,
                static_cast<uint8_t>(sigil >> 24), static_cast<uint8_t>(sigil >> 16),
                static_cast<uint8_t>(sigil >> 8),  static_cast<uint8_t>(sigil)};
    }
};

// ---- helpers shared by the modules ----

// Converts text to a port number (1-65535), or returns -1 if it isn't one.
int to_port(const std::string& text);

// RFC 1071 internet checksum for hand-built headers. Feed buffers to
// checksum_add (carrying the running sum across several buffers), then
// finish with checksum_fold.
uint32_t checksum_add(const uint8_t* data, size_t len, uint32_t sum = 0);
uint16_t checksum_fold(uint32_t sum);

// 16-bit big-endian (network order) access to a byte buffer.
void put_be16(uint8_t* p, uint16_t v);
uint16_t get_be16(const uint8_t* p);

// PortSender: UDP socket "connected" to one remote port, shared by every
// module. Sends/receives raw bytes and retries because the server drops
// roughly 1 in 10 requests. The socket is opened lazily on first send.
class PortSender {
public:
    PortSender(const std::string& remote_ip, int remote_port);
    virtual ~PortSender();

    bool open();
    void close();
    bool is_open() const { return sockfd_ >= 0; }

    // Virtual so EvilPort can send with the evil bit set.
    virtual bool send(const std::vector<uint8_t>& data);

    // Waits up to timeout_ms for one datagram; empty on timeout or error.
    std::vector<uint8_t> receive(int timeout_ms = 3000);

    // Sends `msg` and waits for a reply, resending the same message up to
    // max_attempts times. Empty if every attempt timed out.
    std::vector<uint8_t> send_and_receive(const std::vector<uint8_t>& msg,
                                          int max_attempts = 3,
                                          int timeout_ms = 3000);
    std::vector<uint8_t> send_and_receive(const std::string& msg,
                                          int max_attempts = 3,
                                          int timeout_ms = 3000);

    // Sends the generic probe every puzzle port answers with its banner
    // (any message of 6+ bytes works). Returns the text, or "" if dropped.
    std::string probe();

    const std::string& remote_ip() const { return remote_ip_; }
    int remote_port() const { return remote_port_; }

protected:
    std::string remote_ip_;
    int remote_port_;
    int sockfd_ = -1;
};

// Base class of the four puzzle modules. Each one provides:
//   static bool identify(response) - does this probe banner belong to it?
//   solve(session)                 - run its part of the puzzle
class PuzzlePort : public PortSender {
public:
    using PortSender::PortSender;
    virtual bool solve(PuzzleSession& session) = 0;
};
