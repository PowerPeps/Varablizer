#ifndef VARABLIZER_TYPED_STACK_H
#define VARABLIZER_TYPED_STACK_H

#include "vtypes.h"

#include <vector>
#include <cassert>
#include <cstdint>

namespace execute
{
    // Structure-of-Arrays typed stack.
    //
    // Keeps values and types in separate contiguous arrays for cache efficiency:
    // hot paths (arithmetic) only touch values[], types[] stays cold.
    struct typed_stack
    {
        std::vector<std::int64_t> values;
        std::vector<vtype>        types;

        void reserve(std::size_t n)
        {
            values.reserve(n);
            types.reserve(n);
        }

        void push(std::int64_t v, vtype t) noexcept
        {
            values.push_back(v);
            types.push_back(t);
        }

        // Pop and return both value and type
        [[nodiscard]] std::pair<std::int64_t, vtype> pop() noexcept
        {
            assert(!values.empty());
            auto v = values.back(); values.pop_back();
            auto t = types.back();  types.pop_back();
            return {v, t};
        }

        // Pop only the value (discard type)
        [[nodiscard]] std::int64_t pop_value() noexcept
        {
            assert(!values.empty());
            auto v = values.back(); values.pop_back();
            types.pop_back();
            return v;
        }

        [[nodiscard]] std::int64_t& top_value() noexcept
        {
            assert(!values.empty());
            return values.back();
        }

        [[nodiscard]] const std::int64_t& top_value() const noexcept
        {
            assert(!values.empty());
            return values.back();
        }

        [[nodiscard]] vtype& top_type() noexcept
        {
            assert(!types.empty());
            return types.back();
        }

        [[nodiscard]] vtype top_type() const noexcept
        {
            assert(!types.empty());
            return types.back();
        }

        // Access by index from top (0 = top, 1 = second, ...)
        [[nodiscard]] std::int64_t& peek_value(std::size_t from_top = 0) noexcept
        {
            assert(values.size() > from_top);
            return values[values.size() - 1 - from_top];
        }

        [[nodiscard]] vtype& peek_type(std::size_t from_top = 0) noexcept
        {
            assert(types.size() > from_top);
            return types[types.size() - 1 - from_top];
        }

        [[nodiscard]] std::size_t size() const noexcept { return values.size(); }
        [[nodiscard]] bool        empty() const noexcept { return values.empty(); }

        void clear() noexcept
        {
            values.clear();
            types.clear();
        }
    };

} // namespace execute

#endif // VARABLIZER_TYPED_STACK_H
