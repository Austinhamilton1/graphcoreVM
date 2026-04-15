# GraphCore VM (GCVM)

## Overview

GraphCore VM (GCVM) is a graph computation system build around a custom register-based virtual machine and a domain specific instruction set designed for the Gather-Apply-Scatter (GAS) paradigm.

The goal of GCVM is to provide a **portable, high-performance execution model for graph workloads** that can target CPUs, GPUs, and potentially FPGA backends.

This project explores the intersection of:
 - Systems programming
 - Parallel and concurrent runtimes
 - Domain-specific languages (DSLs)
 - Compiler construction (LLVM-based JIT)
 - Graph processing systems

---

## Key Features

 - **Custom ISA ([GCVM-ISA](docs/isa.md))**
    - Register-based virtual machine
    - Graph-aware instructions (`GATHER`, `SCATTER`, `ITER`)
    - Designed for data-parallel execution
 - **Structured Encoding ([GCVM-ENC](docs/encoding.md))**
    - 32-bit fixed-width instructions
    - RISC-inspired opcode + subopcode design
    - Hardware-friendly decoding
 - **Execution Model ([GCVM-EXEC](docs/execution.md))**
    - Bulk Synchronous Parallel (BSP)
    - Vertex-centric computation
    - Double-buffered state (`v_self`, `v_out`)
    - Active vertex scheduling
 - **Graph Runtime Engine ([GCVM-RUNTIME](docs/runtime.md))**
    - CSR-based graph storage
    - Parallel execution across vertices
    - Frontier-based computation model
 - **Planned JIT Compilation**
    - LLVM-based lowering from bytecode to native code
    - Whole-program vertex kernel compilation
    - Future specialization for graph structure and hardware

---

## Architecture

GCVM is structured as a layered system:

```
+---------------------------+
|  DSL / Frontend (future)  |
+---------------------------+
|    Bytecode (GCVM-ISA)    |
+---------------------------+
|   VM Interpreter / JIT    |
+---------------------------+
|   Graph Runtime Engine    |
+---------------------------+
|    Graph Storage (CSR)    |
+---------------------------+
```

---

## Execution Model

GCVM follows a **Bulk Synchronous Parallel (BSP)** model:
 1. **Gather**: Read neighbor data
 2. **Apply**: Compute new vertex state
 3. **Scatter**: Activate neighbors
 4. **Barrier**: Synchronize and commit updates

Key properties:
 - Deterministic execution
 - No data races within a superstep
 - Scallable parallelism

---

## Project Goals

### Short-Term

 - Build a working interpreter for GCVM bytecode
 - Implement core graph primitives (`GATHER_SUM`, `SCATTER`)
 - Execute basic algorithms:
    - PageRank
    - Breadth-First Search (BFS)

### Medium-Term

 - Optimize runtime:
    - Active vertex queues
    - Cache-aware graph traversal
    - Parallel scheduling

### Long-Term

 - LLVM-based JIT compiler
 - NUMA-aware and lock-free runtime
 - GPU / FPGA backend exploration
 - DSL for graph kernel definition

---

## Example (Conceptual)

A PageRank-like computation in GCVM might look like:

```
GATHER_SUM r1
MUL r2, r1, 0.85
ADD r3, r2, 0.15
MOV v_out, r3
SCATTER
```

---

## Current Status

 - ISA Specification: Done
 - Encoding Specification: Done
 - Execution Model: Done
 - Runtime Engine Design: Done
 - Interpreter: In Progress
 - JIT Compiler: Planned

---

## Future Directions

 - Dataflow-oriented instruction extensions
 - Warp / SIMD-aware execution
 - Graph partitioning and NUMA optimizations
 - Hardware mapping (FPGA pipelines)

---

## Why This Project

Modern graph systems often expose high-level APIs but hide execution details. GCVM takes a different approach:

> Treat graph computation as a **first-class execution architecture**.

By defining an ISA, execution model, and rutime from the ground up, GCVM enables:
 - Fine-grained control over execution
 - Cross-platform portability
 - Research into graph-specific optimizations

---

## Getting Started (Planned)

```
# Build
mkdir build && cd build
cmake ..
make 
```

---

## Inspiration

GCVM draws inspiration from:
 - Vertex-centric graph systems
 - Data-parallel runtimes
 - RISC-style ISA design
 - GPU and accelerator architectures

---

## Authors

Developed by **Austin Hamilton**

M.S. in Computer Science - East Tennessee State University

---

## License

MIT @ 2026 Austin Hamilton

---