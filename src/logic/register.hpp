#pragma once

#include <cstdint>
#include <vector>

#include "sim/circuit.hpp"
#include "sim/signal.hpp"

namespace logic {

struct RegisterBits {
    std::vector<sim::SignalId> q;
    std::vector<sim::SignalId> q_not;
    std::vector<sim::SignalId> master_q_not; // per-bit master latch feedback, for bootstrapping
};

/// N-bit register
RegisterBits build_register(sim::Circuit& circuit, sim::SignalStore& store, const std::vector<sim::SignalId>& d, sim::SignalId clk, std::uint64_t delay);

struct RegisterFileBits {
    std::vector<RegisterBits> registers; // one per addressable register
    std::vector<sim::SignalId> read_data1;
    std::vector<sim::SignalId> read_data2;
};

/// Register file with two combinational read ports and one clocked write port.
RegisterFileBits build_register_file(sim::Circuit& circuit, sim::SignalStore& store, const std::vector<sim::SignalId>& rs1, const std::vector<sim::SignalId>& rs2,
    const std::vector<sim::SignalId>& rd, const std::vector<sim::SignalId>& write_data, sim::SignalId write_enable, sim::SignalId clk, std::uint64_t delay);

} // namespace logic
