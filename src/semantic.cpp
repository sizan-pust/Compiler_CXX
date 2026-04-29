#include "semantic.h"
#include <sstream>
using namespace std;
//  Constructor
SemanticAnalyzer::SemanticAnalyzer()
    : m_currentFuncReturnType("void"), m_loopDepth(0) {}


//  Public entry point
bool SemanticAnalyzer::analyze(ASTNode* root) {
    if (!root) return false;
    visitProgram(root);
    return !hasErrors();
}

//  Error helper
void SemanticAnalyzer::error(const string& msg, int line) {
    m_errors.push_back({ msg, line });
}

//  Type helpers
bool SemanticAnalyzer::isNumeric(const string& t) {
    return t == "int" || t == "float" || t == "double" || t == "char";
}

// Implicit widening rules (C-like):
//   char  → int → float → double
string SemanticAnalyzer::widenType(const string& a, const string& b) {
    if (a == "double" || b == "double") return "double";
    if (a == "float"  || b == "float")  return "float";
    if (a == "int"    || b == "int")    return "int";
    return "char";
}

// Target ← source: OK if same, or source narrows/widens to target numerically
bool SemanticAnalyzer::typesCompatible(const string& target, const string& source) {
    if (target == source)  return true;
    if (target == "unknown" || source == "unknown") return true; // suppress cascade
    // numeric ← numeric is allowed (implicit conversion, may warn but not error)
    if (isNumeric(target) && isNumeric(source)) return true;
    return false;
}

//  Type inference: return the type of an expression node
string SemanticAnalyzer::inferType(ASTNode* node) {
    if (node->sval == "print") {
    for (auto& arg : node->children) {
        inferType(arg.get());
    }
    return "int";
}
    if (!node) return "unknown";

    switch (node->kind) {

        case NodeType::LITERAL_INT:    return "int";
        case NodeType::LITERAL_FLOAT:  return "float";
        case NodeType::LITERAL_CHAR:   return "char";
        case NodeType::LITERAL_STRING: return "char*";

        case NodeType::IDENTIFIER: {
            Symbol* sym = m_symTable.lookup(node->sval);
            if (!sym) {
                error("Undeclared identifier '" + node->sval + "'", node->line);
                return "unknown";
            }
            return sym->type;
        }

        case NodeType::BINARY_OP: {
            if (node->children.size() < 2) return "unknown";
            string lt = inferType(node->children[0].get());
            string rt = inferType(node->children[1].get());
            // Relational / equality / logical → always int (boolean)
            string op = node->sval;
            if (op == "==" || op == "!=" ||
                op == "<"  || op == ">"  ||
                op == "<=" || op == ">=" ||
                op == "&&" || op == "||")
                return "int";
            // Arithmetic → widened type
            return widenType(lt, rt);
        }

        case NodeType::UNARY_OP: {
            if (node->children.empty()) return "unknown";
            string inner = inferType(node->children[0].get());
            // ! operator → int (boolean)
            if (node->sval.find('!') != string::npos) return "int";
            return inner;
        }

        case NodeType::FUNC_CALL: {
            Symbol* sym = m_symTable.lookup(node->sval);
            if (!sym) {
                error("Undeclared function '" + node->sval + "'", node->line);
                return "unknown";
            }
            if (sym->kind != SymbolKind::FUNCTION) {
                error("'" + node->sval + "' is not a function", node->line);
                return "unknown";
            }
            // Check argument count
            int expected = (int)sym->paramTypes.size();
            int got      = (int)node->children.size();
            if (expected != got) {
                error("Function '" + node->sval + "' expects " +
                      to_string(expected) + " argument(s), got " +
                      to_string(got), node->line);
            }
            return sym->returnType;
        }

        case NodeType::ASSIGN: {
            if (node->children.size() < 2) return "unknown";
            return inferType(node->children[0].get());
        }

        default:
            return "unknown";
    }
}


//  Visitor dispatcher
void SemanticAnalyzer::visitNode(ASTNode* node) {
    if (!node) return;
    switch (node->kind) {
        case NodeType::PROGRAM:       visitProgram(node);        break;
        case NodeType::FUNCTION_DECL: visitFunctionDecl(node);   break;
        case NodeType::BLOCK:         visitBlock(node);           break;
        case NodeType::VAR_DECL:      visitVarDecl(node);         break;
        case NodeType::ASSIGN:        visitAssign(node);          break;
        case NodeType::IF_STMT:       visitIfStmt(node);          break;
        case NodeType::WHILE_STMT:    visitWhileStmt(node);       break;
        case NodeType::FOR_STMT:      visitForStmt(node);         break;
        case NodeType::RETURN_STMT:   visitReturnStmt(node);      break;
        case NodeType::BREAK_STMT:
        case NodeType::CONTINUE_STMT: visitBreakContinue(node);   break;
        case NodeType::EXPR_STMT:     visitExprStmt(node);        break;
        default:
            // Expression nodes: just type-infer to trigger checks
            inferType(node);
            break;
    }
}


//  Program: visit all top-level declarations
void SemanticAnalyzer::visitProgram(ASTNode* node) {
    for (auto& child : node->children)
        visitNode(child.get());
}

//  Function declaration
//  1. Register function in global scope
//  2. Open new scope for body
//  3. Declare all parameters
//  4. Visit body

void SemanticAnalyzer::visitFunctionDecl(ASTNode* node) {
    // node->sval = "returnType funcName"  e.g. "int main"
    // Parse return type and name from sval
    string retType, funcName;
    size_t spacePos = node->sval.rfind(' ');
    if (spacePos == string::npos) {
        funcName = node->sval;
        retType  = "int";
    } else {
        retType  = node->sval.substr(0, spacePos);
        funcName = node->sval.substr(spacePos + 1);
    }

    // Collect parameter types (children that are PARAM nodes)
    vector<string> paramTypes;
    ASTNode*       bodyBlock = nullptr;

    for (auto& child : node->children) {
        if (child->kind == NodeType::PARAM) {
            // param sval = "type name"
            size_t sp = child->sval.find(' ');
            if (sp != string::npos)
                paramTypes.push_back(child->sval.substr(0, sp));
        } else if (child->kind == NodeType::BLOCK) {
            bodyBlock = child.get();
        }
    }

    // Register function in current (global) scope
    Symbol funcSym;
    funcSym.name        = funcName;
    funcSym.type        = retType;
    funcSym.kind        = SymbolKind::FUNCTION;
    funcSym.scopeLevel  = m_symTable.currentScopeLevel();
    funcSym.declLine    = node->line;
    funcSym.paramTypes  = paramTypes;
    funcSym.returnType  = retType;

    if (!m_symTable.declare(funcSym)) {
        error("Redeclaration of function '" + funcName + "'", node->line);
    }

    // Enter function scope
    m_symTable.enterScope();
    string prevReturnType    = m_currentFuncReturnType;
    m_currentFuncReturnType  = retType;

    // Declare parameters inside function scope
    for (auto& child : node->children) {
        if (child->kind == NodeType::PARAM) {
            size_t sp    = child->sval.find(' ');
            string pType = (sp != string::npos) ? child->sval.substr(0, sp)       : "int";
            string pName = (sp != string::npos) ? child->sval.substr(sp + 1)      : child->sval;

            Symbol paramSym;
            paramSym.name       = pName;
            paramSym.type       = pType;
            paramSym.kind       = SymbolKind::PARAMETER;
            paramSym.scopeLevel = m_symTable.currentScopeLevel();
            paramSym.declLine   = child->line;

            if (!m_symTable.declare(paramSym)) {
                error("Duplicate parameter name '" + pName + "'", child->line);
            }
        }
    }

    // Visit function body
    if (bodyBlock) visitBlock(bodyBlock);

    // Restore context
    m_currentFuncReturnType = prevReturnType;
    m_symTable.exitScope();
}

//  Block: open scope, visit statements, close
void SemanticAnalyzer::visitBlock(ASTNode* node) {
    m_symTable.enterScope();
    for (auto& child : node->children)
        visitNode(child.get());
    m_symTable.exitScope();
}
//  Variable declaration
//  node->sval = "type name"
void SemanticAnalyzer::visitVarDecl(ASTNode* node) {
    size_t sp    = node->sval.find(' ');
    string vType = (sp != string::npos) ? node->sval.substr(0, sp)  : "int";
    string vName = (sp != string::npos) ? node->sval.substr(sp + 1) : node->sval;

    // Check redeclaration in same scope
    if (m_symTable.lookupCurrentScope(vName)) {
        error("Redeclaration of variable '" + vName + "' in same scope", node->line);
        return;
    }

    // If there's an initializer, check type compatibility
    if (!node->children.empty()) {
        string initType = inferType(node->children[0].get());
        if (!typesCompatible(vType, initType)) {
            error("Type mismatch: cannot assign '" + initType +
                  "' to variable '" + vName + "' of type '" + vType + "'",
                  node->line);
        }
    }

    Symbol sym;
    sym.name       = vName;
    sym.type       = vType;
    sym.kind       = SymbolKind::VARIABLE;
    sym.scopeLevel = m_symTable.currentScopeLevel();
    sym.declLine   = node->line;

    m_symTable.declare(sym);
}
//  Assignment statement
//  node->sval = "=" | "+=" | "-=" etc.
//  children[0] = lhs identifier
//  children[1] = rhs expression
void SemanticAnalyzer::visitAssign(ASTNode* node) {
    if (node->children.size() < 2) return;

    ASTNode* lhs = node->children[0].get();
    ASTNode* rhs = node->children[1].get();

    // lhs must be a declared variable
    if (lhs->kind == NodeType::IDENTIFIER) {
        Symbol* sym = m_symTable.lookup(lhs->sval);
        if (!sym) {
            error("Assignment to undeclared variable '" + lhs->sval + "'", node->line);
            return;
        }
        string rhsType = inferType(rhs);
        if (!typesCompatible(sym->type, rhsType)) {
            error("Type mismatch in assignment: cannot assign '" + rhsType +
                  "' to '" + lhs->sval + "' (type '" + sym->type + "')", node->line);
        }
    }

    // Visit both sides for nested checks
    inferType(rhs);
}

//  If statement
//  children[0] = condition
//  children[1] = then-block
//  children[2] = else-block (optional)
void SemanticAnalyzer::visitIfStmt(ASTNode* node) {
    if (node->children.empty()) return;

    // Condition must be numeric/boolean
    string condType = inferType(node->children[0].get());
    if (condType != "int" && condType != "float" && condType != "unknown") {
        error("If condition must be a numeric expression, got '" + condType + "'",
              node->line);
    }

    // Then block
    if (node->children.size() > 1)
        visitNode(node->children[1].get());

    // Else block (optional)
    if (node->children.size() > 2)
        visitNode(node->children[2].get());
}


//  While statement
//  children[0] = condition
//  children[1] = body block
void SemanticAnalyzer::visitWhileStmt(ASTNode* node) {
    if (node->children.empty()) return;

    string condType = inferType(node->children[0].get());
    if (condType != "int" && condType != "float" && condType != "unknown") {
        error("While condition must be a numeric expression, got '" + condType + "'",
              node->line);
    }

    m_loopDepth++;
    if (node->children.size() > 1)
        visitNode(node->children[1].get());
    m_loopDepth--;
}
//  For statement
//  children[0] = init
//  children[1] = condition
//  children[2] = increment
//  children[3] = body block
void SemanticAnalyzer::visitForStmt(ASTNode* node) {
    m_symTable.enterScope(); // for-init variables scoped to the loop

    size_t idx = 0;
    if (node->children.size() > idx) visitNode(node->children[idx++].get()); // init
    if (node->children.size() > idx) inferType(node->children[idx++].get()); // cond
    if (node->children.size() > idx) inferType(node->children[idx++].get()); // incr

    m_loopDepth++;
    if (node->children.size() > idx)
        visitNode(node->children[idx].get()); // body
    m_loopDepth--;

    m_symTable.exitScope();
}
//  Return statement
void SemanticAnalyzer::visitReturnStmt(ASTNode* node) {
    string retType = "void";

    if (!node->children.empty())
        retType = inferType(node->children[0].get());

    if (!typesCompatible(m_currentFuncReturnType, retType)) {
        error("Return type mismatch: function returns '" +
              m_currentFuncReturnType + "' but got '" + retType + "'",
              node->line);
    }
}


//  Break / Continue — only valid inside a loop
void SemanticAnalyzer::visitBreakContinue(ASTNode* node) {
    if (m_loopDepth == 0) {
        string kw = (node->kind == NodeType::BREAK_STMT) ? "break" : "continue";
        error("'" + kw + "' used outside of a loop", node->line);
    }
}

//  Expression statement — just infer/check type
void SemanticAnalyzer::visitExprStmt(ASTNode* node) {
    if (!node->children.empty())
        inferType(node->children[0].get());
}
