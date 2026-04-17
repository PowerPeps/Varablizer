#ifndef VARABLIZER_PARSER_H
#define VARABLIZER_PARSER_H

#include "lexer.h"
#include "ast.h"
#include "type_resolver.h"

#include <memory>
#include <string>

namespace compile
{
    class Parser
    {
    public:
        explicit Parser(Lexer& lexer);

        std::unique_ptr<Program> parse();

    private:
        Lexer& lex_;

        // ── Token helpers ──────────────────────────────────────────────────
        const Token& peek();
        Token        consume();
        Token        expect(TokenType type);
        bool         check(TokenType type);
        bool         match(TokenType type);

        // ── Type parsing ───────────────────────────────────────────────────
        struct TypeResult
        {
            vtype type;
            bool  is_void    = false;
            bool  is_pointer = false;
            vtype pointee;   // valid when is_pointer == true
        };
        bool         is_type_start();
        TypeResult   parse_type();

        // ── Top-level ──────────────────────────────────────────────────────
        NodePtr parse_decl();
        std::unique_ptr<FunctionDecl> parse_function(TypeResult ret, const Token& name_tok);

        // ── Statements ─────────────────────────────────────────────────────
        NodePtr parse_stmt();
        std::unique_ptr<Block> parse_block();
        NodePtr parse_var_decl(TypeResult type_res, const Token& name_tok);
        NodePtr parse_if_stmt();
        NodePtr parse_while_stmt();
        NodePtr parse_do_while_stmt();
        NodePtr parse_for_stmt();
        NodePtr parse_return_stmt();
        NodePtr parse_label_or_goto();

        // ── Expressions ────────────────────────────────────────────────────
        NodePtr parse_expr();
        NodePtr parse_assignment();
        NodePtr parse_ternary();
        NodePtr parse_logical();
        NodePtr parse_comparison();
        NodePtr parse_bitwise();
        NodePtr parse_shift();
        NodePtr parse_addition();
        NodePtr parse_multiply();
        NodePtr parse_unary();
        NodePtr parse_postfix();
        NodePtr parse_primary();
        NodePtr parse_call(Token name_tok, bool is_builtin = false);

        std::vector<NodePtr> parse_arg_list();

        [[noreturn]] void error(const std::string& msg);
        [[noreturn]] void error(const std::string& msg, const SourceLocation& loc);
    };

} // namespace compile

#endif // VARABLIZER_PARSER_H
