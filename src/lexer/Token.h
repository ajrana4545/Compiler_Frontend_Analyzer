#pragma once
#include <string>
#include <vector>
#include <unordered_map>

// ─── Token Types ─────────────────────────────────────────────────────────────
enum class TokenType {
    // Keywords
    KW_INT, KW_FLOAT, KW_CHAR, KW_VOID, KW_IF, KW_ELSE,
    KW_WHILE, KW_FOR, KW_RETURN, KW_STRUCT,
    // Identifiers / Literals
    IDENTIFIER, INT_LITERAL, FLOAT_LITERAL, CHAR_LITERAL, STRING_LITERAL,
    // Operators
    PLUS, MINUS, STAR, SLASH, PERCENT,
    ASSIGN, EQ, NEQ, LT, GT, LE, GE,
    AND, OR, NOT,
    AMPERSAND, PIPE, CARET, LSHIFT, RSHIFT,
    PLUS_ASSIGN, MINUS_ASSIGN, STAR_ASSIGN, SLASH_ASSIGN,
    INC, DEC,
    // Delimiters
    LPAREN, RPAREN, LBRACE, RBRACE, LBRACKET, RBRACKET,
    SEMICOLON, COLON, COMMA, DOT, ARROW,
    // Special
    EOF_TOK, UNKNOWN, COMMENT,
    KW_CUSTOM  // user-defined keyword loaded at runtime
};

// ─── Token Struct ─────────────────────────────────────────────────────────────
struct Token {
    TokenType type;
    std::string value;
    int         line;
    int         col;

    Token(TokenType t, std::string v, int l, int c)
        : type(t), value(std::move(v)), line(l), col(c) {}
};

// ─── Helpers ──────────────────────────────────────────────────────────────────
std::string tokenTypeName(TokenType t);

inline const std::unordered_map<std::string, TokenType>& keywords() {
    static const std::unordered_map<std::string, TokenType> kw = {
        {"int",    TokenType::KW_INT},
        {"float",  TokenType::KW_FLOAT},
        {"char",   TokenType::KW_CHAR},
        {"void",   TokenType::KW_VOID},
        {"if",     TokenType::KW_IF},
        {"else",   TokenType::KW_ELSE},
        {"while",  TokenType::KW_WHILE},
        {"for",    TokenType::KW_FOR},
        {"return", TokenType::KW_RETURN},
        {"struct", TokenType::KW_STRUCT},
    };
    return kw;
}

using TokenStream = std::vector<Token>;
