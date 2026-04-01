#ifndef VARABLIZER_TOKEN_H
#define VARABLIZER_TOKEN_H

#include "source_location.h"
#include <string>
#include <cstdint>

namespace compile
{
    enum class TokenType
    {
        // ── Literals ─────────────────────────────────────────────────────────
        INT_LIT,        // 42, 0xFF
        CHAR_LIT,       // 'A'  (stored as int64 ASCII value)

        // ── Identifiers ──────────────────────────────────────────────────────
        IDENT,

        // ── Keywords ─────────────────────────────────────────────────────────
        KW_IF,
        KW_ELSE,
        KW_WHILE,
        KW_DO,
        KW_FOR,
        KW_RETURN,
        KW_RT,          // "rt" (alias for return)
        KW_BREAK,
        KW_CONTINUE,
        KW_GOTO,
        KW_FUNCTION,
        KW_FN,
        KW_VOID,
        KW_INT,
        KW_CHAR,
        KW_UNSIGNED,
        KW_SIGNED,
        KW_NOP,
        KW_NULL,        // "null"

        // ── Arithmetic operators ──────────────────────────────────────────────
        PLUS,           // +
        MINUS,          // -
        STAR,           // *
        SLASH,          // /
        PERCENT,        // %

        // ── Bitwise operators ─────────────────────────────────────────────────
        AMP,            // &
        PIPE,           // |
        CARET,          // ^
        TILDE,          // ~
        LSHIFT,         // <<
        RSHIFT,         // >>

        // ── Increment / decrement ─────────────────────────────────────────────
        PLUS_PLUS,      // ++
        MINUS_MINUS,    // --

        // ── Comparison operators ──────────────────────────────────────────────
        EQ_EQ,          // ==
        BANG_EQ,        // !=
        LT,             // <
        GT,             // >
        LT_EQ,          // <=
        GT_EQ,          // >=

        // ── Logical operators ─────────────────────────────────────────────────
        AMP_AMP,        // &&
        PIPE_PIPE,      // ||
        BANG,           // !

        // ── Assignment operators ──────────────────────────────────────────────
        ASSIGN,         // =
        COLON_ASSIGN,   // :=
        PLUS_ASSIGN,    // +=
        MINUS_ASSIGN,   // -=
        STAR_ASSIGN,    // *=
        SLASH_ASSIGN,   // /=
        PERCENT_ASSIGN, // %=
        AMP_ASSIGN,     // &=
        PIPE_ASSIGN,    // |=
        CARET_ASSIGN,   // ^=

        // ── Ternary ───────────────────────────────────────────────────────────
        QUESTION,       // ?
        COLON,          // :

        // ── Delimiters ────────────────────────────────────────────────────────
        LPAREN,         // (
        RPAREN,         // )
        LBRACE,         // {
        RBRACE,         // }
        SEMICOLON,      // ;
        COMMA,          // ,
        HASH,           // #
        LBRACKET,       // [
        RBRACKET,       // ]

        // ── Special ───────────────────────────────────────────────────────────
        EOF_TOK,
    };

    struct Token
    {
        TokenType    type     = TokenType::EOF_TOK;
        std::string  lexeme;
        int64_t      int_val  = 0;   // valid when type == INT_LIT or CHAR_LIT
        SourceLocation loc;
    };

    // Human-readable name (for error messages)
    [[nodiscard]] inline const char* token_type_name(TokenType t) noexcept
    {
        switch (t)
        {
            case TokenType::INT_LIT:        return "integer literal";
            case TokenType::CHAR_LIT:       return "char literal";
            case TokenType::IDENT:          return "identifier";
            case TokenType::KW_IF:          return "'if'";
            case TokenType::KW_ELSE:        return "'else'";
            case TokenType::KW_WHILE:       return "'while'";
            case TokenType::KW_DO:          return "'do'";
            case TokenType::KW_FOR:         return "'for'";
            case TokenType::KW_RETURN:      return "'return'";
            case TokenType::KW_RT:          return "'rt'";
            case TokenType::KW_BREAK:       return "'break'";
            case TokenType::KW_CONTINUE:    return "'continue'";
            case TokenType::KW_GOTO:        return "'goto'";
            case TokenType::KW_FUNCTION:    return "'function'";
            case TokenType::KW_FN:          return "'fn'";
            case TokenType::KW_VOID:        return "'void'";
            case TokenType::KW_INT:         return "'int'";
            case TokenType::KW_CHAR:        return "'char'";
            case TokenType::KW_UNSIGNED:    return "'unsigned'";
            case TokenType::KW_SIGNED:      return "'signed'";
            case TokenType::KW_NOP:         return "'nop'";
            case TokenType::KW_NULL:        return "'null'";
            case TokenType::PLUS:           return "'+'";
            case TokenType::MINUS:          return "'-'";
            case TokenType::STAR:           return "'*'";
            case TokenType::SLASH:          return "'/'";
            case TokenType::PERCENT:        return "'%'";
            case TokenType::AMP:            return "'&'";
            case TokenType::PIPE:           return "'|'";
            case TokenType::CARET:          return "'^'";
            case TokenType::TILDE:          return "'~'";
            case TokenType::LSHIFT:         return "'<<'";
            case TokenType::RSHIFT:         return "'>>'";
            case TokenType::PLUS_PLUS:      return "'++'";
            case TokenType::MINUS_MINUS:    return "'--'";
            case TokenType::EQ_EQ:          return "'=='";
            case TokenType::BANG_EQ:        return "'!='";
            case TokenType::LT:             return "'<'";
            case TokenType::GT:             return "'>'";
            case TokenType::LT_EQ:          return "'<='";
            case TokenType::GT_EQ:          return "'>='";
            case TokenType::AMP_AMP:        return "'&&'";
            case TokenType::PIPE_PIPE:      return "'||'";
            case TokenType::BANG:           return "'!'";
            case TokenType::ASSIGN:         return "'='";
            case TokenType::COLON_ASSIGN:   return "':='";
            case TokenType::PLUS_ASSIGN:    return "'+='";
            case TokenType::MINUS_ASSIGN:   return "'-='";
            case TokenType::STAR_ASSIGN:    return "'*='";
            case TokenType::SLASH_ASSIGN:   return "'/='";
            case TokenType::PERCENT_ASSIGN: return "'%='";
            case TokenType::AMP_ASSIGN:     return "'&='";
            case TokenType::PIPE_ASSIGN:    return "'|='";
            case TokenType::CARET_ASSIGN:   return "'^='";
            case TokenType::QUESTION:       return "'?'";
            case TokenType::COLON:          return "':'";
            case TokenType::LPAREN:         return "'('";
            case TokenType::RPAREN:         return "')'";
            case TokenType::LBRACE:         return "'{'";
            case TokenType::RBRACE:         return "'}'";
            case TokenType::SEMICOLON:      return "';'";
            case TokenType::COMMA:          return "','";
            case TokenType::HASH:           return "'#'";
            case TokenType::LBRACKET:       return "'['";
            case TokenType::RBRACKET:       return "']'";
            case TokenType::EOF_TOK:        return "end of file";
        }
        return "unknown";
    }

} // namespace compile

#endif // VARABLIZER_TOKEN_H
