#if ENABLE_IO

/* [[Opcode::COUT, 0x19, Group::IO]] */
static void h_cout(assembly& vm) noexcept
{
    std::cout << vm.top() << std::flush;
}

/* [[Opcode::CIN, 0x1A, Group::IO]] */
static void h_cin(assembly& vm) noexcept
{
    value_t v{};
    std::cin >> v;
    std::cin.clear();
    vm.push(v);
}

/* [[Opcode::COUT_CHAR, 0x68, Group::IO]] */
static void h_cout_char(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.empty())) { vm.halted_ = true; return; }
    const auto ch = static_cast<char>(vm.eval_.pop_value() & 0xFF);
    std::cout << ch << std::flush;
}

/* [[Opcode::COUT_STR, 0x69, Group::IO]] */
static void h_cout_str(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.empty())) { vm.halted_ = true; return; }
    const value_t raw = vm.eval_.pop_value();
    if (unlikely(tagged_ptr::region(raw) != ptr_region::HEAP))
    {
        vm.halted_ = true;
        return;
    }
    const std::uint64_t addr = tagged_ptr::offset(raw);
    if (unlikely(addr >= vm.heap_.size())) { vm.halted_ = true; return; }
    const char* str = reinterpret_cast<const char*>(vm.heap_.data() + addr);
    const std::size_t max_len = vm.heap_.size() - static_cast<std::size_t>(addr);
    std::size_t len = 0;
    while (len < max_len && str[len] != '\0') ++len;
    std::cout.write(str, static_cast<std::streamsize>(len));
    std::cout << std::flush;
}

#endif // ENABLE_IO
