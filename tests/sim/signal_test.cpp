#include "sim/signal.hpp"

#include <cassert>
#include <cstdio>

static void alloc_and_read_signal() {
    sim::SignalStore store;
    sim::SignalId a = store.alloc("A", false);
    sim::SignalId b = store.alloc("B", true);

    assert(store.get(a) == false);
    assert(store.get(b) == true);
}

static void set_signal_updates_value() {
    sim::SignalStore store;
    sim::SignalId a = store.alloc("A", false);
    store.set(a, true);
    assert(store.get(a) == true);
}

static void signal_ids_are_distinct() {
    sim::SignalStore store;
    sim::SignalId a = store.alloc("A", false);
    sim::SignalId b = store.alloc("B", false);
    assert(a != b);
}

int main() {
    alloc_and_read_signal();
    set_signal_updates_value();
    signal_ids_are_distinct();
    std::puts("signal_test: all tests passed");
    return 0;
}
