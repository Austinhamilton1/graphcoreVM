#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <unistd.h>
#include <getopt.h>
#include <chrono>

#include "program.h"
#include "graph.h"
#include "graphcore.h"

int main(int argc, char **argv) {
    if(argc < 7 || argc > 10) {
        std::cerr << "Program usage: " << argv[0] << " -b [bytecode file] -g [graph file] -s [seed value] -o [output file] -j\n";
        return -1;
    }

    std::string bytecode_file;
    std::string graph_file;
    std::string output_file;
    double seed_value = 0.0;
    bool compile = false;

    // Parse arguments
    for(int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if(arg == "-b") {
            if(i + 1 < argc) {
                bytecode_file = argv[++i];
            } else {
                std::cerr << "Error: -b requires an argument\n";
                return -1;
            }
        } else if(arg == "-g") {
            if(i + 1 < argc) {
                graph_file = argv[++i];
            } else {
                std::cerr << "Error: -g requires an argument\n";
                return -1;
            }
        } else if(arg == "-s") {
            if(i + 1 < argc) {
                seed_value = std::stod(argv[++i]);
            } else {
                std::cerr << "Error: -s requires an argument\n";
                return -1;
            }
        } else if(arg == "-o") {
            if(i + 1 < argc) {
                output_file = argv[++i];
            } else {
                std::cerr << "Error: -o requires an argument\n";
                return -1;
            }
        } else if(arg == "-j") {
            compile = true;
        }
    }

    if(bytecode_file.empty()) {
        std::cerr << "Error: bytecode file not specified\n";
        return -1;
    }

    if(graph_file.empty()) {
        std::cerr << "Error: graph file not specified\n";
        return -1;
    }

    Program program;
    Graph graph;

    program.from_file(bytecode_file);
    graph.build_from_file(graph_file);

    GCVM runtime;
    runtime.load_graph(graph);
    runtime.set_seed_self(seed_value);
    runtime.set_seed_vertices(true);
    runtime.load_program(program, false);

    auto start = std::chrono::high_resolution_clock::now();
    runtime.run(compile);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    auto results = runtime.get_results();

    if(output_file.empty()) {
        std::cout << "Results:\n";
        std::cout << "========================\n";
        for(uint32_t v = 0; v < results.size(); v++) {
            std::cout << "Node " << v << ": " << results[v] << std::endl;
        }
        std::cout << "========================\n";
    } else {
        std::ofstream file(output_file);
        file << "Results:\n";
        file << "========================\n";
        for(uint32_t v = 0; v < results.size(); v++) {
            file << "Node " << v << ": " << results[v] << std::endl;
        }
        file << "========================\n";
    }

    std::cout << "Execution time: " << duration.count() << " microseconds\n";

    return 0;
}