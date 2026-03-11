#include "commands/command.h"
#include "commands/make_opcode.h"
#include "commands/build_execute.h"
#include "commands/execute.h"

#include <iostream>
#include <memory>
#include <vector>
#include <map>
#include <filesystem>
#include <termcolor/termcolor.hpp>

namespace fs = std::filesystem;

void print_help(const std::map<std::string, std::unique_ptr<artisan::Command>>& commands)
{
    std::cout << "Usage: artisan " << termcolor::bright_yellow <<  "<command> " << termcolor::reset << "[arguments]\n\n";
    std::cout << "Available commands:\n";

    for (const auto& [name, cmd] : commands)
    {
        std::cout << "  " << termcolor::bright_green << name << termcolor::reset << "\n";
        std::cout << "    " << cmd->description() << "\n\n";
    }

    std::cout << "  " << termcolor::bright_green << "help\n" << termcolor::reset;
    std::cout << "    Display this help message\n\n";

    std::cout << "For command usage: artisan " << termcolor::bright_yellow << "<command>" << termcolor::reset << " --help\n";
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
        std::cerr << termcolor::bright_red << "[ERROR]" << termcolor::reset << " Unknown command '" << command_name << "'\n\n";
        std::cerr << "Run " << termcolor::bright_yellow << "artisan help" << termcolor::reset << " to see available commands.\n";
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
        std::cerr << termcolor::bright_red << "[ERROR] " << termcolor::reset << e.what() << "\n";
        std::cerr.flush();
        return 1;
    }
}
