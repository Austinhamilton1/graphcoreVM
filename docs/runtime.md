# GraphCore VM Graph Runtime Engine (GCVM-RUNTIME v0.1)

## 1. Overview

The GCVM Graph Runtime Engine is the execution substrate of the GraphCore VM system. It is responsible for:
 - Garph storage and memory layout
 - Vertex activation and scheduling
 - Parallel execution of GCVM bytecode
 - Coordination of BSP supersteps
 - Data movement between graph phases (Gather / Apply / Scatter)

This runtime is designed for high-performance execution on CPU first, with future extensions for GPU and FPGA backends.

---

## 2. Core Responsibilities

The runtime engine sits between the VM and the graph data structures.

It handles:
 - Loading graph data into optimized memory layouts
 - Managing active vertex sets
 - Scheduling vertex execution across threads
 - Ensuring correctness under BSP semantics
 - Coordinating synchronization barriers

---

## 3. Graph Memory Model

### 3.1 CSR Representation

The primary graph representation is Compressed Sparse Row (CSR):

```
row_offsets[v]  -> start index of adjacency list
col_indices[e]  -> neighbor vertex value
edge_attrs[e]   -> optional edge weights/attributes
```

### 3.2 Vertex State Storage

Vertex state is stored in a structure-of-arrays (SoA) layout:

```
v_self[v]       -> current vertex value
v_out[v]        -> next vertex value
active[v]       -> active flag (current superstep)
active_next[v]  -> active flag (next superstep)
```

### 3.3 Design Goals

 - Cache friendly adjacency traversal
 - SIMD/GPU-friendly contiguous memory
 - NUMA-aware partitioning (future extension)

---

## 4. Execution Model Integration

The runtime enforces GCVM's BSP model:

### Superstep Flow

 1. Execute active vertices
 2. Run GCVM bytecode per vertex
 3. Scatter phase updates active set
 4. Barrier synchronization
 5. Commit state updates

---

## 5. Vertex Activation System

### 5.1 Active Set Semantics

Only vertices marked as active are executed in a given superstep.

Activation sources:
 - Initial seed set (e.g., BFS root)
 - `SCATTER` operations
 - External runtime triggers

---

### 5.2 Active Set Storage

Two buffers are maintained

```
active[v]       -> current superstep execution mask
active_next[v]  -> next superstep execution mask
```

At barrier:

```
active = active_next
clear(active_next)
```

---

## Scheduling Model

### 6.1 Default Scheduler

The runtime uses a parallel-for scheduler
 - Graph vertices partitioned across threads
 - Each thread processes a subset of active vertices
 - Work distribution is static in v0.1

---

### 6.2 Future Scheduling Extensions

Planned enhancements:
 - Work-stealing scheduler
 - Dynamic load balancing
 - Priority-based execution (hot vertices)
 - NUMA-aware partitioning

---

## 7. Parallel Execution Model

### 7.1 Thread Model

Each thread executes:

```
for v in assigned_partition:
    if active[v]:
        execute_vertex(v)
```

### 7.2 Synchronization

A global barrier is enforced between supersteps:
 - Ensures deterministic execution
 - Prevents race conditions on `v_self` / `v_out`

---

## 8. Scatter Propagation System

### 8.1 Semantics

`SCATTER` does not directly modify neighbor state. Instead it:
 - Marks neighbors as active in `active_next`

### 8.2 Propagation Flow

```
1. vertex executes SCATTER
2. neighbors marked active_next
3. barrier swap
4. neighbors executed next superstep
```

---

## 9. Gather Execution Model

### 9.1 Locality Principle

Gather operations are always read-only and operate on:
 - neighbor `v_self` values
 - optional edge attributes

### 9.2 Execution Stategy

Two modes:

**A. Interpreter Mode**
 - Explicit loop over adjacency list

**B. Future Optimized Mode**
 - Vectorized reduction kernels
 - Prefetching adjacency blocks

---

## 10. Convergence Detection

The runtime tracks convergence using:
 - `VOTE_CHANGE` signals from vertices
 - Global reduction across threads

Execution stops when:

`no active vertices OR no state changes`

---

## 11. Memory Consistency Model

GCVM rutime guarantees:

### Within a superstep:

 - Reads see only `v_self` from previous superstep
 - Writes go only to `v_out`
 - No inter-thread visibility of partial updates

### Between supersteps:

 - `v_out` is committed to `v_self`
 - `active_next` becomes `active`

---

## 12. Graph Partitioning (Planned)

Future versions will support:
 - Partitioned CSR graphs
 - Thread-local vertex ownership
 - Cross-partition edge handling
 - NUMA-aware memory allocation

---

## 13. Performance Considerations

Key optimization targets:
 - Cache locality in adjacency traversal
 - Minimizing scatter overhead
 - Reducing active set size
 - Avoiding false sharing in vertex state

--- 

## 14. Runtime API (Conceptual)

```
GraphRuntime runtime;

runtime.load_graph(graph);
rutnime.load_program(bytecode);
runtime.set_seed_vertices(...);

runtime.run();
```

---

## 15. Integration with VM

The runtime engine executes GCVM bytecode via:
 - Interpreter dispatch loop (initial version)
 - Future JIT-compiled vertex kernels

Each vertex execution is isolated via a `Context` object.

---

## 16. Future Extensions

 - GPU execution backend (CUDA / Vulkan compute)
 - FPGA pipeline mapping
 - Message-passing mode (alternative to shared-state model)
 - Hybrid BSP + asynchronous execution

---

## 17. Version

 - v0.1 Initial runtime engine specification