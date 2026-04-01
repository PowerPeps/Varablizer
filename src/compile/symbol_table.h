#ifndef VARABLIZER_SYMBOL_TABLE_H
#define VARABLIZER_SYMBOL_TABLE_H

#include "../execute/vtypes.h"
#include "error.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <cstdint>

namespace compile
{
    using execute::vtype;

    struct Symbol
    {
        vtype    type;
        vtype    pointee;       // valid when is_pointer == true
        uint32_t slot;          // local slot index from fbp
        bool     is_param   = false;
        bool     is_pointer = false;
    };

    class SymbolTable
    {
    public:
        // Each scope is a map + pointer to parent
        struct Scope
        {
            std::unordered_map<std::string, Symbol> symbols;
            Scope* parent = nullptr;
        };

        void push_scope()
        {
            auto s = std::make_unique<Scope>();
            s->parent = current_;
            current_  = s.get();
            scopes_.push_back(std::move(s));
        }

        void pop_scope()
        {
            if (current_ && current_->parent)
                current_ = current_->parent;
            // Note: scopes stay allocated (slots don't shrink during function lifetime)
        }

        // Allocate a new slot and define a value symbol
        void define(const std::string& name, vtype type,
                    bool is_param = false)
        {
            if (!current_) push_scope();
            current_->symbols[name] = Symbol{ type, {}, next_slot_++, is_param, false };
        }

        // Allocate a new slot and define a pointer symbol
        void define_ptr(const std::string& name, vtype pointee,
                        bool is_param = false)
        {
            if (!current_) push_scope();
            current_->symbols[name] = Symbol{ execute::types::PTR, pointee, next_slot_++, is_param, true };
        }

        // Define with an explicit pre-assigned slot (for parameters)
        void define_at(const std::string& name, vtype type,
                       uint32_t slot, bool is_param = true)
        {
            if (!current_) push_scope();
            current_->symbols[name] = Symbol{ type, {}, slot, is_param, false };
        }

        // Define a pointer parameter with an explicit pre-assigned slot
        void define_ptr_at(const std::string& name, vtype pointee,
                           uint32_t slot, bool is_param = true)
        {
            if (!current_) push_scope();
            current_->symbols[name] = Symbol{ execute::types::PTR, pointee, slot, is_param, true };
        }

        // Look up a symbol, walking up the scope chain
        [[nodiscard]] Symbol* lookup(const std::string& name) noexcept
        {
            Scope* s = current_;
            while (s)
            {
                auto it = s->symbols.find(name);
                if (it != s->symbols.end())
                    return &it->second;
                s = s->parent;
            }
            return nullptr;
        }

        // Total number of slots allocated so far in this function
        [[nodiscard]] uint32_t total_slots() const noexcept { return next_slot_; }

        // Reset for a new function (keeps scope stack clean)
        void reset()
        {
            scopes_.clear();
            current_    = nullptr;
            next_slot_  = 0;
        }

        // Reserve N slots without defining symbols (for pre-counting)
        uint32_t reserve_slot() { return next_slot_++; }
        void set_next_slot(uint32_t n) { next_slot_ = n; }

    private:
        std::vector<std::unique_ptr<Scope>> scopes_;
        Scope*   current_   = nullptr;
        uint32_t next_slot_ = 0;
    };

} // namespace compile

#endif // VARABLIZER_SYMBOL_TABLE_H
