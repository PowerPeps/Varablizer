#ifndef VARABLIZER_EMITTER_H
#define VARABLIZER_EMITTER_H

#include "../execute/opcodes.h"

#include <string>
#include <fstream>
#include <stdexcept>
#include <cstring>

namespace compile
{
    // Write a program to a .vbin file.
    // Format: each instruction = [opcode:uint8][operand:int64 LE] = 9 bytes
    inline void emit_binary(const execute::program& prog, const std::string& output_path)
    {
        std::ofstream out(output_path, std::ios::binary | std::ios::trunc);
        if (!out)
            throw std::runtime_error("cannot open output file: " + output_path);

        for (const auto& instr : prog)
        {
            const auto op_byte = static_cast<std::uint8_t>(instr.op);
            out.write(reinterpret_cast<const char*>(&op_byte), 1);

            // operand: int64 little-endian
            std::uint8_t operand_bytes[8];
            std::memcpy(operand_bytes, &instr.operand, 8);
            out.write(reinterpret_cast<const char*>(operand_bytes), 8);
        }

        if (!out)
            throw std::runtime_error("write failed: " + output_path);
    }

} // namespace compile

#endif // VARABLIZER_EMITTER_H
