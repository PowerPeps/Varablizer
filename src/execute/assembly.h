#ifndef VARABLIZER_ASSEMBLY_H
#define VARABLIZER_ASSEMBLY_H

#include "opcodes.h"
#include "validator.h"

#include <array>
#include <stdexcept>
#include <cassert>

#ifdef VARABLIZER_DEBUG
#include <sstream>
#include <string>
#include <iostream>
#endif

namespace execute
{
#if defined(__GNUC__) || defined(__clang__)
    #define VARABLIZER_LIKELY(x)   __builtin_expect(!!(x), 1)
    #define VARABLIZER_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define VARABLIZER_LIKELY(x)   (x)
    #define VARABLIZER_UNLIKELY(x) (x)
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define VARABLIZER_FORCE_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
    #define VARABLIZER_FORCE_INLINE __forceinline
#else
    #define VARABLIZER_FORCE_INLINE inline
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

        void load(program prog)
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

        void load_unchecked(program prog) noexcept
        {
            program_ = std::move(prog);
            ip_ = 0;
            halted_ = false;
            stack_.clear();
        }

        void run() noexcept
        {
            const std::size_t prog_size = program_.size();
            while (VARABLIZER_LIKELY(!halted_ && ip_ < prog_size))
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

#ifdef VARABLIZER_DEBUG
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
        VARABLIZER_FORCE_INLINE void step() noexcept
        {
            const instruction& instr = program_[ip_];
            const auto idx = static_cast<std::size_t>(instr.op);
            handlers_[idx](*this);
            ++ip_;
        }

        VARABLIZER_FORCE_INLINE value_t pop() noexcept
        {
            assert(!stack_.empty());
            value_t v = stack_.back();
            stack_.pop_back();
            return v;
        }

        VARABLIZER_FORCE_INLINE void push(value_t v) noexcept
        {
            if (VARABLIZER_UNLIKELY(stack_.size() >= stack_limit_))
            {
                halted_ = true;
                return;
            }
            stack_.push_back(v);
        }

        template <typename Op>
        VARABLIZER_FORCE_INLINE void binary(Op op) noexcept
        {
            value_t b = pop();
            value_t a = pop();
            push(op(a, b));
        }

        static void h_nop(assembly&) noexcept
        {
        }

        static void h_push(assembly& vm) noexcept
        {
            vm.push(vm.program_[vm.ip_].operand);
        }

        static void h_pop(assembly& vm) noexcept
        {
            vm.pop();
        }

        static void h_add(assembly& vm) noexcept
        {
            vm.binary([](value_t a, value_t b) noexcept { return a + b; });
        }

        static void h_sub(assembly& vm) noexcept
        {
            vm.binary([](value_t a, value_t b) noexcept { return a - b; });
        }

        static void h_mul(assembly& vm) noexcept
        {
            vm.binary([](value_t a, value_t b) noexcept { return a * b; });
        }

        static void h_div(assembly& vm) noexcept
        {
            value_t b = vm.pop();
            value_t a = vm.pop();
            vm.push(VARABLIZER_LIKELY(b != 0) ? a / b : 0);
        }

        static void h_mod(assembly& vm) noexcept
        {
            value_t b = vm.pop();
            value_t a = vm.pop();
            vm.push(VARABLIZER_LIKELY(b != 0) ? a % b : 0);
        }

        static void h_dd(assembly& vm) noexcept
        {
#ifdef VARABLIZER_DEBUG
            vm.debug_dump();
#else
            (void)vm;
#endif
        }

        static void h_halt(assembly& vm) noexcept
        {
            vm.halted_ = true;
        }

        static void h_goto(assembly& vm) noexcept
        {
            vm.program_[vm.ip_].op = opcode::NOP;
            vm.ip_ = static_cast<std::size_t>(--vm.program_[vm.ip_].operand);
        }

    private:
        std::vector<value_t> stack_;
        program program_;
        std::size_t ip_ = 0;
        std::size_t stack_limit_;
        bool halted_ = false;

        static constexpr std::array<handler_t, opcode_count> handlers_ =
        {
            &assembly::h_nop,
            &assembly::h_push,
            &assembly::h_pop,
            &assembly::h_add,
            &assembly::h_sub,
            &assembly::h_mul,
            &assembly::h_div,
            &assembly::h_mod,
            &assembly::h_dd,
            &assembly::h_halt,
            &assembly::h_goto
        };
    };

#undef VARABLIZER_LIKELY
#undef VARABLIZER_UNLIKELY
#undef VARABLIZER_FORCE_INLINE
} // namespace execute

#endif // VARABLIZER_ASSEMBLY_H
