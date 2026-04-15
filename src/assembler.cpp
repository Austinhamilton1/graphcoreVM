#include <iostream>
#include <vector>
#include <string>
#include <fstream>

#include "assembler.h"
#include "program.h"

int main(int argc, char **argv) {
    if(argc != 3) {
        std::cerr << "Program usage: " << argv[0] << "[input file] [output file]" << std::endl;
        return -1;
    }

    std::vector<std::string> lines;
    std::ifstream file(argv[1]);

    // Check if the file opened successfully
    if(!file.is_open()) {
        std::cerr << "Could not open " << argv[1] << std::endl;
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

    std::ofstream outFile(argv[2], std::ios::binary);

    // Check if the file opened successfully
    if(!outFile.is_open()) {
        std::cerr << "Could not open " << argv[2] << std::endl;
        return -1;
    }

    // Write the assembled program to the file
    for(Instruction &inst : program.code) {
        outFile.write(reinterpret_cast<char *>(&inst.raw), sizeof(inst.raw));
    }

    outFile.close();

    return 0;
}