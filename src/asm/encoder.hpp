#pragma once

#include <cstdint>
#include <vector>

#include "asm/parser.hpp"

namespace assembler {

/// Encodes one parsed instruction into its 32-bit RV32I word.
std::uint32_t encode(const ParsedInstruction& instr);

/// Encodes a sequence of parsed instructions into a program image.
std::vector<std::uint32_t> encode(const std::vector<ParsedInstruction>& instructions);

} // namespace assembler
