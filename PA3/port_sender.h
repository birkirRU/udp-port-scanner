#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <sys/socket.h> // AF_INET, SOCK_DGRAM defaults for open()

// Shared state threaded through every puzzle module's solve() call.
// Each module reads only the fields it needs and writes only the
// fields it discovers. This replaces passing a single opaque string
// down a fixed chain: the real dependency shape isn't a straight
// line (Dragon needs data from both Evil and Guardian, not just
// whichever module happened to run immediately before it).
struct PuzzleSession {
    // -- from SecretPort --
    uint8_t group_id = 0;
    uint32_t sigil = 0;
    bool secret_done = false;
    int hidden_port = 0;

    // -- from EvilPort (fields TBD -- full protocol not known yet) --
    bool evil_done = false;

    // -- from GuardianPort (fields TBD -- full protocol not known yet) --
    bool guardian_done = false;
    // TODO: at least one secret port number likely surfaces from
    // Evil and/or Guardian -- add fields here once confirmed.

    // -- consumed by DragonPort --
    int secret_port_1 = 0;
    int secret_port_2 = 0;
    std::string secret_phrase;

    // The 5-byte identity message: [group_id][sigil, 4 bytes big-endian].
    std::vector<uint8_t> identity_bytes() const {
        return {group_id,
                static_cast<uint8_t>(sigil >> 24), static_cast<uint8_t>(sigil >> 16),
                static_cast<uint8_t>(sigil >> 8),  static_cast<uint8_t>(sigil)};
    }

};

// PortSender
// -----------
// Generic socket wrapper shared by every puzzle module (secret_port,
// evil_port, gaurdian_port, dragon_port).
//
// Handles opening/closing a socket "connected" to a single remote
// endpoint, raw send/receive with a timeout, and a send-with-retry
// helper for the server's ~1/10 request drop rate.
//
// Each puzzle module derives from PortSender and implements:
//   - identify(): decide whether a given response belongs to it
//   - solve():    run its part of the puzzle, reading/writing
//                 whatever fields of PuzzleSession it needs
//
// puzzle_solver.cpp only talks to modules through this interface;
// it never opens sockets itself.
class PortSender {
public:
    PortSender(const std::string& remote_ip, int remote_port);
    virtual ~PortSender();

    // Opens a socket and connect()s it to remote_ip_:remote_port_.
    // Defaults to IPv4 UDP; pass AF_INET6 (with SOCK_DGRAM) for
    // GuardianPort, for example. Raw sockets (EvilPort) need their
    // own hand-built IP header on the way out and hand back full IP
    // headers on the way in, so that module builds its own raw
    // socket separately rather than reusing send()/receive() below.
    bool open(int domain = AF_INET, int type = SOCK_DGRAM, int protocol = 0);

    // Closes the socket if currently open. Safe to call multiple times.
    void close();

    bool is_open() const { return sockfd_ >= 0; }

    // Sends raw bytes / a string to the configured remote endpoint.
    // Returns true if the full payload was sent. No retry here --
    // see send_and_receive() below for that.
    virtual bool send(const std::vector<uint8_t>& data);
    bool send(const std::string& data);

    // Waits up to timeout_ms for a single datagram from the remote
    // endpoint. Returns the payload, or an empty vector on timeout
    // or error.
    std::vector<uint8_t> receive(int timeout_ms = 3000);

    int remote_port() const { return remote_port_; }
    const std::string& remote_ip() const { return remote_ip_; }

    // ---------------- interface for derived puzzle modules ----------------

    // Sends the generic >=6 byte probe message every puzzle port
    // expects before it reveals its instructions, with the same
    // retry behaviour as send_and_receive(). Returns the text
    // response, or "" if every attempt was dropped.
    virtual std::string probe();

    // Returns true if `response` (as returned by probe()) matches the
    // wording/keywords this module is looking for. Used by
    // identify_modules() to map an unknown port -> the right module.
    // Pure virtual: each port has its own, unrelated identifying text.
    virtual bool identify(const std::string& response) const = 0;

    // Runs this module's full puzzle exchange, reading/writing
    // whatever fields of `session` it needs. Number and order of
    // messages is entirely up to the derived class -- SecretPort
    // needs two round trips with a computed step in between,
    // DragonPort talks to ports other than its own, etc. Returns
    // true on success, false if the exchange could not be completed
    // (e.g. retries exhausted).
    virtual bool solve(PuzzleSession& session) = 0;

protected:
    // Sends `msg` and waits for a reply, retrying up to max_attempts
    // times (server drops roughly 1/10 requests). Returns the reply,
    // or an empty vector if every attempt timed out. Each attempt
    // resends `msg` unchanged -- if a derived class needs a fresh
    // message per attempt (e.g. a nonce that must match a later
    // step), build a manual loop instead of using this helper.
    std::vector<uint8_t> send_and_receive(const std::vector<uint8_t>& msg,
                                           int max_attempts = 3,
                                           int timeout_ms = 3000);
    std::vector<uint8_t> send_and_receive(const std::string& msg,
                                           int max_attempts = 3,
                                           int timeout_ms = 3000);

    std::string remote_ip_;
    int remote_port_;
    int sockfd_ = -1;
};