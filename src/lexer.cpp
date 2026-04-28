#include "lexer.h"
#include <stdexcept>
#include <unordered_map>
#include <cctype>

// ─────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────
Lexer::Lexer(const std::string& source)
    : m_source(source), m_pos(0), m_line(1), m_col(1) {}

// ─────────────────────────────────────────────
//  Public: tokenize()
// ─────────────────────────────────────────────
std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (true) {
        skipWhitespace();

        if (m_pos >= m_source.size()) {
            tokens.push_back(makeToken(TokenType::END_OF_FILE, "EOF"));
            break;
        }

        char c  = peek();
        char c1 = peek(1);

        // ── comments ────────────────────────
        if (c == '/' && c1 == '/') {
            skipLineComment();
            continue;
        }
        if (c == '/' && c1 == '*') {
            skipBlockComment();
            continue;
        }

        // ── preprocessor directive ───────────
        if (c == '#') {
            tokens.push_back(scanOperatorOrPunct());
            continue;
        }

        // ── identifiers / keywords ───────────
        if (std::isalpha(c) || c == '_') {
            tokens.push_back(scanIdentifierOrKeyword());
            continue;
        }

        // ── numbers ─────────────────────────
        if (std::isdigit(c)) {
            tokens.push_back(scanNumber());
            continue;
        }

        // ── character literal ────────────────
        if (c == '\'') {
            tokens.push_back(scanChar());
            continue;
        }

        // ── string literal ───────────────────
        if (c == '"') {
            tokens.push_back(scanString());
            continue;
        }

        // ── operators / punctuation ──────────
        tokens.push_back(scanOperatorOrPunct());
    }

    return tokens;
}

// ─────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────
char Lexer::peek(int offset) const {
    size_t idx = m_pos + offset;
    if (idx >= m_source.size()) return '\0';
    return m_source[idx];
}

char Lexer::advance() {
    char c = m_source[m_pos++];
    if (c == '\n') { m_line++; m_col = 1; }
    else           { m_col++; }
    return c;
}

void Lexer::skipWhitespace() {
    while (m_pos < m_source.size() && std::isspace(peek()))
        advance();
}

void Lexer::skipLineComment() {
    // consume until newline
    while (m_pos < m_source.size() && peek() != '\n')
        advance();
}

void Lexer::skipBlockComment() {
    advance(); advance(); // consume '/' '*'
    while (m_pos + 1 < m_source.size()) {
        if (peek() == '*' && peek(1) == '/') {
            advance(); advance(); // consume '*' '/'
            return;
        }
        advance();
    }
    // unterminated block comment — just consume to end
}

Token Lexer::makeToken(TokenType t, const std::string& val) const {
    return Token{ t, val, m_line, m_col };
}

// ─────────────────────────────────────────────
//  Scanner: identifier or keyword
// ─────────────────────────────────────────────
Token Lexer::scanIdentifierOrKeyword() {
    int startLine = m_line, startCol = m_col;
    std::string word;
    while (m_pos < m_source.size() &&
           (std::isalnum(peek()) || peek() == '_'))
        word += advance();

    TokenType t = lookupKeyword(word);
    return Token{ t, word, startLine, startCol };
}

// ─────────────────────────────────────────────
//  Scanner: integer or float literal
// ─────────────────────────────────────────────
Token Lexer::scanNumber() {
    int startLine = m_line, startCol = m_col;
    std::string num;
    bool isFloat = false;

    while (m_pos < m_source.size() && std::isdigit(peek()))
        num += advance();

    if (peek() == '.' && std::isdigit(peek(1))) {
        isFloat = true;
        num += advance(); // consume '.'
        while (m_pos < m_source.size() && std::isdigit(peek()))
            num += advance();
    }

    // optional exponent  e.g. 1.5e10
    if ((peek() == 'e' || peek() == 'E')) {
        isFloat = true;
        num += advance();
        if (peek() == '+' || peek() == '-') num += advance();
        while (m_pos < m_source.size() && std::isdigit(peek()))
            num += advance();
    }

    // optional float suffix f/F
    if (peek() == 'f' || peek() == 'F') { isFloat = true; num += advance(); }

    TokenType t = isFloat ? TokenType::LIT_FLOAT : TokenType::LIT_INT;
    return Token{ t, num, startLine, startCol };
}

// ─────────────────────────────────────────────
//  Scanner: character literal  'x'  '\n'
// ─────────────────────────────────────────────
Token Lexer::scanChar() {
    int startLine = m_line, startCol = m_col;
    std::string val;
    val += advance(); // opening '
    if (peek() == '\\') { val += advance(); val += advance(); } // escape
    else if (peek() != '\'') val += advance();
    if (peek() == '\'') val += advance(); // closing '
    return Token{ TokenType::LIT_CHAR, val, startLine, startCol };
}

// ─────────────────────────────────────────────
//  Scanner: string literal  "hello\n"
// ─────────────────────────────────────────────
Token Lexer::scanString() {
    int startLine = m_line, startCol = m_col;
    std::string val;
    val += advance(); // opening "
    while (m_pos < m_source.size() && peek() != '"') {
        if (peek() == '\\') { val += advance(); } // keep escape sequence
        val += advance();
    }
    if (peek() == '"') val += advance(); // closing "
    return Token{ TokenType::LIT_STRING, val, startLine, startCol };
}

// ─────────────────────────────────────────────
//  Scanner: operators and punctuation
// ─────────────────────────────────────────────
Token Lexer::scanOperatorOrPunct() {
    int startLine = m_line, startCol = m_col;
    char c  = peek();
    char c1 = peek(1);

    // Two-character operators
    auto two = [&](TokenType t, const std::string& s) -> Token {
        advance(); advance();
        return Token{ t, s, startLine, startCol };
    };

    switch (c) {
        case '+':
            if (c1 == '+') return two(TokenType::OP_INC,         "++");
            if (c1 == '=') return two(TokenType::OP_PLUS_ASSIGN,  "+=");
            advance(); return Token{ TokenType::OP_PLUS,    "+", startLine, startCol };
        case '-':
            if (c1 == '-') return two(TokenType::OP_DEC,          "--");
            if (c1 == '=') return two(TokenType::OP_MINUS_ASSIGN, "-=");
            advance(); return Token{ TokenType::OP_MINUS,   "-", startLine, startCol };
        case '*':
            if (c1 == '=') return two(TokenType::OP_MUL_ASSIGN,   "*=");
            advance(); return Token{ TokenType::OP_MULTIPLY, "*", startLine, startCol };
        case '/':
            if (c1 == '=') return two(TokenType::OP_DIV_ASSIGN,   "/=");
            advance(); return Token{ TokenType::OP_DIVIDE,  "/", startLine, startCol };
        case '%':
            advance(); return Token{ TokenType::OP_MODULO,  "%", startLine, startCol };
        case '=':
            if (c1 == '=') return two(TokenType::OP_EQ,    "==");
            advance(); return Token{ TokenType::OP_ASSIGN,  "=", startLine, startCol };
        case '!':
            if (c1 == '=') return two(TokenType::OP_NEQ,   "!=");
            advance(); return Token{ TokenType::OP_NOT,     "!", startLine, startCol };
        case '<':
            if (c1 == '=') return two(TokenType::OP_LEQ,   "<=");
            advance(); return Token{ TokenType::OP_LT,      "<", startLine, startCol };
        case '>':
            if (c1 == '=') return two(TokenType::OP_GEQ,   ">=");
            advance(); return Token{ TokenType::OP_GT,      ">", startLine, startCol };
     
case '&':
    if (c1 == '&') return two(TokenType::OP_AND, "&&");
    advance(); return Token{ TokenType::OP_ADDRESS, "&", startLine, startCol };
        case '|':
            if (c1 == '|') return two(TokenType::OP_OR,    "||");
            advance(); return Token{ TokenType::ERROR,      "|", startLine, startCol };
        case '(': advance(); return Token{ TokenType::LPAREN,    "(", startLine, startCol };
        case ')': advance(); return Token{ TokenType::RPAREN,    ")", startLine, startCol };
        case '{': advance(); return Token{ TokenType::LBRACE,    "{", startLine, startCol };
        case '}': advance(); return Token{ TokenType::RBRACE,    "}", startLine, startCol };
        case '[': advance(); return Token{ TokenType::LBRACKET,  "[", startLine, startCol };
        case ']': advance(); return Token{ TokenType::RBRACKET,  "]", startLine, startCol };
        case ';': advance(); return Token{ TokenType::SEMICOLON, ";", startLine, startCol };
        case ',': advance(); return Token{ TokenType::COMMA,     ",", startLine, startCol };
        case '.': advance(); return Token{ TokenType::DOT,       ".", startLine, startCol };
        case ':': advance(); return Token{ TokenType::COLON,     ":", startLine, startCol };
        case '#': advance(); return Token{ TokenType::HASH,      "#", startLine, startCol };
        default:
            advance();
            return Token{ TokenType::ERROR, std::string(1, c), startLine, startCol };
    }
}

// ─────────────────────────────────────────────
//  Keyword table
// ─────────────────────────────────────────────
TokenType Lexer::lookupKeyword(const std::string& word) {
    static const std::unordered_map<std::string, TokenType> kw = {
        {"int",      TokenType::KW_INT},
        {"float",    TokenType::KW_FLOAT},
        {"char",     TokenType::KW_CHAR},
        {"double",   TokenType::KW_DOUBLE},
        {"void",     TokenType::KW_VOID},
        {"if",       TokenType::KW_IF},
        {"else",     TokenType::KW_ELSE},
        {"while",    TokenType::KW_WHILE},
        {"for",      TokenType::KW_FOR},
        {"do",       TokenType::KW_DO},
        {"return",   TokenType::KW_RETURN},
        {"break",    TokenType::KW_BREAK},
        {"continue", TokenType::KW_CONTINUE},
        {"struct",   TokenType::KW_STRUCT},
        {"typedef",  TokenType::KW_TYPEDEF},
        {"include",  TokenType::KW_INCLUDE},
    };
    auto it = kw.find(word);
    return (it != kw.end()) ? it->second : TokenType::IDENTIFIER;
}

// ─────────────────────────────────────────────
//  Token type → name string  (for output)
// ─────────────────────────────────────────────
std::string Lexer::tokenTypeName(TokenType t) {
    switch (t) {
        case TokenType::KW_INT:          return "KW_INT";
        case TokenType::KW_FLOAT:        return "KW_FLOAT";
        case TokenType::KW_CHAR:         return "KW_CHAR";
        case TokenType::KW_DOUBLE:       return "KW_DOUBLE";
        case TokenType::KW_VOID:         return "KW_VOID";
        case TokenType::KW_IF:           return "KW_IF";
        case TokenType::KW_ELSE:         return "KW_ELSE";
        case TokenType::KW_WHILE:        return "KW_WHILE";
        case TokenType::KW_FOR:          return "KW_FOR";
        case TokenType::KW_DO:           return "KW_DO";
        case TokenType::KW_RETURN:       return "KW_RETURN";
        case TokenType::KW_BREAK:        return "KW_BREAK";
        case TokenType::KW_CONTINUE:     return "KW_CONTINUE";
        case TokenType::KW_STRUCT:       return "KW_STRUCT";
        case TokenType::KW_TYPEDEF:      return "KW_TYPEDEF";
        case TokenType::KW_INCLUDE:      return "KW_INCLUDE";
        case TokenType::LIT_INT:         return "LIT_INT";
        case TokenType::LIT_FLOAT:       return "LIT_FLOAT";
        case TokenType::LIT_CHAR:        return "LIT_CHAR";
        case TokenType::LIT_STRING:      return "LIT_STRING";
        case TokenType::IDENTIFIER:      return "IDENTIFIER";
        case TokenType::OP_PLUS:         return "OP_PLUS";
        case TokenType::OP_MINUS:        return "OP_MINUS";
        case TokenType::OP_MULTIPLY:     return "OP_MULTIPLY";
        case TokenType::OP_DIVIDE:       return "OP_DIVIDE";
        case TokenType::OP_MODULO:       return "OP_MODULO";
        case TokenType::OP_ASSIGN:       return "OP_ASSIGN";
        case TokenType::OP_PLUS_ASSIGN:  return "OP_PLUS_ASSIGN";
        case TokenType::OP_MINUS_ASSIGN: return "OP_MINUS_ASSIGN";
        case TokenType::OP_MUL_ASSIGN:   return "OP_MUL_ASSIGN";
        case TokenType::OP_DIV_ASSIGN:   return "OP_DIV_ASSIGN";
        case TokenType::OP_EQ:           return "OP_EQ";
        case TokenType::OP_NEQ:          return "OP_NEQ";
        case TokenType::OP_LT:           return "OP_LT";
        case TokenType::OP_GT:           return "OP_GT";
        case TokenType::OP_LEQ:          return "OP_LEQ";
        case TokenType::OP_GEQ:          return "OP_GEQ";
        case TokenType::OP_AND:          return "OP_AND";
        case TokenType::OP_OR:           return "OP_OR";
        case TokenType::OP_NOT:          return "OP_NOT";
        case TokenType::OP_ADDRESS:     return "OP_ADDRESS";
        case TokenType::OP_INC:          return "OP_INC";
        case TokenType::OP_DEC:          return "OP_DEC";
        case TokenType::LPAREN:          return "LPAREN";
        case TokenType::RPAREN:          return "RPAREN";
        case TokenType::LBRACE:          return "LBRACE";
        case TokenType::RBRACE:          return "RBRACE";
        case TokenType::LBRACKET:        return "LBRACKET";
        case TokenType::RBRACKET:        return "RBRACKET";
        case TokenType::SEMICOLON:       return "SEMICOLON";
        case TokenType::COMMA:           return "COMMA";
        case TokenType::DOT:             return "DOT";
        case TokenType::COLON:           return "COLON";
        case TokenType::HASH:            return "HASH";
        case TokenType::END_OF_FILE:     return "EOF";
        case TokenType::ERROR:           return "ERROR";
        default:                         return "UNKNOWN";
    }
}
