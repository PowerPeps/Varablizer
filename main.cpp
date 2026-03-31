#include <iostream>

#include "src/execute/assembly.h"
#include "src/execute/loader.h"

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cerr << "usage: varablizer.exe <program.vbin>\n";
        return 1;
    }

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

    if (vm.stack_size() > 0)
        std::cout << vm.top() << '\n';
}
