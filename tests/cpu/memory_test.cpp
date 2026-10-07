#include "cpu/memory.hpp"

#include <cassert>
#include <cstdio>

static void fetches_instructions_by_word_address() {
    cpu::Memory memory({0x11111111u, 0x22222222u, 0x33333333u});

    assert(memory.read_instruction(0) == 0x11111111u);
    assert(memory.read_instruction(4) == 0x22222222u);
    assert(memory.read_instruction(8) == 0x33333333u);
}

static void fetches_past_the_end_of_the_program_as_zero() {
    cpu::Memory memory({0x11111111u});

    assert(memory.read_instruction(4) == 0u);
    assert(memory.read_instruction(1000) == 0u);
}

static void data_memory_starts_zeroed() {
    cpu::Memory memory({});

    assert(memory.read_word(0) == 0u);
    assert(memory.read_word(400) == 0u);
}

static void write_then_read_same_word() {
    cpu::Memory memory({});

    memory.write_word(0, 0xDEADBEEFu);
    memory.write_word(4, 42u);

    assert(memory.read_word(0) == 0xDEADBEEFu);
    assert(memory.read_word(4) == 42u);
    assert(memory.read_word(8) == 0u);
}

static void out_of_range_access_is_tolerated() {
    cpu::Memory memory({});

    assert(memory.read_word(1u << 20) == 0u);
    memory.write_word(1u << 20, 0xFFu); // must not crash
}

int main() {
    fetches_instructions_by_word_address();
    fetches_past_the_end_of_the_program_as_zero();
    data_memory_starts_zeroed();
    write_then_read_same_word();
    out_of_range_access_is_tolerated();
    std::puts("memory_test: all tests passed");
    return 0;
}
