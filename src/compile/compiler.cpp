#include "compiler.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "emitter.h"
#include "error.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace compile
{

execute::program Compiler::compile(const std::string& source,
                                   const std::string& filename)
{
    Lexer  lexer(source, filename);
    Parser parser(lexer);

    auto program_ast = parser.parse();

    CodeGen codegen;
    return codegen.compile(*program_ast);
}

void Compiler::compile_file(const std::string& input_path,
                             const std::string& output_path)
{
    // Read source file
    std::ifstream in(input_path);
    if (!in)
        throw std::runtime_error("cannot open source file: " + input_path);

    std::ostringstream buf;
    buf << in.rdbuf();
    const std::string source = buf.str();

    // Compile
    execute::program prog = compile(source, input_path);

    // Write binary
    emit_binary(prog, output_path);
}

} // namespace compile
