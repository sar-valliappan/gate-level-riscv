#include "logic/gates.hpp"

namespace logic {

sim::SignalId build_not(sim::Circuit& circuit, sim::SignalStore& store, sim::SignalId a, std::uint64_t delay) {
    // NOT(a) = NAND(a, a)
    sim::SignalId out = store.alloc("not_out", false);
    circuit.add_gate({a, a}, out, delay);
    return out;
}

sim::SignalId build_and(sim::Circuit& circuit, sim::SignalStore& store, sim::SignalId a, sim::SignalId b, std::uint64_t delay) {
    // AND(a,b) = NOT(NAND(a,b))
    sim::SignalId nand_out = store.alloc("and_nand", false);
    circuit.add_gate({a, b}, nand_out, delay);
    return build_not(circuit, store, nand_out, delay);
}

sim::SignalId build_or(sim::Circuit& circuit, sim::SignalStore& store, sim::SignalId a, sim::SignalId b, std::uint64_t delay) {
    // OR(a,b) = NAND(NOT(a), NOT(b))
    sim::SignalId not_a = build_not(circuit, store, a, delay);
    sim::SignalId not_b = build_not(circuit, store, b, delay);
    sim::SignalId out = store.alloc("or_out", false);
    circuit.add_gate({not_a, not_b}, out, delay);
    return out;
}

sim::SignalId build_xor(sim::Circuit& circuit, sim::SignalStore& store, sim::SignalId a, sim::SignalId b, std::uint64_t delay) {
    sim::SignalId n1 = store.alloc("xor_n1", false);
    circuit.add_gate({a, b}, n1, delay);
    sim::SignalId n2 = store.alloc("xor_n2", false);
    circuit.add_gate({a, n1}, n2, delay);
    sim::SignalId n3 = store.alloc("xor_n3", false);
    circuit.add_gate({b, n1}, n3, delay);
    sim::SignalId out = store.alloc("xor_out", false);
    circuit.add_gate({n2, n3}, out, delay);
    return out;
}

sim::SignalId build_nor(sim::Circuit& circuit, sim::SignalStore& store, sim::SignalId a, sim::SignalId b, std::uint64_t delay) {
    sim::SignalId or_out = build_or(circuit, store, a, b, delay);
    return build_not(circuit, store, or_out, delay);
}

sim::SignalId build_xnor(sim::Circuit& circuit, sim::SignalStore& store, sim::SignalId a, sim::SignalId b, std::uint64_t delay) {
    sim::SignalId xor_out = build_xor(circuit, store, a, b, delay);
    return build_not(circuit, store, xor_out, delay);
}

} // namespace logic
