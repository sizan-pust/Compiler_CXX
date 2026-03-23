#pragma once
#include <string>
#include <vector>
#include <memory>

// ─────────────────────────────────────────────
//  Every AST node type
// ─────────────────────────────────────────────
enum class NodeType {
    PROGRAM,
    FUNCTION_DECL,
    PARAM,
    BLOCK,
    VAR_DECL,
    ASSIGN,
    IF_STMT,
    WHILE_STMT,
    FOR_STMT,
    RETURN_STMT,
    BREAK_STMT,
    CONTINUE_STMT,
    EXPR_STMT,
    BINARY_OP,
    UNARY_OP,
    LITERAL_INT,
    LITERAL_FLOAT,
    LITERAL_CHAR,
    LITERAL_STRING,
    IDENTIFIER,
    FUNC_CALL
};

// ─────────────────────────────────────────────
//  Base node — every node has a type + line
// ─────────────────────────────────────────────
struct ASTNode {
    NodeType    kind;
    int         line = 0;
    std::string sval;   // string payload  (name, op, literal text)

    // Children — owned by this node
    std::vector<std::unique_ptr<ASTNode>> children;

    explicit ASTNode(NodeType k, int ln = 0, const std::string& s = "")
        : kind(k), line(ln), sval(s) {}

    // Convenience: add a child and return raw pointer for further use
    ASTNode* addChild(std::unique_ptr<ASTNode> child) {
        children.push_back(std::move(child));
        return children.back().get();
    }

    // Human-readable kind name (for JSON output)
    static std::string kindName(NodeType k) {
        switch (k) {
            case NodeType::PROGRAM:        return "Program";
            case NodeType::FUNCTION_DECL:  return "FunctionDecl";
            case NodeType::PARAM:          return "Param";
            case NodeType::BLOCK:          return "Block";
            case NodeType::VAR_DECL:       return "VarDecl";
            case NodeType::ASSIGN:         return "Assign";
            case NodeType::IF_STMT:        return "IfStmt";
            case NodeType::WHILE_STMT:     return "WhileStmt";
            case NodeType::FOR_STMT:       return "ForStmt";
            case NodeType::RETURN_STMT:    return "ReturnStmt";
            case NodeType::BREAK_STMT:     return "BreakStmt";
            case NodeType::CONTINUE_STMT:  return "ContinueStmt";
            case NodeType::EXPR_STMT:      return "ExprStmt";
            case NodeType::BINARY_OP:      return "BinaryOp";
            case NodeType::UNARY_OP:       return "UnaryOp";
            case NodeType::LITERAL_INT:    return "LiteralInt";
            case NodeType::LITERAL_FLOAT:  return "LiteralFloat";
            case NodeType::LITERAL_CHAR:   return "LiteralChar";
            case NodeType::LITERAL_STRING: return "LiteralString";
            case NodeType::IDENTIFIER:     return "Identifier";
            case NodeType::FUNC_CALL:      return "FuncCall";
            default:                       return "Unknown";
        }
    }
};

using ASTNodePtr = std::unique_ptr<ASTNode>;

// ─────────────────────────────────────────────
//  Factory helpers  (keep construction sites clean)
// ─────────────────────────────────────────────
inline ASTNodePtr makeNode(NodeType k, int line = 0, const std::string& s = "") {
    return std::make_unique<ASTNode>(k, line, s);
}
