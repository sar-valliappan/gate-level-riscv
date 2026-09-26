#include "sim/event.hpp"
#include "sim/signal.hpp"

#include <cassert>
#include <cstdio>

static void pops_in_time_order() {
    sim::EventQueue q;
    sim::SignalId sig{0};
    q.schedule(10, sig, true);
    q.schedule(5, sig, false);
    q.schedule(7, sig, true);

    assert(q.pop()->time == 5);
    assert(q.pop()->time == 7);
    assert(q.pop()->time == 10);
}

static void same_timestamp_pops_in_seq_order() {
    sim::EventQueue q;
    sim::SignalId sig{0};
    q.schedule(5, sig, true);
    q.schedule(5, sig, false);

    assert(q.pop()->new_value == true);  // scheduled first
    assert(q.pop()->new_value == false);
}

static void empty_queue_returns_none() {
    sim::EventQueue q;
    assert(!q.pop().has_value());
}

static void current_time_advances_on_pop() {
    sim::EventQueue q;
    q.schedule(42, sim::SignalId{0}, true);
    q.pop();
    assert(q.current_time == 42);
}

int main() {
    pops_in_time_order();
    same_timestamp_pops_in_seq_order();
    empty_queue_returns_none();
    current_time_advances_on_pop();
    std::puts("event_test: all tests passed");
    return 0;
}
