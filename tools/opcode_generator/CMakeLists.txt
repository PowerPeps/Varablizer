cmake_minimum_required(VERSION 3.15)

add_executable(opcode_generator main.cpp)

target_compile_features(opcode_generator PRIVATE cxx_std_20)

# Link filesystem library if needed (GCC < 9, Clang < 9)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
        target_link_libraries(opcode_generator PRIVATE stdc++fs)
    endif()
endif()
