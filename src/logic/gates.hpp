#pragma once

#include <cstdint>

#include "sim/circuit.hpp"
#include "sim/signal.hpp"

namespace logic {

sim::SignalId build_not(sim::Circuit& circuit, sim::SignalStore& store, sim::SignalId a, std::uint64_t delay);
sim::SignalId build_and(sim::Circuit& circuit, sim::SignalStore& store, sim::SignalId a, sim::SignalId b, std::uint64_t delay);
sim::SignalId build_or(sim::Circuit& circuit, sim::SignalStore& store, sim::SignalId a, sim::SignalId b, std::uint64_t delay);
sim::SignalId build_xor(sim::Circuit& circuit, sim::SignalStore& store, sim::SignalId a, sim::SignalId b, std::uint64_t delay);
sim::SignalId build_nor(sim::Circuit& circuit, sim::SignalStore& store, sim::SignalId a, sim::SignalId b, std::uint64_t delay);
sim::SignalId build_xnor(sim::Circuit& circuit, sim::SignalStore& store, sim::SignalId a, sim::SignalId b, std::uint64_t delay);

} // namespace logic
