#include <assert.h>

#include "GCVM.h"

/*
 * Load a graph into the graph runtime.
 * Arguments:
 *     const Graph &graph - Load this graph.
 */
void GCVM::load_graph(const Graph &graph) {
    this->graph = graph;
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
        case SUBOP_NOP:
            return pc;
        case SUBOP_HALT:
            return -1;
        case SUBOP_JMP:
            return pc + inst.imm();
        case SUBOP_JZ:
            assert(inst.rs1() < R_COUNT);
            return ctx.regs[inst.rs1()] == 0 ? pc + inst.imm() : pc;
        case SUBOP_JNZ:
            assert(inst.rs1() < R_COUNT);
            return ctx.regs[inst.rs1()] != 0 ? pc + inst.imm() : pc;
    }
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
        case SUBOP_ADD:
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            ctx.regs[inst.rd()] = ctx.regs[inst.rs1()] + ctx.regs[inst.rs2()];
            break;
        case SUBOP_SUB:
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            ctx.regs[inst.rd()] = ctx.regs[inst.rs1()] - ctx.regs[inst.rs2()];
            break;
        case SUBOP_MUL:
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            ctx.regs[inst.rd()] = ctx.regs[inst.rs1()] * ctx.regs[inst.rs2()];
            break;
        case SUBOP_DIV:
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            ctx.regs[inst.rd()] = ctx.regs[inst.rs1()] / ctx.regs[inst.rs2()];
            break;
        case SUBOP_MIN:
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            ctx.regs[inst.rd()] = (ctx.regs[inst.rs1()] < ctx.regs[inst.rs2()]) ? ctx.regs[inst.rs1()] : ctx.regs[inst.rs2()];
            break;
        case SUBOP_MAX:
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            ctx.regs[inst.rd()] = (ctx.regs[inst.rs1()] > ctx.regs[inst.rs2()]) ? ctx.regs[inst.rs1()] : ctx.regs[inst.rs2()];
            break;
        case SUBOP_ABS:
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT);
            ctx.regs[inst.rd()] = (ctx.regs[inst.rs1()] < 0) ? -ctx.regs[inst.rs1()] : ctx.regs[inst.rs1()];
            break;
        case SUBOP_MOV:
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT);
            ctx.regs[inst.rd()] = ctx.regs[inst.rs1()];
            break;
        case SUBOP_LOADI:
            assert(inst.rd() < R_COUNT);
            ctx.regs[inst.rd()] = inst.imm();
            break;
        case SUBOP_CMPLT:
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            ctx.regs[inst.rd()] = (ctx.regs[inst.rs1()] < ctx.regs[inst.rs2()]) ? 1 : 0;
            break;
        case SUBOP_CMPLTE:
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            ctx.regs[inst.rd()] = (ctx.regs[inst.rs1()] <= ctx.regs[inst.rs2()]) ? 1 : 0;
            break;
        case SUBOP_CMPEQ:
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            ctx.regs[inst.rd()] = (ctx.regs[inst.rs1()] == ctx.regs[inst.rs2()]) ? 1 : 0;
            break;
        case SUBOP_CMPNEQ:
            assert(inst.rd() < R_COUNT && inst.rs1() < R_COUNT && inst.rs2() < R_COUNT);
            ctx.regs[inst.rd()] = (ctx.regs[inst.rs1()] != ctx.regs[inst.rs2()]) ? 1 : 0;
            break;
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
        case SUBOP_LOADV:
            switch(inst.imm()) {
                case 0:
                    assert(inst.rd() < R_COUNT && vertex_id < vertices.v_self.size());
                    ctx.regs[inst.rd()] = vertices.v_self[vertex_id];
                    break;
                default:
                    break;
            }
            break;
        case SUBOP_STOREV:
            switch(inst.imm()) {
                case 0:
                    assert(inst.rs1() < R_COUNT && vertex_id < vertices.v_out.size());
                    vertices.v_out[vertex_id] = ctx.regs[inst.rs1()];
                    break;
                default:
                    break;
            }
            break;
        case SUBOP_LOADE:
            switch(inst.imm()) {
                case 0:
                    assert(inst.rd() < R_COUNT);
                    ctx.regs[inst.rd()] = ctx.e_attr;
                    break;
                default:
                    break;
            }
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
        case SUBOP_ITER_NEIGHBORS:
            assert(vertex_id < graph.row_offsets.size() - 1);

            // Initialize iterators
            ctx.iter_edge = graph.row_offsets[vertex_id];
            ctx.iter_end = graph.row_offsets[vertex_id + 1];

            if(ctx.iter_edge >= ctx.iter_end) {
                // Skip loop body
                return pc + inst.imm(); // Jump to END_ITER
            }

            // Load first neighbor
            uint32_t e = ctx.iter_edge;
            ctx.n_val = graph.col_indices[e];
            if(!graph.edge_weights.empty()) {
                ctx.e_attr = graph.edge_weights[e];
            }
            break;
        case SUBOP_END_ITER:
            ctx.iter_edge++;

            if(ctx.iter_edge < ctx.iter_end) {
                // Load next neighbor
                uint32_t e = ctx.iter_edge;
                ctx.n_val = graph.col_indices[e];
                if(!graph.edge_weights.empty()) {
                    ctx.e_attr = graph.edge_weights[e];
                }

                return pc + inst.imm(); // Jump back
            }
            break;
        case SUBOP_GATHER_SUM:
            assert(inst.rd() < R_COUNT && vertex_id < graph.row_offsets.size() - 1);

            // Calculate the sum of the neighbors' v_self
            double sum = 0.0;
            for(auto e = graph.row_offsets[vertex_id]; e < graph.row_offsets[vertex_id + 1]; ++e) {
                uint32_t n = graph.col_indices[e];
                sum += vertices.v_self[n];
            }

            // Store sum in RD
            ctx.regs[inst.rd()] = sum;
            break;
        case SUBOP_GATHER_MIN:
            assert(inst.rd() < R_COUNT && vertex_id < graph.row_offsets.size() - 1);

            // Calculate the min of the neighbors' v_self
            double min = __DBL_MAX__;
            for(auto e = graph.row_offsets[vertex_id]; e < graph.row_offsets[vertex_id + 1]; ++e) {
                uint32_t n = graph.col_indices[e];
                if(vertices.v_self[n] < min)
                    min = vertices.v_self[n];
            }

            // Store min in RD
            ctx.regs[inst.rd()] = min;
            break;
        case SUBOP_GATHER_MAX:
            assert(inst.rd() < R_COUNT && vertex_id < graph.row_offsets.size() - 1);

            // Calculate the max of the neighbors' v_self
            double max = __DBL_MIN__;
            for(auto e = graph.row_offsets[vertex_id]; e < graph.row_offsets[vertex_id + 1]; ++e) {
                uint32_t n = graph.col_indices[e];
                if(vertices.v_self[n] > max)
                    max = vertices.v_self[n];
            }

            // Store max in RD
            ctx.regs[inst.rd()] = max;
            break;
        case SUBOP_GATHER_COUNT:
            assert(inst.rd() < R_COUNT && vertex_id < graph.row_offsets.size() - 1);

            // Count the number of neighbors
            double count = 0.0;
            for(auto e = graph.row_offsets[vertex_id]; e < graph.row_offsets[vertex_id + 1]; ++e) {
                count += 1;
            }

            // Store the count in RD
            ctx.regs[inst.rd()] = count;
            break;
        case SUBOP_SCATTER:
            assert(vertex_id < graph.row_offsets.size() - 1);

            // Set the next_active of all neighbors to true
            for(auto e = graph.row_offsets[vertex_id]; e < graph.row_offsets[vertex_id + 1]; ++e) {
                uint32_t n = graph.col_indices[e];
                vertices.next_active[n] = true;
            }
            break;
        case SUBOP_SCATTER_IF:
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
        case SUBOP_BARRIER:
            // BARRIER is automatically handled by the runtime system.
            break;
        case SUBOP_VOTE_CHANGE:
            // Signal there was an update
            updates.fetch_add(1, std::memory_order_relaxed);
            break;
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
            case OPCODE_CONTROL:
                pc = exec_control(inst, ctx, pc);
                break;
            case OPCODE_ALU:
                exec_alu(inst, ctx);
                break;
            case OPCODE_MEMORY:
                exec_memory(inst, ctx, vertex_id);
                break;
            case OPCODE_GRAPH:
                pc = exec_graph(inst, ctx, vertex_id, pc);
                break;
            case OPCODE_SYSTEM:
                exec_system(inst);
                break;
        }

        // Check for validity of the PC (HALT returns -1 for example)
        if(pc < 0) break;

        pc++;
    }
}