#include "cpu/memory.hpp"

namespace cpu {

namespace {
constexpr std::size_t kDataWords = 1024;
} // namespace

Memory::Memory(std::vector<std::uint32_t> program) : instructions_(std::move(program)), data_(kDataWords, 0) {}

std::uint32_t Memory::read_instruction(std::uint32_t pc) const {
    std::size_t index = pc / 4;
    return index < instructions_.size() ? instructions_[index] : 0;
}

std::uint32_t Memory::read_word(std::uint32_t addr) const {
    std::size_t index = addr / 4;
    return index < data_.size() ? data_[index] : 0;
}

void Memory::write_word(std::uint32_t addr, std::uint32_t value) {
    std::size_t index = addr / 4;
    if (index < data_.size()) data_[index] = value;
}

} // namespace cpu
