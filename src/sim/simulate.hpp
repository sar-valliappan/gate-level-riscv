#pragma once

#include <cstddef>
#include <vector>

#include "sim/circuit.hpp"
#include "sim/event.hpp"
#include "sim/signal.hpp"
#include "waveform.hpp"

namespace sim {

void settle(const Circuit& circuit, SignalStore& store, EventQueue& queue, std::size_t max_events = 10000, waveform::Waveform* trace = nullptr);

void deposit(SignalStore& store, EventQueue& queue, const std::vector<SignalId>& signals);

void bootstrap_latch(const Circuit& circuit, SignalStore& store, EventQueue& queue, SignalId feedback_seed, Bit seed_value, const std::vector<SignalId>& inputs);

void bootstrap_flipflop(const Circuit& circuit, SignalStore& store, EventQueue& queue, SignalId master_feedback, SignalId slave_feedback, Bit seed_value, const std::vector<SignalId>& inputs);

void bootstrap_register(const Circuit& circuit, SignalStore& store, EventQueue& queue, const std::vector<SignalId>& master_feedbacks, const std::vector<SignalId>& slave_feedbacks, Bit seed_value, const std::vector<SignalId>& inputs, std::size_t max_events = 10000);

} // namespace sim
