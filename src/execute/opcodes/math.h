#if ENABLE_MATH

/* [[Opcode::ADD, 0x05, Group::MATH]] */
static void h_add(assembly& vm) noexcept
{
    vm.binary([](value_t a, value_t b) noexcept { return a + b; });
}

/* [[Opcode::SUB, 0x06, Group::MATH]] */
static void h_sub(assembly& vm) noexcept
{
    vm.binary([](value_t a, value_t b) noexcept { return a - b; });
}

/* [[Opcode::MUL, 0x07, Group::MATH]] */
static void h_mul(assembly& vm) noexcept
{
    vm.binary([](value_t a, value_t b) noexcept { return a * b; });
}

/* [[Opcode::DIV, 0x08, Group::MATH]] */
static void h_div(assembly& vm) noexcept
{
    value_t b = vm.pop();
    if(unlikely(b == 0)) {
        vm.halted_ = true;
        return;
    }
    value_t a = vm.pop();
    vm.push(a / b);
}

/* [[Opcode::MOD, 0x09, Group::MATH]] */
static void h_mod(assembly& vm) noexcept
{
    value_t b = vm.pop();
    if(unlikely(b == 0)) {
        vm.halted_ = true;
        return;
    }
    value_t a = vm.pop();
    vm.push(a % b);
}

#endif // ENABLE_MATH
