#ifndef VARABLIZER_LOADER_H
#define VARABLIZER_LOADER_H

#include "opcodes.h"

#include <fstream>
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <sys/stat.h>
    #include <fcntl.h>
    #include <unistd.h>
#endif

namespace execute
{
    static constexpr std::size_t instruction_size = sizeof(std::uint8_t) + sizeof(value_t);

    // Fast loader using memory-mapped I/O when possible
    [[nodiscard]] inline program load_binary(const char* path)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file)
        {
            throw std::runtime_error("cannot open file");
        }

        const auto file_size = static_cast<std::size_t>(file.tellg());
        if (file_size == 0 || file_size % instruction_size != 0)
        {
            throw std::runtime_error("invalid file size");
        }

        file.seekg(0, std::ios::beg);

        const std::size_t count = file_size / instruction_size;
        program prog;
        prog.reserve(count);

        std::array<std::uint8_t, instruction_size> buffer{};

        for (std::size_t i = 0; i < count; ++i)
        {
            file.read(reinterpret_cast<char*>(buffer.data()), instruction_size);

            instruction instr;
            instr.op = static_cast<opcode>(buffer[0]);

            // Little-endian
            std::memcpy(&instr.operand, buffer.data() + 1, sizeof(value_t));

            prog.push_back(instr);
        }

        return prog;
    }

    // Overload std::string
    [[nodiscard]] inline program load_binary(const std::string& path)
    {
        return load_binary(path.c_str());
    }

#ifdef _WIN32
    // Windows memory-mapped loader
    [[nodiscard]] inline program load_binary_mmap(const char* path)
    {
        HANDLE hFile = CreateFileA(
            path,
            GENERIC_READ,
            FILE_SHARE_READ,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        if (hFile == INVALID_HANDLE_VALUE)
        {
            throw std::runtime_error("cannot open file");
        }

        LARGE_INTEGER file_size;
        if (!GetFileSizeEx(hFile, &file_size))
        {
            CloseHandle(hFile);
            throw std::runtime_error("cannot get file size");
        }

        const auto size = static_cast<std::size_t>(file_size.QuadPart);
        if (size == 0 || size % instruction_size != 0)
        {
            CloseHandle(hFile);
            throw std::runtime_error("invalid file size");
        }

        HANDLE hMapping = CreateFileMappingA(
            hFile,
            nullptr,
            PAGE_READONLY,
            0,
            0,
            nullptr
        );

        if (!hMapping)
        {
            CloseHandle(hFile);
            throw std::runtime_error("cannot create file mapping");
        }

        const auto* data = static_cast<const std::uint8_t*>(
            MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0)
        );

        if (!data)
        {
            CloseHandle(hMapping);
            CloseHandle(hFile);
            throw std::runtime_error("cannot map file");
        }

        const std::size_t count = size / instruction_size;
        program prog;
        prog.reserve(count);

        for (std::size_t i = 0; i < count; ++i)
        {
            const std::uint8_t* ptr = data + (i * instruction_size);

            instruction instr;
            instr.op = static_cast<opcode>(ptr[0]);
            std::memcpy(&instr.operand, ptr + 1, sizeof(value_t));

            prog.push_back(instr);
        }

        UnmapViewOfFile(data);
        CloseHandle(hMapping);
        CloseHandle(hFile);

        return prog;
    }
#else
    // POSIX memory-mapped loader
    [[nodiscard]] inline program load_binary_mmap(const char* path)
    {
        int fd = open(path, O_RDONLY);
        if (fd == -1)
        {
            throw std::runtime_error("cannot open file");
        }

        struct stat sb;
        if (fstat(fd, &sb) == -1)
        {
            close(fd);
            throw std::runtime_error("cannot stat file");
        }

        const auto size = static_cast<std::size_t>(sb.st_size);
        if (size == 0 || size % instruction_size != 0)
        {
            close(fd);
            throw std::runtime_error("invalid file size");
        }

        const auto* data = static_cast<const std::uint8_t*>(
            mmap(nullptr, size, PROT_READ, MAP_PRIVATE, fd, 0)
        );

        if (data == MAP_FAILED)
        {
            close(fd);
            throw std::runtime_error("cannot mmap file");
        }

        const std::size_t count = size / instruction_size;
        program prog;
        prog.reserve(count);

        for (std::size_t i = 0; i < count; ++i)
        {
            const std::uint8_t* ptr = data + (i * instruction_size);

            instruction instr;
            instr.op = static_cast<opcode>(ptr[0]);
            std::memcpy(&instr.operand, ptr + 1, sizeof(value_t));

            prog.push_back(instr);
        }

        munmap(const_cast<std::uint8_t*>(data), size);
        close(fd);

        return prog;
    }
#endif

    [[nodiscard]] inline program load_binary_mmap(const std::string& path)
    {
        return load_binary_mmap(path.c_str());
    }

} // namespace execute

#endif // VARABLIZER_LOADER_H
