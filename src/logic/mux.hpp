#pragma once

#include <cstdint>
#include <vector>

#include "sim/circuit.hpp"
#include "sim/signal.hpp"

namespace logic {

/// 2:1 multiplexer: sel=0 selects a, sel=1 selects b.
sim::SignalId build_mux2(sim::Circuit& circuit, sim::SignalStore& store, sim::SignalId a, sim::SignalId b, sim::SignalId sel, std::uint64_t delay);

/// N-way multiplexer built from a tree of 2:1 muxes.
sim::SignalId build_mux(sim::Circuit& circuit, sim::SignalStore& store, const std::vector<sim::SignalId>& inputs, const std::vector<sim::SignalId>& sel, std::uint64_t delay);

} // namespace logic
