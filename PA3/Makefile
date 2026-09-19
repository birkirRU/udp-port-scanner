CXX = g++
CXXFLAGS = -Wall -std=c++17
CXXDEBUGFLAGS = -g

SOURCES = puzzle_solver.cpp \
          port_sender.cpp \
          secret_port.cpp \
          evil_port.cpp \
          guardian_port.cpp \
          dragon_port.cpp

all: puzzle_solver

puzzle_solver:
	$(CXX) $(CXXFLAGS) $(SOURCES) -o $@

puzzle_solver_debug:
	$(CXX) $(CXXFLAGS) $(CXXDEBUGFLAGS) $(SOURCES) -o $@

clean:
	rm -f puzzle_solver