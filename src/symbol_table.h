#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <iostream>
using namespace std;

// ─────────────────────────────────────────────
//  What kind of symbol is this entry?
// ─────────────────────────────────────────────
enum class SymbolKind {
    VARIABLE,
    FUNCTION,
    PARAMETER
};

// ─────────────────────────────────────────────
//  A single entry in the symbol table
// ─────────────────────────────────────────────
struct Symbol {
    string     name;
    string     type;          // "int", "float", "char", "void", etc.
    SymbolKind kind;
    int        scopeLevel;    // 0 = global, 1 = function, 2+ = nested
    int        declLine;      // source line where declared

    // For functions only
    vector<string> paramTypes;  // ordered list of parameter types
    string         returnType;
};

// ─────────────────────────────────────────────
//  Scoped symbol table
//
//  Implemented as a stack of hash maps.
//  Each scope is one unordered_map.
//  enterScope() pushes a new map.
//  exitScope()  pops the top map.
//  lookup()     searches from top to bottom.
// ─────────────────────────────────────────────
class SymbolTable {
public:
    SymbolTable() {
        enterScope(); // global scope (level 0)
    }

    // ── Scope management ─────────────────────
    void enterScope() {
        m_scopes.push_back({});
    }

    void exitScope() {
        if (m_scopes.size() > 1)
            m_scopes.pop_back();
    }

    int currentScopeLevel() const {
        return (int)m_scopes.size() - 1;
    }

    // ── Declaration ───────────────────────────
    // Returns false if name already exists in current scope
   bool declare(const Symbol& sym) {
    auto& current = m_scopes.back();

    if (current.count(sym.name)) return false;

    current[sym.name] = sym;
    m_allSymbols.push_back(sym);   // keep for final display

    return true;
}

    // ── Lookup (all scopes, innermost first) ──
    // Returns nullptr if not found
    Symbol* lookup(const string& name) {
        for (int i = (int)m_scopes.size() - 1; i >= 0; --i) {
            auto it = m_scopes[i].find(name);
            if (it != m_scopes[i].end())
                return &it->second;
        }
        return nullptr;
    }

    // ── Lookup only in the current scope ──────
    Symbol* lookupCurrentScope(const string& name) {
        auto it = m_scopes.back().find(name);
        if (it != m_scopes.back().end())
            return &it->second;
        return nullptr;
    }

    // ── Dump symbol table as JSON (for UI) ────
  void printJSON(ostream& out) const {
    out << "[\n";

    for (size_t i = 0; i < m_allSymbols.size(); ++i) {
        const auto& sym = m_allSymbols[i];

        out << "  {\n";
        out << "    \"name\": \""  << sym.name << "\",\n";
        out << "    \"type\": \""  << sym.type << "\",\n";
        out << "    \"kind\": \""  << kindStr(sym.kind) << "\",\n";
        out << "    \"scope\": "   << sym.scopeLevel << ",\n";
        out << "    \"line\": "    << sym.declLine << "\n";
        out << "  }";

        if (i + 1 < m_allSymbols.size()) out << ",";
        out << "\n";
    }

    out << "]\n";
}

private:
    vector<unordered_map<string, Symbol>> m_scopes;
    vector<Symbol> m_allSymbols;

    static string kindStr(SymbolKind k) {
        switch (k) {
            case SymbolKind::VARIABLE:  return "variable";
            case SymbolKind::FUNCTION:  return "function";
            case SymbolKind::PARAMETER: return "parameter";
            default:                    return "unknown";
        }
    }
};
