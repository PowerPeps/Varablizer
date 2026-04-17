#if ENABLE_CONTROL

/* [[Opcode::NOP, 0x00, Group::CONTROL]] */
static void h_nop(assembly&) noexcept
{
}

/* [[Opcode::HALT, 0x01, Group::CONTROL]] */
static void h_halt(assembly& vm) noexcept
{
    vm.halted_ = true;
}

#endif // ENABLE_CONTROL
