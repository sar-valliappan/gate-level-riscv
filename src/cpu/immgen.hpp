#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "cpu/isa.hpp"
#include "sim/circuit.hpp"
#include "sim/signal.hpp"

namespace immgen {

/// Splits an isa::Format into its 3 gate-level select bits {fmt0 (LSB), fmt1, fmt2}.
std::array<sim::Bit, 3> format_bits(isa::Format format);

/// Extracts and sign-extends the immediate from the instruction.
std::vector<sim::SignalId> build_immgen(sim::Circuit& circuit, sim::SignalStore& store, const std::vector<sim::SignalId>& instr,
    const std::vector<sim::SignalId>& format, std::uint64_t delay);

} // namespace immgen
