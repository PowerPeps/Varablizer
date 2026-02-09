#if ENABLE_DEBUG

/* [[Opcode::DD, 0x0A, Group::DEBUG]] */
static void h_dd(assembly& vm) noexcept
{
#ifdef DEBUG
    vm.debug_dump();
#else
    (void)vm;
#endif
}

#endif // ENABLE_DEBUG
