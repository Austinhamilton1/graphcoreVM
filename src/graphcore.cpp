#include <cassert>
#include <cstring>

#include "graphcore.h"

/*
 * Load a graph into the graph runtime.
 * Arguments:
 *     const Graph &graph - Load this graph.
 */
void GCVM::load_graph(const Graph &graph) {
    this->graph = graph;
    vertices.active = std::vector<uint8_t>(graph.size(), 0);
    vertices.next_active = std::vector<uint8_t>(graph.size(), 0);
    vertices.v_self = std::vector<double>(graph.size());
    vertices.v_out = std::vector<double>(graph.size());
}

/*
 * Load a program into the graph runtime.
 * Arguments:
 *     const Program &program - Load this program.
 */
void GCVM::load_program(const Program &program) {
    this->program = program;
    if(jit_compiled) jit_compiled = nullptr;
}

/*
 * Set seed vertices to be initially active.
 * Arguments:
 *     uint32_t v - Vertex to set.
 *     bool active - Active or not.
 */
void GCVM::set_active(uint32_t v, bool active) {
    assert(v < vertices.active.size());
    vertices.active[v] = active ? 1 : 0;
}

/*
 * Set the initial value of v_self.
 * Arguments:
 *     uint32_t v - Vertex to set.
 *     double value - The value to seed the vertex with.
 */
void GCVM::set_seed_self(uint32_t v, double value) {
    assert(v < vertices.v_self.size());
    vertices.v_self[v] = value; 
}

/*
 * Execute a CONTROL operation.
 * Arguments:
 *     const Instruction &inst - Instruction to execute.
 *     Context &ctx - Vertex context.
 *     int pc - Current program counter.
 * Returns:
 *     int - New program counter. 
 */
int GCVM::exec_control(const Instruction &inst, Context &ctx, int pc) {
    // Dispatch CONTROL operations
    switch(inst.subop()) {
        case SUBOP_NOP: {
            return exec_nop(inst, ctx, pc);
        }
        case SUBOP_HALT: {
            return exec_halt(inst, ctx, pc);
        }
        case SUBOP_JMP: {
            return exec_jmp(inst, ctx, pc);
        }
        case SUBOP_JZ: {
            assert(inst.rs1() < R_COUNT);
            return exec_jz(inst, ctx, pc);
        }
        case SUBOP_JNZ: {
            assert(inst.rs1() < R_COUNT);
            return exec_jnz(inst, ctx, pc);
        }
    }

    return pc;
}

/* Dispatch functions for CONTROL operations. */
int GCVM::exec_nop(const Instruction &inst, Context &ctx, int pc) {
    return pc;
}

int GCVM::exec_halt(const Instruction &inst, Context &ctx, int pc) {
    return -1;
}

int GCVM::exec_jmp(const Instruction &inst, Context &ctx, int pc) {
    return pc + inst.imm();
}

int GCVM::exec_jz(const Instruction &inst, Context &ctx, int pc) {
    return ctx.regs[inst.rs1()] == 0 ? pc + inst.imm() : pc;
}

int GCVM::exec_jnz(const Instruction &inst, Context &ctx, int pc) {
    return ctx.regs[inst.rs1()] != 0 ? pc + inst.imm() : pc;
}

/*
 * Execute an ALU operation.
 * Arguments:
 *     const Instruction &inst - Instruction to exectute.
 *     Context &ctx - Vertex context.
 */
void GCVM::exec_alu(const Instruction &inst, Context &ctx) {
    // Dispatch ALU operations
    switch(inst.subop()) {
        case SUBOP_ADD: {
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            exec_add(inst, ctx);
            break;
        }
        case SUBOP_SUB: {
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            exec_sub(inst, ctx);
            break;
        }
        case SUBOP_MUL: {
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            exec_mul(inst, ctx);
            break;
        }
        case SUBOP_DIV: {
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            exec_div(inst, ctx);
            break;
        }
        case SUBOP_MIN: {
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            exec_min(inst, ctx);
            break;
        }
        case SUBOP_MAX: {
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            exec_max(inst, ctx);
            break;
        }
        case SUBOP_ABS: {
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT);
            exec_abs(inst, ctx);
            break;
        }
        case SUBOP_MOV: {
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT);
            exec_mov(inst, ctx);
            break;
        }
        case SUBOP_LOADI: {
            assert(inst.rd() < R_COUNT);
            exec_loadi(inst, ctx);
            break;
        }
        case SUBOP_CMPLT: {
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            exec_cmp_lt(inst, ctx);
            break;
        }
        case SUBOP_CMPLTE: {
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            exec_cmp_lte(inst, ctx);
            break;
        }
        case SUBOP_CMPEQ: {
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            exec_cmp_eq(inst, ctx);
            break;
        }
        case SUBOP_CMPNEQ: {
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            exec_cmp_neq(inst, ctx);
            break;
        }
    }
}

/* Dispatch functions for ALU operations. */
void GCVM::exec_add(const Instruction &inst, Context &ctx) {
    ctx.regs[inst.rd()] = ctx.regs[inst.rs1()] + ctx.regs[inst.rs2()];        
}

void GCVM::exec_sub(const Instruction &inst, Context &ctx) {
    ctx.regs[inst.rd()] = ctx.regs[inst.rs1()] - ctx.regs[inst.rs2()];        
}

void GCVM::exec_mul(const Instruction &inst, Context &ctx) {
    ctx.regs[inst.rd()] = ctx.regs[inst.rs1()] * ctx.regs[inst.rs2()];          
}

void GCVM::exec_div(const Instruction &inst, Context &ctx) {
    ctx.regs[inst.rd()] = ctx.regs[inst.rs1()] / ctx.regs[inst.rs2()];     
}

void GCVM::exec_min(const Instruction &inst, Context &ctx) {
    ctx.regs[inst.rd()] = (ctx.regs[inst.rs1()] < ctx.regs[inst.rs2()]) ? ctx.regs[inst.rs1()] : ctx.regs[inst.rs2()];     
}

void GCVM::exec_max(const Instruction &inst, Context &ctx) {
    ctx.regs[inst.rd()] = (ctx.regs[inst.rs1()] > ctx.regs[inst.rs2()]) ? ctx.regs[inst.rs1()] : ctx.regs[inst.rs2()];
}

void GCVM::exec_abs(const Instruction &inst, Context &ctx) {
    ctx.regs[inst.rd()] = (ctx.regs[inst.rs1()] < 0) ? -ctx.regs[inst.rs1()] : ctx.regs[inst.rs1()];
}

void GCVM::exec_mov(const Instruction &inst, Context &ctx) {
    ctx.regs[inst.rd()] = ctx.regs[inst.rs1()];
}

void GCVM::exec_loadi(const Instruction &inst, Context &ctx) {
    ctx.regs[inst.rd()] = program.constants[inst.imm()];  
}

void GCVM::exec_cmp_lt(const Instruction &inst, Context &ctx) {
    ctx.regs[inst.rd()] = (ctx.regs[inst.rs1()] < ctx.regs[inst.rs2()]) ? 1 : 0;     
}

void GCVM::exec_cmp_lte(const Instruction &inst, Context &ctx) {
    ctx.regs[inst.rd()] = (ctx.regs[inst.rs1()] <= ctx.regs[inst.rs2()]) ? 1 : 0;
}

void GCVM::exec_cmp_eq(const Instruction &inst, Context &ctx) {
    ctx.regs[inst.rd()] = (ctx.regs[inst.rs1()] == ctx.regs[inst.rs2()]) ? 1 : 0;
}

void GCVM::exec_cmp_neq(const Instruction &inst, Context &ctx) {
    ctx.regs[inst.rd()] = (ctx.regs[inst.rs1()] != ctx.regs[inst.rs2()]) ? 1 : 0;
}



/*
 * Execute a MEMORY operation.
 * Arguments:
 *     const Instruction &inst - Instruction to exectute.
 *     Context &ctx - Vertex context.
 *     uint32_t vertex_id - Vertex to operate on.
 */
void GCVM::exec_memory(const Instruction &inst, Context &ctx, uint32_t vertex_id) {
    // Dispatch MEMORY operations
    switch(inst.subop()) {
        case SUBOP_LOADV: {
            exec_loadv(inst, ctx, vertex_id);
            break;
        }
        case SUBOP_STOREV: {
            exec_storev(inst, ctx, vertex_id);
            break;
        }
        case SUBOP_LOADE: {
            exec_loade(inst, ctx, vertex_id);
            break;
        }
        case SUBOP_LOADG: {
            exec_loadg(inst, ctx, vertex_id);
            break;
        }
    }
}

/* Dispatch functions for MEMORY operations. */
void GCVM::exec_loadv(const Instruction &inst, Context &ctx, uint32_t vertex_id) {
    switch(inst.imm()) {
        case 0: {
            assert(inst.rd() < R_COUNT);
            ctx.regs[inst.rd()] = ctx.v_self;
            break;
        }
        case 1: {
            assert(inst.rd() < R_COUNT);
            ctx.regs[inst.rd()] = ctx.n_val;
            break;
        }
        case 2: {
            assert(inst.rd() < R_COUNT);
            ctx.regs[inst.rd()] = graph.in_degree(vertex_id);
            break;
        }
        case 3: {
            assert(inst.rd() < R_COUNT);
            ctx.regs[inst.rd()] = graph.out_degree(vertex_id);
            break;
        }
        default:
            break;
    }
}

void GCVM::exec_storev(const Instruction &inst, Context &ctx, uint32_t vertex_id) {
    switch(inst.imm()) {
        case 0: {
            assert(inst.rs1() < R_COUNT);
            ctx.v_out = ctx.regs[inst.rs1()];
            break;
        }
        default:
            break;
    }
}

void GCVM::exec_loade(const Instruction &inst, Context &ctx, uint32_t vertex_id) {
    switch(inst.imm()) {
        case 0: {
            assert(inst.rd() < R_COUNT);
            ctx.regs[inst.rd()] = ctx.e_attr;
            break;
        }
        default:
            break;
    }
}

void GCVM::exec_loadg(const Instruction &inst, Context &ctx, uint32_t vertex_id) {
    assert(inst.rd() < R_COUNT);
    ctx.regs[inst.rd()] = ctx.g_size;
}

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
int GCVM::exec_graph(const Instruction &inst, Context &ctx, uint32_t vertex_id, int pc) {
    // Dispatch GRAPH operations
    switch(inst.subop()) {
        case SUBOP_ITER_NEIGHBORS: {
            assert(vertex_id < graph.incoming_row_offsets.size() - 1);
            return exec_iter_neighbors(inst, ctx, vertex_id, pc);
        }
        case SUBOP_END_ITER: {
            return exec_end_iter(inst, ctx, vertex_id, pc);
        }
        case SUBOP_GATHER_SUM: {
            assert(inst.rd() < R_COUNT && vertex_id < graph.incoming_row_offsets.size() - 1);
            return exec_gather_sum(inst, ctx, vertex_id, pc);
        }
        case SUBOP_GATHER_MIN: {
            assert(inst.rd() < R_COUNT && vertex_id < graph.incoming_row_offsets.size() - 1);
            return exec_gather_min(inst, ctx, vertex_id, pc);
        }
        case SUBOP_GATHER_MAX: {
            assert(inst.rd() < R_COUNT && vertex_id < graph.incoming_row_offsets.size() - 1);
            return exec_gather_max(inst, ctx, vertex_id, pc);
        }
        case SUBOP_GATHER_COUNT: {
            assert(inst.rd() < R_COUNT && vertex_id < graph.incoming_row_offsets.size() - 1);
            return exec_gather_count(inst, ctx, vertex_id, pc);
        }
        case SUBOP_SCATTER: {
            assert(vertex_id < graph.row_offsets.size() - 1);
            return exec_scatter(inst, ctx, vertex_id, pc);
        }
        case SUBOP_SCATTER_IF: {
            assert(inst.rs1() < R_COUNT && vertex_id < graph.row_offsets.size() - 1);
            return exec_scatter_if(inst, ctx, vertex_id, pc);
        }
    }

    return pc;
}

/* Dispatch functions for GRAPH operations. */
int GCVM::exec_iter_neighbors(const Instruction &inst, Context &ctx, uint32_t vertex_id, int pc) {
    // Initialize iterators
    ctx.iter_edge = graph.incoming_row_offsets[vertex_id];
    ctx.iter_end = graph.incoming_row_offsets[vertex_id + 1];

    if(ctx.iter_edge >= ctx.iter_end) {
        // Skip loop body
        return pc + inst.imm(); // Jump to END_ITER
    }

    // Load first neighbor
    uint32_t e = ctx.iter_edge;
    ctx.n_val = vertices.v_self[graph.incoming_col_indices[e]];
    if(!graph.incoming_edge_attr.empty()) {
        ctx.e_attr = graph.incoming_edge_attr[e];
    }

    return pc;
}

int GCVM::exec_end_iter(const Instruction &inst, Context &ctx, uint32_t vertex_id, int pc) {
    ctx.iter_edge++;

    if(ctx.iter_edge < ctx.iter_end) {
        // Load next neighbor
        uint32_t e = ctx.iter_edge;
        ctx.n_val = vertices.v_self[graph.incoming_col_indices[e]];
        if(!graph.incoming_edge_attr.empty()) {
            ctx.e_attr = graph.incoming_edge_attr[e];
        }

        return pc + inst.imm(); // Jump back
    }

    return pc;
}

int GCVM::exec_gather_sum(const Instruction &inst, Context &ctx, uint32_t vertex_id, int pc) {
    // Calculate the sum of the neighbors' v_self
    double sum = 0.0;
    for(auto e = graph.incoming_row_offsets[vertex_id]; e < graph.incoming_row_offsets[vertex_id + 1]; ++e) {
        uint32_t n = graph.incoming_col_indices[e];
        sum += vertices.v_self[n];
    }

    // Store sum in RD
    ctx.regs[inst.rd()] = sum;

    return pc;
}

int GCVM::exec_gather_min(const Instruction &inst, Context &ctx, uint32_t vertex_id, int pc) {
    // Calculate the min of the neighbors' v_self
    auto start = graph.incoming_row_offsets[vertex_id];
    auto end = graph.incoming_row_offsets[vertex_id + 1];
    if(start >= end) {
        ctx.regs[inst.rd()] = 0.0;
        return pc;
    }

    double min = vertices.v_self[graph.incoming_col_indices[start]];
    for(auto e = start + 1; e < end; ++e) {
        uint32_t n = graph.incoming_col_indices[e];
        if(vertices.v_self[n] < min)
            min = vertices.v_self[n];
    }

    // Store min in RD
    ctx.regs[inst.rd()] = min;

    return pc;
}

int GCVM::exec_gather_max(const Instruction &inst, Context &ctx, uint32_t vertex_id, int pc) {
    // Calculate the max of the neighbors' v_self
    auto start = graph.incoming_row_offsets[vertex_id];
    auto end = graph.incoming_row_offsets[vertex_id + 1];
    if(start >= end) {
        ctx.regs[inst.rd()] = 0.0;
        return pc;
    }

    double max = vertices.v_self[graph.incoming_col_indices[start]];
    for(auto e = start + 1; e < end; ++e) {
        uint32_t n = graph.incoming_col_indices[e];
        if(vertices.v_self[n] > max)
            max = vertices.v_self[n];
    }

    // Store max in RD
    ctx.regs[inst.rd()] = max;

    return pc;
}

int GCVM::exec_gather_count(const Instruction &inst, Context &ctx, uint32_t vertex_id, int pc) {
    // Count the number of neighbors
    double count = 0.0;
    for(auto e = graph.incoming_row_offsets[vertex_id]; e < graph.incoming_row_offsets[vertex_id + 1]; ++e) {
        count += 1;
    }

    // Store the count in RD
    ctx.regs[inst.rd()] = count;

    return pc;
}

int GCVM::exec_scatter(const Instruction &inst, Context &ctx, uint32_t vertex_id, int pc) {
    // Set the next_active of all neighbors to true
    for(auto e = graph.row_offsets[vertex_id]; e < graph.row_offsets[vertex_id + 1]; ++e) {
        uint32_t n = graph.col_indices[e];
        vertices.next_active[n] = 1;
    }

    return pc;
}

int GCVM::exec_scatter_if(const Instruction &inst, Context &ctx, uint32_t vertex_id, int pc) {
    // If RS1 is non-zero, set the next_active of all neighbors to true
    if(ctx.regs[inst.rs1()]) {
        for(auto e = graph.row_offsets[vertex_id]; e < graph.row_offsets[vertex_id + 1]; ++e) {
            uint32_t n = graph.col_indices[e];
            vertices.next_active[n] = 1;
        }
    }

    return pc;
}


/*
 * Execute a SYSTEM operation.
 * Arguments:
 *     const Instruction &inst - Instruction to exectute.
 */
void GCVM::exec_system(const Instruction &inst) {
    // Dispatch SYSTEM operations
    switch(inst.subop()) {
        case SUBOP_VOTE_CHANGE: {
            exec_vote_change(inst);
            break;
        }
    }
}

/* Dispatch functions for SYSTEM operations. */
void GCVM::exec_vote_change(const Instruction &inst) {
    // Signal there was an update
    updates.fetch_add(1, std::memory_order_relaxed);
}

/*
 * Execute the kernel on a vertex given its context and vertex ID.
 * Arguments:
 *     Context &ctx - Execute with this context (register values).
 *     uint32_t vertex_id - Execute on this vertex.
 */
void GCVM::execute_vertex(Context &ctx, uint32_t vertex_id) {
    int pc = 0;

    // Run through all instructions in the program
    while(pc < program.code.size()) {
        const Instruction& inst = program.code[pc];

        // Dispatch the instructions 
        switch(inst.opcode()) {
            case OPCODE_CONTROL: {
                pc = exec_control(inst, ctx, pc);
                break;
            }
            case OPCODE_ALU: {
                exec_alu(inst, ctx);
                break;
            }
            case OPCODE_MEMORY: {
                exec_memory(inst, ctx, vertex_id);
                break;
            }
            case OPCODE_GRAPH: {
                pc = exec_graph(inst, ctx, vertex_id, pc);
                break;
            }
            case OPCODE_SYSTEM: {
                exec_system(inst);
                break;
            }
        }

        // Check for validity of the PC (HALT returns -1 for example)
        if(pc < 0) break;

        pc++;
    }
}

/*
 * Load a new Context based on the vertex.
 * Arguments:
 *     Context &ctx - Load this context.
 *     uint32_t vertex_id - Load context for this vertex.
 */
void GCVM::load_context(Context &ctx, uint32_t vertex_id) {
    assert(vertex_id < vertices.v_self.size() && vertex_id < vertices.v_out.size());
    
    // Copy from global memory
    ctx.v_self = vertices.v_self[vertex_id];
    ctx.v_out = vertices.v_out[vertex_id];
    ctx.in_deg = graph.in_degree(vertex_id);
    ctx.out_deg = graph.out_degree(vertex_id);
    ctx.g_size = graph.size();
    
    // Initialize as empty
    memset(ctx.regs, 0, sizeof(ctx.regs));
    ctx.n_val = 0.0;
    ctx.e_attr = 0.0;
    ctx.iter_edge = 0;
    ctx.iter_end = 0;
}

/*
 * Store a used Context into the vertex shared memory.
 * Arguments:
 *     const Context &ctx - Store this context.
 *     uint32_t vertex_id - Store context for this vertex.
 */
void GCVM::store_context(const Context &ctx, uint32_t vertex_id) {
    assert(vertex_id < vertices.v_self.size() && vertex_id < vertices.v_out.size());
    
    // Copy to global memory
    vertices.v_self[vertex_id] = ctx.v_self;
    vertices.v_out[vertex_id] = ctx.v_out;
}

/*
 * Update the active vertex list.
 * Returns:
 *     int - Number of updates applied. 
 */
int GCVM::update_active_vertices() {
    int active_count = 0;   // Keep track of how many vertices are still active

    // Swap active and next active buffers
    for(int v = 0; v < graph.size(); v++) {
        if(vertices.next_active[v]) active_count++;
        vertices.active[v] = vertices.next_active[v];
        vertices.next_active[v] = 0;
    }
    
    return active_count;
}

/*
 * Check for the convergence of the kernel (count VOTE_CHANGE signals).
 * Returns:
 *     bool - true if the kernel has converged, false otherwise.
 */
bool GCVM::check_convergence() {
    size_t update_count = updates.load();
    updates.store(0);
    return update_count == 0;
}

/*
 * Compile parts of the graph kernel.
 */
void GCVM::compile() {
    if(!jit.is_initialized())
        jit.init(graph, vertices, &updates);
    jit_compiled = jit.compile(program);
    if(jit_compiled == nullptr)
        throw std::runtime_error("JIT compiled code return nullptr");
}

/*
 * Run the program on the VM.
 * Arguments:
 *     bool should_compile - Should we JIT parts of the kernel?
 */
void GCVM::run(bool should_compile) {
    bool converged = false;

    while(!converged) {
        if(should_compile && !jit_compiled) {
            compile();
        }
        // 1. Iterate active vertices
        for(int v = 0; v < graph.size(); v++) {
            // Only iterate over active vertices
            if(vertices.active[v]) {
                // Create context
                Context ctx;
                load_context(ctx, v);

                // Execute the kernel for each active vertex
                if(jit_compiled) {
                    jit_compiled(&ctx, v, jit.get_runtime());
                } else {
                    execute_vertex(ctx, v);
                }

                // Store context
                store_context(ctx, v);
            } 
        }

        // 2. For now BARRIER is implicit (via threads joining)

        // 3. Apply phase (commit v_out -> v_self)
        for(int v = 0; v < graph.size(); v++) {
            vertices.v_self[v] = vertices.v_out[v];
        } 

        // 4. Update active set (from SCATTER)
        int updates = update_active_vertices();
        if(updates == 0) break;

        // 5. Convergence check
        converged = check_convergence();
    }
}

/*
 * Report the results of running the kernel.
 * Returns:
 *     const std::vector<double> & - Copy of the results calculated.
 */
const std::vector<double> &GCVM::get_results() const {
    return vertices.v_self;
}