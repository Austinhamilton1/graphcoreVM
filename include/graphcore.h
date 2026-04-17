#pragma once

#include <vector>
#include <cstdint>
#include <initializer_list>
#include <atomic>

#include "graph.h"
#include "program.h"
#include "lowered_ir.h"

#define R_COUNT 32


/*
 * Vertex state held across execution runs.
 * VertexState[v] = state of vertex v.
 */
struct VertexState {
    std::vector<double> v_self;     // Current (read-only) value of node
    std::vector<double> v_out;      // Next (write-only) value of node
    std::vector<bool> active;       // Current active state of node
    std::vector<bool> next_active;  // Next active state of node
};

/* 
 * Transient state that wraps each node for each superstep.
 * This is a stack-allocated data structure. It's purpose is
 * to ensure global state is managed by the runtime and not
 * touched by the kernel directly.
 */
struct Context {
    double regs[32];    // General purpose registers

    double v_self;      // Read-only current value of node
    double v_out;       // Write-only next value of node

    double n_val;       // Iterator (next neighbor node ID)
    size_t in_deg;      // Node in-degree
    size_t out_deg;     // Node out-degree
    size_t g_size;      // Graph size
    double e_attr;      // Iterator (next edge attribute)

    uint32_t iter_edge; // Current edge index
    uint32_t iter_end;  // End of adjacency list
};

class GCVM;
using JITFunc = void(*)(Context *, uint32_t, GCVM *);

/* 
 * Graph Runtime. This class manages the runtime.
 * It acts as an execution system for GCVM bytecode.
 */
class GCVM {
private:
    Graph graph;                    // CSR format graph
    Program program;                // Bytecode program
    VertexState vertices;           // SoA of vertex states
    std::atomic<size_t> updates;    // Number of vertices affected by kernel
    std::vector<LInstruction> lir;  // Lowered IR representation for JIT compilation
    JITFunc jit_compiled;

    /* Allow JIT functions access to GCVM internals. */
    friend bool gcvm_iter_begin(GCVM *vm, Context *ctx, uint32_t vertex);
    friend bool gcvm_iter_next(GCVM *vm, Context *ctx, uint32_t vertex);
    friend double gcvm_gather_sum(GCVM *vm, uint32_t vertex);
    friend double gcvm_gather_min(GCVM *vm, uint32_t vertex);
    friend double gcvm_gather_max(GCVM *vm, uint32_t vertex);
    friend double gcvm_gather_count(GCVM *vm, uint32_t vertex);
    friend void gcvm_scatter(GCVM *vm, uint32_t vertex);
    friend void gcvm_vote_change(GCVM *vm);

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
     * Compile parts of the graph kernel.
     * Returns:
     *     JITFunc - JIT compiled kernel.
     */
    JITFunc jit_compile();

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
     *     std::initializer_list<uint32_t> vertices - Initialize these vertices.
     */
    void set_seed_vertices(std::initializer_list<uint32_t> vertices);

    /*
     * Set seed vertices to be initially active.
     * Arguments:
     *     bool all - Initialize all vertices if true.
     */
    void set_seed_vertices(bool all);

    /*
     * Set the initial value of v_self.
     * Arguments:
     *     double value - The value to seed the graph with.
     */
    void set_seed_self(double value);

    /*
     * Pass over the bytecode and convert it to lowered IR.
     */
    void lower();

    /*
     * Print the lowered IR of the program.
     */
    void dump_lir();

    /*
     * Run the program on the VM.
     * Arguments:
     *     bool compile - Should we JIT parts of the kernel?
     */
    void run(bool compile);

    /*
     * Report the results of running the kernel.
     * Returns:
     *     const std::vector<double> & - Copy of the results calculated.
     */
    const std::vector<double> &get_results() const;
};