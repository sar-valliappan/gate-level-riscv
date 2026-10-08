#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "cpu/isa.hpp"

namespace assembler {

/// One parsed instruction: a mnemonic plus resolved operands.
struct ParsedInstruction {
    isa::Mnemonic mnemonic;
    std::uint32_t rd = 0;
    std::uint32_t rs1 = 0;
    std::uint32_t rs2 = 0;
    std::int32_t imm = 0;
};

/// Two-pass parse of a RISC-V-assembly-style source into labeled instructions.
std::vector<ParsedInstruction> parse(const std::string& source);

} // namespace assembler
