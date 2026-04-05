#ifndef ARTISAN_COMPILE_H
#define ARTISAN_COMPILE_H

#include "command.h"
#include <filesystem>
#include <iostream>
#include <utility>

namespace fs = std::filesystem;

namespace artisan
{
    class CompileCommand : public Command
    {
    public:
        explicit CompileCommand(fs::path project_root)
            : project_root_(std::move(project_root))
        {}

        [[nodiscard]] std::string name() const override
        {
            return "translate";
        }

        [[nodiscard]] std::string description() const override
        {
            return "Translate between .txt (source) and .vbin (binary) — direction is auto-detected";
        }

        [[nodiscard]] std::string usage() const override
        {
            return "artisan translate <file.txt|file.vbin> [output]\n"
                   "  file.txt  : Assemble source into a .vbin binary\n"
                   "  file.vbin : Decompile binary back into a .txt source\n"
                   "  output    : Optional output path (default: same stem, swapped extension)";
        }

        int execute(const std::vector<std::string>& args) override
        {
            if (args.empty())
            {
                std::cerr << "\033[1;31m[ERROR]\033[0m Missing file path\n\n";
                std::cerr << "Usage: " << usage() << "\n";
                return 1;
            }

            if (args.size() > 2)
            {
                std::cerr << "\033[1;31m[ERROR]\033[0m Too many arguments\n\n";
                std::cerr << "Usage: " << usage() << "\n";
                return 1;
            }

            std::string file_arg = args[0];
            if (file_arg.size() >= 2 && file_arg.front() == '"' && file_arg.back() == '"')
                file_arg = file_arg.substr(1, file_arg.size() - 2);

            const fs::path input = fs::absolute(file_arg);

            if (!fs::exists(input))
            {
                std::cerr << "\033[1;31m[ERROR]\033[0m File not found: " << input << "\n";
                std::cerr << "   Current directory: " << fs::current_path() << "\n";
                return 1;
            }

            const std::string ext = input.extension().string();

            if (ext != ".txt" && ext != ".vbin")
            {
                std::cerr << "\033[1;31m[ERROR]\033[0m Unknown extension '" << ext
                          << "' (expected .txt or .vbin)\n";
                return 1;
            }

            const bool to_vbin = (ext == ".txt");
            const std::string out_ext  = to_vbin ? ".vbin" : ".txt";
            const std::string flag     = to_vbin ? "--compile" : "--decompile";

            const fs::path output = (args.size() == 2)
                ? fs::absolute(args[1])
                : input.parent_path() / (input.stem().string() + out_ext);

            fs::path varablizer_exe;
            const std::vector<fs::path> possible_paths = {
                project_root_ / "cmake-build-debug"   / "varablizer.exe",
                project_root_ / "build"               / "varablizer.exe",
                project_root_ / "cmake-build-release" / "varablizer.exe",
            };

            for (const auto& path : possible_paths)
            {
                if (fs::exists(path)) { varablizer_exe = path; break; }
            }

            if (varablizer_exe.empty())
            {
                std::cerr << "\033[1;31m[ERROR]\033[0m varablizer.exe not found\n";
                std::cerr << "   Searched in:\n";
                for (const auto& path : possible_paths)
                    std::cerr << "   - " << path << "\n";
                std::cerr << "   Build the project first\n";
                return 1;
            }

            std::cout << "\033[1;35m[TRANSLATE]\033[0m "
                      << input.filename().string() << " -> "
                      << output.filename().string() << "\n";

            const std::string cmd = "\"" + varablizer_exe.generic_string() + "\""
                                  + " " + flag
                                  + " \"" + input.generic_string()  + "\""
                                  + " \"" + output.generic_string() + "\"";

            const int ret = std::system(cmd.c_str());

            if (ret == 0)
                std::cout << "\033[1;32m[OK]\033[0m Output: " << output << "\n";
            else
                std::cerr << "\033[1;31m[ERROR]\033[0m Translation failed with code " << ret << "\n";

            return ret;
        }

    private:
        fs::path project_root_;
    };

} // namespace artisan

#endif // ARTISAN_COMPILE_H