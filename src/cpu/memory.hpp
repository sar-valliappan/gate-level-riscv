#pragma once

#include <cstdint>
#include <vector>

namespace cpu {

/// Word-addressed instruction memory and byte-addressed data memory, kept as
/// plain data rather than simulated at the gate level (see docs/DESIGN.md,
/// "Memory model"). The gate-level datapath only ever sees this through its
/// mem_addr / mem_read_data / mem_write ports; the caller is responsible for
/// shuttling values between the two each cycle.
class Memory {
public:
    explicit Memory(std::vector<std::uint32_t> program);

    /// Fetches the instruction word at byte address `pc`. Out-of-range reads
    /// return 0, matching how a real instruction stream runs off the end of
    /// the loaded program into untouched (zeroed) memory.
    std::uint32_t read_instruction(std::uint32_t pc) const;

    /// Reads the data word at byte address `addr`. Out-of-range reads return
    /// 0; the datapath samples this every cycle regardless of whether the
    /// current instruction actually uses it, so this must never crash.
    std::uint32_t read_word(std::uint32_t addr) const;

    /// Writes the data word at byte address `addr`. Out-of-range writes are
    /// silently ignored, for the same reason reads are tolerant.
    void write_word(std::uint32_t addr, std::uint32_t value);

private:
    std::vector<std::uint32_t> instructions_;
    std::vector<std::uint32_t> data_;
};

} // namespace cpu
