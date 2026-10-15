#include "asm/encoder.hpp"
#include "asm/parser.hpp"
#include "cpu/memory.hpp"
#include "cpu/runner.hpp"
#include "sim/circuit.hpp"
#include "sim/event.hpp"
#include "sim/signal.hpp"

#include <cassert>
#include <cstdio>

static void runs_a_small_hand_assembled_program() {
    auto program = assembler::parse(R"(
    addi x1, x0, 5       # x1 = 5
    addi x2, x0, 7       # x2 = 7
    add  x3, x1, x2      # x3 = x1 + x2 = 12
    sub  x4, x2, x1      # x4 = x2 - x1 = 2
    sw   x3, 0(x0)       # mem[0] = x3
    lw   x5, 0(x0)       # x5 = mem[0]
    beq  x3, x5, skip1   # x3 == x5: taken, skip the next instruction
    addi x6, x0, 99      # skipped
skip1:
    andi x7, x3, 6       # x7 = x3 & 6 = 4
    blt  x4, x1, skip2   # x4 < x1: taken, skip the next instruction
    addi x8, x0, 111     # skipped
skip2:
    lui  x9, 0x1000      # x9 = 0x1000
    jal  x10, after      # x10 = return address, jump to after
    addi x11, x0, 123    # skipped
after:
    addi x12, x0, 1      # landing pad
)");
    cpu::Memory memory = cpu::make_memory(assembler::encode(program));

    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    cpu::Runner runner = cpu::build_runner(circuit, store, queue);
    assert(cpu::pc_value(store, runner) == cpu::kResetPcOffset);

    // Dynamic step count, not program.size(): the taken beq/blt/jal each skip
    // one static instruction, so only 12 of the 15 lines actually execute.
    constexpr int kCycles = 12;
    for (int i = 0; i < kCycles; i++) {
        cpu::step(circuit, store, queue, runner, memory);
    }

    assert(cpu::pc_value(store, runner) == cpu::kResetPcOffset + 4 * program.size()); // one past the landing pad

    assert(cpu::register_value(store, runner, 1) == 5u);
    assert(cpu::register_value(store, runner, 2) == 7u);
    assert(cpu::register_value(store, runner, 3) == 12u);
    assert(cpu::register_value(store, runner, 4) == 2u);
    assert(cpu::register_value(store, runner, 5) == 12u); // loaded back from memory
    assert(cpu::register_value(store, runner, 6) == 0u);  // skipped by the taken beq
    assert(cpu::register_value(store, runner, 7) == 4u);  // 12 & 6
    assert(cpu::register_value(store, runner, 8) == 0u);  // skipped by the taken blt
    assert(cpu::register_value(store, runner, 9) == 0x1000u);
    assert(cpu::register_value(store, runner, 10) == 56u); // jal's return address
    assert(cpu::register_value(store, runner, 11) == 0u);  // skipped by the jump
    assert(cpu::register_value(store, runner, 12) == 1u);

    assert(memory.read_word(0) == 12u);

    std::puts("datapath_test: all tests passed");
}

int main() {
    runs_a_small_hand_assembled_program();
    return 0;
}
