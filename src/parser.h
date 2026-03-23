#pragma once
#include "lexer.h"
#include "ast.h"
#include <vector>
#include <string>
#include <stdexcept>

// ─────────────────────────────────────────────
//  Parse error — carries line info
// ─────────────────────────────────────────────
struct ParseError {
    std::string message;
    int         line;
    int         col;
};

// ─────────────────────────────────────────────
//  Parser
//
//  Implements a hand-written recursive-descent
//  parser for a C subset.
//
//  Grammar summary (simplified):
//
//  program       → decl*
//  decl          → funcDecl | varDecl
//  funcDecl      → type IDENT '(' paramList ')' block
//  paramList     → (param (',' param)*)?
//  param         → type IDENT
//  block         → '{' stmt* '}'
//  stmt          → varDecl | ifStmt | whileStmt
//                | forStmt | returnStmt | breakStmt
//                | continueStmt | exprStmt
//  varDecl       → type IDENT ('=' expr)? ';'
//  ifStmt        → 'if' '(' expr ')' block ('else' (ifStmt|block))?
//  whileStmt     → 'while' '(' expr ')' block
//  forStmt       → 'for' '(' forInit expr? ';' expr? ')' block
//  returnStmt    → 'return' expr? ';'
//  exprStmt      → expr ';'
//  expr          → assignment
//  assignment    → IDENT ('='|'+='|'-='|'*='|'/=') expr | logicOr
//  logicOr       → logicAnd ('||' logicAnd)*
//  logicAnd      → equality ('&&' equality)*
//  equality      → relational (('=='|'!=') relational)*
//  relational    → additive (('<'|'>'|'<='|'>=') additive)*
//  additive      → multiplicative (('+'|'-') multiplicative)*
//  multiplicative→ unary (('*'|'/'|'%') unary)*
//  unary         → ('!'|'-'|'++'|'--') unary | postfix
//  postfix       → primary ('++'|'--')?
//  primary       → LIT_INT | LIT_FLOAT | LIT_CHAR | LIT_STRING
//                | IDENT '(' argList ')' | IDENT | '(' expr ')'
// ─────────────────────────────────────────────
class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens);

    // Parse and return the root AST node; collect errors
    ASTNodePtr parse();

    const std::vector<ParseError>& errors() const { return m_errors; }
    bool hasErrors() const { return !m_errors.empty(); }

private:
    std::vector<Token>      m_tokens;
    size_t                  m_pos;
    std::vector<ParseError> m_errors;

    // ── token navigation ─────────────────────
    const Token& peek(int offset = 0) const;
    const Token& advance();
    bool          check(TokenType t) const;
    bool          match(TokenType t);                    // consume if match
    const Token&  expect(TokenType t, const std::string& msg);
    bool          isAtEnd() const;
    bool          isTypeName() const;                    // int/float/char/void/double

    // ── error handling ────────────────────────
    void          error(const std::string& msg);
    void          synchronize();                         // panic-mode recovery

    // ── grammar rules ────────────────────────
    ASTNodePtr    parseDecl();
    ASTNodePtr    parseFuncDecl(const std::string& retType, const Token& nameTok);
    ASTNodePtr    parseVarDecl(const std::string& typeName, const Token& nameTok);
    ASTNodePtr    parseBlock();
    ASTNodePtr    parseStmt();
    ASTNodePtr    parseIfStmt();
    ASTNodePtr    parseWhileStmt();
    ASTNodePtr    parseForStmt();
    ASTNodePtr    parseReturnStmt();
    ASTNodePtr    parseExprStmt();

    // ── expression hierarchy ─────────────────
    ASTNodePtr    parseExpr();
    ASTNodePtr    parseAssignment();
    ASTNodePtr    parseLogicOr();
    ASTNodePtr    parseLogicAnd();
    ASTNodePtr    parseEquality();
    ASTNodePtr    parseRelational();
    ASTNodePtr    parseAdditive();
    ASTNodePtr    parseMultiplicative();
    ASTNodePtr    parseUnary();
    ASTNodePtr    parsePostfix();
    ASTNodePtr    parsePrimary();

    std::string   parseTypeName();    // consume type tokens, return string
};
