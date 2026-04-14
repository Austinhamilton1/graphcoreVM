#pragma

#include <vector>
#include <cstdint>

/*
 * In GCVM, graphs are static and are stored
 * in compressed sparse row (CSR) format.
 */
struct Graph {
    std::vector<uint32_t> row_offsets;
    std::vector<uint32_t> col_indices;

    // Optional
    std::vector<float>  edge_weights;
};