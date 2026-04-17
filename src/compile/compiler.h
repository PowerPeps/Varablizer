#ifndef VARABLIZER_COMPILER_H
#define VARABLIZER_COMPILER_H

#include "../execute/opcodes.h"
#include <string>

namespace compile
{
    class Compiler
    {
    public:
        // Compile source string → program (vector<instruction>)
        [[nodiscard]] execute::program compile(const std::string& source,
                                               const std::string& filename = "<input>");

        // Compile source file → write .vbin
        void compile_file(const std::string& input_path,
                          const std::string& output_path);
    };

} // namespace compile

#endif // VARABLIZER_COMPILER_H
