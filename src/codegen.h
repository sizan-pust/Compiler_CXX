#pragma once
#include "tac.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
using namespace std;

// ─────────────────────────────────────────────
//  A code generation error message
// ─────────────────────────────────────────────
struct CodeGenError {
    string message;
    int    line;
};

// ─────────────────────────────────────────────
//  CodeGenerator
//
//  Converts TAC (Three-Address Code) to
//  x86-64 assembly code (Intel syntax).
//
//  Features:
//    - Register allocation (simple linear scan)
//    - Stack frame management
//    - Function prologue/epilogue
//    - Label and jump resolution
//    - Call convention (System V AMD64 ABI)
//
//  Registers used:
//    - rax, rbx, rcx, rdx, rsi, rdi, r8-r15
//      (caller-saved and callee-saved)
//    - rsp (stack pointer)
//    - rbp (base pointer)
//
//  Stack layout (per function):
//    [rbp + 16]  param 2
//    [rbp + 8]   param 1
//    [rbp + 0]   return address
//    [rbp - 8]   saved rbp
//    [rbp - ...] local variables
// ─────────────────────────────────────────────
class CodeGenerator {
public:
    CodeGenerator();

    // Generate assembly from TAC program
    bool generate(const TACProgram& tacProg);

    // Get generated assembly code
    const vector<string>& assembly() const { return m_asmCode; }
    const vector<CodeGenError>& errors()    const { return m_errors; }
    bool                        hasErrors() const { return !m_errors.empty(); }

private:
    vector<string>       m_asmCode;       // generated assembly instructions
    vector<CodeGenError> m_errors;

    // ── Register management ───────────────────
    struct RegContext {
        unordered_map<string, string> varToReg;  // temp/var → register
        set<string> usedRegs;                    // which regs are allocated
    };

    RegContext m_regCtx;
    int        m_stackOffset;  // current stack offset for locals

    // Available registers for allocation
    vector<string> m_generalRegs = {
        "rax", "rbx", "rcx", "rdx", "rsi", "rdi",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
    };

    // ── Assembly generation ───────────────────
    void emitLine(const string& code);
    void emitError(const string& msg, int line);

    string allocateRegister(const string& var);
    string getRegisterFor(const string& var);
    void   deallocateRegister(const string& reg);
    void   freeAllRegisters();

    void   emitLabel(const string& label);

    void genProgram(const TACProgram& prog);
    void genFuncBegin(const string& funcName);
    void genFuncEnd(const string& funcName);
    void genInstruction(const TACInstr& instr);

    // ── Instruction generation ────────────────
    void genBinaryOp(const TACInstr& instr);
    void genUnaryOp(const TACInstr& instr);
    void genCopy(const TACInstr& instr);
    void genLabel(const TACInstr& instr);
    void genGoto(const TACInstr& instr);
    void genIfFalse(const TACInstr& instr);
    void genParamAndCall(const TACInstr& instr);
    void genReturn(const TACInstr& instr);

    // ── Helper functions ──────────────────────
    bool isRegister(const string& s);
    bool isLiteral(const string& s);
    string getEffectiveAddress(const string& var);
};
