#pragma once

#include <cstdint>
#include <optional>
#include <queue>
#include <vector>

#include "sim/signal.hpp"

namespace sim {

struct Event {
    std::uint64_t time;
    std::uint64_t seq;          // insertion-order
    SignalId signal;
    Bit new_value;
};

namespace detail {

// Reverse comparator: std::priority_queue is a max-heap, so this makes it
// pop the smallest (time, seq) first.
struct EventOrder {
    bool operator()(const Event& a, const Event& b) const;
};

} // namespace detail

class EventQueue {
public:
    EventQueue() = default;

    void schedule(std::uint64_t time, SignalId signal, Bit new_value);
    std::optional<Event> pop();
    bool is_empty() const;

    std::uint64_t current_time = 0;

private:
    std::priority_queue<Event, std::vector<Event>, detail::EventOrder> heap_;
    std::uint64_t next_seq_ = 0;
};

} // namespace sim
