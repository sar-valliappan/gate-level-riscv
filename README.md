# gate-level-riscv

A discrete-event, gate-level simulator for a single-cycle CPU implementing a
subset of RV32I (covering all six RISC-V instruction formats),
built entirely from a single universal gate (NAND) up through flip-flops,
registers, an ALU, and a full datapath. Written in C++.

Full architecture and design rationale: see [docs/DESIGN.md](docs/DESIGN.md).

## Status
🚧 In development. See DESIGN.md §7 for current phase.

## Build & run
```
cmake -S . -B build
cmake --build build
ctest --test-dir build
./build/gate-level-riscv
```

## Instruction subset
ADD, SUB, ADDI, ANDI, LW, SW, BEQ, BLT, LUI, JAL
(all six instruction formats: R/I/S/B/U/J)