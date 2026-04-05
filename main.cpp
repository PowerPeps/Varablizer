#include <iostream>
#include <string_view>
#include <filesystem>

#include "src/execute/assembly.h"
#include "src/execute/loader.h"
#include "src/execute/assembler.h"

namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "usage: varablizer.exe <program.vbin>\n";
        std::cerr << "       varablizer.exe --compile   <program.txt>  [out.vbin]\n";
        std::cerr << "       varablizer.exe --decompile <program.vbin> [out.txt]\n";
        return 1;
    }

    const std::string_view first(argv[1]);

    // ── translate: txt → vbin ──────────────────────────────────────────────
    if (first == "--compile")
    {
        if (argc < 3) { std::cerr << "error: missing input file\n"; return 1; }
        const fs::path input(argv[2]);
        const fs::path output = (argc >= 4)
            ? fs::path(argv[3])
            : input.parent_path() / (input.stem().string() + ".vbin");

        execute::write_binary(execute::assemble(input.string()), output.string());
        return 0;
    }

    // ── translate: vbin → txt ──────────────────────────────────────────────
    if (first == "--decompile")
    {
        if (argc < 3) { std::cerr << "error: missing input file\n"; return 1; }
        const fs::path input(argv[2]);
        const fs::path output = (argc >= 4)
            ? fs::path(argv[3])
            : input.parent_path() / (input.stem().string() + ".txt");

        execute::decompile(execute::load_binary(input.string()), output.string());
        return 0;
    }

    // ── auto-detect by extension ───────────────────────────────────────────
    const fs::path input(argv[1]);
    const std::string ext = input.extension().string();

    if (ext == ".txt")
    {
        const fs::path output = input.parent_path() / (input.stem().string() + ".vbin");
        execute::write_binary(execute::assemble(input.string()), output.string());
        return 0;
    }

    if (ext == ".vbin")
    {
        auto prog = execute::load_binary_mmap(argv[1]);
#ifdef DEBUG
        std::cerr << "loaded " << prog.size() << " instructions\n";
#endif
        execute::assembly vm;
        vm.load(std::move(prog));
        vm.run();
#ifdef DEBUG
        std::cerr << "stack_size=" << vm.stack_size() << " halted=" << vm.is_halted() << '\n';
#endif
        std::cout << vm.top() << '\n';
        return 0;
    }

    std::cerr << "error: unknown extension '" << ext << "' (expected .txt or .vbin)\n";
    return 1;
}