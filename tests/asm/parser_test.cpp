#include "asm/parser.hpp"

#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <string>

using assembler::parse;
using assembler::ParsedInstruction;

static void parses_r_type() {
    auto instrs = parse("add x3, x1, x2\nsub x4, x2, x1");
    assert(instrs.size() == 2);
    assert(instrs[0].mnemonic == isa::Mnemonic::Add);
    assert(instrs[0].rd == 3 && instrs[0].rs1 == 1 && instrs[0].rs2 == 2);
    assert(instrs[1].mnemonic == isa::Mnemonic::Sub);
    assert(instrs[1].rd == 4 && instrs[1].rs1 == 2 && instrs[1].rs2 == 1);
}

static void parses_i_type_alu() {
    auto instrs = parse("addi x1, x0, 5\nandi x7, x3, -1");
    assert(instrs[0].mnemonic == isa::Mnemonic::Addi);
    assert(instrs[0].rd == 1 && instrs[0].rs1 == 0 && instrs[0].imm == 5);
    assert(instrs[1].mnemonic == isa::Mnemonic::Andi);
    assert(instrs[1].imm == -1);
}

static void parses_hex_immediate() {
    auto instrs = parse("addi x1, x0, 0x1F");
    assert(instrs[0].imm == 0x1F);
}

static void parses_load_and_store() {
    auto instrs = parse("lw x5, 8(x2)\nsw x3, -4(x6)");
    assert(instrs[0].mnemonic == isa::Mnemonic::Lw);
    assert(instrs[0].rd == 5 && instrs[0].rs1 == 2 && instrs[0].imm == 8);
    assert(instrs[1].mnemonic == isa::Mnemonic::Sw);
    assert(instrs[1].rs2 == 3 && instrs[1].rs1 == 6 && instrs[1].imm == -4);
}

static void parses_u_and_j_type() {
    auto instrs = parse("lui x9, 0x1000\njal x10, 8");
    assert(instrs[0].mnemonic == isa::Mnemonic::Lui);
    assert(instrs[0].rd == 9 && instrs[0].imm == 0x1000);
    assert(instrs[1].mnemonic == isa::Mnemonic::Jal);
    assert(instrs[1].rd == 10 && instrs[1].imm == 8);
}

static void resolves_forward_and_backward_branch_labels() {
    // beq at address 0 targets "skip" at address 8: +8.
    // blt at address 8 targets "loop" at address 0: -8.
    auto instrs = parse(R"(
loop:
    beq x1, x2, skip
    addi x3, x0, 1
skip:
    blt x1, x2, loop
)");
    assert(instrs.size() == 3);
    assert(instrs[0].mnemonic == isa::Mnemonic::Beq);
    assert(instrs[0].imm == 8);
    assert(instrs[2].mnemonic == isa::Mnemonic::Blt);
    assert(instrs[2].imm == -8);
}

static void resolves_jal_label() {
    auto instrs = parse("jal x1, end\naddi x2, x0, 1\nend:\naddi x3, x0, 2");
    assert(instrs[0].imm == 8);
}

static void ignores_comments_and_blank_lines() {
    auto instrs = parse("# a comment\n\naddi x1, x0, 1 # trailing comment\n\n");
    assert(instrs.size() == 1);
    assert(instrs[0].rd == 1);
}

static void inline_label_with_instruction_is_supported() {
    auto instrs = parse("start: addi x1, x0, 1\njal x2, start");
    assert(instrs.size() == 2);
    assert(instrs[1].imm == -4);
}

static void rejects_unknown_mnemonic() {
    bool threw = false;
    try {
        parse("nop");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);
}

static void rejects_undefined_label() {
    bool threw = false;
    try {
        parse("jal x1, nowhere");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);
}

static void rejects_bad_register() {
    bool threw = false;
    try {
        parse("addi x32, x0, 1");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);
}

static void rejects_wrong_operand_count() {
    bool threw = false;
    try {
        parse("add x1, x2");
    } catch (const std::runtime_error&) {
        threw = true;
    }
    assert(threw);
}

int main() {
    parses_r_type();
    parses_i_type_alu();
    parses_hex_immediate();
    parses_load_and_store();
    parses_u_and_j_type();
    resolves_forward_and_backward_branch_labels();
    resolves_jal_label();
    ignores_comments_and_blank_lines();
    inline_label_with_instruction_is_supported();
    rejects_unknown_mnemonic();
    rejects_undefined_label();
    rejects_bad_register();
    rejects_wrong_operand_count();
    std::puts("parser_test: all tests passed");
    return 0;
}
