#include "logic/gates.hpp"
#include "sim/circuit.hpp"
#include "sim/event.hpp"
#include "sim/signal.hpp"
#include "test_utils.hpp"

#include <cassert>
#include <cstdio>
#include <functional>

static sim::Bit eval_unary(const std::function<sim::SignalId(sim::Circuit&, sim::SignalStore&, sim::SignalId, std::uint64_t)>& build, sim::Bit a_val) {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;

    sim::SignalId a = store.alloc("A", sim::Bit::Zero);
    sim::SignalId out = build(circuit, store, a, 1);

    queue.schedule(0, a, a_val);
    test_utils::settle(circuit, store, queue);

    return store.get(out);
}

static sim::Bit eval_binary(const std::function<sim::SignalId(sim::Circuit&, sim::SignalStore&, sim::SignalId, sim::SignalId, std::uint64_t)>& build, sim::Bit a_val, sim::Bit b_val) {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;

    sim::SignalId a = store.alloc("A", sim::Bit::Zero);
    sim::SignalId b = store.alloc("B", sim::Bit::Zero);
    sim::SignalId out = build(circuit, store, a, b, 1);

    queue.schedule(0, a, a_val);
    test_utils::settle(circuit, store, queue);
    queue.schedule(queue.current_time + 10, b, b_val);
    test_utils::settle(circuit, store, queue);

    return store.get(out);
}

static void not_truth_table() {
    assert(eval_unary(logic::build_not, sim::Bit::Zero) == sim::Bit::One);
    assert(eval_unary(logic::build_not, sim::Bit::One) == sim::Bit::Zero);
}

static void not_of_unknown_is_unknown() {
    assert(eval_unary(logic::build_not, sim::Bit::X) == sim::Bit::X);
}

static void and_truth_table() {
    assert(eval_binary(logic::build_and, sim::Bit::Zero, sim::Bit::Zero) == sim::Bit::Zero);
    assert(eval_binary(logic::build_and, sim::Bit::Zero, sim::Bit::One) == sim::Bit::Zero);
    assert(eval_binary(logic::build_and, sim::Bit::One, sim::Bit::Zero) == sim::Bit::Zero);
    assert(eval_binary(logic::build_and, sim::Bit::One, sim::Bit::One) == sim::Bit::One);
}

static void and_with_unknown_input() {
    // A 0 anywhere still forces AND to 0, even alongside an X.
    assert(eval_binary(logic::build_and, sim::Bit::Zero, sim::Bit::X) == sim::Bit::Zero);
    assert(eval_binary(logic::build_and, sim::Bit::X, sim::Bit::One) == sim::Bit::X);
}

static void or_truth_table() {
    assert(eval_binary(logic::build_or, sim::Bit::Zero, sim::Bit::Zero) == sim::Bit::Zero);
    assert(eval_binary(logic::build_or, sim::Bit::Zero, sim::Bit::One) == sim::Bit::One);
    assert(eval_binary(logic::build_or, sim::Bit::One, sim::Bit::Zero) == sim::Bit::One);
    assert(eval_binary(logic::build_or, sim::Bit::One, sim::Bit::One) == sim::Bit::One);
}

static void or_with_unknown_input() {
    // A 1 anywhere still forces OR to 1, even alongside an X.
    assert(eval_binary(logic::build_or, sim::Bit::One, sim::Bit::X) == sim::Bit::One);
    assert(eval_binary(logic::build_or, sim::Bit::X, sim::Bit::Zero) == sim::Bit::X);
}

static void xor_truth_table() {
    assert(eval_binary(logic::build_xor, sim::Bit::Zero, sim::Bit::Zero) == sim::Bit::Zero);
    assert(eval_binary(logic::build_xor, sim::Bit::Zero, sim::Bit::One) == sim::Bit::One);
    assert(eval_binary(logic::build_xor, sim::Bit::One, sim::Bit::Zero) == sim::Bit::One);
    assert(eval_binary(logic::build_xor, sim::Bit::One, sim::Bit::One) == sim::Bit::Zero);
}

static void xor_with_unknown_input() {
    assert(eval_binary(logic::build_xor, sim::Bit::One, sim::Bit::X) == sim::Bit::X);
}

static void nor_truth_table() {
    assert(eval_binary(logic::build_nor, sim::Bit::Zero, sim::Bit::Zero) == sim::Bit::One);
    assert(eval_binary(logic::build_nor, sim::Bit::Zero, sim::Bit::One) == sim::Bit::Zero);
    assert(eval_binary(logic::build_nor, sim::Bit::One, sim::Bit::Zero) == sim::Bit::Zero);
    assert(eval_binary(logic::build_nor, sim::Bit::One, sim::Bit::One) == sim::Bit::Zero);
}

static void xnor_truth_table() {
    assert(eval_binary(logic::build_xnor, sim::Bit::Zero, sim::Bit::Zero) == sim::Bit::One);
    assert(eval_binary(logic::build_xnor, sim::Bit::Zero, sim::Bit::One) == sim::Bit::Zero);
    assert(eval_binary(logic::build_xnor, sim::Bit::One, sim::Bit::Zero) == sim::Bit::Zero);
    assert(eval_binary(logic::build_xnor, sim::Bit::One, sim::Bit::One) == sim::Bit::One);
}

int main() {
    not_truth_table();
    not_of_unknown_is_unknown();
    and_truth_table();
    and_with_unknown_input();
    or_truth_table();
    or_with_unknown_input();
    xor_truth_table();
    xor_with_unknown_input();
    nor_truth_table();
    xnor_truth_table();
    std::puts("gates_test: all tests passed");
    return 0;
}
