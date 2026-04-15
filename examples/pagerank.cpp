#include <vector>
#include <string>
#include <iostream>

#include "assembler.h"
#include "gcvm.h"

std::vector<Edge> page_rank_graph_edges() {
    std::vector<Edge> edges = {
        { 1, 2, 1 },
        { 2, 1, 1 },
        { 3, 0, 1 },
        { 3, 1, 1 },
        { 4, 1, 1 },
        { 4, 3, 1 },
        { 4, 5, 1 },
        { 5, 1, 1 },
        { 5, 4, 1 },
        { 6, 1, 1 },
        { 6, 4, 1 },
        { 7, 1, 1 },
        { 7, 4, 1 },
        { 8, 1, 1 },
        { 8, 4, 1 },
        { 9, 4, 1 },
        { 10, 4, 1 },
    };

    return edges;
}

int main(int argc, char **argv) {
    Graph page_rank_graph;
    page_rank_graph.build(page_rank_graph_edges(), 11);

    // Create the edge attributes of the Graph Runtime
    // (For page rank, these should be 1 / out_degree)
    for(uint32_t v = 0; v < page_rank_graph.size(); v++) {
        uint32_t start = page_rank_graph.incoming_row_offsets[v];
        uint32_t end = page_rank_graph.incoming_row_offsets[v + 1];
        for(auto e = start; e < end; e++) {
            uint32_t n = page_rank_graph.incoming_col_indices[e];
            size_t degree = page_rank_graph.out_degree(n);
            if(degree == 0) {
                page_rank_graph.incoming_edge_attr[e] = 0.0;
            } else {
                page_rank_graph.incoming_edge_attr[e] = 1.0 / degree;
            }
        }
    }

    GCVM runtime;
    runtime.load_graph(page_rank_graph);
    runtime.set_seed_vertices(true);
    runtime.set_seed_self(1.0 / page_rank_graph.size());

    Assembler assmblr;
    std::vector<std::string> source = {
        "start:",

        // Sum = 0
        "LOADI r1, 0",

        // Calculate 0.85 (integer immediates for now)
        "LOADI r30, 85",
        "LOADI r31, 100",

        // r5 = damping (0.85)
        "DIV r5, r30, r31",

        // r6 = (1 - d) / N
        "LOADI r30, 1",
        "SUB r30, r30, r5",
        "LOADV r6, 4",
        "DIV r6, r30, r6",

        // Iterate over incoming neighbors
        "ITER_NEIGHBORS end_iter",
        "loop:",
        // r2 = PR(neighbor)
        "LOADV r2, 1",

        // r3 = edge attr (1 / out degree)
        "LOADE r3, 0",
        "JZ r3, skip",

        // r2 = r2 * r3
        "MUL r2, r2, r3",

        // sum += r2
        "ADD r1, r1, r2",

        "skip:",

        "END_ITER loop",

        "end_iter:",
        // Apply damping
        // r1 = r1 * r5
        "MUL r1, r1, r5",

        // r1 = r1 + base
        "ADD r1, r1, r6",

        // Save old value
        "LOADV r8, 0",

        // New PR
        "MOV r7, r1",

        // Write new value
        "STOREV r7, 0",

        // Convergence check
        // diff = |old - new|
        "SUB r9, r8, r7",
        "ABS r9, r9",

        // Threshold = 0.01
        "LOADI r30, 1",
        "LOADI r31, 100",
        "DIV r4, r30, r31",

        // if diff > threshold, update
        "CMP_LT r9, r4, r9",
        "JNZ r9, changed",

        // No change -> halt this vertex
        "HALT",

        // Signal global update
        "changed:",
        "VOTE_CHANGE",
        
        // Activate neighbors
        "SCATTER",

        "HALT"
    };

    Program program = assmblr.assemble(source);

    runtime.load_program(program);

    runtime.run();

    auto results = runtime.get_results();

    std::cout << "Page Rank Results:" << std::endl;
    for(int i = 0; i < results.size(); i++) {
        std::cout << "Node " << i << ": " << results[i] << std::endl;
    }
}