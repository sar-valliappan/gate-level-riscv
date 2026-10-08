#pragma once

#include <cstdint>
#include <vector>

#include "cpu/alu.hpp"
#include "cpu/control.hpp"
#include "logic/register.hpp"
#include "sim/circuit.hpp"
#include "sim/event.hpp"
#include "sim/signal.hpp"

namespace cpu {

struct DatapathBits {
    std::vector<sim::SignalId> pc;             // current PC (register output)
    std::vector<sim::SignalId> mem_addr;       // data memory address (ALU result)
    std::vector<sim::SignalId> mem_write_data; // data memory write data (rs2)
    sim::SignalId mem_write;                   // data memory write enable

    control::ControlBits control;
    alu::AluBits alu;
    logic::RegisterFileBits register_file;
    logic::RegisterBits pc_register;
};

DatapathBits build_datapath(sim::Circuit& circuit, sim::SignalStore& store, const std::vector<sim::SignalId>& instr,
    const std::vector<sim::SignalId>& mem_read_data, sim::SignalId clk, sim::EventQueue& queue, std::uint64_t delay);

} // namespace cpu
