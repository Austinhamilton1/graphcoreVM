#include <cstring>

#include "jit.h"

/*
 * Constructor.
 * Arguments:
 *     Graph &graph - Underlying CSR graph of runtime.
 *     VertexState &state - Underlying vertex state of the runtime.
 *     std::atomic<uint32_t> *updates - Atomic update counter.
 */
void JIT::init(Graph &graph, VertexState &state, std::atomic<uint32_t> *updates) {
    // Outgoing edges
    jit_runtime.graph.row_offsets_out = graph.row_offsets.data();
    jit_runtime.graph.col_indices_out = graph.col_indices.data();
    jit_runtime.graph.edge_attr_out = graph.edge_attr.data();

    // Incoming edges
    jit_runtime.graph.row_offsets_in = graph.incoming_row_offsets.data();
    jit_runtime.graph.col_indices_in = graph.incoming_col_indices.data();
    jit_runtime.graph.edge_attr_in = graph.incoming_edge_attr.data();

    // Vertex state
    jit_runtime.vertices.v_self = state.v_self.data();
    jit_runtime.vertices.next_active = state.next_active.data();

    // Atomic updates
    jit_runtime.updates = updates;

    initialized = true;
}

/*
 * Return a reference to the JIT runtime for kernel execution.
 * Returns:
 *     JITGCVM * - Pointer to JIT runtime.
 */
JITGCVM *JIT::get_runtime() {
    return &jit_runtime;
}

/*
 * Is the runtime initialized?
 * Returns:
 *     bool - true if initialized, false otherwise.
 */
bool JIT::is_initialized() const {
    return initialized;
}

/*
 * Lower a program to LIR format.
 * Arguments:
 *     const Program &program - The program to lower.
 * Returns:
 *     std::vector<LInstruction> - Lowered IR version of the program.
 */
std::vector<LInstruction> JIT::lower(const Program &program) const {
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
                        out.target = pc + inst.imm() + 1;
                        break;

                    case SUBOP_JZ:
                        out.op = LOp::JZ;
                        out.src1 = inst.rs1();
                        out.target = pc + inst.imm() + 1;
                        break;

                    case SUBOP_JNZ:
                        out.op = LOp::JNZ;
                        out.src1 = inst.rs1();
                        out.target = pc + inst.imm() + 1;
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

                    case SUBOP_LOADG: {
                        out.op = LOp::LOAD_GRAPH_SIZE; 
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
                        out.target = pc + inst.imm() + 1;
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

    return lowered;
}

/* 
 * Load an immediate value into a register. 
 * Arguments: 
 *     x86::Assembler& a - Assembler to run commands on. 
 *     x86::Vec dst - Destination computer register. 
 *     double val - Value to load. 
 */ 
static void load_imm(x86::Compiler& cc, x86::Vec dst, double val) { 
    uint64_t bits;
    std::memcpy(&bits, &val, sizeof(bits));

    cc.mov(x86::rax, bits);
    cc.movq(dst, x86::rax);
}

/*
 * Compile a JIT kernel given the lowered instructions.
 * Arguments:
 *     const Program &program - The program to compile.
 * Returns:
 *     JITFunc - The compiled kernel.
 */
JITFunc JIT::compile(const Program &program) {
    std::vector<LInstruction> lir = lower(program);
    if(lir.size() == 0)
        throw std::runtime_error("Program lowered to empty LIR");

    // Initialization
    CodeHolder code;
    code.init(rt.environment(), rt.cpu_features());
    x86::Compiler cc(&code);
    // FileLogger logger(stdout);
    // code.set_logger(&logger);

    auto fn_ptr = cc.add_func(FuncSignature::build<void, Context *, uint32_t, JITGCVM *>());

    // Create virtual registers for the arguments of the function
    x86::Gp ctx = cc.new_gp_ptr("ctx");
    x86::Gp vid = cc.new_gp32("vid");
    x86::Gp jit_vm = cc.new_gp_ptr("jit_gcvm");

    /* Labels */
    std::vector<Label> labels(lir.size());
    for(auto &l : labels) l = cc.new_label();
    Label exit = cc.new_label();

    /* Register file. */
    struct VMRegisterFile {
        std::vector<x86::Vec> regs;

        VMRegisterFile(x86::Compiler &cc, size_t count) {
            regs.reserve(count);

            for(size_t i = 0; i < count; i++) {
                // Scalar float/double container in XMM register
                regs.push_back(cc.new_xmm());
            }
        }

        inline x86::Vec &get(uint32_t idx) {
            if(idx >= R_COUNT)
                throw std::runtime_error("Invalid register: " + idx);
            return regs[idx];
        }

        inline const x86::Vec &get(uint32_t idx) const {
            if(idx >= R_COUNT)
                throw std::runtime_error("Invalid register: " + idx);
            return regs[idx];
        }
    };

    VMRegisterFile vregs(cc, R_COUNT);
    
    // Set the arguments of the function to those passed in from the runtime
    fn_ptr->set_arg(0, ctx);
    fn_ptr->set_arg(1, vid);
    fn_ptr->set_arg(2, jit_vm);

    for(size_t pc = 0; pc < lir.size(); pc++) {
        cc.bind(labels[pc]);
        const auto &inst = lir[pc];

        switch (inst.op) {
            case LOp::ADD: {
                // Load virtual registers
                auto &r1 = vregs.get(inst.src1);
                auto &r2 = vregs.get(inst.src2);
                auto &rd = vregs.get(inst.dst);

                // Add to a temporary register to resolve potential collisions
                auto tmp = cc.new_xmm();
                cc.movsd(tmp, r1);
                cc.addsd(tmp, r2);
                cc.movsd(rd, tmp);
                break;
            }
            case LOp::SUB: {
                // Load virtual registers
                auto &r1 = vregs.get(inst.src1);
                auto &r2 = vregs.get(inst.src2);
                auto &rd = vregs.get(inst.dst);

                // Subtract from a temporary register to resolve potential collisions
                auto tmp = cc.new_xmm();
                cc.movsd(tmp, r1);
                cc.subsd(tmp, r2);
                cc.movsd(rd, tmp);
                break;
            }
            case LOp::MUL: {
                // Load virtual registers
                auto &r1 = vregs.get(inst.src1);
                auto &r2 = vregs.get(inst.src2);
                auto &rd = vregs.get(inst.dst);

                // Multiply to a temporary register to resolve potential collisions
                auto tmp = cc.new_xmm();
                cc.movsd(tmp, r1);
                cc.mulsd(tmp, r2);
                cc.movsd(rd, tmp);
                break;
            }
            case LOp::DIV: {
                // Load virtual registers
                auto &r1 = vregs.get(inst.src1);
                auto &r2 = vregs.get(inst.src2);
                auto &rd = vregs.get(inst.dst);

                // Divide from a temporary register to resolve potential collisions
                auto tmp = cc.new_xmm();
                cc.movsd(tmp, r1);
                cc.divsd(tmp, r2);
                cc.movsd(rd, tmp);
                break;
            }
            case LOp::MIN: {
                // Load virtual registers
                auto &r1 = vregs.get(inst.src1);
                auto &r2 = vregs.get(inst.src2);
                auto &rd = vregs.get(inst.dst);

                // Min with a temporary register to resolve potential collisions
                auto tmp = cc.new_xmm();
                cc.movsd(tmp, r1);
                cc.minsd(tmp, r2);
                cc.movsd(rd, tmp);
                break;
            }
            case LOp::MAX: {
                // Load virtual registers
                auto &r1 = vregs.get(inst.src1);
                auto &r2 = vregs.get(inst.src2);
                auto &rd = vregs.get(inst.dst);

                // Max with a temporary register to resolve potential collisions
                auto tmp = cc.new_xmm();
                cc.movsd(tmp, r1);
                cc.maxsd(tmp, r2);
                cc.movsd(rd, tmp);
                break;                
            }
            case LOp::ABS: {
                // Load virtual registers
                auto &r1 = vregs.get(inst.src1);
                auto &rd = vregs.get(inst.dst);

                // Temporary register to hold the mask
                auto tmp = cc.new_xmm();
                auto val = cc.new_xmm();    // Computation tmp

                // Clear sign bit via AND with 0x7FFFFFFFFFFFFFFFULL
                uint64_t mask = 0x7FFFFFFFFFFFFFFFULL; 
                cc.mov(x86::rax, mask);
                cc.movq(tmp, x86::rax);

                // Calculate ABS with correct register aliasing
                cc.movsd(val, r1);
                cc.andpd(val, tmp);
                cc.movsd(rd, val);
                break;
            }
            case LOp::MOV: {
                // Load virtual registers
                auto &r1 = vregs.get(inst.src1);
                auto &rd = vregs.get(inst.dst);

                // Move registers
                cc.movsd(rd, r1);
                break;
            }
            case LOp::LOADI: {
                // Load virtual registers
                auto &rd = vregs.get(inst.dst);

                // Load immediate values
                load_imm(cc, rd, inst.imm);
                break;
            }
            case LOp::CMPLT: {
                // Load virtual registers
                auto &r1 = vregs.get(inst.src1);
                auto &r2 = vregs.get(inst.src2);
                auto &rd = vregs.get(inst.dst);

                // CMPLT
                Label is_true = cc.new_label();
                Label done = cc.new_label();
                cc.ucomisd(r1, r2);
                cc.setb(x86::al);
                cc.test(x86::al, x86::al);
                cc.jnz(is_true);
                load_imm(cc, rd, 0.0);
                cc.jmp(done);
                cc.bind(is_true);
                load_imm(cc, rd, 1.0);
                cc.bind(done);
                break;
            }
            case LOp::CMPLTE: {
                // Load virtual registers
                auto &r1 = vregs.get(inst.src1);
                auto &r2 = vregs.get(inst.src2);
                auto &rd = vregs.get(inst.dst);

                // CMPLTE
                Label is_true = cc.new_label();
                Label done = cc.new_label();
                cc.ucomisd(r1, r2);
                cc.setbe(x86::al);
                cc.test(x86::al, x86::al);
                cc.jnz(is_true);
                load_imm(cc, rd, 0.0);
                cc.jmp(done);
                cc.bind(is_true);
                load_imm(cc, rd, 1.0);
                cc.bind(done);
                break;
            }
            case LOp::CMPEQ: {
                // Load virtual registers
                auto &r1 = vregs.get(inst.src1);
                auto &r2 = vregs.get(inst.src2);
                auto &rd = vregs.get(inst.dst);

                // CMPEQ
                Label is_true = cc.new_label();
                Label done = cc.new_label();
                cc.ucomisd(r1, r2);
                cc.sete(x86::al);
                cc.test(x86::al, x86::al);
                cc.jnz(is_true);
                load_imm(cc, rd, 0.0);
                cc.jmp(done);
                cc.bind(is_true);
                load_imm(cc, rd, 1.0);
                cc.bind(done);
                break;
            }
            case LOp::CMPNEQ: {
                // Load virtual registers
                auto &r1 = vregs.get(inst.src1);
                auto &r2 = vregs.get(inst.src2);
                auto &rd = vregs.get(inst.dst);

                // CMPNEQ
                Label is_true = cc.new_label();
                Label done = cc.new_label();
                cc.ucomisd(r1, r2);
                cc.setne(x86::al);
                cc.test(x86::al, x86::al);
                cc.jnz(is_true);
                load_imm(cc, rd, 0.0);
                cc.jmp(done);
                cc.bind(is_true);
                load_imm(cc, rd, 1.0);
                cc.bind(done);
                break;
            }
            case LOp::JMP: {
                // Unconditional jump
                cc.jmp(labels[inst.target]);
                break;
            }
            case LOp::JZ: {
                // Load virtual registers
                auto &r1 = vregs.get(inst.src1);
                auto tmp = cc.new_xmm();    // Fixes name aliasing

                // Conditional jump
                cc.xorpd(tmp, tmp);
                cc.ucomisd(r1, tmp);
                cc.sete(x86::al);
                cc.test(x86::al, x86::al);
                cc.jnz(labels[inst.target]);
                break;
            }
            case LOp::JNZ: {
                // Load virtual registers
                auto &r1 = vregs.get(inst.src1);
                auto xmm = cc.new_xmm();    // Fixes name aliasing

                // Conditional jump
                cc.xorpd(xmm, xmm);
                cc.ucomisd(r1, xmm);
                cc.setne(x86::al);
                cc.test(x86::al, x86::al);
                cc.jnz(labels[inst.target]);
                break;
                break;
            }
            case LOp::LOAD_V_SELF: {
                // Load virtual registers
                auto &rd = vregs.get(inst.dst);

                // Load from memory
                cc.movsd(rd, x86::ptr(ctx, offsetof(Context, v_self)));
                break;
            }
            case LOp::LOAD_N_VAL: {
                // Load virtual registers
                auto &rd = vregs.get(inst.dst);

                // Load from memory
                cc.movsd(rd, x86::ptr(ctx, offsetof(Context, n_val)));
                break;
            }
            case LOp::LOAD_IN_DEG: {
                // Load virtual registers
                auto &rd = vregs.get(inst.dst);

                // Load from memory
                cc.movsd(rd, x86::ptr(ctx, offsetof(Context, in_deg)));
                break;
            }
            case LOp::LOAD_OUT_DEG: {
                // Load virtual registers
                auto &rd = vregs.get(inst.dst);

                // Load from memory
                cc.movsd(rd, x86::ptr(ctx, offsetof(Context, out_deg)));
                break;
            }
            case LOp::LOAD_GRAPH_SIZE: {
                // Load virtual registers
                auto &rd = vregs.get(inst.dst);

                // Load from memory
                cc.movsd(rd, x86::ptr(ctx, offsetof(Context, g_size)));
                break;
            }
            case LOp::STORE_V_OUT: {
                // Load virtual registers
                auto &r1 = vregs.get(inst.src1);

                // Store into memory
                cc.movsd(x86::ptr(ctx, offsetof(Context, v_out)), r1);
                break;
            }
            case LOp::LOAD_E_ATTR: {
                // Load virtual registers
                auto &rd = vregs.get(inst.dst);

                // Load from memory
                cc.movsd(rd, x86::ptr(ctx, offsetof(Context, e_attr)));
                break;
            }
            case LOp::GATHER_SUM: {
                // Load virtual registers
                auto &rd = vregs.get(inst.dst);

                // Temporary accumulator register for aiasing
                x86::Vec acc = cc.new_xmm();
                cc.xorpd(acc, acc);

                // Load row_offsets
                // Load col_indices
                // Load v_self
                x86::Gp row_offsets = cc.new_gp_ptr();
                x86::Gp col_indices = cc.new_gp_ptr();
                x86::Gp v_self = cc.new_gp_ptr();

                cc.mov(row_offsets, x86::ptr(jit_vm, offsetof(JITGCVM, graph) + offsetof(JITGraph, row_offsets_in)));
                cc.mov(col_indices, x86::ptr(jit_vm, offsetof(JITGCVM, graph) + offsetof(JITGraph, col_indices_in)));
                cc.mov(v_self, x86::ptr(jit_vm, offsetof(JITGCVM, vertices) + offsetof(JITVertexState, v_self)));

                // Start index (row_offsets[vid])
                // End index (row_offsets[vid + 1])
                x86::Gp start = cc.new_gp32();
                x86::Gp end = cc.new_gp32();
                x86::Gp vid_plus1 = cc.new_gp32();

                cc.mov(start, x86::ptr(row_offsets, vid, 2));   // uint32_t = 4 btyes
                cc.lea(vid_plus1, x86::ptr(vid, 1));
                cc.mov(end, x86::ptr(row_offsets, vid_plus1, 2));   // uint32_t = 4 bytes

                // Hoist n and val outside of loop (neighbor and value)
                x86::Gp n = cc.new_gp32();
                x86::Vec val = cc.new_xmm();

                Label loop = cc.new_label();
                Label done = cc.new_label();

                // if(start >= end) jump out of loop
                cc.bind(loop);
                cc.cmp(start, end);
                cc.jae(done);

                // Load incoming edge
                cc.mov(n, x86::ptr(col_indices, start, 2)); // uint32_t = 4 bytes
                cc.movsd(val, x86::ptr(v_self, n, 3));    // double = 8 bytes
                cc.addsd(acc, val);

                // Update pointer
                cc.add(start, 1);
                cc.jmp(loop);

                // Save results
                cc.bind(done);
                cc.movsd(rd, acc);
                break;
            }
            case LOp::GATHER_MIN: {
                // Load virtual registers
                auto &rd = vregs.get(inst.dst);

                // Temporary accumulator register for aiasing
                x86::Vec acc = cc.new_xmm();

                // Load row_offsets
                // Load col_indices
                // Load v_self
                x86::Gp row_offsets = cc.new_gp_ptr();
                x86::Gp col_indices = cc.new_gp_ptr();
                x86::Gp v_self = cc.new_gp_ptr();

                cc.mov(row_offsets, x86::ptr(jit_vm, offsetof(JITGCVM, graph) + offsetof(JITGraph, row_offsets_in)));
                cc.mov(col_indices, x86::ptr(jit_vm, offsetof(JITGCVM, graph) + offsetof(JITGraph, col_indices_in)));
                cc.mov(v_self, x86::ptr(jit_vm, offsetof(JITGCVM, vertices) + offsetof(JITVertexState, v_self)));

                // Start index (row_offsets[vid])
                // End index (row_offsets[vid + 1])
                x86::Gp start = cc.new_gp32();
                x86::Gp end = cc.new_gp32();
                x86::Gp vid_plus1 = cc.new_gp32();

                cc.mov(start, x86::ptr(row_offsets, vid, 2));       // uint32_t = 4 btyes
                cc.lea(vid_plus1, x86::ptr(vid, 1));
                cc.mov(end, x86::ptr(row_offsets, vid_plus1, 2));   // uint32_t = 4 bytes

                Label empty = cc.new_label();
                Label loop = cc.new_label();
                Label done = cc.new_label();

                // Check for empty
                cc.cmp(start, end);
                cc.jae(empty);

                // Hoist n and val outside of loop (neighbor and value)
                x86::Gp n = cc.new_gp32();
                x86::Vec val = cc.new_xmm();

                // Initialize acc with first element
                cc.mov(n, x86::ptr(col_indices, start, 2));     // uint32_t = 4 bytes
                cc.movsd(acc, x86::ptr(v_self, n, 3));          // double = 8 bytes
                cc.add(start, 1);

                // if(start >= end) jump out of loop
                cc.bind(loop);
                cc.cmp(start, end);
                cc.jae(done);

                // Load incoming edge
                cc.mov(n, x86::ptr(col_indices, start, 2)); // uint32_t = 4 bytes
                cc.movsd(val, x86::ptr(v_self, n, 3));      // double = 8 bytes
                cc.minsd(acc, val);

                // Update pointer
                cc.add(start, 1);
                cc.jmp(loop);
            
                // If empty return 0.0
                cc.bind(empty);
                cc.xorpd(acc, acc);
            
                // If done, return accumulator
                cc.bind(done);
                cc.movsd(rd, acc);
                break;
            }
            case LOp::GATHER_MAX: {
                // Load virtual registers
                auto &rd = vregs.get(inst.dst);

                // Temporary accumulator register for aiasing
                x86::Vec acc = cc.new_xmm();

                // Load row_offsets
                // Load col_indices
                // Load v_self
                x86::Gp row_offsets = cc.new_gp_ptr();
                x86::Gp col_indices = cc.new_gp_ptr();
                x86::Gp v_self = cc.new_gp_ptr();

                cc.mov(row_offsets, x86::ptr(jit_vm, offsetof(JITGCVM, graph) + offsetof(JITGraph, row_offsets_in)));
                cc.mov(col_indices, x86::ptr(jit_vm, offsetof(JITGCVM, graph) + offsetof(JITGraph, col_indices_in)));
                cc.mov(v_self, x86::ptr(jit_vm, offsetof(JITGCVM, vertices) + offsetof(JITVertexState, v_self)));

                // Start index (row_offsets[vid])
                // End index (row_offsets[vid + 1])
                x86::Gp start = cc.new_gp32();
                x86::Gp end = cc.new_gp32();
                x86::Gp vid_plus1 = cc.new_gp32();

                cc.mov(start, x86::ptr(row_offsets, vid, 2));       // uint32_t = 4 btyes
                cc.lea(vid_plus1, x86::ptr(vid, 1));
                cc.mov(end, x86::ptr(row_offsets, vid_plus1, 2));   // uint32_t = 4 bytes

                Label empty = cc.new_label();
                Label loop = cc.new_label();
                Label done = cc.new_label();

                // Check for empty
                cc.cmp(start, end);
                cc.jae(empty);

                // Hoist n and val outside of loop (neighbor and value)
                x86::Gp n = cc.new_gp32();
                x86::Vec val = cc.new_xmm();

                // Initialize acc with first element
                cc.mov(n, x86::ptr(col_indices, start, 2));     // uint32_t = 4 bytes
                cc.movsd(acc, x86::ptr(v_self, n, 3));          // double = 8 bytes
                cc.add(start, 1);

                // if(start >= end) jump out of loop
                cc.bind(loop);
                cc.cmp(start, end);
                cc.jae(done);

                // Load incoming edge
                cc.mov(n, x86::ptr(col_indices, start, 2)); // uint32_t = 4 bytes
                cc.movsd(val, x86::ptr(v_self, n, 3));      // double = 8 bytes
                cc.maxsd(acc, val);

                // Update pointer
                cc.add(start, 1);
                cc.jmp(loop);
            
                // If empty return 0.0
                cc.bind(empty);
                cc.xorpd(acc, acc);
            
                // If done, return accumulator
                cc.bind(done);
                cc.movsd(rd, acc);
                break;
            }
            case LOp::GATHER_COUNT: {
                // Load virtual registers
                auto &rd = vregs.get(inst.dst);

                // Temporary count register for conversion
                x86::Gp count = cc.new_gp32();

                // Load row_offsets
                // Load col_indices
                // Load v_self
                x86::Gp row_offsets = cc.new_gp_ptr();
                x86::Gp col_indices = cc.new_gp_ptr();
                x86::Gp v_self = cc.new_gp_ptr();

                cc.mov(row_offsets, x86::ptr(jit_vm, offsetof(JITGCVM, graph) + offsetof(JITGraph, row_offsets_in)));
                cc.mov(col_indices, x86::ptr(jit_vm, offsetof(JITGCVM, graph) + offsetof(JITGraph, col_indices_in)));
                cc.mov(v_self, x86::ptr(jit_vm, offsetof(JITGCVM, vertices) + offsetof(JITVertexState, v_self)));

                // Start index (row_offsets[vid])
                // End index (row_offsets[vid + 1])
                x86::Gp start = cc.new_gp32();
                x86::Gp end = cc.new_gp32();
                x86::Gp vid_plus1 = cc.new_gp32();

                // Calculate start and end
                cc.mov(start, x86::ptr(row_offsets, vid, 2));       // uint32_t = 4 btyes
                cc.lea(vid_plus1, x86::ptr(vid, 1));
                cc.mov(end, x86::ptr(row_offsets, vid_plus1, 2));   // uint32_t = 4 bytes

                // Calculate count
                cc.mov(count, end);
                cc.sub(count, start);
                
                // Copy data from int to double
                cc.cvtsi2sd(rd, count);
                break;
            }
            case LOp::SCATTER: {
                x86::Gp row_offsets = cc.new_gp_ptr();
                x86::Gp col_indices = cc.new_gp_ptr();
                x86::Gp next_active = cc.new_gp_ptr();

                // Load pointers
                cc.mov(row_offsets, x86::ptr(jit_vm, offsetof(JITGCVM, graph) + offsetof(JITGraph, row_offsets_out)));
                cc.mov(col_indices, x86::ptr(jit_vm, offsetof(JITGCVM, graph) + offsetof(JITGraph, col_indices_out)));
                cc.mov(next_active, x86::ptr(jit_vm, offsetof(JITGCVM, vertices) + offsetof(JITVertexState, next_active)));
                
                // Start index (row_offsets[vid]) 
                // End index (row_offsets[vid + 1]) 
                x86::Gp start = cc.new_gp32(); 
                x86::Gp end = cc.new_gp32(); 
                x86::Gp vid_plus1 = cc.new_gp32(); 
                
                cc.mov(start, x86::ptr(row_offsets, vid, 2)); // uint32_t = 4 btyes 
                cc.lea(vid_plus1, x86::ptr(vid, 1)); 
                cc.mov(end, x86::ptr(row_offsets, vid_plus1, 2)); // uint32_t = 4 bytes

                // Hoist n outside of loop (neighbor)
                x86::Gp n = cc.new_gp32();
                x86::Gp val = cc.new_gp8();
                cc.mov(val, 1);

                Label loop = cc.new_label();
                Label done = cc.new_label();

                cc.bind(loop);
                // if(start >= end) jump out of loop
                cc.cmp(start, end);
                cc.jae(done);

                // Load incoming edge
                cc.mov(n, x86::ptr(col_indices, start, 2)); // uint32_t = 4 bytes
                cc.mov(x86::ptr(next_active, n), val);

                cc.add(start, 1);
                cc.jmp(loop);
                cc.bind(done);
                break;
            }
            case LOp::SCATTER_IF: {
                // Load virtual registers
                auto &r1 = vregs.get(inst.src1);
                auto tmp = cc.new_xmm();    // Prevents name aliasing

                // Conditional scatter
                cc.xorpd(tmp, tmp);
                cc.ucomisd(r1, tmp);
                cc.je(labels[pc + 1]);

                x86::Gp row_offsets = cc.new_gp_ptr();
                x86::Gp col_indices = cc.new_gp_ptr();
                x86::Gp next_active = cc.new_gp_ptr();

                // Load pointers
                cc.mov(row_offsets, x86::ptr(jit_vm, offsetof(JITGCVM, graph) + offsetof(JITGraph, row_offsets_out)));
                cc.mov(col_indices, x86::ptr(jit_vm, offsetof(JITGCVM, graph) + offsetof(JITGraph, col_indices_out)));
                cc.mov(next_active, x86::ptr(jit_vm, offsetof(JITGCVM, vertices) + offsetof(JITVertexState, next_active)));
                
                // Start index (row_offsets[vid]) 
                // End index (row_offsets[vid + 1]) 
                x86::Gp start = cc.new_gp32(); 
                x86::Gp end = cc.new_gp32(); 
                x86::Gp vid_plus1 = cc.new_gp32(); 
                
                cc.mov(start, x86::ptr(row_offsets, vid, 2)); // uint32_t = 4 btyes 
                cc.lea(vid_plus1, x86::ptr(vid, 1)); 
                cc.mov(end, x86::ptr(row_offsets, vid_plus1, 2)); // uint32_t = 4 bytes

                // Hoist n outside of loop (neighbor)
                x86::Gp n = cc.new_gp32();
                x86::Gp val = cc.new_gp8();
                cc.mov(val, 1);

                Label loop = cc.new_label();
                Label done = cc.new_label();

                cc.bind(loop);
                // if(start >= end) jump out of loop
                cc.cmp(start, end);
                cc.jae(done);

                // Load incoming edge
                cc.mov(n, x86::ptr(col_indices, start, 2)); // uint32_t = 4 bytes
                cc.mov(x86::ptr(next_active, n), val);

                cc.add(start, 1);
                cc.jmp(loop);
                cc.bind(done);
                break;
            }
            case LOp::ITER_BEGIN: {
                x86::Gp iter_edge = cc.new_gp32();
                x86::Gp iter_end = cc.new_gp32();

                // Load offset base
                x86::Gp row_offsets = cc.new_gp_ptr();
                cc.mov(row_offsets, x86::ptr(jit_vm, offsetof(JITGCVM, graph) + offsetof(JITGraph, row_offsets_in)));

                // iter_edge = row_offsets[vid]
                cc.mov(iter_edge, x86::ptr(row_offsets, vid, 2));   // uint32_t = 4 bytes

                // iter_end = row_offsets[vid + 1]
                x86::Gp vid_plus1 = cc.new_gp32();
                cc.lea(vid_plus1, x86::ptr(vid, 1));
                cc.mov(iter_end, x86::ptr(row_offsets, vid_plus1, 2));  // uint32_t = 4 bytes

                // Store into ctx
                cc.mov(x86::ptr(ctx, offsetof(Context, iter_edge)), iter_edge);
                cc.mov(x86::ptr(ctx, offsetof(Context, iter_end)), iter_end);

                // if(iter_edge >= iter_end) jump
                cc.cmp(iter_edge, iter_end);
                cc.jae(labels[inst.target]);

                // Load col_indices
                x86::Gp col_indices = cc.new_gp_ptr();
                cc.mov(col_indices, x86::ptr(jit_vm, offsetof(JITGCVM, graph) + offsetof(JITGraph, col_indices_in)));

                x86::Gp neighbor = cc.new_gp32();
                cc.mov(neighbor, x86::ptr(col_indices, iter_edge, 2));

                // Load v_self
                x86::Gp v_self = cc.new_gp_ptr();
                cc.mov(v_self, x86::ptr(jit_vm, offsetof(JITGCVM, vertices) + offsetof(JITVertexState, v_self)));
                
                // Set n_val
                x86::Vec n_val = cc.new_xmm();
                cc.movsd(n_val, x86::ptr(v_self, neighbor, 3)); // Double = 8 bytes
                cc.movsd(x86::ptr(ctx, offsetof(Context, n_val)), n_val);

                // Load edge_attr
                x86::Gp edge_attr = cc.new_gp_ptr();
                cc.mov(edge_attr, x86::ptr(jit_vm, offsetof(JITGCVM, graph) + offsetof(JITGraph, edge_attr_in)));

                x86::Vec e_attr = cc.new_xmm();
                cc.movsd(e_attr, x86::ptr(edge_attr, iter_edge, 3));    // Double = 8 bytes
                cc.movsd(x86::ptr(ctx, offsetof(Context, e_attr)), e_attr);

                break;
            }
            case LOp::ITER_NEXT: {
                x86::Gp iter_edge = cc.new_gp32();
                x86::Gp iter_end  = cc.new_gp32();

                // load from ctx
                cc.mov(iter_edge, x86::ptr(ctx, offsetof(Context, iter_edge)));
                cc.mov(iter_end,  x86::ptr(ctx, offsetof(Context, iter_end)));

                // iter_edge++
                cc.add(iter_edge, 1);
                cc.mov(x86::ptr(ctx, offsetof(Context, iter_edge)), iter_edge);

                // if (iter_edge >= iter_end) exit loop
                cc.cmp(iter_edge, iter_end);
                cc.jae(labels[pc + 1]);

                // same load sequence as ITER_BEGIN
                x86::Gp col_indices = cc.new_gp_ptr();
                cc.mov(col_indices, x86::ptr(jit_vm, offsetof(JITGCVM, graph) + offsetof(JITGraph, col_indices_in)));

                x86::Gp neighbor = cc.new_gp32();
                cc.mov(neighbor, x86::ptr(col_indices, iter_edge, 2));  // uint32_t = 4 bytes

                // Load v_self
                x86::Gp v_self = cc.new_gp_ptr();
                cc.mov(v_self, x86::ptr(jit_vm, offsetof(JITGCVM, vertices) + offsetof(JITVertexState, v_self)));
                
                // Set n_val
                x86::Vec n_val = cc.new_xmm();
                cc.movsd(n_val, x86::ptr(v_self, neighbor, 3)); // Double = 8 bytes
                cc.movsd(x86::ptr(ctx, offsetof(Context, n_val)), n_val);

                // Load edge attr
                x86::Gp edge_attr = cc.new_gp_ptr();
                cc.mov(edge_attr, x86::ptr(jit_vm, offsetof(JITGCVM, graph) + offsetof(JITGraph, edge_attr_in)));

                x86::Vec e_attr = cc.new_xmm();
                cc.movsd(e_attr, x86::ptr(edge_attr, iter_edge, 3));    // Double = 8 bytes
                cc.movsd(x86::ptr(ctx, offsetof(Context, e_attr)), e_attr);

                cc.jmp(labels[inst.target]);

                break;
            }
            case LOp::VOTE_CHANGE: {
                x86::Gp updates = cc.new_gp_ptr();

                cc.mov(updates, x86::ptr(jit_vm, offsetof(JITGCVM, updates)));

                cc.lock();
                cc.inc(x86::dword_ptr(updates));
                break;
            }
            case LOp::RETURN: {
                // Jump to exit to finalize
                cc.jmp(exit);
                break;
            }
            default: {
                throw std::runtime_error("Invalid operation");
            }
        }
    }

    // Return from function and end function
    cc.bind(exit);
    cc.ret();
    cc.end_func();
    cc.finalize();

    // Compile the function
    JITFunc fn;
    Error err = rt.add(&fn, &code);
    if(err != Error::kOk) {
        throw std::runtime_error("Could not create function");
        return nullptr;
    }

    return fn;
}
