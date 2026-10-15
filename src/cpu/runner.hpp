#pragma once

#include <cstdint>
#include <vector>

#include "cpu/datapath.hpp"
#include "cpu/memory.hpp"
#include "sim/circuit.hpp"
#include "sim/event.hpp"
#include "sim/signal.hpp"

namespace cpu {

constexpr std::uint32_t kResetPcOffset = 4;

/// One running datapath instance: its instruction/data-memory buses, clock, and the datapath itself.
struct Runner {
    std::vector<sim::SignalId> instr_bus;
    std::vector<sim::SignalId> mem_read_bus;
    sim::SignalId clk;
    DatapathBits bits;
};

/// Builds a datapath and resets it to PC = kResetPcOffset, every register file entry = 0.
Runner build_runner(sim::Circuit& circuit, sim::SignalStore& store, sim::EventQueue& queue);

Memory make_memory(std::vector<std::uint32_t> program);

/// Runs one fetch/decode/execute/memory/writeback cycle against `memory`, then pulses the clock to commit it.
void step(sim::Circuit& circuit, sim::SignalStore& store, sim::EventQueue& queue, Runner& runner, Memory& memory);

/// Current PC.
std::uint32_t pc_value(const sim::SignalStore& store, const Runner& runner);

/// Current value of register file entry `index` (0-31).
std::uint32_t register_value(const sim::SignalStore& store, const Runner& runner, std::size_t index);

} // namespace cpu
