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

/// Driven low, driven high, or unknown.
enum class Bit : std::uint8_t {
    Zero,
    One,
    X,
};

/// Renders a Bit as '0', '1', or 'X' for logging.
char to_char(Bit bit);

/// Signal base
struct Signal {
    std::string name;
    Bit value;
};

/// A collection of signals in the simulation
class SignalStore {
public:
    SignalStore() = default;

    SignalId alloc(std::string name, Bit initial);
    Bit get(SignalId id) const;
    void set(SignalId id, Bit value);
    const std::string& name(SignalId id) const;

private:
    std::vector<Signal> signals_;
};

} // namespace sim
