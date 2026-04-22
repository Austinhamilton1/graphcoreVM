# GraphCore VM ISA Specification (GCVM-ISA v0.1)

## 1. Overview

The GraphCore Virtual Machine (GCVM) is a register-based execution model designed for graph computation using the Gather-Apply-Scatter (GAS) paradigm.

This ISA is designed to be:

 - Portable across CPU, GPU, and FPGA backends
 - Friendly to parallel and dataflow execution
 - Optimizable via compiler pipelines (e.g., LLVM)

Each program executes per-vertex in a graph context.

---

## 2. Execution Model

Each vertex executes the same program with the following implicit context:

 - `v_self`: current vertex value
 - `v_out`: output vertex value (next state)
 - `in_deg`: current vertex in-degree
 - `out_deg`: current vertex out-degree
 - `g_size`: graph size
 - `n_val`: neighbor value (during traversal)
 - `e_attr`: edge attribute (during traversal)

Execution proceeds in supersteps unless asynchronous mode is enabled.

---

## 3. Register File

### 3.1 General Purpose Registers

 - `r0-r31` (32 registers, 64-bit each)

### 3.2 Special Registers

| Register | Description |
| -------- | ----------- |
| `v_self` | Current vertex value (read-only) |
| `v_out` | Output vertex value (write-only) |
| `n_val` | Neighbor value (iteration scope) |
| `e_attr` | Edge attribute (iteration scope) |

---

## 4. Instruction Set

### 4.1 Control Instructions

**NOP**
```
Opcode: 0x00
Subop:  0x00
Description: No operation
```

**HALT**
```
Opcode: 0x00
Subop:  0x01
Description: Terminate execution
```

**JMP**
```
Opcode: 0x00
Subop:  0x02
Params: IMM = offset
Description: Unconditional jump by IMM
```

**JZ**
```
Opcode: 0x00
Subop:  0x03
Params: RS1 = source register, IMM = offset
Description: Jump by IMM if RS1 is zero
```

**JNZ**
```
Opcode: 0x00
Subop:  0x04
Params: RS1 = source register, IMM = offset
Description: Jump by IMM if RS1 is not zero
```

---

### 4.2 Arithmetic Instructions

**ADD**
```
Opcode: 0x01
Subop:  0x00
Params: RD = RS1 + RS2
Description: Add two registers
```

**SUB**
```
Opcode: 0x01
Subop:  0x01
Params: RD = RS1 - RS2
Description: Subtract two registers
```

**MUL**
```
Opcode: 0x01
Subop:  0x02
Params: RD = RS1 * RS2
Description: Multiply two registers
```

**DIV**
```
Opcode: 0x01
Subop:  0x03
Params: RD = RS1 / RS2
Description: Divide two registers
```

**MIN**
```
Opcode: 0x01
Subop:  0x04
Params: RD = Min(RS1, RS2)
Description: Store the minimum of two registers
```

**MAX**
```
Opcode: 0x01
Subop:  0x05
Params: RD = Max(RS1, RS2)
Description: Store the maximum of two registers
```

**ABS**
```
Opcode: 0x01
Subop:  0x06
Params: RD = |RS1|
Description: Store the absolute value of a register
```

---

### 4.3 Data Movement

**MOV**
```
Opcode: 0x01
Subop:  0x07
Params: RD = RS1
Description: Move a register
```

**LOADI**
```
Opcode: 0x01
Subop:  0x08
Params: RD = IMM
Description: Load an immediate value into a register
```

### 4.4 Comparison

**CMP_LT**
```
Opcode: 0x01
Subop:  0x09
Params: RD = RS1 < RS2 ? 1 : 0
Description: Determine if RS1 is less than RS2
```

**CMP_LTE**
```
Opcode: 0x01
Subop:  0x0A
Params: RD = RS1 <= RS2 ? 1 : 0
Description: Determine if RS1 is less than or equal to RS2
```

**CMP_EQ**
```
Opcode: 0x01
Subop:  0x0B
Params: RD = RS1 == RS2 ? 1 : 0
Description: Determine if RS1 is equal to RS2
```

**CMP_NEQ**
```
Opcode: 0x01
Subop:  0x0C
Params: RD = RS1 != RS2 ? 1 : 0
Description: Determine if RS1 is not equal to RS2
```

---

### 4.5 Graph Memory Access

**LOADV**
```
Opcode: 0x02
Subop:  0x00
Params: RD = vertex[IMM], IMM = attr_id
Description: Load a vertex attribute
  - attr_id 0 = v_self
  - attr_id 1 = n_val
  - attr_id 2 = in_deg
  - attr_id 3 = out_deg
```

**STOREV**
```
Opcode: 0x02
Subop:  0x01
Params: v_out = RS1
Description: Store a vertex attribute
```

**LOADE**
```
Opcode: 0x02
Subop:  0x02
Params: RD = edge_attr
Description: Load an edge attribute
```

**LOADG**
```
Opcode: 0x02
Subop:  0x03
Params: RD = g_size
Description: Load the graph size into a register
```

---

### 4.6 Traversal Instructions

**ITER_NEIGHBORS**
```
Opcode: 0x03
Subop:  0x00
Description: Begin iteration over neighbors, sets n_val and e_attr
```

**END_ITER**
```
Opcode: 0x03
Subop:  0x01
Description: End iteration block
```

---

### 4.7 Gather Instructions

**GATHER_SUM**
```
Opcode: 0x03
Subop:  0x02
Params: RD = Sum(neighbors)
Description: Accumulate sum over neighbors
```

**GATHER_MIN**
```
Opcode: 0x03
Subop:  0x03
Params: RD = Min(neighbors)
Description: Accumulate minimum over neighbors
```

**GATHER_MAX**
```
Opcode: 0x03
Subop:  0x04
Params: RD = Max(neighbors)
Description: Accumulate maximum over neighbors
```

**GATHER_COUNT**
```
Opcode: 0x03
Subop:  0x05
Params: RD = Count(neighbors)
Description: Accumulate count over neighbors
```

---

### 4.8 Scatter Instructions

**SCATTER**
```
Opcode: 0x03
Subop:  0x06
Description: Propagate v_out to neighbors
```

**SCATTER_IF**
```
Opcode: 0x03
Subop:  0x07
Params: RS1 = condition
Description: Propagate v_out to neighbors if RS1 is non-zero
```

---

### 4.9 Synchronization

**VOTE_CHANGE**
```
Opcode: 0x04
Subop:  0x00
Description: Contribute to convergence detection
```

---

## 5. Future Extensions

 - Vector registers for SIMD execution
 - Warp/group execution semantics
 - Explicit memory hierarchy control
 - FPGA pipeline mapping hints

## 6. Versioning
 - v0.1: Initial draft ISA