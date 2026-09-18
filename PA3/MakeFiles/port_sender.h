#pragma once
#include <string>
#include <vector>
#include <cstdint>

// PortSender
// -----------
// Generic UDP socket wrapper shared by every puzzle module
// (secret_port, evil_port, gaurdian_port, dragon_port).
//
// Handles opening/closing a UDP socket "connected" to a single
// remote ip:port, and raw send/receive of bytes with a timeout.
//
// Each puzzle module derives from PortSender and implements:
//   - identify(): decide whether a given response belongs to it
//   - solve():    run its part of the puzzle and return whatever
//                 downstream modules need (group id, sigil, secret
//                 ports, phrase, etc.)
//
// puzzle_solver.cpp only talks to modules through this interface;
// it never opens sockets itself.
class PortSender {
public:
    PortSender(const std::string& ip, int port);
    virtual ~PortSender();

    // Opens a UDP socket and connect()s it to ip_:port_ so we can use
    // send()/recv() instead of sendto()/recvfrom(). Returns true on success.
    bool open();

    // Closes the socket if currently open. Safe to call multiple times.
    void close();

    bool is_open() const { return sockfd_ >= 0; }

    // Sends raw bytes / a string to the configured remote endpoint.
    // Returns true if the full payload was sent.
    bool send(const std::vector<uint8_t>& data);
    bool send(const std::string& data);

    // Waits up to timeout_ms for a UDP datagram from the remote
    // endpoint. Returns the payload, or an empty vector on timeout
    // or error (check errno / is_open() to disambiguate if needed).
    std::vector<uint8_t> receive(int timeout_ms = 3000);

    int port() const { return port_; }
    const std::string& ip() const { return ip_; }

    // ---------------- interface for derived puzzle modules ----------------

    // Sends the generic >=6 byte probe message every puzzle port expects
    // before it will reveal its instructions. Returns the text response
    // (empty string on timeout). Default implementation sends a filler
    // string of spaces; override if a module needs something specific.
    virtual std::string probe();

    // Returns true if `response` (as returned by probe()) matches the
    // wording/keywords this module is looking for. Used by
    // puzzle_solver.cpp to map an unknown port -> the right module.
    virtual bool identify(const std::string& response) const = 0;

    // Runs this module's full puzzle exchange. `input` carries whatever
    // state earlier modules produced (e.g. "<group_id>,<sigil>"); the
    // return value is passed on to later modules by puzzle_solver.cpp.
    // The exact encoding of input/output strings is up to the group,
    // just keep it consistent across modules.
    virtual std::string solve(const std::string& input = "") = 0;

protected:
    std::string ip_;
    int port_;
    int sockfd_ = -1;
};