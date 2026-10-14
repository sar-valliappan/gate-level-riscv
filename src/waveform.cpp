#include "waveform.hpp"

#include <algorithm>
#include <cassert>

namespace waveform {

void Waveform::watch(sim::SignalId id, std::string name) {
    index_by_signal_[id.value] = traces_.size();
    traces_.push_back(Trace{std::move(name), {}});
}

void Waveform::record(sim::SignalId id, std::uint64_t time, sim::Bit value) {
    auto it = index_by_signal_.find(id.value);
    if (it == index_by_signal_.end()) return;

    Trace& trace = traces_[it->second];
    if (!trace.transitions.empty() && trace.transitions.back().second == value) return;
    trace.transitions.emplace_back(time, value);
}

std::string Waveform::render(std::uint64_t resolution) const {
    assert(resolution > 0);

    std::uint64_t max_time = 0;
    std::size_t name_width = 0;
    for (const Trace& trace : traces_) {
        if (!trace.transitions.empty()) max_time = std::max(max_time, trace.transitions.back().first);
        name_width = std::max(name_width, trace.name.size());
    }

    std::string out;
    for (const Trace& trace : traces_) {
        out += trace.name + std::string(name_width - trace.name.size(), ' ') + ": ";

        std::size_t next = 0;
        sim::Bit held = sim::Bit::X;
        for (std::uint64_t t = 0; t <= max_time; t += resolution) {
            while (next < trace.transitions.size() && trace.transitions[next].first <= t) {
                held = trace.transitions[next].second;
                next++;
            }
            out += sim::to_char(held);
        }
        out += '\n';
    }
    return out;
}

} // namespace waveform
