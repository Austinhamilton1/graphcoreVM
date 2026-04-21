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

    std::string to_string() {
        switch (op) {
            case LOp::ADD:
                return "ADD";
            case LOp::SUB:
                return "SUB";
            case LOp::MUL:
                return "MUL";
            case LOp::DIV:
                return "DIV";
            case LOp::MIN:
                return "MIN";
            case LOp::MAX:
                return "MAX";
            case LOp::ABS:
                return "ABS";
            case LOp::MOV:
                return "MOV";
            case LOp::LOADI:
                return "LOADI";
            case LOp::CMPLT:
                return "CMPLT";
            case LOp::CMPLTE:
                return "CMPLTE";
            case LOp::CMPEQ:
                return "CMPEQ";
            case LOp::CMPNEQ:
                return "CMPNEQ";
            case LOp::JMP:
                return "JMP";
            case LOp::JZ:
                return "JZ";
            case LOp::JNZ:
                return "JNZ";
            case LOp::LOAD_V_SELF:
                return "LOAD_V_SELF";
            case LOp::LOAD_N_VAL:
                return "LOAD_N_VAL";
            case LOp::LOAD_IN_DEG:
                return "LOAD_IN_DEG";
            case LOp::LOAD_OUT_DEG:
                return "LOAD_OUT_DEG";
            case LOp::LOAD_GRAPH_SIZE:
                return "LOAD_GRAPH_SIZE";
            case LOp::STORE_V_OUT:
                return "STORE_V_OUT";
            case LOp::LOAD_E_ATTR:
                return "LOAD_E_ATTR";
            case LOp::GATHER_SUM:
                return "GATHER_SUM";
            case LOp::GATHER_MIN:
                return "GATHER_MIN";
            case LOp::GATHER_MAX:
                return "GATHER_MAX";
            case LOp::GATHER_COUNT:
                return "GATHER_COUNT";
            case LOp::SCATTER:
                return "SCATTER";
            case LOp::SCATTER_IF:
                return "SCATTER_IF";
            case LOp::ITER_BEGIN:
                return "ITER_BEGIN";
            case LOp::ITER_NEXT:
                return "ITER_NEXT";
            case LOp::VOTE_CHANGE:
                return "VOTE_CHANGE";
            case LOp::RETURN:
                return "RETURN";
            default:
                return "??";
        }
    }
};