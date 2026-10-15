cmake_minimum_required(VERSION 3.16)
project(gate-level-riscv CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(gate_level_riscv_core
    src/sim/signal.cpp
    src/sim/event.cpp
    src/sim/gate.cpp
    src/sim/circuit.cpp
    src/sim/simulate.cpp
    src/logic/gates.cpp
    src/logic/latch.cpp
    src/logic/flipflop.cpp
    src/logic/register.cpp
    src/logic/mux.cpp
    src/cpu/isa.cpp
    src/cpu/alu.cpp
    src/cpu/immgen.cpp
    src/cpu/control.cpp
    src/cpu/memory.cpp
    src/cpu/datapath.cpp
    src/cpu/runner.cpp
    src/asm/parser.cpp
    src/asm/encoder.cpp
    src/waveform.cpp
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

add_executable(register_test tests/logic/register_test.cpp)
target_link_libraries(register_test PRIVATE test_utils)
add_test(NAME register_test COMMAND register_test)

add_executable(register_file_test tests/logic/register_file_test.cpp)
target_link_libraries(register_file_test PRIVATE test_utils)
add_test(NAME register_file_test COMMAND register_file_test)

add_executable(mux_test tests/logic/mux_test.cpp)
target_link_libraries(mux_test PRIVATE test_utils)
add_test(NAME mux_test COMMAND mux_test)

add_executable(isa_test tests/cpu/isa_test.cpp)
target_link_libraries(isa_test PRIVATE gate_level_riscv_core)
add_test(NAME isa_test COMMAND isa_test)

add_executable(alu_test tests/cpu/alu_test.cpp)
target_link_libraries(alu_test PRIVATE test_utils)
add_test(NAME alu_test COMMAND alu_test)

add_executable(immgen_test tests/cpu/immgen_test.cpp)
target_link_libraries(immgen_test PRIVATE test_utils)
add_test(NAME immgen_test COMMAND immgen_test)

add_executable(control_test tests/cpu/control_test.cpp)
target_link_libraries(control_test PRIVATE test_utils)
add_test(NAME control_test COMMAND control_test)

add_executable(memory_test tests/cpu/memory_test.cpp)
target_link_libraries(memory_test PRIVATE gate_level_riscv_core)
add_test(NAME memory_test COMMAND memory_test)

add_executable(datapath_test tests/cpu/datapath_test.cpp)
target_link_libraries(datapath_test PRIVATE gate_level_riscv_core)
add_test(NAME datapath_test COMMAND datapath_test)

add_executable(parser_test tests/asm/parser_test.cpp)
target_link_libraries(parser_test PRIVATE gate_level_riscv_core)
add_test(NAME parser_test COMMAND parser_test)

add_executable(encoder_test tests/asm/encoder_test.cpp)
target_link_libraries(encoder_test PRIVATE gate_level_riscv_core)
add_test(NAME encoder_test COMMAND encoder_test)

add_executable(waveform_test tests/waveform_test.cpp)
target_link_libraries(waveform_test PRIVATE test_utils)
add_test(NAME waveform_test COMMAND waveform_test)
