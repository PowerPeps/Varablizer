#ifndef ARTISAN_EXECUTE_H
#define ARTISAN_EXECUTE_H

#include "command.h"
#include <filesystem>
#include <iostream>
#include <utility>

namespace fs = std::filesystem;

namespace artisan
{
    class ExecuteCommand : public Command
    {
    public:
        explicit ExecuteCommand(fs::path  project_root)
            : project_root_(std::move(project_root))
        {}

        [[nodiscard]] std::string name() const override
        {
            return "execute";
        }

        [[nodiscard]] std::string description() const override
        {
            return "Execute a compiled .vbin file";
        }

        [[nodiscard]] std::string usage() const override
        {
            return "artisan execute <file.vbin>\n"
                   "  file.vbin : Path to the binary file to execute";
        }

        int execute(const std::vector<std::string>& args) override
        {
            if (args.empty())
            {
                std::cerr << "\033[1;31m[ERROR]\033[0m Missing file path\n\n";
                std::cerr << "Usage: " << usage() << "\n";
                return 1;
            }

            if (args.size() > 1)
            {
                std::cerr << "\033[1;31m[ERROR]\033[0m Too many arguments\n\n";
                std::cerr << "Usage: " << usage() << "\n";
                return 1;
            }


            std::string file_arg = args[0];
            if (file_arg.size() >= 2 && file_arg.front() == '"' && file_arg.back() == '"')
            {
                file_arg = file_arg.substr(1, file_arg.size() - 2);
            }


            fs::path vbin_file = fs::absolute(file_arg);


            if (!fs::exists(vbin_file))
            {
                std::cerr << "\033[1;31m[ERROR]\033[0m File not found: " << vbin_file << "\n";
                std::cerr << "   Current directory: " << fs::current_path() << "\n";
                return 1;
            }


            fs::path varablizer_exe;
            std::vector<fs::path> possible_paths = {
                project_root_ / "cmake-build-debug" / "varablizer.exe",
                project_root_ / "build" / "varablizer.exe",
                project_root_ / "cmake-build-release" / "varablizer.exe"
            };

            for (const auto& path : possible_paths)
            {
                if (fs::exists(path))
                {
                    varablizer_exe = path;
                    break;
                }
            }

            if (varablizer_exe.empty())
            {
                std::cerr << "\033[1;31m[ERROR]\033[0m varablizer.exe not found\n";
                std::cerr << "   Searched in:\n";
                for (const auto& path : possible_paths)
                {
                    std::cerr << "   - " << path << "\n";
                }
                std::cerr << "   Build the project first\n";
                return 1;
            }


            std::cout << "\033[1;35m[RUN]\033[0m Executing: " << vbin_file.filename() << "\n";
            std::cout << "==========================================\n";


            const std::string cmd = "\"" + varablizer_exe.generic_string() + "\""
                                  + " \"" + vbin_file.generic_string() + "\"";

            const int ret = std::system(cmd.c_str());

            std::cout << "==========================================\n";

            if (ret == 0)
            {
                std::cout << "\033[1;32m[OK]\033[0m Execution completed successfully\n";
            }
            else
            {
                std::cerr << "\033[1;31mExecution failed with code " << ret << "\033[0m\n";
            }

            return ret;
        }

    private:
        fs::path project_root_;
    };

} // namespace artisan

#endif // ARTISAN_EXECUTE_H