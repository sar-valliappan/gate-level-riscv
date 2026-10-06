#include "test_utils.hpp"

#include <cstdio>
#include <cstdlib>
#include <unordered_map>

namespace test_utils {

namespace {

constexpr std::size_t kDefaultMaxEvents = 10000;

} // namespace

void settle(const sim::Circuit& circuit, sim::SignalStore& store, sim::EventQueue& queue) {
    std::unordered_map<std::uint32_t, sim::Bit> pending;

    auto current_target = [&](sim::SignalId sig) {
        auto it = pending.find(sig.value);
        return it != pending.end() ? it->second : store.get(sig);
    };

    std::size_t events_processed = 0;
    while (auto event = queue.pop()) {
        events_processed++;
        if (events_processed > kDefaultMaxEvents) {
            std::fprintf(stderr, "did not settle within %zu events\n", kDefaultMaxEvents);
            std::abort();
        }

        auto it = pending.find(event->signal.value);
        if (it != pending.end() && it->second != event->new_value) {
            continue; // superseded by a later re-evaluation of this signal; discard
        }
        if (it != pending.end()) pending.erase(it);

        store.set(event->signal, event->new_value);
        for (sim::GateId gate_id : circuit.fanout_of(event->signal)) {
            const sim::Gate& gate = circuit.gate(gate_id);
            sim::Bit new_output = gate.eval(store);
            if (new_output != current_target(gate.output)) {
                queue.schedule(queue.current_time + gate.delay, gate.output, new_output);
                pending[gate.output.value] = new_output;
            }
        }
    }
}

void deposit(sim::SignalStore& store, sim::EventQueue& queue, const std::vector<sim::SignalId>& signals) {
    for (sim::SignalId sig : signals) {
        queue.schedule(0, sig, store.get(sig));
    }
}

void bootstrap_latch(const sim::Circuit& circuit, sim::SignalStore& store, sim::EventQueue& queue, sim::SignalId feedback_seed, sim::Bit seed_value, const std::vector<sim::SignalId>& inputs) {
    store.force(feedback_seed, seed_value);
    deposit(store, queue, inputs);
    settle(circuit, store, queue);
    store.release(feedback_seed);
}

void bootstrap_flipflop(const sim::Circuit& circuit, sim::SignalStore& store, sim::EventQueue& queue, sim::SignalId master_feedback, sim::SignalId slave_feedback, sim::Bit seed_value, const std::vector<sim::SignalId>& inputs) {
    store.force(master_feedback, seed_value);
    store.force(slave_feedback, seed_value);
    deposit(store, queue, inputs);
    settle(circuit, store, queue);
    store.release(master_feedback);
    store.release(slave_feedback);
}

void bootstrap_register(const sim::Circuit& circuit, sim::SignalStore& store, sim::EventQueue& queue, const std::vector<sim::SignalId>& master_feedbacks, const std::vector<sim::SignalId>& slave_feedbacks, sim::Bit seed_value, const std::vector<sim::SignalId>& inputs) {
    for (sim::SignalId feedback : master_feedbacks) store.force(feedback, seed_value);
    for (sim::SignalId feedback : slave_feedbacks) store.force(feedback, seed_value);
    deposit(store, queue, inputs);
    settle(circuit, store, queue);
    for (sim::SignalId feedback : master_feedbacks) store.release(feedback);
    for (sim::SignalId feedback : slave_feedbacks) store.release(feedback);
}

} // namespace test_utils
