#include "logic/register.hpp"

#include "logic/flipflop.hpp"
#include "logic/gates.hpp"
#include "logic/mux.hpp"

namespace logic {

namespace {

std::vector<sim::SignalId> build_decoder(sim::Circuit& circuit, sim::SignalStore& store, const std::vector<sim::SignalId>& addr, std::uint64_t delay) {
    std::vector<sim::SignalId> addr_bar;
    addr_bar.reserve(addr.size());
    for (sim::SignalId bit : addr) {
        addr_bar.push_back(build_not(circuit, store, bit, delay));
    }

    std::size_t count = std::size_t(1) << addr.size();
    std::vector<sim::SignalId> lines;
    lines.reserve(count);
    for (std::size_t i = 0; i < count; i++) {
        sim::SignalId term = ((i >> 0) & 1) ? addr[0] : addr_bar[0];
        for (std::size_t j = 1; j < addr.size(); j++) {
            sim::SignalId bit_line = ((i >> j) & 1) ? addr[j] : addr_bar[j];
            term = build_and(circuit, store, term, bit_line, delay);
        }
        lines.push_back(term);
    }
    return lines;
}

} // namespace

RegisterBits build_register(sim::Circuit& circuit, sim::SignalStore& store, const std::vector<sim::SignalId>& d, sim::SignalId clk, std::uint64_t delay) {
    RegisterBits bits;
    bits.q.reserve(d.size());
    bits.q_not.reserve(d.size());
    bits.master_q_not.reserve(d.size());

    for (sim::SignalId bit : d) {
        auto [q, q_not, master_q_not] = build_d_flipflop(circuit, store, bit, clk, delay);
        bits.q.push_back(q);
        bits.q_not.push_back(q_not);
        bits.master_q_not.push_back(master_q_not);
    }

    return bits;
}

RegisterFileBits build_register_file(sim::Circuit& circuit, sim::SignalStore& store, const std::vector<sim::SignalId>& rs1, const std::vector<sim::SignalId>& rs2,
    const std::vector<sim::SignalId>& rd, const std::vector<sim::SignalId>& write_data, sim::SignalId write_enable, sim::SignalId clk, std::uint64_t delay) {
    std::vector<sim::SignalId> write_select = build_decoder(circuit, store, rd, delay);
    std::size_t count = write_select.size();

    RegisterFileBits file;
    file.registers.reserve(count);
    for (std::size_t i = 0; i < count; i++) {
        sim::SignalId write_this = build_and(circuit, store, write_select[i], write_enable, delay);
        sim::SignalId reg_clk = build_and(circuit, store, write_this, clk, delay);
        file.registers.push_back(build_register(circuit, store, write_data, reg_clk, delay));
    }

    std::size_t width = write_data.size();
    file.read_data1.reserve(width);
    file.read_data2.reserve(width);
    for (std::size_t bit = 0; bit < width; bit++) {
        std::vector<sim::SignalId> column;
        column.reserve(count);
        for (std::size_t i = 0; i < count; i++) {
            column.push_back(file.registers[i].q[bit]);
        }
        file.read_data1.push_back(build_mux(circuit, store, column, rs1, delay));
        file.read_data2.push_back(build_mux(circuit, store, column, rs2, delay));
    }

    return file;
}

} // namespace logic
