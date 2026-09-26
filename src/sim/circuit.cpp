#include "sim/circuit.hpp"

#include <functional>

namespace sim {

std::size_t Circuit::SignalIdHash::operator()(SignalId id) const noexcept {
    return std::hash<std::uint32_t>{}(id.value);
}

GateId Circuit::add_gate(std::array<SignalId, 2> inputs, SignalId output, std::uint64_t delay) {
    GateId id{static_cast<std::uint32_t>(gates.size())};
    for (SignalId input : inputs) {
        fanout_[input].push_back(id);
    }
    gates.push_back(Gate{inputs, output, delay});
    return id;
}

const Gate& Circuit::gate(GateId id) const { return gates[id.value]; }

const std::vector<GateId>& Circuit::fanout_of(SignalId signal) const {
    static const std::vector<GateId> empty;
    auto it = fanout_.find(signal);
    return it == fanout_.end() ? empty : it->second;
}

} // namespace sim
