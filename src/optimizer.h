#pragma once
#include "tac.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
using namespace std;

// ─────────────────────────────────────────────
//  An optimization error message
// ─────────────────────────────────────────────
struct OptError {
    string message;
    int    line;
};

// ─────────────────────────────────────────────
//  CodeOptimizer
//
//  Applies multiple optimization passes to TAC:
//    1. Dead Code Elimination (DCE)
//       - Remove assignments to variables that
//         are never used
//    2. Constant Folding
//       - Evaluate constant expressions at
//         compile time
//    3. Copy Propagation
//       - Replace variable uses with their
//         source if they're just copies
//    4. Common Subexpression Elimination (CSE)
//       - Eliminate redundant computations
//    5. Strength Reduction
//       - Replace expensive ops with cheaper ones
//         (e.g., x*2 → x+x)
// ─────────────────────────────────────────────
class CodeOptimizer {
public:
    CodeOptimizer();

    // Optimize the TAC program. Returns optimized version.
    TACProgram optimize(const TACProgram& input);

    const vector<OptError>& errors()    const { return m_errors; }
    bool                    hasErrors() const { return !m_errors.empty(); }

    // Statistics
    int originalInstrCount()  const { return m_origInstrCount; }
    int optimizedInstrCount() const { return m_optInstrCount; }

private:
    vector<OptError> m_errors;
    int              m_origInstrCount;
    int              m_optInstrCount;

    // ── Optimization passes ───────────────────
    TACProgram deadCodeElimination(const TACProgram& input);
    TACProgram constantFolding(const TACProgram& input);
    TACProgram copyPropagation(const TACProgram& input);
    TACProgram commonSubexprElim(const TACProgram& input);
    TACProgram strengthReduction(const TACProgram& input);

    // ── Helpers ───────────────────────────────
    void emitError(const string& msg, int line);

    // Is a variable used after this instruction index?
    bool isVariableUsedAfter(const TACProgram& prog, int instrIdx, 
                             const string& var);

    // Is a string a literal constant?  ("42", "3.14", etc.)
    bool isLiteral(const string& s);

    // Evaluate constant expression
    bool tryEvalConstant(const string& arg1, const string& op, 
                        const string& arg2, string& result);

    // Replace all uses of oldVar with newVar in the program
    TACProgram replaceVariable(const TACProgram& prog, 
                              const string& oldVar, 
                              const string& newVar);

    // Create a key for CSE — represents an expression
    string makeExprKey(TACOp op, const string& arg1, const string& arg2);

    // Check if this TAC op can be strength-reduced
    bool canStrengthReduce(TACOp op, const string& arg1, 
                          const string& arg2);
};
