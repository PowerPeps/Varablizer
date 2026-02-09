#if ENABLE_STACK

/* [[Opcode::PUSH, 0x02, Group::STACK]] */
static void h_push(assembly& vm) noexcept
{
    vm.push(vm.program_[vm.ip_].operand);
}

/* [[Opcode::DUP, 0x03, Group::STACK]] */
static void h_dup(assembly& vm) noexcept
{
    if (vm.stack_.empty())
    {
        vm.halted_ = true;
        return;
    }
    vm.push(vm.top());
}

/* [[Opcode::POP, 0x04, Group::STACK]] */
static void h_pop(assembly& vm) noexcept
{
    vm.pop();
}

#endif // ENABLE_STACK
