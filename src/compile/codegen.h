#ifndef VARABLIZER_CODEGEN_H
#define VARABLIZER_CODEGEN_H

#include "ast.h"
#include "symbol_table.h"
#include "error.h"
#include "type_resolver.h"
#include "../execute/opcodes.h"

#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>

namespace compile
{
    using execute::program;
    using execute::instruction;
    using execute::opcode;
    using execute::vtype;

    class CodeGen
    {
    public:
        program compile(const Program& prog);

    private:
        // ── Output ──────────────────────────────────────────────────────────
        program out_;

        // ── Symbol / scope management ────────────────────────────────────────
        SymbolTable syms_;

        // ── Function registry ────────────────────────────────────────────────
        struct FnInfo
        {
            std::size_t ip          = 0;
            uint16_t    argc        = 0;
            uint16_t    total_slots = 0;
            vtype       return_type;
            bool        is_void     = false;
        };
        std::unordered_map<std::string, FnInfo> functions_;

        // ── Loop context (break / continue) ──────────────────────────────────
        struct LoopCtx
        {
            std::size_t              continue_ip = 0;
            std::vector<std::size_t> break_sites;
            std::vector<std::size_t> continue_sites;
        };
        std::vector<LoopCtx> loop_stack_;

        // ── Label support (goto) ─────────────────────────────────────────────
        std::unordered_map<std::string, std::size_t>              label_defs_;
        std::unordered_map<std::string, std::vector<std::size_t>> label_refs_;

        // ── Emit helpers ─────────────────────────────────────────────────────
        std::size_t emit(opcode op, int64_t operand = 0);
        void        patch(std::size_t idx, int64_t operand);
        std::size_t current_ip() const noexcept;

        // ── Pre-scan ─────────────────────────────────────────────────────────
        void collect_functions(const Program& prog);
        uint32_t count_locals_in_block(const Block& blk, uint32_t start) const;
        uint32_t count_locals_in_node(const AstNode* n, uint32_t start) const;

        // ── Code generation ──────────────────────────────────────────────────
        void gen_program(const Program&);
        void gen_function(const FunctionDecl&);
        void gen_block(const Block&);
        void gen_stmt(const AstNode&);
        void gen_expr(const AstNode&);           // leaves value on eval_
        void gen_cond(const AstNode&);           // leaves 0/1 on eval_
        void gen_assign_op(const std::string& op);  // emit ADD/SUB/etc. for compound assigns

        void resolve_labels();

        [[noreturn]] void error(const std::string& msg, const SourceLocation& loc);
    };

} // namespace compile

#endif // VARABLIZER_CODEGEN_H
