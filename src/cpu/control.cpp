#include "cpu/control.hpp"

#include <array>
#include <cstddef>
#include <optional>

#include "cpu/alu.hpp"
#include "cpu/isa.hpp"
#include "logic/gates.hpp"

namespace control {

namespace {

/// One instruction's datapath control values.
struct Microcode {
    isa::Mnemonic mnemonic;
    bool reg_write;
    bool alu_src;
    bool mem_write;
    std::uint8_t result_src; // 00 ALU result, 01 memory read data, 10 PC+4, 11 immediate
    bool branch;
    bool jump;
    isa::Format format;
    std::optional<alu::AluOp> alu_op;
};

const std::array<Microcode, 10>& microcode_table() {
    static const std::array<Microcode, 10> table = {{
        {isa::Mnemonic::Add, true, false, false, 0b00, false, false, isa::Format::R, alu::AluOp::Add},
        {isa::Mnemonic::Sub, true, false, false, 0b00, false, false, isa::Format::R, alu::AluOp::Sub},
        {isa::Mnemonic::Addi, true, true, false, 0b00, false, false, isa::Format::I, alu::AluOp::Add},
        {isa::Mnemonic::Andi, true, true, false, 0b00, false, false, isa::Format::I, alu::AluOp::And},
        {isa::Mnemonic::Lw, true, true, false, 0b01, false, false, isa::Format::I, alu::AluOp::Add},
        {isa::Mnemonic::Sw, false, true, true, 0b00, false, false, isa::Format::S, alu::AluOp::Add},
        {isa::Mnemonic::Beq, false, false, false, 0b00, true, false, isa::Format::B, alu::AluOp::Sub},
        {isa::Mnemonic::Blt, false, false, false, 0b00, true, false, isa::Format::B, alu::AluOp::Slt},
        {isa::Mnemonic::Lui, true, false, false, 0b11, false, false, isa::Format::U, std::nullopt},
        {isa::Mnemonic::Jal, true, false, false, 0b10, false, true, isa::Format::J, std::nullopt},
    }};
    return table;
}

sim::SignalId build_equals_const(sim::Circuit& circuit, sim::SignalStore& store, const std::vector<sim::SignalId>& bus, std::uint32_t constant, std::uint64_t delay) {
    auto bit_line = [&](std::size_t j) {
        return ((constant >> j) & 1u) ? bus[j] : logic::build_not(circuit, store, bus[j], delay);
    };

    sim::SignalId term = bit_line(0);
    for (std::size_t j = 1; j < bus.size(); j++) {
        term = logic::build_and(circuit, store, term, bit_line(j), delay);
    }
    return term;
}

} // namespace

ControlBits build_control(sim::Circuit& circuit, sim::SignalStore& store, const std::vector<sim::SignalId>& opcode, const std::vector<sim::SignalId>& funct3,
    const std::vector<sim::SignalId>& funct7, std::uint64_t delay) {
    const auto& encodings = isa::instruction_table();
    const auto& microcode = microcode_table();

    // One-hot "is this the decoded instruction" signal per mnemonic.
    std::array<sim::SignalId, 10> is_mnemonic;
    for (std::size_t i = 0; i < encodings.size(); i++) {
        const isa::InstructionInfo& info = encodings[i];
        sim::SignalId matches = build_equals_const(circuit, store, opcode, info.opcode, delay);
        switch (info.format) {
            case isa::Format::U:
            case isa::Format::J:
                break; // opcode alone disambiguates
            case isa::Format::R:
                matches = logic::build_and(circuit, store, matches, build_equals_const(circuit, store, funct7, info.funct7, delay), delay);
                [[fallthrough]];
            case isa::Format::I:
            case isa::Format::S:
            case isa::Format::B:
                matches = logic::build_and(circuit, store, matches, build_equals_const(circuit, store, funct3, info.funct3, delay), delay);
                break;
        }
        is_mnemonic[i] = matches;
    }

    sim::SignalId const_zero = store.alloc("control_zero_const", sim::Bit::Zero);

    auto or_reduce_where = [&](auto predicate) {
        sim::SignalId acc = const_zero;
        bool first = true;
        for (std::size_t i = 0; i < microcode.size(); i++) {
            if (!predicate(microcode[i])) continue;
            acc = first ? is_mnemonic[i] : logic::build_or(circuit, store, acc, is_mnemonic[i], delay);
            first = false;
        }
        return acc;
    };

    ControlBits bits;
    bits.reg_write = or_reduce_where([](const Microcode& m) { return m.reg_write; });
    bits.alu_src = or_reduce_where([](const Microcode& m) { return m.alu_src; });
    bits.mem_write = or_reduce_where([](const Microcode& m) { return m.mem_write; });
    bits.branch = or_reduce_where([](const Microcode& m) { return m.branch; });
    bits.jump = or_reduce_where([](const Microcode& m) { return m.jump; });

    bits.result_src.resize(2);
    for (std::size_t bit = 0; bit < 2; bit++) {
        bits.result_src[bit] = or_reduce_where([bit](const Microcode& m) { return (m.result_src >> bit) & 1u; });
    }

    bits.format.resize(3);
    for (std::size_t bit = 0; bit < 3; bit++) {
        bits.format[bit] = or_reduce_where([bit](const Microcode& m) { return (static_cast<std::uint8_t>(m.format) >> bit) & 1u; });
    }

    bits.alu_op.resize(2);
    for (std::size_t bit = 0; bit < 2; bit++) {
        bits.alu_op[bit] = or_reduce_where([bit](const Microcode& m) { return m.alu_op.has_value() && ((static_cast<std::uint8_t>(*m.alu_op) >> bit) & 1u); });
    }

    return bits;
}

} // namespace control
