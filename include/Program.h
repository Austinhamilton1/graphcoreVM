#pragma once

#include <cstdint>
#include <vector>

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
#define SUBOP_CMPLTE            0x10
#define SUBOP_CMPEQ             0x11
#define SUBOP_CMPNEQ            0x12

#define SUBOP_LOADV             0x00
#define SUBOP_STOREV            0x01
#define SUBOP_LOADE             0x02

#define SUBOP_ITER_NEIGHBORS    0x00
#define SUBOP_END_ITER          0x01
#define SUBOP_GATHER_SUM        0x02
#define SUBOP_GATHER_MIN        0x03
#define SUBOP_GATHER_MAX        0x04
#define SUBOP_GATHER_COUNT      0x05
#define SUBOP_SCATTER           0x06
#define SUBOP_SCATTER_IF        0x07

#define SUBOP_BARRIER           0x00
#define SUBOP_VOTE_CHANGE       0x01

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

    // Return the subop (bits 4 - 7)
    uint8_t subop() const {
        return (raw >> 24) & 0x0F;
    }

    // Return the destination register (bits 8 - 12)
    uint8_t rd() const {
        return (raw >> 19) & 0x1F;
    }

    // Return the first source register (bits 13 - 17)
    uint8_t rs1() const {
        return (raw >> 14) & 0x1F;
    }

    // Return the mode value (bits 13 - 17)
    uint8_t mode() const {
        return (raw >> 14) & 0x1F;
    }

    // Return the second source register (bits 27 - 31)
    uint8_t rs2() const {
        return raw & 0x1F;
    }

    // Return the immediate value (bits 18 - 31)
    int16_t imm() const {
        int16_t raw_data = raw & 0x3FFF;
        return (static_cast<int16_t>(raw_data << 2)) >> 2;    
    }

    // Return the offset value (bits 18 - 31)
    int16_t offset() const {
        int16_t raw_data = raw & 0x3FFF;
        return (static_cast<int16_t>(raw_data << 6)) >> 6;
    }

    // Return the flags value (bits 18 - 31)
    uint16_t flags() const {
        return raw & 0x3FFF;
    }
};

/*
 * In GCVM, a program is just a series of instructions.
 * (This may be improved upon in later iterations).
 */
struct Program {
    std::vector<Instruction> code;
};