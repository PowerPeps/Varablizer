cmake_minimum_required(VERSION 4.0)
project(varablizer)

set(CMAKE_CXX_STANDARD 20)

# Build code generation tools
add_subdirectory(tools/opcode_generator)
add_subdirectory(tools/artisan)

# Define paths
set(OPCODES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/execute/opcodes")
set(GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/execute/generated")

# List of opcode header files
set(OPCODE_HEADERS
    "${OPCODES_DIR}/control.h"
    "${OPCODES_DIR}/stack.h"
    "${OPCODES_DIR}/math.h"
    "${OPCODES_DIR}/debug.h"
    "${OPCODES_DIR}/flow.h"
    "${OPCODES_DIR}/io.h"
    "${OPCODES_DIR}/locals.h"
    "${OPCODES_DIR}/heap.h"
    "${OPCODES_DIR}/call.h"
    "${OPCODES_DIR}/bitwise.h"
    "${OPCODES_DIR}/compare.h"
)

# Generated files
set(GENERATED_FILES
    "${GENERATED_DIR}/opcode_enum.inl"
    "${GENERATED_DIR}/handlers_table.inl"
    "${GENERATED_DIR}/disassembler.h"
    "${GENERATED_DIR}/disassembler.cpp"
    "${GENERATED_DIR}/OPCODES.md"
)

# Custom target to always regenerate opcode tables before building varablizer
add_custom_target(generate_opcodes ALL
    COMMAND opcode_generator "${OPCODES_DIR}" "${GENERATED_DIR}"
    DEPENDS opcode_generator ${OPCODE_HEADERS}
    COMMENT "Generating opcode tables and disassembler..."
    VERBATIM
)

add_executable(varablizer main.cpp
        src/execute/assembly.h
        ${GENERATED_FILES}  # Include generated files in project
)

# Ensure opcodes are generated before building varablizer
add_dependencies(varablizer generate_opcodes)

# Static linking for standalone executable
if(WIN32)
    if(MINGW)
        target_link_options(varablizer PRIVATE -static-libgcc -static-libstdc++ -static)
    endif()
endif()
