#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "sim/gate.hpp"
#include "sim/signal.hpp"

namespace sim {

class Circuit {
public:
    Circuit() = default;

    GateId add_gate(std::array<SignalId, 2> inputs, SignalId output, std::uint64_t delay);
    const Gate& gate(GateId id) const;
    const std::vector<GateId>& fanout_of(SignalId signal) const;

    std::vector<Gate> gates;

private:
    struct SignalIdHash {
        std::size_t operator()(SignalId id) const noexcept;
    };

    std::unordered_map<SignalId, std::vector<GateId>, SignalIdHash> fanout_;
};

} // namespace sim
