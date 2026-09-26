#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sim {

/// Unique identifier for each signal
struct SignalId {
    std::uint32_t value;

    friend bool operator==(SignalId a, SignalId b);
    friend bool operator!=(SignalId a, SignalId b);
    friend bool operator<(SignalId a, SignalId b);
};

/// Signal base
struct Signal {
    std::string name;
    bool value;
};

/// A collection of signals in the simulation
class SignalStore {
public:
    SignalStore() = default;

    SignalId alloc(std::string name, bool initial);
    bool get(SignalId id) const;
    void set(SignalId id, bool value);
    const std::string& name(SignalId id) const;

private:
    std::vector<Signal> signals_;
};

} // namespace sim
