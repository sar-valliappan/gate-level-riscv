#pragma once

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "cpu/isa.hpp"
#include "sim/circuit.hpp"
#include "sim/signal.hpp"

namespace alu {

/// Gate-level ALU operation selection
enum class AluOp : std::uint8_t { Add = 0b00, Sub = 0b01, And = 0b10, Slt = 0b11 };

/// AMps an instruction to the ALU operation its execute stage needs.
std::optional<AluOp> decode_alu_op(isa::Mnemonic mnemonic);

/// Splits an AluOp into its two gate-level control bits.
std::pair<sim::Bit, sim::Bit> op_bits(AluOp op);

struct AluBits {
    std::vector<sim::SignalId> result; 
    sim::SignalId zero;                
    sim::SignalId overflow;           
};

AluBits build_alu(sim::Circuit& circuit, sim::SignalStore& store, const std::vector<sim::SignalId>& a, const std::vector<sim::SignalId>& b,
    const std::vector<sim::SignalId>& op, std::uint64_t delay);

} // namespace alu
