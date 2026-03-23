#pragma once
#include "ast.h"
#include <iostream>
#include <string>

// ─────────────────────────────────────────────
//  Recursively print an AST node as JSON.
//
//  Output format:
//  {
//    "kind": "FunctionDecl",
//    "value": "int main",
//    "line": 3,
//    "children": [ ... ]
//  }
// ─────────────────────────────────────────────

static std::string jsonEscapeAST(const std::string& s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:   out += c;
        }
    }
    return out;
}

inline void printASTJson(const ASTNode* node,
                         std::ostream& out,
                         int indent = 0)
{
    if (!node) return;
    std::string pad(indent * 2, ' ');
    std::string pad2((indent + 1) * 2, ' ');

    out << pad << "{\n";
    out << pad2 << "\"kind\": \""  << ASTNode::kindName(node->kind)       << "\",\n";
    out << pad2 << "\"value\": \"" << jsonEscapeAST(node->sval)           << "\",\n";
    out << pad2 << "\"line\": "    << node->line                          << ",\n";
    out << pad2 << "\"children\": ";

    if (node->children.empty()) {
        out << "[]\n";
    } else {
        out << "[\n";
        for (size_t i = 0; i < node->children.size(); ++i) {
            printASTJson(node->children[i].get(), out, indent + 2);
            if (i + 1 < node->children.size()) out << ",";
            out << "\n";
        }
        out << pad2 << "]\n";
    }
    out << pad << "}";
}
