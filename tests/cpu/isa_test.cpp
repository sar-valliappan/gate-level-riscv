#include "cpu/isa.hpp"

#include <cassert>
#include <cstdio>

static void lookup_returns_matching_mnemonic() {
    assert(isa::lookup(isa::Mnemonic::Add).mnemonic == isa::Mnemonic::Add);
    assert(isa::lookup(isa::Mnemonic::Jal).opcode == isa::opcode::kJal);
}

static void decode_disambiguates_r_type_by_funct7() {
    auto add = isa::decode(isa::opcode::kOp, isa::funct3::kAddSub, isa::funct7::kAdd);
    auto sub = isa::decode(isa::opcode::kOp, isa::funct3::kAddSub, isa::funct7::kSub);
    assert(add.has_value() && *add == isa::Mnemonic::Add);
    assert(sub.has_value() && *sub == isa::Mnemonic::Sub);
}

static void decode_disambiguates_i_type_alu_by_funct3() {
    auto addi = isa::decode(isa::opcode::kOpImm, isa::funct3::kAddi, 0);
    auto andi = isa::decode(isa::opcode::kOpImm, isa::funct3::kAndi, 0);
    assert(addi.has_value() && *addi == isa::Mnemonic::Addi);
    assert(andi.has_value() && *andi == isa::Mnemonic::Andi);
}

static void decode_disambiguates_branches_by_funct3() {
    auto beq = isa::decode(isa::opcode::kBranch, isa::funct3::kBeq, 0);
    auto blt = isa::decode(isa::opcode::kBranch, isa::funct3::kBlt, 0);
    assert(beq.has_value() && *beq == isa::Mnemonic::Beq);
    assert(blt.has_value() && *blt == isa::Mnemonic::Blt);
}

static void decode_ignores_funct3_funct7_for_u_and_j_types() {
    auto lui = isa::decode(isa::opcode::kLui, 0b101, 0b1010101);
    auto jal = isa::decode(isa::opcode::kJal, 0b111, 0b1111111);
    assert(lui.has_value() && *lui == isa::Mnemonic::Lui);
    assert(jal.has_value() && *jal == isa::Mnemonic::Jal);
}

static void decode_rejects_unimplemented_opcode() {
    auto result = isa::decode(0b1110011, 0, 0); // SYSTEM opcode, not in the subset
    assert(!result.has_value());
}

static void decode_rejects_unimplemented_funct3_for_known_opcode() {
    auto result = isa::decode(isa::opcode::kOpImm, 0b001, 0); // SLLI, not implemented
    assert(!result.has_value());
}

static void field_accessors_extract_expected_bits() {
    // R-type ADD x1, x2, x3: funct7=0000000 rs2=00011 rs1=00010 funct3=000 rd=00001 opcode=0110011
    std::uint32_t instr = (isa::funct7::kAdd << 25) | (3u << 20) | (2u << 15) | (isa::funct3::kAddSub << 12) | (1u << 7) | isa::opcode::kOp;

    assert(isa::opcode_of(instr) == isa::opcode::kOp);
    assert(isa::rd_of(instr) == 1u);
    assert(isa::funct3_of(instr) == isa::funct3::kAddSub);
    assert(isa::rs1_of(instr) == 2u);
    assert(isa::rs2_of(instr) == 3u);
    assert(isa::funct7_of(instr) == isa::funct7::kAdd);
}

int main() {
    lookup_returns_matching_mnemonic();
    decode_disambiguates_r_type_by_funct7();
    decode_disambiguates_i_type_alu_by_funct3();
    decode_disambiguates_branches_by_funct3();
    decode_ignores_funct3_funct7_for_u_and_j_types();
    decode_rejects_unimplemented_opcode();
    decode_rejects_unimplemented_funct3_for_known_opcode();
    field_accessors_extract_expected_bits();
    std::puts("isa_test: all tests passed");
    return 0;
}
