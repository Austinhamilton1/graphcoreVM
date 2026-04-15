#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <stdexcept>
#include <cctype>
#include <algorithm>

#include "program.h"

struct OpInfo {
    uint8_t opcode;
    uint8_t subop;
};

class Assembler {
public:
    Program assemble(const std::vector<std::string>& lines) {
        auto cleaned = preprocess(lines);

        // Pass 1: collect labels
        std::unordered_map<std::string, int> labels;
        std::vector<std::vector<std::string>> parsed;

        int pc = 0;
        for (auto& tokens : cleaned) {
            if (is_label(tokens)) {
                std::string label = strip_label(tokens[0]);
                labels[label] = pc;
            } else {
                parsed.push_back(tokens);
                pc++;
            }
        }

        // Pass 2: emit instructions
        std::vector<Instruction> program;
        pc = 0;

        for (auto& tokens : parsed) {
            program.push_back(encode(tokens, labels, pc));
            pc++;
        }

        return { program };
    }

private:

    // ============================
    // Preprocessing
    // ============================

    std::vector<std::vector<std::string>> preprocess(const std::vector<std::string>& lines) {
        std::vector<std::vector<std::string>> result;

        for (auto line : lines) {
            // Remove comments
            auto pos = line.find(';');
            if (pos != std::string::npos) {
                line = line.substr(0, pos);
            }

            trim(line);
            if (line.empty()) continue;

            auto tokens = tokenize(line);
            if (!tokens.empty()) {
                result.push_back(tokens);
            }
        }

        return result;
    }

    std::vector<std::string> tokenize(const std::string& line) {
        std::vector<std::string> tokens;
        std::stringstream ss(line);
        std::string tok;

        while (ss >> tok) {
            // remove commas
            tok.erase(std::remove(tok.begin(), tok.end(), ','), tok.end());
            tokens.push_back(tok);
        }

        return tokens;
    }

    // ============================
    // Label Helpers
    // ============================

    bool is_label(const std::vector<std::string>& tokens) {
        return tokens.size() == 1 && tokens[0].back() == ':';
    }

    std::string strip_label(const std::string& tok) {
        return tok.substr(0, tok.size() - 1);
    }

    // ============================
    // Encoding
    // ============================

    Instruction encode(const std::vector<std::string>& t,
                       const std::unordered_map<std::string, int>& labels,
                       int pc)
    {
        const std::string& op = t[0];

        auto it = op_table().find(op);
        if (it == op_table().end()) {
            throw std::runtime_error("Unknown instruction: " + op);
        }

        Instruction inst{};
        inst.set_opcode(it->second.opcode);
        inst.set_subop(it->second.subop);

        // ---- ALU ----
        if (op == "ADD" || op == "SUB" || op == "MUL" || op == "DIV" ||
            op == "MIN" || op == "MAX" || op == "CMP_LT" || 
            op == "CMP_LTE" || op == "CMP_EQ" || op == "CMP_NEQ") {

            expect(t, 4);
            inst.set_rd(parse_reg(t[1]));
            inst.set_rs1(parse_reg(t[2]));
            inst.set_rs2(parse_reg(t[3]));
        }
        else if (op == "ABS" || op == "MOV") {
            expect(t, 3);
            inst.set_rd(parse_reg(t[1]));
            inst.set_rs1(parse_reg(t[2]));
        }
        else if (op == "LOADI") {
            expect(t, 3);
            inst.set_rd(parse_reg(t[1]));
            inst.set_imm(parse_imm(t[2]));
        }

        // ---- MEMORY ----
        else if (op == "LOADV" || op == "LOADE") {
            expect(t, 3);
            inst.set_rd(parse_reg(t[1]));
            inst.set_imm(parse_imm(t[2]));
        }
        else if (op == "STOREV") {
            expect(t, 3);
            inst.set_rs1(parse_reg(t[1]));
            inst.set_imm(parse_imm(t[2]));
        }

        // ---- GRAPH ----
        else if (op == "ITER_NEIGHBORS" || op == "END_ITER") {
            expect(t, 2);
            int target = resolve_label(t[1], labels);
            int offset = target - pc - 1;
            inst.set_imm(offset);
        }
        else if (op == "SCATTER_IF") {
            expect(t, 2);
            inst.set_rs1(parse_reg(t[1]));
        }
        else if (op == "GATHER_SUM" || op == "GATHER_MIN" ||
                 op == "GATHER_MAX" || op == "GATHER_COUNT") {
            expect(t, 2);
            inst.set_rd(parse_reg(t[1]));
        }
        else if (op == "SCATTER") {
            // no operands
        }

        // ---- CONTROL ----
        else if (op == "JMP") {
            expect(t, 2);
            int target = resolve_label(t[1], labels);
            int offset = target - pc - 1;
            inst.set_imm(offset);
        }
        else if (op == "JZ" || op == "JNZ") {
            expect(t, 3);
            inst.set_rs1(parse_reg(t[1]));
            int target = resolve_label(t[2], labels);
            int offset = target - pc - 1;
            inst.set_imm(offset);
        }
        else if (op == "HALT") {
            // no operands
        }
        else if (op == "NOP") {
            // no operands
        }

        // ---- SYSTEM ----
        else if (op == "VOTE_CHANGE") {
            // no operands
        }

        return inst;
    }

    // ============================
    // Helpers
    // ============================

    int parse_reg(const std::string& tok) {
        if (tok.size() < 2 || tok[0] != 'r') {
            throw std::runtime_error("Invalid register: " + tok);
        }
        return std::stoi(tok.substr(1));
    }

    int parse_imm(const std::string& tok) {
        return std::stoi(tok);
    }

    int resolve_label(const std::string& name,
                      const std::unordered_map<std::string, int>& labels)
    {
        auto it = labels.find(name);
        if (it == labels.end()) {
            throw std::runtime_error("Unknown label: " + name);
        }
        return it->second;
    }

    void expect(const std::vector<std::string>& t, size_t n) {
        if (t.size() != n) {
            throw std::runtime_error("Invalid operand count for: " + t[0]);
        }
    }

    void trim(std::string& s) {
        auto is_space = [](char c) { return std::isspace(c); };

        s.erase(s.begin(), std::find_if_not(s.begin(), s.end(), is_space));
        s.erase(std::find_if_not(s.rbegin(), s.rend(), is_space).base(), s.end());
    }

    // ============================
    // Instruction Table
    // ============================

    const std::unordered_map<std::string, OpInfo>& op_table() {
        static std::unordered_map<std::string, OpInfo> table = {
            // ALU
            {"ADD", {OPCODE_ALU, SUBOP_ADD}},
            {"SUB", {OPCODE_ALU, SUBOP_SUB}},
            {"MUL", {OPCODE_ALU, SUBOP_MUL}},
            {"DIV", {OPCODE_ALU, SUBOP_DIV}},
            {"MIN", {OPCODE_ALU, SUBOP_MIN}},
            {"MAX", {OPCODE_ALU, SUBOP_MAX}},
            {"ABS", {OPCODE_ALU, SUBOP_ABS}},
            {"MOV", {OPCODE_ALU, SUBOP_MOV}},
            {"LOADI", {OPCODE_ALU, SUBOP_LOADI}},
            {"CMP_LT", {OPCODE_ALU, SUBOP_CMPLT}},
            {"CMP_LTE", {OPCODE_ALU, SUBOP_CMPLTE}},
            {"CMP_EQ", {OPCODE_ALU, SUBOP_CMPEQ}},
            {"CMP_NEQ", {OPCODE_ALU, SUBOP_CMPNEQ}},

            // MEMORY
            {"LOADV", {OPCODE_MEMORY, SUBOP_LOADV}},
            {"STOREV", {OPCODE_MEMORY, SUBOP_STOREV}},
            {"LOADE", {OPCODE_MEMORY, SUBOP_LOADE}},

            // GRAPH
            {"ITER_NEIGHBORS", {OPCODE_GRAPH, SUBOP_ITER_NEIGHBORS}},
            {"END_ITER", {OPCODE_GRAPH, SUBOP_END_ITER}},
            {"GATHER_SUM", {OPCODE_GRAPH, SUBOP_GATHER_SUM}},
            {"GATHER_MIN", {OPCODE_GRAPH, SUBOP_GATHER_MIN}},
            {"GATHER_MAX", {OPCODE_GRAPH, SUBOP_GATHER_MAX}},
            {"GATHER_COUNT", {OPCODE_GRAPH, SUBOP_GATHER_COUNT}},
            {"SCATTER", {OPCODE_GRAPH, SUBOP_SCATTER}},
            {"SCATTER_IF", {OPCODE_GRAPH, SUBOP_SCATTER_IF}},

            // CONTROL
            {"NOP", {OPCODE_CONTROL, SUBOP_NOP}},
            {"HALT", {OPCODE_CONTROL, SUBOP_HALT}},
            {"JMP", {OPCODE_CONTROL, SUBOP_JMP}},
            {"JZ", {OPCODE_CONTROL, SUBOP_JZ}},
            {"JNZ", {OPCODE_CONTROL, SUBOP_JNZ}},

            // SYSTEM
            {"VOTE_CHANGE", {OPCODE_SYSTEM, SUBOP_VOTE_CHANGE}},
        };

        return table;
    }
};