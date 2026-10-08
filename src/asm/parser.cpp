#include "asm/parser.hpp"

#include <cctype>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <utility>

namespace assembler {

namespace {

struct Line {
    int number;
    std::string label; // empty if this line has no label
    std::string body;  // empty if this line is a label-only line
};

std::string trim(const std::string& s) {
    std::size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    std::size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> parts;
    std::stringstream ss(s);
    std::string part;
    while (std::getline(ss, part, delim)) parts.push_back(trim(part));
    return parts;
}

/// Splits each source line into an optional label and an optional instruction body.
std::vector<Line> preprocess(const std::string& source) {
    std::vector<Line> lines;
    std::stringstream ss(source);
    std::string raw;
    int number = 0;
    while (std::getline(ss, raw, '\n')) {
        number++;
        std::string text = trim(raw.substr(0, raw.find('#')));
        if (text.empty()) continue;

        Line line{number, "", ""};
        std::size_t colon = text.find(':');
        if (colon != std::string::npos) {
            line.label = trim(text.substr(0, colon));
            text = trim(text.substr(colon + 1));
        }
        line.body = text;
        if (line.label.empty() && line.body.empty()) continue;
        lines.push_back(std::move(line));
    }
    return lines;
}

[[noreturn]] void fail(int line_number, const std::string& message) {
    throw std::runtime_error("line " + std::to_string(line_number) + ": " + message);
}

std::optional<isa::Mnemonic> mnemonic_from_string(const std::string& name) {
    static const std::unordered_map<std::string, isa::Mnemonic> table = {
        {"add", isa::Mnemonic::Add},
        {"sub", isa::Mnemonic::Sub},
        {"addi", isa::Mnemonic::Addi},
        {"andi", isa::Mnemonic::Andi},
        {"lw", isa::Mnemonic::Lw},
        {"sw", isa::Mnemonic::Sw},
        {"beq", isa::Mnemonic::Beq},
        {"blt", isa::Mnemonic::Blt},
        {"lui", isa::Mnemonic::Lui},
        {"jal", isa::Mnemonic::Jal},
    };
    auto it = table.find(name);
    return it != table.end() ? std::optional(it->second) : std::nullopt;
}

std::uint32_t parse_register(const std::string& token, int line_number) {
    if (token.size() < 2 || (token[0] != 'x' && token[0] != 'X')) {
        fail(line_number, "expected a register like 'x5', got '" + token + "'");
    }
    try {
        std::size_t consumed = 0;
        int reg = std::stoi(token.substr(1), &consumed);
        if (consumed != token.size() - 1 || reg < 0 || reg > 31) throw std::invalid_argument("");
        return static_cast<std::uint32_t>(reg);
    } catch (const std::exception&) {
        fail(line_number, "invalid register '" + token + "' (expected x0-x31)");
    }
}

bool looks_numeric(const std::string& token) {
    return !token.empty() && (std::isdigit(static_cast<unsigned char>(token[0])) || token[0] == '-' || token[0] == '+');
}

std::int32_t parse_literal_immediate(const std::string& token, int line_number) {
    try {
        std::size_t consumed = 0;
        long value = std::stol(token, &consumed, 0);
        if (consumed != token.size()) throw std::invalid_argument("");
        return static_cast<std::int32_t>(value);
    } catch (const std::exception&) {
        fail(line_number, "invalid immediate '" + token + "'");
    }
}

/// Resolve branch/jal target operand.
std::int32_t parse_target(const std::string& token, std::uint32_t address, const std::unordered_map<std::string, std::uint32_t>& labels, int line_number) {
    if (looks_numeric(token)) return parse_literal_immediate(token, line_number);
    auto it = labels.find(token);
    if (it == labels.end()) fail(line_number, "undefined label '" + token + "'");
    return static_cast<std::int32_t>(it->second) - static_cast<std::int32_t>(address);
}

/// Parse "imm(rs1)" memory operand
std::pair<std::int32_t, std::uint32_t> parse_mem_operand(const std::string& token, int line_number) {
    std::size_t open = token.find('(');
    std::size_t close = token.find(')');
    if (open == std::string::npos || close == std::string::npos || close < open) {
        fail(line_number, "expected 'imm(rs1)', got '" + token + "'");
    }
    std::int32_t imm = parse_literal_immediate(trim(token.substr(0, open)), line_number);
    std::uint32_t reg = parse_register(trim(token.substr(open + 1, close - open - 1)), line_number);
    return {imm, reg};
}

void expect_operand_count(const std::vector<std::string>& operands, std::size_t count, int line_number) {
    if (operands.size() != count) {
        fail(line_number, "expected " + std::to_string(count) + " operand(s), got " + std::to_string(operands.size()));
    }
}

ParsedInstruction parse_instruction(const std::string& body, std::uint32_t address, const std::unordered_map<std::string, std::uint32_t>& labels, int line_number) {
    std::size_t split_at = body.find_first_of(" \t");
    std::string mnemonic_text = split_at == std::string::npos ? body : body.substr(0, split_at);
    for (char& c : mnemonic_text) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    std::optional<isa::Mnemonic> mnemonic = mnemonic_from_string(mnemonic_text);
    if (!mnemonic.has_value()) fail(line_number, "unknown instruction '" + mnemonic_text + "'");

    std::vector<std::string> operands = split_at == std::string::npos ? std::vector<std::string>{} : split(body.substr(split_at + 1), ',');

    ParsedInstruction instr;
    instr.mnemonic = *mnemonic;
    switch (*mnemonic) {
        case isa::Mnemonic::Add:
        case isa::Mnemonic::Sub:
            expect_operand_count(operands, 3, line_number);
            instr.rd = parse_register(operands[0], line_number);
            instr.rs1 = parse_register(operands[1], line_number);
            instr.rs2 = parse_register(operands[2], line_number);
            break;
        case isa::Mnemonic::Addi:
        case isa::Mnemonic::Andi:
            expect_operand_count(operands, 3, line_number);
            instr.rd = parse_register(operands[0], line_number);
            instr.rs1 = parse_register(operands[1], line_number);
            instr.imm = parse_literal_immediate(operands[2], line_number);
            break;
        case isa::Mnemonic::Lw:
            expect_operand_count(operands, 2, line_number);
            instr.rd = parse_register(operands[0], line_number);
            std::tie(instr.imm, instr.rs1) = parse_mem_operand(operands[1], line_number);
            break;
        case isa::Mnemonic::Sw:
            expect_operand_count(operands, 2, line_number);
            instr.rs2 = parse_register(operands[0], line_number);
            std::tie(instr.imm, instr.rs1) = parse_mem_operand(operands[1], line_number);
            break;
        case isa::Mnemonic::Beq:
        case isa::Mnemonic::Blt:
            expect_operand_count(operands, 3, line_number);
            instr.rs1 = parse_register(operands[0], line_number);
            instr.rs2 = parse_register(operands[1], line_number);
            instr.imm = parse_target(operands[2], address, labels, line_number);
            break;
        case isa::Mnemonic::Lui:
            expect_operand_count(operands, 2, line_number);
            instr.rd = parse_register(operands[0], line_number);
            instr.imm = parse_literal_immediate(operands[1], line_number);
            break;
        case isa::Mnemonic::Jal:
            expect_operand_count(operands, 2, line_number);
            instr.rd = parse_register(operands[0], line_number);
            instr.imm = parse_target(operands[1], address, labels, line_number);
            break;
    }
    return instr;
}

} // namespace

std::vector<ParsedInstruction> parse(const std::string& source) {
    std::vector<Line> lines = preprocess(source);

    std::unordered_map<std::string, std::uint32_t> labels;
    std::uint32_t address = 0;
    for (const Line& line : lines) {
        if (!line.label.empty()) labels[line.label] = address;
        if (!line.body.empty()) address += 4;
    }

    std::vector<ParsedInstruction> instructions;
    address = 0;
    for (const Line& line : lines) {
        if (line.body.empty()) continue;
        instructions.push_back(parse_instruction(line.body, address, labels, line.number));
        address += 4;
    }
    return instructions;
}

} // namespace assembler
