#pragma once

#include <cstdint>
#include <tuple>

#include "sim/circuit.hpp"
#include "sim/signal.hpp"

namespace logic {

/// Positive-edge-triggered D flip-flop, built master-slave from two D latches.
/// Returns {q, q_not, master_q_not}.
std::tuple<sim::SignalId, sim::SignalId, sim::SignalId> build_d_flipflop(sim::Circuit& circuit, sim::SignalStore& store, sim::SignalId d, sim::SignalId clk, std::uint64_t delay);

} // namespace logic
