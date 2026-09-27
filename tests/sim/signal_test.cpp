#include "sim/signal.hpp"

#include <cassert>
#include <cstdio>

static void alloc_and_read_signal() {
    sim::SignalStore store;
    sim::SignalId a = store.alloc("A", sim::Bit::Zero);
    sim::SignalId b = store.alloc("B", sim::Bit::One);

    assert(store.get(a) == sim::Bit::Zero);
    assert(store.get(b) == sim::Bit::One);
}

static void alloc_defaults_to_unknown() {
    sim::SignalStore store;
    sim::SignalId a = store.alloc("A", sim::Bit::X);
    assert(store.get(a) == sim::Bit::X);
}

static void set_signal_updates_value() {
    sim::SignalStore store;
    sim::SignalId a = store.alloc("A", sim::Bit::Zero);
    store.set(a, sim::Bit::One);
    assert(store.get(a) == sim::Bit::One);
}

static void signal_ids_are_distinct() {
    sim::SignalStore store;
    sim::SignalId a = store.alloc("A", sim::Bit::Zero);
    sim::SignalId b = store.alloc("B", sim::Bit::Zero);
    assert(a != b);
}

static void to_char_renders_each_level() {
    assert(sim::to_char(sim::Bit::Zero) == '0');
    assert(sim::to_char(sim::Bit::One) == '1');
    assert(sim::to_char(sim::Bit::X) == 'X');
}

int main() {
    alloc_and_read_signal();
    alloc_defaults_to_unknown();
    set_signal_updates_value();
    signal_ids_are_distinct();
    to_char_renders_each_level();
    std::puts("signal_test: all tests passed");
    return 0;
}
