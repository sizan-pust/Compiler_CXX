#include "parser.h"
#include <stdexcept>
using namespace std;

//  Constructor
Parser::Parser(const std::vector<Token>& tokens)
    : m_tokens(tokens), m_pos(0) {}


//  Token navigation
const Token& Parser::peek(int offset) const {
    size_t idx = m_pos + offset;
    if (idx >= m_tokens.size()) return m_tokens.back(); // EOF
    return m_tokens[idx];
}

const Token& Parser::advance() {
    if (!isAtEnd()) m_pos++;
    return m_tokens[m_pos - 1];
}

bool Parser::check(TokenType t) const {
    return peek().type == t;
}

bool Parser::match(TokenType t) {
    if (check(t)) { advance(); return true; }
    return false;
}

const Token& Parser::expect(TokenType t, const std::string& msg) {
    if (check(t)) return advance();
    error(msg);
    return peek(); // best-effort continue
}

bool Parser::isAtEnd() const {
    return peek().type == TokenType::END_OF_FILE;
}

bool Parser::isTypeName() const {
    TokenType t = peek().type;
    return t == TokenType::KW_INT    || t == TokenType::KW_FLOAT  ||
           t == TokenType::KW_CHAR   || t == TokenType::KW_DOUBLE ||
           t == TokenType::KW_VOID;
}


//  Error handling
void Parser::error(const std::string& msg) {
    m_errors.push_back({ msg, peek().line, peek().col });
}

// Panic-mode: skip tokens until we find a safe restart point
void Parser::synchronize() {
    advance();
    while (!isAtEnd()) {
        // After a semicolon, we can restart cleanly
        if (m_tokens[m_pos - 1].type == TokenType::SEMICOLON) return;
        // Before these keywords a new statement starts
        switch (peek().type) {
            case TokenType::KW_INT:
            case TokenType::KW_FLOAT:
            case TokenType::KW_CHAR:
            case TokenType::KW_DOUBLE:
            case TokenType::KW_VOID:
            case TokenType::KW_IF:
            case TokenType::KW_WHILE:
            case TokenType::KW_FOR:
            case TokenType::KW_RETURN:
            case TokenType::RBRACE:
                return;
            default:
                break;
        }
        advance();
    }
}

//  Type name parser
//  Returns e.g. "int", "float", "void"
std::string Parser::parseTypeName() {
    if (!isTypeName()) {
        error("Expected type name (int/float/char/double/void)");
        if (!isAtEnd()) advance();   // IMPORTANT: prevent infinite loop
        return "unknown";
    }
    return advance().value;
}


//  Top-level: parse()
ASTNodePtr Parser::parse() {
    auto program = makeNode(NodeType::PROGRAM, 0, "program");

    while (!isAtEnd()) {
        if (check(TokenType::HASH)) {
            int startLine = peek().line;
            while (!isAtEnd() && peek().line == startLine) advance();
            continue;
        }

        try {
            if (isTypeName()) {
                program->addChild(parseDecl());
            } else {
                program->addChild(parseStmt());
            }
        } catch (...) {
            synchronize();
        }
    }

    return program;
}


//  Declaration: variable or function
ASTNodePtr Parser::parseDecl() {
    if (!isTypeName()) {
        error("Expected declaration type");
        synchronize();
        return makeNode(NodeType::VAR_DECL, peek().line, "error");
    }

    std::string typeName = parseTypeName();

    Token nameTok = peek();
    if (!check(TokenType::IDENTIFIER)) {
        error("Expected identifier after type");
        synchronize();
        return makeNode(NodeType::VAR_DECL, nameTok.line, "error");
    }

    advance(); // consume identifier

    if (check(TokenType::LPAREN)) {
        return parseFuncDecl(typeName, nameTok);
    }

    return parseVarDecl(typeName, nameTok);
}

//  Function declaration
//  funcDecl → type IDENT '(' paramList ')' block
ASTNodePtr Parser::parseFuncDecl(const std::string& retType, const Token& nameTok) {
    auto node = makeNode(NodeType::FUNCTION_DECL, nameTok.line,
                         retType + " " + nameTok.value);
    expect(TokenType::LPAREN, "Expected '(' after function name");

    // Parameter list
    if (!check(TokenType::RPAREN)) {
        do {
            string pType = parseTypeName();
            Token       pName = peek();
            expect(TokenType::IDENTIFIER, "Expected parameter name");
            auto param = makeNode(NodeType::PARAM, pName.line,
                                  pType + " " + pName.value);
            node->addChild(std::move(param));
        } while (match(TokenType::COMMA));
    }
    expect(TokenType::RPAREN, "Expected ')' after parameters");

    // Body
    node->addChild(parseBlock());
    return node;
}


//  Variable declaration
//  varDecl → type IDENT ('=' expr)? ';'

ASTNodePtr Parser::parseVarDecl(const std::string& typeName, const Token& nameTok) {
    auto node = makeNode(NodeType::VAR_DECL, nameTok.line,
                         typeName + " " + nameTok.value);
    if (match(TokenType::OP_ASSIGN)) {
        node->addChild(parseExpr());
    }
    expect(TokenType::SEMICOLON, "Expected ';' after variable declaration");
    return node;
}


//  Block  → '{' stmt* '}'

ASTNodePtr Parser::parseBlock() {
    Token brace = peek();
    expect(TokenType::LBRACE, "Expected '{'");
    auto block = makeNode(NodeType::BLOCK, brace.line);

    while (!isAtEnd() && !check(TokenType::RBRACE)) {
        try {
            block->addChild(parseStmt());
        } catch (...) {
            synchronize();
        }
    }
    expect(TokenType::RBRACE, "Expected '}'");
    return block;
}


//  Statement dispatcher
ASTNodePtr Parser::parseStmt() {
    // Variable declaration (type keyword at start)
    if (isTypeName()) {
        std::string typeName = parseTypeName();
        Token       nameTok  = peek();
        expect(TokenType::IDENTIFIER, "Expected variable name");
        return parseVarDecl(typeName, nameTok);
    }

    switch (peek().type) {
        case TokenType::KW_IF:       return parseIfStmt();
        case TokenType::KW_WHILE:    return parseWhileStmt();
        case TokenType::KW_FOR:      return parseForStmt();
        case TokenType::KW_RETURN:   return parseReturnStmt();
        case TokenType::KW_BREAK: {
            auto n = makeNode(NodeType::BREAK_STMT, peek().line);
            advance();
            expect(TokenType::SEMICOLON, "Expected ';' after break");
            return n;
        }
        case TokenType::KW_CONTINUE: {
            auto n = makeNode(NodeType::CONTINUE_STMT, peek().line);
            advance();
            expect(TokenType::SEMICOLON, "Expected ';' after continue");
            return n;
        }
        default:
            return parseExprStmt();
    }
}


//  if ( expr ) block ( else (ifStmt | block) )?
ASTNodePtr Parser::parseIfStmt() {
    int line = peek().line;
    advance(); // consume 'if'
    expect(TokenType::LPAREN, "Expected '(' after if");
    auto node = makeNode(NodeType::IF_STMT, line);
    node->addChild(parseExpr());           // condition
    expect(TokenType::RPAREN, "Expected ')' after if condition");
    node->addChild(parseBlock());          // then-branch

    if (match(TokenType::KW_ELSE)) {
        if (check(TokenType::KW_IF))
            node->addChild(parseIfStmt()); // else-if chain
        else
            node->addChild(parseBlock()); // else block
    }
    return node;
}


//  while ( expr ) block

ASTNodePtr Parser::parseWhileStmt() {
    int line = peek().line;
    advance(); // consume 'while'
    expect(TokenType::LPAREN, "Expected '(' after while");
    auto node = makeNode(NodeType::WHILE_STMT, line);
    node->addChild(parseExpr());
    expect(TokenType::RPAREN, "Expected ')' after while condition");
    node->addChild(parseBlock());
    return node;
}


//  for ( init ; cond ; incr ) block
ASTNodePtr Parser::parseForStmt() {
    int line = peek().line;
    advance(); // consume 'for'
    expect(TokenType::LPAREN, "Expected '(' after for");
    auto node = makeNode(NodeType::FOR_STMT, line);

    // init: varDecl or exprStmt or empty ';'
    if (isTypeName()) {
        std::string t = parseTypeName();
        Token n = peek();
        expect(TokenType::IDENTIFIER, "Expected variable name");
        node->addChild(parseVarDecl(t, n));
    } else if (!check(TokenType::SEMICOLON)) {
        node->addChild(parseExprStmt());
    } else {
        advance(); // empty init
        node->addChild(makeNode(NodeType::EXPR_STMT, line, "empty"));
    }

    // condition (optional)
    if (!check(TokenType::SEMICOLON)) {
        node->addChild(parseExpr());
    } else {
        node->addChild(makeNode(NodeType::LITERAL_INT, line, "1")); // always true
    }
    expect(TokenType::SEMICOLON, "Expected ';' after for condition");

    // increment (optional)
    if (!check(TokenType::RPAREN)) {
        node->addChild(parseExpr());
    } else {
        node->addChild(makeNode(NodeType::EXPR_STMT, line, "empty"));
    }
    expect(TokenType::RPAREN, "Expected ')' after for increment");
    node->addChild(parseBlock());
    return node;
}

//  return expr? ;
ASTNodePtr Parser::parseReturnStmt() {
    int line = peek().line;
    advance(); // consume 'return'
    auto node = makeNode(NodeType::RETURN_STMT, line);
    if (!check(TokenType::SEMICOLON)) {
        node->addChild(parseExpr());
    }
    expect(TokenType::SEMICOLON, "Expected ';' after return");
    return node;
}


//  Expression statement:  expr ;
ASTNodePtr Parser::parseExprStmt() {
    int line = peek().line;
    auto node = makeNode(NodeType::EXPR_STMT, line);
    node->addChild(parseExpr());
    expect(TokenType::SEMICOLON, "Expected ';' after expression");
    return node;
}

//  Expression → assignment
ASTNodePtr Parser::parseExpr() {
    return parseAssignment();
}


//  Assignment: IDENT op= expr  |  logicOr
ASTNodePtr Parser::parseAssignment() {
    // Check for assignment: IDENT followed by assignment operator
    if (check(TokenType::IDENTIFIER)) {
        TokenType next = peek(1).type;
        if (next == TokenType::OP_ASSIGN      ||
            next == TokenType::OP_PLUS_ASSIGN  ||
            next == TokenType::OP_MINUS_ASSIGN ||
            next == TokenType::OP_MUL_ASSIGN   ||
            next == TokenType::OP_DIV_ASSIGN) {

            Token nameTok = advance();            // consume IDENT
            Token opTok   = advance();            // consume operator
            auto  node    = makeNode(NodeType::ASSIGN, nameTok.line, opTok.value);
            node->addChild(makeNode(NodeType::IDENTIFIER, nameTok.line, nameTok.value));
            node->addChild(parseAssignment());    // right-associative
            return node;
        }
    }
    return parseLogicOr();
}

//  logicOr → logicAnd ('||' logicAnd)*
ASTNodePtr Parser::parseLogicOr() {
    auto left = parseLogicAnd();
    while (check(TokenType::OP_OR)) {
        Token op = advance();
        auto node = makeNode(NodeType::BINARY_OP, op.line, "||");
        node->addChild(std::move(left));
        node->addChild(parseLogicAnd());
        left = std::move(node);
    }
    return left;
}

//  logicAnd → equality ('&&' equality)*
ASTNodePtr Parser::parseLogicAnd() {
    auto left = parseEquality();
    while (check(TokenType::OP_AND)) {
        Token op = advance();
        auto node = makeNode(NodeType::BINARY_OP, op.line, "&&");
        node->addChild(std::move(left));
        node->addChild(parseEquality());
        left = std::move(node);
    }
    return left;
}


//  equality → relational (('=='|'!=') relational)*
ASTNodePtr Parser::parseEquality() {
    auto left = parseRelational();
    while (check(TokenType::OP_EQ) || check(TokenType::OP_NEQ)) {
        Token op = advance();
        auto node = makeNode(NodeType::BINARY_OP, op.line, op.value);
        node->addChild(std::move(left));
        node->addChild(parseRelational());
        left = std::move(node);
    }
    return left;
}

//  relational → additive (('<'|'>'|'<='|'>=') additive)*
ASTNodePtr Parser::parseRelational() {
    auto left = parseAdditive();
    while (check(TokenType::OP_LT)  || check(TokenType::OP_GT) ||
           check(TokenType::OP_LEQ) || check(TokenType::OP_GEQ)) {
        Token op = advance();
        auto node = makeNode(NodeType::BINARY_OP, op.line, op.value);
        node->addChild(std::move(left));
        node->addChild(parseAdditive());
        left = std::move(node);
    }
    return left;
}

//  additive → multiplicative (('+' | '-') multiplicative)*
ASTNodePtr Parser::parseAdditive() {
    auto left = parseMultiplicative();
    while (check(TokenType::OP_PLUS) || check(TokenType::OP_MINUS)) {
        Token op = advance();
        auto node = makeNode(NodeType::BINARY_OP, op.line, op.value);
        node->addChild(std::move(left));
        node->addChild(parseMultiplicative());
        left = std::move(node);
    }
    return left;
}


//  multiplicative → unary (('*'|'/'|'%') unary)*
ASTNodePtr Parser::parseMultiplicative() {
    auto left = parseUnary();
    while (check(TokenType::OP_MULTIPLY) ||
           check(TokenType::OP_DIVIDE)   ||
           check(TokenType::OP_MODULO)) {
        Token op = advance();
        auto node = makeNode(NodeType::BINARY_OP, op.line, op.value);
        node->addChild(std::move(left));
        node->addChild(parseUnary());
        left = std::move(node);
    }
    return left;
}

//  unary → ('!'|'-'|'++'|'--') unary | postfix
ASTNodePtr Parser::parseUnary() {
if (check(TokenType::OP_NOT)     ||
    check(TokenType::OP_MINUS)   ||
    check(TokenType::OP_ADDRESS) ||
    check(TokenType::OP_INC)     ||
    check(TokenType::OP_DEC)) {
        Token op = advance();
        auto node = makeNode(NodeType::UNARY_OP, op.line, "pre:" + op.value);
        node->addChild(parseUnary());
        return node;
    }
    return parsePostfix();
}
//  postfix → primary ('++'|'--')?
ASTNodePtr Parser::parsePostfix() {
    auto operand = parsePrimary();
    if (check(TokenType::OP_INC) || check(TokenType::OP_DEC)) {
        Token op = advance();
        auto node = makeNode(NodeType::UNARY_OP, op.line, "post:" + op.value);
        node->addChild(std::move(operand));
        return node;
    }
    return operand;
}


//  primary → literal | funcCall | ident | '(' expr ')'

ASTNodePtr Parser::parsePrimary() {
    Token t = peek();

    // Integer literal
    if (t.type == TokenType::LIT_INT) {
        advance();
        return makeNode(NodeType::LITERAL_INT, t.line, t.value);
    }
    // Float literal
    if (t.type == TokenType::LIT_FLOAT) {
        advance();
        return makeNode(NodeType::LITERAL_FLOAT, t.line, t.value);
    }
    // Char literal
    if (t.type == TokenType::LIT_CHAR) {
        advance();
        return makeNode(NodeType::LITERAL_CHAR, t.line, t.value);
    }
    // String literal
    if (t.type == TokenType::LIT_STRING) {
        advance();
        return makeNode(NodeType::LITERAL_STRING, t.line, t.value);
    }
    // Identifier or function call
    if (t.type == TokenType::IDENTIFIER) {
        advance();
        if (check(TokenType::LPAREN)) {
            // function call: name '(' argList ')'
            advance(); // consume '('
            auto node = makeNode(NodeType::FUNC_CALL, t.line, t.value);
            if (!check(TokenType::RPAREN)) {
                do {
                    node->addChild(parseExpr());
                } while (match(TokenType::COMMA));
            }
            expect(TokenType::RPAREN, "Expected ')' after function arguments");
            return node;
        }
        return makeNode(NodeType::IDENTIFIER, t.line, t.value);
    }
    // Grouped expression: '(' expr ')'
    if (t.type == TokenType::LPAREN) {
        advance();
        auto inner = parseExpr();
        expect(TokenType::RPAREN, "Expected ')' after grouped expression");
        return inner;
    }

    error("Unexpected token '" + t.value + "'");
    advance(); // skip bad token
    return makeNode(NodeType::LITERAL_INT, t.line, "0"); // dummy
}
