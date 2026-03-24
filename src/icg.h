#pragma once
#include "ast.h"
#include "tac.h"
#include <string>
#include <unordered_map>
using namespace std;

// ─────────────────────────────────────────────
//  ICGError — generation-time error
// ─────────────────────────────────────────────
struct ICGError {
    string message;
    int    line;
};

// ─────────────────────────────────────────────
//  ICGenerator
//
//  Walks the (semantically-checked) AST and
//  emits a flat list of TAC instructions.
//
//  Key ideas:
//    - newTemp()  → fresh temp variable  t0, t1 ...
//    - newLabel() → fresh jump label     L0, L1 ...
//    - genExpr()  → returns the name (temp or var)
//                   that holds the expression result
//    - genStmt()  → emits instructions for a statement
//
//  Control flow translation:
//
//    if (cond) { T } else { E }
//      →  t0  = cond
//         if t0 == 0 goto L_else
//         <T instructions>
//         goto L_end
//         L_else:
//         <E instructions>
//         L_end:
//
//    while (cond) { B }
//      →  L_start:
//         t0 = cond
//         if t0 == 0 goto L_end
//         <B instructions>
//         goto L_start
//         L_end:
//
//    for (init; cond; incr) { B }
//      →  <init>
//         L_start:
//         t0 = cond
//         if t0 == 0 goto L_end
//         <B instructions>
//         L_incr:
//         <incr>
//         goto L_start
//         L_end:
// ─────────────────────────────────────────────
class ICGenerator {
public:
    ICGenerator();

    // Generate TAC for the whole AST
    bool generate(ASTNode* root);

    const TACProgram&        program()   const { return m_prog; }
    const vector<ICGError>&  errors()    const { return m_errors; }
    bool                     hasErrors() const { return !m_errors.empty(); }

private:
    TACProgram       m_prog;
    vector<ICGError> m_errors;

    int m_tempCount;   // counter for t0, t1, t2 ...
    int m_labelCount;  // counter for L0, L1, L2 ...

    // loop context stacks (for break/continue)
    vector<string> m_breakStack;    // label to jump to on break
    vector<string> m_continueStack; // label to jump to on continue

    // ── helpers ──────────────────────────────
    string newTemp();
    string newLabel();
    void   emitError(const string& msg, int line);

    void   emit(TACOp op,
                const string& arg1   = "",
                const string& arg2   = "",
                const string& result = "",
                int           line   = 0);

    // ── visitors ─────────────────────────────
    void   genProgram     (ASTNode* node);
    void   genFuncDecl    (ASTNode* node);
    void   genBlock       (ASTNode* node);
    void   genStmt        (ASTNode* node);
    void   genVarDecl     (ASTNode* node);
    void   genAssign      (ASTNode* node);
    void   genIfStmt      (ASTNode* node);
    void   genWhileStmt   (ASTNode* node);
    void   genForStmt     (ASTNode* node);
    void   genReturnStmt  (ASTNode* node);
    void   genExprStmt    (ASTNode* node);

    // genExpr: emit instructions and return the
    // name of the variable / temp holding the result
    string genExpr        (ASTNode* node);
    string genBinaryOp    (ASTNode* node);
    string genUnaryOp     (ASTNode* node);
    string genFuncCall    (ASTNode* node);
};
