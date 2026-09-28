#include "logic/flipflop.hpp"

#include "logic/gates.hpp"
#include "logic/latch.hpp"

namespace logic {

std::tuple<sim::SignalId, sim::SignalId, sim::SignalId> build_d_flipflop(sim::Circuit& circuit, sim::SignalStore& store, sim::SignalId d, sim::SignalId clk, std::uint64_t delay) {
    sim::SignalId clk_bar = build_not(circuit, store, clk, delay);

    auto [qm, qm_not] = build_d_latch(circuit, store, d, clk_bar, delay);
    auto [q, q_not] = build_d_latch(circuit, store, qm, clk, delay);

    return {q, q_not, qm_not};
}

} // namespace logic
