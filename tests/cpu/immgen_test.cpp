#include "cpu/immgen.hpp"
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

constexpr std::size_t kWidth = 32;

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

// -- RISC-V instruction encoders, used only to build test fixtures --

std::uint32_t encode_i_type(std::int32_t imm, std::uint32_t rs1, std::uint32_t funct3, std::uint32_t rd, std::uint32_t opcode) {
    std::uint32_t imm12 = static_cast<std::uint32_t>(imm) & 0xFFFu;
    return (imm12 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
}

std::uint32_t encode_s_type(std::int32_t imm, std::uint32_t rs2, std::uint32_t rs1, std::uint32_t funct3, std::uint32_t opcode) {
    std::uint32_t u = static_cast<std::uint32_t>(imm);
    std::uint32_t imm_hi = (u >> 5) & 0x7Fu;
    std::uint32_t imm_lo = u & 0x1Fu;
    return (imm_hi << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (imm_lo << 7) | opcode;
}

std::uint32_t encode_b_type(std::int32_t imm, std::uint32_t rs2, std::uint32_t rs1, std::uint32_t funct3, std::uint32_t opcode) {
    std::uint32_t u = static_cast<std::uint32_t>(imm); // imm[0] must be 0
    std::uint32_t bit12 = (u >> 12) & 0x1u;
    std::uint32_t bits10_5 = (u >> 5) & 0x3Fu;
    std::uint32_t bits4_1 = (u >> 1) & 0xFu;
    std::uint32_t bit11 = (u >> 11) & 0x1u;
    return (bit12 << 31) | (bits10_5 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (bits4_1 << 8) | (bit11 << 7) | opcode;
}

std::uint32_t encode_u_type(std::int32_t imm, std::uint32_t rd, std::uint32_t opcode) {
    std::uint32_t u = static_cast<std::uint32_t>(imm);
    return (u & 0xFFFFF000u) | (rd << 7) | opcode;
}

std::uint32_t encode_j_type(std::int32_t imm, std::uint32_t rd, std::uint32_t opcode) {
    std::uint32_t u = static_cast<std::uint32_t>(imm); // imm[0] must be 0
    std::uint32_t bit20 = (u >> 20) & 0x1u;
    std::uint32_t bits19_12 = (u >> 12) & 0xFFu;
    std::uint32_t bit11 = (u >> 11) & 0x1u;
    std::uint32_t bits10_1 = (u >> 1) & 0x3FFu;
    return (bit20 << 31) | (bits19_12 << 12) | (bit11 << 20) | (bits10_1 << 21) | (rd << 7) | opcode;
}

struct ImmGen {
    std::vector<sim::SignalId> instr;
    std::vector<sim::SignalId> format;
    std::vector<sim::SignalId> imm;
};

ImmGen make_immgen(sim::Circuit& circuit, sim::SignalStore& store) {
    ImmGen unit;
    unit.instr = alloc_bus(store, "INSTR_", kWidth);
    unit.format = alloc_bus(store, "FMT_", 3);
    unit.imm = immgen::build_immgen(circuit, store, unit.instr, unit.format, 1);
    return unit;
}

void apply(sim::Circuit& circuit, sim::SignalStore& store, sim::EventQueue& queue, ImmGen& unit, std::uint32_t instr_value, isa::Format format) {
    auto fmt = immgen::format_bits(format);
    set_bus(queue, unit.instr, instr_value);
    for (std::size_t i = 0; i < unit.format.size(); i++) {
        queue.schedule(queue.current_time + 10, unit.format[i], fmt[i]);
    }
    test_utils::settle(circuit, store, queue);
}

} // namespace

static void i_type_sign_extends_negative_immediate() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    ImmGen unit = make_immgen(circuit, store);

    std::uint32_t instr = encode_i_type(-1, /*rs1=*/2, isa::funct3::kAddi, /*rd=*/1, isa::opcode::kOpImm);
    apply(circuit, store, queue, unit, instr, isa::Format::I);
    assert(bus_value(store, unit.imm) == 0xFFFFFFFFu);
}

static void i_type_passes_through_positive_immediate() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    ImmGen unit = make_immgen(circuit, store);

    std::uint32_t instr = encode_i_type(5, /*rs1=*/2, isa::funct3::kAddi, /*rd=*/1, isa::opcode::kOpImm);
    apply(circuit, store, queue, unit, instr, isa::Format::I);
    assert(bus_value(store, unit.imm) == 5u);
}

static void s_type_sign_extends_negative_offset() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    ImmGen unit = make_immgen(circuit, store);

    std::uint32_t instr = encode_s_type(-4, /*rs2=*/3, /*rs1=*/2, isa::funct3::kSw, isa::opcode::kStore);
    apply(circuit, store, queue, unit, instr, isa::Format::S);
    assert(bus_value(store, unit.imm) == 0xFFFFFFFCu);
}

static void b_type_recovers_branch_offset() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    ImmGen unit = make_immgen(circuit, store);

    std::uint32_t instr = encode_b_type(8, /*rs2=*/3, /*rs1=*/2, isa::funct3::kBeq, isa::opcode::kBranch);
    apply(circuit, store, queue, unit, instr, isa::Format::B);
    assert(bus_value(store, unit.imm) == 8u);

    instr = encode_b_type(-16, /*rs2=*/3, /*rs1=*/2, isa::funct3::kBlt, isa::opcode::kBranch);
    apply(circuit, store, queue, unit, instr, isa::Format::B);
    assert(bus_value(store, unit.imm) == 0xFFFFFFF0u);
}

static void u_type_leaves_low_bits_clear() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    ImmGen unit = make_immgen(circuit, store);

    std::uint32_t instr = encode_u_type(static_cast<std::int32_t>(0x12345000u), /*rd=*/1, isa::opcode::kLui);
    apply(circuit, store, queue, unit, instr, isa::Format::U);
    assert(bus_value(store, unit.imm) == 0x12345000u);
}

static void j_type_recovers_signed_offset() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    ImmGen unit = make_immgen(circuit, store);

    std::uint32_t instr = encode_j_type(-8, /*rd=*/1, isa::opcode::kJal);
    apply(circuit, store, queue, unit, instr, isa::Format::J);
    assert(bus_value(store, unit.imm) == 0xFFFFFFF8u);

    instr = encode_j_type(1024, /*rd=*/1, isa::opcode::kJal);
    apply(circuit, store, queue, unit, instr, isa::Format::J);
    assert(bus_value(store, unit.imm) == 1024u);
}

static void r_type_produces_no_immediate() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    ImmGen unit = make_immgen(circuit, store);

    // Garbage bits everywhere except opcode; R-type must still yield zero.
    apply(circuit, store, queue, unit, 0xFFFFFFFFu, isa::Format::R);
    assert(bus_value(store, unit.imm) == 0u);
}

int main() {
    i_type_sign_extends_negative_immediate();
    i_type_passes_through_positive_immediate();
    s_type_sign_extends_negative_offset();
    b_type_recovers_branch_offset();
    u_type_leaves_low_bits_clear();
    j_type_recovers_signed_offset();
    r_type_produces_no_immediate();
    std::puts("immgen_test: all tests passed");
    return 0;
}
