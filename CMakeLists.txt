cmake_minimum_required(VERSION 4.0)
project(varablizer)

set(CMAKE_CXX_STANDARD 20)

add_executable(varablizer main.cpp
        src/compile/Lexer.cpp
        src/compile/Lexer.h
        src/compile/Lexer.cpp
        src/execute/assembly.h)
