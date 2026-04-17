#if ENABLE_BITWISE

/* [[Opcode::NEG, 0x38, Group::BITWISE]] */
static void h_neg(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.empty())) { vm.halted_ = true; return; }
    auto& val = vm.eval_.top_value();
    const vtype t = vm.eval_.top_type();
    val = truncate_to_type(-val, t);
}

/* [[Opcode::INC, 0x39, Group::BITWISE]] */
static void h_inc(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.empty())) { vm.halted_ = true; return; }
    auto& val = vm.eval_.top_value();
    const vtype t = vm.eval_.top_type();
    val = truncate_to_type(val + 1, t);
}

/* [[Opcode::DEC, 0x3A, Group::BITWISE]] */
static void h_dec(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.empty())) { vm.halted_ = true; return; }
    auto& val = vm.eval_.top_value();
    const vtype t = vm.eval_.top_type();
    val = truncate_to_type(val - 1, t);
}

/* [[Opcode::AND, 0x3B, Group::BITWISE]] */
static void h_and(assembly& vm) noexcept
{
    vm.binary_typed([](value_t a, value_t b) noexcept { return a & b; });
}

/* [[Opcode::OR, 0x3C, Group::BITWISE]] */
static void h_or(assembly& vm) noexcept
{
    vm.binary_typed([](value_t a, value_t b) noexcept { return a | b; });
}

/* [[Opcode::XOR, 0x3D, Group::BITWISE]] */
static void h_xor(assembly& vm) noexcept
{
    vm.binary_typed([](value_t a, value_t b) noexcept { return a ^ b; });
}

/* [[Opcode::NOT, 0x3E, Group::BITWISE]] */
static void h_not(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.empty())) { vm.halted_ = true; return; }
    auto& val = vm.eval_.top_value();
    const vtype t = vm.eval_.top_type();
    // Bitwise NOT then re-mask to type width
    val = truncate_to_type(~val, t);
}

/* [[Opcode::SHL, 0x3F, Group::BITWISE]] */
// Stack: ..., value, count  (count on top)
static void h_shl(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.size() < 2)) { vm.halted_ = true; return; }
    const auto count = static_cast<std::uint8_t>(vm.eval_.pop_value() & 0x3F);
    auto& val  = vm.eval_.top_value();
    const vtype t = vm.eval_.top_type();
    val = truncate_to_type(val << count, t);
}

/* [[Opcode::SHR, 0x40, Group::BITWISE]] */
// Arithmetic (signed) shift right
// Stack: ..., value, count  (count on top)
static void h_shr(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.size() < 2)) { vm.halted_ = true; return; }
    const auto count = static_cast<std::uint8_t>(vm.eval_.pop_value() & 0x3F);
    auto& val  = vm.eval_.top_value();
    const vtype t = vm.eval_.top_type();
    // Arithmetic right shift: well-defined for signed in C++20
    val = truncate_to_type(val >> count, t);
}

#endif // ENABLE_BITWISE
