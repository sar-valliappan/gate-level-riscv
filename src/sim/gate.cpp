#include "sim/gate.hpp"

namespace sim {

bool operator==(GateId a, GateId b) { return a.value == b.value; }
bool operator!=(GateId a, GateId b) { return !(a == b); }

bool Gate::eval(const SignalStore& store) const {
    return !(store.get(inputs[0]) && store.get(inputs[1]));
}

} // namespace sim
