#pragma once
#include <string>
#include <vector>
#include <iostream>
using namespace std;

// ─────────────────────────────────────────────
//  Every kind of TAC instruction
// ─────────────────────────────────────────────
enum class TACOp {
    // Arithmetic / logic:  result = arg1 op arg2
    ADD, SUB, MUL, DIV, MOD,
    AND, OR,
    EQ, NEQ, LT, GT, LEQ, GEQ,

    // Unary:   result = op arg1
    NEG,        // result = -arg1
    NOT,        // result = !arg1
    INC,        // result = arg1 + 1   (++ sugar)
    DEC,        // result = arg1 - 1   (-- sugar)

    // Copy:    result = arg1
    COPY,

    // Control flow
    LABEL,      // arg1:                (label declaration)
    GOTO,       // goto arg1
    IF_FALSE,   // if arg1 == 0 goto result
    IF_TRUE,    // if arg1 != 0 goto result

    // Function
    FUNC_BEGIN, // begin of function  (arg1 = name)
    FUNC_END,   // end of function    (arg1 = name)
    PARAM,      // push param arg1
    CALL,       // result = call arg1, arg2 (arg2 = argc)
    RETURN,     // return arg1  (arg1 may be empty for void)
};

// ─────────────────────────────────────────────
//  A single TAC instruction
//
//  Quadruple form:  (op, arg1, arg2, result)
//  Not every field is used by every op — unused
//  fields are left as empty string "".
//
//  Examples:
//    ADD      arg1="a"  arg2="2"  result="t0"   → t0 = a + 2
//    COPY     arg1="t0" arg2=""   result="x"    → x = t0
//    LABEL    arg1="L0" arg2=""   result=""      → L0:
//    GOTO     arg1="L1" arg2=""   result=""      → goto L1
//    IF_FALSE arg1="t1" arg2=""   result="L2"   → if t1==0 goto L2
//    PARAM    arg1="x"  arg2=""   result=""      → param x
//    CALL     arg1="add" arg2="2" result="t3"   → t3 = call add, 2
//    RETURN   arg1="t3" arg2=""   result=""      → return t3
// ─────────────────────────────────────────────
struct TACInstr {
    TACOp  op;
    string arg1;
    string arg2;
    string result;
    int    srcLine;   // source line (for error messages in later phases)

    // Human-readable op name
    static string opName(TACOp o) {
        switch (o) {
            case TACOp::ADD:        return "ADD";
            case TACOp::SUB:        return "SUB";
            case TACOp::MUL:        return "MUL";
            case TACOp::DIV:        return "DIV";
            case TACOp::MOD:        return "MOD";
            case TACOp::AND:        return "AND";
            case TACOp::OR:         return "OR";
            case TACOp::EQ:         return "EQ";
            case TACOp::NEQ:        return "NEQ";
            case TACOp::LT:         return "LT";
            case TACOp::GT:         return "GT";
            case TACOp::LEQ:        return "LEQ";
            case TACOp::GEQ:        return "GEQ";
            case TACOp::NEG:        return "NEG";
            case TACOp::NOT:        return "NOT";
            case TACOp::INC:        return "INC";
            case TACOp::DEC:        return "DEC";
            case TACOp::COPY:       return "COPY";
            case TACOp::LABEL:      return "LABEL";
            case TACOp::GOTO:       return "GOTO";
            case TACOp::IF_FALSE:   return "IF_FALSE";
            case TACOp::IF_TRUE:    return "IF_TRUE";
            case TACOp::FUNC_BEGIN: return "FUNC_BEGIN";
            case TACOp::FUNC_END:   return "FUNC_END";
            case TACOp::PARAM:      return "PARAM";
            case TACOp::CALL:       return "CALL";
            case TACOp::RETURN:     return "RETURN";
            default:                return "UNKNOWN";
        }
    }

    // Pretty-print one instruction as human-readable text
    string toText() const {
        switch (op) {
            case TACOp::LABEL:
                return arg1 + ":";
            case TACOp::GOTO:
                return "    goto " + arg1;
            case TACOp::IF_FALSE:
                return "    if " + arg1 + " == 0 goto " + result;
            case TACOp::IF_TRUE:
                return "    if " + arg1 + " != 0 goto " + result;
            case TACOp::PARAM:
                return "    param " + arg1;
            case TACOp::CALL:
                return "    " + result + " = call " + arg1 + ", " + arg2;
            case TACOp::RETURN:
                return "    return" + (arg1.empty() ? "" : " " + arg1);
            case TACOp::FUNC_BEGIN:
                return "\nfunc " + arg1 + ":";
            case TACOp::FUNC_END:
                return "endfunc " + arg1 + "\n";
            case TACOp::COPY:
                return "    " + result + " = " + arg1;
            case TACOp::NEG:
                return "    " + result + " = -" + arg1;
            case TACOp::NOT:
                return "    " + result + " = !" + arg1;
            case TACOp::INC:
                return "    " + result + " = " + arg1 + " + 1";
            case TACOp::DEC:
                return "    " + result + " = " + arg1 + " - 1";
            default: {
                // Binary: result = arg1 op arg2
                string opStr;
                switch (op) {
                    case TACOp::ADD: opStr = "+";  break;
                    case TACOp::SUB: opStr = "-";  break;
                    case TACOp::MUL: opStr = "*";  break;
                    case TACOp::DIV: opStr = "/";  break;
                    case TACOp::MOD: opStr = "%";  break;
                    case TACOp::AND: opStr = "&&"; break;
                    case TACOp::OR:  opStr = "||"; break;
                    case TACOp::EQ:  opStr = "=="; break;
                    case TACOp::NEQ: opStr = "!="; break;
                    case TACOp::LT:  opStr = "<";  break;
                    case TACOp::GT:  opStr = ">";  break;
                    case TACOp::LEQ: opStr = "<="; break;
                    case TACOp::GEQ: opStr = ">="; break;
                    default:         opStr = "?";  break;
                }
                return "    " + result + " = " + arg1 + " " + opStr + " " + arg2;
            }
        }
    }
};

// ─────────────────────────────────────────────
//  The full TAC program — a flat list of instrs
// ─────────────────────────────────────────────
struct TACProgram {
    vector<TACInstr> instrs;

    void emit(TACInstr instr) {
        instrs.push_back(move(instr));
    }

    // Print as human-readable text to stdout
    void printText(ostream& out) const {
        for (auto& instr : instrs)
            out << instr.toText() << "\n";
    }

    // Print as JSON array to stdout
    void printJSON(ostream& out) const {
        out << "[\n";
        for (size_t i = 0; i < instrs.size(); ++i) {
            const TACInstr& ins = instrs[i];
            out << "  {\n";
            out << "    \"op\": \""     << TACInstr::opName(ins.op) << "\",\n";
            out << "    \"arg1\": \""   << escapeJSON(ins.arg1)     << "\",\n";
            out << "    \"arg2\": \""   << escapeJSON(ins.arg2)     << "\",\n";
            out << "    \"result\": \"" << escapeJSON(ins.result)   << "\",\n";
            out << "    \"line\": "     << ins.srcLine              << "\n";
            out << "  }";
            if (i + 1 < instrs.size()) out << ",";
            out << "\n";
        }
        out << "]\n";
    }

private:
    static string escapeJSON(const string& s) {
        string out;
        for (char c : s) {
            if (c == '"')  out += "\\\"";
            else if (c == '\\') out += "\\\\";
            else out += c;
        }
        return out;
    }
};
