#include "asm/encoder.hpp"
#include "asm/parser.hpp"

#include <cassert>
#include <cstdio>

using assembler::encode;
using assembler::ParsedInstruction;

static void encodes_r_type() {
    ParsedInstruction add{isa::Mnemonic::Add, 3, 1, 2, 0};
    std::uint32_t word = encode(add);
    assert(isa::opcode_of(word) == isa::opcode::kOp);
    assert(isa::funct3_of(word) == isa::funct3::kAddSub);
    assert(isa::funct7_of(word) == isa::funct7::kAdd);
    assert(isa::rd_of(word) == 3);
    assert(isa::rs1_of(word) == 1);
    assert(isa::rs2_of(word) == 2);

    ParsedInstruction sub{isa::Mnemonic::Sub, 3, 1, 2, 0};
    assert(isa::funct7_of(encode(sub)) == isa::funct7::kSub);
}

static void encodes_i_type_alu() {
    ParsedInstruction addi{isa::Mnemonic::Addi, 1, 0, 0, -1};
    std::uint32_t word = encode(addi);
    assert(isa::opcode_of(word) == isa::opcode::kOpImm);
    assert(isa::funct3_of(word) == isa::funct3::kAddi);
    assert(isa::rd_of(word) == 1);
    assert(isa::rs1_of(word) == 0);
    assert((word >> 20) == 0xFFFu); // imm[11:0], sign-extended -1
}

static void encodes_load_and_store() {
    ParsedInstruction load{isa::Mnemonic::Lw, 5, 2, 0, 8};
    std::uint32_t lw_word = encode(load);
    assert(isa::opcode_of(lw_word) == isa::opcode::kLoad);
    assert(isa::rd_of(lw_word) == 5);
    assert(isa::rs1_of(lw_word) == 2);
    assert((lw_word >> 20) == 8u);

    ParsedInstruction store{isa::Mnemonic::Sw, 0, 6, 3, -4};
    std::uint32_t sw_word = encode(store);
    assert(isa::opcode_of(sw_word) == isa::opcode::kStore);
    assert(isa::rs1_of(sw_word) == 6);
    assert(isa::rs2_of(sw_word) == 3);
    std::uint32_t s_imm = ((sw_word >> 25) << 5) | ((sw_word >> 7) & 0x1Fu);
    assert(static_cast<std::int32_t>(s_imm << 20) >> 20 == -4); // sign-extend from bit 11
}

static void encodes_branch() {
    ParsedInstruction beq{isa::Mnemonic::Beq, 0, 1, 2, 8};
    std::uint32_t word = encode(beq);
    assert(isa::opcode_of(word) == isa::opcode::kBranch);
    assert(isa::funct3_of(word) == isa::funct3::kBeq);
    assert(isa::rs1_of(word) == 1);
    assert(isa::rs2_of(word) == 2);

    ParsedInstruction blt{isa::Mnemonic::Blt, 0, 1, 2, -16};
    assert(isa::funct3_of(encode(blt)) == isa::funct3::kBlt);
}

static void encodes_u_and_j_type() {
    ParsedInstruction lui{isa::Mnemonic::Lui, 9, 0, 0, static_cast<std::int32_t>(0x12345000u)};
    std::uint32_t lui_word = encode(lui);
    assert(isa::opcode_of(lui_word) == isa::opcode::kLui);
    assert(isa::rd_of(lui_word) == 9);
    assert((lui_word & 0xFFFFF000u) == 0x12345000u);

    ParsedInstruction jal{isa::Mnemonic::Jal, 10, 0, 0, 8};
    std::uint32_t jal_word = encode(jal);
    assert(isa::opcode_of(jal_word) == isa::opcode::kJal);
    assert(isa::rd_of(jal_word) == 10);
}

static void round_trips_a_small_program() {
    auto program = assembler::parse(R"(
loop:
    addi x1, x0, 1
    beq x1, x0, done
    jal x0, loop
done:
    sw x1, 0(x0)
)");
    auto words = encode(program);
    assert(words.size() == program.size());

    for (std::size_t i = 0; i < words.size(); i++) {
        isa::Mnemonic expected = program[i].mnemonic;
        auto decoded = isa::decode(isa::opcode_of(words[i]), isa::funct3_of(words[i]), isa::funct7_of(words[i]));
        assert(decoded.has_value());
        assert(*decoded == expected);
    }
}

int main() {
    encodes_r_type();
    encodes_i_type_alu();
    encodes_load_and_store();
    encodes_branch();
    encodes_u_and_j_type();
    round_trips_a_small_program();
    std::puts("encoder_test: all tests passed");
    return 0;
}
