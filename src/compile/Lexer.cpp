#include "lexer.h"
#include <cctype>
#include <stdexcept>

namespace compile
{

const std::unordered_map<std::string, TokenType> Lexer::keywords_ = {
    {"if",       TokenType::KW_IF},
    {"else",     TokenType::KW_ELSE},
    {"while",    TokenType::KW_WHILE},
    {"do",       TokenType::KW_DO},
    {"for",      TokenType::KW_FOR},
    {"return",   TokenType::KW_RETURN},
    {"rt",       TokenType::KW_RT},
    {"break",    TokenType::KW_BREAK},
    {"continue", TokenType::KW_CONTINUE},
    {"goto",     TokenType::KW_GOTO},
    {"function", TokenType::KW_FUNCTION},
    {"fn",       TokenType::KW_FN},
    {"void",     TokenType::KW_VOID},
    {"int",      TokenType::KW_INT},
    {"char",     TokenType::KW_CHAR},
    {"unsigned", TokenType::KW_UNSIGNED},
    {"signed",   TokenType::KW_SIGNED},
    {"nop",      TokenType::KW_NOP},
    {"null",     TokenType::KW_NULL},
};

Lexer::Lexer(std::string_view source, std::string filename)
    : source_(source), filename_(std::move(filename))
{}

SourceLocation Lexer::loc() const noexcept
{
    return { filename_, line_, col_ };
}

char Lexer::current() const noexcept
{
    if (pos_ >= source_.size()) return '\0';
    return source_[pos_];
}

char Lexer::peek_char(std::size_t offset) const noexcept
{
    const std::size_t idx = pos_ + offset;
    if (idx >= source_.size()) return '\0';
    return source_[idx];
}

char Lexer::advance()
{
    char c = current();
    ++pos_;
    if (c == '\n') { ++line_; col_ = 1; }
    else           { ++col_; }
    return c;
}

bool Lexer::match(char expected)
{
    if (current() == expected)
    {
        advance();
        return true;
    }
    return false;
}

void Lexer::skip_whitespace_and_comments()
{
    while (pos_ < source_.size())
    {
        char c = current();

        // Whitespace
        if (std::isspace(static_cast<unsigned char>(c)))
        {
            advance();
            continue;
        }

        // Line comment: // ...
        if (c == '/' && peek_char() == '/')
        {
            while (pos_ < source_.size() && current() != '\n')
                advance();
            continue;
        }

        // Block comment: /* ... */
        if (c == '/' && peek_char() == '*')
        {
            advance(); advance(); // consume /*
            while (pos_ < source_.size())
            {
                if (current() == '*' && peek_char() == '/')
                {
                    advance(); advance(); // consume */
                    break;
                }
                advance();
            }
            continue;
        }

        break;
    }
}

bool Lexer::at_end() const noexcept
{
    return pos_ >= source_.size();
}

const Token& Lexer::peek()
{
    if (!lookahead_)
        lookahead_ = scan_token();
    return *lookahead_;
}

Token Lexer::next()
{
    if (lookahead_)
    {
        Token t = std::move(*lookahead_);
        lookahead_.reset();
        return t;
    }
    return scan_token();
}

Token Lexer::scan_token()
{
    skip_whitespace_and_comments();

    if (pos_ >= source_.size())
    {
        Token t;
        t.type   = TokenType::EOF_TOK;
        t.lexeme = "";
        t.loc    = loc();
        return t;
    }

    const char c = current();

    // Numbers
    if (std::isdigit(static_cast<unsigned char>(c)))
        return scan_number();

    // Char literal
    if (c == '\'')
        return scan_char_literal();

    // Identifiers and keywords
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')
        return scan_identifier();

    // Operators and delimiters
    SourceLocation l = loc();
    advance();

    auto make = [&](TokenType t, std::string lex) -> Token {
        Token tok;
        tok.type   = t;
        tok.lexeme = std::move(lex);
        tok.loc    = l;
        return tok;
    };

    switch (c)
    {
        case '(': return make(TokenType::LPAREN,    "(");
        case ')': return make(TokenType::RPAREN,    ")");
        case '{': return make(TokenType::LBRACE,    "{");
        case '}': return make(TokenType::RBRACE,    "}");
        case ';': return make(TokenType::SEMICOLON, ";");
        case ',': return make(TokenType::COMMA,     ",");
        case '#': return make(TokenType::HASH,      "#");
        case '~': return make(TokenType::TILDE,     "~");
        case '[': return make(TokenType::LBRACKET,  "[");
        case ']': return make(TokenType::RBRACKET,  "]");

        case '+':
            if (match('+')) return make(TokenType::PLUS_PLUS,   "++");
            if (match('=')) return make(TokenType::PLUS_ASSIGN, "+=");
            return make(TokenType::PLUS, "+");

        case '-':
            if (match('-')) return make(TokenType::MINUS_MINUS,   "--");
            if (match('=')) return make(TokenType::MINUS_ASSIGN,  "-=");
            return make(TokenType::MINUS, "-");

        case '*':
            if (match('=')) return make(TokenType::STAR_ASSIGN, "*=");
            return make(TokenType::STAR, "*");

        case '/':
            if (match('=')) return make(TokenType::SLASH_ASSIGN, "/=");
            return make(TokenType::SLASH, "/");

        case '%':
            if (match('=')) return make(TokenType::PERCENT_ASSIGN, "%=");
            return make(TokenType::PERCENT, "%");

        case '&':
            if (match('&')) return make(TokenType::AMP_AMP,    "&&");
            if (match('=')) return make(TokenType::AMP_ASSIGN, "&=");
            return make(TokenType::AMP, "&");

        case '|':
            if (match('|')) return make(TokenType::PIPE_PIPE,    "||");
            if (match('=')) return make(TokenType::PIPE_ASSIGN,  "|=");
            return make(TokenType::PIPE, "|");

        case '^':
            if (match('=')) return make(TokenType::CARET_ASSIGN, "^=");
            return make(TokenType::CARET, "^");

        case '!':
            if (match('=')) return make(TokenType::BANG_EQ, "!=");
            return make(TokenType::BANG, "!");

        case '=':
            if (match('=')) return make(TokenType::EQ_EQ,  "==");
            return make(TokenType::ASSIGN, "=");

        case '<':
            if (match('<')) return make(TokenType::LSHIFT, "<<");
            if (match('=')) return make(TokenType::LT_EQ,  "<=");
            return make(TokenType::LT, "<");

        case '>':
            if (match('>')) return make(TokenType::RSHIFT, ">>");
            if (match('=')) return make(TokenType::GT_EQ,  ">=");
            return make(TokenType::GT, ">");

        case '?':
            return make(TokenType::QUESTION, "?");

        case ':':
            if (match('=')) return make(TokenType::COLON_ASSIGN, ":=");
            return make(TokenType::COLON, ":");

        default:
            throw CompileError(
                std::string("unexpected character '") + c + "'",
                l
            );
    }
}

Token Lexer::scan_number()
{
    SourceLocation l = loc();
    std::string lexeme;
    int64_t value = 0;

    // Hex: 0x...
    if (current() == '0' && (peek_char() == 'x' || peek_char() == 'X'))
    {
        lexeme += advance(); // '0'
        lexeme += advance(); // 'x'
        while (pos_ < source_.size() && std::isxdigit(static_cast<unsigned char>(current())))
            lexeme += advance();
        value = std::stoll(lexeme, nullptr, 16);
    }
    else
    {
        while (pos_ < source_.size() && std::isdigit(static_cast<unsigned char>(current())))
            lexeme += advance();
        value = std::stoll(lexeme);
    }

    Token t;
    t.type    = TokenType::INT_LIT;
    t.lexeme  = lexeme;
    t.int_val = value;
    t.loc     = l;
    return t;
}

Token Lexer::scan_char_literal()
{
    SourceLocation l = loc();
    advance(); // consume opening '

    char ch = '\0';
    if (current() == '\\')
    {
        advance();
        switch (current())
        {
            case 'n':  ch = '\n'; break;
            case 't':  ch = '\t'; break;
            case 'r':  ch = '\r'; break;
            case '0':  ch = '\0'; break;
            case '\'': ch = '\''; break;
            case '\\': ch = '\\'; break;
            default:
                throw CompileError(
                    std::string("unknown escape sequence '\\") + current() + "'",
                    loc()
                );
        }
        advance();
    }
    else if (current() != '\'')
    {
        ch = advance();
    }
    else
    {
        throw CompileError("empty char literal", l);
    }

    if (current() != '\'')
        throw CompileError("unterminated char literal", l);
    advance(); // consume closing '

    Token t;
    t.type    = TokenType::CHAR_LIT;
    t.lexeme  = std::string(1, ch);
    t.int_val = static_cast<int64_t>(static_cast<unsigned char>(ch));
    t.loc     = l;
    return t;
}

Token Lexer::scan_identifier()
{
    SourceLocation l = loc();
    std::string lexeme;

    while (pos_ < source_.size())
    {
        char c = current();
        // Allow b_8, b_16, y_4 etc. (digits after underscore)
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_')
            lexeme += advance();
        else
            break;
    }

    TokenType type = keyword_type(lexeme);

    Token t;
    t.type   = type;
    t.lexeme = lexeme;
    t.loc    = l;
    return t;
}

TokenType Lexer::keyword_type(const std::string& s) noexcept
{
    auto it = keywords_.find(s);
    if (it != keywords_.end())
        return it->second;
    return TokenType::IDENT;
}

} // namespace compile
