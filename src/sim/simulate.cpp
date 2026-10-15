#include "sim/simulate.hpp"

#include <cstdio>
#include <cstdlib>
#include <unordered_map>

namespace sim {

void settle(const Circuit& circuit, SignalStore& store, EventQueue& queue, std::size_t max_events, waveform::Waveform* trace) {
    std::unordered_map<std::uint32_t, Bit> pending;

    auto current_target = [&](SignalId sig) {
        auto it = pending.find(sig.value);
        return it != pending.end() ? it->second : store.get(sig);
    };

    std::size_t events_processed = 0;
    while (auto event = queue.pop()) {
        events_processed++;
        if (events_processed > max_events) {
            std::fprintf(stderr, "did not settle within %zu events\n", max_events);
            std::abort();
        }

        auto it = pending.find(event->signal.value);
        if (it != pending.end() && it->second != event->new_value) {
            continue; // superseded by a later re-evaluation of this signal; discard
        }
        if (it != pending.end()) pending.erase(it);

        store.set(event->signal, event->new_value);
        if (trace) trace->record(event->signal, event->time, event->new_value);
        for (GateId gate_id : circuit.fanout_of(event->signal)) {
            const Gate& gate = circuit.gate(gate_id);
            Bit new_output = gate.eval(store);
            if (new_output != current_target(gate.output)) {
                queue.schedule(queue.current_time + gate.delay, gate.output, new_output);
                pending[gate.output.value] = new_output;
            }
        }
    }
}

void deposit(SignalStore& store, EventQueue& queue, const std::vector<SignalId>& signals) {
    for (SignalId sig : signals) {
        queue.schedule(0, sig, store.get(sig));
    }
}

void bootstrap_latch(const Circuit& circuit, SignalStore& store, EventQueue& queue, SignalId feedback_seed, Bit seed_value, const std::vector<SignalId>& inputs) {
    store.force(feedback_seed, seed_value);
    deposit(store, queue, inputs);
    settle(circuit, store, queue);
    store.release(feedback_seed);
}

void bootstrap_flipflop(const Circuit& circuit, SignalStore& store, EventQueue& queue, SignalId master_feedback, SignalId slave_feedback, Bit seed_value, const std::vector<SignalId>& inputs) {
    store.force(master_feedback, seed_value);
    store.force(slave_feedback, seed_value);
    deposit(store, queue, inputs);
    settle(circuit, store, queue);
    store.release(master_feedback);
    store.release(slave_feedback);
}

void bootstrap_register(const Circuit& circuit, SignalStore& store, EventQueue& queue, const std::vector<SignalId>& master_feedbacks, const std::vector<SignalId>& slave_feedbacks, Bit seed_value, const std::vector<SignalId>& inputs, std::size_t max_events) {
    for (SignalId feedback : master_feedbacks) store.force(feedback, seed_value);
    for (SignalId feedback : slave_feedbacks) store.force(feedback, seed_value);
    deposit(store, queue, inputs);
    settle(circuit, store, queue, max_events);
    for (SignalId feedback : master_feedbacks) store.release(feedback);
    for (SignalId feedback : slave_feedbacks) store.release(feedback);
}

} // namespace sim
