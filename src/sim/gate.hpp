#pragma once

#include <array>
#include <cstdint>

#include "sim/signal.hpp"

namespace sim {

/// Unique identifier for each gate
struct GateId {
    std::uint32_t value;

    friend bool operator==(GateId a, GateId b);
    friend bool operator!=(GateId a, GateId b);
};

struct Gate {
    std::array<SignalId, 2> inputs;
    SignalId output;
    std::uint64_t delay;

    // Universal gate (NAND)
    bool eval(const SignalStore& store) const;
};

} // namespace sim
