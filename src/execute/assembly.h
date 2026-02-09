#ifndef VARABLIZER_ASSEMBLY_H
#define VARABLIZER_ASSEMBLY_H

#include "opcodes.h"
#include "opcodes_config.h"
#include "validator.h"

#include <array>
#include <stdexcept>
#include <cassert>

#ifdef DEBUG
#include <sstream>
#include <string>
#include <iostream>
#endif

namespace execute
{
#if defined(__GNUC__) || defined(__clang__)
    #define likely(x)   __builtin_expect(!!(x), 1)
    #define unlikely(x) __builtin_expect(!!(x), 0)
#else
    #define likely(x)   (x)
    #define unlikely(x) (x)
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define f_incline __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
    #define f_incline __forceinline
#else
    #define f_incline inline
#endif

    class assembly
    {
    public:
        using handler_t = void (*)(assembly&) noexcept;

        static constexpr std::size_t default_stack_limit = 1024 * 1024;

        explicit assembly(std::size_t stack_limit = default_stack_limit) noexcept
            : stack_limit_(stack_limit)
        {
            stack_.reserve(1024);
        }

        void load(program&& prog)
        {
            auto result = validate_program(prog);
            if (!result)
            {
                throw std::invalid_argument(
                    "invalid program at instruction " +
                    std::to_string(result.error->instruction_index) +
                    ": " + result.error->message
                );
            }

            program_ = std::move(prog);
            ip_ = 0;
            halted_ = false;
            stack_.clear();
        }

        void load_unchecked(program&& prog) noexcept
        {
            program_ = std::move(prog);
            ip_ = 0;
            halted_ = false;
            stack_.clear();
        }

        void run() noexcept
        {
            const std::size_t prog_size = program_.size();
            while (likely(!halted_ && ip_ < prog_size))
            {
                step();
            }
        }

        [[nodiscard]] bool is_halted() const noexcept { return halted_; }
        [[nodiscard]] std::size_t ip() const noexcept { return ip_; }
        [[nodiscard]] std::size_t stack_size() const noexcept { return stack_.size(); }

        [[nodiscard]] value_t top() const noexcept
        {
            assert(!stack_.empty());
            return stack_.back();
        }

        void set_stack_limit(std::size_t limit) noexcept { stack_limit_ = limit; }
        [[nodiscard]] std::size_t stack_limit() const noexcept { return stack_limit_; }

#ifdef DEBUG
        [[nodiscard]] std::string to_string() const
        {
            std::ostringstream oss;
            oss << "-- \033[34m" << ip_ << "\033[0m\n";

            for (auto it = stack_.rbegin(); it != stack_.rend(); ++it)
            {
                if (it == stack_.rbegin())
                {
                    oss << "\033[32m|\033[0m\033[33m" << *it << "\033[0m\033[32m|\033[0m\n";
                }
                else
                {
                    oss << "|" << *it << "|\n";
                }
            }
            oss << "\n";
            return oss.str();
        }

        void debug_dump() const
        {
            std::cout << to_string() << std::flush;
        }
#endif

    private:
        f_incline void step() noexcept
        {
            const instruction& instr = program_[ip_];
            const auto idx = static_cast<std::size_t>(instr.op);
            handlers_[idx](*this);
            ++ip_;
        }

        f_incline value_t pop() noexcept
        {
            assert(!stack_.empty());
            value_t v = stack_.back();
            stack_.pop_back();
            return v;
        }

        f_incline void push(value_t v) noexcept
        {
            if (unlikely(stack_.size() >= stack_limit_))
            {
                halted_ = true;
                return;
            }
            stack_.push_back(v);
        }

        template <typename Op>
        f_incline void binary(Op op) noexcept
        {
            auto& b = stack_.back();
            auto& a = stack_[stack_.size() - 2];
            a = op(a, b);
            stack_.pop_back();
        }

    private:
        std::vector<value_t> stack_;
        program program_;
        std::size_t ip_ = 0;
        std::size_t stack_limit_;
        bool halted_ = false;

#include "opcodes/control.h"
#include "opcodes/stack.h"
#include "opcodes/math.h"
#include "opcodes/debug.h"
#include "opcodes/flow.h"
#include "opcodes/io.h"

#include "generated/handlers_table.inl"

    };

#undef likely
#undef unlikely
#undef f_incline
} // namespace execute

#endif // VARABLIZER_ASSEMBLY_H
