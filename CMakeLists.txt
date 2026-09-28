cmake_minimum_required(VERSION 3.16)
project(gate-level-riscv CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(gate_level_riscv_core
    src/sim/signal.cpp
    src/sim/event.cpp
    src/sim/gate.cpp
    src/sim/circuit.cpp
    src/logic/gates.cpp
    src/logic/latch.cpp
    src/logic/flipflop.cpp
)
target_include_directories(gate_level_riscv_core PUBLIC ${CMAKE_SOURCE_DIR}/src)

add_library(test_utils tests/test_utils.cpp)
target_include_directories(test_utils PUBLIC ${CMAKE_SOURCE_DIR}/tests)
target_link_libraries(test_utils PUBLIC gate_level_riscv_core)

add_executable(gate-level-riscv src/main.cpp)
target_link_libraries(gate-level-riscv PRIVATE gate_level_riscv_core)

enable_testing()

add_executable(signal_test tests/sim/signal_test.cpp)
target_link_libraries(signal_test PRIVATE gate_level_riscv_core)
add_test(NAME signal_test COMMAND signal_test)

add_executable(event_test tests/sim/event_test.cpp)
target_link_libraries(event_test PRIVATE gate_level_riscv_core)
add_test(NAME event_test COMMAND event_test)

add_executable(gate_test tests/sim/gate_test.cpp)
target_link_libraries(gate_test PRIVATE gate_level_riscv_core)
add_test(NAME gate_test COMMAND gate_test)

add_executable(circuit_test tests/sim/circuit_test.cpp)
target_link_libraries(circuit_test PRIVATE gate_level_riscv_core)
add_test(NAME circuit_test COMMAND circuit_test)

add_executable(gates_test tests/logic/gates_test.cpp)
target_link_libraries(gates_test PRIVATE test_utils)
add_test(NAME gates_test COMMAND gates_test)

add_executable(latch_test tests/logic/latch_test.cpp)
target_link_libraries(latch_test PRIVATE test_utils)
add_test(NAME latch_test COMMAND latch_test)

add_executable(flipflop_test tests/logic/flipflop_test.cpp)
target_link_libraries(flipflop_test PRIVATE test_utils)
add_test(NAME flipflop_test COMMAND flipflop_test)
