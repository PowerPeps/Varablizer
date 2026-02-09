cmake_minimum_required(VERSION 3.15)

add_executable(artisan main.cpp)

target_compile_features(artisan PRIVATE cxx_std_20)

# Force console subsystem on Windows + static linking
if(WIN32)
    if(MINGW)
        target_link_options(artisan PRIVATE -mconsole -static-libgcc -static-libstdc++ -static)
    elseif(MSVC)
        set_target_properties(artisan PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
    endif()
endif()

# Copy executable to project root after build
add_custom_command(TARGET artisan POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:artisan>
        "${CMAKE_SOURCE_DIR}/artisan${CMAKE_EXECUTABLE_SUFFIX}"
    COMMENT "Copying artisan to project root..."
)

# Link filesystem library if needed (GCC < 9, Clang < 9)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
        target_link_libraries(artisan PRIVATE stdc++fs)
    endif()
endif()
