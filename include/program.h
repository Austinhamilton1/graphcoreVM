#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <fstream>

#define OPCODE_CONTROL          0x00
#define OPCODE_ALU              0x01
#define OPCODE_MEMORY           0x02
#define OPCODE_GRAPH            0x03
#define OPCODE_SYSTEM           0x04

#define SUBOP_NOP               0x00
#define SUBOP_HALT              0x01
#define SUBOP_JMP               0x02
#define SUBOP_JZ                0x03
#define SUBOP_JNZ               0x04

#define SUBOP_ADD               0x00
#define SUBOP_SUB               0x01
#define SUBOP_MUL               0x02
#define SUBOP_DIV               0x03
#define SUBOP_MIN               0x04
#define SUBOP_MAX               0x05
#define SUBOP_ABS               0x06
#define SUBOP_MOV               0x07
#define SUBOP_LOADI             0x08
#define SUBOP_CMPLT             0x09
#define SUBOP_CMPLTE            0x0A
#define SUBOP_CMPEQ             0x0B
#define SUBOP_CMPNEQ            0x0C

#define SUBOP_LOADV             0x00
#define SUBOP_STOREV            0x01
#define SUBOP_LOADE             0x02
#define SUBOP_LOADG             0x03

#define SUBOP_ITER_NEIGHBORS    0x00
#define SUBOP_END_ITER          0x01
#define SUBOP_GATHER_SUM        0x02
#define SUBOP_GATHER_MIN        0x03
#define SUBOP_GATHER_MAX        0x04
#define SUBOP_GATHER_COUNT      0x05
#define SUBOP_SCATTER           0x06
#define SUBOP_SCATTER_IF        0x07

#define SUBOP_VOTE_CHANGE       0x00

/*
 * In GCVM, program instructions are 32-bit fixed-width.
 * The encoding is explained in docs/encoding.md
 */
struct Instruction {
    uint32_t raw;

    // Return the opcode (bits 0 - 3)
    uint8_t opcode() const {
        return (raw >> 28) & 0x0F;
    }

    // Set the opcode (bits 0 - 3)
    void set_opcode(uint8_t opcode) {
        raw |= (opcode & 0x0F) << 28;
    }

    // Return the subop (bits 4 - 7)
    uint8_t subop() const {
        return (raw >> 24) & 0x0F;
    }

    // Set the subop (bits 4 - 7)
    void set_subop(uint8_t subop) {
        raw |= (subop & 0x0F) << 24;
    }

    // Return the destination register (bits 8 - 12)
    uint8_t rd() const {
        return (raw >> 19) & 0x1F;
    }

    // Set the desination register (bits 8 - 12)
    void set_rd(uint8_t rd) {
        raw |= (rd & 0x1F) << 19;
    }

    // Return the first source register (bits 13 - 17)
    uint8_t rs1() const {
        return (raw >> 14) & 0x1F;
    }

    // Set the first source register (bits 13 - 17)
    void set_rs1(uint8_t rs1) {
        raw |= (rs1 & 0x1F) << 14;
    }

    // Return the mode value (bits 13 - 17)
    uint8_t mode() const {
        return (raw >> 14) & 0x1F;
    }

    // Set the mode value (bits 13 - 17)
    void set_mode(uint8_t mode) {
        raw |= (mode & 0x1F) << 14;
    }

    // Return the second source register (bits 27 - 31)
    uint8_t rs2() const {
        return raw & 0x1F;
    }

    // Set the second source register (bits 27 - 31)
    void set_rs2(uint8_t rs2) {
        raw |= (rs2 & 0x1F);
    }

    // Return the immediate value (bits 18 - 31)
    int16_t imm() const {
        int16_t raw_data = raw & 0x3FFF;
        return (static_cast<int16_t>(raw_data << 2)) >> 2;    
    }

    // Set the immediate value (bits 18 - 31)
    void set_imm(int16_t imm) {
        raw |= (imm & 0x3FFF);
    }

    // Return the offset value (bits 18 - 31)
    int16_t offset() const {
        int16_t raw_data = raw & 0x3FFF;
        return (static_cast<int16_t>(raw_data << 2)) >> 2;
    }
    // Set the offset value (bits 18 - 31)
    void set_offset(int16_t offset) {
        raw |= (offset & 0x3FFF);
    }

    // Return the flags value (bits 18 - 31)
    uint16_t flags() const {
        return raw & 0x3FFF;
    }

    // Set the flags value (bits 18 - 31)
    void set_flags(int16_t flags) {
        raw |= (flags & 0x3FFF);
    }
};

/*
 * In GCVM, a program is just a series of instructions.
 * (This may be improved upon in later iterations).
 */
struct Program {
    std::vector<Instruction> code;

    /*
     * Read bytecode from a binary instruction file.
     * Arguments:
     *     const std::string& filename - Name of the bytecode file.
     */
    void from_file(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);

        if(!file.is_open()) {
            throw std::runtime_error("Could not open bytecode file");
        }

        // Read in an instruction at a time
        uint32_t raw_instr;
        while(file.read(reinterpret_cast<char *>(&raw_instr), sizeof(raw_instr))) {
            Instruction instr = { raw_instr };
            code.push_back(instr);
        }
    }
};