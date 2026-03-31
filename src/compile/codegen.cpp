#include "codegen.h"
#include <stdexcept>
#include <cassert>

namespace compile
{

using execute::opcode;
using execute::types::I64;

// ── Emit helpers ─────────────────────────────────────────────────────────────

std::size_t CodeGen::emit(opcode op, int64_t operand)
{
    std::size_t idx = out_.size();
    out_.push_back({ op, operand });
    return idx;
}

void CodeGen::patch(std::size_t idx, int64_t operand)
{
    out_[idx].operand = operand;
}

std::size_t CodeGen::current_ip() const noexcept
{
    return out_.size();
}

[[noreturn]] void CodeGen::error(const std::string& msg, const SourceLocation& loc)
{
    throw CompileError(msg, loc);
}

// ── Pre-scan: count local variable declarations in a function body ────────────

uint32_t CodeGen::count_locals_in_node(const AstNode* n, uint32_t start) const
{
    if (!n) return start;

    if (auto* vd = node_cast<VarDecl>(n))
    {
        (void)vd;
        return start + 1;
    }
    if (auto* blk = node_cast<Block>(n))
        return count_locals_in_block(*blk, start);
    if (auto* ifs = node_cast<IfStmt>(n))
    {
        start = count_locals_in_node(ifs->then_.get(), start);
        start = count_locals_in_node(ifs->else_.get(), start);
        return start;
    }
    if (auto* ws = node_cast<WhileStmt>(n))
        return count_locals_in_node(ws->body.get(), start);
    if (auto* fs = node_cast<ForStmt>(n))
    {
        start = count_locals_in_node(fs->init.get(), start);
        start = count_locals_in_node(fs->body.get(), start);
        return start;
    }
    if (auto* es = node_cast<ExprStmt>(n))
    {
        (void)es;
        return start;
    }
    return start;
}

uint32_t CodeGen::count_locals_in_block(const Block& blk, uint32_t start) const
{
    for (const auto& s : blk.stmts)
        start = count_locals_in_node(s.get(), start);
    return start;
}

// ── Collect function signatures (first pass) ──────────────────────────────────

void CodeGen::collect_functions(const Program& prog)
{
    for (const auto& decl : prog.decls)
    {
        auto* fn = node_cast<FunctionDecl>(decl.get());
        if (!fn) continue;

        FnInfo info;
        info.ip          = 0; // patched later
        info.argc        = static_cast<uint16_t>(fn->params.size());
        info.return_type = fn->return_type;
        info.is_void     = fn->is_void;

        // Count total locals: params + declared variables in body
        uint32_t declared = count_locals_in_block(*fn->body, 0);
        info.total_slots  = static_cast<uint16_t>(fn->params.size() + declared);

        functions_[fn->name] = info;
    }
}

// ── Top-level program ─────────────────────────────────────────────────────────

program CodeGen::compile(const Program& prog)
{
    out_.clear();
    functions_.clear();
    label_defs_.clear();
    label_refs_.clear();
    loop_stack_.clear();
    syms_.reset();

    collect_functions(prog);
    gen_program(prog);
    resolve_labels();

    return std::move(out_);
}

void CodeGen::gen_program(const Program& prog)
{
    // Separate functions from top-level statements
    std::vector<const FunctionDecl*> fns;
    std::vector<const AstNode*>      stmts;

    for (const auto& d : prog.decls)
    {
        if (auto* fn = node_cast<FunctionDecl>(d.get()))
            fns.push_back(fn);
        else
            stmts.push_back(d.get());
    }

    // Emit GOTO main_start if there are functions
    std::size_t goto_main = SIZE_MAX;
    if (!fns.empty())
        goto_main = emit(opcode::GOTO, 0); // patched below

    // Emit function bodies
    for (auto* fn : fns)
        gen_function(*fn);

    // Patch GOTO main_start
    const std::size_t main_start = current_ip();
    if (goto_main != SIZE_MAX)
        patch(goto_main, static_cast<int64_t>(main_start));

    // Count top-level locals
    uint32_t main_locals = 0;
    for (auto* s : stmts)
        main_locals = count_locals_in_node(s, main_locals);

    // Set up symbol table for main scope
    syms_.reset();
    syms_.push_scope();

    if (main_locals > 0)
        emit(opcode::FRAME, static_cast<int64_t>(main_locals));

    for (auto* s : stmts)
        gen_stmt(*s);

    emit(opcode::HALT);

    // Resolve remaining labels (in main scope)
    // (resolve_labels is called after gen_program returns)
}

void CodeGen::gen_function(const FunctionDecl& fn)
{
    // Record function IP
    const std::size_t fn_ip = current_ip();
    functions_[fn.name].ip = fn_ip;

    // Set up fresh symbol table for this function
    syms_.reset();
    syms_.push_scope();

    // Bind parameters to slots 0..argc-1
    for (uint32_t i = 0; i < fn.params.size(); ++i)
        syms_.define_at(fn.params[i].first, fn.params[i].second, i, true);

    // Set next_slot to argc so that local vars start after params
    syms_.set_next_slot(static_cast<uint32_t>(fn.params.size()));

    // Emit body
    gen_block(*fn.body);

    // Implicit void return if function doesn't end with explicit return
    if (fn.is_void)
        emit(opcode::RET_VOID);
    else
        emit(opcode::RET_VOID); // safety fallback (well-formed code has explicit return)

    // Patch all label refs within this function
    // (they'll be resolved globally in resolve_labels)
}

// ── Statements ────────────────────────────────────────────────────────────────

void CodeGen::gen_block(const Block& blk)
{
    syms_.push_scope();
    for (const auto& s : blk.stmts)
        gen_stmt(*s);
    syms_.pop_scope();
}

void CodeGen::gen_stmt(const AstNode& node)
{
    if (auto* blk = node_cast<Block>(&node))
    {
        gen_block(*blk);
        return;
    }

    if (auto* vd = node_cast<VarDecl>(&node))
    {
        // Allocate slot
        syms_.define(vd->name, vd->type);
        Symbol* sym = syms_.lookup(vd->name);
        assert(sym);

        if (vd->init)
        {
            gen_expr(*vd->init);
            emit(opcode::STORE_LOCAL, static_cast<int64_t>(sym->slot));
        }
        return;
    }

    if (auto* es = node_cast<ExprStmt>(&node))
    {
        gen_expr(*es->expr);
        // Pop result if any (expressions as statements produce a value we discard)
        // Most expressions leave a value on the stack; pop it
        // Exception: assignments already store and don't push an extra value
        // We'll always emit POP for safety — assignments in .vz return void
        if (!node_cast<AssignExpr>(es->expr.get()) &&
            !node_cast<UnaryExpr>(es->expr.get()))  // ++ -- handled carefully
        {
            // CallExpr as stmt: if it returns a value, pop it
            if (node_cast<CallExpr>(es->expr.get()))
            {
                auto* ce = node_cast<CallExpr>(es->expr.get());
                auto it = functions_.find(ce->callee);
                if (it != functions_.end() && !it->second.is_void)
                    emit(opcode::POP);
            }
        }
        return;
    }

    if (auto* ifs = node_cast<IfStmt>(&node))
    {
        gen_cond(*ifs->cond);
        std::size_t jz_site = emit(opcode::JZ, 0);

        gen_stmt(*ifs->then_);

        if (ifs->else_)
        {
            std::size_t goto_end = emit(opcode::GOTO, 0);
            patch(jz_site, static_cast<int64_t>(current_ip()));
            gen_stmt(*ifs->else_);
            patch(goto_end, static_cast<int64_t>(current_ip()));
        }
        else
        {
            patch(jz_site, static_cast<int64_t>(current_ip()));
        }
        return;
    }

    if (auto* ws = node_cast<WhileStmt>(&node))
    {
        loop_stack_.push_back({});
        LoopCtx& ctx = loop_stack_.back();

        if (ws->do_while)
        {
            // do { body } while (cond)
            std::size_t body_start = current_ip();
            ctx.continue_ip = body_start;
            gen_stmt(*ws->body);
            // Patch continue sites to point here (before cond)
            std::size_t cond_start = current_ip();
            for (auto s : ctx.continue_sites)
                patch(s, static_cast<int64_t>(cond_start));
            gen_cond(*ws->cond);
            std::size_t jnz = emit(opcode::JNZ, static_cast<int64_t>(body_start));
            (void)jnz;
            std::size_t loop_end = current_ip();
            for (auto s : ctx.break_sites)
                patch(s, static_cast<int64_t>(loop_end));
        }
        else
        {
            // while (cond) { body }
            std::size_t loop_start = current_ip();
            ctx.continue_ip = loop_start;
            gen_cond(*ws->cond);
            std::size_t jz_site = emit(opcode::JZ, 0);
            gen_stmt(*ws->body);
            for (auto s : ctx.continue_sites)
                patch(s, static_cast<int64_t>(loop_start));
            emit(opcode::GOTO, static_cast<int64_t>(loop_start));
            std::size_t loop_end = current_ip();
            patch(jz_site, static_cast<int64_t>(loop_end));
            for (auto s : ctx.break_sites)
                patch(s, static_cast<int64_t>(loop_end));
        }

        loop_stack_.pop_back();
        return;
    }

    if (auto* fs = node_cast<ForStmt>(&node))
    {
        loop_stack_.push_back({});
        LoopCtx& ctx = loop_stack_.back();

        syms_.push_scope();

        // Init
        if (fs->init)
        {
            if (auto* vd = node_cast<VarDecl>(fs->init.get()))
            {
                syms_.define(vd->name, vd->type);
                Symbol* sym = syms_.lookup(vd->name);
                assert(sym);
                if (vd->init)
                {
                    gen_expr(*vd->init);
                    emit(opcode::STORE_LOCAL, static_cast<int64_t>(sym->slot));
                }
            }
            else
            {
                gen_stmt(*fs->init);
            }
        }

        std::size_t cond_start = current_ip();
        gen_cond(*fs->cond);
        std::size_t jz_site = emit(opcode::JZ, 0);

        // Body
        gen_stmt(*fs->body);

        // Step — continue jumps here
        std::size_t step_start = current_ip();
        ctx.continue_ip = step_start;
        for (auto s : ctx.continue_sites)
            patch(s, static_cast<int64_t>(step_start));

        // Gen step expr and discard result
        {
            gen_expr(*fs->step);
            // step is usually an assignment or ++, which doesn't leave a value
            // For ++ postfix, there IS a value on stack — pop it
            if (node_cast<UnaryExpr>(fs->step.get()))
            {
                auto* ue = node_cast<UnaryExpr>(fs->step.get());
                if (ue->postfix) emit(opcode::POP);
            }
            else if (!node_cast<AssignExpr>(fs->step.get()))
            {
                emit(opcode::POP);
            }
        }

        emit(opcode::GOTO, static_cast<int64_t>(cond_start));

        std::size_t loop_end = current_ip();
        patch(jz_site, static_cast<int64_t>(loop_end));
        for (auto s : ctx.break_sites)
            patch(s, static_cast<int64_t>(loop_end));

        syms_.pop_scope();
        loop_stack_.pop_back();
        return;
    }

    if (auto* rs = node_cast<ReturnStmt>(&node))
    {
        if (rs->value)
        {
            gen_expr(*rs->value);
            emit(opcode::RET);
        }
        else
        {
            emit(opcode::RET_VOID);
        }
        return;
    }

    if (node_cast<BreakStmt>(&node))
    {
        if (loop_stack_.empty())
            error("'break' outside loop", node.loc);
        std::size_t site = emit(opcode::GOTO, 0);
        loop_stack_.back().break_sites.push_back(site);
        return;
    }

    if (node_cast<ContinueStmt>(&node))
    {
        if (loop_stack_.empty())
            error("'continue' outside loop", node.loc);
        std::size_t site = emit(opcode::GOTO, 0);
        loop_stack_.back().continue_sites.push_back(site);
        return;
    }

    if (node_cast<NopStmt>(&node))
    {
        emit(opcode::NOP);
        return;
    }

    if (auto* ls = node_cast<LabelStmt>(&node))
    {
        label_defs_[ls->name] = current_ip();
        return;
    }

    if (auto* gs = node_cast<GotoStmt>(&node))
    {
        std::size_t site = emit(opcode::GOTO, 0);
        label_refs_[gs->label].push_back(site);
        return;
    }

    // Should not reach here
    error("unhandled statement type", node.loc);
}

// ── Expression generation ──────────────────────────────────────────────────────

void CodeGen::gen_expr(const AstNode& node)
{
    if (auto* lit = node_cast<IntLit>(&node))
    {
        // Use PUSH_T: byte 0 = vtype.bits, bytes 1-7 = 56-bit value
        // Encode: (vtype.bits) | (value << 8)
        const int64_t operand = static_cast<int64_t>(
            (static_cast<uint64_t>(lit->type.bits)) |
            (static_cast<uint64_t>(static_cast<int64_t>(lit->value)) << 8)
        );
        emit(opcode::PUSH_T, operand);
        return;
    }

    if (auto* vr = node_cast<VarRef>(&node))
    {
        Symbol* sym = syms_.lookup(vr->name);
        if (!sym)
            error("undefined variable '" + vr->name + "'", vr->loc);
        emit(opcode::LOAD_LOCAL, static_cast<int64_t>(sym->slot));
        return;
    }

    if (auto* bin = node_cast<BinaryExpr>(&node))
    {
        gen_expr(*bin->left);
        gen_expr(*bin->right);
        const std::string& op = bin->op;
        if      (op == "+")  emit(opcode::ADD);
        else if (op == "-")  emit(opcode::SUB);
        else if (op == "*")  emit(opcode::MUL);
        else if (op == "/")  emit(opcode::DIV);
        else if (op == "%")  emit(opcode::MOD);
        else if (op == "&")  emit(opcode::AND);
        else if (op == "|")  emit(opcode::OR);
        else if (op == "^")  emit(opcode::XOR);
        else if (op == "<<") emit(opcode::SHL);
        else if (op == ">>") emit(opcode::SHR);
        else if (op == "==") { emit(opcode::CMP_EQ); }
        else if (op == "!=") { emit(opcode::CMP_NEQ); }
        else if (op == "<")  { emit(opcode::CMP_LT); }
        else if (op == "<=") { emit(opcode::CMP_LTE); }
        else if (op == ">")  { emit(opcode::CMP_GT); }
        else if (op == ">=") { emit(opcode::CMP_GTE); }
        else if (op == "&&")
        {
            // Short-circuit: already computed both sides (simplified — no short-circuit for now)
            // Emit: a AND b != 0
            // We already pushed both; AND gives bitwise, then CMP_NEQ 0
            emit(opcode::AND);
        }
        else if (op == "||")
        {
            emit(opcode::OR);
        }
        else
            error("unknown binary operator '" + op + "'", bin->loc);
        return;
    }

    if (auto* un = node_cast<UnaryExpr>(&node))
    {
        const std::string& op = un->op;

        if (op == "-")      { gen_expr(*un->operand); emit(opcode::NEG); return; }
        if (op == "~")      { gen_expr(*un->operand); emit(opcode::NOT); return; }
        if (op == "!")
        {
            gen_expr(*un->operand);
            // Logical NOT: push 0, CMP_EQ
            int64_t zero_operand = static_cast<int64_t>(
                (static_cast<uint64_t>(execute::types::I64.bits)) | 0LL
            );
            emit(opcode::PUSH_T, zero_operand);
            emit(opcode::CMP_EQ);
            return;
        }

        // ++ or --
        if (op == "++" || op == "--")
        {
            // Operand must be a VarRef
            auto* vr = node_cast<VarRef>(un->operand.get());
            if (!vr) error("++ / -- requires a variable", un->loc);
            Symbol* sym = syms_.lookup(vr->name);
            if (!sym) error("undefined variable '" + vr->name + "'", vr->loc);

            const int64_t slot = static_cast<int64_t>(sym->slot);

            if (un->postfix)
            {
                // x++ : push old value, then increment and store
                emit(opcode::LOAD_LOCAL, slot);  // old value (expression result)
                emit(opcode::LOAD_LOCAL, slot);  // copy to modify
                emit(op == "++" ? opcode::INC : opcode::DEC);
                emit(opcode::STORE_LOCAL, slot);
            }
            else
            {
                // ++x : increment, store, then push new value
                emit(opcode::LOAD_LOCAL, slot);
                emit(op == "++" ? opcode::INC : opcode::DEC);
                emit(opcode::DUP);                // duplicate new value for expression result
                emit(opcode::STORE_LOCAL, slot);
            }
            return;
        }

        error("unknown unary operator '" + op + "'", un->loc);
    }

    if (auto* ae = node_cast<AssignExpr>(&node))
    {
        Symbol* sym = syms_.lookup(ae->name);
        if (!sym) error("undefined variable '" + ae->name + "'", ae->loc);
        const int64_t slot = static_cast<int64_t>(sym->slot);

        if (ae->op == "=")
        {
            gen_expr(*ae->value);
            emit(opcode::STORE_LOCAL, slot);
        }
        else
        {
            // Compound assignment: load current, compute new value, store
            emit(opcode::LOAD_LOCAL, slot);
            gen_expr(*ae->value);
            gen_assign_op(ae->op);
            emit(opcode::STORE_LOCAL, slot);
        }
        return;
    }

    if (auto* ce = node_cast<CallExpr>(&node))
    {
        // Builtin: pf, p, dd
        if (ce->is_builtin || ce->callee == "pf" || ce->callee == "p" || ce->callee == "dd")
        {
            for (const auto& arg : ce->args)
                gen_expr(*arg);
            if (ce->callee == "dd")
                emit(opcode::DD);
            else
                emit(opcode::COUT);
            return;
        }

        // User-defined function call
        auto it = functions_.find(ce->callee);
        if (it == functions_.end())
            error("undefined function '" + ce->callee + "'", ce->loc);

        const FnInfo& fn = it->second;

        // Push arguments left-to-right
        for (const auto& arg : ce->args)
            gen_expr(*arg);

        // Encode CALL operand
        const int64_t operand =
            static_cast<int64_t>(fn.ip) |
            (static_cast<int64_t>(fn.argc) << 32) |
            (static_cast<int64_t>(fn.total_slots) << 48);

        emit(opcode::CALL, operand);
        return;
    }

    if (auto* te = node_cast<TernaryExpr>(&node))
    {
        gen_cond(*te->cond);
        std::size_t jz = emit(opcode::JZ, 0);
        gen_expr(*te->then_);
        std::size_t go = emit(opcode::GOTO, 0);
        patch(jz, static_cast<int64_t>(current_ip()));
        gen_expr(*te->else_);
        patch(go, static_cast<int64_t>(current_ip()));
        return;
    }

    error("unhandled expression type", node.loc);
}

void CodeGen::gen_assign_op(const std::string& op)
{
    if      (op == "+=")  emit(opcode::ADD);
    else if (op == "-=")  emit(opcode::SUB);
    else if (op == "*=")  emit(opcode::MUL);
    else if (op == "/=")  emit(opcode::DIV);
    else if (op == "%=")  emit(opcode::MOD);
    else if (op == "&=")  emit(opcode::AND);
    else if (op == "|=")  emit(opcode::OR);
    else if (op == "^=")  emit(opcode::XOR);
}

void CodeGen::gen_cond(const AstNode& node)
{
    // If node is already a comparison, gen_expr leaves a 0/1 boolean
    gen_expr(node);
    // For non-comparison expressions (e.g. bare variable), the value
    // is already on the stack — JZ/JNZ treat 0 as false, non-zero as true.
}

// ── Label resolution ───────────────────────────────────────────────────────────

void CodeGen::resolve_labels()
{
    for (auto& [name, sites] : label_refs_)
    {
        auto it = label_defs_.find(name);
        if (it == label_defs_.end())
            throw CompileError("undefined label '#" + name + "'", SourceLocation{});
        for (auto site : sites)
            patch(site, static_cast<int64_t>(it->second));
    }
}

} // namespace compile
