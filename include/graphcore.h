#pragma once

#include <vector>
#include <cstdint>
#include <initializer_list>
#include <atomic>
#include <algorithm>
#include <unordered_set>

#include "state.h"
#include "graph.h"
#include "program.h"
#include "jit.h"

/*
 * Debugging information returned by debug functions.
 */
struct DebugInfo {
    int pc;                 // Current program counter
    Context ctx;            // Current context
    VertexState *vstate;    // Current vertex state
    uint32_t update_value;  // Current update value
    bool finished;          // Is the program done for all vertices?
};

/*
 * Manages state of debugging.
 */
struct Debugger {
    std::unordered_set<int> breakpoints;    // List of break points
    int pc = 0;                             // Current program counter
    uint32_t current_vertex = 0;            // Current vertex
    bool finished = false;                  // Is the program done for all vertices?
};

/* 
 * Graph Runtime. This class manages the runtime.
 * It acts as an execution system for GCVM bytecode.
 */
class GCVM {
private:
    Graph graph;                    // CSR format graph
    Program program;                // Bytecode program
    VertexState vertices;           // SoA of vertex states
    std::atomic<uint32_t> updates;  // Number of vertices affected by kernel
    std::vector<LInstruction> lir;  // Lowered IR representation for JIT compilation
    JIT jit;                        // JIT runtime for compiling kernels
    JITFunc jit_compiled;           // JIT compiled kernel

    /* Used for debugging. */
    Debugger dbg;

    /*
     * Execute a CONTROL operation.
     * Arguments:
     *     const Instruction &inst - Instruction to execute.
     *     Context &ctx - Vertex context.
     *     int pc - Current program counter.
     * Returns:
     *     int - New program counter. 
     */
    int exec_control(const Instruction &inst, Context &ctx, int pc);

    /* Dispatch functions for CONTROL operations. */
    int exec_nop(const Instruction &inst, Context &ctx, int pc);
    int exec_halt(const Instruction &inst, Context &ctx, int pc);
    int exec_jmp(const Instruction &inst, Context &ctx, int pc);
    int exec_jz(const Instruction &inst, Context &ctx, int pc);
    int exec_jnz(const Instruction &inst, Context &ctx, int pc);

    /*
     * Execute an ALU operation.
     * Arguments:
     *     const Instruction &inst - Instruction to exectute.
     *     Context &ctx - Vertex context.
     */
    void exec_alu(const Instruction &inst, Context &ctx);

    /* Dispatch functions for ALU operations. */
    void exec_add(const Instruction &inst, Context &ctx);
    void exec_sub(const Instruction &inst, Context &ctx);
    void exec_mul(const Instruction &inst, Context &ctx);
    void exec_div(const Instruction &inst, Context &ctx);
    void exec_min(const Instruction &inst, Context &ctx);
    void exec_max(const Instruction &inst, Context &ctx);
    void exec_abs(const Instruction &inst, Context &ctx);
    void exec_mov(const Instruction &inst, Context &ctx);
    void exec_loadi(const Instruction &inst, Context &ctx);
    void exec_cmp_lt(const Instruction &inst, Context &ctx);
    void exec_cmp_lte(const Instruction &inst, Context &ctx);
    void exec_cmp_eq(const Instruction &inst, Context &ctx);
    void exec_cmp_neq(const Instruction &inst, Context &ctx);

    /*
     * Execute a MEMORY operation.
     * Arguments:
     *     const Instruction &inst - Instruction to exectute.
     *     Context &ctx - Vertex context.
     *     uint32_t vertex_id - Vertex to operate on.
     */
    void exec_memory(const Instruction &inst, Context &ctx, uint32_t vertex_id);

    /* Dispatch functions for MEMORY operations. */
    void exec_loadv(const Instruction &inst, Context &ctx, uint32_t vertex_id);
    void exec_storev(const Instruction &inst, Context &ctx, uint32_t vertex_id);
    void exec_loade(const Instruction &inst, Context &ctx, uint32_t vertex_id);
    void exec_loadg(const Instruction &inst, Context &ctx, uint32_t vertex_id);

    /*
     * Execute a GRAPH operation.
     * Arguments:
     *     const Instruction &inst - Instruction to exectute.
     *     Context &ctx - Vertex context.
     *     uint32_t vertex_id - Vertex to operate on.
     *     int pc - Current program counter.
     * Returns:
     *     int - New program counter.
     */
    int exec_graph(const Instruction &inst, Context &ctx, uint32_t vertex_id, int pc);

    /* Dispatch functions for GRAPH operations. */
    int exec_iter_neighbors(const Instruction &inst, Context &ctx, uint32_t vertex_id, int pc);
    int exec_end_iter(const Instruction &inst, Context &ctx, uint32_t vertex_id, int pc);
    int exec_gather_sum(const Instruction &inst, Context &ctx, uint32_t vertex_id, int pc);
    int exec_gather_min(const Instruction &inst, Context &ctx, uint32_t vertex_id, int pc);
    int exec_gather_max(const Instruction &inst, Context &ctx, uint32_t vertex_id, int pc);
    int exec_gather_count(const Instruction &inst, Context &ctx, uint32_t vertex_id, int pc);
    int exec_scatter(const Instruction &inst, Context &ctx, uint32_t vertex_id, int pc);
    int exec_scatter_if(const Instruction &inst, Context &ctx, uint32_t vertex_id, int pc);

    /*
     * Execute a SYSTEM operation.
     * Arguments:
     *     const Instruction &inst - Instruction to exectute.
     */
    void exec_system(const Instruction &inst);

    /* Dispatch functions for SYSTEM operations. */
    void exec_vote_change(const Instruction &inst);

    /*
     * Load a new Context based on the vertex.
     * Arguments:
     *     Context &ctx - Load this context.
     *     uint32_t vertex_id - Load context for this vertex.
     */
    void load_context(Context &ctx, uint32_t vertex_id);

    /*
     * Store a used Context into the vertex shared memory.
     * Arguments:
     *     const Context &ctx - Store this context.
     *     uint32_t vertex_id - Store context for this vertex.
     */
    void store_context(const Context &ctx, uint32_t vertex_id);

    /*
     * Update the active vertex list.
     * Returns:
     *     int - Number of updates applied. 
     */
    int update_active_vertices();

    /*
     * Check for the convergence of the kernel (count VOTE_CHANGE signals).
     * Returns:
     *     bool - true if the kernel has converged, false otherwise.
     */
    bool check_convergence();

    /*
     * Execute the kernel on a vertex given its context and vertex ID.
     * Arguments:
     *     Context &ctx - Execute with this context (register values).
     *     uint32_t vertex_id - Execute on this vertex.
     */
    void execute_vertex(Context &ctx, uint32_t vertex_id);

    /*
     * Execute a single instruction on a vertex given its context and vertex ID.
     * Arguments:
     *     Context &ctx - Execute with this context (register values).
     *     uint32_t vertex_id - Execute on this vertex.
     */
    void execute_instruction_vertex(Context &ctx, uint32_t vertex_id);

    /*
     * Check if debugging is finished.
     */
    void check_finished();

    /*
     * Snapshot of current state of the debugger.
     * Arguments:
     *     Context *ctx - Current context.
     * Returns:
     *     DebugInfo - Current state.
     */
    DebugInfo make_debug_info(Context *ctx);

    /*
     * Compile parts of the graph kernel.
     */
    void compile();

public:
    /* Default constructor and destructor. */
    GCVM() : updates(0), jit_compiled(nullptr) {}
    ~GCVM() = default;

    /*
     * Load a graph into the graph runtime.
     * Arguments:
     *     const Graph &graph - Load this graph.
     */
    void load_graph(const Graph &graph);

    /*
     * Load a program into the graph runtime.
     * Arguments:
     *     const Program &program - Load this program.
     */
    void load_program(const Program &program);

    /*
     * Set seed vertices to be initially active.
     * Arguments:
     *     uint32_t v - Vertex to set.
     *     bool active - Active or not.
     */
    void set_active(uint32_t v, bool active);

    /*
     * Set the initial value of v_self.
     * Arguments:
     *     uint32_t v - Vertex to set.
     *     double value - The value to seed the vertex with.
     */
    void set_seed_self(uint32_t v, double value);

    /*
     * Run the program on the VM.
     * Arguments:
     *     bool should_compile - Should we JIT parts of the kernel?
     */
    void run(bool should_compile);

    /*
     * Set a break point in the kernel.
     * Arguments:
     *     int pc - Break at this PC (must be positive).
     */
    void debug_add_breakpoint(int pc);

    /*
     * Take a single step through the program.
     * Returns:
     *     DebugInfo - Current program state.
     */
    DebugInfo debug_step();

    /*
     * Continue through the program until the next break point.
     * Returns:
     *     DebugInfo - Current program state.
     */
    DebugInfo debug_continue();

    /*
     * Report the results of running the kernel.
     * Returns:
     *     const std::vector<double> & - Copy of the results calculated.
     */
    const std::vector<double> &get_results() const;
};