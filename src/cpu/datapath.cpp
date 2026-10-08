#include "cpu/datapath.hpp"

#include <cassert>
#include <string>

#include "cpu/immgen.hpp"
#include "logic/gates.hpp"
#include "logic/mux.hpp"

namespace cpu {

namespace {

constexpr std::size_t kWidth = 32;

std::vector<sim::SignalId> alloc_bus(sim::SignalStore& store, const std::string& prefix, std::size_t width) {
    std::vector<sim::SignalId> bus;
    bus.reserve(width);
    for (std::size_t i = 0; i < width; i++) {
        bus.push_back(store.alloc(prefix + std::to_string(i), sim::Bit::X));
    }
    return bus;
}

std::vector<sim::SignalId> const_bus(sim::SignalStore& store, const std::string& prefix, std::uint32_t value, std::size_t width) {
    std::vector<sim::SignalId> bus;
    bus.reserve(width);
    for (std::size_t i = 0; i < width; i++) {
        sim::Bit bit = ((value >> i) & 1u) ? sim::Bit::One : sim::Bit::Zero;
        bus.push_back(store.alloc(prefix + std::to_string(i), bit));
    }
    return bus;
}

std::vector<sim::SignalId> slice(const std::vector<sim::SignalId>& bus, std::size_t lo, std::size_t count) {
    return std::vector<sim::SignalId>(bus.begin() + lo, bus.begin() + static_cast<long>(lo + count));
}

/// Drives `dst` to always equal `src`; used to wire a register's D-input bus
/// after it's allocated, once the logic computing it exists.
void connect(sim::Circuit& circuit, sim::SignalStore& store, sim::SignalId src, sim::SignalId dst, std::uint64_t delay) {
    sim::SignalId inv = logic::build_not(circuit, store, src, delay);
    circuit.add_gate({inv, inv}, dst, delay);
}

void connect_bus(sim::Circuit& circuit, sim::SignalStore& store, const std::vector<sim::SignalId>& src, const std::vector<sim::SignalId>& dst, std::uint64_t delay) {
    assert(src.size() == dst.size());
    for (std::size_t i = 0; i < src.size(); i++) connect(circuit, store, src[i], dst[i], delay);
}

/// Kicks a constant bus's already-correct value through as a real event, so
/// gates reading only constants (no changing operand to trigger them) don't
/// sit at `X` forever.
void deposit_constant(sim::SignalStore& store, sim::EventQueue& queue, const std::vector<sim::SignalId>& bus) {
    for (sim::SignalId sig : bus) queue.schedule(0, sig, store.get(sig));
}

} // namespace

DatapathBits build_datapath(sim::Circuit& circuit, sim::SignalStore& store, const std::vector<sim::SignalId>& instr,
    const std::vector<sim::SignalId>& mem_read_data, sim::SignalId clk, sim::EventQueue& queue, std::uint64_t delay) {
    assert(instr.size() == kWidth);
    assert(mem_read_data.size() == kWidth);

    std::vector<sim::SignalId> opcode = slice(instr, 0, 7);
    std::vector<sim::SignalId> rd = slice(instr, 7, 5);
    std::vector<sim::SignalId> funct3 = slice(instr, 12, 3);
    std::vector<sim::SignalId> rs1 = slice(instr, 15, 5);
    std::vector<sim::SignalId> rs2 = slice(instr, 20, 5);
    std::vector<sim::SignalId> funct7 = slice(instr, 25, 7);

    control::ControlBits ctrl = control::build_control(circuit, store, opcode, funct3, funct7, delay);
    std::vector<sim::SignalId> imm = immgen::build_immgen(circuit, store, instr, ctrl.format, delay);

    // Write-data depends on the register file's own read ports, so allocate it now and connect later.
    std::vector<sim::SignalId> write_data_placeholder = alloc_bus(store, "wb_", kWidth);
    logic::RegisterFileBits regfile = logic::build_register_file(circuit, store, rs1, rs2, rd, write_data_placeholder, ctrl.reg_write, clk, delay);

    std::vector<sim::SignalId> alu_b(kWidth);
    for (std::size_t i = 0; i < kWidth; i++) {
        alu_b[i] = logic::build_mux2(circuit, store, regfile.read_data2[i], imm[i], ctrl.alu_src, delay);
    }
    alu::AluBits alu_bits = alu::build_alu(circuit, store, regfile.read_data1, alu_b, ctrl.alu_op, delay);

    // Same deal for the PC register's D-input: it depends on PC itself.
    std::vector<sim::SignalId> pc_next_placeholder = alloc_bus(store, "pc_next_", kWidth);
    logic::RegisterBits pc_register = logic::build_register(circuit, store, pc_next_placeholder, clk, delay);
    const std::vector<sim::SignalId>& pc = pc_register.q;

    std::vector<sim::SignalId> four = const_bus(store, "four_", 4, kWidth);
    auto [add_op0, add_op1] = alu::op_bits(alu::AluOp::Add);
    std::vector<sim::SignalId> add_op = {store.alloc("add_op0", add_op0), store.alloc("add_op1", add_op1)};
    deposit_constant(store, queue, four);
    deposit_constant(store, queue, add_op);

    alu::AluBits pc_plus4 = alu::build_alu(circuit, store, pc, four, add_op, delay);
    alu::AluBits pc_plus_imm = alu::build_alu(circuit, store, pc, imm, add_op, delay);

    // BEQ reads the zero flag, BLT reads the slt result's low bit; alu_op[1] tells Sub from Slt.
    sim::SignalId branch_condition = logic::build_mux2(circuit, store, alu_bits.zero, alu_bits.result[0], ctrl.alu_op[1], delay);
    sim::SignalId branch_taken = logic::build_and(circuit, store, ctrl.branch, branch_condition, delay);
    sim::SignalId pc_src = logic::build_or(circuit, store, ctrl.jump, branch_taken, delay);

    std::vector<sim::SignalId> pc_next_value(kWidth);
    for (std::size_t i = 0; i < kWidth; i++) {
        pc_next_value[i] = logic::build_mux2(circuit, store, pc_plus4.result[i], pc_plus_imm.result[i], pc_src, delay);
    }
    connect_bus(circuit, store, pc_next_value, pc_next_placeholder, delay);

    std::vector<sim::SignalId> result(kWidth);
    for (std::size_t i = 0; i < kWidth; i++) {
        result[i] = logic::build_mux(circuit, store, {alu_bits.result[i], mem_read_data[i], pc_plus4.result[i], imm[i]}, ctrl.result_src, delay);
    }
    connect_bus(circuit, store, result, write_data_placeholder, delay);

    DatapathBits bits;
    bits.pc = pc;
    bits.mem_addr = alu_bits.result;
    bits.mem_write_data = regfile.read_data2;
    bits.mem_write = ctrl.mem_write;
    bits.control = ctrl;
    bits.alu = alu_bits;
    bits.register_file = regfile;
    bits.pc_register = pc_register;
    return bits;
}

} // namespace cpu
