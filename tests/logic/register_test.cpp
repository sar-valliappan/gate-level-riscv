#include "logic/register.hpp"
#include "sim/circuit.hpp"
#include "sim/event.hpp"
#include "sim/signal.hpp"
#include "test_utils.hpp"

#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

namespace {

constexpr std::size_t kWidth = 4;

struct Register {
    std::vector<sim::SignalId> d;
    sim::SignalId clk;
    std::vector<sim::SignalId> q;
    std::vector<sim::SignalId> q_not;
};

void set_bits(sim::EventQueue& queue, const std::vector<sim::SignalId>& bits, unsigned value) {
    for (std::size_t i = 0; i < bits.size(); i++) {
        sim::Bit b = ((value >> i) & 1) ? sim::Bit::One : sim::Bit::Zero;
        queue.schedule(queue.current_time + 10, bits[i], b);
    }
}

bool bits_equal(const sim::SignalStore& store, const std::vector<sim::SignalId>& bits, unsigned value) {
    for (std::size_t i = 0; i < bits.size(); i++) {
        sim::Bit expected = ((value >> i) & 1) ? sim::Bit::One : sim::Bit::Zero;
        if (store.get(bits[i]) != expected) return false;
    }
    return true;
}

Register make_register(sim::Circuit& circuit, sim::SignalStore& store, sim::EventQueue& queue) {
    std::vector<sim::SignalId> d;
    for (std::size_t i = 0; i < kWidth; i++) {
        d.push_back(store.alloc("D" + std::to_string(i), sim::Bit::Zero));
    }
    sim::SignalId clk = store.alloc("CLK", sim::Bit::Zero);

    logic::RegisterBits bits = logic::build_register(circuit, store, d, clk, 1);

    test_utils::bootstrap_register(circuit, store, queue, bits.master_q_not, bits.q_not, sim::Bit::One,
        {d[0], d[1], d[2], d[3], clk});

    return {d, clk, bits.q, bits.q_not};
}

} // namespace

static void register_captures_all_bits_on_rising_edge() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    Register reg = make_register(circuit, store, queue);

    set_bits(queue, reg.d, 0b1011);
    test_utils::settle(circuit, store, queue);
    assert(bits_equal(store, reg.q, 0b0000)); // clk still low: not captured yet

    queue.schedule(queue.current_time + 10, reg.clk, sim::Bit::One); // rising edge
    test_utils::settle(circuit, store, queue);

    assert(bits_equal(store, reg.q, 0b1011));
}

static void register_ignores_d_while_clk_high() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    Register reg = make_register(circuit, store, queue);

    set_bits(queue, reg.d, 0b1010);
    queue.schedule(queue.current_time + 10, reg.clk, sim::Bit::One);
    test_utils::settle(circuit, store, queue);
    assert(bits_equal(store, reg.q, 0b1010));

    set_bits(queue, reg.d, 0b0101); // changes mid-cycle, while clk is still high
    test_utils::settle(circuit, store, queue);
    assert(bits_equal(store, reg.q, 0b1010)); // unaffected
}

static void register_holds_across_falling_edge_and_loads_on_next_edge() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    Register reg = make_register(circuit, store, queue);

    set_bits(queue, reg.d, 0b1010);
    queue.schedule(queue.current_time + 10, reg.clk, sim::Bit::One);
    test_utils::settle(circuit, store, queue);
    assert(bits_equal(store, reg.q, 0b1010));

    queue.schedule(queue.current_time + 10, reg.clk, sim::Bit::Zero); // falling edge
    set_bits(queue, reg.d, 0b0101);
    test_utils::settle(circuit, store, queue);
    assert(bits_equal(store, reg.q, 0b1010)); // still holding the first captured value

    queue.schedule(queue.current_time + 10, reg.clk, sim::Bit::One); // second rising edge
    test_utils::settle(circuit, store, queue);
    assert(bits_equal(store, reg.q, 0b0101));
}

int main() {
    register_captures_all_bits_on_rising_edge();
    register_ignores_d_while_clk_high();
    register_holds_across_falling_edge_and_loads_on_next_edge();
    std::puts("register_test: all tests passed");
    return 0;
}
