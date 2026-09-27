#include "sim/signal.hpp"

#include <utility>

namespace sim {

bool operator==(SignalId a, SignalId b) { return a.value == b.value; }
bool operator!=(SignalId a, SignalId b) { return !(a == b); }
bool operator<(SignalId a, SignalId b) { return a.value < b.value; }

char to_char(Bit bit) {
    switch (bit) {
        case Bit::Zero: return '0';
        case Bit::One: return '1';
        case Bit::X: return 'X';
    }
    return 'X';
}

SignalId SignalStore::alloc(std::string name, Bit initial) {
    SignalId id{static_cast<std::uint32_t>(signals_.size())};
    signals_.push_back(Signal{std::move(name), initial});
    return id;
}

Bit SignalStore::get(SignalId id) const { return signals_[id.value].value; }

void SignalStore::set(SignalId id, Bit value) { signals_[id.value].value = value; }

const std::string& SignalStore::name(SignalId id) const { return signals_[id.value].name; }

} // namespace sim
