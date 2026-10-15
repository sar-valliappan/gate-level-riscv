#include "asm/encoder.hpp"
#include "asm/parser.hpp"
#include "cpu/memory.hpp"
#include "cpu/runner.hpp"
#include "sim/circuit.hpp"
#include "sim/event.hpp"
#include "sim/signal.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr int kDefaultCycles = 100;

std::string read_file(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("cannot open '" + path + "'");
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void print_registers(const sim::SignalStore& store, const cpu::Runner& runner) {
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 4; col++) {
            int reg = row + col * 8;
            std::printf("x%-2d = 0x%08x   ", reg, cpu::register_value(store, runner, reg));
        }
        std::printf("\n");
    }
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <program.s> [cycles]\n", argv[0]);
        return 1;
    }
    int cycles = argc >= 3 ? std::atoi(argv[2]) : kDefaultCycles;

    std::vector<std::uint32_t> words;
    try {
        std::string source = read_file(argv[1]);
        words = assembler::encode(assembler::parse(source));
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }

    cpu::Memory memory = cpu::make_memory(std::move(words));

    sim::SignalStore store;
    sim::Circuit circuit;
    sim::EventQueue queue;
    cpu::Runner runner = cpu::build_runner(circuit, store, queue);

    for (int i = 0; i < cycles; i++) {
        cpu::step(circuit, store, queue, runner, memory);
    }

    std::printf("ran %d cycles, pc = 0x%08x\n\n", cycles, cpu::pc_value(store, runner));
    print_registers(store, runner);
    return 0;
}
