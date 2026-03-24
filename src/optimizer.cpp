#include "optimizer.h"
#include <sstream>
#include <cctype>
#include <cstdlib>
using namespace std;

// ─────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────
CodeOptimizer::CodeOptimizer()
    : m_origInstrCount(0), m_optInstrCount(0) {}

// ─────────────────────────────────────────────
//  Error reporting
// ─────────────────────────────────────────────
void CodeOptimizer::emitError(const string& msg, int line) {
    m_errors.push_back({ msg, line });
}

// ─────────────────────────────────────────────
//  Main optimization entry point
//
//  Applies optimization passes in sequence:
//  1. Constant folding (simplifies expressions)
//  2. Dead code elimination (removes unused assigns)
//  3. Copy propagation (replaces copies)
//  4. Common subexpression elimination
//  5. Strength reduction (expensive→cheap ops)
// ─────────────────────────────────────────────
TACProgram CodeOptimizer::optimize(const TACProgram& input) {
    m_origInstrCount = input.instrs.size();

    TACProgram current = input;

    // Apply passes multiple times for fixed-point optimization
    bool changed = true;
    int iterations = 0;
    const int MAX_ITERATIONS = 3;

    while (changed && iterations < MAX_ITERATIONS) {
        changed = false;

        // Pass 1: Constant folding
        TACProgram folded = constantFolding(current);
        if (folded.instrs.size() != current.instrs.size()) {
            current = folded;
            changed = true;
        }

        // Pass 2: Dead code elimination
        TACProgram dce = deadCodeElimination(current);
        if (dce.instrs.size() != current.instrs.size()) {
            current = dce;
            changed = true;
        }

        // Pass 3: Copy propagation
        TACProgram copyprop = copyPropagation(current);
        if (copyprop.instrs.size() != current.instrs.size()) {
            current = copyprop;
            changed = true;
        }

        // Pass 4: Common subexpression elimination
        TACProgram cse = commonSubexprElim(current);
        if (cse.instrs.size() != current.instrs.size()) {
            current = cse;
            changed = true;
        }

        // Pass 5: Strength reduction
        TACProgram sr = strengthReduction(current);
        if (sr.instrs.size() != current.instrs.size()) {
            current = sr;
            changed = true;
        }

        iterations++;
    }

    m_optInstrCount = current.instrs.size();
    return current;
}

// ─────────────────────────────────────────────
//  Check if a string is a literal constant
// ─────────────────────────────────────────────
bool CodeOptimizer::isLiteral(const string& s) {
    if (s.empty()) return false;
    
    // Check for integer or float literal
    bool hasDecimal = false;
    for (size_t i = 0; i < s.length(); ++i) {
        char c = s[i];
        if (i == 0 && c == '-') continue; // negative sign
        if (c == '.') {
            if (hasDecimal) return false; // multiple decimals
            hasDecimal = true;
        } else if (!isdigit(c)) {
            return false;
        }
    }
    return true;
}

// ─────────────────────────────────────────────
//  Attempt to evaluate constant expression
// ─────────────────────────────────────────────
bool CodeOptimizer::tryEvalConstant(const string& arg1, const string& op,
                                    const string& arg2, string& result) {
    if (!isLiteral(arg1) || !isLiteral(arg2)) return false;

    try {
        double val1 = stod(arg1);
        double val2 = stod(arg2);
        double res;

        if      (op == "+")  res = val1 + val2;
        else if (op == "-")  res = val1 - val2;
        else if (op == "*")  res = val1 * val2;
        else if (op == "/")  {
            if (val2 == 0) return false;
            res = val1 / val2;
        }
        else if (op == "%")  {
            if (val2 == 0) return false;
            res = (int)val1 % (int)val2;
        }
        else if (op == "==") res = (val1 == val2) ? 1 : 0;
        else if (op == "!=") res = (val1 != val2) ? 1 : 0;
        else if (op == "<")  res = (val1 < val2) ? 1 : 0;
        else if (op == ">")  res = (val1 > val2) ? 1 : 0;
        else if (op == "<=") res = (val1 <= val2) ? 1 : 0;
        else if (op == ">=") res = (val1 >= val2) ? 1 : 0;
        else return false;

        // Convert back to string (prefer integer if whole number)
        if (res == (long long)res) {
            result = to_string((long long)res);
        } else {
            result = to_string(res);
        }
        return true;
    } catch (...) {
        return false;
    }
}

// ─────────────────────────────────────────────
//  Constant Folding
//
//  Evaluate compile-time constant expressions.
//  Example:  t0 = 5 + 3   →  t0 = 8
// ─────────────────────────────────────────────
TACProgram CodeOptimizer::constantFolding(const TACProgram& input) {
    TACProgram result;
    unordered_map<string, string> constMap; // maps temp vars to constants

    for (const auto& instr : input.instrs) {
        TACInstr newInstr = instr;

        // Check if this is a binary operation with constant args
        if (instr.op >= TACOp::ADD && instr.op <= TACOp::GEQ) {
            string opStr;
            switch (instr.op) {
                case TACOp::ADD: opStr = "+";  break;
                case TACOp::SUB: opStr = "-";  break;
                case TACOp::MUL: opStr = "*";  break;
                case TACOp::DIV: opStr = "/";  break;
                case TACOp::MOD: opStr = "%";  break;
                case TACOp::EQ:  opStr = "=="; break;
                case TACOp::NEQ: opStr = "!="; break;
                case TACOp::LT:  opStr = "<";  break;
                case TACOp::GT:  opStr = ">";  break;
                case TACOp::LEQ: opStr = "<="; break;
                case TACOp::GEQ: opStr = ">="; break;
                default: opStr = "";
            }

            string evalResult;
            if (tryEvalConstant(instr.arg1, opStr, instr.arg2, evalResult)) {
                // Fold to a COPY of the constant
                newInstr.op = TACOp::COPY;
                newInstr.arg1 = evalResult;
                newInstr.arg2 = "";
                
                if (!instr.result.empty()) {
                    constMap[instr.result] = evalResult;
                }
            }
        }

        // For COPY operations, track constants
        if (newInstr.op == TACOp::COPY && isLiteral(newInstr.arg1)) {
            if (!newInstr.result.empty()) {
                constMap[newInstr.result] = newInstr.arg1;
            }
        }

        result.emit(newInstr);
    }

    return result;
}

// ─────────────────────────────────────────────
//  Dead Code Elimination
//
//  Remove assignments to variables that are
//  never used afterwards.
// ─────────────────────────────────────────────
TACProgram CodeOptimizer::deadCodeElimination(const TACProgram& input) {
    TACProgram result;

    for (size_t i = 0; i < input.instrs.size(); ++i) {
        const auto& instr = input.instrs[i];

        // Keep control flow and function-related instructions
        if (instr.op == TACOp::LABEL   || instr.op == TACOp::GOTO    ||
            instr.op == TACOp::IF_FALSE || instr.op == TACOp::IF_TRUE ||
            instr.op == TACOp::FUNC_BEGIN || instr.op == TACOp::FUNC_END ||
            instr.op == TACOp::PARAM   || instr.op == TACOp::CALL    ||
            instr.op == TACOp::RETURN) {
            result.emit(instr);
            continue;
        }

        // For assignments: check if result is ever used
        if (instr.op == TACOp::COPY || 
            (instr.op >= TACOp::ADD && instr.op <= TACOp::DEC)) {
            
            if (instr.result.empty()) {
                result.emit(instr);
                continue;
            }

            // Check if this result variable is used later
            bool isUsed = isVariableUsedAfter(input, i, instr.result);

            if (isUsed) {
                result.emit(instr);
            }
            // else: skip dead assignment
        } else {
            result.emit(instr);
        }
    }

    return result;
}

// ─────────────────────────────────────────────
//  Check if variable is used after instruction at index
// ─────────────────────────────────────────────
bool CodeOptimizer::isVariableUsedAfter(const TACProgram& prog, int instrIdx,
                                       const string& var) {
    // Scan forward from instrIdx+1 to see if var is used
    for (size_t i = instrIdx + 1; i < prog.instrs.size(); ++i) {
        const auto& instr = prog.instrs[i];
        
        // Check if var appears in any argument
        if (instr.arg1 == var || instr.arg2 == var || instr.result == var) {
            return true;
        }
    }
    return false;
}

// ─────────────────────────────────────────────
//  Copy Propagation
//
//  If x = y, replace future uses of x with y.
//  Example:   t0 = t1;  t2 = t0 + 1   →  t2 = t1 + 1; t0 = t1 (kept)
// ─────────────────────────────────────────────
TACProgram CodeOptimizer::copyPropagation(const TACProgram& input) {
    TACProgram result;
    unordered_map<string, string> copyMap; // x → what x is a copy of

    for (const auto& instr : input.instrs) {
        TACInstr newInstr = instr;

        // Build copy map for COPY operations
        if (instr.op == TACOp::COPY && !isLiteral(instr.arg1)) {
            copyMap[instr.result] = instr.arg1;
        } else {
            // Clear copy map for non-copy instructions that define results
            if (!instr.result.empty() && instr.op != TACOp::COPY) {
                copyMap.erase(instr.result);
            }
        }

        // Propagate copies in arguments
        if (copyMap.count(newInstr.arg1)) {
            newInstr.arg1 = copyMap[newInstr.arg1];
        }
        if (copyMap.count(newInstr.arg2)) {
            newInstr.arg2 = copyMap[newInstr.arg2];
        }

        result.emit(newInstr);
    }

    return result;
}

// ─────────────────────────────────────────────
//  Create expression key for CSE
// ─────────────────────────────────────────────
string CodeOptimizer::makeExprKey(TACOp op, const string& arg1,
                                 const string& arg2) {
    return TACInstr::opName(op) + ":" + arg1 + ":" + arg2;
}

// ─────────────────────────────────────────────
//  Common Subexpression Elimination
//
//  If we compute the same expression twice,
//  reuse the first result.
//  Example:  t0 = a + b;  t1 = a + b;  → t0 = a + b; t1 = t0
// ─────────────────────────────────────────────
TACProgram CodeOptimizer::commonSubexprElim(const TACProgram& input) {
    TACProgram result;
    unordered_map<string, string> exprMap; // expression key → result temp

    for (const auto& instr : input.instrs) {
        // Don't optimize across control flow
        if (instr.op == TACOp::LABEL || instr.op == TACOp::GOTO ||
            instr.op == TACOp::IF_FALSE || instr.op == TACOp::IF_TRUE) {
            exprMap.clear(); // reset after jumps
            result.emit(instr);
            continue;
        }

        // For binary operations: check if we've seen this before
        if (instr.op >= TACOp::ADD && instr.op <= TACOp::GEQ) {
            string key = makeExprKey(instr.op, instr.arg1, instr.arg2);
            
            if (exprMap.count(key)) {
                // Replace with copy of previous result
                TACInstr copyInstr;
                copyInstr.op = TACOp::COPY;
                copyInstr.arg1 = exprMap[key];
                copyInstr.arg2 = "";
                copyInstr.result = instr.result;
                copyInstr.srcLine = instr.srcLine;
                result.emit(copyInstr);
            } else {
                // Fresh computation; track it
                exprMap[key] = instr.result;
                result.emit(instr);
            }
        } else {
            result.emit(instr);
        }
    }

    return result;
}

// ─────────────────────────────────────────────
//  Check if operation can be strength-reduced
// ─────────────────────────────────────────────
bool CodeOptimizer::canStrengthReduce(TACOp op, const string& /*arg1*/,
                                     const string& arg2) {
    // Multiplication by constant power of 2 can be reduced to shift
    // Division by constant power of 2 can be reduced to shift
    // We'll implement simple multiplication folding for now
    
    if (op == TACOp::MUL) {
        // x * 2 or x * 4 etc. can be reduced
        if (arg2 == "2" || arg2 == "4" || arg2 == "8") return true;
    }
    return false;
}

// ─────────────────────────────────────────────
//  Strength Reduction
//
//  Replace expensive operations with cheaper ones.
//  Example:  t0 = x * 2   →  t0 = x + x
// ─────────────────────────────────────────────
TACProgram CodeOptimizer::strengthReduction(const TACProgram& input) {
    TACProgram result;

    for (const auto& instr : input.instrs) {
        TACInstr newInstr = instr;

        // Reduce multiplication by powers of 2
        if (instr.op == TACOp::MUL && instr.arg2 == "2") {
            // x * 2  →  x + x
            newInstr.op = TACOp::ADD;
            newInstr.arg2 = instr.arg1; // both args are the same
        }
        else if (instr.op == TACOp::MUL && instr.arg2 == "4") {
            // x * 4  →  t0 = x + x; t1 = t0 + t0
            // For simplicity, we'll transform to x * 2 instead
            // (let constant folding handle it)
            newInstr.arg2 = "4";
        }

        result.emit(newInstr);
    }

    return result;
}
