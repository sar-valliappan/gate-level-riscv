#include "cpu/immgen.hpp"

#include <cassert>

#include "logic/mux.hpp"

namespace immgen {

namespace {

constexpr std::size_t kWidth = 32;

} // namespace

std::array<sim::Bit, 3> format_bits(isa::Format format) {
    auto code = static_cast<std::uint8_t>(format);
    return {
        (code & 0b001) ? sim::Bit::One : sim::Bit::Zero,
        (code & 0b010) ? sim::Bit::One : sim::Bit::Zero,
        (code & 0b100) ? sim::Bit::One : sim::Bit::Zero,
    };
}

std::vector<sim::SignalId> build_immgen(sim::Circuit& circuit, sim::SignalStore& store, const std::vector<sim::SignalId>& instr,
    const std::vector<sim::SignalId>& format, std::uint64_t delay) {
    assert(instr.size() == kWidth);
    assert(format.size() == 3);

    sim::SignalId zero_const = store.alloc("immgen_zero_const", sim::Bit::Zero);
    sim::SignalId sign = instr[31];

    // imm[11:0] = instr[31:20], sign-extended from instr[31].
    auto i_src = [&](std::size_t i) { return i < 12 ? instr[20 + i] : sign; };

    // imm[4:0] = instr[11:7], imm[11:5] = instr[31:25], sign-extended from instr[31].
    auto s_src = [&](std::size_t i) {
        if (i < 5) return instr[7 + i];
        if (i < 12) return instr[20 + i];
        return sign;
    };

    // imm[0] = 0, imm[4:1] = instr[11:8], imm[10:5] = instr[30:25], imm[11] = instr[7], imm[12] = instr[31], sign-extended from instr[31].
    auto b_src = [&](std::size_t i) -> sim::SignalId {
        if (i == 0) return zero_const;
        if (i <= 4) return instr[7 + i];
        if (i <= 10) return instr[20 + i];
        if (i == 11) return instr[7];
        return sign;
    };

    // imm[11:0] = 0, imm[31:12] = instr[31:12].
    auto u_src = [&](std::size_t i) -> sim::SignalId { return i < 12 ? zero_const : instr[i]; };

    // imm[0] = 0, imm[10:1] = instr[30:21], imm[11] = instr[20], imm[19:12] = instr[19:12], imm[20] = instr[31], sign-extended from instr[31].
    auto j_src = [&](std::size_t i) -> sim::SignalId {
        if (i == 0) return zero_const;
        if (i <= 10) return instr[20 + i];
        if (i == 11) return instr[20];
        if (i <= 19) return instr[i];
        return sign;
    };

    std::vector<sim::SignalId> imm(kWidth);
    for (std::size_t i = 0; i < kWidth; i++) {
        std::vector<sim::SignalId> candidates = {
            zero_const, // R: no immediate
            i_src(i),
            s_src(i),
            b_src(i),
            u_src(i),
            j_src(i),
            zero_const, // unused select
            zero_const, // unused select
        };
        imm[i] = logic::build_mux(circuit, store, candidates, format, delay);
    }

    return imm;
}

} // namespace immgen
