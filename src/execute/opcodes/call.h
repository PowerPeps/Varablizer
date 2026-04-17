#if ENABLE_CALL

/* [[Opcode::CALL, 0x30, Group::CALL]] */
// Operand encoding (8 bytes):
//   bits  0-31 : target instruction address
//   bits 32-47 : argument count (argc)
//   bits 48-63 : total local slots (args + locals declared in function)
static void h_call(assembly& vm) noexcept
{
    const value_t operand = vm.program_[vm.ip_].operand;

    const auto target      = static_cast<std::uint32_t>(operand & 0xFFFF'FFFF);
    const auto argc        = static_cast<std::uint16_t>((operand >> 32) & 0xFFFF);
    const auto total_slots = static_cast<std::uint16_t>((operand >> 48) & 0xFFFF);

    if (unlikely(vm.eval_.size() < argc))
    {
        vm.halted_ = true;
        return;
    }

    // Build the call frame
    call_frame frame;
    frame.return_ip    = static_cast<std::uint32_t>(vm.ip_ + 1);
    frame.locals_base  = static_cast<std::uint32_t>(vm.local_vals_.size());
    frame.locals_count = total_slots;
    frame.args_count   = argc;

    // Reserve local slots (zero-initialised)
    vm.local_vals_.resize(vm.local_vals_.size() + total_slots, 0);
    vm.local_types_.resize(vm.local_types_.size() + total_slots, vtype{});

    // Pop arguments from eval stack into the first `argc` local slots
    // Arguments are pushed left-to-right, so arg0 is deepest — pop in reverse
    for (int i = static_cast<int>(argc) - 1; i >= 0; --i)
    {
        auto [val, typ] = vm.eval_.pop();
        vm.local_vals_[frame.locals_base + static_cast<std::uint32_t>(i)]  = val;
        vm.local_types_[frame.locals_base + static_cast<std::uint32_t>(i)] = typ;
    }

    vm.calls_.push_back(frame);
    vm.fbp_ = frame.locals_base;

    // Jump: -1 because step() will ++ip_
    vm.ip_ = static_cast<std::size_t>(target) - 1;
}

/* [[Opcode::CALL_IND, 0x31, Group::CALL]] */
// Pop a CODE-tagged pointer from eval, call that address
// Operand bits 32-47 = argc, bits 48-63 = total_slots
static void h_call_ind(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.empty())) { vm.halted_ = true; return; }

    const value_t operand = vm.program_[vm.ip_].operand;
    const auto argc        = static_cast<std::uint16_t>((operand >> 32) & 0xFFFF);
    const auto total_slots = static_cast<std::uint16_t>((operand >> 48) & 0xFFFF);

    const value_t addr_raw = vm.eval_.pop_value();
    if (unlikely(tagged_ptr::region(addr_raw) != ptr_region::CODE))
    {
        vm.halted_ = true;
        return;
    }
    const auto target = static_cast<std::uint32_t>(tagged_ptr::offset(addr_raw));

    if (unlikely(vm.eval_.size() < argc)) { vm.halted_ = true; return; }

    call_frame frame;
    frame.return_ip    = static_cast<std::uint32_t>(vm.ip_ + 1);
    frame.locals_base  = static_cast<std::uint32_t>(vm.local_vals_.size());
    frame.locals_count = total_slots;
    frame.args_count   = argc;

    vm.local_vals_.resize(vm.local_vals_.size() + total_slots, 0);
    vm.local_types_.resize(vm.local_types_.size() + total_slots, vtype{});

    for (int i = static_cast<int>(argc) - 1; i >= 0; --i)
    {
        auto [val, typ] = vm.eval_.pop();
        vm.local_vals_[frame.locals_base + static_cast<std::uint32_t>(i)]  = val;
        vm.local_types_[frame.locals_base + static_cast<std::uint32_t>(i)] = typ;
    }

    vm.calls_.push_back(frame);
    vm.fbp_ = frame.locals_base;
    vm.ip_ = static_cast<std::size_t>(target) - 1;
}

/* [[Opcode::RET, 0x32, Group::CALL]] */
// Return value is on top of eval_; pop frame and push result back
static void h_ret(assembly& vm) noexcept
{
    if (unlikely(vm.calls_.empty())) { vm.halted_ = true; return; }

    // Capture return value before destroying the frame
    value_t ret_val{};
    vtype   ret_typ{};
    const bool has_val = !vm.eval_.empty();
    if (has_val)
    {
        auto [v, t] = vm.eval_.pop();
        ret_val = v;
        ret_typ = t;
    }

    const call_frame& frame = vm.calls_.back();

    // Shrink locals
    vm.local_vals_.resize(frame.locals_base);
    vm.local_types_.resize(frame.locals_base);

    // Restore frame pointer and instruction pointer
    vm.ip_  = static_cast<std::size_t>(frame.return_ip) - 1; // step() will ++ip_
    vm.fbp_ = vm.calls_.size() >= 2
                ? vm.calls_[vm.calls_.size() - 2].locals_base
                : 0;

    vm.calls_.pop_back();

    if (has_val)
        vm.eval_.push(ret_val, ret_typ);
}

/* [[Opcode::RET_VOID, 0x33, Group::CALL]] */
static void h_ret_void(assembly& vm) noexcept
{
    if (unlikely(vm.calls_.empty())) { vm.halted_ = true; return; }

    const call_frame& frame = vm.calls_.back();

    vm.local_vals_.resize(frame.locals_base);
    vm.local_types_.resize(frame.locals_base);

    vm.ip_  = static_cast<std::size_t>(frame.return_ip) - 1;
    vm.fbp_ = vm.calls_.size() >= 2
                ? vm.calls_[vm.calls_.size() - 2].locals_base
                : 0;

    vm.calls_.pop_back();
}

/* [[Opcode::FRAME, 0x34, Group::CALL]] */
// Reserve N additional local slots in the current frame (beyond args).
// Typically emitted at function entry when there are declared locals.
static void h_frame(assembly& vm) noexcept
{
    const auto n = static_cast<std::uint32_t>(vm.program_[vm.ip_].operand);
    vm.local_vals_.resize(vm.local_vals_.size() + n, 0);
    vm.local_types_.resize(vm.local_types_.size() + n, vtype{});

    if (!vm.calls_.empty())
        vm.calls_.back().locals_count += static_cast<std::uint16_t>(n);
}

/* [[Opcode::CALL_CLOSURE, 0x35, Group::CALL]] */
// Pop closure ptr (heap), extract code_address + captures, call
// Closure heap layout:
//   bytes 0-7   : code_address (uint64)
//   bytes 8-9   : capture_count (uint16)
//   bytes 10+   : captured values (9 bytes each: 1 vtype + 8 value)
static void h_call_closure(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.empty())) { vm.halted_ = true; return; }

    const value_t operand = vm.program_[vm.ip_].operand;
    const auto argc        = static_cast<std::uint16_t>((operand >> 32) & 0xFFFF);
    const auto total_slots = static_cast<std::uint16_t>((operand >> 48) & 0xFFFF);

    const value_t closure_raw = vm.eval_.pop_value();
    if (unlikely(tagged_ptr::region(closure_raw) != ptr_region::HEAP))
    {
        vm.halted_ = true;
        return;
    }

    const std::uint64_t base_addr = tagged_ptr::offset(closure_raw);
    if (unlikely(base_addr + 10 > vm.heap_.size())) { vm.halted_ = true; return; }

    // Read code address
    std::uint64_t code_addr{};
    std::memcpy(&code_addr, vm.heap_.data() + base_addr, 8);

    // Read capture count
    std::uint16_t cap_count{};
    std::memcpy(&cap_count, vm.heap_.data() + base_addr + 8, 2);

    const std::size_t captures_start = base_addr + 10;
    if (unlikely(captures_start + cap_count * 9 > vm.heap_.size()))
    {
        vm.halted_ = true;
        return;
    }

    if (unlikely(vm.eval_.size() < argc)) { vm.halted_ = true; return; }

    const auto effective_slots = static_cast<std::uint16_t>(total_slots + cap_count);

    call_frame frame;
    frame.return_ip    = static_cast<std::uint32_t>(vm.ip_ + 1);
    frame.locals_base  = static_cast<std::uint32_t>(vm.local_vals_.size());
    frame.locals_count = effective_slots;
    frame.args_count   = argc;

    vm.local_vals_.resize(vm.local_vals_.size() + effective_slots, 0);
    vm.local_types_.resize(vm.local_types_.size() + effective_slots, vtype{});

    // Pop explicit args into first slots
    for (int i = static_cast<int>(argc) - 1; i >= 0; --i)
    {
        auto [val, typ] = vm.eval_.pop();
        vm.local_vals_[frame.locals_base + static_cast<std::uint32_t>(i)]  = val;
        vm.local_types_[frame.locals_base + static_cast<std::uint32_t>(i)] = typ;
    }

    // Copy captures into slots after args
    for (std::uint16_t c = 0; c < cap_count; ++c)
    {
        const std::size_t off = captures_start + c * 9;
        vtype t;
        t.bits = vm.heap_[off];
        value_t v{};
        std::memcpy(&v, vm.heap_.data() + off + 1, 8);

        const std::size_t slot = frame.locals_base + argc + c;
        vm.local_vals_[slot]  = v;
        vm.local_types_[slot] = t;
    }

    vm.calls_.push_back(frame);
    vm.fbp_ = frame.locals_base;
    vm.ip_  = static_cast<std::size_t>(code_addr) - 1;
}

/* [[Opcode::ALLOC_CLOSURE, 0x36, Group::CALL]] */
// Operand = capture_count (low 16 bits)
// Stack (bottom to top): code_addr (CODE ptr), cap0, cap1, ..., capN
// Allocates closure on heap, pushes closure ptr
static void h_alloc_closure(assembly& vm) noexcept
{
    const auto cap_count = static_cast<std::uint16_t>(vm.program_[vm.ip_].operand & 0xFFFF);

    if (unlikely(vm.eval_.size() < static_cast<std::size_t>(1 + cap_count)))
    {
        vm.halted_ = true;
        return;
    }

    // Collect captures (on stack in order cap0, cap1, ..., capN on top)
    struct capture_entry { vtype t; value_t v; };
    std::vector<capture_entry> caps(cap_count);
    for (int i = cap_count - 1; i >= 0; --i)
    {
        auto [v, t] = vm.eval_.pop();
        caps[static_cast<std::size_t>(i)] = { t, v };
    }

    // Pop code address
    const value_t code_raw = vm.eval_.pop_value();
    std::uint64_t code_addr{};
    if (tagged_ptr::region(code_raw) == ptr_region::CODE)
        code_addr = tagged_ptr::offset(code_raw);
    else
        code_addr = static_cast<std::uint64_t>(code_raw); // bare IP

    // Allocate: 8 (code_addr) + 2 (cap_count) + N*9
    const std::size_t closure_size = 10 + cap_count * 9;
    const std::size_t header = sizeof(std::uint64_t);
    const std::size_t base   = vm.heap_.size();

    vm.heap_.resize(base + header + closure_size, 0);

    // Write alloc header
    const std::uint64_t sz = static_cast<std::uint64_t>(closure_size);
    std::memcpy(vm.heap_.data() + base, &sz, 8);

    const std::size_t data = base + header;

    // Write code address
    std::memcpy(vm.heap_.data() + data, &code_addr, 8);

    // Write capture count
    std::memcpy(vm.heap_.data() + data + 8, &cap_count, 2);

    // Write captures
    for (std::uint16_t i = 0; i < cap_count; ++i)
    {
        const std::size_t off = data + 10 + i * 9;
        vm.heap_[off] = caps[i].t.bits;
        std::memcpy(vm.heap_.data() + off + 1, &caps[i].v, 8);
    }

    const std::uint64_t user_addr = static_cast<std::uint64_t>(data);
    vm.eval_.push(tagged_ptr::encode(ptr_region::HEAP, user_addr), types::PTR);
}

#endif // ENABLE_CALL
