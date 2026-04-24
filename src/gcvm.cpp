#include <string>
#include <unordered_map>
#include <cstdint>
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <unordered_set>
#include <chrono>

#include "assembler.h"
#include "program.h"
#include "graph.h"
#include "graphcore.h"

/* Dispatch functions for running gcvm runtime. */
void print_help();
int cmd_assemble(int argc, char **argv);
int cmd_graph(int argc, char **argv);
int cmd_run(int argc, char **argv);
int cmd_shell(int argc, char **argv);

int main(int argc, char **argv) {
    if(argc < 2) {
        print_help();
        return -1;
    }

    std::string cmd = argv[1];

    if(cmd == "-h") { print_help(); return 0; }
    if(cmd == "assemble") return cmd_assemble(argc-1, argv+1);
    if(cmd == "graph") return cmd_graph(argc-1, argv+1);
    if(cmd == "run") return cmd_run(argc-1, argv+1);
    if(cmd == "shell") return cmd_shell(argc-1, argv+1);

    std::cerr << "Unknown command: '" << cmd << "'" << std::endl;
    return -1; 
}

/*
 * Print a usage message for the user.
 */
void print_help() {
    std::cout << "GraphCore Virtual Machine:" << std::endl;
    std::cout << "A graph computation system for the Gather-Apply-Scatter paradigm."    << std::endl;
    std::cout << "================================================================="    << std::endl;
    std::cout << "To use GCVM, use the commands to compile and run programs on the"     << std::endl;
    std::cout << "runtime. The typical flow is to compile a program to bytecode,"       << std::endl;
    std::cout << "compile a graph file to bytecode, then run the compiled program"      << std::endl;
    std::cout << "on the VM."                                                           << std::endl;
    std::cout << "================================================================="    << std::endl;
    std::cout << "Commands:"                                                            << std::endl;
    std::cout << "\t- gcvm assemble [:options] {input_file}"                            << std::endl;
    std::cout << "\t- gcvm graph [:options] {input_file}"                               << std::endl;
    std::cout << "\t- gcvm run [:options]"                                              << std::endl;
    std::cout << "\t- gcvm shell"                                                       << std::endl;
    std::cout << "================================================================="    << std::endl;
    std::cout << "Assemble:"                                                            << std::endl;
    std::cout << "**Compile a plaintext assembly file**"                                << std::endl;
    std::cout << "\t- input_file - Plaintext assembly file"                             << std::endl;
    std::cout << "\t- -o option - Output file name (default: ./a.bc)"                   << std::endl;
    std::cout << "================================================================="    << std::endl;
    std::cout << "Graph:"                                                               << std::endl;
    std::cout << "**Compile a plaintext graph file**"                                   << std::endl;
    std::cout << "\t- input_file - Plaintext graph file"                                << std::endl;
    std::cout << "\t- -o option - Output file name (default: ./a.g)"                    << std::endl;
    std::cout << "================================================================="    << std::endl;
    std::cout << "Run:"                                                                 << std::endl;
    std::cout << "**Execute a graph kernel**"                                           << std::endl;
    std::cout << "\t- -b option - Input (compiled) bytecode file"                       << std::endl;
    std::cout << "\t- -g option - Input (compiled) graph file"                          << std::endl;
    std::cout << "\t- -c option - Input YAML configuration file"                        << std::endl;
    std::cout << "\t- -o option - Output file (default: stdout)"                        << std::endl;
    std::cout << "================================================================="    << std::endl;
    std::cout << "Shell:"                                                               << std::endl;
    std::cout << "\t**Run an interactive shell**"                                       << std::endl;
    std::cout << "================================================================="    << std::endl;
}

/*
 * Run the 'assemble' command.
 * Arguments:
 *     int argc - Number of arguments.
 *     char **argv - Arguments.
 * Return:
 *     int - 0 if successful, -1 if failure.
 */
int cmd_assemble(int argc, char **argv) {
    std::string input_file;
    std::string output_file;

    for(int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if(arg == "-h") {
            print_help();
            return 0;
        } else if(arg == "-o") {
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
        output_file = "./a.bc";
    }

    std::vector<std::string> lines;
    std::ifstream file(input_file);

    // Check if the file opened successfully
    if(!file.is_open()) {
        std::cerr << "Could not open " << input_file << std::endl;
        return -1;
    }

    // Copy file line by line
    std::string line;
    while(std::getline(file, line)) {
        lines.push_back(line);
    }

    file.close();

    // Assemble the program
    Assembler assmblr;
    Program program = assmblr.assemble(lines);

    std::ofstream outFile(output_file, std::ios::binary);

    // Check if the file opened successfully
    if(!outFile.is_open()) {
        std::cerr << "Could not open " << output_file << std::endl;
        return -1;
    }

    int16_t constant_count = static_cast<int16_t>(program.constants.size());
    outFile.write(reinterpret_cast<char *>(&constant_count), sizeof(constant_count));
    for(double &constant : program.constants) {
        outFile.write(reinterpret_cast<char *>(&constant), sizeof(constant));
    }

    // Write the assembled program to the file
    for(Instruction &inst : program.code) {
        outFile.write(reinterpret_cast<char *>(&inst.raw), sizeof(inst.raw));
    }

    outFile.close();

    return 0;
}

/*
 * Parse a line from a plaintext graph file.
 * Arguments:
 *     const std::string& line - Line to parse.
 * Returns:
 *     Edge - A graph edge to insert into a graph.
 */
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

/*
 * Run the 'graph' command.
 * Arguments:
 *     int argc - Number of arguments.
 *     char **argv - Arguments.
 * Returns:
 *     int - 0 on success, -1 on failure.
 */
int cmd_graph(int argc, char **argv) {
    std::string input_file;
    std::string output_file;

    for(int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if(arg == "-h") {
            print_help();
            return 0;
        } else if(arg == "-o") {
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
        output_file = "./a.g";
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

/*
 * Configuration parameters for running
 * a GCVM environment.
 */
struct VMConfig {
    double default_seed = 0.0;
    std::unordered_map<uint32_t, double> seed_overrides;

    bool default_active = true;
    std::unordered_map<uint32_t, bool> active_overrides;

    bool jit = false;
};

/*
 * Load a configuration file.
 * Arguments:
 *     const std::string &path - Path to the YAML config file.
 * Returns:
 *     VMConfig - Configuration for GCVM instance.
 */
VMConfig load_config(const std::string &path) {
    YAML::Node config = YAML::LoadFile(path);

    VMConfig cfg;

    // If seed is defined, configure seeds
    if(config["seed"]) {
        // Set default seed
        if(config["seed"]["default"]) {
            if(config["seed"]["default"].as<std::string>() == "inf") {
                cfg.default_seed = std::numeric_limits<double>::infinity();
            } else if(config["seed"]["default"].as<std::string>() == "-inf") {
                cfg.default_seed = -std::numeric_limits<double>::infinity();
            } else {
                cfg.default_seed = config["seed"]["default"].as<double>();
            }
        }

        // Set seed overrides
        if(config["seed"]["overrides"]) {
            for(auto it : config["seed"]["overrides"]) {
                uint32_t node = it.first.as<uint32_t>();
                double val = 0.0;
                if(it.second.as<std::string>() == "inf") {
                    val = std::numeric_limits<double>::infinity();
                } else if(it.second.as<std::string>() == "-inf") {
                    val = -std::numeric_limits<double>::infinity();
                } else {
                    val = it.second.as<double>();
                }
                cfg.seed_overrides[node] = val;
            }
        }
    }

    // If active is defined, configure active flags
    if(config["active"]) {
        // Set default active state
        if(config["active"]["default"])
            cfg.default_active = config["active"]["default"].as<bool>();

        // Set active overrides
        if(config["active"]["overrides"]) {
            for(auto it : config["active"]["overrides"]) {
                uint32_t node = it.first.as<uint32_t>();
                bool val = it.second.as<bool>();
                cfg.active_overrides[node] = val;
            }
        }
    }

    // If execution is defined, configure execution type
    if(config["execution"]) {
        if(config["execution"]["jit"])
            cfg.jit = config["execution"]["jit"].as<bool>();
    }

    return cfg;
}

/*
 * Run the 'run' command.
 * Arguments:
 *     int argc - Number of arguments.
 *     char **argv - Arguments.
 * Returns:
 *     int - 0 on success, -1 on failure.
 */
int cmd_run(int argc, char **argv) {
    std::string bytecode_file;
    std::string graph_file;
    std::string config_file;
    std::string output_file;

    for(int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if(arg == "-h") {
            print_help();
            return 0;
        } else if(arg == "-b") {
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
        } else if(arg == "-c") {
            if(i + 1 < argc) {
                config_file = argv[++i];
            } else {
                std::cerr << "Error: -c requires an argument\n";
                return -1;
            }
        } else if(arg == "-o") {
            if(i + 1 < argc) {
                output_file = argv[++i];
            } else {
                std::cerr << "Error: -o requires an argument\n";
                return -1;
            }
        } else {
            std::cerr << "Error: invalid option: " << arg << std::endl;
            return -1;
        }
    }

    if(bytecode_file.empty()) {
        std::cerr << "Error: bytecode file not specified\n";
        return -1;
    }

    if(graph_file.empty()) {
        std::cerr << "Error: graph file is not specified\n";
        return -1;
    }

    VMConfig cfg;
    if(!config_file.empty()) {
        load_config(config_file);
    }

    // Create the program
    Program program;
    program.from_file(bytecode_file);

    // Build the graph
    Graph graph;
    graph.build_from_file(graph_file);

    // Create and initialize the runtime
    GCVM runtime;
    runtime.load_graph(graph);
    runtime.load_program(program);
    
    // Set cofiguration parameters
    for(uint32_t v = 0; v < graph.size(); v++) {
        if(cfg.active_overrides.find(v) == cfg.active_overrides.end()) {
            runtime.set_active(v, cfg.default_active);
        } else {
            runtime.set_active(v, cfg.active_overrides[v]);
        }

        if(cfg.seed_overrides.find(v) == cfg.seed_overrides.end()) {
            runtime.set_seed_self(v, cfg.default_seed);
        } else {
            runtime.set_seed_self(v, cfg.seed_overrides[v]);
        }
    }

    // Run the kernel
    runtime.run(cfg.jit);

    auto results = runtime.get_results();
    if(output_file.empty()) {
        std::cout << "Results:" << std::endl;
        std::cout << "============================" << std::endl;
        for(uint32_t v = 0; v < results.size(); v++) {
            std::cout << "Node " << v << ": " << results[v] << std::endl;
        }
        std::cout << "============================" << std::endl;
    } else {
        std::ofstream file(output_file);
        if(file.is_open()) {
            file << "Results:" << std::endl;
            file << "============================" << std::endl;
            for(uint32_t v = 0; v < results.size(); v++) {
                file << "Node " << v << ": " << results[v] << std::endl;
            }
            file << "============================" << std::endl;
        }
    }

    return 0;
}

/*
 * Statistics of the run of a GCVM instance.
 */
struct VMStats {
    int64_t runtime = 0;
};

/*
 * Run an interactive shell on the VM.
 * Arguments:
 *     int argc - Number of arguments.
 *     char **argv - Arguments.
 * Returns:
 *     int - 0 on success, -1 on failure.
 */
int cmd_shell(int argc, char **argv) {
    for(int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if(arg == "-h") {
            print_help();
            return 0;
        } else {
            std::cerr << "Error: invalid option: " << arg << std::endl;
            return -1;
        }
    }

    std::string line;

    Program program;
    Graph graph;
    VMConfig config;
    VMStats stats;
    GCVM runtime;

    while(true) {
        std::cout << "gcvm> ";
        if(!std::getline(std::cin, line)) break;

        // Parse the shell commands
        std::stringstream ss(line);
        std::string cmd;
        ss >> cmd;

        if(cmd == "exit") break;
        else if(cmd == "load") {
            std::string type, file;
            ss >> type >> file;

            if(type == "program") {
                program.from_file(file);
                runtime.load_program(program);
                std::cout << "Program loaded" << std::endl;
            } else if(type == "graph") {
                graph.build_from_file(file);
                runtime.load_graph(graph);
                std::cout << "Graph loaded" << std::endl;
            } else if(type == "config") {
                config = load_config(file);
                std::cout << "Config loaded" << std::endl;
            }
        } else if(cmd == "run") {
            // Set cofiguration parameters
            for(uint32_t v = 0; v < graph.size(); v++) {
                if(config.active_overrides.find(v) == config.active_overrides.end()) {
                    runtime.set_active(v, config.default_active);
                } else {
                    runtime.set_active(v, config.active_overrides[v]);
                }

                if(config.seed_overrides.find(v) == config.seed_overrides.end()) {
                    runtime.set_seed_self(v, config.default_seed);
                } else {
                    runtime.set_seed_self(v, config.seed_overrides[v]);
                }
            }

            // Run the executable
            auto start = std::chrono::high_resolution_clock::now();
            runtime.run(config.jit);
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            std::cout << "Done" << std::endl;
            stats.runtime = duration.count();
        } else if(cmd == "print") {
            std::string sub;
            ss >> sub;

            if(sub == "node") {
                uint32_t id;
                ss >> id;
                auto res = runtime.get_results();
                std::cout << "Node " << id << ": " << res[id] << std::endl;
            }
        } else if(cmd == "stats") {
            std::cout << "Stats:" << std::endl;
            std::cout << "================================\n";
            std::cout << "Last runtime: " << stats.runtime << " microseconds\n";
            std::cout << "================================\n";
        } else if(cmd == "debug") {
            std::cout << "Debugger not implemented yet" << std::endl;
        } else if(cmd == "help") {
            std::cout << "Commands:" << std::endl;
            std::cout << "\texit" << std::endl;
            std::cout << "\tload [program|graph|config] {input_file}" << std::endl;
            std::cout << "\trun" << std::endl;
            std::cout << "\tprint [node] {node_id}" << std::endl;
            std::cout << "\tstats" << std::endl;
            std::cout << "\tdebug" << std::endl;
            std::cout << "**Note: stats and debug are not fully implemented yet**" << std::endl;
        } else {
            std::cout << "Unknown command\n";
        }
    }

    return 0;
}