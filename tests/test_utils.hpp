#pragma once
// Test-only helper.

#include <vector>

#include "sim/circuit.hpp"
#include "sim/event.hpp"
#include "sim/signal.hpp"

namespace test_utils {

/// Drains the event queue, applying each event and re-triggering fanout until no events remain.
void settle(const sim::Circuit& circuit, sim::SignalStore& store, sim::EventQueue& queue);

/// Deposits each signal's currently stored value onto it via a time-0 event.
/// Must be followed by `settle()` for the deposits to propagate.
void deposit(sim::SignalStore& store, sim::EventQueue& queue, const std::vector<sim::SignalId>& signals);

/// Bootstraps one bistable feedback pair (an SR/D latch's `q`/`q_not`): force, deposit+settle `inputs`, release.
void bootstrap_latch(const sim::Circuit& circuit, sim::SignalStore& store, sim::EventQueue& queue, sim::SignalId feedback_seed, sim::Bit seed_value, const std::vector<sim::SignalId>& inputs);

/// Bootstraps a flip-flop's master and slave feedback pairs together (must be forced in the same pass; see DECISIONS.md).
void bootstrap_flipflop(const sim::Circuit& circuit, sim::SignalStore& store, sim::EventQueue& queue, sim::SignalId master_feedback, sim::SignalId slave_feedback, sim::Bit seed_value, const std::vector<sim::SignalId>& inputs);

/// Bootstraps every bit of a `build_register` output together, same as `bootstrap_flipflop` but N-wide.
void bootstrap_register(const sim::Circuit& circuit, sim::SignalStore& store, sim::EventQueue& queue, const std::vector<sim::SignalId>& master_feedbacks, const std::vector<sim::SignalId>& slave_feedbacks, sim::Bit seed_value, const std::vector<sim::SignalId>& inputs);

} // namespace test_utils
