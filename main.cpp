#include <iostream>

#include "src/execute/assembly.h"
using namespace std;

int main()
{
    const execute::program prog = {
        { execute::opcode::PUSH, 12 },
        { execute::opcode::PUSH, 164 },
        { execute::opcode::DD},
        { execute::opcode::MUL },
        { execute::opcode::DD},
        { execute::opcode::PUSH, 24 },
        { execute::opcode::DD},
        { execute::opcode::DIV },
        { execute::opcode::HALT },
    };


    execute::assembly vm;
    vm.load(prog);
    vm.run();

    std::cout << vm.to_string();

}