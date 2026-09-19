# PA3 - UDP puzzle solver

## Development environment

Developed on macOS (Apple Silicon / ARM) and also built and run on Linux.
Requires `g++` with C++17 support.

## Compile and run

```
make
```

builds two programs: `scanner` and `puzzle_solver`.

1. Find the four puzzle ports:
   ```
   ./scanner <IP address> <low port> <high port>
   ```
2. Solve the puzzle by giving the four ports in any order. Evil port needs a raw
   socket, so run as root:
   ```
   sudo ./puzzle_solver <IP address> <port1> <port2> <port3> <port4>
   ```

`make clean` removes the binaries.

## How it works

`puzzle_solver` probes each port and matches it to a module by the text of its
banner, then runs the modules in order, passing what each one learns to the next:

| File | Role |
|---|---|
| `port_sender` | UDP socket wrapper with retries, shared by all modules |
| `secret_port` | S.E.C.R.E.T.: gives group id, sigil and the first secret port |
| `evil_port` | Sets the IPv4 "evil bit" via a raw socket; gives the second secret port |
| `guardian_port` | Hand-built IPv6+UDP packet inside a UDP payload; gives the secret phrase |
| `dragon_port` | Takes both secret ports, then knocks on the returned ports with the phrase |
| `puzzle_solver` | Identifies the ports and runs the modules |

## Assumptions

- The server drops about 1 in 10 requests, so each message is retried (3 times, 10 for knocks).
- Ports are identified by their banner text, so their order on the command line does not matter.
- The target is reachable over IPv4.
- Group members and the secret number are hard-coded in `secret_port.cpp`.

## Known limitations

- Nothing known to be broken.
- Without root, the Evil port fails, and Dragon cannot run without its result.
