#include "logic/latch.hpp"

#include "logic/gates.hpp"

namespace logic {

std::pair<sim::SignalId, sim::SignalId> build_sr_latch(sim::Circuit& circuit, sim::SignalStore& store, sim::SignalId s, sim::SignalId r, std::uint64_t delay) {
    sim::SignalId s_bar = build_not(circuit, store, s, delay);
    sim::SignalId r_bar = build_not(circuit, store, r, delay);

    // No defined initial state for q/q_not, so it needs an external bootstrap.
    sim::SignalId q = store.alloc("sr_q", sim::Bit::X);
    sim::SignalId q_not = store.alloc("sr_q_not", sim::Bit::X);

    // q = NAND(s_bar, q_not)
    circuit.add_gate({s_bar, q_not}, q, delay);
    // q_not = NAND(r_bar, q)
    circuit.add_gate({r_bar, q}, q_not, delay);

    return {q, q_not};
}

std::pair<sim::SignalId, sim::SignalId> build_d_latch(sim::Circuit& circuit, sim::SignalStore& store, sim::SignalId d, sim::SignalId enable, std::uint64_t delay) {
    sim::SignalId d_bar = build_not(circuit, store, d, delay);

    sim::SignalId s_bar = store.alloc("dlatch_s_bar", sim::Bit::X);
    circuit.add_gate({d, enable}, s_bar, delay); // s_bar = NAND(d, enable)

    sim::SignalId r_bar = store.alloc("dlatch_r_bar", sim::Bit::X);
    circuit.add_gate({d_bar, enable}, r_bar, delay); // r_bar = NAND(d_bar, enable)

    sim::SignalId q = store.alloc("dlatch_q", sim::Bit::X);
    sim::SignalId q_not = store.alloc("dlatch_q_not", sim::Bit::X);
    circuit.add_gate({s_bar, q_not}, q, delay);
    circuit.add_gate({r_bar, q}, q_not, delay);

    return {q, q_not};
}

} // namespace logic
