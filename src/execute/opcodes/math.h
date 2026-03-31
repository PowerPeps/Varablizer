#if ENABLE_MATH

/* [[Opcode::ADD, 0x05, Group::MATH]] */
static void h_add(assembly& vm) noexcept
{
    vm.binary_typed([](value_t a, value_t b) noexcept { return a + b; });
}

/* [[Opcode::SUB, 0x06, Group::MATH]] */
static void h_sub(assembly& vm) noexcept
{
    vm.binary_typed([](value_t a, value_t b) noexcept { return a - b; });
}

/* [[Opcode::MUL, 0x07, Group::MATH]] */
static void h_mul(assembly& vm) noexcept
{
    vm.binary_typed([](value_t a, value_t b) noexcept { return a * b; });
}

/* [[Opcode::DIV, 0x08, Group::MATH]] */
static void h_div(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.size() < 2)) { vm.halted_ = true; return; }
    const value_t bv = vm.eval_.values.back();
    const vtype   bt = vm.eval_.types.back();
    if (unlikely(bv == 0)) { vm.halted_ = true; return; }
    vm.eval_.values.pop_back();
    vm.eval_.types.pop_back();
    value_t& av = vm.eval_.values.back();
    vtype&   at = vm.eval_.types.back();
    const vtype rt = promote(at, bt);
    av = truncate_to_type(av / bv, rt);
    at = rt;
}

/* [[Opcode::MOD, 0x09, Group::MATH]] */
static void h_mod(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.size() < 2)) { vm.halted_ = true; return; }
    const value_t bv = vm.eval_.values.back();
    const vtype   bt = vm.eval_.types.back();
    if (unlikely(bv == 0)) { vm.halted_ = true; return; }
    vm.eval_.values.pop_back();
    vm.eval_.types.pop_back();
    value_t& av = vm.eval_.values.back();
    vtype&   at = vm.eval_.types.back();
    const vtype rt = promote(at, bt);
    av = truncate_to_type(av % bv, rt);
    at = rt;
}

#endif // ENABLE_MATH
