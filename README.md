# Orderbook
A basic orderbook implementation in C++

# Purpose
The reason for this repository's existence is to have a place for various experiments. This orderbook acts as a starting ground for such experiments, be it trying different compiler versions, new language features, extra tooling or code changes.

# Dependencies
- Just needs C++14, libasan and the Google unit testing framework.
- R if you want to build a larger test input file

# How to build
'make'

# How to run
./main test-input.txt (optionally 'silent')

# Input format
- When adding/modifying
[Action],[Order id],[Side],[Volume],[Price]
- When removing
[Action],[Order id],[Side],[Price]
- See test-input.txt for examples.

Action is.
- A for add
- M for modify
- X for remove

Order id, volume and price are all uint32_t.

Side is:
- B for buy
- S for sell

