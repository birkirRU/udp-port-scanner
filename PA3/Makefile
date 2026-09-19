CXX = g++
CXXFLAGS = -Wall -std=c++17
CXXDEBUGFLAGS = -g

SOURCES = puzzle_solver.cpp \
          port_sender.cpp \
          secret_port.cpp \
          evil_port.cpp \
          guardian_port.cpp \
          dragon_port.cpp

HEADERS = port_sender.h puzzle_solver.h secret_port.h evil_port.h \
          guardian_port.h dragon_port.h

all: puzzle_solver scanner

puzzle_solver: $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $@

puzzle_solver_debug: $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) $(CXXDEBUGFLAGS) $(SOURCES) -o $@

scanner: scanner.cpp
	$(CXX) $(CXXFLAGS) scanner.cpp -o $@

clean:
	rm -f puzzle_solver puzzle_solver_debug scanner

.PHONY: all clean
