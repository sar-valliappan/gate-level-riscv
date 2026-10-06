#include "cpu/alu.hpp"
#include "cpu/control.hpp"
#include "cpu/isa.hpp"
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

std::vector<sim::SignalId> alloc_bus(sim::SignalStore& store, const std::string& prefix, std::size_t width) {
    std::vector<sim::SignalId> bus;
    bus.reserve(width);
    for (std::size_t i = 0; i < width; i++) {
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

bool bit_value(const sim::SignalStore& store, sim::SignalId sig) { return store.get(sig) == sim::Bit::One; }

struct Control {
    std::vector<sim::SignalId> opcode;
    std::vector<sim::SignalId> funct3;
    std::vector<sim::SignalId> funct7;
    control::ControlBits bits;
};

Control make_control(sim::Circuit& circuit, sim::SignalStore& store) {
    Control unit;
    unit.opcode = alloc_bus(store, "OPCODE_", 7);
    unit.funct3 = alloc_bus(store, "FUNCT3_", 3);
    unit.funct7 = alloc_bus(store, "FUNCT7_", 7);
    unit.bits = control::build_control(circuit, store, unit.opcode, unit.funct3, unit.funct7, 1);
    return unit;
}

void apply(sim::Circuit& circuit, sim::SignalStore& store, sim::EventQueue& queue, Control& unit, isa::Mnemonic mnemonic) {
    const isa::InstructionInfo& info = isa::lookup(mnemonic);
    set_bus(queue, unit.opcode, info.opcode);
    set_bus(queue, unit.funct3, info.funct3);
    set_bus(queue, unit.funct7, info.funct7);
    test_utils::settle(circuit, store, queue);
}

struct Expected {
    bool reg_write;
    bool alu_src;
    bool mem_write;
    std::uint32_t result_src;
    bool branch;
    bool jump;
    isa::Format format;
    alu::AluOp alu_op;
};

void check(sim::SignalStore& store, Control& unit, const Expected& expected) {
    assert(bit_value(store, unit.bits.reg_write) == expected.reg_write);
    assert(bit_value(store, unit.bits.alu_src) == expected.alu_src);
    assert(bit_value(store, unit.bits.mem_write) == expected.mem_write);
    assert(bus_value(store, unit.bits.result_src) == expected.result_src);
    assert(bit_value(store, unit.bits.branch) == expected.branch);
    assert(bit_value(store, unit.bits.jump) == expected.jump);
    assert(bus_value(store, unit.bits.format) == static_cast<std::uint32_t>(expected.format));
    assert(bus_value(store, unit.bits.alu_op) == static_cast<std::uint32_t>(expected.alu_op));
}

} // namespace

static void add_sets_r_type_alu_control() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    Control unit = make_control(circuit, store);

    apply(circuit, store, queue, unit, isa::Mnemonic::Add);
    check(store, unit, {true, false, false, 0b00, false, false, isa::Format::R, alu::AluOp::Add});
}

static void sub_is_disambiguated_from_add_by_funct7() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    Control unit = make_control(circuit, store);

    apply(circuit, store, queue, unit, isa::Mnemonic::Sub);
    check(store, unit, {true, false, false, 0b00, false, false, isa::Format::R, alu::AluOp::Sub});
}

static void addi_and_andi_use_immediate_operand() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    Control unit = make_control(circuit, store);

    apply(circuit, store, queue, unit, isa::Mnemonic::Addi);
    check(store, unit, {true, true, false, 0b00, false, false, isa::Format::I, alu::AluOp::Add});

    apply(circuit, store, queue, unit, isa::Mnemonic::Andi);
    check(store, unit, {true, true, false, 0b00, false, false, isa::Format::I, alu::AluOp::And});
}

static void lw_reads_memory_into_the_register_file() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    Control unit = make_control(circuit, store);

    apply(circuit, store, queue, unit, isa::Mnemonic::Lw);
    check(store, unit, {true, true, false, 0b01, false, false, isa::Format::I, alu::AluOp::Add});
}

static void sw_writes_memory_without_touching_the_register_file() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    Control unit = make_control(circuit, store);

    apply(circuit, store, queue, unit, isa::Mnemonic::Sw);
    check(store, unit, {false, true, true, 0b00, false, false, isa::Format::S, alu::AluOp::Add});
}

static void beq_and_blt_set_branch_with_distinct_alu_ops() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    Control unit = make_control(circuit, store);

    apply(circuit, store, queue, unit, isa::Mnemonic::Beq);
    check(store, unit, {false, false, false, 0b00, true, false, isa::Format::B, alu::AluOp::Sub});

    apply(circuit, store, queue, unit, isa::Mnemonic::Blt);
    check(store, unit, {false, false, false, 0b00, true, false, isa::Format::B, alu::AluOp::Slt});
}

static void lui_selects_the_immediate_as_the_result() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    Control unit = make_control(circuit, store);

    apply(circuit, store, queue, unit, isa::Mnemonic::Lui);
    check(store, unit, {true, false, false, 0b11, false, false, isa::Format::U, alu::AluOp::Add});
}

static void jal_selects_pc_plus_4_and_sets_jump() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    Control unit = make_control(circuit, store);

    apply(circuit, store, queue, unit, isa::Mnemonic::Jal);
    check(store, unit, {true, false, false, 0b10, false, true, isa::Format::J, alu::AluOp::Add});
}

int main() {
    add_sets_r_type_alu_control();
    sub_is_disambiguated_from_add_by_funct7();
    addi_and_andi_use_immediate_operand();
    lw_reads_memory_into_the_register_file();
    sw_writes_memory_without_touching_the_register_file();
    beq_and_blt_set_branch_with_distinct_alu_ops();
    lui_selects_the_immediate_as_the_result();
    jal_selects_pc_plus_4_and_sets_jump();
    std::puts("control_test: all tests passed");
    return 0;
}
