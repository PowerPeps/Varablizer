#ifndef VARABLIZER_LEXER_H
#define VARABLIZER_LEXER_H

#include "token.h"
#include "error.h"

#include <string>
#include <string_view>
#include <unordered_map>
#include <optional>

namespace compile
{
    class Lexer
    {
    public:
        Lexer(std::string_view source, std::string filename = "<input>");

        // Return the next token (advances position)
        Token next();

        // Return the next token without advancing
        [[nodiscard]] const Token& peek();

        // Return true if at end of file
        [[nodiscard]] bool at_end() const noexcept;

    private:
        std::string_view source_;
        std::string      filename_;
        std::size_t      pos_   = 0;
        std::uint32_t    line_  = 1;
        std::uint32_t    col_   = 1;

        std::optional<Token> lookahead_;

        [[nodiscard]] SourceLocation loc() const noexcept;
        [[nodiscard]] char current() const noexcept;
        [[nodiscard]] char peek_char(std::size_t offset = 1) const noexcept;
        char advance();
        bool match(char expected);
        void skip_whitespace_and_comments();

        Token scan_token();
        Token scan_number();
        Token scan_char_literal();
        Token scan_identifier();

        static TokenType keyword_type(const std::string& s) noexcept;
        static const std::unordered_map<std::string, TokenType> keywords_;
    };

} // namespace compile

#endif // VARABLIZER_LEXER_H
