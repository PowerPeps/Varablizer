#ifndef ARTISAN_BUILD_EXECUTE_H
#define ARTISAN_BUILD_EXECUTE_H

#include "command.h"
#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <utility>

namespace fs = std::filesystem;

namespace artisan
{
    class BuildExecuteCommand : public Command
    {
    public:
        explicit BuildExecuteCommand(fs::path  project_root)
            : project_root_(std::move(project_root))
        {}

        [[nodiscard]] std::string name() const override
        {
            return "build:execute";
        }

        [[nodiscard]] std::string description() const override
        {
            return "Build the execute engine (varablizer executable)";
        }

        [[nodiscard]] std::string usage() const override
        {
            return "artisan build:execute [--clean]\n"
                   "  --clean  : Clean build directory before building";
        }

        int execute(const std::vector<std::string>& args) override
        {
            bool clean = false;

            for (const auto& arg : args)
            {
                if (arg == "--clean")
                    clean = true;
                else
                {
                    std::cerr << "\033[1;31m[ERROR]\033[0m Unknown option '" << arg << "'\n";
                    return 1;
                }
            }

            fs::path build_dir = project_root_ / "cmake-build-debug";

            if (clean)
            {
                std::cout << "\033[1;33m[CLEAN]\033[0m Cleaning build directory...\n";
                if (fs::exists(build_dir))
                {
                    fs::remove_all(build_dir);
                }
                fs::create_directories(build_dir);
            }

            if (!fs::exists(build_dir / "CMakeCache.txt"))
            {
                std::cout << "\033[1;36m[CONFIG]\033[0m Configuring project...\n";
                std::string cmd = "cmake -S \"" + project_root_.string() + "\" -B \"" + build_dir.string() + "\"";
                int ret = std::system(cmd.c_str());
                if (ret != 0)
                {
                    std::cerr << "\033[1;31m[ERROR]\033[0m CMake configuration failed\n";
                    return ret;
                }
            }

            std::cout << "\033[1;34m[BUILD]\033[0m Building varablizer...\n";
            std::string build_cmd = "cmake --build \"" + build_dir.string() + "\" --target varablizer";
            int ret = std::system(build_cmd.c_str());

            if (ret == 0)
            {
                std::cout << "\033[1;32m[OK]\033[0m Build successful!\n";
                std::cout << "   Executable: " << (build_dir / "varablizer.exe").string() << "\n";
            }
            else
            {
                std::cerr << "\033[1;31m[ERROR]\033[0m Build failed\n";
            }

            return ret;
        }

    private:
        fs::path project_root_;
    };

} // namespace artisan

#endif // ARTISAN_BUILD_EXECUTE_H
