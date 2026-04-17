#ifndef VARABLIZER_VTYPES_H
#define VARABLIZER_VTYPES_H

#include <cstdint>

namespace execute
{
    // Width of a value in bytes (power of 2)
    enum class vwidth : std::uint8_t
    {
        W8  = 0,   // b_8,  y_1       — 1 byte
        W16 = 1,   // b_16, w_1       — 2 bytes
        W32 = 2,   // b_32, y_4, d_1  — 4 bytes
        W64 = 3,   // b_64, q_1       — 8 bytes
    };

    // Kind (semantic type)
    enum class vkind : std::uint8_t
    {
        INT_S = 0,   // signed int (default)
        INT_U = 1,   // unsigned int
        CHAR  = 2,   // character
        RAW   = 3,   // raw bytes (untyped)
        PTR   = 4,   // pointer (tagged: region + offset)
    };

    // Packed type descriptor — 1 byte
    //   bits [7:6] = width  (2 bits)
    //   bits [5:3] = kind   (3 bits)
    //   bits [2:0] = reserved
    struct vtype
    {
        std::uint8_t bits;

        constexpr vtype() noexcept : bits(0) {}

        constexpr vtype(vwidth w, vkind k) noexcept
            : bits(static_cast<std::uint8_t>(
                (static_cast<std::uint8_t>(w) << 6) |
                (static_cast<std::uint8_t>(k) << 3)
              )) {}

        [[nodiscard]] constexpr vwidth width() const noexcept
        {
            return static_cast<vwidth>(bits >> 6);
        }

        [[nodiscard]] constexpr vkind kind() const noexcept
        {
            return static_cast<vkind>((bits >> 3) & 0x07);
        }

        [[nodiscard]] constexpr bool is_signed() const noexcept
        {
            return kind() == vkind::INT_S;
        }

        [[nodiscard]] constexpr bool is_unsigned() const noexcept
        {
            return kind() == vkind::INT_U;
        }

        [[nodiscard]] constexpr bool is_ptr() const noexcept
        {
            return kind() == vkind::PTR;
        }

        // Size in bytes: 1, 2, 4, or 8
        [[nodiscard]] constexpr std::uint8_t byte_width() const noexcept
        {
            return static_cast<std::uint8_t>(1u << static_cast<std::uint8_t>(width()));
        }

        // Size in bits: 8, 16, 32, or 64
        [[nodiscard]] constexpr std::uint8_t bit_width() const noexcept
        {
            return static_cast<std::uint8_t>(byte_width() * 8);
        }

        // Bitmask for truncation after arithmetic
        [[nodiscard]] constexpr std::uint64_t mask() const noexcept
        {
            switch (width())
            {
                case vwidth::W8:  return 0xFFULL;
                case vwidth::W16: return 0xFFFFULL;
                case vwidth::W32: return 0xFFFF'FFFFULL;
                case vwidth::W64: return 0xFFFF'FFFF'FFFF'FFFFULL;
            }
            return 0xFFFF'FFFF'FFFF'FFFFULL;
        }

        [[nodiscard]] constexpr bool operator==(vtype other) const noexcept
        {
            return bits == other.bits;
        }

        [[nodiscard]] constexpr bool operator!=(vtype other) const noexcept
        {
            return bits != other.bits;
        }
    };

    static_assert(sizeof(vtype) == 1, "vtype must be exactly 1 byte");

    // Common type constants
    namespace types
    {
        inline constexpr vtype I8   = vtype(vwidth::W8,  vkind::INT_S);
        inline constexpr vtype I16  = vtype(vwidth::W16, vkind::INT_S);
        inline constexpr vtype I32  = vtype(vwidth::W32, vkind::INT_S);
        inline constexpr vtype I64  = vtype(vwidth::W64, vkind::INT_S);

        inline constexpr vtype U8   = vtype(vwidth::W8,  vkind::INT_U);
        inline constexpr vtype U16  = vtype(vwidth::W16, vkind::INT_U);
        inline constexpr vtype U32  = vtype(vwidth::W32, vkind::INT_U);
        inline constexpr vtype U64  = vtype(vwidth::W64, vkind::INT_U);

        inline constexpr vtype CHAR = vtype(vwidth::W8,  vkind::CHAR);
        inline constexpr vtype RAW  = vtype(vwidth::W8,  vkind::RAW);
        inline constexpr vtype PTR  = vtype(vwidth::W64, vkind::PTR);
    }

    // Promote two types for a binary operation:
    //   - width = max(a, b)
    //   - kind  = unsigned wins over signed (C semantics), else keep signed
    [[nodiscard]] constexpr vtype promote(vtype a, vtype b) noexcept
    {
        vwidth w = (a.width() >= b.width()) ? a.width() : b.width();

        vkind k;
        if (a.kind() == vkind::INT_U || b.kind() == vkind::INT_U)
            k = vkind::INT_U;
        else
            k = vkind::INT_S;

        return vtype(w, k);
    }

    // Apply mask + sign-extension to a raw value given a type
    [[nodiscard]] constexpr std::int64_t truncate_to_type(std::int64_t raw, vtype t) noexcept
    {
        std::int64_t masked = raw & static_cast<std::int64_t>(t.mask());

        if (t.is_signed() && t.width() != vwidth::W64)
        {
            std::uint8_t shift = static_cast<std::uint8_t>(64 - t.bit_width());
            masked = (masked << shift) >> shift; // arithmetic right shift = sign extension
        }

        return masked;
    }

    // Tagged pointer: region in top 2 bits, offset in lower 62 bits
    enum class ptr_region : std::uint8_t
    {
        HEAP   = 0,  // 00
        LOCALS = 1,  // 01
        CODE   = 2,  // 10
    };

    struct tagged_ptr
    {
        static constexpr std::uint64_t REGION_SHIFT = 62;
        static constexpr std::uint64_t OFFSET_MASK  = (1ULL << 62) - 1;

        [[nodiscard]] static constexpr std::int64_t encode(ptr_region r, std::uint64_t offset) noexcept
        {
            return static_cast<std::int64_t>(
                (static_cast<std::uint64_t>(r) << REGION_SHIFT) | (offset & OFFSET_MASK)
            );
        }

        [[nodiscard]] static constexpr ptr_region region(std::int64_t raw) noexcept
        {
            return static_cast<ptr_region>(
                static_cast<std::uint64_t>(raw) >> REGION_SHIFT
            );
        }

        [[nodiscard]] static constexpr std::uint64_t offset(std::int64_t raw) noexcept
        {
            return static_cast<std::uint64_t>(raw) & OFFSET_MASK;
        }

        [[nodiscard]] static constexpr bool is_null(std::int64_t raw) noexcept
        {
            return raw == 0;
        }
    };

} // namespace execute

#endif // VARABLIZER_VTYPES_H
