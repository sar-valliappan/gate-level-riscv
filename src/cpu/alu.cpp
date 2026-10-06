#include "cpu/alu.hpp"

#include <cassert>

#include "logic/gates.hpp"
#include "logic/mux.hpp"

namespace alu {

namespace {

struct FullAdder {
    sim::SignalId sum;
    sim::SignalId carry_out;
};

/// sum = a XOR b XOR cin
/// carry_out = majority(a, b, cin)
FullAdder build_full_adder(sim::Circuit& circuit, sim::SignalStore& store, sim::SignalId a, sim::SignalId b, sim::SignalId cin, std::uint64_t delay) {
    sim::SignalId a_xor_b = logic::build_xor(circuit, store, a, b, delay);
    sim::SignalId sum = logic::build_xor(circuit, store, a_xor_b, cin, delay);
    sim::SignalId carry_a_b = logic::build_and(circuit, store, a, b, delay);
    sim::SignalId carry_in_term = logic::build_and(circuit, store, a_xor_b, cin, delay);
    sim::SignalId carry_out = logic::build_or(circuit, store, carry_a_b, carry_in_term, delay);
    return {sum, carry_out};
}

} // namespace

std::optional<AluOp> decode_alu_op(isa::Mnemonic mnemonic) {
    switch (mnemonic) {
        case isa::Mnemonic::Add:
        case isa::Mnemonic::Addi:
        case isa::Mnemonic::Lw:
        case isa::Mnemonic::Sw:
            return AluOp::Add;
        case isa::Mnemonic::Sub:
        case isa::Mnemonic::Beq:
            return AluOp::Sub;
        case isa::Mnemonic::Andi:
            return AluOp::And;
        case isa::Mnemonic::Blt:
            return AluOp::Slt;
        case isa::Mnemonic::Lui:
        case isa::Mnemonic::Jal:
            return std::nullopt;
    }
    return std::nullopt;
}

std::pair<sim::Bit, sim::Bit> op_bits(AluOp op) {
    auto code = static_cast<std::uint8_t>(op);
    sim::Bit op0 = (code & 0b01) ? sim::Bit::One : sim::Bit::Zero;
    sim::Bit op1 = (code & 0b10) ? sim::Bit::One : sim::Bit::Zero;
    return {op0, op1};
}

AluBits build_alu(sim::Circuit& circuit, sim::SignalStore& store, const std::vector<sim::SignalId>& a, const std::vector<sim::SignalId>& b,
    const std::vector<sim::SignalId>& op, std::uint64_t delay) {
    assert(a.size() == b.size());
    assert(op.size() == 2);
    std::size_t width = a.size();

    sim::SignalId op0 = op[0];
    sim::SignalId op1 = op[1];
    sim::SignalId binvert = op0;

    sim::SignalId zero_const = store.alloc("alu_zero_const", sim::Bit::Zero);

    std::vector<sim::SignalId> sum(width);
    std::vector<sim::SignalId> and_bits(width);
    std::vector<sim::SignalId> carry(width);

    sim::SignalId carry_in = binvert;
    for (std::size_t i = 0; i < width; i++) {
        sim::SignalId b_operand = logic::build_xor(circuit, store, b[i], binvert, delay);
        FullAdder fa = build_full_adder(circuit, store, a[i], b_operand, carry_in, delay);
        sum[i] = fa.sum;
        carry[i] = fa.carry_out;
        and_bits[i] = logic::build_and(circuit, store, a[i], b[i], delay);
        carry_in = fa.carry_out;
    }

    // Overflow handling
    sim::SignalId overflow = logic::build_xor(circuit, store, carry[width - 2], carry[width - 1], delay);
    sim::SignalId sign = sum[width - 1];
    sim::SignalId less = logic::build_xor(circuit, store, sign, overflow, delay);

    AluBits bits;
    bits.result.resize(width);
    for (std::size_t i = 0; i < width; i++) {
        sim::SignalId slt_slot = (i == 0) ? less : zero_const;
        bits.result[i] = logic::build_mux(circuit, store, {sum[i], sum[i], and_bits[i], slt_slot}, {op0, op1}, delay);
    }

    sim::SignalId not0 = logic::build_not(circuit, store, bits.result[0], delay);
    sim::SignalId zero_acc = not0;
    for (std::size_t i = 1; i < width; i++) {
        sim::SignalId not_i = logic::build_not(circuit, store, bits.result[i], delay);
        zero_acc = logic::build_and(circuit, store, zero_acc, not_i, delay);
    }
    bits.zero = zero_acc;
    bits.overflow = overflow;

    return bits;
}

} // namespace alu
