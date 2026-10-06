#include "logic/mux.hpp"
#include "sim/circuit.hpp"
#include "sim/event.hpp"
#include "sim/signal.hpp"
#include "test_utils.hpp"

#include <cassert>
#include <cstdio>
#include <vector>

static sim::Bit eval_mux2(sim::Bit a_val, sim::Bit b_val, sim::Bit sel_val) {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;

    sim::SignalId a = store.alloc("A", sim::Bit::Zero);
    sim::SignalId b = store.alloc("B", sim::Bit::Zero);
    sim::SignalId sel = store.alloc("SEL", sim::Bit::Zero);
    sim::SignalId out = logic::build_mux2(circuit, store, a, b, sel, 1);

    queue.schedule(queue.current_time + 10, a, a_val);
    queue.schedule(queue.current_time + 10, b, b_val);
    queue.schedule(queue.current_time + 10, sel, sel_val);
    test_utils::settle(circuit, store, queue);

    return store.get(out);
}

static void mux2_selects_a_when_sel_zero() {
    assert(eval_mux2(sim::Bit::Zero, sim::Bit::One, sim::Bit::Zero) == sim::Bit::Zero);
    assert(eval_mux2(sim::Bit::One, sim::Bit::Zero, sim::Bit::Zero) == sim::Bit::One);
}

static void mux2_selects_b_when_sel_one() {
    assert(eval_mux2(sim::Bit::Zero, sim::Bit::One, sim::Bit::One) == sim::Bit::One);
    assert(eval_mux2(sim::Bit::One, sim::Bit::Zero, sim::Bit::One) == sim::Bit::Zero);
}

static void mux4_selects_indexed_input() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;

    std::vector<sim::SignalId> inputs;
    for (int i = 0; i < 4; i++) {
        inputs.push_back(store.alloc("IN" + std::to_string(i), sim::Bit::Zero));
    }
    sim::SignalId sel0 = store.alloc("SEL0", sim::Bit::Zero);
    sim::SignalId sel1 = store.alloc("SEL1", sim::Bit::Zero);

    sim::SignalId out = logic::build_mux(circuit, store, inputs, {sel0, sel1}, 1);

    queue.schedule(queue.current_time + 10, inputs[0], sim::Bit::Zero);
    queue.schedule(queue.current_time + 10, inputs[1], sim::Bit::One);
    queue.schedule(queue.current_time + 10, inputs[2], sim::Bit::Zero);
    queue.schedule(queue.current_time + 10, inputs[3], sim::Bit::One);
    test_utils::settle(circuit, store, queue);

    // index 0 (sel1=0, sel0=0) -> inputs[0]
    queue.schedule(queue.current_time + 10, sel0, sim::Bit::Zero);
    queue.schedule(queue.current_time + 10, sel1, sim::Bit::Zero);
    test_utils::settle(circuit, store, queue);
    assert(store.get(out) == sim::Bit::Zero);

    // index 1 (sel1=0, sel0=1) -> inputs[1]
    queue.schedule(queue.current_time + 10, sel0, sim::Bit::One);
    test_utils::settle(circuit, store, queue);
    assert(store.get(out) == sim::Bit::One);

    // index 3 (sel1=1, sel0=1) -> inputs[3]
    queue.schedule(queue.current_time + 10, sel1, sim::Bit::One);
    test_utils::settle(circuit, store, queue);
    assert(store.get(out) == sim::Bit::One);

    // index 2 (sel1=1, sel0=0) -> inputs[2]
    queue.schedule(queue.current_time + 10, sel0, sim::Bit::Zero);
    test_utils::settle(circuit, store, queue);
    assert(store.get(out) == sim::Bit::Zero);
}

static void mux1_passes_through_regardless_of_sel() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;

    sim::SignalId a = store.alloc("A", sim::Bit::Zero);
    sim::SignalId out = logic::build_mux(circuit, store, {a}, {}, 1);

    queue.schedule(queue.current_time + 10, a, sim::Bit::One);
    test_utils::settle(circuit, store, queue);
    assert(store.get(out) == sim::Bit::One);
}

int main() {
    mux2_selects_a_when_sel_zero();
    mux2_selects_b_when_sel_one();
    mux4_selects_indexed_input();
    mux1_passes_through_regardless_of_sel();
    std::puts("mux_test: all tests passed");
    return 0;
}
