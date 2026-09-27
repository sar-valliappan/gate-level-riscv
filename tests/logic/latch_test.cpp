#include "logic/latch.hpp"
#include "sim/circuit.hpp"
#include "sim/event.hpp"
#include "sim/signal.hpp"
#include "test_utils.hpp"

#include <cassert>
#include <cstdio>

static void sr_latch_set() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;

    sim::SignalId s = store.alloc("S", sim::Bit::Zero);
    sim::SignalId r = store.alloc("R", sim::Bit::Zero);
    auto [q, q_not] = logic::build_sr_latch(circuit, store, s, r, 1);
    test_utils::bootstrap_latch(circuit, store, queue, q_not, sim::Bit::One, {s, r});

    queue.schedule(queue.current_time + 10, s, sim::Bit::One);
    test_utils::settle(circuit, store, queue);

    assert(store.get(q) == sim::Bit::One);
    assert(store.get(q_not) == sim::Bit::Zero);
}

static void sr_latch_reset() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;

    sim::SignalId s = store.alloc("S", sim::Bit::Zero);
    sim::SignalId r = store.alloc("R", sim::Bit::Zero);
    auto [q, q_not] = logic::build_sr_latch(circuit, store, s, r, 1);
    test_utils::bootstrap_latch(circuit, store, queue, q_not, sim::Bit::One, {s, r});

    queue.schedule(queue.current_time + 10, r, sim::Bit::One);
    test_utils::settle(circuit, store, queue);

    assert(store.get(q) == sim::Bit::Zero);
    assert(store.get(q_not) == sim::Bit::One);
}

static void sr_latch_holds_when_both_low() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;

    sim::SignalId s = store.alloc("S", sim::Bit::Zero);
    sim::SignalId r = store.alloc("R", sim::Bit::Zero);
    auto [q, q_not] = logic::build_sr_latch(circuit, store, s, r, 1);
    test_utils::bootstrap_latch(circuit, store, queue, q_not, sim::Bit::One, {s, r});

    // Set, then release both S and R — should hold q=1.
    queue.schedule(queue.current_time + 10, s, sim::Bit::One);
    test_utils::settle(circuit, store, queue);
    queue.schedule(queue.current_time + 10, s, sim::Bit::Zero);
    test_utils::settle(circuit, store, queue);

    assert(store.get(q) == sim::Bit::One);
}

static void sr_latch_invalid_state_drives_both_outputs_high() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;

    sim::SignalId s = store.alloc("S", sim::Bit::Zero);
    sim::SignalId r = store.alloc("R", sim::Bit::Zero);
    auto [q, q_not] = logic::build_sr_latch(circuit, store, s, r, 1);
    test_utils::bootstrap_latch(circuit, store, queue, q_not, sim::Bit::One, {s, r});

    std::uint64_t t = queue.current_time + 10;
    queue.schedule(t, s, sim::Bit::One);
    queue.schedule(t, r, sim::Bit::One);
    test_utils::settle(circuit, store, queue);

    // Invalid state: both outputs driven high.
    assert(store.get(q) == sim::Bit::One);
    assert(store.get(q_not) == sim::Bit::One);
}

static void sr_latch_unknown_input_makes_outputs_unknown() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;

    sim::SignalId s = store.alloc("S", sim::Bit::Zero);
    sim::SignalId r = store.alloc("R", sim::Bit::Zero);
    auto [q, q_not] = logic::build_sr_latch(circuit, store, s, r, 1);
    test_utils::bootstrap_latch(circuit, store, queue, q_not, sim::Bit::One, {s, r});

    // Set first so q/q_not sit at a known state...
    queue.schedule(queue.current_time + 10, s, sim::Bit::One);
    test_utils::settle(circuit, store, queue);
    assert(store.get(q) == sim::Bit::One);

    // ...then drive S to X. s_bar becomes X, but q = NAND(s_bar=X, q_not=0)
    // is still forced to 1: a 0 input dominates an X (0 dominates).
    queue.schedule(queue.current_time + 10, s, sim::Bit::X);
    test_utils::settle(circuit, store, queue);

    assert(store.get(q) == sim::Bit::One);
    assert(store.get(q_not) == sim::Bit::Zero);
}

static void d_latch_transparent_when_enabled() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;

    sim::SignalId d = store.alloc("D", sim::Bit::Zero);
    sim::SignalId en = store.alloc("EN", sim::Bit::Zero);
    auto [q, q_not] = logic::build_d_latch(circuit, store, d, en, 1);
    test_utils::bootstrap_latch(circuit, store, queue, q_not, sim::Bit::One, {d, en});

    std::uint64_t t = queue.current_time + 10;
    queue.schedule(t, en, sim::Bit::One);
    queue.schedule(t, d, sim::Bit::One);
    test_utils::settle(circuit, store, queue);
    assert(store.get(q) == sim::Bit::One);

    queue.schedule(queue.current_time + 10, d, sim::Bit::Zero);
    test_utils::settle(circuit, store, queue);
    assert(store.get(q) == sim::Bit::Zero); // still transparent, follows d
}

static void d_latch_holds_when_disabled() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;

    sim::SignalId d = store.alloc("D", sim::Bit::Zero);
    sim::SignalId en = store.alloc("EN", sim::Bit::Zero);
    auto [q, q_not] = logic::build_d_latch(circuit, store, d, en, 1);
    test_utils::bootstrap_latch(circuit, store, queue, q_not, sim::Bit::One, {d, en});

    std::uint64_t t = queue.current_time + 10;
    queue.schedule(t, en, sim::Bit::One);
    queue.schedule(t, d, sim::Bit::One);
    test_utils::settle(circuit, store, queue);
    assert(store.get(q) == sim::Bit::One);

    queue.schedule(queue.current_time + 10, en, sim::Bit::Zero); // disable
    test_utils::settle(circuit, store, queue);
    queue.schedule(queue.current_time + 10, d, sim::Bit::Zero); // change d while disabled
    test_utils::settle(circuit, store, queue);

    assert(store.get(q) == sim::Bit::One); // holds old value, ignores new d
}

static void d_latch_holds_through_unknown_d_while_disabled() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;

    sim::SignalId d = store.alloc("D", sim::Bit::Zero);
    sim::SignalId en = store.alloc("EN", sim::Bit::Zero);
    auto [q, q_not] = logic::build_d_latch(circuit, store, d, en, 1);
    test_utils::bootstrap_latch(circuit, store, queue, q_not, sim::Bit::One, {d, en});

    std::uint64_t t = queue.current_time + 10;
    queue.schedule(t, en, sim::Bit::One);
    queue.schedule(t, d, sim::Bit::One);
    test_utils::settle(circuit, store, queue);
    assert(store.get(q) == sim::Bit::One);

    queue.schedule(queue.current_time + 10, en, sim::Bit::Zero); // disable
    test_utils::settle(circuit, store, queue);
    queue.schedule(queue.current_time + 10, d, sim::Bit::X); // d becomes unknown while disabled
    test_utils::settle(circuit, store, queue);

    assert(store.get(q) == sim::Bit::One); // still holds, immune to d
}

int main() {
    sr_latch_set();
    sr_latch_reset();
    sr_latch_holds_when_both_low();
    sr_latch_invalid_state_drives_both_outputs_high();
    sr_latch_unknown_input_makes_outputs_unknown();
    d_latch_transparent_when_enabled();
    d_latch_holds_when_disabled();
    d_latch_holds_through_unknown_d_while_disabled();
    std::puts("latch_test: all tests passed");
    return 0;
}
