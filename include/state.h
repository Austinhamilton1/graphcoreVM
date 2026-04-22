#pragma once

#include <vector>
#include <cstdint>

#define R_COUNT 32

/*
 * Vertex state held across execution runs.
 * VertexState[v] = state of vertex v.
 */
struct VertexState {
    std::vector<double> v_self;         // Current (read-only) value of node
    std::vector<double> v_out;          // Next (write-only) value of node
    std::vector<uint8_t> active;         // Current active state of node
    std::vector<uint8_t> next_active;   // Next active state of node
};

/* 
 * Transient state that wraps each node for each superstep.
 * This is a stack-allocated data structure. It's purpose is
 * to ensure global state is managed by the runtime and not
 * touched by the kernel directly.
 */
struct Context {
    double regs[32];    // General purpose registers

    double v_self;      // Read-only current value of node
    double v_out;       // Write-only next value of node

    double n_val;       // Iterator (next neighbor node ID)
    double in_deg;      // Node in-degree
    double out_deg;     // Node out-degree
    double g_size;      // Graph size
    double e_attr;      // Iterator (next edge attribute)

    uint32_t iter_edge; // Current edge index
    uint32_t iter_end;  // End of adjacency list
};
