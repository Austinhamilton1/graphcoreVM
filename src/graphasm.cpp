#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <unistd.h>
#include <getopt.h>
#include <sstream>
#include <unordered_set>

#include "graph.h"

Edge parse_line(const std::string& line) {
    std::vector<std::string> tokens;
    std::stringstream ss(line);
    std::string item;
    while(std::getline(ss, item, ',')) {
        tokens.push_back(item);
    }

    Edge edge;
    
    if(tokens.size() != 2 && tokens.size() != 3)
        throw std::runtime_error("Invalid number of columns");

    edge.src = (uint32_t)std::stoi(tokens[0]);
    edge.dst = (uint32_t)std::stoi(tokens[1]);

    if(tokens.size() == 3)
        edge.weight = (double)std::stod(tokens[2]);

    return edge;
}

int main(int argc, char **argv) {
    if(argc != 4) {
        std::cerr << "Program usage: " << argv[0] << " [input file] -o [output file]\n";
        return -1;
    }

    std::string input_file;
    std::string output_file;

    for(int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if(arg == "-o") {
            if(i + 1 < argc) {
                output_file = argv[++i];
            } else {
                std::cerr << "Error: -o requires an argument\n";
                return -1;
            }
        } else {
            input_file = arg;
        }
    }

    if(input_file.empty()) {
        std::cerr << "Error: input file not specified\n";
        return -1;
    }

    if(output_file.empty()) {
        std::cerr << "Error: output file not specified\n";
        return -1;
    }

    std::vector<Edge> edges;
    std::ifstream file(input_file);

    // Check if the file opened successfully
    if(!file.is_open()) {
        std::cerr << "Could not open " << input_file << std::endl;
        return -1;
    }

    // Count how many nodes are in the graph
    std::unordered_set<uint32_t> unique_vertices;

    // Parse file line by line
    std::string line;
    while(std::getline(file, line)) {
        auto edge = parse_line(line);
        edges.push_back(edge);
        unique_vertices.insert(edge.src);
        unique_vertices.insert(edge.dst);
    }

    file.close();

    std::ofstream outFile(output_file, std::ios::binary);

    // Check if the file opened successfully
    if(!outFile.is_open()) {
        std::cerr << "Could not open " << output_file << std::endl;
        return -1;
    }

    size_t num_vertices = unique_vertices.size();
    outFile.write(reinterpret_cast<char *>(&num_vertices), sizeof(num_vertices));

    // Write the assembled program to the file
    for(Edge &edge : edges) {
        outFile.write(reinterpret_cast<char *>(&edge.src), sizeof(edge.src));
        outFile.write(reinterpret_cast<char *>(&edge.dst), sizeof(edge.dst));
        outFile.write(reinterpret_cast<char *>(&edge.weight), sizeof(edge.weight));
    }

    outFile.close();

    return 0;
}