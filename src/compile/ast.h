#ifndef VARABLIZER_AST_H
#define VARABLIZER_AST_H

#include "source_location.h"
#include "../execute/vtypes.h"

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

namespace compile
{
    using execute::vtype;

    // ── Base node ────────────────────────────────────────────────────────────

    struct AstNode
    {
        SourceLocation loc;
        virtual ~AstNode() = default;
    };

    using NodePtr = std::unique_ptr<AstNode>;

    // ── Expressions ──────────────────────────────────────────────────────────

    // Integer literal: 42, 0xFF, 'A'
    struct IntLit : AstNode
    {
        int64_t value;
        vtype   type;   // resolved at parse time from context (default I64)
    };

    // Variable reference: x, counter
    struct VarRef : AstNode
    {
        std::string name;
    };

    // Binary expression: a + b, a == b, etc.
    struct BinaryExpr : AstNode
    {
        std::string op;    // "+", "-", "*", "/", "%", "&", "|", "^", "<<", ">>",
                           // "==", "!=", "<", ">", "<=", ">=", "&&", "||"
        NodePtr left;
        NodePtr right;
    };

    // Unary expression
    struct UnaryExpr : AstNode
    {
        std::string op;       // "-", "!", "~", "++", "--"
        NodePtr     operand;
        bool        postfix = false;  // true = expr++ / expr--
    };

    // Function call: add(a, b), pf(x)
    struct CallExpr : AstNode
    {
        std::string          callee;
        std::vector<NodePtr> args;
        bool                 is_builtin = false; // pf/p/dd
    };

    // Assignment: x = expr, x += expr
    struct AssignExpr : AstNode
    {
        std::string name;    // target variable
        std::string op;      // "=", "+=", "-=", "*=", "/=", "%=", "&=", "|=", "^="
        NodePtr     value;
    };

    // Ternary: cond ? then : else
    struct TernaryExpr : AstNode
    {
        NodePtr cond;
        NodePtr then_;
        NodePtr else_;
    };

    // ── Statements ───────────────────────────────────────────────────────────

    struct Block : AstNode
    {
        std::vector<NodePtr> stmts;
    };

    // Variable declaration: b_8 int x = expr;
    struct VarDecl : AstNode
    {
        std::string name;
        vtype       type;
        NodePtr     init;   // nullptr if no initializer
    };

    // Expression used as a statement: f(); i++;
    struct ExprStmt : AstNode
    {
        NodePtr expr;
    };

    struct IfStmt : AstNode
    {
        NodePtr cond;
        NodePtr then_;
        NodePtr else_;  // nullptr if no else
    };

    struct WhileStmt : AstNode
    {
        NodePtr cond;
        NodePtr body;
        bool    do_while = false;  // true = do { body } while (cond)
    };

    struct ForStmt : AstNode
    {
        NodePtr init;   // var_decl or expr_stmt (or nullptr)
        NodePtr cond;
        NodePtr step;
        NodePtr body;
    };

    struct ReturnStmt : AstNode
    {
        NodePtr value;  // nullptr = void return
    };

    struct BreakStmt    : AstNode {};
    struct ContinueStmt : AstNode {};
    struct NopStmt      : AstNode {};

    // #label:
    struct LabelStmt : AstNode
    {
        std::string name;
    };

    // goto #label;
    struct GotoStmt : AstNode
    {
        std::string label;
    };

    // ── Top-level ─────────────────────────────────────────────────────────────

    struct FunctionDecl : AstNode
    {
        std::string name;
        vtype       return_type;
        bool        is_void = false;
        std::vector<std::pair<std::string, vtype>> params;
        std::unique_ptr<Block> body;
    };

    struct Program : AstNode
    {
        std::vector<NodePtr> decls;   // FunctionDecl or any stmt
    };

    // ── Helpers ───────────────────────────────────────────────────────────────

    template <typename T>
    T* node_cast(AstNode* n) noexcept
    {
        return dynamic_cast<T*>(n);
    }

    template <typename T>
    const T* node_cast(const AstNode* n) noexcept
    {
        return dynamic_cast<const T*>(n);
    }

} // namespace compile

#endif // VARABLIZER_AST_H
