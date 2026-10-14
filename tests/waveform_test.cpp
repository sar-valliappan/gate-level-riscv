#include "waveform.hpp"

#include "cpu/alu.hpp"
#include "sim/circuit.hpp"
#include "sim/event.hpp"
#include "sim/signal.hpp"
#include "test_utils.hpp"

#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

static void renders_held_values_between_transitions() {
    waveform::Waveform wave;
    sim::SignalId a{0};
    sim::SignalId b{1};
    wave.watch(a, "A");
    wave.watch(b, "B");

    wave.record(a, 0, sim::Bit::Zero);
    wave.record(a, 5, sim::Bit::One);
    wave.record(b, 3, sim::Bit::One);

    assert(wave.render(1) == "A: 000001\nB: XXX111\n");
}

static void collapses_repeated_values_and_ignores_unwatched_signals() {
    waveform::Waveform wave;
    sim::SignalId a{0};
    wave.watch(a, "A");

    wave.record(a, 0, sim::Bit::Zero);
    wave.record(a, 1, sim::Bit::Zero); // no change: shouldn't add a transition
    wave.record(sim::SignalId{99}, 0, sim::Bit::One); // never watched: ignored
    wave.record(a, 2, sim::Bit::One);

    assert(wave.render(1) == "A: 001\n");
}

static void aligns_names_and_supports_coarser_resolution() {
    waveform::Waveform wave;
    sim::SignalId clk{0};
    sim::SignalId reset{1};
    wave.watch(clk, "clk");
    wave.watch(reset, "reset");

    wave.record(clk, 0, sim::Bit::Zero);
    wave.record(reset, 0, sim::Bit::One);
    wave.record(reset, 20, sim::Bit::Zero);

    assert(wave.render(10) == "clk  : 000\nreset: 110\n");
}

static void shows_ripple_carry_propagation_delay() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;

    constexpr std::size_t kWidth = 4;
    std::vector<sim::SignalId> a, b;
    for (std::size_t i = 0; i < kWidth; i++) {
        a.push_back(store.alloc("a" + std::to_string(i), sim::Bit::Zero));
        b.push_back(store.alloc("b" + std::to_string(i), sim::Bit::Zero));
    }
    sim::SignalId op0 = store.alloc("op0", sim::Bit::Zero);
    sim::SignalId op1 = store.alloc("op1", sim::Bit::Zero);
    alu::AluBits bits = alu::build_alu(circuit, store, a, b, {op0, op1}, /*delay=*/1);

    waveform::Waveform wave;
    for (std::size_t i = 0; i < kWidth; i++) wave.watch(bits.result[i], "SUM" + std::to_string(i));

    for (std::size_t i = 0; i < kWidth; i++) {
        queue.schedule(0, a[i], i == 0 ? sim::Bit::One : sim::Bit::Zero);
        queue.schedule(0, b[i], sim::Bit::One);
    }
    queue.schedule(0, op0, sim::Bit::Zero);
    queue.schedule(0, op1, sim::Bit::Zero);
    test_utils::settle(circuit, store, queue, 10000, &wave);

    std::string trace = wave.render(1);
    assert(!trace.empty());
    std::puts(trace.c_str());
}

int main() {
    renders_held_values_between_transitions();
    collapses_repeated_values_and_ignores_unwatched_signals();
    aligns_names_and_supports_coarser_resolution();
    shows_ripple_carry_propagation_delay();
    std::puts("waveform_test: all tests passed");
    return 0;
}
