#pragma once
// Test-only re-export of sim::simulate under the test_utils:: name tests already use.

#include "sim/simulate.hpp"

namespace test_utils {

using sim::settle;
using sim::deposit;
using sim::bootstrap_latch;
using sim::bootstrap_flipflop;
using sim::bootstrap_register;

} // namespace test_utils
