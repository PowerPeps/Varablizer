#ifndef VARABLIZER_VALIDATOR_H
#define VARABLIZER_VALIDATOR_H

#include "opcodes.h"
#include <string>
#include <optional>

namespace execute
{
    struct validation_error
    {
        std::size_t instruction_index;
        std::string message;
    };

    struct validation_result
    {
        bool valid = true;
        std::optional<validation_error> error = std::nullopt;

        [[nodiscard]] explicit operator bool() const noexcept { return valid; }
    };

    [[nodiscard]] inline validation_result validate_program(const program& prog) noexcept
    {
        if (prog.empty())
        {
            return { false, validation_error{ 0, "program is empty" } };
        }

        for (std::size_t i = 0; i < prog.size(); ++i)
        {
            const auto& instr = prog[i];

            if (!is_valid_opcode(instr.op))
            {
                return { false, validation_error{ i, "invalid opcode" } };
            }
        }

        if (prog.back().op != opcode::HALT)
        {
            return { false, validation_error{ prog.size() - 1, "program must end with HALT" } };
        }

        return { true, std::nullopt };
    }

} // namespace execute

#endif // VARABLIZER_VALIDATOR_H
