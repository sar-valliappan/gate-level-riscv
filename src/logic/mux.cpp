#include "logic/mux.hpp"

#include "logic/gates.hpp"

namespace logic {

sim::SignalId build_mux2(sim::Circuit& circuit, sim::SignalStore& store, sim::SignalId a, sim::SignalId b, sim::SignalId sel, std::uint64_t delay) {
    // out = (a AND NOT sel) OR (b AND sel)
    sim::SignalId sel_bar = build_not(circuit, store, sel, delay);
    sim::SignalId a_path = build_and(circuit, store, a, sel_bar, delay);
    sim::SignalId b_path = build_and(circuit, store, b, sel, delay);
    return build_or(circuit, store, a_path, b_path, delay);
}

sim::SignalId build_mux(sim::Circuit& circuit, sim::SignalStore& store, const std::vector<sim::SignalId>& inputs, const std::vector<sim::SignalId>& sel, std::uint64_t delay) {
    if (inputs.size() == 1) {
        return inputs[0];
    }

    std::vector<sim::SignalId> next_level;
    next_level.reserve(inputs.size() / 2);
    for (std::size_t i = 0; i < inputs.size(); i += 2) {
        next_level.push_back(build_mux2(circuit, store, inputs[i], inputs[i + 1], sel[0], delay));
    }

    std::vector<sim::SignalId> remaining_sel(sel.begin() + 1, sel.end());
    return build_mux(circuit, store, next_level, remaining_sel, delay);
}

} // namespace logic
