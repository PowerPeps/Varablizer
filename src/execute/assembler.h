#ifndef VARABLIZER_ASSEMBLER_H
#define VARABLIZER_ASSEMBLER_H

#include "opcodes.h"
#include "loader.h"       // instruction_size, program (réutilisé tel quel)

#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <array>
#include <string_view>
#include <cstring>

namespace execute
{
    namespace detail
    {
        // Single source of truth: name table aligned on opcode values.
        // Must stay in sync with the enum in opcodes.h — same order, same values.
        inline constexpr std::array<std::pair<std::string_view, opcode>, opcode_count> opcode_names =
        {{
            { "NOP",    opcode::NOP    },
            { "HALT",   opcode::HALT   },
            { "PUSH",   opcode::PUSH   },
            { "DUP",    opcode::DUP    },
            { "POP",    opcode::POP    },
            { "ADD",    opcode::ADD    },
            { "SUB",    opcode::SUB    },
            { "MUL",    opcode::MUL    },
            { "DIV",    opcode::DIV    },
            { "MOD",    opcode::MOD    },
            { "DD",     opcode::DD     },
            { "GOTO",   opcode::GOTO   },
            { "GOTO_E", opcode::GOTO_E },
            { "EQ",     opcode::EQ     },
            { "EQ_E",   opcode::EQ_E   },
            { "NEQ",    opcode::NEQ    },
            { "NEQ_E",  opcode::NEQ_E  },
            { "LT",     opcode::LT     },
            { "LT_E",   opcode::LT_E   },
            { "LTE",    opcode::LTE    },
            { "LTE_E",  opcode::LTE_E  },
            { "GT",     opcode::GT     },
            { "GT_E",   opcode::GT_E   },
            { "GTE",    opcode::GTE    },
            { "GTE_E",  opcode::GTE_E  },
            { "COUT",   opcode::COUT   },
            { "CIN",    opcode::CIN    },
        }};

        [[nodiscard]] inline const std::unordered_map<std::string_view, opcode>& mnemonic_map()
        {
            static const auto map = []
            {
                std::unordered_map<std::string_view, opcode> m;
                m.reserve(opcode_names.size());
                for (const auto& [name, op] : opcode_names)
                    m.emplace(name, op);
                return m;
            }();
            return map;
        }

        inline std::string to_upper(std::string s)
        {
            std::transform(s.begin(), s.end(), s.begin(),
                           [](unsigned char c) { return std::toupper(c); });
            return s;
        }
    } // namespace detail

    // Parse a .txt source file into a program.
    // One instruction per non-empty line. '#' starts a comment.
    // Syntax: MNEMONIC [operand]
    // Example:
    //   PUSH 10   # push 10
    //   PUSH 20
    //   ADD
    //   HALT
    [[nodiscard]] inline program assemble(const char* path)
    {
        std::ifstream file(path);
        if (!file)
            throw std::runtime_error("cannot open file");

        const auto& map = detail::mnemonic_map();
        program prog;
        std::size_t line_number = 0;
        std::string line;

        while (std::getline(file, line))
        {
            ++line_number;

            // Strip comment
            const auto comment = line.find('#');
            if (comment != std::string::npos)
                line.erase(comment);

            // Trim surrounding whitespace
            const auto first = line.find_first_not_of(" \t\r\n");
            if (first == std::string::npos)
                continue;
            line = line.substr(first, line.find_last_not_of(" \t\r\n") - first + 1);

            std::istringstream iss(line);
            std::string mnemonic;
            iss >> mnemonic;

            const auto it = map.find(detail::to_upper(mnemonic));
            if (it == map.end())
            {
                throw std::runtime_error(
                    "unknown mnemonic '" + mnemonic + "' at line " + std::to_string(line_number)
                );
            }

            instruction instr;
            instr.op      = it->second;
            instr.operand = 0;

            std::string operand_str;
            if (iss >> operand_str)
            {
                try
                {
                    std::size_t pos;
                    instr.operand = std::stoll(operand_str, &pos, 0);
                    if (pos != operand_str.size())
                        throw std::invalid_argument("trailing chars");
                }
                catch (const std::exception&)
                {
                    throw std::runtime_error(
                        "invalid operand '" + operand_str + "' at line " + std::to_string(line_number)
                    );
                }
            }

            prog.push_back(instr);
        }

        return prog;
    }

    [[nodiscard]] inline program assemble(const std::string& path)
    {
        return assemble(path.c_str());
    }

    // Write a program to a .vbin file — exact mirror of load_binary().
    // Each instruction: 1 byte opcode + sizeof(value_t) bytes operand (little-endian).
    inline void write_binary(const program& prog, const char* path)
    {
        std::ofstream file(path, std::ios::binary);
        if (!file)
            throw std::runtime_error("cannot open file for writing");

        std::array<std::uint8_t, instruction_size> buffer{};

        for (const auto& instr : prog)
        {
            buffer[0] = static_cast<std::uint8_t>(instr.op);
            std::memcpy(buffer.data() + 1, &instr.operand, sizeof(value_t));
            file.write(reinterpret_cast<const char*>(buffer.data()), instruction_size);
        }
    }

    inline void write_binary(const program& prog, const std::string& path)
    {
        write_binary(prog, path.c_str());
    }

    // Write a program to a .txt file — exact mirror of assemble().
    // Each instruction is written as: MNEMONIC [operand]
    // Operand is omitted when zero to keep the output readable.
    inline void decompile(const program& prog, const char* path)
    {
        std::ofstream file(path);
        if (!file)
            throw std::runtime_error("cannot open file for writing");

        for (const auto& instr : prog)
        {
            const auto idx = static_cast<std::size_t>(instr.op);
            const std::string_view name = (idx < opcode_count)
                ? detail::opcode_names[idx].first
                : std::string_view("UNKNOWN");

            file << name;
            if (instr.operand != 0)
                file << ' ' << instr.operand;
            file << '\n';
        }
    }

    inline void decompile(const program& prog, const std::string& path)
    {
        decompile(prog, path.c_str());
    }

} // namespace execute

#endif // VARABLIZER_ASSEMBLER_H