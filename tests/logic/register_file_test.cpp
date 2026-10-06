#include "logic/register.hpp"
#include "sim/circuit.hpp"
#include "sim/event.hpp"
#include "sim/signal.hpp"
#include "test_utils.hpp"

#include <cassert>
#include <cstdio>
#include <string>
#include <vector>

namespace {

constexpr std::size_t kWidth = 8;    // word width, kept small so the test circuit builds fast
constexpr std::size_t kAddrBits = 3; // 8 addressable registers

struct RegFile {
    std::vector<sim::SignalId> rs1;
    std::vector<sim::SignalId> rs2;
    std::vector<sim::SignalId> rd;
    std::vector<sim::SignalId> write_data;
    sim::SignalId write_enable;
    sim::SignalId clk;
    logic::RegisterFileBits bits;
};

std::vector<sim::SignalId> alloc_bus(sim::SignalStore& store, const std::string& prefix, std::size_t width) {
    std::vector<sim::SignalId> bus;
    bus.reserve(width);
    for (std::size_t i = 0; i < width; i++) {
        bus.push_back(store.alloc(prefix + std::to_string(i), sim::Bit::Zero));
    }
    return bus;
}

void set_bus(sim::EventQueue& queue, const std::vector<sim::SignalId>& bus, unsigned value) {
    for (std::size_t i = 0; i < bus.size(); i++) {
        sim::Bit b = ((value >> i) & 1) ? sim::Bit::One : sim::Bit::Zero;
        queue.schedule(queue.current_time + 10, bus[i], b);
    }
}

bool bus_equals(const sim::SignalStore& store, const std::vector<sim::SignalId>& bus, unsigned value) {
    for (std::size_t i = 0; i < bus.size(); i++) {
        sim::Bit expected = ((value >> i) & 1) ? sim::Bit::One : sim::Bit::Zero;
        if (store.get(bus[i]) != expected) return false;
    }
    return true;
}

RegFile make_register_file(sim::Circuit& circuit, sim::SignalStore& store, sim::EventQueue& queue) {
    RegFile rf;
    rf.rs1 = alloc_bus(store, "RS1_", kAddrBits);
    rf.rs2 = alloc_bus(store, "RS2_", kAddrBits);
    rf.rd = alloc_bus(store, "RD_", kAddrBits);
    rf.write_data = alloc_bus(store, "WD_", kWidth);
    rf.write_enable = store.alloc("WE", sim::Bit::Zero);
    rf.clk = store.alloc("CLK", sim::Bit::Zero);

    rf.bits = logic::build_register_file(circuit, store, rf.rs1, rf.rs2, rf.rd, rf.write_data, rf.write_enable, rf.clk, 1);

    std::vector<sim::SignalId> master_feedbacks;
    std::vector<sim::SignalId> slave_feedbacks;
    for (const logic::RegisterBits& reg : rf.bits.registers) {
        master_feedbacks.insert(master_feedbacks.end(), reg.master_q_not.begin(), reg.master_q_not.end());
        slave_feedbacks.insert(slave_feedbacks.end(), reg.q_not.begin(), reg.q_not.end());
    }

    std::vector<sim::SignalId> inputs;
    inputs.insert(inputs.end(), rf.rs1.begin(), rf.rs1.end());
    inputs.insert(inputs.end(), rf.rs2.begin(), rf.rs2.end());
    inputs.insert(inputs.end(), rf.rd.begin(), rf.rd.end());
    inputs.insert(inputs.end(), rf.write_data.begin(), rf.write_data.end());
    inputs.push_back(rf.write_enable);
    inputs.push_back(rf.clk);

    test_utils::bootstrap_register(circuit, store, queue, master_feedbacks, slave_feedbacks, sim::Bit::One, inputs);

    return rf;
}

void write(sim::EventQueue& queue, sim::Circuit& circuit, sim::SignalStore& store, RegFile& rf, unsigned addr, unsigned value) {
    set_bus(queue, rf.rd, addr);
    set_bus(queue, rf.write_data, value);
    queue.schedule(queue.current_time + 10, rf.write_enable, sim::Bit::One);
    test_utils::settle(circuit, store, queue);

    queue.schedule(queue.current_time + 10, rf.clk, sim::Bit::One); // rising edge: commit the write
    test_utils::settle(circuit, store, queue);

    queue.schedule(queue.current_time + 10, rf.clk, sim::Bit::Zero);
    queue.schedule(queue.current_time + 10, rf.write_enable, sim::Bit::Zero);
    test_utils::settle(circuit, store, queue);
}

} // namespace

static void write_then_read_same_register() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    RegFile rf = make_register_file(circuit, store, queue);

    write(queue, circuit, store, rf, 5, 0xA5);

    set_bus(queue, rf.rs1, 5);
    test_utils::settle(circuit, store, queue);
    assert(bus_equals(store, rf.bits.read_data1, 0xA5));
}

static void two_read_ports_read_independently() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    RegFile rf = make_register_file(circuit, store, queue);

    write(queue, circuit, store, rf, 2, 0x11);
    write(queue, circuit, store, rf, 3, 0x22);

    set_bus(queue, rf.rs1, 2);
    set_bus(queue, rf.rs2, 3);
    test_utils::settle(circuit, store, queue);

    assert(bus_equals(store, rf.bits.read_data1, 0x11));
    assert(bus_equals(store, rf.bits.read_data2, 0x22));
}

static void write_without_enable_is_ignored() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    RegFile rf = make_register_file(circuit, store, queue);

    set_bus(queue, rf.rd, 4);
    set_bus(queue, rf.write_data, 0xFF);
    test_utils::settle(circuit, store, queue);

    queue.schedule(queue.current_time + 10, rf.clk, sim::Bit::One); // rising edge, but write_enable is low
    test_utils::settle(circuit, store, queue);

    set_bus(queue, rf.rs1, 4);
    test_utils::settle(circuit, store, queue);
    assert(bus_equals(store, rf.bits.read_data1, 0));
}

static void write_to_register_zero_works_like_any_other() {
    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    RegFile rf = make_register_file(circuit, store, queue);

    write(queue, circuit, store, rf, 0, 0xFF);

    set_bus(queue, rf.rs1, 0);
    test_utils::settle(circuit, store, queue);
    assert(bus_equals(store, rf.bits.read_data1, 0xFF));
}

int main() {
    write_then_read_same_register();
    two_read_ports_read_independently();
    write_without_enable_is_ignored();
    write_to_register_zero_works_like_any_other();
    std::puts("register_file_test: all tests passed");
    return 0;
}
