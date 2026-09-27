#include "sim/gate.hpp"

namespace sim {

bool operator==(GateId a, GateId b) { return a.value == b.value; }
bool operator!=(GateId a, GateId b) { return !(a == b); }

Bit Gate::eval(const SignalStore& store) const {
    Bit a = store.get(inputs[0]);
    Bit b = store.get(inputs[1]);

    // 0 NAND anything is 1
    // 1 NAND 1 is 0
    // any remaining combination with X is X
    if (a == Bit::Zero || b == Bit::Zero) return Bit::One;
    if (a == Bit::X || b == Bit::X) return Bit::X;
    return (a == Bit::One && b == Bit::One) ? Bit::Zero : Bit::One;
}

} // namespace sim
