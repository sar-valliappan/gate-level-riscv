#include "cpu/runner.hpp"

#include <string>

#include "sim/simulate.hpp"

namespace cpu {

namespace {

constexpr std::size_t kWidth = 32;
constexpr std::size_t kMaxEvents = 2'000'000; // a full 32x32 register file needs far more than settle()'s default budget

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

constexpr int kResetPulses = 2;

} // namespace

Runner build_runner(sim::Circuit& circuit, sim::SignalStore& store, sim::EventQueue& queue) {
    Runner runner;
    runner.instr_bus = alloc_bus(store, "INSTR_", kWidth);
    runner.mem_read_bus = alloc_bus(store, "MEMRD_", kWidth);
    runner.clk = store.alloc("CLK", sim::Bit::Zero);

    runner.bits = build_datapath(circuit, store, runner.instr_bus, runner.mem_read_bus, runner.clk, queue, 1);

    std::vector<sim::SignalId> master_feedbacks = runner.bits.pc_register.master_q_not;
    std::vector<sim::SignalId> slave_feedbacks = runner.bits.pc_register.q_not;
    for (const logic::RegisterBits& reg : runner.bits.register_file.registers) {
        master_feedbacks.insert(master_feedbacks.end(), reg.master_q_not.begin(), reg.master_q_not.end());
        slave_feedbacks.insert(slave_feedbacks.end(), reg.q_not.begin(), reg.q_not.end());
    }

    std::vector<sim::SignalId> inputs;
    inputs.insert(inputs.end(), runner.instr_bus.begin(), runner.instr_bus.end());
    inputs.insert(inputs.end(), runner.mem_read_bus.begin(), runner.mem_read_bus.end());
    inputs.push_back(runner.clk);

    // Seeds PC = 0 and every register file entry = 0.
    sim::bootstrap_register(circuit, store, queue, master_feedbacks, slave_feedbacks, sim::Bit::One, inputs, kMaxEvents);

    for (int pulse = 0; pulse < kResetPulses; pulse++) {
        queue.schedule(queue.current_time + 10, runner.clk, sim::Bit::One);
        sim::settle(circuit, store, queue, kMaxEvents);
        queue.schedule(queue.current_time + 10, runner.clk, sim::Bit::Zero);
        sim::settle(circuit, store, queue, kMaxEvents);
    }

    return runner;
}

void step(sim::Circuit& circuit, sim::SignalStore& store, sim::EventQueue& queue, Runner& runner, Memory& memory) {
    std::uint32_t pc = bus_value(store, runner.bits.pc);
    set_bus(queue, runner.instr_bus, memory.read_instruction(pc));
    sim::settle(circuit, store, queue, kMaxEvents);

    std::uint32_t addr = bus_value(store, runner.bits.mem_addr);
    set_bus(queue, runner.mem_read_bus, memory.read_word(addr));
    sim::settle(circuit, store, queue, kMaxEvents);

    if (store.get(runner.bits.mem_write) == sim::Bit::One) {
        memory.write_word(addr, bus_value(store, runner.bits.mem_write_data));
    }

    queue.schedule(queue.current_time + 10, runner.clk, sim::Bit::One); // rising edge: commit PC + reg file
    sim::settle(circuit, store, queue, kMaxEvents);
    queue.schedule(queue.current_time + 10, runner.clk, sim::Bit::Zero);
    sim::settle(circuit, store, queue, kMaxEvents);
}

Memory make_memory(std::vector<std::uint32_t> program) {
    program.insert(program.begin(), 0u);
    return Memory(std::move(program));
}

std::uint32_t pc_value(const sim::SignalStore& store, const Runner& runner) { return bus_value(store, runner.bits.pc); }

std::uint32_t register_value(const sim::SignalStore& store, const Runner& runner, std::size_t index) {
    return bus_value(store, runner.bits.register_file.registers[index].q);
}

} // namespace cpu
