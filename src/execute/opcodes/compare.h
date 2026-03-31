#if ENABLE_COMPARE

// Helper: compare two values respecting signedness
// Returns -1, 0, or +1
static inline value_t cmp_values(value_t a, value_t b, vtype ta, vtype tb) noexcept
{
    // If either operand is unsigned, treat both as unsigned
    if (ta.is_unsigned() || tb.is_unsigned())
    {
        const auto ua = static_cast<std::uint64_t>(a);
        const auto ub = static_cast<std::uint64_t>(b);
        if (ua < ub) return -1;
        if (ua > ub) return  1;
        return 0;
    }
    // Both signed
    if (a < b) return -1;
    if (a > b) return  1;
    return 0;
}

/* [[Opcode::CMP_EQ, 0x54, Group::COMPARE]] */
static void h_cmp_eq(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.size() < 2)) { vm.halted_ = true; return; }
    auto [b, tb] = vm.eval_.pop();
    auto [a, ta] = vm.eval_.pop();
    vm.eval_.push(cmp_values(a, b, ta, tb) == 0 ? 1 : 0, types::I8);
}

/* [[Opcode::CMP_NEQ, 0x55, Group::COMPARE]] */
static void h_cmp_neq(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.size() < 2)) { vm.halted_ = true; return; }
    auto [b, tb] = vm.eval_.pop();
    auto [a, ta] = vm.eval_.pop();
    vm.eval_.push(cmp_values(a, b, ta, tb) != 0 ? 1 : 0, types::I8);
}

/* [[Opcode::CMP_LT, 0x56, Group::COMPARE]] */
static void h_cmp_lt(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.size() < 2)) { vm.halted_ = true; return; }
    auto [b, tb] = vm.eval_.pop();
    auto [a, ta] = vm.eval_.pop();
    vm.eval_.push(cmp_values(a, b, ta, tb) < 0 ? 1 : 0, types::I8);
}

/* [[Opcode::CMP_LTE, 0x57, Group::COMPARE]] */
static void h_cmp_lte(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.size() < 2)) { vm.halted_ = true; return; }
    auto [b, tb] = vm.eval_.pop();
    auto [a, ta] = vm.eval_.pop();
    vm.eval_.push(cmp_values(a, b, ta, tb) <= 0 ? 1 : 0, types::I8);
}

/* [[Opcode::CMP_GT, 0x58, Group::COMPARE]] */
static void h_cmp_gt(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.size() < 2)) { vm.halted_ = true; return; }
    auto [b, tb] = vm.eval_.pop();
    auto [a, ta] = vm.eval_.pop();
    vm.eval_.push(cmp_values(a, b, ta, tb) > 0 ? 1 : 0, types::I8);
}

/* [[Opcode::CMP_GTE, 0x59, Group::COMPARE]] */
static void h_cmp_gte(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.size() < 2)) { vm.halted_ = true; return; }
    auto [b, tb] = vm.eval_.pop();
    auto [a, ta] = vm.eval_.pop();
    vm.eval_.push(cmp_values(a, b, ta, tb) >= 0 ? 1 : 0, types::I8);
}

#endif // ENABLE_COMPARE
