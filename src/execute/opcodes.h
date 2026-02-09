    #ifndef VARABLIZER_OPCODES_H
    #define VARABLIZER_OPCODES_H

    #include <cstdint>
    #include <vector>

    namespace execute
    {
        using value_t = std::int64_t;

        enum class Group : std::uint8_t
        {
            CONTROL,
            STACK,
            MATH,
            DEBUG,
            FLOW,
            IO,
            MISC,
        };

        enum class opcode : std::uint8_t
        {
            NOP = 0x00,
            HALT = 0x01,

            PUSH = 0x02,
            DUP = 0x03,
            POP = 0x04,

            ADD = 0x05,
            SUB = 0x06,
            MUL = 0x07,
            DIV = 0x08,
            MOD = 0x09,

            DD = 0x0A,

            GOTO = 0x0B,
            GOTO_E = 0x0C,
            EQ = 0x0D,
            EQ_E = 0x0E,
            NEQ = 0x0F,
            NEQ_E = 0x10,
            LT = 0x11,
            LT_E = 0x12,
            LTE = 0x13,
            LTE_E = 0x14,
            GT = 0x15,
            GT_E = 0x16,
            GTE = 0x17,
            GTE_E = 0x18,

            COUT = 0x19,
            CIN = 0x1A,

            COUNT = 0x1B,
        };

        struct instruction
        {
            opcode  op = opcode::NOP;
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
