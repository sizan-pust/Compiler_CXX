// // #include "lexer.h"
// // #include "parser.h"
// // #include "ast_printer.h"
// // #include <iostream>
// // #include <fstream>
// // #include <sstream>

// // // ─────────────────────────────────────────────
// // //  Escape a string for safe JSON output
// // // ─────────────────────────────────────────────
// // static std::string jsonEscape(const std::string& s) {
// //     std::string out;
// //     for (char c : s) {
// //         switch (c) {
// //             case '"':  out += "\\\""; break;
// //             case '\\': out += "\\\\"; break;
// //             case '\n': out += "\\n";  break;
// //             case '\r': out += "\\r";  break;
// //             case '\t': out += "\\t";  break;
// //             default:   out += c;
// //         }
// //     }
// //     return out;
// // }

// // // ─────────────────────────────────────────────
// // //  Print all tokens as a JSON array to stdout.
// // //  The Python UI reads this via subprocess.
// // //
// // //  Format:
// // //  [
// // //    {"type":"KW_INT","value":"int","line":1,"col":1},
// // //    ...
// // //  ]
// // // ─────────────────────────────────────────────
// // static void printTokensJSON(const std::vector<Token>& tokens) {
// //     std::cout << "[\n";
// //     for (size_t i = 0; i < tokens.size(); ++i) {
// //         const Token& t = tokens[i];
// //         std::cout << "  {"
// //                   << "\"type\":\""  << Lexer::tokenTypeName(t.type) << "\","
// //                   << "\"value\":\"" << jsonEscape(t.value)           << "\","
// //                   << "\"line\":"    << t.line                        << ","
// //                   << "\"col\":"     << t.col
// //                   << "}";
// //         if (i + 1 < tokens.size()) std::cout << ",";
// //         std::cout << "\n";
// //     }
// //     std::cout << "]\n";
// // }

// // // ─────────────────────────────────────────────
// // //  Entry point
// // //
// // //  Usage:
// // //    compiler.exe <phase> <filepath>
// // //
// // //  Phase values (we add more as we build):
// // //    --lex    run lexer only  (Phase 1)
// // // ─────────────────────────────────────────────
// // int main(int argc, char* argv[]) {
// //     if (argc < 3) {
// //         std::cerr << "Usage: compiler <phase> <source_file>\n";
// //         std::cerr << "  Phases: --lex\n";
// //         return 1;
// //     }

// //     std::string phase      = argv[1];
// //     std::string sourceFile = argv[2];

// //     // ── Read source file ─────────────────────
// //     std::ifstream file(sourceFile);
// //     if (!file.is_open()) {
// //         std::cerr << "ERROR: Cannot open file: " << sourceFile << "\n";
// //         return 1;
// //     }
// //     std::ostringstream buf;
// //     buf << file.rdbuf();
// //     std::string source = buf.str();

// //     // ── Dispatch by phase ────────────────────
// //             Lexer lexer(source);
// //         std::vector<Token> tokens = lexer.tokenize();
// //     if (phase == "--lex") {


// //         // Report errors to stderr (Python UI reads stderr separately)
// //         for (const Token& t : tokens) {
// //             if (t.type == TokenType::ERROR) {
// //                 std::cerr << "LEX_ERROR line " << t.line
// //                           << " col " << t.col
// //                           << ": unrecognised character '"
// //                           << t.value << "'\n";
// //             }
// //         }

// //         // Send token list as JSON to stdout
// //         printTokensJSON(tokens);
// //         return 0;
// //     }

// //         if (phase == "--parse") {
// //         for (const Token& t : tokens)
// //             if (t.type == TokenType::ERROR)
// //                 std::cerr << "LEX_ERROR line " << t.line << " col " << t.col
// //                           << ": unrecognised character '" << t.value << "'\n";
 
// //         Parser     parser(tokens);
// //         ASTNodePtr root = parser.parse();
 
// //         for (const ParseError& e : parser.errors())
// //             std::cerr << "PARSE_ERROR line " << e.line << " col " << e.col
// //                       << ": " << e.message << "\n";
 
// //         printASTJson(root.get(), std::cout, 0);
// //         std::cout << "\n";
// //         return parser.hasErrors() ? 1 : 0;
// //     }

// //     std::cerr << "ERROR: Unknown phase '" << phase << "'\n";
// //     return 1;
// // }
// #include "lexer.h"
// #include "parser.h"
// #include "ast_printer.h"
// #include "semantic.h"
// #include <iostream>
// #include <fstream>
// #include <sstream>

// static std::string jsonEscape(const std::string& s) {
//     std::string out;
//     for (char c : s) {
//         switch (c) {
//             case '"':  out += "\\\""; break;
//             case '\\': out += "\\\\"; break;
//             case '\n': out += "\\n";  break;
//             case '\r': out += "\\r";  break;
//             case '\t': out += "\\t";  break;
//             default:   out += c;
//         }
//     }
//     return out;
// }

// static void printTokensJSON(const std::vector<Token>& tokens) {
//     std::cout << "[\n";
//     for (size_t i = 0; i < tokens.size(); ++i) {
//         const Token& t = tokens[i];
//         std::cout << "  {"
//                   << "\"type\":\""  << Lexer::tokenTypeName(t.type) << "\","
//                   << "\"value\":\"" << jsonEscape(t.value)           << "\","
//                   << "\"line\":"    << t.line                        << ","
//                   << "\"col\":"     << t.col
//                   << "}";
//         if (i + 1 < tokens.size()) std::cout << ",";
//         std::cout << "\n";
//     }
//     std::cout << "]\n";
// }

// int main(int argc, char* argv[]) {
//     if (argc < 3) {
//         std::cerr << "Usage: compiler <phase> <source_file>\n";
//         std::cerr << "  Phases: --lex  --parse\n";
//         return 1;
//     }

//     std::string phase      = argv[1];
//     std::string sourceFile = argv[2];

//     std::ifstream file(sourceFile);
//     if (!file.is_open()) {
//         std::cerr << "ERROR: Cannot open file: " << sourceFile << "\n";
//         return 1;
//     }
//     std::ostringstream buf;
//     buf << file.rdbuf();
//     std::string source = buf.str();

//     Lexer              lexer(source);
//     std::vector<Token> tokens = lexer.tokenize();

//     if (phase == "--lex") {
//         for (const Token& t : tokens)
//             if (t.type == TokenType::ERROR)
//                 std::cerr << "LEX_ERROR line " << t.line << " col " << t.col
//                           << ": unrecognised character '" << t.value << "'\n";
//         printTokensJSON(tokens);
//         return 0;
//     }

//     if (phase == "--parse") {
//         for (const Token& t : tokens)
//             if (t.type == TokenType::ERROR)
//                 std::cerr << "LEX_ERROR line " << t.line << " col " << t.col
//                           << ": unrecognised character '" << t.value << "'\n";

//         Parser     parser(tokens);
//         ASTNodePtr root = parser.parse();

//         for (const ParseError& e : parser.errors())
//             std::cerr << "PARSE_ERROR line " << e.line << " col " << e.col
//                       << ": " << e.message << "\n";

//         printASTJson(root.get(), std::cout, 0);
//         std::cout << "\n";
//         return parser.hasErrors() ? 1 : 0;
//     }
//         // ── Phase 3: Semantic ─────────────────────
//     if (phase == "--semantic") {
//         SemanticAnalyzer sem;
//         sem.analyze(root.get());
 
//         // Report semantic errors to stderr
//         for (const SemanticError& e : sem.errors())
//             cerr << "SEMANTIC_ERROR line " << e.line
//                  << ": " << e.message << "\n";
 
//         // stdout: two JSON objects separated by a separator line
//         // Object 1: annotated AST
//         cout << "{ \"ast\":\n";
//         printASTJson(root.get(), cout, 0);
//         cout << ",\n";
 
//         // Object 2: symbol table
//         cout << "\"symbols\":\n";
//         sem.symbolTable().printJSON(cout);
//         cout << "}\n";
 
//         return sem.hasErrors() ? 1 : 0;
//     }

//     std::cerr << "ERROR: Unknown phase '" << phase << "'\n";
//     return 1;
// }
#include "lexer.h"
#include "parser.h"
#include "ast_printer.h"
#include "semantic.h"
#include "symbol_table.h"
#include "icg.h"
#include "optimizer.h"
#include "codegen.h"
#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

static string jsonEscape(const string& s) {
    string out;
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

static void printTokensJSON(const vector<Token>& tokens) {
    cout << "[\n";
    for (size_t i = 0; i < tokens.size(); ++i) {
        const Token& t = tokens[i];
        cout << "  {"
             << "\"type\":\""  << Lexer::tokenTypeName(t.type) << "\","
             << "\"value\":\"" << jsonEscape(t.value)           << "\","
             << "\"line\":"    << t.line                        << ","
             << "\"col\":"     << t.col
             << "}";
        if (i + 1 < tokens.size()) cout << ",";
        cout << "\n";
    }
    cout << "]\n";
}

// ─────────────────────────────────────────────
//  Usage:  compiler <phase> <source_file>
//  Phases: --lex   --parse   --semantic  --icg  --optimize  --codegen
// ─────────────────────────────────────────────
int main(int argc, char* argv[]) {
    if (argc < 3) {
        cerr << "Usage: compiler <phase> <source_file>\n";
        cerr << "  Phases: --lex  --parse  --semantic  --icg  --optimize  --codegen\n";
        return 1;
    }

    string phase      = argv[1];
    string sourceFile = argv[2];

    // ── Read source ──────────────────────────
    ifstream file(sourceFile);
    if (!file.is_open()) {
        cerr << "ERROR: Cannot open file: " << sourceFile << "\n";
        return 1;
    }
    ostringstream buf;
    buf << file.rdbuf();
    string source = buf.str();

    // ── Phase 1: Lex ─────────────────────────
    Lexer         lexer(source);
    vector<Token> tokens = lexer.tokenize();

    if (phase == "--lex") {
        for (const Token& t : tokens)
            if (t.type == TokenType::ERROR)
                cerr << "LEX_ERROR line " << t.line << " col " << t.col
                     << ": unrecognised character '" << t.value << "'\n";
        printTokensJSON(tokens);
        return 0;
    }

    // ── Phase 2: Parse ────────────────────────
    for (const Token& t : tokens)
        if (t.type == TokenType::ERROR)
            cerr << "LEX_ERROR line " << t.line << " col " << t.col
                 << ": unrecognised character '" << t.value << "'\n";

    Parser     parser(tokens);
    ASTNodePtr root = parser.parse();

    for (const ParseError& e : parser.errors())
        cerr << "PARSE_ERROR line " << e.line << " col " << e.col
             << ": " << e.message << "\n";

    if (phase == "--parse") {
        printASTJson(root.get(), cout, 0);
        cout << "\n";
        return parser.hasErrors() ? 1 : 0;
    }

    // ── Phase 3: Semantic ─────────────────────
    if (phase == "--semantic") {
        SemanticAnalyzer sem;
        sem.analyze(root.get());

        // Report semantic errors to stderr
        for (const SemanticError& e : sem.errors())
            cerr << "SEMANTIC_ERROR line " << e.line
                 << ": " << e.message << "\n";

        // stdout: two JSON objects separated by a separator line
        // Object 1: annotated AST
        cout << "{ \"ast\":\n";
        printASTJson(root.get(), cout, 0);
        cout << ",\n";

        // Object 2: symbol table
        cout << "\"symbols\":\n";
        sem.symbolTable().printJSON(cout);
        cout << "}\n";

        return sem.hasErrors() ? 1 : 0;
    }
        // ── Phase 4: ICG ──────────────────────────
    if (phase == "--icg") {
        ICGenerator icg;
        icg.generate(root.get());
 
        for (const ICGError& e : icg.errors())
            cerr << "ICG_ERROR line " << e.line
                 << ": " << e.message << "\n";
 
        // stdout: JSON TAC list + human-readable text
        cout << "{ \"tac\":\n";
        icg.program().printJSON(cout);
        cout << ",\n\"text\":\n\"";
 
        // Embed text form as a JSON string
        ostringstream textBuf;
        icg.program().printText(textBuf);
        string textStr = textBuf.str();
        cout << jsonEscape(textStr);
        cout << "\"\n}\n";
 
        return icg.hasErrors() ? 1 : 0;
    }

    // ── Phase 6: Code Optimization ────────────
    if (phase == "--optimize") {
        ICGenerator icg;
        icg.generate(root.get());

        for (const ICGError& e : icg.errors())
            cerr << "ICG_ERROR line " << e.line
                 << ": " << e.message << "\n";

        CodeOptimizer optimizer;
        TACProgram optProgram = optimizer.optimize(icg.program());

        for (const OptError& e : optimizer.errors())
            cerr << "OPT_ERROR line " << e.line
                 << ": " << e.message << "\n";

        // Report optimization statistics
        int origCount = optimizer.originalInstrCount();
        int optCount = optimizer.optimizedInstrCount();
        int reduction = origCount - optCount;
        
        cerr << "Optimization: " << origCount << " → " << optCount 
             << " instructions (" << reduction << " removed)\n";

        // stdout: JSON optimized TAC + statistics
        cout << "{ \"tac\":\n";
        optProgram.printJSON(cout);
        cout << ",\n\"text\":\n\"";

        ostringstream textBuf;
        optProgram.printText(textBuf);
        string textStr = textBuf.str();
        cout << jsonEscape(textStr);

        cout << "\",\n\"stats\": {\n";
        cout << "  \"original\": " << origCount << ",\n";
        cout << "  \"optimized\": " << optCount << ",\n";
        cout << "  \"reduction\": " << reduction << "\n";
        cout << "}\n}\n";

        return optimizer.hasErrors() ? 1 : 0;
    }

    // ── Phase 7: Code Generation ──────────────
    if (phase == "--codegen") {
        ICGenerator icg;
        icg.generate(root.get());

        for (const ICGError& e : icg.errors())
            cerr << "ICG_ERROR line " << e.line
                 << ": " << e.message << "\n";

        // Optimize the TAC
        CodeOptimizer optimizer;
        TACProgram optProgram = optimizer.optimize(icg.program());

        for (const OptError& e : optimizer.errors())
            cerr << "OPT_ERROR line " << e.line
                 << ": " << e.message << "\n";

        // Generate assembly code
        CodeGenerator codeGen;
        codeGen.generate(optProgram);

        for (const CodeGenError& e : codeGen.errors())
            cerr << "CODEGEN_ERROR line " << e.line
                 << ": " << e.message << "\n";

        // Output assembly code
        cout << "; Generated x86-64 Assembly Code\n";
        cout << "; ───────────────────────────────────\n\n";

        for (const auto& line : codeGen.assembly()) {
            cout << line << "\n";
        }

        return codeGen.hasErrors() ? 1 : 0;
    }

    cerr << "ERROR: Unknown phase '" << phase << "'\n";
    return 1;
}