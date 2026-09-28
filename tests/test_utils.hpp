#pragma once
// Test-only helper.

#include <initializer_list>

#include "sim/circuit.hpp"
#include "sim/event.hpp"
#include "sim/signal.hpp"

namespace test_utils {

/// Drains the event queue, applying each event and re-triggering fanout until no events remain.
void settle(const sim::Circuit& circuit, sim::SignalStore& store, sim::EventQueue& queue);

/// Deposits each signal's currently stored value onto it via a time-0 event.
/// Must be followed by `settle()` for the deposits to propagate.
void deposit(sim::SignalStore& store, sim::EventQueue& queue, std::initializer_list<sim::SignalId> signals);

/// Bootstraps a bistable feedback pair (an SR/D latch's `q`/`q_not`).
/// Force one side of the loop to a known level (not X), deposit and settle `inputs` around it, then hand control back to the gates.
void bootstrap_latch(const sim::Circuit& circuit, sim::SignalStore& store, sim::EventQueue& queue, sim::SignalId feedback_seed, sim::Bit seed_value, std::initializer_list<sim::SignalId> inputs);

/// Bootstraps a master-slave flip-flop's two bistable feedback pairs (master and slave
/// latch `q_not`s) together in one settle pass. They can't be bootstrapped one at a time
/// like a lone latch: master and slave gates both read the raw clock directly, so the
/// first pass's settle resolves the slave's internal nodes as a side effect before its
/// feedback pair is ever forced, leaving no later edge to retrigger its `q` gate.
void bootstrap_flipflop(const sim::Circuit& circuit, sim::SignalStore& store, sim::EventQueue& queue, sim::SignalId master_feedback, sim::SignalId slave_feedback, sim::Bit seed_value, std::initializer_list<sim::SignalId> inputs);

} // namespace test_utils
