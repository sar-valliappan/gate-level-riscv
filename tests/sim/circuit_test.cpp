#include "sim/circuit.hpp"
#include "sim/signal.hpp"

#include <cassert>
#include <cstdio>
#include <vector>

static bool contains(const std::vector<sim::GateId>& v, sim::GateId id) {
    for (sim::GateId g : v) {
        if (g == id) return true;
    }
    return false;
}

static void add_gate_returns_sequential_ids() {
    sim::SignalStore store;
    sim::SignalId a = store.alloc("A", sim::Bit::Zero);
    sim::SignalId b = store.alloc("B", sim::Bit::Zero);
    sim::SignalId out1 = store.alloc("OUT1", sim::Bit::X);
    sim::SignalId out2 = store.alloc("OUT2", sim::Bit::X);

    sim::Circuit circuit;
    sim::GateId g1 = circuit.add_gate({a, b}, out1, 1);
    sim::GateId g2 = circuit.add_gate({a, b}, out2, 1);

    assert(g1 == sim::GateId{0});
    assert(g2 == sim::GateId{1});
}

static void gate_lookup_returns_correct_fields() {
    sim::SignalStore store;
    sim::SignalId a = store.alloc("A", sim::Bit::Zero);
    sim::SignalId b = store.alloc("B", sim::Bit::Zero);
    sim::SignalId out = store.alloc("OUT", sim::Bit::X);

    sim::Circuit circuit;
    sim::GateId g = circuit.add_gate({a, b}, out, 3);

    const sim::Gate& stored = circuit.gate(g);
    assert(stored.inputs[0] == a && stored.inputs[1] == b);
    assert(stored.output == out);
    assert(stored.delay == 3);
}

static void fanout_tracks_correct_gates() {
    sim::SignalStore store;
    sim::SignalId a = store.alloc("A", sim::Bit::Zero);
    sim::SignalId b = store.alloc("B", sim::Bit::Zero);
    sim::SignalId out1 = store.alloc("OUT1", sim::Bit::X);
    sim::SignalId out2 = store.alloc("OUT2", sim::Bit::X);

    sim::Circuit circuit;
    // g1 reads both a and b; g2 reads only a
    sim::GateId g1 = circuit.add_gate({a, b}, out1, 1);
    sim::GateId g2 = circuit.add_gate({a, a}, out2, 1);

    const auto& fanout_a = circuit.fanout_of(a);
    assert(fanout_a.size() == 3); // g1 once, g2 twice (self-input)
    assert(contains(fanout_a, g1));
    assert(contains(fanout_a, g2));

    const auto& fanout_b = circuit.fanout_of(b);
    assert(fanout_b.size() == 1 && fanout_b[0] == g1);
}

static void signal_with_no_fanout_returns_empty() {
    sim::SignalStore store;
    sim::SignalId a = store.alloc("A", sim::Bit::Zero);
    sim::SignalId out = store.alloc("OUT", sim::Bit::X);

    sim::Circuit circuit;
    assert(circuit.fanout_of(a).empty());
    assert(circuit.fanout_of(out).empty());
}

int main() {
    add_gate_returns_sequential_ids();
    gate_lookup_returns_correct_fields();
    fanout_tracks_correct_gates();
    signal_with_no_fanout_returns_empty();
    std::puts("circuit_test: all tests passed");
    return 0;
}
