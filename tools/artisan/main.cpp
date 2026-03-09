#include "commands/command.h"
#include "commands/make_opcode.h"
#include "commands/build_execute.h"
#include "commands/execute.h"

#include <iostream>
#include <memory>
#include <vector>
#include <map>
#include <filesystem>

namespace fs = std::filesystem;

void print_help(const std::map<std::string, std::unique_ptr<artisan::Command>>& commands)
{
    std::cout << "Usage: artisan \033[33m<command>\033[0m [arguments]\n\n";
    std::cout << "Available commands:\n";

    for (const auto& [name, cmd] : commands)
    {
        std::cout << "  \033[32m" << name << "\033[0m\n";
        std::cout << "    " << cmd->description() << "\n\n";
    }

    std::cout << "  \033[32mhelp\033[0m\n";
    std::cout << "    Display this help message\n\n";

    std::cout << "For command usage: artisan \033[33m<command>\033[0m --help\n";
    std::cout.flush();
}

int main(int argc, char** argv)
{
    fs::path exe_path;
    try {
        exe_path = fs::absolute(argv[0]);
    } catch (...) {
        exe_path = argv[0];
    }

    fs::path project_root = exe_path.parent_path();

    std::map<std::string, std::unique_ptr<artisan::Command>> commands;
    commands["make:opcode"] = std::make_unique<artisan::MakeOpcodeCommand>(project_root);
    commands["build:execute"] = std::make_unique<artisan::BuildExecuteCommand>(project_root);
    commands["execute"] = std::make_unique<artisan::ExecuteCommand>(project_root);

    if (argc < 2)
    {
        print_help(commands);
        return 1;
    }

    std::string command_name = argv[1];

    if (command_name == "help" || command_name == "--help" || command_name == "-h")
    {
        print_help(commands);
        return 0;
    }

    auto it = commands.find(command_name);
    if (it == commands.end())
    {
        std::cerr << "\033[1;31m[ERROR]\033[0m Unknown command '" << command_name << "'\n\n";
        std::cerr << "Run '\033[32martisan help\033[0m' to see available commands.\n";
        std::cerr.flush();
        return 1;
    }

    if (argc > 2 && (std::string(argv[2]) == "--help" || std::string(argv[2]) == "-h"))
    {
        std::cout << it->second->usage() << "\n";
        std::cout.flush();
        return 0;
    }

    std::vector<std::string> args;
    for (int i = 2; i < argc; ++i)
    {
        args.push_back(argv[i]);
    }

    try
    {
        int ret = it->second->execute(args);
        std::cout.flush();
        std::cerr.flush();
        return ret;
    }
    catch (const std::exception& e)
    {
        std::cerr << "\033[1;31m[ERROR]\033[0m " << e.what() << "\n";
        std::cerr.flush();
        return 1;
    }
}
