#ifndef VARABLIZER_ASSEMBLY_H
#define VARABLIZER_ASSEMBLY_H

#include <cstdint>
#include <stack>
#include <vector>
#include <string>
#include <sstream>
#include <array>
#include <stdexcept>
#include <functional>
#include <iostream>

namespace execute
{
    using value_t = long long;

    enum class opcode : uint8_t
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

    class assembly
    {
    public:
        using handler_t = void (*)(assembly&);

        assembly() = default;

        void load(program prog)
        {
            program_ = std::move(prog);
            ip_ = 0;
            halted_ = false;
            clear_stack();
        }

        void run()
        {
            while (!halted_ && ip_ < program_.size())
            {
                step();
            }
        }

        [[nodiscard]] std::string to_string() const
        {
            std::ostringstream oss;
            oss << "-- \033[34m" << ip_ << "\033[0m\n";
            auto tmp = stack_;
            bool first = true;
            while (!tmp.empty())
            {
                if (first)
                {
                    oss << "\033[32m|\033[0m\033[33m" << tmp.top() << "\033[0m\033[32m|\033[0m\n";
                    first = false;
                }
                else
                {
                    oss << "|" << tmp.top() << "|\n";
                }
                tmp.pop();
            }

            oss << "\n";

            return oss.str();
        }

    private:

        void step()
        {

            const instruction& instr = program_[ip_];
            const auto idx = static_cast<std::size_t>(instr.op);

            handler_t handler = handlers_[idx];
            if (!handler)
                throw std::runtime_error("invalid opcode");

            handler(*this);
            ++ip_;
        }

        value_t pop()
        {
            if (stack_.empty())
                throw std::runtime_error("stack underflow");

            value_t v = stack_.top();
            stack_.pop();
            return v;
        }

        void push(value_t v)
        {
            stack_.push(v);
        }

        template<typename Op>
        void binary(Op op)
        {
            value_t b = pop();
            value_t a = pop();
            push(op(a, b));
        }

        void clear_stack()
        {
            while (!stack_.empty())
                stack_.pop();
        }

        static void h_nop(assembly&) {}

        static void h_push(assembly& vm)
        {
            vm.push(vm.program_[vm.ip_].operand);
        }

        static void h_pop(assembly& vm)
        {
            vm.pop();
        }

        static void h_add(assembly& vm)
        {
            vm.binary(std::plus<>());
        }

        static void h_sub(assembly& vm)
        {
            vm.binary(std::minus<>());
        }

        static void h_mul(assembly& vm)
        {
            vm.binary(std::multiplies<>());
        }

        static void h_div(assembly& vm)
        {
            vm.binary(std::divides<>());
        }

        static void h_mod(assembly& vm)
        {
            auto b = vm.pop();
            auto a = vm.pop();
            vm.push(a % b);
        }

        static void h_dd(assembly& vm)
        {
            std::cout << vm.to_string() << std::flush;
        }

        static void h_halt(assembly& vm)
        {
            vm.halted_ = true;
        }

    private:
        std::stack<value_t> stack_;
        program program_;
        std::size_t ip_ = 0;
        bool halted_ = false;

        static inline constexpr std::array<handler_t,
                                           static_cast<std::size_t>(opcode::COUNT)> handlers_ =
        {
            &assembly::h_nop,   // NOP
            &assembly::h_push,  // PUSH
            &assembly::h_pop,   // POP
            &assembly::h_add,   // ADD
            &assembly::h_sub,   // SUB
            &assembly::h_mul,   // MUL
            &assembly::h_div,   // DIV
            &assembly::h_mod,   // MOD
            &assembly::h_dd,    // DD
            &assembly::h_halt   // HALT
        };
    };

} // namespace execute

#endif // VARABLIZER_ASSEMBLY_H
