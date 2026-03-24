#include "codegen.h"
#include <sstream>
#include <cctype>
#include <algorithm>
using namespace std;

// ─────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────
CodeGenerator::CodeGenerator()
    : m_stackOffset(0) {}

// ─────────────────────────────────────────────
//  Emit a single assembly line
// ─────────────────────────────────────────────
void CodeGenerator::emitLine(const string& code) {
    m_asmCode.push_back(code);
}

// ─────────────────────────────────────────────
//  Error reporting
// ─────────────────────────────────────────────
void CodeGenerator::emitError(const string& msg, int line) {
    m_errors.push_back({ msg, line });
}

// ─────────────────────────────────────────────
//  Check if string is a register name
// ─────────────────────────────────────────────
bool CodeGenerator::isRegister(const string& s) {
    return s == "rax" || s == "rbx" || s == "rcx" || s == "rdx" ||
           s == "rsi" || s == "rdi" || s == "rbp" || s == "rsp" ||
           s == "r8" || s == "r9" || s == "r10" || s == "r11" ||
           s == "r12" || s == "r13" || s == "r14" || s == "r15";
}

// ─────────────────────────────────────────────
//  Check if string is a literal constant
// ─────────────────────────────────────────────
bool CodeGenerator::isLiteral(const string& s) {
    if (s.empty()) return false;
    for (size_t i = 0; i < s.length(); ++i) {
        char c = s[i];
        if (i == 0 && c == '-') continue;
        if (!isdigit(c) && c != '.') return false;
    }
    return true;
}

// ─────────────────────────────────────────────
//  Allocate a register for a variable
// ─────────────────────────────────────────────
string CodeGenerator::allocateRegister(const string& var) {
    // Check if already allocated
    if (m_regCtx.varToReg.count(var)) {
        return m_regCtx.varToReg[var];
    }

    // Find a free register
    for (const auto& reg : m_generalRegs) {
        if (m_regCtx.usedRegs.find(reg) == m_regCtx.usedRegs.end()) {
            m_regCtx.varToReg[var] = reg;
            m_regCtx.usedRegs.insert(reg);
            return reg;
        }
    }

    // Fallback: use rax and spill to stack
    m_regCtx.varToReg[var] = "rax";
    m_regCtx.usedRegs.insert("rax");
    return "rax";
}

// ─────────────────────────────────────────────
//  Get register for a variable or allocate it
// ─────────────────────────────────────────────
string CodeGenerator::getRegisterFor(const string& var) {
    if (isRegister(var)) return var;
    if (isLiteral(var)) {
        // Load literal into rax
        emitLine("    mov rax, " + var);
        return "rax";
    }
    return allocateRegister(var);
}

// ─────────────────────────────────────────────
//  Deallocate a register
// ─────────────────────────────────────────────
void CodeGenerator::deallocateRegister(const string& reg) {
    m_regCtx.usedRegs.erase(reg);
    // Also remove from varToReg mappings
    for (auto it = m_regCtx.varToReg.begin(); it != m_regCtx.varToReg.end(); ) {
        if (it->second == reg) {
            it = m_regCtx.varToReg.erase(it);
        } else {
            ++it;
        }
    }
}

// ─────────────────────────────────────────────
//  Free all registers (function boundary)
// ─────────────────────────────────────────────
void CodeGenerator::freeAllRegisters() {
    m_regCtx.varToReg.clear();
    m_regCtx.usedRegs.clear();
}

// ─────────────────────────────────────────────
//  Emit a label definition
// ─────────────────────────────────────────────
void CodeGenerator::emitLabel(const string& label) {
    emitLine(label + ":");
}

// ─────────────────────────────────────────────
//  Get effective address for a variable
//  (on stack or in register)
// ─────────────────────────────────────────────
string CodeGenerator::getEffectiveAddress(const string& var) {
    if (m_regCtx.varToReg.count(var)) {
        return m_regCtx.varToReg[var];
    }
    // Otherwise, assume it's on stack (simplified)
    return "[rbp - " + to_string(m_stackOffset) + "]";
}

// ─────────────────────────────────────────────
//  Main code generation entry point
// ─────────────────────────────────────────────
bool CodeGenerator::generate(const TACProgram& tacProg) {
    emitLine("; Generated x86-64 assembly from TAC");
    emitLine("    section .text");
    emitLine("    global main");
    emitLine("");

    genProgram(tacProg);

    return !hasErrors();
}

// ─────────────────────────────────────────────
//  Generate code for entire program
// ─────────────────────────────────────────────
void CodeGenerator::genProgram(const TACProgram& prog) {
    for (const auto& instr : prog.instrs) {
        if (instr.op == TACOp::FUNC_BEGIN) {
            genFuncBegin(instr.arg1);
        } else if (instr.op == TACOp::FUNC_END) {
            genFuncEnd(instr.arg1);
        } else {
            genInstruction(instr);
        }
    }
}

// ─────────────────────────────────────────────
//  Generate function prologue
// ─────────────────────────────────────────────
void CodeGenerator::genFuncBegin(const string& funcName) {
    freeAllRegisters();
    m_stackOffset = 0;

    emitLine(funcName + ":");
    emitLine("    push rbp                ; save old rbp");
    emitLine("    mov rbp, rsp            ; set up new rbp");
    emitLine("    sub rsp, 32             ; allocate local space");
}

// ─────────────────────────────────────────────
//  Generate function epilogue
// ─────────────────────────────────────────────
void CodeGenerator::genFuncEnd(const string& /*funcName*/) {
    emitLine("    leave                   ; restore rsp and rbp");
    emitLine("    ret                     ; return from function");
    emitLine("");
}

// ─────────────────────────────────────────────
//  Dispatch instruction generation
// ─────────────────────────────────────────────
void CodeGenerator::genInstruction(const TACInstr& instr) {
    switch (instr.op) {
        // Arithmetic and logic operations
        case TACOp::ADD:
        case TACOp::SUB:
        case TACOp::MUL:
        case TACOp::DIV:
        case TACOp::MOD:
        case TACOp::AND:
        case TACOp::OR:
        case TACOp::EQ:
        case TACOp::NEQ:
        case TACOp::LT:
        case TACOp::GT:
        case TACOp::LEQ:
        case TACOp::GEQ:
            genBinaryOp(instr);
            break;

        // Unary operations
        case TACOp::NEG:
        case TACOp::NOT:
        case TACOp::INC:
        case TACOp::DEC:
            genUnaryOp(instr);
            break;

        // Data movement
        case TACOp::COPY:
            genCopy(instr);
            break;

        // Control flow
        case TACOp::LABEL:
            genLabel(instr);
            break;

        case TACOp::GOTO:
            genGoto(instr);
            break;

        case TACOp::IF_FALSE:
            genIfFalse(instr);
            break;

        case TACOp::PARAM:
        case TACOp::CALL:
            genParamAndCall(instr);
            break;

        case TACOp::RETURN:
            genReturn(instr);
            break;

        default:
            emitError("Unknown TAC operation", instr.srcLine);
            break;
    }
}

// ─────────────────────────────────────────────
//  Generate binary operation
//  result = arg1 op arg2
// ─────────────────────────────────────────────
void CodeGenerator::genBinaryOp(const TACInstr& instr) {
    string arg1Reg = getRegisterFor(instr.arg1);
    string arg2Reg = getRegisterFor(instr.arg2);
    string resultReg = allocateRegister(instr.result);

    // Move arg1 to result register
    if (arg1Reg != resultReg) {
        emitLine("    mov " + resultReg + ", " + arg1Reg);
    }

    // Perform operation
    string opStr;
    switch (instr.op) {
        case TACOp::ADD: opStr = "add"; break;
        case TACOp::SUB: opStr = "sub"; break;
        case TACOp::MUL: opStr = "imul"; break;
        case TACOp::DIV: opStr = "idiv"; break;
        case TACOp::MOD: opStr = "idiv"; break; // result in rdx
        case TACOp::AND: opStr = "and"; break;
        case TACOp::OR:  opStr = "or"; break;
        case TACOp::EQ:  opStr = "cmp"; break;  // comparison only
        case TACOp::NEQ: opStr = "cmp"; break;
        case TACOp::LT:  opStr = "cmp"; break;
        case TACOp::GT:  opStr = "cmp"; break;
        case TACOp::LEQ: opStr = "cmp"; break;
        case TACOp::GEQ: opStr = "cmp"; break;
        default: opStr = "mov"; break;
    }

    if (opStr == "cmp") {
        emitLine("    cmp " + resultReg + ", " + arg2Reg);
        emitLine("    sete al");  // set byte if equal
        emitLine("    movzx " + resultReg + ", al");  // zero extend
    } else if (opStr == "idiv") {
        emitLine("    cdq");  // sign extend rax to rdx:rax
        emitLine("    idiv " + arg2Reg);
        if (instr.op == TACOp::MOD) {
            emitLine("    mov " + resultReg + ", rdx");  // MOD result in rdx
        }
    } else {
        emitLine("    " + opStr + " " + resultReg + ", " + arg2Reg);
    }
}

// ─────────────────────────────────────────────
//  Generate unary operation
// ─────────────────────────────────────────────
void CodeGenerator::genUnaryOp(const TACInstr& instr) {
    string argReg = getRegisterFor(instr.arg1);
    string resultReg = allocateRegister(instr.result);

    if (argReg != resultReg) {
        emitLine("    mov " + resultReg + ", " + argReg);
    }

    switch (instr.op) {
        case TACOp::NEG:
            emitLine("    neg " + resultReg);
            break;
        case TACOp::NOT:
            emitLine("    not " + resultReg);
            break;
        case TACOp::INC:
            emitLine("    inc " + resultReg);
            break;
        case TACOp::DEC:
            emitLine("    dec " + resultReg);
            break;
        default:
            break;
    }
}

// ─────────────────────────────────────────────
//  Generate copy operation
//  result = arg1
// ─────────────────────────────────────────────
void CodeGenerator::genCopy(const TACInstr& instr) {
    string srcReg = getRegisterFor(instr.arg1);
    string dstReg = allocateRegister(instr.result);

    if (srcReg != dstReg) {
        emitLine("    mov " + dstReg + ", " + srcReg);
    }
}

// ─────────────────────────────────────────────
//  Generate label
// ─────────────────────────────────────────────
void CodeGenerator::genLabel(const TACInstr& instr) {
    emitLabel(instr.arg1);
}

// ─────────────────────────────────────────────
//  Generate unconditional jump
// ─────────────────────────────────────────────
void CodeGenerator::genGoto(const TACInstr& instr) {
    emitLine("    jmp " + instr.arg1);
}

// ─────────────────────────────────────────────
//  Generate conditional jump (if false)
//  if arg1 == 0 goto result
// ─────────────────────────────────────────────
void CodeGenerator::genIfFalse(const TACInstr& instr) {
    string testReg = getRegisterFor(instr.arg1);
    emitLine("    test " + testReg + ", " + testReg);
    emitLine("    jz " + instr.result);
}

// ─────────────────────────────────────────────
//  Generate parameter passing and function call
// ─────────────────────────────────────────────
void CodeGenerator::genParamAndCall(const TACInstr& instr) {
    if (instr.op == TACOp::PARAM) {
        // Push parameter on stack (simplified: just record it)
        emitLine("    ; param " + instr.arg1);
    } else if (instr.op == TACOp::CALL) {
        // Call function
        // arg1 = function name, arg2 = argc, result = return value
        emitLine("    call " + instr.arg1);
        emitLine("    ; return value in rax");
        
        if (!instr.result.empty()) {
            string resultReg = allocateRegister(instr.result);
            if (resultReg != "rax") {
                emitLine("    mov " + resultReg + ", rax");
            }
        }
    }
}

// ─────────────────────────────────────────────
//  Generate return statement
// ─────────────────────────────────────────────
void CodeGenerator::genReturn(const TACInstr& instr) {
    if (!instr.arg1.empty()) {
        // Return value
        string retReg = getRegisterFor(instr.arg1);
        if (retReg != "rax") {
            emitLine("    mov rax, " + retReg);
        }
    }
    emitLine("    jmp " + string(instr.arg1.empty() ? "_return" : "_return"));
}
