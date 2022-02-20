#include "lexer.h"
#include "parse.h"
#include "runtime.h"
#include "statement.h"
#include <iostream>
#include <fstream>

void RunMythonProgram(std::istream& input, std::ostream& output) {
    parse::Lexer lexer(input);
    auto program = ParseProgram(lexer);

    runtime::SimpleContext context{output};
    runtime::Closure closure;
    program->Execute(closure, context);
}

void PrintUsage() {
    std::cerr << "Usage : mython <input_file> <output_file> \n";
}

int main(int argc, const char** argv) {
    if(argc != 3){
        PrintUsage();
        return 1;
    }

    std::ifstream input_file(argv[1]);
    if (!input_file.is_open())
    {
        std::cerr << "Failed to open input file: " << argv[1]  << std::endl;
        return 2;
    }
    std::ofstream output_file(argv[2]);
    if (!output_file.is_open())
    {
        std::cerr << "Failed to open output_file: " << argv[2]  << std::endl;
        return 2;
    }
    RunMythonProgram(input_file, output_file);
    return 0;
}
