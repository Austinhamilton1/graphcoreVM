#include <assert.h>
#include <omp.h>
#include <string.h>

#include "graphcore.h"

/*
 * Load a graph into the graph runtime.
 * Arguments:
 *     const Graph &graph - Load this graph.
 */
void GCVM::load_graph(const Graph &graph) {
    this->graph = graph;
    num_vertices = graph.size();
    vertices.active = std::vector<bool>(num_vertices, false);
    vertices.next_active = std::vector<bool>(num_vertices, false);
    vertices.v_self = std::vector<double>(num_vertices);
    vertices.v_out = std::vector<double>(num_vertices);
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
        for(int v = 0; v < num_vertices; v++)
            vertices.active[v] = true;
    }
}

/*
 * Set the initial value of v_self.
 * Arguments:
 *     double value - The value to seed the graph with.
 */
void GCVM::set_seed_self(double value) {
    for(int v = 0; v < num_vertices; v++) {
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
            return pc;
        }
        case SUBOP_HALT: {
            return -1;
        }
        case SUBOP_JMP: {
            return pc + inst.imm();
        }
        case SUBOP_JZ: {
            assert(inst.rs1() < R_COUNT);
            return ctx.regs[inst.rs1()] == 0 ? pc + inst.imm() : pc;
        }
        case SUBOP_JNZ: {
            assert(inst.rs1() < R_COUNT);
            return ctx.regs[inst.rs1()] != 0 ? pc + inst.imm() : pc;
        }
    }

    return pc;
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
            ctx.regs[inst.rd()] = ctx.regs[inst.rs1()] + ctx.regs[inst.rs2()];
            break;
        }
        case SUBOP_SUB: {
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            ctx.regs[inst.rd()] = ctx.regs[inst.rs1()] - ctx.regs[inst.rs2()];
            break;
        }
        case SUBOP_MUL: {
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            ctx.regs[inst.rd()] = ctx.regs[inst.rs1()] * ctx.regs[inst.rs2()];
            break;
        }
        case SUBOP_DIV: {
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            ctx.regs[inst.rd()] = ctx.regs[inst.rs1()] / ctx.regs[inst.rs2()];
            break;
        }
        case SUBOP_MIN: {
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            ctx.regs[inst.rd()] = (ctx.regs[inst.rs1()] < ctx.regs[inst.rs2()]) ? ctx.regs[inst.rs1()] : ctx.regs[inst.rs2()];
            break;
        }
        case SUBOP_MAX: {
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            ctx.regs[inst.rd()] = (ctx.regs[inst.rs1()] > ctx.regs[inst.rs2()]) ? ctx.regs[inst.rs1()] : ctx.regs[inst.rs2()];
            break;
        }
        case SUBOP_ABS: {
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT);
            ctx.regs[inst.rd()] = (ctx.regs[inst.rs1()] < 0) ? -ctx.regs[inst.rs1()] : ctx.regs[inst.rs1()];
            break;
        }
        case SUBOP_MOV: {
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT);
            ctx.regs[inst.rd()] = ctx.regs[inst.rs1()];
            break;
        }
        case SUBOP_LOADI: {
            assert(inst.rd() < R_COUNT);
            ctx.regs[inst.rd()] = inst.imm();
            break;
        }
        case SUBOP_CMPLT: {
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            ctx.regs[inst.rd()] = (ctx.regs[inst.rs1()] < ctx.regs[inst.rs2()]) ? 1 : 0;
            break;
        }
        case SUBOP_CMPLTE: {
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            ctx.regs[inst.rd()] = (ctx.regs[inst.rs1()] <= ctx.regs[inst.rs2()]) ? 1 : 0;
            break;
        }
        case SUBOP_CMPEQ: {
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            ctx.regs[inst.rd()] = (ctx.regs[inst.rs1()] == ctx.regs[inst.rs2()]) ? 1 : 0;
            break;
        }
        case SUBOP_CMPNEQ: {
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            ctx.regs[inst.rd()] = (ctx.regs[inst.rs1()] != ctx.regs[inst.rs2()]) ? 1 : 0;
            break;
        }
    }
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
            break;
        }
        case SUBOP_STOREV: {
            switch(inst.imm()) {
                case 0: {
                    assert(inst.rs1() < R_COUNT);
                    ctx.v_out = ctx.regs[inst.rs1()];
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case SUBOP_LOADE: {
            switch(inst.imm()) {
                case 0: {
                    assert(inst.rd() < R_COUNT);
                    ctx.regs[inst.rd()] = ctx.e_attr;
                    break;
                }
                default:
                    break;
            }
            break;
        }
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
            break;
        }
        case SUBOP_END_ITER: {
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
            break;
        }
        case SUBOP_GATHER_SUM: {
            assert(inst.rd() < R_COUNT && vertex_id < graph.incoming_row_offsets.size() - 1);

            // Calculate the sum of the neighbors' v_self
            double sum = 0.0;
            for(auto e = graph.incoming_row_offsets[vertex_id]; e < graph.incoming_row_offsets[vertex_id + 1]; ++e) {
                uint32_t n = graph.incoming_col_indices[e];
                sum += vertices.v_self[n];
            }

            // Store sum in RD
            ctx.regs[inst.rd()] = sum;
            break;
        }
        case SUBOP_GATHER_MIN: {
            assert(inst.rd() < R_COUNT && vertex_id < graph.incoming_row_offsets.size() - 1);

            // Calculate the min of the neighbors' v_self
            double min = __DBL_MAX__;
            for(auto e = graph.incoming_row_offsets[vertex_id]; e < graph.incoming_row_offsets[vertex_id + 1]; ++e) {
                uint32_t n = graph.incoming_col_indices[e];
                if(vertices.v_self[n] < min)
                    min = vertices.v_self[n];
            }

            // Store min in RD
            ctx.regs[inst.rd()] = min;
            break;
        }
        case SUBOP_GATHER_MAX: {
            assert(inst.rd() < R_COUNT && vertex_id < graph.incoming_row_offsets.size() - 1);

            // Calculate the max of the neighbors' v_self
            double max = __DBL_MIN__;
            for(auto e = graph.incoming_row_offsets[vertex_id]; e < graph.incoming_row_offsets[vertex_id + 1]; ++e) {
                uint32_t n = graph.col_indices[e];
                if(vertices.v_self[n] > max)
                    max = vertices.v_self[n];
            }

            // Store max in RD
            ctx.regs[inst.rd()] = max;
            break;
        }
        case SUBOP_GATHER_COUNT: {
            assert(inst.rd() < R_COUNT && vertex_id < graph.incoming_row_offsets.size() - 1);

            // Count the number of neighbors
            double count = 0.0;
            for(auto e = graph.incoming_row_offsets[vertex_id]; e < graph.incoming_row_offsets[vertex_id + 1]; ++e) {
                count += 1;
            }

            // Store the count in RD
            ctx.regs[inst.rd()] = count;
            break;
        }
        case SUBOP_SCATTER: {
            assert(vertex_id < graph.row_offsets.size() - 1);

            // Set the next_active of all neighbors to true
            for(auto e = graph.row_offsets[vertex_id]; e < graph.row_offsets[vertex_id + 1]; ++e) {
                uint32_t n = graph.col_indices[e];
                vertices.next_active[n] = true;
            }
            break;
        }
        case SUBOP_SCATTER_IF: {
            assert(inst.rs1() < R_COUNT && vertex_id < graph.row_offsets.size() - 1);

            // If RS1 is non-zero, set the next_active of all neighbors to true
            if(ctx.regs[inst.rs1()]) {
                for(auto e = graph.row_offsets[vertex_id]; e < graph.row_offsets[vertex_id + 1]; ++e) {
                    uint32_t n = graph.col_indices[e];
                    vertices.next_active[n] = true;
                }
            }
            break;
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
            // Signal there was an update
            updates.fetch_add(1, std::memory_order_relaxed);
            break;
        }
    }
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
    for(int v = 0; v < num_vertices; v++) {
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
 * Run the program on the VM.
 */
void GCVM::run() {
    bool converged = false;

    while(!converged) {
        // 1. Iterate active vertices
        #pragma omp parallel for
        for(int v = 0; v < num_vertices; v++) {
            // Only iterate over active vertices
            if(vertices.active[v]) {
                // Create context
                Context ctx;
                load_context(ctx, v);

                // Execute the kernel for each active vertex
                execute_vertex(ctx, v);

                // Store context
                store_context(ctx, v);
            } 
        }

        // 2. For now BARRIER is implicit (via threads joining)

        // 3. Apply phase (commit v_out -> v_self)
        for(int v = 0; v < num_vertices; v++) {
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