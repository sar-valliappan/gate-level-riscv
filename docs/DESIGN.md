# DESIGN.md

Current-state architecture document. For a chronological log of individual decisions and why they were made, see `docs/DECISIONS.md`.

---

## 1. Description

This project is a discrete-event, gate-level simulator for a single-cycle CPU
implementing a subset of RV32I. It has two layers:

1. A general-purpose **event-driven simulation engine**, written in C++, that models digital logic signals and gates with real propagation delay, scheduling and propagating signal changes through a netlist over simulated time.
2. A **CPU built on top of that engine**: flip-flops, registers, a register file, an ALU, an immediate generator, a control unit, and a program counter, all constructed from the engine's gate primitives and wired into a full datapath. Instruction execution is the emergent result of signals propagating through that netlist, the same way it happens in real digital hardware.

The instruction subset covers all six RISC-V instruction formats (R/I/S/B/U/J), and the project also includes a small assembler so hand-written `.s` programs can be assembled and run directly on the simulated CPU.

---

## 2. Module map

```
src/
├── main.cpp       Entry point / demo runner (loads a program, runs the CPU)
├── sim/           Phase 1: Simulation engine
│   ├── signal.hpp  SignalId, Signal, SignalStore
│   ├── event.hpp   Event, EventQueue
│   ├── gate.hpp    Gate, GateId
│   └── circuit.hpp Circuit (gate storage + fanout tracking), eval/scheduling loop
├── logic/         Phase 2: Sequential logic
│   ├── gates.hpp   Composite gates built from NAND: NOT, AND, OR, NOR, XOR, XNOR
│   ├── latch.hpp   SR latch, D latch
│   ├── flipflop.hpp D flip-flop (edge-triggered, master-slave)
│   ├── register.hpp N-bit register, register file
│   └── mux.hpp     N-way multiplexer built from gates
├── cpu/           Phase 3: ALU, Phase 4: Full single-cycle datapath
│   ├── isa.hpp      Instruction formats, opcode/funct3/funct7 tables
│   ├── alu.hpp      32-bit ALU + ALU control decode
│   ├── immgen.hpp   Immediate generator
│   ├── control.hpp  Main control unit
│   ├── datapath.hpp Wires everything into the single-cycle CPU
│   └── memory.hpp   Abstracted instruction/data memory
├── asm/           Phase 4: Assembler
│   ├── parser.hpp   Parses .s-style text into instruction structs
│   └── encoder.hpp  Instruction struct -> 32-bit encoding
└── waveform.hpp   Signal-transition trace output
```

Each module's `.hpp` declares its types/functions; the matching `.cpp` in the same directory holds the implementation.

---

## 3. Core types

### Signal representation

Signals are **three-valued**: `Zero`, `One`, or `X` (unknown).

```cpp
struct SignalId {
    uint32_t value;
};

enum class Bit : uint8_t { Zero, One, X };

struct Signal {
    std::string name;
    Bit value;
    bool forced = false;   // true while overridden via SignalStore::force()
};

class SignalStore {
    std::vector<Signal> signals;
};
```

`SignalStore::set()` (how a gate drives its output) is a no-op while a signal is `forced`. `force()`/`release()` are the only way to override a signal regardless of what's actually driving it — see "Bootstrapping bistable feedback loops" below.

### Event queue

```cpp
struct Event {
    uint64_t time;
    uint64_t seq;          // insertion-order tiebreaker
    SignalId signal;
    Bit new_value;
};

class EventQueue {
    std::priority_queue<Event, std::vector<Event>, EventOrder> heap; // custom comparator: smallest (time, seq) pops first
    uint64_t next_seq;
    uint64_t current_time;
};
```

Ordered by `(time, seq)` rather than `time` alone, so that events scheduled at the same timestamp are processed in deterministic (FIFO, insertion-order) sequence.

### Gates

The simulator has exactly **one primitive gate**: NAND. NAND is a "universal gate": every other gate used anywhere in the project (NOT, AND, OR, NOR, XOR, XNOR) is **built from NAND gates**.

```cpp
struct Gate {
    std::array<SignalId, 2> inputs;   // NAND: always exactly 2 inputs
    SignalId output;
    uint64_t delay;
};
```

`Gate::eval` implements 4-valued NAND with `X` propagation.

Every other gate (NOT, AND, OR, NOR, XOR, XNOR) is a reusable function built once from NAND gates. Every composite gate costs *multiple* simulated NAND delays in series (e.g. XOR is 4 delays, not 1), so timing behavior downstream reflects per-NAND delay, not per-operator delay.
Every gate's output signal, including a bistable feedback pair like an SR/D latch's `q`/`q_not`, is allocated as `X` at construction time; nothing is special-cased in the netlist itself. Getting a fresh circuit to a defined starting state is the caller's job.

### Bootstrapping bistable feedback loops

- **`initial`-block-style deposits:** scheduling each input's currently stored value onto it as a time-0 `Event` then draining the queue once is enough to carry every purely-combinational signal from `X` to its correct value.
- **`force`/`release`:** a cross-coupled feedback pair (`q`/`q_not`) with no inputs driven yet is two gates each reading the other's `X`, which evaluates to `X` forever, the same way a real cross-coupled NAND latch has no defined power-on state without an explicit reset. Solve this issue by forcing one side of the loop to a concrete level regardless of what its driving gate computes.

### Circuit / fanout tracking

```cpp
class Circuit {
    std::vector<Gate> gates;
    std::unordered_map<SignalId, std::vector<GateId>> fanout;  // signal -> gates that read it as input
};
```

When a signal changes, only the gates in its fanout list are re-evaluated, not the entire circuit.

### Simulation loop (conceptual, implemented in `circuit.cpp`)

```
1. Pop the next Event (smallest time, then smallest seq) from the queue.
2. Advance current_time to the event's time.
3. Apply the new value to the signal in the SignalStore.
4. Look up the signal's fanout.
5. For each gate in the fanout: evaluate its output from current input values.
6. If the gate's output value changes, schedule a new Event at
   (current_time + gate.delay) for the output signal.
7. Repeat until the queue is empty.
```

---

## 4. Instruction subset

A subset of 10 commonly used, representative RV32I instructions, chosen to cover all
six RISC-V instruction formats rather than full opcode enumeration. This is the
complete implementation target for the project — not a "minimum" carved out of a
larger list.

| Format | Instructions | Count |
|---|---|---|
| R-type | ADD, SUB | 2 |
| I-type (ALU) | ADDI, ANDI | 2 |
| I-type (load) | LW | 1 |
| S-type (store) | SW | 1 |
| B-type (branch) | BEQ, BLT | 2 |
| U-type | LUI | 1 |
| J-type | JAL | 1 |
| **Total** | | **10** |

More instructions can be implemented once these are completed.

---

## 5. Memory model

Instruction and data memory are just plain `std::vector<uint32_t>` / byte arrays in `cpu/memory.hpp`. This is deliberate: simulating RAM at the gate level would be substantial additional work for minimal extra learning.

Everything that constitutes "the CPU proper", like ALU, register file, control unit, and PC logic is gate-simulated. Memory is treated as a black-box peripheral.

---

## 6. Roadmap / phases

- [ ] **Phase 1**: Event-driven simulation core (`sim/`): signals, event queue, the NAND primitive, fanout-based circuit evaluation, plus the composite gate layer (`logic/gates.hpp`) deriving NOT/AND/OR/NOR/XOR/XNOR from NAND alone. Validated with a 4-bit ripple-carry adder and waveform output showing real (NAND-chain) propagation delay.
- [ ] **Phase 2**: Sequential logic (`logic/`): clock generator, SR/D latches, edge-triggered D flip-flop, N-bit register, 32x32 register file, program counter.
- [ ] **Phase 3**: ALU + combinational support (`cpu/alu.hpp`, `cpu/immgen.hpp`): ALU control decode, 32-bit ALU covering ADD/SUB/ADDI/ANDI, immediate generator for I/S/B/U/J formats, branch comparator for BEQ/BLT.
- [ ] **Phase 4**: Full single-cycle datapath (`cpu/datapath.hpp`, `cpu/control.hpp`, `cpu/memory.hpp`): instruction decoder, main control unit, complete datapath wiring, abstracted instruction/data memory. Validated by hand-assembled test programs run end-to-end.
- [ ] **Phase 5**: Assembler (`asm/`): instruction encoder + two-pass parser with label resolution, so `.s` source files can be assembled and run directly.
- [ ] **Validation**: run the relevant subset of the official `riscv-tests` suite against implemented instructions.

---

## 7. Status

_Updated as of each work session. See `docs/DECISIONS.md` for the detailed,
chronological rationale behind each change._

Currently: **Project Start**