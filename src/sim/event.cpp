#include "sim/event.hpp"

namespace sim {

namespace detail {

bool EventOrder::operator()(const Event& a, const Event& b) const {
    if (a.time != b.time) return a.time > b.time;
    return a.seq > b.seq;
}

} // namespace detail

void EventQueue::schedule(std::uint64_t time, SignalId signal, bool new_value) {
    std::uint64_t seq = next_seq_++;
    heap_.push(Event{time, seq, signal, new_value});
}

std::optional<Event> EventQueue::pop() {
    if (heap_.empty()) return std::nullopt;
    Event ev = heap_.top();
    heap_.pop();
    current_time = ev.time;
    return ev;
}

bool EventQueue::is_empty() const { return heap_.empty(); }

} // namespace sim
