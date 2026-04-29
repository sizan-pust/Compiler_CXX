#include "icg.h"
#include <sstream>
using namespace std;

// ─────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────
ICGenerator::ICGenerator()
    : m_tempCount(0), m_labelCount(0) {}

// ─────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────
string ICGenerator::newTemp() {
    return "t" + to_string(m_tempCount++);
}

string ICGenerator::newLabel() {
    return "L" + to_string(m_labelCount++);
}

void ICGenerator::emitError(const string& msg, int line) {
    m_errors.push_back({ msg, line });
}

void ICGenerator::emit(TACOp op,
                       const string& arg1,
                       const string& arg2,
                       const string& result,
                       int           line) {
    m_prog.emit({ op, arg1, arg2, result, line });
}

// ─────────────────────────────────────────────
//  Public entry point
// ─────────────────────────────────────────────
bool ICGenerator::generate(ASTNode* root) {
    if (!root) return false;
    genProgram(root);
    return !hasErrors();
}

// ─────────────────────────────────────────────
//  Program — visit all top-level declarations
// ─────────────────────────────────────────────
void ICGenerator::genProgram(ASTNode* node) {
    bool hasFunction = false;

    for (auto& child : node->children) {
        if (child->kind == NodeType::FUNCTION_DECL) {
            hasFunction = true;
            genFuncDecl(child.get());
        }
    }

    // Mini-C mode: top-level statements
    if (!hasFunction) {
        emit(TACOp::FUNC_BEGIN, "main", "", "", node->line);

        for (auto& child : node->children) {
            genStmt(child.get());
        }

        emit(TACOp::RETURN, "0", "", "", node->line);
        emit(TACOp::FUNC_END, "main", "", "", node->line);
    }
}

// ─────────────────────────────────────────────
//  Function declaration
//  Emits: FUNC_BEGIN, params marked, body, FUNC_END
// ─────────────────────────────────────────────
void ICGenerator::genFuncDecl(ASTNode* node) {
    // Parse function name from sval ("int main" → "main")
    string funcName = node->sval;
    size_t sp = funcName.rfind(' ');
    if (sp != string::npos) funcName = funcName.substr(sp + 1);

    emit(TACOp::FUNC_BEGIN, funcName, "", "", node->line);

    // Declare each PARAM as a receive instruction (just a label comment)
    for (auto& child : node->children) {
        if (child->kind == NodeType::PARAM) {
            size_t s   = child->sval.rfind(' ');
            string pName = (s != string::npos) ? child->sval.substr(s + 1)
                                               : child->sval;
            // PARAM with result = param name signals "receive parameter"
            emit(TACOp::PARAM, pName, "", "", child->line);
        }
    }

    // Generate body block
    for (auto& child : node->children)
        if (child->kind == NodeType::BLOCK)
            genBlock(child.get());

    emit(TACOp::FUNC_END, funcName, "", "", node->line);
}

// ─────────────────────────────────────────────
//  Block — generate each statement
// ─────────────────────────────────────────────
void ICGenerator::genBlock(ASTNode* node) {
    for (auto& child : node->children)
        genStmt(child.get());
}

// ─────────────────────────────────────────────
//  Statement dispatcher
// ─────────────────────────────────────────────
void ICGenerator::genStmt(ASTNode* node) {
    if (!node) return;
    switch (node->kind) {
        case NodeType::VAR_DECL:      genVarDecl(node);    break;
        case NodeType::ASSIGN:        genAssign(node);     break;
        case NodeType::IF_STMT:       genIfStmt(node);     break;
        case NodeType::WHILE_STMT:    genWhileStmt(node);  break;
        case NodeType::FOR_STMT:      genForStmt(node);    break;
        case NodeType::RETURN_STMT:   genReturnStmt(node); break;
        case NodeType::EXPR_STMT:     genExprStmt(node);   break;
        case NodeType::BLOCK:         genBlock(node);      break;

        case NodeType::BREAK_STMT:
            if (!m_breakStack.empty())
                emit(TACOp::GOTO, m_breakStack.back(), "", "", node->line);
            else
                emitError("break outside loop", node->line);
            break;

        case NodeType::CONTINUE_STMT:
            if (!m_continueStack.empty())
                emit(TACOp::GOTO, m_continueStack.back(), "", "", node->line);
            else
                emitError("continue outside loop", node->line);
            break;

        default:
            genExpr(node); // expression used as statement
            break;
    }
}

// ─────────────────────────────────────────────
//  Variable declaration:  type name = expr
//  TAC: evaluate init expr, COPY into variable
// ─────────────────────────────────────────────
void ICGenerator::genVarDecl(ASTNode* node) {
    // Extract variable name from sval ("int x" → "x")
    string varName = node->sval;
    size_t sp = varName.rfind(' ');
    if (sp != string::npos) varName = varName.substr(sp + 1);

    if (!node->children.empty()) {
        string src = genExpr(node->children[0].get());
        emit(TACOp::COPY, src, "", varName, node->line);
    }
    // If no initializer, no TAC needed (variable just exists)
}

// ─────────────────────────────────────────────
//  Assignment:  x = expr  /  x += expr  etc.
//  children[0] = identifier (lhs)
//  children[1] = rhs expression
// ─────────────────────────────────────────────
void ICGenerator::genAssign(ASTNode* node) {
    if (node->children.size() < 2) return;

    string lhsName = node->children[0]->sval;
    string op      = node->sval; // "=", "+=", "-=", "*=", "/="

    string rhs = genExpr(node->children[1].get());

    if (op == "=") {
        emit(TACOp::COPY, rhs, "", lhsName, node->line);
    } else {
        // Compound assignment: x += e  →  t = x + e; x = t
        TACOp binOp;
        if      (op == "+=") binOp = TACOp::ADD;
        else if (op == "-=") binOp = TACOp::SUB;
        else if (op == "*=") binOp = TACOp::MUL;
        else if (op == "/=") binOp = TACOp::DIV;
        else                 binOp = TACOp::ADD; // fallback

        string tmp = newTemp();
        emit(binOp, lhsName, rhs, tmp, node->line);
        emit(TACOp::COPY, tmp, "", lhsName, node->line);
    }
}

// ─────────────────────────────────────────────
//  If statement
//  children[0] = condition
//  children[1] = then-block
//  children[2] = else-block (optional)
//
//  TAC pattern:
//    t0 = <cond>
//    if t0 == 0 goto L_else
//    <then>
//    goto L_end
//    L_else:
//    <else>           (if present)
//    L_end:
// ─────────────────────────────────────────────
void ICGenerator::genIfStmt(ASTNode* node) {
    if (node->children.empty()) return;

    string lElse = newLabel();
    string lEnd  = newLabel();

    // Evaluate condition
    string condTemp = genExpr(node->children[0].get());
    emit(TACOp::IF_FALSE, condTemp, "", lElse, node->line);

    // Then-block
    if (node->children.size() > 1)
        genStmt(node->children[1].get());

    emit(TACOp::GOTO, lEnd, "", "", node->line);

    // Else label
    emit(TACOp::LABEL, lElse, "", "", node->line);

    // Else-block (optional)
    if (node->children.size() > 2)
        genStmt(node->children[2].get());

    // End label
    emit(TACOp::LABEL, lEnd, "", "", node->line);
}

// ─────────────────────────────────────────────
//  While statement
//
//  TAC pattern:
//    L_start:
//    t0 = <cond>
//    if t0 == 0 goto L_end
//    <body>
//    goto L_start
//    L_end:
// ─────────────────────────────────────────────
void ICGenerator::genWhileStmt(ASTNode* node) {
    if (node->children.empty()) return;

    string lStart = newLabel();
    string lEnd   = newLabel();

    m_breakStack.push_back(lEnd);
    m_continueStack.push_back(lStart);

    emit(TACOp::LABEL, lStart, "", "", node->line);

    string condTemp = genExpr(node->children[0].get());
    emit(TACOp::IF_FALSE, condTemp, "", lEnd, node->line);

    if (node->children.size() > 1)
        genStmt(node->children[1].get());

    emit(TACOp::GOTO, lStart, "", "", node->line);
    emit(TACOp::LABEL, lEnd, "", "", node->line);

    m_breakStack.pop_back();
    m_continueStack.pop_back();
}

// ─────────────────────────────────────────────
//  For statement
//
//  TAC pattern:
//    <init>
//    L_start:
//    t0 = <cond>
//    if t0 == 0 goto L_end
//    <body>
//    L_incr:
//    <incr>
//    goto L_start
//    L_end:
// ─────────────────────────────────────────────
void ICGenerator::genForStmt(ASTNode* node) {
    // children: [init, cond, incr, body]
    if (node->children.size() < 4) return;

    string lStart = newLabel();
    string lIncr  = newLabel();
    string lEnd   = newLabel();

    m_breakStack.push_back(lEnd);
    m_continueStack.push_back(lIncr);

    // Init
    genStmt(node->children[0].get());

    // Start label + condition
    emit(TACOp::LABEL, lStart, "", "", node->line);
    string condTemp = genExpr(node->children[1].get());
    emit(TACOp::IF_FALSE, condTemp, "", lEnd, node->line);

    // Body
    genStmt(node->children[3].get());

    // Increment label + increment expression
    emit(TACOp::LABEL, lIncr, "", "", node->line);
    genExpr(node->children[2].get());

    emit(TACOp::GOTO, lStart, "", "", node->line);
    emit(TACOp::LABEL, lEnd, "", "", node->line);

    m_breakStack.pop_back();
    m_continueStack.pop_back();
}

// ─────────────────────────────────────────────
//  Return statement
// ─────────────────────────────────────────────
void ICGenerator::genReturnStmt(ASTNode* node) {
    if (!node->children.empty()) {
        string val = genExpr(node->children[0].get());
        emit(TACOp::RETURN, val, "", "", node->line);
    } else {
        emit(TACOp::RETURN, "", "", "", node->line);
    }
}

// ─────────────────────────────────────────────
//  Expression statement (expr used as stmt)
// ─────────────────────────────────────────────
void ICGenerator::genExprStmt(ASTNode* node) {
    if (!node->children.empty())
        genExpr(node->children[0].get());
}

// ─────────────────────────────────────────────
//  Expression generation
//  Returns the name of the variable/temp that
//  holds the result.
// ─────────────────────────────────────────────
string ICGenerator::genExpr(ASTNode* node) {
    if (!node) return "";

    switch (node->kind) {

        // Literals — return their text directly (no temp needed)
        case NodeType::LITERAL_INT:
        case NodeType::LITERAL_FLOAT:
        case NodeType::LITERAL_CHAR:
        case NodeType::LITERAL_STRING:
            return node->sval;

        // Identifier — return variable name directly
        case NodeType::IDENTIFIER:
            return node->sval;

        // Binary operation
        case NodeType::BINARY_OP:
            return genBinaryOp(node);

        // Unary operation
        case NodeType::UNARY_OP:
            return genUnaryOp(node);

        // Function call
        case NodeType::FUNC_CALL:
            return genFuncCall(node);

        // Assignment used as expression  (x = 5 inside an expr)
        case NodeType::ASSIGN:
            genAssign(node);
            return node->children[0]->sval; // value is lhs after assignment

        default:
            return "";
    }
}

// ─────────────────────────────────────────────
//  Binary operation
//  t_result = arg1 op arg2
// ─────────────────────────────────────────────
string ICGenerator::genBinaryOp(ASTNode* node) {
    if (node->children.size() < 2) return "";

    string left  = genExpr(node->children[0].get());
    string right = genExpr(node->children[1].get());
    string tmp   = newTemp();

    string op = node->sval;
    TACOp  tacOp;

    if      (op == "+")  tacOp = TACOp::ADD;
    else if (op == "-")  tacOp = TACOp::SUB;
    else if (op == "*")  tacOp = TACOp::MUL;
    else if (op == "/")  tacOp = TACOp::DIV;
    else if (op == "%")  tacOp = TACOp::MOD;
    else if (op == "&&") tacOp = TACOp::AND;
    else if (op == "||") tacOp = TACOp::OR;
    else if (op == "==") tacOp = TACOp::EQ;
    else if (op == "!=") tacOp = TACOp::NEQ;
    else if (op == "<")  tacOp = TACOp::LT;
    else if (op == ">")  tacOp = TACOp::GT;
    else if (op == "<=") tacOp = TACOp::LEQ;
    else if (op == ">=") tacOp = TACOp::GEQ;
    else {
        emitError("Unknown binary operator '" + op + "'", node->line);
        return tmp;
    }

    emit(tacOp, left, right, tmp, node->line);
    return tmp;
}

// ─────────────────────────────────────────────
//  Unary operation
//  sval format: "pre:--" / "post:++" / "pre:-" etc.
// ─────────────────────────────────────────────
string ICGenerator::genUnaryOp(ASTNode* node) {
    if (node->children.empty()) return "";

    string operand = genExpr(node->children[0].get());
    string sval    = node->sval;
    string tmp     = newTemp();

    // bool isPost = (sval.rfind("post:", 0) == 0);
    bool isPre  = (sval.rfind("pre:",  0) == 0);
    string op   = sval.substr(sval.find(':') + 1);

    if (op == "-") {
        // Negate
        emit(TACOp::NEG, operand, "", tmp, node->line);
        return tmp;
    }

    if (op == "!") {
        emit(TACOp::NOT, operand, "", tmp, node->line);
        return tmp;
    }

    if (op == "++") {
        if (isPre) {
            // ++x  → x = x + 1; result = x
            emit(TACOp::INC, operand, "", operand, node->line);
            return operand;
        } else {
            // x++  → tmp = x; x = x + 1; result = tmp
            emit(TACOp::COPY, operand, "", tmp, node->line);
            emit(TACOp::INC,  operand, "", operand, node->line);
            return tmp;
        }
    }

    if (op == "--") {
        if (isPre) {
            emit(TACOp::DEC, operand, "", operand, node->line);
            return operand;
        } else {
            emit(TACOp::COPY, operand, "", tmp, node->line);
            emit(TACOp::DEC,  operand, "", operand, node->line);
            return tmp;
        }
    }

    emitError("Unknown unary operator '" + op + "'", node->line);
    return tmp;
}

// ─────────────────────────────────────────────
//  Function call
//  TAC pattern:
//    param arg1
//    param arg2
//    ...
//    t0 = call funcName, argc
// ─────────────────────────────────────────────
string ICGenerator::genFuncCall(ASTNode* node) {
    string funcName = node->sval;
    int    argc     = (int)node->children.size();

    // Evaluate each argument and emit PARAM
    for (auto& arg : node->children) {
        string argVal = genExpr(arg.get());
        emit(TACOp::PARAM, argVal, "", "", node->line);
    }

    // Emit CALL — result goes into a fresh temp
    string tmp = newTemp();
    emit(TACOp::CALL, funcName, to_string(argc), tmp, node->line);
    return tmp;
}
