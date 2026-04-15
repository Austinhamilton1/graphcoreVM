#include <vector>
#include <cassert>
#include <iostream>

#include "graph.h"

void graph_test() {
    std::cout << "Starting graph test...";

    std::vector<Edge> graph_edges = {
        { 0, 1, 1 },
        { 0, 2, 1 },
        { 1, 2, 1 },
    };

    Graph graph;
    graph.build(graph_edges, 3);

    // Row offset
    assert(graph.row_offsets.size() == 4);
    assert(graph.row_offsets[0] == 0);
    assert(graph.row_offsets[1] == 2);
    assert(graph.row_offsets[2] == 3);
    assert(graph.row_offsets[3] == 3);

    // Col indices
    assert(graph.col_indices.size() == 3);
    assert(graph.col_indices[0] == 1);
    assert(graph.col_indices[1] == 2);
    assert(graph.col_indices[2] == 2);

    // Edge attributes
    assert(graph.edge_attr.size() == 3);
    assert(graph.edge_attr[0] == 1);
    assert(graph.edge_attr[1] == 1);
    assert(graph.edge_attr[2] == 1);

    // Incoming row offset
    assert(graph.incoming_row_offsets.size() == 4);
    assert(graph.incoming_row_offsets[0] == 0);
    assert(graph.incoming_row_offsets[1] == 0);
    assert(graph.incoming_row_offsets[2] == 1);
    assert(graph.incoming_row_offsets[3] == 3);

    // Incoming col indices
    assert(graph.incoming_col_indices.size() == 3);
    assert(graph.incoming_col_indices[0] == 0);
    assert(graph.incoming_col_indices[1] == 0);
    assert(graph.incoming_col_indices[2] == 1);

    // Incoming edge attributes
    assert(graph.incoming_edge_attr.size() == 3);
    assert(graph.incoming_edge_attr[0] == 1);
    assert(graph.incoming_edge_attr[1] == 1);
    assert(graph.incoming_edge_attr[2] == 1);

    std::cout << "PASSED." << std::endl;
}

int main(int argc, char **argv) {
    graph_test();
}