#ifndef VARABLIZER_ASSEMBLY_H
#define VARABLIZER_ASSEMBLY_H

#include "opcodes.h"
#include "opcodes_config.h"
#include "vtypes.h"
#include "typed_stack.h"
#include "validator.h"

#include <array>
#include <vector>
#include <stdexcept>
#include <cassert>
#include <cstring>

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

    // Per-function call frame metadata
    struct call_frame
    {
        std::uint32_t return_ip;     // instruction pointer to resume after RET
        std::uint32_t locals_base;   // index into local_vals_/local_types_ where frame starts
        std::uint16_t locals_count;  // total local slots (args + declared locals)
        std::uint16_t args_count;    // number of arguments passed
    };

    class assembly
    {
    public:
        using handler_t = void (*)(assembly&) noexcept;

        static constexpr std::size_t default_stack_limit = 1024 * 1024;

        explicit assembly(std::size_t stack_limit = default_stack_limit) noexcept
            : stack_limit_(stack_limit)
        {
            eval_.reserve(1024);
            local_vals_.reserve(256);
            local_types_.reserve(256);
            heap_.reserve(4096);
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
            load_unchecked(std::move(prog));
        }

        void load_unchecked(program&& prog) noexcept
        {
            program_ = std::move(prog);
            ip_      = 0;
            halted_  = false;
            fbp_     = 0;
            eval_.clear();
            calls_.clear();
            local_vals_.clear();
            local_types_.clear();
            heap_.clear();
        }

        void run() noexcept
        {
            const std::size_t prog_size = program_.size();
            while (likely(!halted_ && ip_ < prog_size))
            {
                step();
            }
        }

        [[nodiscard]] bool        is_halted()   const noexcept { return halted_; }
        [[nodiscard]] std::size_t ip()          const noexcept { return ip_; }
        [[nodiscard]] std::size_t stack_size()  const noexcept { return eval_.size(); }

        // Top value on the evaluation stack
        [[nodiscard]] value_t top() const noexcept
        {
            assert(!eval_.empty());
            return eval_.top_value();
        }

        // Top type on the evaluation stack
        [[nodiscard]] vtype top_type() const noexcept
        {
            assert(!eval_.empty());
            return eval_.top_type();
        }

        void set_stack_limit(std::size_t limit) noexcept { stack_limit_ = limit; }
        [[nodiscard]] std::size_t stack_limit() const noexcept { return stack_limit_; }

#ifdef DEBUG
        [[nodiscard]] std::string to_string() const
        {
            std::ostringstream oss;
            oss << "-- ip=\033[34m" << ip_ << "\033[0m  fbp=" << fbp_ << "\n";

            for (std::size_t i = eval_.values.size(); i > 0; --i)
            {
                const value_t v = eval_.values[i - 1];
                const vtype   t = eval_.types[i - 1];
                if (i == eval_.values.size())
                    oss << "\033[32m|\033[0m\033[33m" << v << "\033[0m\033[32m|\033[0m";
                else
                    oss << "|" << v << "|";
                oss << "  (w" << static_cast<int>(t.bit_width()) << ")\n";
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

        // Pop a raw value (no type) — for legacy handlers
        f_incline value_t pop() noexcept
        {
            assert(!eval_.empty());
            return eval_.pop_value();
        }

        // Push with default I64 type — for legacy PUSH opcode
        f_incline void push(value_t v) noexcept
        {
            if (unlikely(eval_.size() >= stack_limit_))
            {
                halted_ = true;
                return;
            }
            eval_.push(v, types::I64);
        }

        // Typed binary operation: promotes types, auto-masks result
        template <typename Op>
        f_incline void binary_typed(Op op) noexcept
        {
            if (unlikely(eval_.size() < 2))
            {
                halted_ = true;
                return;
            }

            const value_t bv = eval_.values.back();
            const vtype   bt = eval_.types.back();
            eval_.values.pop_back();
            eval_.types.pop_back();

            value_t& av = eval_.values.back();
            vtype&   at = eval_.types.back();

            const vtype result_type = promote(at, bt);
            const value_t raw = op(av, bv);
            av = truncate_to_type(raw, result_type);
            at = result_type;
        }

        // Legacy untyped binary — kept for backward compatibility with old PUSH usage
        template <typename Op>
        f_incline void binary(Op op) noexcept
        {
            binary_typed(op);
        }

    private:
        // ── Evaluation stack (expression temporaries) ─────────────────────────
        typed_stack eval_;

        // ── Call stack (return addresses + frame metadata) ────────────────────
        std::vector<call_frame> calls_;

        // ── Local variable table (flat SoA, partitioned by frames) ───────────
        std::vector<value_t> local_vals_;
        std::vector<vtype>   local_types_;
        std::uint32_t fbp_ = 0;   // frame base pointer (index into local_vals_)

        // ── Heap (byte-addressable, bump allocator) ───────────────────────────
        std::vector<std::uint8_t> heap_;

        // ── Program state ─────────────────────────────────────────────────────
        program       program_;
        std::size_t   ip_          = 0;
        std::size_t   stack_limit_;
        bool          halted_      = false;

        // ── Opcode handlers (included as members) ────────────────────────────
#include "opcodes/control.h"
#include "opcodes/stack.h"
#include "opcodes/math.h"
#include "opcodes/debug.h"
#include "opcodes/flow.h"
#include "opcodes/io.h"
#include "opcodes/locals.h"
#include "opcodes/heap.h"
#include "opcodes/call.h"
#include "opcodes/bitwise.h"
#include "opcodes/compare.h"

#include "generated/handlers_table.inl"

    };

#undef likely
#undef unlikely
#undef f_incline
} // namespace execute

#endif // VARABLIZER_ASSEMBLY_H
