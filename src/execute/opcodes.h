#ifndef VARABLIZER_OPCODES_H
#define VARABLIZER_OPCODES_H

#include <cstdint>
#include <vector>

namespace execute
{
    using value_t = std::int64_t;

    enum class opcode : std::uint8_t
    {
        NOP = 0,
        PUSH,
        POP,
        ADD,
        SUB,
        MUL,
        DIV,
        MOD,
        DD,
        HALT,
        COUNT
    };

    struct instruction
    {
        opcode  op;
        value_t operand = 0;
    };

    using program = std::vector<instruction>;

    inline constexpr std::size_t opcode_count = static_cast<std::size_t>(opcode::COUNT);

    [[nodiscard]] constexpr bool is_valid_opcode(opcode op) noexcept
    {
        return static_cast<std::uint8_t>(op) < static_cast<std::uint8_t>(opcode::COUNT);
    }

} // namespace execute

#endif // VARABLIZER_OPCODES_H
