#include "parser.h"
#include <stdexcept>
#include <cassert>

namespace compile
{

Parser::Parser(Lexer& lexer) : lex_(lexer) {}

// ── Token helpers ──────────────────────────────────────────────────────────

const Token& Parser::peek()  { return lex_.peek(); }
Token        Parser::consume() { return lex_.next(); }

bool Parser::check(TokenType t)
{
    return peek().type == t;
}

bool Parser::match(TokenType t)
{
    if (check(t)) { consume(); return true; }
    return false;
}

Token Parser::expect(TokenType t)
{
    if (!check(t))
    {
        error("expected " + std::string(token_type_name(t)) +
              ", got " + std::string(token_type_name(peek().type)) +
              " '" + peek().lexeme + "'");
    }
    return consume();
}

[[noreturn]] void Parser::error(const std::string& msg)
{
    throw CompileError(msg, peek().loc);
}

[[noreturn]] void Parser::error(const std::string& msg, const SourceLocation& loc)
{
    throw CompileError(msg, loc);
}

// ── Type parsing ────────────────────────────────────────────────────────────

bool Parser::is_type_start()
{
    const Token& t = peek();
    if (t.type == TokenType::KW_VOID)     return true;
    if (t.type == TokenType::KW_INT)      return true;
    if (t.type == TokenType::KW_CHAR)     return true;
    if (t.type == TokenType::KW_UNSIGNED) return true;
    if (t.type == TokenType::KW_SIGNED)   return true;
    if (t.type == TokenType::IDENT && is_size_spec(t.lexeme)) return true;
    return false;
}

Parser::TypeResult Parser::parse_type()
{
    // void
    if (check(TokenType::KW_VOID))
    {
        consume();
        TypeResult r;
        r.is_void = true;
        r.type    = execute::types::I64; // placeholder
        return r;
    }

    std::string size_spec;
    bool is_unsigned = false;

    // Optional size specifier: b_8, b_16, ...
    if (peek().type == TokenType::IDENT && is_size_spec(peek().lexeme))
        size_spec = consume().lexeme;

    // Optional endianness annotation: int<le> / int<be> — just consume and ignore
    // handled below after base type

    // Optional signedness
    if (check(TokenType::KW_UNSIGNED)) { consume(); is_unsigned = true; }
    else if (check(TokenType::KW_SIGNED)) { consume(); }

    // Base type
    std::string base;
    if (check(TokenType::KW_INT))  { consume(); base = "int"; }
    else if (check(TokenType::KW_CHAR)) { consume(); base = "char"; }
    else if (!size_spec.empty())   { base = "int"; } // bare b_8 → b_8 int
    else
        error("expected type");

    // Optional endianness: int<le> or int<be> — consume and ignore
    if (check(TokenType::LT))
    {
        consume(); // <
        consume(); // le / be identifier
        expect(TokenType::GT);
    }

    TypeResult r;
    r.is_void = false;
    r.type    = resolve_vtype(size_spec, is_unsigned, base);

    // Optional pointer: b_8 int*
    if (check(TokenType::STAR))
    {
        consume();
        r.is_pointer = true;
        r.pointee    = r.type;
        r.type       = execute::types::PTR;
    }

    return r;
}

// ── Program ─────────────────────────────────────────────────────────────────

std::unique_ptr<Program> Parser::parse()
{
    auto prog = std::make_unique<Program>();
    prog->loc = peek().loc;

    while (!check(TokenType::EOF_TOK))
        prog->decls.push_back(parse_decl());

    return prog;
}

NodePtr Parser::parse_decl()
{
    // Lookahead: if we see a type followed by "function"/"fn", it's a function decl.
    // If we see a type followed by an IDENT followed by ("="|":="|";"), it's a var decl.
    // Otherwise it's a statement.

    if (is_type_start())
    {
        // Try to distinguish function decl from var decl
        // Save lexer state by peeking further via parsing the type first, then checking
        auto saved_loc = peek().loc;

        TypeResult type_res = parse_type();

        if (check(TokenType::KW_FUNCTION) || check(TokenType::KW_FN))
        {
            consume(); // consume 'function' or 'fn'
            Token name = expect(TokenType::IDENT);
            return parse_function(type_res, name);
        }

        // Must be a var decl or a statement starting with a type-cast-like pattern
        if (check(TokenType::IDENT))
        {
            Token name = consume();

            // If followed by '(' this is a function decl without explicit "function" keyword?
            // .vz doesn't support that — treat as error. Just parse as var decl.
            return parse_var_decl(type_res, name);
        }

        // Bare type with no following name — error
        error("expected identifier after type", saved_loc);
    }

    return parse_stmt();
}

std::unique_ptr<FunctionDecl> Parser::parse_function(TypeResult ret, const Token& name_tok)
{
    auto fn    = std::make_unique<FunctionDecl>();
    fn->loc    = name_tok.loc;
    fn->name   = name_tok.lexeme;
    fn->return_type = ret.type;
    fn->is_void     = ret.is_void;

    expect(TokenType::LPAREN);

    // Parameters
    if (!check(TokenType::RPAREN))
    {
        do {
            TypeResult ptype = parse_type();
            Token pname      = expect(TokenType::IDENT);
            ParamInfo pi;
            pi.name       = pname.lexeme;
            pi.type       = ptype.type;
            pi.is_pointer = ptype.is_pointer;
            pi.pointee    = ptype.pointee;
            fn->params.push_back(std::move(pi));
        } while (match(TokenType::COMMA));
    }

    expect(TokenType::RPAREN);
    fn->body = parse_block();
    return fn;
}

// ── Statements ──────────────────────────────────────────────────────────────

std::unique_ptr<Block> Parser::parse_block()
{
    auto block = std::make_unique<Block>();
    block->loc = peek().loc;
    expect(TokenType::LBRACE);
    while (!check(TokenType::RBRACE) && !check(TokenType::EOF_TOK))
        block->stmts.push_back(parse_stmt());
    expect(TokenType::RBRACE);
    return block;
}

NodePtr Parser::parse_stmt()
{
    const Token& t = peek();

    switch (t.type)
    {
        case TokenType::LBRACE:
            return parse_block();

        case TokenType::KW_IF:
            return parse_if_stmt();

        case TokenType::KW_WHILE:
            return parse_while_stmt();

        case TokenType::KW_DO:
            return parse_do_while_stmt();

        case TokenType::KW_FOR:
            return parse_for_stmt();

        case TokenType::KW_RETURN:
        case TokenType::KW_RT:
            return parse_return_stmt();

        case TokenType::KW_BREAK:
        {
            auto n = std::make_unique<BreakStmt>(); n->loc = consume().loc;
            expect(TokenType::SEMICOLON);
            return n;
        }
        case TokenType::KW_CONTINUE:
        {
            auto n = std::make_unique<ContinueStmt>(); n->loc = consume().loc;
            expect(TokenType::SEMICOLON);
            return n;
        }
        case TokenType::KW_NOP:
        {
            auto n = std::make_unique<NopStmt>(); n->loc = consume().loc;
            match(TokenType::SEMICOLON);
            return n;
        }
        case TokenType::KW_GOTO:
            return parse_label_or_goto();

        case TokenType::HASH:
            return parse_label_or_goto();

        default:
            break;
    }

    // Possible var decl starting with type specifier
    if (is_type_start())
    {
        TypeResult type_res = parse_type();
        Token name = expect(TokenType::IDENT);
        return parse_var_decl(type_res, name);
    }

    // Expression statement
    auto expr_node = parse_expr();
    auto stmt      = std::make_unique<ExprStmt>();
    stmt->loc      = expr_node->loc;
    stmt->expr     = std::move(expr_node);
    expect(TokenType::SEMICOLON);
    return stmt;
}

NodePtr Parser::parse_var_decl(TypeResult type_res, const Token& name_tok)
{
    auto decl       = std::make_unique<VarDecl>();
    decl->loc       = name_tok.loc;
    decl->name      = name_tok.lexeme;
    decl->type      = type_res.type;
    decl->is_pointer = type_res.is_pointer;
    decl->pointee   = type_res.pointee;

    if (check(TokenType::ASSIGN) || check(TokenType::COLON_ASSIGN))
    {
        consume();
        decl->init = parse_expr();
    }

    expect(TokenType::SEMICOLON);
    return decl;
}

NodePtr Parser::parse_if_stmt()
{
    auto node = std::make_unique<IfStmt>();
    node->loc = consume().loc; // consume 'if'

    expect(TokenType::LPAREN);
    node->cond = parse_expr();
    expect(TokenType::RPAREN);

    node->then_ = parse_block();

    if (match(TokenType::KW_ELSE))
    {
        if (check(TokenType::KW_IF))
            node->else_ = parse_if_stmt();
        else
            node->else_ = parse_block();
    }

    return node;
}

NodePtr Parser::parse_while_stmt()
{
    auto node = std::make_unique<WhileStmt>();
    node->loc = consume().loc; // 'while'

    expect(TokenType::LPAREN);
    node->cond = parse_expr();
    expect(TokenType::RPAREN);

    if (match(TokenType::KW_DO))
        node->body = parse_stmt();
    else
        node->body = parse_block();

    return node;
}

NodePtr Parser::parse_do_while_stmt()
{
    auto node     = std::make_unique<WhileStmt>();
    node->loc     = consume().loc; // 'do'
    node->do_while = true;
    node->body    = parse_block();
    expect(TokenType::KW_WHILE);
    expect(TokenType::LPAREN);
    node->cond    = parse_expr();
    expect(TokenType::RPAREN);
    expect(TokenType::SEMICOLON);
    return node;
}

NodePtr Parser::parse_for_stmt()
{
    // for ( init , cond , step ) { body }
    // .vz uses commas as separators, not semicolons
    auto node = std::make_unique<ForStmt>();
    node->loc = consume().loc; // 'for'

    expect(TokenType::LPAREN);

    // init: var decl or expr
    if (is_type_start())
    {
        TypeResult type_res = parse_type();
        Token name          = expect(TokenType::IDENT);
        // Parse var decl without the trailing semicolon (use comma instead)
        auto decl  = std::make_unique<VarDecl>();
        decl->loc  = name.loc;
        decl->name = name.lexeme;
        decl->type = type_res.type;
        if (check(TokenType::ASSIGN) || check(TokenType::COLON_ASSIGN))
        {
            consume();
            decl->init = parse_expr();
        }
        node->init = std::move(decl);
    }
    else
    {
        auto expr_node = parse_expr();
        auto s         = std::make_unique<ExprStmt>();
        s->loc         = expr_node->loc;
        s->expr        = std::move(expr_node);
        node->init     = std::move(s);
    }

    expect(TokenType::COMMA); // separator between init and cond

    node->cond = parse_expr();

    expect(TokenType::COMMA); // separator between cond and step

    node->step = parse_expr();

    expect(TokenType::RPAREN);

    if (match(TokenType::KW_DO))
        node->body = parse_stmt();
    else
        node->body = parse_block();

    return node;
}

NodePtr Parser::parse_return_stmt()
{
    auto node = std::make_unique<ReturnStmt>();
    node->loc = consume().loc; // 'return' or 'rt'

    if (!check(TokenType::SEMICOLON))
        node->value = parse_expr();

    expect(TokenType::SEMICOLON);
    return node;
}

NodePtr Parser::parse_label_or_goto()
{
    if (check(TokenType::KW_GOTO))
    {
        auto node  = std::make_unique<GotoStmt>();
        node->loc  = consume().loc; // 'goto'
        expect(TokenType::HASH);
        Token name = expect(TokenType::IDENT);
        node->label = name.lexeme;
        expect(TokenType::SEMICOLON);
        return node;
    }

    // #label:
    auto node = std::make_unique<LabelStmt>();
    node->loc = consume().loc; // '#'
    Token name = expect(TokenType::IDENT);
    node->name = name.lexeme;
    expect(TokenType::COLON);
    return node;
}

// ── Expressions ─────────────────────────────────────────────────────────────

NodePtr Parser::parse_expr()     { return parse_assignment(); }

NodePtr Parser::parse_assignment()
{
    // Check for: IDENT op_assign expr  |  IDENT++  |  IDENT--
    // We need lookahead. If peek is IDENT and peek+1 is an assignment op or ++/--, handle it.
    // Otherwise fall through to ternary.

    if (check(TokenType::IDENT))
    {
        // peek ahead by consuming and re-checking
        // We rely on the lexer's lookahead being only 1 deep.
        // To handle multi-token lookahead, we parse ternary and check if it's an assignable lhs.
        // Simple approach: if next is IDENT and following is assignment op, parse as assign.
        // We'll detect ++ and -- as postfix in parse_postfix.
        // For assignments: IDENT = expr etc.

        // We parse ternary first; if the result is a VarRef followed by assign op, rewrite.
        // But we can't do that easily. Instead, peek at the next-next token.
        // The lexer only has 1 lookahead. We'll handle this by parsing and checking.
    }

    NodePtr node = parse_ternary();

    // Helper lambda: check if current token is any assignment operator
    auto is_assign_op = [&]() {
        TokenType t = peek().type;
        return t == TokenType::ASSIGN      ||
               t == TokenType::PLUS_ASSIGN || t == TokenType::MINUS_ASSIGN ||
               t == TokenType::STAR_ASSIGN || t == TokenType::SLASH_ASSIGN ||
               t == TokenType::PERCENT_ASSIGN ||
               t == TokenType::AMP_ASSIGN  || t == TokenType::PIPE_ASSIGN  ||
               t == TokenType::CARET_ASSIGN;
    };

    // If the result is a VarRef, check for assignment operator that follows
    if (auto* vr = node_cast<VarRef>(node.get()))
    {
        if (is_assign_op())
        {
            std::string op = consume().lexeme;
            auto assign    = std::make_unique<AssignExpr>();
            assign->loc    = node->loc;
            assign->name   = vr->name;
            assign->op     = op;
            assign->value  = parse_expr(); // right-associative
            return assign;
        }
    }

    // If the result is a DerefExpr (*ptr), check for assignment operator
    if (auto* de = node_cast<DerefExpr>(node.get()))
    {
        if (is_assign_op())
        {
            std::string op = consume().lexeme;
            auto da        = std::make_unique<DerefAssign>();
            da->loc        = node->loc;
            da->ptr_expr   = std::move(de->ptr_expr); // steal inner ptr
            da->op         = op;
            da->value      = parse_expr();
            return da;
        }
    }

    return node;
}

NodePtr Parser::parse_ternary()
{
    NodePtr node = parse_logical();

    if (match(TokenType::QUESTION))
    {
        auto tern   = std::make_unique<TernaryExpr>();
        tern->loc   = node->loc;
        tern->cond  = std::move(node);
        tern->then_ = parse_expr();
        expect(TokenType::COLON);
        tern->else_ = parse_expr();
        return tern;
    }

    return node;
}

NodePtr Parser::parse_logical()
{
    NodePtr left = parse_comparison();

    while (check(TokenType::AMP_AMP) || check(TokenType::PIPE_PIPE))
    {
        std::string op  = consume().lexeme;
        auto bin        = std::make_unique<BinaryExpr>();
        bin->loc        = left->loc;
        bin->op         = op;
        bin->left       = std::move(left);
        bin->right      = parse_comparison();
        left            = std::move(bin);
    }

    return left;
}

NodePtr Parser::parse_comparison()
{
    NodePtr left = parse_bitwise();

    while (check(TokenType::EQ_EQ)   || check(TokenType::BANG_EQ) ||
           check(TokenType::LT)      || check(TokenType::GT)      ||
           check(TokenType::LT_EQ)   || check(TokenType::GT_EQ))
    {
        std::string op  = consume().lexeme;
        auto bin        = std::make_unique<BinaryExpr>();
        bin->loc        = left->loc;
        bin->op         = op;
        bin->left       = std::move(left);
        bin->right      = parse_bitwise();
        left            = std::move(bin);
    }

    return left;
}

NodePtr Parser::parse_bitwise()
{
    NodePtr left = parse_shift();

    while (check(TokenType::AMP) || check(TokenType::PIPE) || check(TokenType::CARET))
    {
        std::string op  = consume().lexeme;
        auto bin        = std::make_unique<BinaryExpr>();
        bin->loc        = left->loc;
        bin->op         = op;
        bin->left       = std::move(left);
        bin->right      = parse_shift();
        left            = std::move(bin);
    }

    return left;
}

NodePtr Parser::parse_shift()
{
    NodePtr left = parse_addition();

    while (check(TokenType::LSHIFT) || check(TokenType::RSHIFT))
    {
        std::string op  = consume().lexeme;
        auto bin        = std::make_unique<BinaryExpr>();
        bin->loc        = left->loc;
        bin->op         = op;
        bin->left       = std::move(left);
        bin->right      = parse_addition();
        left            = std::move(bin);
    }

    return left;
}

NodePtr Parser::parse_addition()
{
    NodePtr left = parse_multiply();

    while (check(TokenType::PLUS) || check(TokenType::MINUS))
    {
        std::string op  = consume().lexeme;
        auto bin        = std::make_unique<BinaryExpr>();
        bin->loc        = left->loc;
        bin->op         = op;
        bin->left       = std::move(left);
        bin->right      = parse_multiply();
        left            = std::move(bin);
    }

    return left;
}

NodePtr Parser::parse_multiply()
{
    NodePtr left = parse_unary();

    while (check(TokenType::STAR) || check(TokenType::SLASH) || check(TokenType::PERCENT))
    {
        std::string op  = consume().lexeme;
        auto bin        = std::make_unique<BinaryExpr>();
        bin->loc        = left->loc;
        bin->op         = op;
        bin->left       = std::move(left);
        bin->right      = parse_unary();
        left            = std::move(bin);
    }

    return left;
}

NodePtr Parser::parse_unary()
{
    // Prefix: -, !, ~, ++, --
    if (check(TokenType::MINUS) || check(TokenType::BANG) || check(TokenType::TILDE))
    {
        std::string op = consume().lexeme;
        auto u         = std::make_unique<UnaryExpr>();
        u->loc         = peek().loc;
        u->op          = op;
        u->operand     = parse_unary();
        return u;
    }

    if (check(TokenType::PLUS_PLUS) || check(TokenType::MINUS_MINUS))
    {
        std::string op = consume().lexeme;
        auto u         = std::make_unique<UnaryExpr>();
        u->loc         = peek().loc;
        u->op          = op;
        u->postfix     = false;
        u->operand     = parse_postfix();
        return u;
    }

    // Address-of: &varname
    if (check(TokenType::AMP))
    {
        SourceLocation l = consume().loc; // consume '&'
        Token name_tok   = expect(TokenType::IDENT);
        auto ao  = std::make_unique<AddrOf>();
        ao->loc  = l;
        ao->name = name_tok.lexeme;
        return ao;
    }

    // Dereference: *expr
    if (check(TokenType::STAR))
    {
        SourceLocation l = consume().loc; // consume '*'
        auto de          = std::make_unique<DerefExpr>();
        de->loc          = l;
        de->ptr_expr     = parse_unary(); // recursive: handles **ptr
        return de;
    }

    return parse_postfix();
}

NodePtr Parser::parse_postfix()
{
    NodePtr node = parse_primary();

    if (check(TokenType::PLUS_PLUS) || check(TokenType::MINUS_MINUS))
    {
        std::string op = consume().lexeme;
        auto u         = std::make_unique<UnaryExpr>();
        u->loc         = node->loc;
        u->op          = op;
        u->postfix     = true;
        u->operand     = std::move(node);
        return u;
    }

    return node;
}

NodePtr Parser::parse_primary()
{
    const Token& t = peek();

    // Integer literal
    if (t.type == TokenType::INT_LIT || t.type == TokenType::CHAR_LIT)
    {
        Token tok = consume();
        auto lit  = std::make_unique<IntLit>();
        lit->loc  = tok.loc;
        lit->value = tok.int_val;
        lit->type  = execute::types::I64; // will be coerced at assignment
        return lit;
    }

    // null literal
    if (t.type == TokenType::KW_NULL)
    {
        auto n = std::make_unique<NullLit>();
        n->loc = consume().loc;
        return n;
    }

    // Builtin calls: pf, p, dd, malloc, free — only when followed by '('
    if (t.type == TokenType::IDENT &&
        (t.lexeme == "pf" || t.lexeme == "p" || t.lexeme == "dd" ||
         t.lexeme == "malloc" || t.lexeme == "free"))
    {
        Token name_tok = consume();
        if (check(TokenType::LPAREN))
            return parse_call(name_tok, /*is_builtin=*/true);
        // Not a call — treat as a regular variable reference
        auto vr  = std::make_unique<VarRef>();
        vr->loc  = name_tok.loc;
        vr->name = name_tok.lexeme;
        return vr;
    }

    // Identifier: variable or function call
    if (t.type == TokenType::IDENT)
    {
        Token name_tok = consume();
        if (check(TokenType::LPAREN))
            return parse_call(name_tok, false);

        auto vr   = std::make_unique<VarRef>();
        vr->loc   = name_tok.loc;
        vr->name  = name_tok.lexeme;
        return vr;
    }

    // Parenthesised expression
    if (t.type == TokenType::LPAREN)
    {
        consume();
        NodePtr inner = parse_expr();
        expect(TokenType::RPAREN);
        return inner;
    }

    error("unexpected token " + std::string(token_type_name(t.type)) +
          " '" + t.lexeme + "'");
}

NodePtr Parser::parse_call(Token name_tok, bool is_builtin)
{
    auto call      = std::make_unique<CallExpr>();
    call->loc      = name_tok.loc;
    call->callee   = name_tok.lexeme;
    call->is_builtin = is_builtin;

    expect(TokenType::LPAREN);
    if (!check(TokenType::RPAREN))
        call->args = parse_arg_list();
    expect(TokenType::RPAREN);

    return call;
}

std::vector<NodePtr> Parser::parse_arg_list()
{
    std::vector<NodePtr> args;
    args.push_back(parse_expr());
    while (match(TokenType::COMMA))
        args.push_back(parse_expr());
    return args;
}

} // namespace compile
