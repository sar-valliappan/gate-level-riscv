#pragma once

#include <cstdint>
#include <vector>

#include "sim/circuit.hpp"
#include "sim/signal.hpp"

namespace control {

/// Main control unit outputs.
struct ControlBits {
    sim::SignalId reg_write;
    sim::SignalId alu_src;                  // 0: register rs2, 1: immediate
    sim::SignalId mem_write;
    std::vector<sim::SignalId> result_src;  // 2 bits: 00 ALU result, 01 memory read data, 10 PC+4, 11 immediate
    sim::SignalId branch;
    sim::SignalId jump;
    std::vector<sim::SignalId> format;      // 3 bits; matches isa::Format encoding
    std::vector<sim::SignalId> alu_op;      // 2 bits; matches alu::AluOp encoding
};

/// Decodes opcode/funct3/funct7 into datapath control signals.
ControlBits build_control(sim::Circuit& circuit, sim::SignalStore& store, const std::vector<sim::SignalId>& opcode, const std::vector<sim::SignalId>& funct3,
    const std::vector<sim::SignalId>& funct7, std::uint64_t delay);

} // namespace control
