#pragma once

#include <vector>
#include <cstdint>

enum class LOp {
    // ALU
    ADD,
    SUB,
    MUL,
    DIV,
    MIN,
    MAX,
    ABS,
    MOV,

    // Constants
    LOADI,

    // Comparisons
    CMPLT,
    CMPLTE,
    CMPEQ,
    CMPNEQ,

    // Control flow
    JMP,
    JZ,
    JNZ,

    // Memory (vertex context)
    LOAD_V_SELF,
    LOAD_N_VAL,
    LOAD_IN_DEG,
    LOAD_OUT_DEG,
    LOAD_GRAPH_SIZE,

    STORE_V_OUT,

    LOAD_E_ATTR,

    // Graph ops (keep as calls for now)
    GATHER_SUM,
    GATHER_MIN,
    GATHER_MAX,
    GATHER_COUNT,

    SCATTER,
    SCATTER_IF,

    ITER_BEGIN,
    ITER_NEXT,

    // System
    VOTE_CHANGE,

    // Exit
    RETURN
};

struct LInstruction {
    LOp op;

    int dst = -1;
    int src1 = -1;
    int src2 = -1;

    double imm = 0.0;

    int target = -1; // for jumps
};