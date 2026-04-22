#pragma once

#include <cstdint>
#include <atomic>
#include <vector>
#include <asmjit/x86.h>

#include "graph.h"
#include "program.h"
#include "lowered_ir.h"
#include "state.h"

using namespace asmjit;

/*
 * Determines what from a Graph the JIT can see.
 */
struct JITGraph {
    uint32_t *row_offsets_out;  // Outgoing row offsets
    uint32_t *col_indices_out;  // Outgoing column indices
    double *edge_attr_out;      // Outgoing_edge attributes

    uint32_t *row_offsets_in;   // Incoming row offsets
    uint32_t *col_indices_in;   // Incoming column indices
    double *edge_attr_in;       // Incoming edge attributes
};

/*
 * Determines what from a VertexState the JIT can see.
 */
struct JITVertexState {
    double *v_self;         // Current vertex state
    uint8_t *next_active;   // Next active vertex state
};

/*
 * Determines what from the runtime the JIT can see.
 */
struct JITGCVM {
    JITGraph graph;
    JITVertexState vertices;
    std::atomic<uint32_t> *updates;
};

/* This is the function signature of our JIT kernel. */
using JITFunc = void(*)(Context *, uint32_t, JITGCVM *);

class JIT {
private:
    JitRuntime rt;              // Runtime for adding code
    JITGCVM jit_runtime;        // Runtime for JIT kernel
    bool initialized = false;   // Is the runtime initialized?

    /*
     * Lower a program to LIR format.
     * Arguments:
     *     const Program &program - The program to lower.
     * Returns:
     *     std::vector<LInstruction> - Lowered IR version of the program.
     */
    std::vector<LInstruction> lower(const Program &program) const;

public:
    /*
     * Constructor.
     * Arguments:
     *     const Graph &graph - Underlying CSR graph of runtime.
     *     const VertexState &state - Underlying vertex state of the runtime.
     *     std::atomic<uint32_t> *updates - Atomic update counter.
     */
    void init(Graph &graph, VertexState &state, std::atomic<uint32_t> *updates);

    /*
     * Compile a JIT kernel given the lowered instructions.
     * Arguments:
     *     const Program &program - The lowered instructions to compile.
     * Returns:
     *     JITFunc - The compiled kernel.
     */
    JITFunc compile(const Program &program);

    /*
     * Return a reference to the JIT runtime for kernel execution.
     * Returns:
     *     JITGCVM * - Pointer to JIT runtime.
     */
    JITGCVM *get_runtime();

    /*
     * Is the runtime initialized?
     * Returns:
     *     bool - true if initialized, false otherwise.
     */
    bool is_initialized() const;
};