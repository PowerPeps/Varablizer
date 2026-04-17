#ifndef VARABLIZER_TYPE_RESOLVER_H
#define VARABLIZER_TYPE_RESOLVER_H

#include "../execute/vtypes.h"
#include "../execute/opcodes.h"

#include <string>
#include <unordered_map>

namespace compile
{
    using namespace execute;

    // Map a .vz size specifier string to vwidth
    // Supports: b_8, b_16, b_32, b_64, y_1..y_8, w_1..w_4, d_1..d_2, q_1
    [[nodiscard]] inline vwidth size_spec_to_width(const std::string& spec) noexcept
    {
        static const std::unordered_map<std::string, vwidth> table = {
            {"b_8",  vwidth::W8},  {"y_1", vwidth::W8},
            {"b_16", vwidth::W16}, {"y_2", vwidth::W16}, {"w_1", vwidth::W16},
            {"b_32", vwidth::W32}, {"y_4", vwidth::W32}, {"w_2", vwidth::W32}, {"d_1", vwidth::W32},
            {"b_64", vwidth::W64}, {"y_8", vwidth::W64}, {"w_4", vwidth::W64}, {"d_2", vwidth::W64}, {"q_1", vwidth::W64},
        };
        auto it = table.find(spec);
        return (it != table.end()) ? it->second : vwidth::W64; // default: 64-bit
    }

    // Returns true if the identifier looks like a size specifier
    [[nodiscard]] inline bool is_size_spec(const std::string& s) noexcept
    {
        static const std::unordered_map<std::string, bool> table = {
            {"b_8",1},{"b_16",1},{"b_32",1},{"b_64",1},
            {"y_1",1},{"y_2",1},{"y_4",1},{"y_8",1},
            {"w_1",1},{"w_2",1},{"w_4",1},
            {"d_1",1},{"d_2",1},{"q_1",1},
        };
        return table.count(s) > 0;
    }

    // Resolve a fully parsed type descriptor to vtype
    // size_spec: "b_8", "b_16", etc., or "" for bare "int"
    // is_unsigned: true if "unsigned" keyword seen
    // base: "int" or "char"
    [[nodiscard]] inline vtype resolve_vtype(const std::string& size_spec,
                                              bool is_unsigned,
                                              const std::string& base) noexcept
    {
        if (base == "char")
            return types::CHAR;

        const vwidth w = size_spec_to_width(size_spec.empty() ? "b_64" : size_spec);
        const vkind  k = is_unsigned ? vkind::INT_U : vkind::INT_S;
        return vtype(w, k);
    }

    // Return the LOAD_HEAP_N opcode matching vtype width
    [[nodiscard]] inline opcode load_heap_opcode(vtype t) noexcept
    {
        switch (t.width())
        {
            case vwidth::W8:  return opcode::LOAD_HEAP_8;
            case vwidth::W16: return opcode::LOAD_HEAP_16;
            case vwidth::W32: return opcode::LOAD_HEAP_32;
            default:          return opcode::LOAD_HEAP_64;
        }
    }

    // Return the STORE_DEREF_N opcode for writing through a tagged pointer (HEAP or LOCALS)
    [[nodiscard]] inline opcode store_deref_opcode(vtype pointee) noexcept
    {
        switch (pointee.width())
        {
            case vwidth::W8:  return opcode::STORE_DEREF_8;
            case vwidth::W16: return opcode::STORE_DEREF_16;
            case vwidth::W32: return opcode::STORE_DEREF_32;
            default:          return opcode::STORE_DEREF_64;
        }
    }

    // Return the DEREF_N opcode for reading through a pointer to pointee type
    [[nodiscard]] inline opcode deref_opcode(vtype pointee) noexcept
    {
        switch (pointee.width())
        {
            case vwidth::W8:  return opcode::DEREF_8;
            case vwidth::W16: return opcode::DEREF_16;
            case vwidth::W32: return opcode::DEREF_32;
            default:          return opcode::DEREF_64;
        }
    }

    // Return the STORE_HEAP_N opcode matching vtype width
    [[nodiscard]] inline opcode store_heap_opcode(vtype t) noexcept
    {
        switch (t.width())
        {
            case vwidth::W8:  return opcode::STORE_HEAP_8;
            case vwidth::W16: return opcode::STORE_HEAP_16;
            case vwidth::W32: return opcode::STORE_HEAP_32;
            default:          return opcode::STORE_HEAP_64;
        }
    }

} // namespace compile

#endif // VARABLIZER_TYPE_RESOLVER_H
