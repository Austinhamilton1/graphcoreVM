#pragma

#include <vector>
#include <cstdint>
#include <cassert>
#include <algorithm>

struct Edge {
    uint32_t src;
    uint32_t dst;
    double weight; // Optional
};

/*
 * In GCVM, graphs are static and are stored
 * in compressed sparse row (CSR) format.
 */
struct Graph {
    /* Outgoing edges */
    std::vector<uint32_t> row_offsets;
    std::vector<uint32_t> col_indices;
    std::vector<double>  edge_attr;   // Optional

    /* Incoming edges */
    std::vector<uint32_t> incoming_row_offsets;
    std::vector<uint32_t> incoming_col_indices;
    std::vector<double> incoming_edge_attr;   // Optional

    size_t num_vertices = 0;
    size_t num_edges = 0;

    /*
     * Build the outgoing CSR.
     * Arguments:
     *     const std::vector<Edge>& edges - Build the graph from this edge list.
     *     size_t V - Number of vertices in the graph.
     */
    void build_outgoing(const std::vector<Edge>& edges, size_t V) {
        num_vertices = V;
        num_edges = edges.size();

        row_offsets.assign(V + 1, 0);
        col_indices.resize(edges.size());
        edge_attr.resize(edges.size());

        // 1. Count degrees
        for (const auto& e : edges) {
            row_offsets[e.src + 1]++;
        }

        // 2. Prefix sum
        for (size_t i = 1; i <= V; i++) {
            row_offsets[i] += row_offsets[i - 1];
        }

        // 3. Temporary cursor (important!)
        std::vector<uint32_t> cursor = row_offsets;

        // 4. Fill CSR
        for (const auto& e : edges) {
            uint32_t pos = cursor[e.src]++;

            col_indices[pos] = e.dst;
            edge_attr[pos] = e.weight;
        }
    }

    /*
     * Build the incoming CSR.
     * Arguments:
     *     const std::vector<Edge>& edges - Build the graph from this edge list.
     *     size_t V - Number of vertices in the graph.
     */
    void build_incoming(const std::vector<Edge>& edges, size_t V) {
        incoming_row_offsets.assign(V + 1, 0);
        incoming_col_indices.resize(edges.size());
        incoming_edge_attr.resize(edges.size());

        // 1. Count in-degrees
        for (const auto& e : edges) {
            incoming_row_offsets[e.dst + 1]++;
        }

        // 2. Prefix sum
        for (size_t i = 1; i <= V; i++) {
            incoming_row_offsets[i] += incoming_row_offsets[i - 1];
        }

        // 3. Cursor
        std::vector<uint32_t> cursor = incoming_row_offsets;

        // 4. Fill
        for (const auto& e : edges) {
            uint32_t pos = cursor[e.dst]++;

            incoming_col_indices[pos] = e.src;
            incoming_edge_attr[pos] = e.weight;
        }
    }

    /*
     * Build the CSR (outgoing and incoming).
     * Arguments:
     *     const std::vector<Edge>& edges - Build the graph from this edge list.
     *     size_t V - Number of vertices in the graph.
     */
    void build(const std::vector<Edge>& edges, size_t V) {
        build_outgoing(edges, V);
        build_incoming(edges, V);
    }

    size_t size() const {
        return num_vertices;
    }

    size_t out_degree(uint32_t v) const {
        return row_offsets[v + 1] - row_offsets[v];
    }

    size_t in_degree(uint32_t v) const {
        return incoming_row_offsets[v + 1] - incoming_row_offsets[v];
    }
};