#pragma once
#include "ast.h"
#include "symbol_table.h"
#include <string>
#include <vector>
#include <iostream>
using namespace std;

// ─────────────────────────────────────────────
//  A semantic error message
// ─────────────────────────────────────────────
struct SemanticError {
    string message;
    int    line;
};

// ─────────────────────────────────────────────
//  SemanticAnalyzer
//
//  Walks the AST recursively (visitor pattern).
//  For each node it:
//    1. Resolves identifiers against the symbol table
//    2. Infers / checks types
//    3. Reports errors without stopping (collects all)
//
//  Checks performed:
//    - Undeclared variable
//    - Redeclaration in same scope
//    - Undeclared function call
//    - Argument count mismatch
//    - Type mismatch in assignment
//    - Return type mismatch
//    - break/continue outside loop
// ─────────────────────────────────────────────
class SemanticAnalyzer {
public:
    SemanticAnalyzer();

    // Run analysis on the whole AST. Returns false if errors found.
    bool analyze(ASTNode* root);

    const vector<SemanticError>& errors()    const { return m_errors; }
    bool                         hasErrors() const { return !m_errors.empty(); }

    // Access symbol table after analysis (for JSON dump)
    SymbolTable& symbolTable() { return m_symTable; }

private:
    SymbolTable          m_symTable;
    vector<SemanticError>m_errors;
    string               m_currentFuncReturnType; // track return type context
    int                  m_loopDepth;             // track loop nesting

    // ── Error reporting ───────────────────────
    void error(const string& msg, int line);

    // ── Type helpers ──────────────────────────
    // Returns "int", "float", "char", "void", or "unknown"
    string inferType(ASTNode* node);

    // Are these types compatible for assignment?  (e.g. int←int, float←int)
    bool   typesCompatible(const string& target, const string& source);

    // Wider of two numeric types  (int+float → float)
    string widenType(const string& a, const string& b);

    // Is this a numeric type?
    bool   isNumeric(const string& t);

    // ── AST visitors ──────────────────────────
    void visitProgram       (ASTNode* node);
    void visitFunctionDecl  (ASTNode* node);
    void visitBlock         (ASTNode* node);
    void visitVarDecl       (ASTNode* node);
    void visitAssign        (ASTNode* node);
    void visitIfStmt        (ASTNode* node);
    void visitWhileStmt     (ASTNode* node);
    void visitForStmt       (ASTNode* node);
    void visitReturnStmt    (ASTNode* node);
    void visitBreakContinue (ASTNode* node);
    void visitExprStmt      (ASTNode* node);
    void visitNode          (ASTNode* node); // dispatcher
};
