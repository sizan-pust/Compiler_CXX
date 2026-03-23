#pragma once
#include <string>
#include <vector>

// ─────────────────────────────────────────────
//  All token types the lexer can produce
// ─────────────────────────────────────────────
enum class TokenType {
    // Keywords
    KW_INT, KW_FLOAT, KW_CHAR, KW_DOUBLE, KW_VOID,
    KW_IF, KW_ELSE, KW_WHILE, KW_FOR, KW_DO,
    KW_RETURN, KW_BREAK, KW_CONTINUE,
    KW_STRUCT, KW_TYPEDEF, KW_INCLUDE,

    // Literals
    LIT_INT,        // e.g.  42
    LIT_FLOAT,      // e.g.  3.14
    LIT_CHAR,       // e.g.  'a'
    LIT_STRING,     // e.g.  "hello"

    // Identifier
    IDENTIFIER,     // e.g.  myVar, foo

    // Arithmetic operators
    OP_PLUS,        // +
    OP_MINUS,       // -
    OP_MULTIPLY,    // *
    OP_DIVIDE,      // /
    OP_MODULO,      // %

    // Assignment
    OP_ASSIGN,      // =
    OP_PLUS_ASSIGN, // +=
    OP_MINUS_ASSIGN,// -=
    OP_MUL_ASSIGN,  // *=
    OP_DIV_ASSIGN,  // /=

    // Relational
    OP_EQ,          // ==
    OP_NEQ,         // !=
    OP_LT,          // <
    OP_GT,          // >
    OP_LEQ,         // <=
    OP_GEQ,         // >=

    // Logical
    OP_AND,         // &&
    OP_OR,          // ||
    OP_NOT,         // !

    // Increment / Decrement
    OP_INC,         // ++
    OP_DEC,         // --

    // Punctuation / Delimiters
    LPAREN,         // (
    RPAREN,         // )
    LBRACE,         // {
    RBRACE,         // }
    LBRACKET,       // [
    RBRACKET,       // ]
    SEMICOLON,      // ;
    COMMA,          // ,
    DOT,            // .
    COLON,          // :
    HASH,           // #

    // Special
    END_OF_FILE,
    ERROR           // unrecognised character
};

// ─────────────────────────────────────────────
//  A single token
// ─────────────────────────────────────────────
struct Token {
    TokenType   type;
    std::string value;   // exact text from source
    int         line;
    int         col;
};

// ─────────────────────────────────────────────
//  Lexer class
// ─────────────────────────────────────────────
class Lexer {
public:
    explicit Lexer(const std::string& source);

    // Tokenize the entire source; returns all tokens including EOF
    std::vector<Token> tokenize();

    // Human-readable name for any TokenType (used for output)
    static std::string tokenTypeName(TokenType t);

private:
    std::string m_source;
    size_t      m_pos;
    int         m_line;
    int         m_col;

    // ── helpers ──────────────────────────────
    char        peek(int offset = 0) const;
    char        advance();
    void        skipWhitespace();
    void        skipLineComment();
    void        skipBlockComment();

    // ── scanners ─────────────────────────────
    Token       scanIdentifierOrKeyword();
    Token       scanNumber();
    Token       scanChar();
    Token       scanString();
    Token       scanOperatorOrPunct();

    Token       makeToken(TokenType t, const std::string& val) const;

    // Keyword lookup
    static TokenType lookupKeyword(const std::string& word);
};
