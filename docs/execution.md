# GraphCore VM Execution & Memory Model (GCVM-EXEC v0.1)

## 1. Overview

This document defines the execution semantics and memory model of the GraphCore Virtual Machine (GCVM).

The model is designed to support scalable graph computation across CPUs, GPUs, and FPGA backends.

---

## 2. Execution Model

GCVM follows a **Bulk Synchronous Parallel (BSP)** model by default, with optional support for asynchronous execution in future versions.

Executions proceeds in iterations called **supersteps**.

Each superstep consists of:
 1. Gather phase
 2. Apply phase
 3. Scatter phase
 4. Synchronization barrier

---

## 3. Vertex-Centric Execution

Programs execute per vertex:
 - Each vertex runs the same bytecode
 - Execution is logically parallel
 - Vertices operate on local and neighbor data

---

## 4. Phases

### 4.1 Gather Phase

 - Reads data from neighboring vertices and edges
 - Uses `GATHER_*` instruction or explicit iteration
 - No writes to global state allowed

---

### 4.2 Apply Phase

 - Computes new vertex value (`v_out`)
 - Uses ALU instructions
 - Writes are local to the vertex

---

### 4.3 Scatter Phase

 - Propagates updates to neighbors
 - Triggered via `SCATTER` / `SCATTER_IF`
 - Marks neighbors as active for next superstep

---

### 4.4 Barrier

 - All vertices synchronize
 - `v_out` becomes new `v_self`
 - Ensures deterministic execution

---

## 5. Memory Model

### 5.1 Vertex State

 - Double-buffered:
    - `v_self` (read-only during superstep)
    - `v_out` (write-only during superstep)

### 5.2 Edge State

 - Read-only during execution
 - Mutable only between supersteps (optional extension)

### 5.3 Register State

 - Local to each vertex execution
 - Not shared across threads

---

## 6. Consistency Model

GCVM provides **superstep consistency**:
 - All reads observe state from previous superstep
 - Writes become visible only after barrier
 - No data races within a superstep

---

## 7. Active Vertex Model

 - Only active vertices execute
 - `SCATTER` marks neighbors as active
 - Inactive vertices are skipped

---

## 8. Convergence Detection

 - `VOTE_CHANGE` instruction signals updates
 - Runtime aggregates votes
 - Execution stops when no changes occur

---

## 9. Scheduling Model

### 9.1 Default

 - Parallel execution across threads
 - Partitioned graph

### 9.2 Future Extensions
 
 - Work-stealing scheduler
 - NUMA-aware partitioning
 - Priority-based scheduling

---

## 10. Communication Model

GCVM uses implicit communication:
 - No explicit message passing
 - Updates propagate via shared state + activation

Future versions may introduce explicit message buffers.

---

## 11. Determinism

 - BSP mode guarantees deterministic results
 - Order of execution does not affect outcome

---

## 12. Future Extensions

 - Asynchronous execution model
 - Relaxed consistency modes
 - Message-passing primitives
 - GPU/FPGA-specific memory hierarchies

---

## 13. Version

 - v0.1 Initial execution and memory model