#include "cpu/alu.hpp"
#include "sim/circuit.hpp"
#include "sim/event.hpp"
#include "sim/signal.hpp"
#include "test_utils.hpp"

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace {

constexpr std::size_t kWidth = 32;

std::vector<sim::SignalId> alloc_bus(sim::SignalStore& store, const std::string& prefix) {
    std::vector<sim::SignalId> bus;
    bus.reserve(kWidth);
    for (std::size_t i = 0; i < kWidth; i++) {
        bus.push_back(store.alloc(prefix + std::to_string(i), sim::Bit::Zero));
    }
    return bus;
}

void set_bus(sim::EventQueue& queue, const std::vector<sim::SignalId>& bus, std::uint32_t value) {
    for (std::size_t i = 0; i < bus.size(); i++) {
        sim::Bit b = ((value >> i) & 1u) ? sim::Bit::One : sim::Bit::Zero;
        queue.schedule(queue.current_time + 10, bus[i], b);
    }
}

std::uint32_t bus_value(const sim::SignalStore& store, const std::vector<sim::SignalId>& bus) {
    std::uint32_t value = 0;
    for (std::size_t i = 0; i < bus.size(); i++) {
        if (store.get(bus[i]) == sim::Bit::One) value |= (1u << i);
    }
    return value;
}

struct Alu {
    std::vector<sim::SignalId> a;
    std::vector<sim::SignalId> b;
    std::vector<sim::SignalId> op;
    alu::AluBits bits;
};

Alu make_alu(sim::Circuit& circuit, sim::SignalStore& store) {
    Alu unit;
    unit.a = alloc_bus(store, "A_");
    unit.b = alloc_bus(store, "B_");
    unit.op = {store.alloc("OP0", sim::Bit::Zero), store.alloc("OP1", sim::Bit::Zero)};
    unit.bits = alu::build_alu(circuit, store, unit.a, unit.b, unit.op, 1);
    return unit;
}

void apply(sim::Circuit& circuit, sim::SignalStore& store, sim::EventQueue& queue, Alu& unit, std::uint32_t a_val, std::uint32_t b_val, alu::AluOp op) {
    auto [op0, op1] = alu::op_bits(op);
    set_bus(queue, unit.a, a_val);
    set_bus(queue, unit.b, b_val);
    queue.schedule(queue.current_time + 10, unit.op[0], op0);
    queue.schedule(queue.current_time + 10, unit.op[1], op1);
    test_utils::settle(circuit, store, queue);
}

} // namespace

static void add_computes_sum() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    Alu unit = make_alu(circuit, store);

    apply(circuit, store, queue, unit, 12, 30, alu::AluOp::Add);
    assert(bus_value(store, unit.bits.result) == 42u);
    assert(store.get(unit.bits.zero) == sim::Bit::Zero);
}

static void sub_computes_difference_and_zero_flag() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    Alu unit = make_alu(circuit, store);

    apply(circuit, store, queue, unit, 50, 8, alu::AluOp::Sub);
    assert(bus_value(store, unit.bits.result) == 42u);
    assert(store.get(unit.bits.zero) == sim::Bit::Zero);

    apply(circuit, store, queue, unit, 7, 7, alu::AluOp::Sub);
    assert(bus_value(store, unit.bits.result) == 0u);
    assert(store.get(unit.bits.zero) == sim::Bit::One); // BEQ: equal operands -> zero flag set
}

static void sub_wraps_on_negative_result() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    Alu unit = make_alu(circuit, store);

    apply(circuit, store, queue, unit, 8, 50, alu::AluOp::Sub);
    assert(bus_value(store, unit.bits.result) == static_cast<std::uint32_t>(8 - 50)); // two's complement wraparound
}

static void and_masks_bits() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    Alu unit = make_alu(circuit, store);

    apply(circuit, store, queue, unit, 0xFF00FF00u, 0x0F0F0F0Fu, alu::AluOp::And);
    assert(bus_value(store, unit.bits.result) == 0x0F000F00u);
}

static void slt_detects_signed_less_than() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    Alu unit = make_alu(circuit, store);

    apply(circuit, store, queue, unit, 3, 5, alu::AluOp::Slt); // 3 < 5
    assert(bus_value(store, unit.bits.result) == 1u);

    apply(circuit, store, queue, unit, 5, 3, alu::AluOp::Slt); // 5 < 3 is false
    assert(bus_value(store, unit.bits.result) == 0u);

    apply(circuit, store, queue, unit, 7, 7, alu::AluOp::Slt); // equal is not less-than
    assert(bus_value(store, unit.bits.result) == 0u);
}

static void slt_handles_negative_operands_and_signed_overflow() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    Alu unit = make_alu(circuit, store);

    // -1 < 1 (0xFFFFFFFF as signed is -1)
    apply(circuit, store, queue, unit, 0xFFFFFFFFu, 1u, alu::AluOp::Slt);
    assert(bus_value(store, unit.bits.result) == 1u);

    // INT_MIN < INT_MAX: true, but computing INT_MIN - INT_MAX overflows,
    // which is exactly the case the sign-xor-overflow correction handles.
    apply(circuit, store, queue, unit, 0x80000000u, 0x7FFFFFFFu, alu::AluOp::Slt);
    assert(bus_value(store, unit.bits.result) == 1u);
    assert(store.get(unit.bits.overflow) == sim::Bit::One);

    // INT_MAX < INT_MIN is false, also an overflowing subtraction.
    apply(circuit, store, queue, unit, 0x7FFFFFFFu, 0x80000000u, alu::AluOp::Slt);
    assert(bus_value(store, unit.bits.result) == 0u);
    assert(store.get(unit.bits.overflow) == sim::Bit::One);
}

static void decode_alu_op_matches_datapath_usage() {
    assert(alu::decode_alu_op(isa::Mnemonic::Add) == alu::AluOp::Add);
    assert(alu::decode_alu_op(isa::Mnemonic::Addi) == alu::AluOp::Add);
    assert(alu::decode_alu_op(isa::Mnemonic::Lw) == alu::AluOp::Add);
    assert(alu::decode_alu_op(isa::Mnemonic::Sw) == alu::AluOp::Add);
    assert(alu::decode_alu_op(isa::Mnemonic::Sub) == alu::AluOp::Sub);
    assert(alu::decode_alu_op(isa::Mnemonic::Beq) == alu::AluOp::Sub);
    assert(alu::decode_alu_op(isa::Mnemonic::Andi) == alu::AluOp::And);
    assert(alu::decode_alu_op(isa::Mnemonic::Blt) == alu::AluOp::Slt);
    assert(!alu::decode_alu_op(isa::Mnemonic::Lui).has_value());
    assert(!alu::decode_alu_op(isa::Mnemonic::Jal).has_value());
}

int main() {
    add_computes_sum();
    sub_computes_difference_and_zero_flag();
    sub_wraps_on_negative_result();
    and_masks_bits();
    slt_detects_signed_less_than();
    slt_handles_negative_operands_and_signed_overflow();
    decode_alu_op_matches_datapath_usage();
    std::puts("alu_test: all tests passed");
    return 0;
}
