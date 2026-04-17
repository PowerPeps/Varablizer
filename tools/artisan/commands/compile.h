#pragma once

#include "command.h"
#include "compile/compiler.h"
#include "compile/error.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace artisan
{
    class CompileCommand : public Command
    {
    public:
        explicit CompileCommand(const fs::path& project_root)
            : project_root_(project_root) {}

        [[nodiscard]] std::string name() const override { return "compile"; }

        [[nodiscard]] std::string description() const override
        {
            return "Compile a .vz source file to a .vbin bytecode file";
        }

        [[nodiscard]] std::string usage() const override
        {
            return "artisan compile <file.vz> [-o output.vbin]\n\n"
                   "  <file.vz>          Source file to compile\n"
                   "  -o <output.vbin>   Output path (default: same name with .vbin extension)\n";
        }

        int execute(const std::vector<std::string>& args) override
        {
            if (args.empty())
            {
                std::cerr << "\033[1;31m[ERROR]\033[0m No source file specified.\n";
                std::cerr << usage() << "\n";
                return 1;
            }

            fs::path input_path = args[0];
            if (input_path.is_relative())
                input_path = project_root_ / input_path;

            if (!fs::exists(input_path))
            {
                std::cerr << "\033[1;31m[ERROR]\033[0m File not found: " << input_path << "\n";
                return 1;
            }

            // Output path: -o flag or replace extension with .vbin
            fs::path output_path;
            for (std::size_t i = 1; i < args.size(); ++i)
            {
                if (args[i] == "-o" && i + 1 < args.size())
                {
                    output_path = args[++i];
                    if (output_path.is_relative())
                        output_path = project_root_ / output_path;
                    break;
                }
            }
            if (output_path.empty())
                output_path = input_path.replace_extension(".vbin");

            std::cout << "\033[32m[compile]\033[0m " << input_path.filename().string()
                      << " → " << output_path.filename().string() << "\n";

            try
            {
                compile::Compiler compiler;
                compiler.compile_file(input_path.string(), output_path.string());
                std::cout << "\033[32m[OK]\033[0m Compiled successfully → "
                          << output_path.string() << "\n";
                return 0;
            }
            catch (const compile::CompileError& e)
            {
                std::cerr << "\033[1;31m[COMPILE ERROR]\033[0m " << e.what() << "\n";
                return 1;
            }
            catch (const std::exception& e)
            {
                std::cerr << "\033[1;31m[ERROR]\033[0m " << e.what() << "\n";
                return 1;
            }
        }

    private:
        fs::path project_root_;
    };

} // namespace artisan
