#if ENABLE_LOCALS

/* [[Opcode::PUSH_T, 0x1B, Group::LOCALS]] */
// Operand encoding: byte 0 = vtype bits, bytes 1-7 = 7-byte sign-extended value
static void h_push_t(assembly& vm) noexcept
{
    const value_t operand = vm.program_[vm.ip_].operand;

    // Extract vtype from lowest byte of operand
    vtype t;
    t.bits = static_cast<std::uint8_t>(operand & 0xFF);

    // Extract the 7-byte value from bytes 1-7 (sign-extended from 56 bits)
    value_t raw = operand >> 8;

    // Sign-extend from 56 bits if the type is signed
    if (t.is_signed() && t.width() != vwidth::W64)
    {
        // The value is stored in bits [62:8] of the operand as a 56-bit field,
        // but it represents a value bounded by the type width. Just apply the mask.
        raw = truncate_to_type(raw, t);
    }
    else
    {
        raw &= static_cast<value_t>(t.mask());
    }

    vm.eval_.push(raw, t);
}

/* [[Opcode::LOAD_LOCAL, 0x1C, Group::LOCALS]] */
static void h_load_local(assembly& vm) noexcept
{
    const auto slot = static_cast<std::uint32_t>(vm.program_[vm.ip_].operand);
    const std::size_t idx = vm.fbp_ + slot;

    if (unlikely(idx >= vm.local_vals_.size()))
    {
        vm.halted_ = true;
        return;
    }

    vm.eval_.push(vm.local_vals_[idx], vm.local_types_[idx]);
}

/* [[Opcode::STORE_LOCAL, 0x1D, Group::LOCALS]] */
static void h_store_local(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.empty()))
    {
        vm.halted_ = true;
        return;
    }

    const auto slot = static_cast<std::uint32_t>(vm.program_[vm.ip_].operand);
    const std::size_t idx = vm.fbp_ + slot;

    if (unlikely(idx >= vm.local_vals_.size()))
    {
        vm.halted_ = true;
        return;
    }

    auto [val, typ] = vm.eval_.pop();
    vm.local_vals_[idx]  = val;
    vm.local_types_[idx] = typ;
}

/* [[Opcode::LEA_LOCAL, 0x1E, Group::LOCALS]] */
// Push a tagged pointer to a local variable slot
static void h_lea_local(assembly& vm) noexcept
{
    const auto slot = static_cast<std::uint32_t>(vm.program_[vm.ip_].operand);
    const std::uint64_t absolute = vm.fbp_ + slot;
    vm.eval_.push(tagged_ptr::encode(ptr_region::LOCALS, absolute), types::PTR);
}

/* [[Opcode::CAST, 0x1F, Group::LOCALS]] */
// Re-type the top of eval_ to target vtype (truncating/extending as needed)
static void h_cast(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.empty()))
    {
        vm.halted_ = true;
        return;
    }

    vtype target;
    target.bits = static_cast<std::uint8_t>(vm.program_[vm.ip_].operand & 0xFF);

    auto& val = vm.eval_.top_value();
    auto& typ = vm.eval_.top_type();

    val = truncate_to_type(val, target);
    typ = target;
}

#endif // ENABLE_LOCALS
