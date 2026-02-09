#if ENABLE_FLOW

/* [[Opcode::GOTO, 0x0B, Group::FLOW]] */
static void h_goto(assembly& vm) noexcept
{
    vm.ip_ = static_cast<std::size_t>(--vm.program_[vm.ip_].operand);
}

/* [[Opcode::GOTO_E, 0x0C, Group::FLOW]] */
static void h_goto_e(assembly& vm) noexcept
{
    vm.program_[vm.ip_].op = opcode::NOP;
    vm.ip_ = static_cast<std::size_t>(--vm.program_[vm.ip_].operand);
}

/* [[Opcode::EQ, 0x0D, Group::FLOW]] */
static void h_eq(assembly& vm) noexcept
{
    value_t b = vm.pop();
    value_t a = vm.pop();

    if(a == b) {
        vm.ip_ = static_cast<std::size_t>(--vm.program_[vm.ip_].operand);
    }
}

/* [[Opcode::EQ_E, 0x0E, Group::FLOW]] */
static void h_eq_e(assembly& vm) noexcept
{
    value_t b = vm.pop();
    value_t a = vm.pop();

    if(a == b) {
        vm.program_[vm.ip_].op = opcode::NOP;
        vm.ip_ = static_cast<std::size_t>(--vm.program_[vm.ip_].operand);
    }
}

/* [[Opcode::NEQ, 0x0F, Group::FLOW]] */
static void h_neq(assembly& vm) noexcept
{
    value_t b = vm.pop();
    value_t a = vm.pop();

    if(a != b) {
        vm.ip_ = static_cast<std::size_t>(--vm.program_[vm.ip_].operand);
    }
}

/* [[Opcode::NEQ_E, 0x10, Group::FLOW]] */
static void h_neq_e(assembly& vm) noexcept
{
    value_t b = vm.pop();
    value_t a = vm.pop();

    if(a != b) {
        vm.program_[vm.ip_].op = opcode::NOP;
        vm.ip_ = static_cast<std::size_t>( --vm.program_[vm.ip_].operand );
    }
}

/* [[Opcode::LT, 0x11, Group::FLOW]] */
static void h_lt(assembly& vm) noexcept
{
    value_t b = vm.pop();
    value_t a = vm.pop();

    if(a < b) {
        vm.ip_ = static_cast<std::size_t>( --vm.program_[vm.ip_].operand );
    }
}

/* [[Opcode::LT_E, 0x12, Group::FLOW]] */
static void h_lt_e(assembly& vm) noexcept
{
    value_t b = vm.pop();
    value_t a = vm.pop();

    if(a < b) {
        vm.program_[vm.ip_].op = opcode::NOP;
        vm.ip_ = static_cast<std::size_t>( --vm.program_[vm.ip_].operand );
    }
}

/* [[Opcode::LTE, 0x13, Group::FLOW]] */
static void h_lte(assembly& vm) noexcept
{
    value_t b = vm.pop();
    value_t a = vm.pop();

    if(a <= b) {
        vm.ip_ = static_cast<std::size_t>( --vm.program_[vm.ip_].operand );
    }
}

/* [[Opcode::LTE_E, 0x14, Group::FLOW]] */
static void h_lte_e(assembly& vm) noexcept
{
    value_t b = vm.pop();
    value_t a = vm.pop();

    if(a <= b) {
        vm.program_[vm.ip_].op = opcode::NOP;
        vm.ip_ = static_cast<std::size_t>( --vm.program_[vm.ip_].operand );
    }
}

/* [[Opcode::GT, 0x15, Group::FLOW]] */
static void h_gt(assembly& vm) noexcept
{
    value_t b = vm.pop();
    value_t a = vm.pop();

    if(a > b) {
        vm.ip_ = static_cast<std::size_t>( --vm.program_[vm.ip_].operand );
    }
}

/* [[Opcode::GT_E, 0x16, Group::FLOW]] */
static void h_gt_e(assembly& vm) noexcept
{
    value_t b = vm.pop();
    value_t a = vm.pop();

    if(a > b) {
        vm.program_[vm.ip_].op = opcode::NOP;
        vm.ip_ = static_cast<std::size_t>( --vm.program_[vm.ip_].operand );
    }
}

/* [[Opcode::GTE, 0x17, Group::FLOW]] */
static void h_gte(assembly& vm) noexcept
{
    value_t b = vm.pop();
    value_t a = vm.pop();

    if(a >= b) {
        vm.ip_ = static_cast<std::size_t>( --vm.program_[vm.ip_].operand );
    }
}

/* [[Opcode::GTE_E, 0x18, Group::FLOW]] */
static void h_gte_e(assembly& vm) noexcept
{
    value_t b = vm.pop();
    value_t a = vm.pop();

    if(a >= b) {
        vm.program_[vm.ip_].op = opcode::NOP;
        vm.ip_ = static_cast<std::size_t>( --vm.program_[vm.ip_].operand );
    }
}

#endif // ENABLE_FLOW
