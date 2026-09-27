#pragma once

#include <cstdint>
#include <utility>

#include "sim/circuit.hpp"
#include "sim/signal.hpp"

namespace logic {

/// NAND-based SR latch.
/// s=1 sets (q->1)
/// r=1 resets (q->0)
/// s=r=0 holds
/// s=r=1 invalid
std::pair<sim::SignalId, sim::SignalId> build_sr_latch(sim::Circuit& circuit, sim::SignalStore& store, sim::SignalId s, sim::SignalId r, std::uint64_t delay);

/// Gated D latch:
/// clk = 1: q = d
/// clk = 0: q holds its previous value.
std::pair<sim::SignalId, sim::SignalId> build_d_latch(sim::Circuit& circuit, sim::SignalStore& store, sim::SignalId d, sim::SignalId clk, std::uint64_t delay);

} // namespace logic
