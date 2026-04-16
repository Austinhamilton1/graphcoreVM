#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <unistd.h>
#include <getopt.h>

#include "assembler.h"
#include "program.h"

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

    // Write the assembled program to the file
    for(Instruction &inst : program.code) {
        outFile.write(reinterpret_cast<char *>(&inst.raw), sizeof(inst.raw));
    }

    outFile.close();

    return 0;
}