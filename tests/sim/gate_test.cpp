#include "sim/gate.hpp"
#include "sim/signal.hpp"

#include <cassert>
#include <cstdio>

static void nand_truth_table() {
    sim::SignalStore store;
    sim::SignalId a = store.alloc("A", sim::Bit::Zero);
    sim::SignalId b = store.alloc("B", sim::Bit::Zero);
    sim::SignalId out = store.alloc("OUT", sim::Bit::X);
    sim::Gate gate{{a, b}, out, 1};

    store.set(a, sim::Bit::Zero); store.set(b, sim::Bit::Zero);
    assert(gate.eval(store) == sim::Bit::One);

    store.set(a, sim::Bit::One); store.set(b, sim::Bit::Zero);
    assert(gate.eval(store) == sim::Bit::One);

    store.set(a, sim::Bit::Zero); store.set(b, sim::Bit::One);
    assert(gate.eval(store) == sim::Bit::One);

    store.set(a, sim::Bit::One); store.set(b, sim::Bit::One);
    assert(gate.eval(store) == sim::Bit::Zero);
}

static void nand_with_unknown_input() {
    sim::SignalStore store;
    sim::SignalId a = store.alloc("A", sim::Bit::X);
    sim::SignalId b = store.alloc("B", sim::Bit::X);
    sim::SignalId out = store.alloc("OUT", sim::Bit::X);
    sim::Gate gate{{a, b}, out, 1};

    // A 0 on either input forces the output high, even if the other is X.
    store.set(a, sim::Bit::Zero); store.set(b, sim::Bit::X);
    assert(gate.eval(store) == sim::Bit::One);

    store.set(a, sim::Bit::X); store.set(b, sim::Bit::Zero);
    assert(gate.eval(store) == sim::Bit::One);

    // Otherwise, any X input makes the result unknown.
    store.set(a, sim::Bit::X); store.set(b, sim::Bit::X);
    assert(gate.eval(store) == sim::Bit::X);

    store.set(a, sim::Bit::One); store.set(b, sim::Bit::X);
    assert(gate.eval(store) == sim::Bit::X);

    store.set(a, sim::Bit::X); store.set(b, sim::Bit::One);
    assert(gate.eval(store) == sim::Bit::X);
}

int main() {
    nand_truth_table();
    nand_with_unknown_input();
    std::puts("gate_test: all tests passed");
    return 0;
}
