#ifndef VARABLIZER_OPCODES_H
#define VARABLIZER_OPCODES_H

#include "vtypes.h"

#include <cstdint>
#include <vector>

namespace execute
{
    using value_t = std::int64_t;

    enum class Group : std::uint8_t
    {
        CONTROL,
        STACK,
        MATH,
        DEBUG,
        FLOW,
        IO,
        LOCALS,
        HEAP,
        CALL,
        BITWISE,
        COMPARE,
        MISC,
    };

    enum class opcode : std::uint8_t
    {
        // CONTROL (0x00-0x01)
        NOP  = 0x00,
        HALT = 0x01,

        // STACK (0x02-0x04)
        PUSH = 0x02,
        DUP  = 0x03,
        POP  = 0x04,

        // MATH (0x05-0x09)
        ADD  = 0x05,
        SUB  = 0x06,
        MUL  = 0x07,
        DIV  = 0x08,
        MOD  = 0x09,

        // DEBUG (0x0A)
        DD   = 0x0A,

        // FLOW — legacy conditional jumps (0x0B-0x18)
        GOTO   = 0x0B,
        GOTO_E = 0x0C,
        EQ     = 0x0D,
        EQ_E   = 0x0E,
        NEQ    = 0x0F,
        NEQ_E  = 0x10,
        LT     = 0x11,
        LT_E   = 0x12,
        LTE    = 0x13,
        LTE_E  = 0x14,
        GT     = 0x15,
        GT_E   = 0x16,
        GTE    = 0x17,
        GTE_E  = 0x18,

        // IO (0x19-0x1A)
        COUT      = 0x19,
        CIN       = 0x1A,

        // LOCALS (0x1B-0x1F)
        PUSH_T      = 0x1B,  // push typed immediate (vtype in byte 0 of operand, value in bytes 1-7)
        LOAD_LOCAL  = 0x1C,  // push locals_[fbp_ + slot]
        STORE_LOCAL = 0x1D,  // pop eval_ → locals_[fbp_ + slot]
        LEA_LOCAL   = 0x1E,  // push tagged_ptr(LOCALS, fbp_ + slot)
        CAST        = 0x1F,  // re-type top of eval_ (target vtype in operand low byte)

        // HEAP (0x20-0x2D)
        ALLOC        = 0x20,  // pop size (bytes), allocate heap, push ptr
        DEALLOC      = 0x21,  // pop ptr, free heap block
        LOAD_HEAP_8  = 0x22,  // pop addr, read 1 byte from heap, sign/zero-extend, push
        LOAD_HEAP_16 = 0x23,  // pop addr, read 2 bytes (LE), extend, push
        LOAD_HEAP_32 = 0x24,  // pop addr, read 4 bytes, extend, push
        LOAD_HEAP_64 = 0x25,  // pop addr, read 8 bytes, push
        STORE_HEAP_8  = 0x26, // pop val + addr, write 1 byte
        STORE_HEAP_16 = 0x27, // pop val + addr, write 2 bytes (LE)
        STORE_HEAP_32 = 0x28, // pop val + addr, write 4 bytes
        STORE_HEAP_64 = 0x29, // pop val + addr, write 8 bytes
        DEREF_8      = 0x2A,  // pop tagged_ptr, read 1 byte from region
        DEREF_16     = 0x2B,  // pop tagged_ptr, read 2 bytes from region
        DEREF_32     = 0x2C,  // pop tagged_ptr, read 4 bytes from region
        DEREF_64     = 0x2D,  // pop tagged_ptr, read 8 bytes from region

        // CALL (0x30-0x36)
        CALL         = 0x30,  // operand = target_ip | (argc << 32) | (total_locals << 48)
        CALL_IND     = 0x31,  // pop target_ip from stack, call indirectly
        RET          = 0x32,  // pop return value, restore frame, push result
        RET_VOID     = 0x33,  // restore frame, no return value
        FRAME        = 0x34,  // operand = N: reserve N local slots in current frame
        CALL_CLOSURE = 0x35,  // pop closure ptr, bind captures, call
        ALLOC_CLOSURE= 0x36,  // pop code_addr + captures, allocate closure, push ptr

        // BITWISE (0x38-0x40)
        NEG = 0x38,  // negate top
        INC = 0x39,  // top + 1
        DEC = 0x3A,  // top - 1
        AND = 0x3B,  // bitwise AND of top two
        OR  = 0x3C,  // bitwise OR
        XOR = 0x3D,  // bitwise XOR
        NOT = 0x3E,  // bitwise NOT
        SHL = 0x3F,  // shift left:  pop count, pop value, push value << count
        SHR = 0x40,  // shift right: arithmetic (signed), pop count, pop value

        // FLOW extensions (0x60-0x61)
        JZ  = 0x60,  // pop top, jump to operand if zero
        JNZ = 0x61,  // pop top, jump to operand if non-zero

        // COMPARE (0x54-0x59) — push boolean result (1/0) onto stack
        CMP_EQ  = 0x54,
        CMP_NEQ = 0x55,
        CMP_LT  = 0x56,
        CMP_LTE = 0x57,
        CMP_GT  = 0x58,
        CMP_GTE = 0x59,

        // IO extensions (0x68-0x69)
        COUT_CHAR = 0x68,  // pop value, print as ASCII char
        COUT_STR  = 0x69,  // pop heap addr, print null-terminated string

        COUNT = 0x6A,
    };

    struct instruction
    {
        opcode  op      = opcode::NOP;
        value_t operand = 0;
    };

    using program = std::vector<instruction>;

    inline constexpr std::size_t opcode_count = static_cast<std::size_t>(opcode::COUNT);

    [[nodiscard]] constexpr bool is_valid_opcode(opcode op) noexcept
    {
        return static_cast<std::uint8_t>(op) < static_cast<std::uint8_t>(opcode::COUNT);
    }

} // namespace execute

#endif // VARABLIZER_OPCODES_H
