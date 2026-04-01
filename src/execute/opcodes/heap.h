#if ENABLE_HEAP

/* [[Opcode::ALLOC, 0x20, Group::HEAP]] */
// Pop size (bytes), allocate on heap, push tagged heap pointer
static void h_alloc(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.empty()))
    {
        vm.halted_ = true;
        return;
    }

    const auto size = static_cast<std::size_t>(vm.eval_.pop_value());
    if (unlikely(size == 0 || size > 64 * 1024 * 1024)) // max 64 MB per alloc
    {
        vm.halted_ = true;
        return;
    }

    // Store a 8-byte header before the data: [size:u64]
    const std::size_t header = sizeof(std::uint64_t);
    const std::size_t base   = vm.heap_.size();

    vm.heap_.resize(base + header + size, 0);

    // Write size into header
    std::uint64_t sz = static_cast<std::uint64_t>(size);
    std::memcpy(vm.heap_.data() + base, &sz, sizeof(sz));

    // The user-visible address points past the header
    const std::uint64_t addr = static_cast<std::uint64_t>(base + header);
    vm.eval_.push(tagged_ptr::encode(ptr_region::HEAP, addr), types::PTR);
}

/* [[Opcode::DEALLOC, 0x21, Group::HEAP]] */
// Pop heap pointer — in this simple bump allocator we just validate and ignore
static void h_dealloc(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.empty()))
    {
        vm.halted_ = true;
        return;
    }
    (void)vm.eval_.pop_value(); // bump allocator: memory released at VM shutdown
}

// Helper: resolve a heap address from the top of eval_
// Returns pointer into heap_ or nullptr on error
static inline const std::uint8_t* heap_read_ptr(assembly& vm, std::size_t byte_count) noexcept
{
    if (unlikely(vm.eval_.empty())) { vm.halted_ = true; return nullptr; }

    const auto raw = vm.eval_.pop_value();

    if (unlikely(tagged_ptr::region(raw) != ptr_region::HEAP))
    {
        vm.halted_ = true;
        return nullptr;
    }

    const std::uint64_t addr = tagged_ptr::offset(raw);
    if (unlikely(addr + byte_count > vm.heap_.size()))
    {
        vm.halted_ = true;
        return nullptr;
    }

    return vm.heap_.data() + addr;
}

static inline std::uint8_t* heap_write_ptr(assembly& vm, std::size_t byte_count) noexcept
{
    if (unlikely(vm.eval_.empty())) { vm.halted_ = true; return nullptr; }

    const auto raw = vm.eval_.pop_value();

    if (unlikely(tagged_ptr::region(raw) != ptr_region::HEAP))
    {
        vm.halted_ = true;
        return nullptr;
    }

    const std::uint64_t addr = tagged_ptr::offset(raw);
    if (unlikely(addr + byte_count > vm.heap_.size()))
    {
        vm.halted_ = true;
        return nullptr;
    }

    return vm.heap_.data() + addr;
}

/* [[Opcode::LOAD_HEAP_8, 0x22, Group::HEAP]] */
static void h_load_heap_8(assembly& vm) noexcept
{
    const auto* ptr = heap_read_ptr(vm, 1);
    if (unlikely(!ptr)) return;
    // sign-extend from 8 bits
    const value_t val = static_cast<value_t>(static_cast<std::int8_t>(*ptr));
    vm.eval_.push(val, types::I8);
}

/* [[Opcode::LOAD_HEAP_16, 0x23, Group::HEAP]] */
static void h_load_heap_16(assembly& vm) noexcept
{
    const auto* ptr = heap_read_ptr(vm, 2);
    if (unlikely(!ptr)) return;
    std::uint16_t raw{};
    std::memcpy(&raw, ptr, 2);
    const value_t val = static_cast<value_t>(static_cast<std::int16_t>(raw));
    vm.eval_.push(val, types::I16);
}

/* [[Opcode::LOAD_HEAP_32, 0x24, Group::HEAP]] */
static void h_load_heap_32(assembly& vm) noexcept
{
    const auto* ptr = heap_read_ptr(vm, 4);
    if (unlikely(!ptr)) return;
    std::uint32_t raw{};
    std::memcpy(&raw, ptr, 4);
    const value_t val = static_cast<value_t>(static_cast<std::int32_t>(raw));
    vm.eval_.push(val, types::I32);
}

/* [[Opcode::LOAD_HEAP_64, 0x25, Group::HEAP]] */
static void h_load_heap_64(assembly& vm) noexcept
{
    const auto* ptr = heap_read_ptr(vm, 8);
    if (unlikely(!ptr)) return;
    value_t val{};
    std::memcpy(&val, ptr, 8);
    vm.eval_.push(val, types::I64);
}

/* [[Opcode::STORE_HEAP_8, 0x26, Group::HEAP]] */
// Stack order: ..., addr, value  (value on top)
static void h_store_heap_8(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.size() < 2)) { vm.halted_ = true; return; }
    const auto val = static_cast<std::uint8_t>(vm.eval_.pop_value() & 0xFF);
    auto* ptr = heap_write_ptr(vm, 1);
    if (unlikely(!ptr)) return;
    *ptr = val;
}

/* [[Opcode::STORE_HEAP_16, 0x27, Group::HEAP]] */
static void h_store_heap_16(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.size() < 2)) { vm.halted_ = true; return; }
    const auto val = static_cast<std::uint16_t>(vm.eval_.pop_value() & 0xFFFF);
    auto* ptr = heap_write_ptr(vm, 2);
    if (unlikely(!ptr)) return;
    std::memcpy(ptr, &val, 2);
}

/* [[Opcode::STORE_HEAP_32, 0x28, Group::HEAP]] */
static void h_store_heap_32(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.size() < 2)) { vm.halted_ = true; return; }
    const auto val = static_cast<std::uint32_t>(vm.eval_.pop_value() & 0xFFFF'FFFF);
    auto* ptr = heap_write_ptr(vm, 4);
    if (unlikely(!ptr)) return;
    std::memcpy(ptr, &val, 4);
}

/* [[Opcode::STORE_HEAP_64, 0x29, Group::HEAP]] */
static void h_store_heap_64(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.size() < 2)) { vm.halted_ = true; return; }
    const value_t val = vm.eval_.pop_value();
    auto* ptr = heap_write_ptr(vm, 8);
    if (unlikely(!ptr)) return;
    std::memcpy(ptr, &val, 8);
}

/* [[Opcode::DEREF_8, 0x2A, Group::HEAP]] */
// Pop tagged_ptr, route to heap or locals, read 1 byte
static void h_deref_8(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.empty())) { vm.halted_ = true; return; }
    const value_t raw = vm.eval_.pop_value();

    value_t result{};
    switch (tagged_ptr::region(raw))
    {
        case ptr_region::HEAP:
        {
            const std::uint64_t addr = tagged_ptr::offset(raw);
            if (unlikely(addr + 1 > vm.heap_.size())) { vm.halted_ = true; return; }
            result = static_cast<value_t>(static_cast<std::int8_t>(vm.heap_[addr]));
            break;
        }
        case ptr_region::LOCALS:
        {
            const std::uint64_t slot = tagged_ptr::offset(raw);
            if (unlikely(slot >= vm.local_vals_.size())) { vm.halted_ = true; return; }
            result = vm.local_vals_[slot] & 0xFF;
            result = static_cast<value_t>(static_cast<std::int8_t>(static_cast<std::uint8_t>(result)));
            break;
        }
        default:
            vm.halted_ = true;
            return;
    }
    vm.eval_.push(result, types::I8);
}

/* [[Opcode::DEREF_16, 0x2B, Group::HEAP]] */
static void h_deref_16(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.empty())) { vm.halted_ = true; return; }
    const value_t raw = vm.eval_.pop_value();

    value_t result{};
    switch (tagged_ptr::region(raw))
    {
        case ptr_region::HEAP:
        {
            const std::uint64_t addr = tagged_ptr::offset(raw);
            if (unlikely(addr + 2 > vm.heap_.size())) { vm.halted_ = true; return; }
            std::uint16_t tmp{};
            std::memcpy(&tmp, vm.heap_.data() + addr, 2);
            result = static_cast<value_t>(static_cast<std::int16_t>(tmp));
            break;
        }
        case ptr_region::LOCALS:
        {
            const std::uint64_t slot = tagged_ptr::offset(raw);
            if (unlikely(slot >= vm.local_vals_.size())) { vm.halted_ = true; return; }
            result = vm.local_vals_[slot] & 0xFFFF;
            result = static_cast<value_t>(static_cast<std::int16_t>(static_cast<std::uint16_t>(result)));
            break;
        }
        default:
            vm.halted_ = true;
            return;
    }
    vm.eval_.push(result, types::I16);
}

/* [[Opcode::DEREF_32, 0x2C, Group::HEAP]] */
static void h_deref_32(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.empty())) { vm.halted_ = true; return; }
    const value_t raw = vm.eval_.pop_value();

    value_t result{};
    switch (tagged_ptr::region(raw))
    {
        case ptr_region::HEAP:
        {
            const std::uint64_t addr = tagged_ptr::offset(raw);
            if (unlikely(addr + 4 > vm.heap_.size())) { vm.halted_ = true; return; }
            std::uint32_t tmp{};
            std::memcpy(&tmp, vm.heap_.data() + addr, 4);
            result = static_cast<value_t>(static_cast<std::int32_t>(tmp));
            break;
        }
        case ptr_region::LOCALS:
        {
            const std::uint64_t slot = tagged_ptr::offset(raw);
            if (unlikely(slot >= vm.local_vals_.size())) { vm.halted_ = true; return; }
            result = vm.local_vals_[slot] & 0xFFFF'FFFF;
            result = static_cast<value_t>(static_cast<std::int32_t>(static_cast<std::uint32_t>(result)));
            break;
        }
        default:
            vm.halted_ = true;
            return;
    }
    vm.eval_.push(result, types::I32);
}

/* [[Opcode::DEREF_64, 0x2D, Group::HEAP]] */
static void h_deref_64(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.empty())) { vm.halted_ = true; return; }
    const value_t raw = vm.eval_.pop_value();

    value_t result{};
    switch (tagged_ptr::region(raw))
    {
        case ptr_region::HEAP:
        {
            const std::uint64_t addr = tagged_ptr::offset(raw);
            if (unlikely(addr + 8 > vm.heap_.size())) { vm.halted_ = true; return; }
            std::memcpy(&result, vm.heap_.data() + addr, 8);
            break;
        }
        case ptr_region::LOCALS:
        {
            const std::uint64_t slot = tagged_ptr::offset(raw);
            if (unlikely(slot >= vm.local_vals_.size())) { vm.halted_ = true; return; }
            result = vm.local_vals_[slot];
            break;
        }
        default:
            vm.halted_ = true;
            return;
    }
    vm.eval_.push(result, types::I64);
}

/* [[Opcode::STORE_DEREF_8, 0x2E, Group::HEAP]] */
// Stack order: ..., addr, value (value on top)
// Writes 1 byte through a tagged pointer (HEAP or LOCALS region)
static void h_store_deref_8(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.size() < 2)) { vm.halted_ = true; return; }
    const auto val = static_cast<std::uint8_t>(vm.eval_.pop_value() & 0xFF);
    const value_t raw = vm.eval_.pop_value();
    switch (tagged_ptr::region(raw))
    {
        case ptr_region::HEAP:
        {
            const std::uint64_t addr = tagged_ptr::offset(raw);
            if (unlikely(addr + 1 > vm.heap_.size())) { vm.halted_ = true; return; }
            vm.heap_[addr] = val;
            break;
        }
        case ptr_region::LOCALS:
        {
            const std::uint64_t slot = tagged_ptr::offset(raw);
            if (unlikely(slot >= vm.local_vals_.size())) { vm.halted_ = true; return; }
            vm.local_vals_[slot] = (vm.local_vals_[slot] & ~0xFFLL) | val;
            break;
        }
        default: vm.halted_ = true; return;
    }
}

/* [[Opcode::STORE_DEREF_16, 0x2F, Group::HEAP]] */
static void h_store_deref_16(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.size() < 2)) { vm.halted_ = true; return; }
    const auto val = static_cast<std::uint16_t>(vm.eval_.pop_value() & 0xFFFF);
    const value_t raw = vm.eval_.pop_value();
    switch (tagged_ptr::region(raw))
    {
        case ptr_region::HEAP:
        {
            const std::uint64_t addr = tagged_ptr::offset(raw);
            if (unlikely(addr + 2 > vm.heap_.size())) { vm.halted_ = true; return; }
            std::memcpy(vm.heap_.data() + addr, &val, 2);
            break;
        }
        case ptr_region::LOCALS:
        {
            const std::uint64_t slot = tagged_ptr::offset(raw);
            if (unlikely(slot >= vm.local_vals_.size())) { vm.halted_ = true; return; }
            vm.local_vals_[slot] = (vm.local_vals_[slot] & ~0xFFFFLL) | val;
            break;
        }
        default: vm.halted_ = true; return;
    }
}

/* [[Opcode::STORE_DEREF_32, 0x50, Group::HEAP]] */
static void h_store_deref_32(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.size() < 2)) { vm.halted_ = true; return; }
    const auto val = static_cast<std::uint32_t>(vm.eval_.pop_value() & 0xFFFF'FFFF);
    const value_t raw = vm.eval_.pop_value();
    switch (tagged_ptr::region(raw))
    {
        case ptr_region::HEAP:
        {
            const std::uint64_t addr = tagged_ptr::offset(raw);
            if (unlikely(addr + 4 > vm.heap_.size())) { vm.halted_ = true; return; }
            std::memcpy(vm.heap_.data() + addr, &val, 4);
            break;
        }
        case ptr_region::LOCALS:
        {
            const std::uint64_t slot = tagged_ptr::offset(raw);
            if (unlikely(slot >= vm.local_vals_.size())) { vm.halted_ = true; return; }
            vm.local_vals_[slot] = (vm.local_vals_[slot] & ~0xFFFF'FFFFLL) | val;
            break;
        }
        default: vm.halted_ = true; return;
    }
}

/* [[Opcode::STORE_DEREF_64, 0x51, Group::HEAP]] */
static void h_store_deref_64(assembly& vm) noexcept
{
    if (unlikely(vm.eval_.size() < 2)) { vm.halted_ = true; return; }
    const value_t val = vm.eval_.pop_value();
    const value_t raw = vm.eval_.pop_value();
    switch (tagged_ptr::region(raw))
    {
        case ptr_region::HEAP:
        {
            const std::uint64_t addr = tagged_ptr::offset(raw);
            if (unlikely(addr + 8 > vm.heap_.size())) { vm.halted_ = true; return; }
            std::memcpy(vm.heap_.data() + addr, &val, 8);
            break;
        }
        case ptr_region::LOCALS:
        {
            const std::uint64_t slot = tagged_ptr::offset(raw);
            if (unlikely(slot >= vm.local_vals_.size())) { vm.halted_ = true; return; }
            vm.local_vals_[slot] = val;
            break;
        }
        default: vm.halted_ = true; return;
    }
}

#endif // ENABLE_HEAP
