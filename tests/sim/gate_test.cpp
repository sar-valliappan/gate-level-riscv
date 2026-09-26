#include "sim/gate.hpp"
#include "sim/signal.hpp"

#include <cassert>
#include <cstdio>

static void nand_truth_table() {
    sim::SignalStore store;
    sim::SignalId a = store.alloc("A", false);
    sim::SignalId b = store.alloc("B", false);
    sim::SignalId out = store.alloc("OUT", false);
    sim::Gate gate{{a, b}, out, 1};

    store.set(a, false); store.set(b, false);
    assert(gate.eval(store) == true);

    store.set(a, true); store.set(b, false);
    assert(gate.eval(store) == true);

    store.set(a, false); store.set(b, true);
    assert(gate.eval(store) == true);

    store.set(a, true); store.set(b, true);
    assert(gate.eval(store) == false);
}

int main() {
    nand_truth_table();
    std::puts("gate_test: all tests passed");
    return 0;
}
