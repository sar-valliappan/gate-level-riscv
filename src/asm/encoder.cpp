#include "asm/encoder.hpp"

#include <cassert>

namespace assembler {

namespace {

std::uint32_t encode_r_type(const isa::InstructionInfo& info, const ParsedInstruction& instr) {
    return (info.funct7 << 25) | (instr.rs2 << 20) | (instr.rs1 << 15) | (info.funct3 << 12) | (instr.rd << 7) | info.opcode;
}

std::uint32_t encode_i_type(const isa::InstructionInfo& info, const ParsedInstruction& instr) {
    std::uint32_t imm12 = static_cast<std::uint32_t>(instr.imm) & 0xFFFu;
    return (imm12 << 20) | (instr.rs1 << 15) | (info.funct3 << 12) | (instr.rd << 7) | info.opcode;
}

std::uint32_t encode_s_type(const isa::InstructionInfo& info, const ParsedInstruction& instr) {
    std::uint32_t u = static_cast<std::uint32_t>(instr.imm);
    std::uint32_t imm_hi = (u >> 5) & 0x7Fu;
    std::uint32_t imm_lo = u & 0x1Fu;
    return (imm_hi << 25) | (instr.rs2 << 20) | (instr.rs1 << 15) | (info.funct3 << 12) | (imm_lo << 7) | info.opcode;
}

std::uint32_t encode_b_type(const isa::InstructionInfo& info, const ParsedInstruction& instr) {
    std::uint32_t u = static_cast<std::uint32_t>(instr.imm); // imm[0] must be 0
    std::uint32_t bit12 = (u >> 12) & 0x1u;
    std::uint32_t bits10_5 = (u >> 5) & 0x3Fu;
    std::uint32_t bits4_1 = (u >> 1) & 0xFu;
    std::uint32_t bit11 = (u >> 11) & 0x1u;
    return (bit12 << 31) | (bits10_5 << 25) | (instr.rs2 << 20) | (instr.rs1 << 15) | (info.funct3 << 12) | (bits4_1 << 8) | (bit11 << 7) | info.opcode;
}

std::uint32_t encode_u_type(const isa::InstructionInfo& info, const ParsedInstruction& instr) {
    std::uint32_t u = static_cast<std::uint32_t>(instr.imm);
    return (u & 0xFFFFF000u) | (instr.rd << 7) | info.opcode;
}

std::uint32_t encode_j_type(const isa::InstructionInfo& info, const ParsedInstruction& instr) {
    std::uint32_t u = static_cast<std::uint32_t>(instr.imm); // imm[0] must be 0
    std::uint32_t bit20 = (u >> 20) & 0x1u;
    std::uint32_t bits19_12 = (u >> 12) & 0xFFu;
    std::uint32_t bit11 = (u >> 11) & 0x1u;
    std::uint32_t bits10_1 = (u >> 1) & 0x3FFu;
    return (bit20 << 31) | (bits19_12 << 12) | (bit11 << 20) | (bits10_1 << 21) | (instr.rd << 7) | info.opcode;
}

} // namespace

std::uint32_t encode(const ParsedInstruction& instr) {
    const isa::InstructionInfo& info = isa::lookup(instr.mnemonic);
    switch (info.format) {
        case isa::Format::R: return encode_r_type(info, instr);
        case isa::Format::I: return encode_i_type(info, instr);
        case isa::Format::S: return encode_s_type(info, instr);
        case isa::Format::B: return encode_b_type(info, instr);
        case isa::Format::U: return encode_u_type(info, instr);
        case isa::Format::J: return encode_j_type(info, instr);
    }
    assert(false && "unreachable: isa::Format covers every enumerator");
    return 0;
}

std::vector<std::uint32_t> encode(const std::vector<ParsedInstruction>& instructions) {
    std::vector<std::uint32_t> program;
    program.reserve(instructions.size());
    for (const ParsedInstruction& instr : instructions) program.push_back(encode(instr));
    return program;
}

} // namespace assembler
