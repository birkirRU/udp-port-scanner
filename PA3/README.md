


### Program seperation



### What to do

- Have a generic udp/socket sender module that each internal module (e.g. `secret_port.cpp`) uses.
- `puzzle_solver.cpp` is the main coordinator, maps port to internal module, recieves output of modules and redirects them into next corresponding module.
