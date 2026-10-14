#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "sim/signal.hpp"

namespace waveform {

/// Records watched signals' value transitions over simulated time and renders them as an ASCII timing diagram.
class Waveform {
public:
    void watch(sim::SignalId id, std::string name);

    void record(sim::SignalId id, std::uint64_t time, sim::Bit value);

    std::string render(std::uint64_t resolution = 1) const;

private:
    struct Trace {
        std::string name;
        std::vector<std::pair<std::uint64_t, sim::Bit>> transitions;
    };

    std::unordered_map<std::uint32_t, std::size_t> index_by_signal_;
    std::vector<Trace> traces_;
};

} // namespace waveform
