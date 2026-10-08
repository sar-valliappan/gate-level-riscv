#include "cpu/datapath.hpp"
#include "cpu/isa.hpp"
#include "cpu/memory.hpp"
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
constexpr std::size_t kMaxEvents = 2'000'000;

std::uint32_t encode_r_type(std::uint32_t funct7, std::uint32_t rs2, std::uint32_t rs1, std::uint32_t funct3, std::uint32_t rd, std::uint32_t opcode) {
    return (funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode;
}

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

std::uint32_t addi(std::uint32_t rd, std::uint32_t rs1, std::int32_t imm) { return encode_i_type(imm, rs1, isa::funct3::kAddi, rd, isa::opcode::kOpImm); }
std::uint32_t andi(std::uint32_t rd, std::uint32_t rs1, std::int32_t imm) { return encode_i_type(imm, rs1, isa::funct3::kAndi, rd, isa::opcode::kOpImm); }
std::uint32_t add_(std::uint32_t rd, std::uint32_t rs1, std::uint32_t rs2) { return encode_r_type(isa::funct7::kAdd, rs2, rs1, isa::funct3::kAddSub, rd, isa::opcode::kOp); }
std::uint32_t sub_(std::uint32_t rd, std::uint32_t rs1, std::uint32_t rs2) { return encode_r_type(isa::funct7::kSub, rs2, rs1, isa::funct3::kAddSub, rd, isa::opcode::kOp); }
std::uint32_t lw(std::uint32_t rd, std::uint32_t rs1, std::int32_t imm) { return encode_i_type(imm, rs1, isa::funct3::kLw, rd, isa::opcode::kLoad); }
std::uint32_t sw(std::uint32_t rs2, std::uint32_t rs1, std::int32_t imm) { return encode_s_type(imm, rs2, rs1, isa::funct3::kSw, isa::opcode::kStore); }
std::uint32_t beq(std::uint32_t rs1, std::uint32_t rs2, std::int32_t imm) { return encode_b_type(imm, rs2, rs1, isa::funct3::kBeq, isa::opcode::kBranch); }
std::uint32_t blt(std::uint32_t rs1, std::uint32_t rs2, std::int32_t imm) { return encode_b_type(imm, rs2, rs1, isa::funct3::kBlt, isa::opcode::kBranch); }
std::uint32_t lui(std::uint32_t rd, std::int32_t imm) { return encode_u_type(imm, rd, isa::opcode::kLui); }
std::uint32_t jal(std::uint32_t rd, std::int32_t imm) { return encode_j_type(imm, rd, isa::opcode::kJal); }

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

struct Cpu {
    std::vector<sim::SignalId> instr_bus;
    std::vector<sim::SignalId> mem_read_bus;
    sim::SignalId clk;
    cpu::DatapathBits bits;
};

constexpr int kResetPulses = 2;
constexpr std::uint32_t kResetPcOffset = 4;

Cpu make_cpu(sim::Circuit& circuit, sim::SignalStore& store, sim::EventQueue& queue) {
    Cpu cpu_state;
    cpu_state.instr_bus = alloc_bus(store, "INSTR_", kWidth);
    cpu_state.mem_read_bus = alloc_bus(store, "MEMRD_", kWidth);
    cpu_state.clk = store.alloc("CLK", sim::Bit::Zero);

    cpu_state.bits = cpu::build_datapath(circuit, store, cpu_state.instr_bus, cpu_state.mem_read_bus, cpu_state.clk, queue, 1);

    std::vector<sim::SignalId> master_feedbacks = cpu_state.bits.pc_register.master_q_not;
    std::vector<sim::SignalId> slave_feedbacks = cpu_state.bits.pc_register.q_not;
    for (const logic::RegisterBits& reg : cpu_state.bits.register_file.registers) {
        master_feedbacks.insert(master_feedbacks.end(), reg.master_q_not.begin(), reg.master_q_not.end());
        slave_feedbacks.insert(slave_feedbacks.end(), reg.q_not.begin(), reg.q_not.end());
    }

    std::vector<sim::SignalId> inputs;
    inputs.insert(inputs.end(), cpu_state.instr_bus.begin(), cpu_state.instr_bus.end());
    inputs.insert(inputs.end(), cpu_state.mem_read_bus.begin(), cpu_state.mem_read_bus.end());
    inputs.push_back(cpu_state.clk);

    // Seeds PC = 0 and every register file entry = 0.
    test_utils::bootstrap_register(circuit, store, queue, master_feedbacks, slave_feedbacks, sim::Bit::One, inputs, kMaxEvents);

    // instr is still all-zero here, so reg_write stays low; only PC moves.
    for (int pulse = 0; pulse < kResetPulses; pulse++) {
        queue.schedule(queue.current_time + 10, cpu_state.clk, sim::Bit::One);
        test_utils::settle(circuit, store, queue, kMaxEvents);
        queue.schedule(queue.current_time + 10, cpu_state.clk, sim::Bit::Zero);
        test_utils::settle(circuit, store, queue, kMaxEvents);
    }

    return cpu_state;
}

/// Runs one fetch/decode/execute/memory/writeback cycle, then pulses clock to commit it.
void step(sim::Circuit& circuit, sim::SignalStore& store, sim::EventQueue& queue, Cpu& cpu_state, cpu::Memory& memory) {
    std::uint32_t pc_value = bus_value(store, cpu_state.bits.pc);
    set_bus(queue, cpu_state.instr_bus, memory.read_instruction(pc_value));
    test_utils::settle(circuit, store, queue, kMaxEvents);

    std::uint32_t addr = bus_value(store, cpu_state.bits.mem_addr);
    set_bus(queue, cpu_state.mem_read_bus, memory.read_word(addr));
    test_utils::settle(circuit, store, queue, kMaxEvents);

    if (store.get(cpu_state.bits.mem_write) == sim::Bit::One) {
        memory.write_word(addr, bus_value(store, cpu_state.bits.mem_write_data));
    }

    queue.schedule(queue.current_time + 10, cpu_state.clk, sim::Bit::One); // rising edge: commit PC + reg file
    test_utils::settle(circuit, store, queue, kMaxEvents);
    queue.schedule(queue.current_time + 10, cpu_state.clk, sim::Bit::Zero);
    test_utils::settle(circuit, store, queue, kMaxEvents);
}

std::uint32_t register_value(const sim::SignalStore& store, Cpu& cpu_state, std::size_t index) {
    return bus_value(store, cpu_state.bits.register_file.registers[index].q);
}

} // namespace

static void runs_a_small_hand_assembled_program() {
    // First word is never fetched: reset leaves PC at kResetPcOffset, where the real program starts.
    std::vector<std::uint32_t> program = {
        /*0 */ 0,                // unused: skipped by the reset pulses
        /*4 */ addi(1, 0, 5),    // x1 = 5
        /*8 */ addi(2, 0, 7),    // x2 = 7
        /*12*/ add_(3, 1, 2),    // x3 = x1 + x2 = 12
        /*16*/ sub_(4, 2, 1),    // x4 = x2 - x1 = 2
        /*20*/ sw(3, 0, 0),      // mem[0] = x3
        /*24*/ lw(5, 0, 0),      // x5 = mem[0]
        /*28*/ beq(3, 5, 8),     // x3 == x5: taken, skip the next instruction
        /*32*/ addi(6, 0, 99),   // skipped
        /*36*/ andi(7, 3, 6),    // x7 = x3 & 6 = 4
        /*40*/ blt(4, 1, 8),     // x4 < x1: taken, skip the next instruction
        /*44*/ addi(8, 0, 111),  // skipped
        /*48*/ lui(9, 0x1000),   // x9 = 0x1000
        /*52*/ jal(10, 8),       // x10 = pc + 4 = 56, jump to 60
        /*56*/ addi(11, 0, 123), // skipped
        /*60*/ addi(12, 0, 1),   // landing pad
    };
    cpu::Memory memory(program);

    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    Cpu cpu_state = make_cpu(circuit, store, queue);
    assert(bus_value(store, cpu_state.bits.pc) == kResetPcOffset);

    constexpr int kCycles = 12; // program counter advances 4,8,12,...,40,48,52,60,64
    for (int i = 0; i < kCycles; i++) {
        step(circuit, store, queue, cpu_state, memory);
    }

    assert(bus_value(store, cpu_state.bits.pc) == 64u); // one plain addi past the landing pad

    assert(register_value(store, cpu_state, 1) == 5u);
    assert(register_value(store, cpu_state, 2) == 7u);
    assert(register_value(store, cpu_state, 3) == 12u);
    assert(register_value(store, cpu_state, 4) == 2u);
    assert(register_value(store, cpu_state, 5) == 12u);  // loaded back from memory
    assert(register_value(store, cpu_state, 6) == 0u);   // skipped by the taken beq
    assert(register_value(store, cpu_state, 7) == 4u);   // 12 & 6
    assert(register_value(store, cpu_state, 8) == 0u);   // skipped by the taken blt
    assert(register_value(store, cpu_state, 9) == 0x1000u);
    assert(register_value(store, cpu_state, 10) == 56u); // jal's return address
    assert(register_value(store, cpu_state, 11) == 0u);  // skipped by the jump
    assert(register_value(store, cpu_state, 12) == 1u);

    assert(memory.read_word(0) == 12u);

    std::puts("datapath_test: all tests passed");
}

int main() {
    runs_a_small_hand_assembled_program();
    return 0;
}
