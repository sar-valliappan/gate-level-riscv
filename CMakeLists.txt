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
)
target_include_directories(gate_level_riscv_core PUBLIC ${CMAKE_SOURCE_DIR}/src)

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
