#include "lexer.h"
#include <iostream>
#include <fstream>
#include <sstream>

// ─────────────────────────────────────────────
//  Escape a string for safe JSON output
// ─────────────────────────────────────────────
static std::string jsonEscape(const std::string& s) {
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

// ─────────────────────────────────────────────
//  Print all tokens as a JSON array to stdout.
//  The Python UI reads this via subprocess.
//
//  Format:
//  [
//    {"type":"KW_INT","value":"int","line":1,"col":1},
//    ...
//  ]
// ─────────────────────────────────────────────
static void printTokensJSON(const std::vector<Token>& tokens) {
    std::cout << "[\n";
    for (size_t i = 0; i < tokens.size(); ++i) {
        const Token& t = tokens[i];
        std::cout << "  {"
                  << "\"type\":\""  << Lexer::tokenTypeName(t.type) << "\","
                  << "\"value\":\"" << jsonEscape(t.value)           << "\","
                  << "\"line\":"    << t.line                        << ","
                  << "\"col\":"     << t.col
                  << "}";
        if (i + 1 < tokens.size()) std::cout << ",";
        std::cout << "\n";
    }
    std::cout << "]\n";
}

// ─────────────────────────────────────────────
//  Entry point
//
//  Usage:
//    compiler.exe <phase> <filepath>
//
//  Phase values (we add more as we build):
//    --lex    run lexer only  (Phase 1)
// ─────────────────────────────────────────────
int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: compiler <phase> <source_file>\n";
        std::cerr << "  Phases: --lex\n";
        return 1;
    }

    std::string phase      = argv[1];
    std::string sourceFile = argv[2];

    // ── Read source file ─────────────────────
    std::ifstream file(sourceFile);
    if (!file.is_open()) {
        std::cerr << "ERROR: Cannot open file: " << sourceFile << "\n";
        return 1;
    }
    std::ostringstream buf;
    buf << file.rdbuf();
    std::string source = buf.str();

    // ── Dispatch by phase ────────────────────
    if (phase == "--lex") {
        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();

        // Report errors to stderr (Python UI reads stderr separately)
        for (const Token& t : tokens) {
            if (t.type == TokenType::ERROR) {
                std::cerr << "LEX_ERROR line " << t.line
                          << " col " << t.col
                          << ": unrecognised character '"
                          << t.value << "'\n";
            }
        }

        // Send token list as JSON to stdout
        printTokensJSON(tokens);
        return 0;
    }

    std::cerr << "ERROR: Unknown phase '" << phase << "'\n";
    return 1;
}
