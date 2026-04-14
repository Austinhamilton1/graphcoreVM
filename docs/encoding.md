# GraphCore VM ISA Encoding Specification (GCVM-ENC v0.2)

## 1. Overview

This document defines the refined binary encoding for the GraphCore VM (GCVM). This version improves decode simplicity, consistency, and hardware friendliness.

All instructions are 32 bits wide and use structured formats with clearly defined field roles.

---

## 2. Design Principles

 - Fixed-width 32-bit instructions
 - Minimal decode ambiguity
 - Consistent field placement across formats
 - Explicit sub-opcodes (no overloaded flags)
 - First class support for graph operations

---

## 3. Common Bit Layout

All instructions share a common high-level structure:

`[ OPCODE (6) | SUBOP (6) | FIELD_A (5) | FIELD_B (5) | FIELD_C/IMM (10) ]`

 - `OPCODE`: major instruction category
 - `SUBOP`: specific operation within category
 - Remaining fields depend on format

---

## 4. Instruction Formats

### 4.1 R-Type (Register-Register ALU)

`[ OPCODE (6) | SUBOP (6) | RD (5) | RS1 (5) | RS2 (5) ]`

 - `RD`: destination register
 - `RS1`, `RS2`: source registers

---

### 4.2 I-Type (Immediate)

`[ OPCODE (6) | SUBOP (6) | RD (5) | RS1 (5) | IMM (10) ] `

 - `RD`: destination register
 - `RS1`: source register
 - `IMM`: signed 10-bit immediate

---

### 4.3 B-Type (Branch)

`[ OPCODE (6) | SUBOP (6) | UNUSED (5) | RS1 (5) | OFFSET (10) ]`

 - `RS1`: source register
 - `OFFSET`: signed relative jump

---

### 4.4 G-Type (Graph Operations)

` [ OPCODE (6) | SUBOP (6) | RD (5) | MODE (5) | PARAM (10) ]`

 - `RD`: destination register
 - `MODE`: graph execution mode
 - `PARAM`: operation-specific parameter

*This format is central to GCVM.*

---

### 4.5 S-Type (System / Control)

`[ OPCODE (6) | SUBOP (6) | UNUSED (5) | RS1 (5) | FLAGS (10) ]`

 - `RS1`: source register
 - `FLAGS`: operation-specific flags

---

## 5. Opcode Categories

| OPCODE | Category |
| ------ | -------- |
| 0x00 | Control |
| 0x01 | ALU |
| 0x02 | Memory |
| 0x03 | Graph |
| 0x04 | System |

---

## 6. SUBOP Definitions

### 6.1 Control (OPCODE = 0x00)

| SUBOP | Instruction |
| ----- | ----------- |
| 0x00 | NOP |
| 0x01 | HALT |
| 0x02 | JMP |
| 0x03 | JZ |
| 0x04 | JNZ |

---

### 6.2 ALU (OPCODE = 0x01)

| SUBOP | Instruction |
| ----- | ----------- |
| 0x00 | ADD |
| 0x01 | SUB |
| 0x02 | MUL |
| 0x03 | DIV |
| 0x04 | MIN |
| 0x05 | MAX |
| 0x06 | ABS |
| 0x07 | MOV |
| 0x08 | LOADI |

---

### 6.3 Memory (OPCODE = 0x02)

| SUBOP | Instruction |
| ----- | ----------- |
| 0x00 | LOADV |
| 0x01 | STOREV |
| 0x02 | LOADE |

---

### 6.4 Graph (OPCODE = 0x03)

| SUBOP | Instruction |
| ----- | ----------- |
| 0x00 | ITER_NEIGHBORS |
| 0x01 | END_ITER |
| 0x02 | GATHER_SUM |
| 0x03 | GATHER_MIN |
| 0x04 | GATHER_MAX |
| 0x05 | GATHER_COUNT |
| 0x06 | SCATTER |
| 0x07 | SCATTER_IF |

---

### 6.5 System (OPCODE = 0x04)

| SUBOP | Instruction |
| ----- | ----------- |
| 0x00 | BARRIER |
| 0x01 | VOTE_CHANGE |

---

## 7. G-Type Mode Field

The `MODE` field refines graph behavior:

| MODE | Meaning |
| ---- | ------- |
| 0 | Default |
| 1 | Edge-weighted |
| 2 | Directed traversal |
| 3 | Reverse traversal |

---

## 8. Example Encodings 

### `ADD r1, r2, r3`

```
OPCODE = 0x01
SUBOP  = 0x00
RD     = 1
RS1    = 2
RS2    = 3
```

---

### `LOADI r1 42`

```
OPCODE = 0x01
SUBOP  = 0x08
RD     = 1
IMM    = 42
```

---

### `GATHER_SUM r1`

```
OPCODE = 0x03
SUBOP  = 0x02
RD     = 1
MODE   = 0
```

---

## 9. Key Improvements from v0.1

 - Unified field layout across all instructions
 - Explicit opcode + subopcode hierarchy
 - Cleaner separation of instruction categories
 - First class G-type format for graph operations
 - Hardware-friendly decoding (fixed positions)

---

## 10. Future Work

 - Expand immediate width (prefix instructions)
 - Add vector/warp execution formats
 - Add hardware hint bits for FPGA/GPU
 - Introduce message-passing specific encodings

---

## 11. Version

 - v0.2 Refined encoding with structured formats