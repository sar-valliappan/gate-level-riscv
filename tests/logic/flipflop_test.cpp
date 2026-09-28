#include "logic/flipflop.hpp"
#include "sim/circuit.hpp"
#include "sim/event.hpp"
#include "sim/signal.hpp"
#include "test_utils.hpp"

#include <cassert>
#include <cstdio>

namespace {

struct Flipflop {
    sim::SignalId d;
    sim::SignalId clk;
    sim::SignalId q;
    sim::SignalId q_not;
};

Flipflop make_flipflop(sim::Circuit& circuit, sim::SignalStore& store, sim::EventQueue& queue) {
    sim::SignalId d = store.alloc("D", sim::Bit::Zero);
    sim::SignalId clk = store.alloc("CLK", sim::Bit::Zero);
    auto [q, q_not, qm_not] = logic::build_d_flipflop(circuit, store, d, clk, 1);

    test_utils::bootstrap_flipflop(circuit, store, queue, qm_not, q_not, sim::Bit::One, {d, clk});

    return {d, clk, q, q_not};
}

} // namespace

static void flipflop_captures_d_on_rising_edge() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    Flipflop ff = make_flipflop(circuit, store, queue);

    queue.schedule(queue.current_time + 10, ff.d, sim::Bit::One);
    test_utils::settle(circuit, store, queue);
    assert(store.get(ff.q) == sim::Bit::Zero);

    queue.schedule(queue.current_time + 10, ff.clk, sim::Bit::One); // rising edge
    test_utils::settle(circuit, store, queue);

    assert(store.get(ff.q) == sim::Bit::One);
    assert(store.get(ff.q_not) == sim::Bit::Zero);
}

static void flipflop_ignores_d_while_clk_high() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    Flipflop ff = make_flipflop(circuit, store, queue);

    queue.schedule(queue.current_time + 10, ff.d, sim::Bit::One);
    queue.schedule(queue.current_time + 10, ff.clk, sim::Bit::One);
    test_utils::settle(circuit, store, queue);
    assert(store.get(ff.q) == sim::Bit::One);

    queue.schedule(queue.current_time + 10, ff.d, sim::Bit::Zero);
    test_utils::settle(circuit, store, queue);
    assert(store.get(ff.q) == sim::Bit::One);
}

static void flipflop_holds_across_falling_edge() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    Flipflop ff = make_flipflop(circuit, store, queue);

    queue.schedule(queue.current_time + 10, ff.d, sim::Bit::One);
    queue.schedule(queue.current_time + 10, ff.clk, sim::Bit::One);
    test_utils::settle(circuit, store, queue);
    assert(store.get(ff.q) == sim::Bit::One);

    queue.schedule(queue.current_time + 10, ff.clk, sim::Bit::Zero);
    test_utils::settle(circuit, store, queue);
    assert(store.get(ff.q) == sim::Bit::One);

    queue.schedule(queue.current_time + 10, ff.d, sim::Bit::Zero);
    test_utils::settle(circuit, store, queue);
    assert(store.get(ff.q) == sim::Bit::One);
}

static void flipflop_captures_new_value_on_next_rising_edge() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    Flipflop ff = make_flipflop(circuit, store, queue);

    queue.schedule(queue.current_time + 10, ff.d, sim::Bit::One);
    queue.schedule(queue.current_time + 10, ff.clk, sim::Bit::One);
    test_utils::settle(circuit, store, queue);
    assert(store.get(ff.q) == sim::Bit::One);

    queue.schedule(queue.current_time + 10, ff.clk, sim::Bit::Zero);
    queue.schedule(queue.current_time + 10, ff.d, sim::Bit::Zero);
    test_utils::settle(circuit, store, queue);
    assert(store.get(ff.q) == sim::Bit::One); 

    queue.schedule(queue.current_time + 10, ff.clk, sim::Bit::One); 
    test_utils::settle(circuit, store, queue);

    assert(store.get(ff.q) == sim::Bit::Zero);
    assert(store.get(ff.q_not) == sim::Bit::One);
}

int main() {
    flipflop_captures_d_on_rising_edge();
    flipflop_ignores_d_while_clk_high();
    flipflop_holds_across_falling_edge();
    flipflop_captures_new_value_on_next_rising_edge();
    std::puts("flipflop_test: all tests passed");
    return 0;
}
