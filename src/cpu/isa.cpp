#include "cpu/isa.hpp"

namespace isa {

const std::array<InstructionInfo, 10>& instruction_table() {
    static const std::array<InstructionInfo, 10> table = {{
        {Mnemonic::Add, Format::R, opcode::kOp, funct3::kAddSub, funct7::kAdd},
        {Mnemonic::Sub, Format::R, opcode::kOp, funct3::kAddSub, funct7::kSub},
        {Mnemonic::Addi, Format::I, opcode::kOpImm, funct3::kAddi, 0},
        {Mnemonic::Andi, Format::I, opcode::kOpImm, funct3::kAndi, 0},
        {Mnemonic::Lw, Format::I, opcode::kLoad, funct3::kLw, 0},
        {Mnemonic::Sw, Format::S, opcode::kStore, funct3::kSw, 0},
        {Mnemonic::Beq, Format::B, opcode::kBranch, funct3::kBeq, 0},
        {Mnemonic::Blt, Format::B, opcode::kBranch, funct3::kBlt, 0},
        {Mnemonic::Lui, Format::U, opcode::kLui, 0, 0},
        {Mnemonic::Jal, Format::J, opcode::kJal, 0, 0},
    }};
    return table;
}

const InstructionInfo& lookup(Mnemonic mnemonic) {
    const auto& table = instruction_table();
    return table[static_cast<std::size_t>(mnemonic)];
}

std::optional<Mnemonic> decode(std::uint32_t opcode, std::uint32_t funct3, std::uint32_t funct7) {
    for (const InstructionInfo& info : instruction_table()) {
        if (info.opcode != opcode) continue;

        switch (info.format) {
            case Format::U:
            case Format::J:
                return info.mnemonic; // no funct3/funct7 to disambiguate
            case Format::R:
                if (info.funct3 == funct3 && info.funct7 == funct7) return info.mnemonic;
                break;
            case Format::I:
            case Format::S:
            case Format::B:
                if (info.funct3 == funct3) return info.mnemonic;
                break;
        }
    }
    return std::nullopt;
}

std::uint32_t opcode_of(std::uint32_t instr) { return instr & 0x7F; }
std::uint32_t rd_of(std::uint32_t instr) { return (instr >> 7) & 0x1F; }
std::uint32_t funct3_of(std::uint32_t instr) { return (instr >> 12) & 0x7; }
std::uint32_t rs1_of(std::uint32_t instr) { return (instr >> 15) & 0x1F; }
std::uint32_t rs2_of(std::uint32_t instr) { return (instr >> 20) & 0x1F; }
std::uint32_t funct7_of(std::uint32_t instr) { return (instr >> 25) & 0x7F; }

} // namespace isa
