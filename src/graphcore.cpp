#include <assert.h>
#include <string.h>
#include <asmjit/asmjit.h>

#include "graphcore.h"

using namespace asmjit;

/*
 * Load a graph into the graph runtime.
 * Arguments:
 *     const Graph &graph - Load this graph.
 */
void GCVM::load_graph(const Graph &graph) {
    this->graph = graph;
    vertices.active = std::vector<bool>(graph.size(), false);
    vertices.next_active = std::vector<bool>(graph.size(), false);
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
}

/*
 * Set seed vertices to be initially active.
 * Arguments:
 *     std::initializer_list<uint32_t> vertices - Initialize these vertices.
 */
void GCVM::set_seed_vertices(std::initializer_list<uint32_t> vertices) {
    for(uint32_t v : vertices) {
        assert(v < this->vertices.active.size());
        this->vertices.active[v] = true;
    }
}

/*
 * Set seed vertices to be initially active.
 * Arguments:
 *     bool all - Initialize all vertices if true.
 */
void GCVM::set_seed_vertices(bool all) {
    if(all) {
        for(int v = 0; v < graph.size(); v++)
            vertices.active[v] = true;
    }
}

/*
 * Set the initial value of v_self.
 * Arguments:
 *     double value - The value to seed the graph with.
 */
void GCVM::set_seed_self(double value) {
    for(int v = 0; v < graph.size(); v++) {
        vertices.v_self[v] = value;
    }
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
    ctx.regs[inst.rd()] = inst.imm();    
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
        case 4: {
            assert(inst.rd() < R_COUNT);
            ctx.regs[inst.rd()] = graph.size();
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
    double min = __DBL_MAX__;
    for(auto e = graph.incoming_row_offsets[vertex_id]; e < graph.incoming_row_offsets[vertex_id + 1]; ++e) {
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
    double max = __DBL_MIN__;
    for(auto e = graph.incoming_row_offsets[vertex_id]; e < graph.incoming_row_offsets[vertex_id + 1]; ++e) {
        uint32_t n = graph.col_indices[e];
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
        vertices.next_active[n] = true;
    }

    return pc;
}

int GCVM::exec_scatter_if(const Instruction &inst, Context &ctx, uint32_t vertex_id, int pc) {
    // If RS1 is non-zero, set the next_active of all neighbors to true
    if(ctx.regs[inst.rs1()]) {
        for(auto e = graph.row_offsets[vertex_id]; e < graph.row_offsets[vertex_id + 1]; ++e) {
            uint32_t n = graph.col_indices[e];
            vertices.next_active[n] = true;
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
        vertices.next_active[v] = false;
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
 * Pass over the bytecode and convert it to lowered IR.
 */
void GCVM::lower() {
    std::vector<LInstruction> lowered;
    lowered.reserve(program.code.size());

    /* Loop through the program and convert to lowered instructions */
    for(int pc = 0; pc < program.code.size(); pc++) {
        const Instruction &inst = program.code[pc];

        LInstruction out{};

        switch (inst.opcode()) {
            /* CONTROL */
            case OPCODE_CONTROL: {
                switch(inst.subop()) {
                    case SUBOP_NOP:
                        // skip
                        continue;

                    case SUBOP_HALT:
                        out.op = LOp::RETURN;
                        break;

                    case SUBOP_JMP:
                        out.op = LOp::JMP;
                        out.target = pc + inst.imm();
                        break;

                    case SUBOP_JZ:
                        out.op = LOp::JZ;
                        out.src1 = inst.rs1();
                        out.target = pc + inst.imm();
                        break;

                    case SUBOP_JNZ:
                        out.op = LOp::JNZ;
                        out.src1 = inst.rs1();
                        out.target = pc + inst.imm();
                        break;
                }
                break;
            }

            /* ALU */
            case OPCODE_ALU: {
                out.dst = inst.rd();
                out.src1 = inst.rs1();
                out.src2 = inst.rs2();

                switch(inst.subop()) {
                    case SUBOP_ADD: out.op = LOp::ADD; break;
                    case SUBOP_SUB: out.op = LOp::SUB; break;
                    case SUBOP_MUL: out.op = LOp::MUL; break;
                    case SUBOP_DIV: out.op = LOp::DIV; break;
                    case SUBOP_MIN: out.op = LOp::MIN; break;
                    case SUBOP_MAX: out.op = LOp::MAX; break;
                    case SUBOP_ABS: out.op = LOp::ABS; break;
                    case SUBOP_MOV: out.op = LOp::MOV; break;

                    case SUBOP_LOADI:
                        out.op = LOp::LOADI;
                        out.imm = inst.imm();
                        break;

                    case SUBOP_CMPLT: out.op = LOp::CMPLT; break;
                    case SUBOP_CMPLTE: out.op = LOp::CMPLTE; break;
                    case SUBOP_CMPEQ: out.op = LOp::CMPEQ; break;
                    case SUBOP_CMPNEQ: out.op = LOp::CMPNEQ; break;
                }
                break;
            }
            
            /* MEMORY */
            case OPCODE_MEMORY: {
                switch(inst.subop()) {
                    case SUBOP_LOADV: {
                        out.dst = inst.rd();

                        switch(inst.imm()) {
                            case 0: out.op = LOp::LOAD_V_SELF; break;
                            case 1: out.op = LOp::LOAD_N_VAL; break;
                            case 2: out.op = LOp::LOAD_IN_DEG; break;
                            case 3: out.op = LOp::LOAD_OUT_DEG; break;
                            case 4: out.op = LOp::LOAD_GRAPH_SIZE; break;
                        }
                        break;
                    }

                    case SUBOP_STOREV: {
                        out.op = LOp::STORE_V_OUT;
                        out.src1 = inst.rs1();
                        break;
                    }

                    case SUBOP_LOADE: {
                        out.op = LOp::LOAD_E_ATTR;
                        out.dst = inst.rd();
                        break;
                    }
                }
                break;
            }

            /* GRAPH */
            case OPCODE_GRAPH: {
                switch (inst.subop()) {
                    case SUBOP_ITER_NEIGHBORS:
                        out.op = LOp::ITER_BEGIN;
                        out.target = pc + inst.imm();
                        break;

                    case SUBOP_END_ITER:
                        out.op = LOp::ITER_NEXT;
                        out.target = pc + inst.imm();
                        break;

                    case SUBOP_GATHER_SUM:
                        out.op = LOp::GATHER_SUM;
                        out.dst = inst.rd();
                        break;

                    case SUBOP_GATHER_MIN:
                        out.op = LOp::GATHER_MIN;
                        out.dst = inst.rd();
                        break;

                    case SUBOP_GATHER_MAX:
                        out.op = LOp::GATHER_MAX;
                        out.dst = inst.rd();
                        break;

                    case SUBOP_GATHER_COUNT:
                        out.op = LOp::GATHER_COUNT;
                        out.dst = inst.rd();
                        break;

                    case SUBOP_SCATTER:
                        out.op = LOp::SCATTER;
                        break;

                    case SUBOP_SCATTER_IF:
                        out.op = LOp::SCATTER_IF;
                        out.src1 = inst.rs1();
                        break;
                }
                break;
            }

            /* SYSTEM */
            case OPCODE_SYSTEM: {
                switch(inst.subop()) {
                    case SUBOP_VOTE_CHANGE:
                        out.op = LOp::VOTE_CHANGE;
                        break;
                }
                break;
            }
        }

        lowered.push_back(out);
    }

    lir = lowered;
}

/*
 * Print the lowered IR of the program.
 */
void GCVM::dump_lir() {
    for(int i = 0; i < lir.size(); i++) {
        printf("%d: op=%d dst=%d src1=%d src2=%d target=%d\n",
            i,
            (int)lir[i].op,
            lir[i].dst,
            lir[i].src1,
            lir[i].src2,
            lir[i].target
        );
    }
}

/*
 * Load/Store a VM register.
 * Arguments:
 *     const x86::Gp& ctx - Pointer to context object.
 *     int reg_index - Index of register to load/store
 * Returns:
 *     x86::Mem - Pointer to the register.
 */
static x86::Mem reg_mem(const x86::Gp& ctx, int reg_index) {
    return x86::ptr(ctx, offsetof(Context, regs) + reg_index * sizeof(double));
}

static void load_imm(x86::Assembler& a, x86::Vec dst, double val) {
    uint64_t bits;
    mempcpy(&bits, &val, sizeof(bits));

    a.mov(x86::rax, bits);
    a.movq(dst, x86::rax);
}

/*
 * Compile parts of the graph kernel.
 * Returns:
 *     JITFunc - JIT compiled kernel.
 */
JITFunc GCVM::jit_compile() {
    // Lower the program if it hasn't been done yet
    if(lir.size() == 0) lower();
    if(lir.size() == 0) return;

    /* Set up */
    JitRuntime rt;
    CodeHolder code;
    code.init(rt.environment());

    x86::Assembler a(&code);

    /* rdi = Context * */
    /* rsi = vertex_id */
    /* rdx = GCVM * */
    x86::Gp ctx = x86::rdi;
    x86::Gp vid = x86::rsi;
    x86::Gp vm = x86::rdx;

    /* Set up labels */
    std::vector<Label> labels;
    for(size_t i = 0; i < lir.size(); i++) {
        labels.push_back(a.new_label());
    }

    // Jump to first line of asm
    a.jmp(labels[0]);

    /* Emit instructions */
    for(size_t pc = 0; pc < lir.size(); pc++) {
        a.bind(labels[pc]);
        const auto &inst = lir[pc];

        switch(inst.op) {
            case LOp::ADD: {
                // Map context registers to computer registers
                a.movsd(x86::xmm0, reg_mem(ctx, inst.src1));
                a.movsd(x86::xmm1, reg_mem(ctx, inst.src2));

                // Instruction
                a.addsd(x86::xmm0, x86::xmm1);

                // Write back to context destination
                a.movsd(reg_mem(ctx, inst.dst), x86::xmm0);
                a.jmp(labels[pc + 1]);
                break;
            }
            case LOp::SUB: {
                // Map context registers to computer registers
                a.movsd(x86::xmm0, reg_mem(ctx, inst.src1));
                a.movsd(x86::xmm1, reg_mem(ctx, inst.src2));

                // Instruction
                a.subsd(x86::xmm0, x86::xmm1);

                // Write back to context destination
                a.movsd(reg_mem(ctx, inst.dst), x86::xmm0);
                a.jmp(labels[pc + 1]);
                break;
            }
            case LOp::MUL: {
                // Map context registers to computer registers
                a.movsd(x86::xmm0, reg_mem(ctx, inst.src1));
                a.movsd(x86::xmm1, reg_mem(ctx, inst.src2));

                // Instruction
                a.mulsd(x86::xmm0, x86::xmm1);

                // Write back to context destination
                a.movsd(reg_mem(ctx, inst.dst), x86::xmm0);
                a.jmp(labels[pc + 1]);
                break;
            }
            case LOp::DIV: {
                // Map context registers to computer registers
                a.movsd(x86::xmm0, reg_mem(ctx, inst.src1));
                a.movsd(x86::xmm1, reg_mem(ctx, inst.src2));

                // Instruction
                a.divsd(x86::xmm0, x86::xmm1);

                // Write back to context destination
                a.movsd(reg_mem(ctx, inst.dst), x86::xmm0);
                a.jmp(labels[pc + 1]);
                break;
            }
            case LOp::LOADI: {
                // Load immediate into computer register and move to context destination
                load_imm(a, x86::xmm0, inst.imm);
                a.movsd(reg_mem(ctx, inst.dst), x86::xmm0);
                a.jmp(labels[pc + 1]);
                break;
            }
            case LOp::MOV: {
                // Move context register into computer register then write back to context destination
                a.movsd(x86::xmm0, reg_mem(ctx, inst.src1));
                a.movsd(reg_mem(ctx, inst.dst), x86::xmm0);
                a.jmp(labels[pc + 1]);
                break;
            }
            case LOp::JMP: {
                a.jmp(labels[inst.target]);
                break;
            }
            case LOp::JZ: {
                // Move context register to computer register
                a.movsd(x86::xmm0, reg_mem(ctx, inst.src1));
                a.xorpd(x86::xmm1, x86::xmm1); // Set xmm1 = 0

                // Check if xmm0 == xmm1 (0)
                a.ucomisd(x86::xmm0, x86::xmm1);
                a.je(labels[inst.target]);

                a.jmp(labels[pc + 1]);
                break;
            }
            case LOp::JNZ: {
                // Move context register to computer register
                a.movsd(x86::xmm0, reg_mem(ctx, inst.src1));
                a.xorpd(x86::xmm1, x86::xmm1);  // Set xmm1 = 0

                // Check if xmm0 != xmm1 (0)
                a.ucomisd(x86::xmm0, x86::xmm1);
                a.jne(labels[inst.target]);

                a.jmp(labels[pc + 1]);
                break;
            }
            case LOp::LOAD_V_SELF: {
                // Load the value stored at ctx->v_self
                a.movsd(x86::xmm0, x86::ptr(ctx, offsetof(Context, v_self)));
                a.movsd(reg_mem(ctx, inst.dst), x86::xmm0);
                a.jmp(labels[pc + 1]);
                break;
            }
            case LOp::LOAD_N_VAL: {
                // Load the value stored at ctx->n_val
                a.movsd(x86::xmm0, x86::ptr(ctx, offsetof(Context, n_val)));
                a.movsd(reg_mem(ctx, inst.dst), x86::xmm0);
                a.jmp(labels[pc + 1]);
                break;
            }
            case LOp::LOAD_IN_DEG: {
                // Load the value stored at ctx->in_deg
                a.movsd(x86::xmm0, x86::ptr(ctx, offsetof(Context, in_deg)));
                a.movsd(reg_mem(ctx, inst.dst), x86::xmm0);
                a.jmp(labels[pc + 1]);
                break;
            }
            case LOp::LOAD_OUT_DEG: {
                // Load the value stored at ctx->out_deg
                a.movsd(x86::xmm0, x86::ptr(ctx, offsetof(Context, out_deg)));
                a.movsd(reg_mem(ctx, inst.dst), x86::xmm0);
                a.jmp(labels[pc + 1]);
                break;
            }
            case LOp::LOAD_GRAPH_SIZE: {
                // Load the value stored at ctx->g_size
                a.movsd(x86::xmm0, x86::ptr(ctx, offsetof(Context, g_size)));
                a.movsd(reg_mem(ctx, inst.dst), x86::xmm0);
                a.jmp(labels[pc + 1]);
                break;
            }
            case LOp::STORE_V_OUT: {
                // Store context register value in the memory location ctx->v_out
                a.movsd(x86::xmm0, reg_mem(ctx, inst.src1));
                a.movsd(x86::ptr(ctx, offsetof(Context, v_out)), x86::xmm0);
                a.jmp(labels[pc + 1]);
                break;
            }
            case LOp::LOAD_E_ATTR: {
                // Load the value stored at ctx->e_attr
                a.movsd(x86::xmm0, x86::ptr(ctx, offsetof(Context, e_attr)));
                a.movsd(reg_mem(ctx, inst.dst), x86::xmm0);
                a.jmp(labels[pc + 1]);
                break;
            }
            case LOp::RETURN: {
                a.ret();
                break;
            }
        }
    }

    JITFunc fn;
    rt.add(&fn, &code);
    return fn;
}

/*
 * Run the program on the VM.
 * Arguments:
 *     bool compile - Should we JIT parts of the kernel?
 */
void GCVM::run(bool compile) {
    bool converged = false;

    while(!converged) {
        if(compile && !jit_compiled) {
            jit_compiled = jit_compile();
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
                    jit_compiled(&ctx, v, this);
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