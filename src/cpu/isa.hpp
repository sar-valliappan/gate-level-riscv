#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace isa {

/// The RISC-V instruction formats in this project.
enum class Format : std::uint8_t { R, I, S, B, U, J };

/// The instruction subset this project implements.
enum class Mnemonic : std::uint8_t {
    Add,
    Sub,
    Addi,
    Andi,
    Lw,
    Sw,
    Beq,
    Blt,
    Lui,
    Jal,
};

/// 7-bit base opcodes, i.e. instr[6:0].
namespace opcode {
constexpr std::uint32_t kOp = 0b0110011;     
constexpr std::uint32_t kOpImm = 0b0010011;  
constexpr std::uint32_t kLoad = 0b0000011;   
constexpr std::uint32_t kStore = 0b0100011; 
constexpr std::uint32_t kBranch = 0b1100011;
constexpr std::uint32_t kLui = 0b0110111; 
constexpr std::uint32_t kJal = 0b1101111; 
} // namespace opcode

/// 3-bit funct3 field, i.e. instr[14:12].
namespace funct3 {
constexpr std::uint32_t kAddSub = 0b000;
constexpr std::uint32_t kAddi = 0b000;
constexpr std::uint32_t kAndi = 0b111;
constexpr std::uint32_t kLw = 0b010;
constexpr std::uint32_t kSw = 0b010;
constexpr std::uint32_t kBeq = 0b000;
constexpr std::uint32_t kBlt = 0b100;
} // namespace funct3

/// 7-bit funct7 field, i.e. instr[31:25].
namespace funct7 {
constexpr std::uint32_t kAdd = 0b0000000;
constexpr std::uint32_t kSub = 0b0100000;
} // namespace funct7

/// Encoding fields identifying one instruction.
struct InstructionInfo {
    Mnemonic mnemonic;
    Format format;
    std::uint32_t opcode;
    std::uint32_t funct3;
    std::uint32_t funct7;
};

/// The implemented subset's encoding table, indexed by Mnemonic.
const std::array<InstructionInfo, 10>& instruction_table();

/// Looks up a mnemonic's encoding fields.
const InstructionInfo& lookup(Mnemonic mnemonic);

/// Decodes raw (opcode, funct3, funct7) fields into a mnemonic.
std::optional<Mnemonic> decode(std::uint32_t opcode, std::uint32_t funct3, std::uint32_t funct7);

std::uint32_t opcode_of(std::uint32_t instr);
std::uint32_t rd_of(std::uint32_t instr);
std::uint32_t funct3_of(std::uint32_t instr);
std::uint32_t rs1_of(std::uint32_t instr);
std::uint32_t rs2_of(std::uint32_t instr);
std::uint32_t funct7_of(std::uint32_t instr);

} // namespace isa
