## Compiling and Running
### Dependencies:

- C++ compiler with C++11 or newer
- g++
- make (if using the provided Makefile)
- POSIX socket APIs (sys/socket.h, arpa/inet.h, etc.)
- Linux
Install dependencies:

`sudo <package manager> install g++ make`

### Compile:

`make`

Or directly:

`g++ -std=c++11 -Wall -Wextra -o scanner scanner.cpp`

How to run the compiled program
The program takes three arguments:

`./scanner <IP address> <low port> <high port>`

Linux / macOS
`./scanner 130.208.246.98 4000 4100`

The program sends a UDP message to each port and considers a port open when it receives a response.