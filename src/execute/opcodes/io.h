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

#endif // ENABLE_IO
