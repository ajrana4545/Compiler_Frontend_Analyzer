#include "RegexLexer.h"
#include "DFALexer.h"   // for tokenTypeName
#include "../utils/Timer.h"
#include <iostream>
#include <stdexcept>

RegexLexer::RegexLexer(const std::string& source) : _src(source) {
    buildPatterns();
}

void RegexLexer::buildPatterns() {
    using T = TokenType;
    // Order matters: longer/more-specific patterns first.
    auto add = [&](T t, const std::string& pat, const std::string& name) {
        _patterns.push_back({ t, std::regex(pat), name });
    };

    // Whitespace & comments (skip)
    add(T::COMMENT, R"(^\/\/[^\n]*)",                  "LINE_COMMENT");
    add(T::COMMENT, R"(^\/\*[\s\S]*?\*\/)",            "BLOCK_COMMENT");
    add(T::COMMENT, R"(^\s+)",                         "WHITESPACE");

    // Keywords (must come before IDENTIFIER)
    add(T::KW_INT,    R"(^int\b)",    "KW_INT");
    add(T::KW_FLOAT,  R"(^float\b)",  "KW_FLOAT");
    add(T::KW_CHAR,   R"(^char\b)",   "KW_CHAR");
    add(T::KW_VOID,   R"(^void\b)",   "KW_VOID");
    add(T::KW_IF,     R"(^if\b)",     "KW_IF");
    add(T::KW_ELSE,   R"(^else\b)",   "KW_ELSE");
    add(T::KW_WHILE,  R"(^while\b)",  "KW_WHILE");
    add(T::KW_FOR,    R"(^for\b)",    "KW_FOR");
    add(T::KW_RETURN, R"(^return\b)", "KW_RETURN");
    add(T::KW_STRUCT, R"(^struct\b)", "KW_STRUCT");

    // Literals
    add(T::FLOAT_LITERAL,  R"(^[0-9]+\.[0-9]+([eE][+-]?[0-9]+)?)", "FLOAT_LITERAL");
    add(T::INT_LITERAL,    R"(^[0-9]+)",                            "INT_LITERAL");
    add(T::CHAR_LITERAL,   R"(^'(?:\\.|[^'\\])*')",                 "CHAR_LITERAL");
    add(T::STRING_LITERAL, R"(^"(?:\\.|[^"\\])*")",                 "STRING_LITERAL");

    // Identifier
    add(T::IDENTIFIER, R"(^[a-zA-Z_][a-zA-Z0-9_]*)", "IDENTIFIER");

    // Two-char operators
    add(T::EQ,          R"(^==)",  "EQ");
    add(T::NEQ,         R"(^!=)",  "NEQ");
    add(T::LE,          R"(^<=)",  "LE");
    add(T::GE,          R"(^>=)",  "GE");
    add(T::AND,         R"(^&&)",  "AND");
    add(T::OR,          R"(^\|\|)","OR");
    add(T::LSHIFT,      R"(^<<)",  "LSHIFT");
    add(T::RSHIFT,      R"(^>>)",  "RSHIFT");
    add(T::INC,         R"(^\+\+)","INC");
    add(T::DEC,         R"(^--)",  "DEC");
    add(T::PLUS_ASSIGN, R"(^\+=)", "PLUS_ASSIGN");
    add(T::MINUS_ASSIGN,R"(^-=)",  "MINUS_ASSIGN");
    add(T::STAR_ASSIGN, R"(^\*=)", "STAR_ASSIGN");
    add(T::SLASH_ASSIGN,R"(^/=)",  "SLASH_ASSIGN");
    add(T::ARROW,       R"(^->)",  "ARROW");

    // Single-char operators & delimiters
    add(T::PLUS,    R"(^\+)", "PLUS");
    add(T::MINUS,   R"(^-)",  "MINUS");
    add(T::STAR,    R"(^\*)", "STAR");
    add(T::SLASH,   R"(^/)",  "SLASH");
    add(T::PERCENT, R"(^%)",  "PERCENT");
    add(T::ASSIGN,  R"(^=)",  "ASSIGN");
    add(T::LT,      R"(^<)",  "LT");
    add(T::GT,      R"(^>)",  "GT");
    add(T::NOT,     R"(^!)",  "NOT");
    add(T::AMPERSAND,R"(^&)", "AMPERSAND");
    add(T::PIPE,    R"(^\|)", "PIPE");
    add(T::CARET,   R"(^\^)", "CARET");
    add(T::LPAREN,  R"(^\()", "LPAREN");
    add(T::RPAREN,  R"(^\))", "RPAREN");
    add(T::LBRACE,  R"(^\{)", "LBRACE");
    add(T::RBRACE,  R"(^\})", "RBRACE");
    add(T::LBRACKET,R"(^\[)", "LBRACKET");
    add(T::RBRACKET,R"(^\])", "RBRACKET");
    add(T::SEMICOLON,R"(^;)", "SEMICOLON");
    add(T::COLON,   R"(^:)",  "COLON");
    add(T::COMMA,   R"(^,)",  "COMMA");
    add(T::DOT,     R"(^\.)", "DOT");
}

TokenStream RegexLexer::tokenize() {
    Timer timer; timer.start();
    TokenStream ts;
    std::string remaining = _src;
    int line = 1, col = 1;

    while (!remaining.empty()) {
        bool matched = false;
        for (auto& lp : _patterns) {
            std::smatch m;
            if (std::regex_search(remaining, m, lp.pattern,
                                  std::regex_constants::match_continuous)) {
                std::string val = m[0].str();
                // Count newlines in match for line tracking
                int newlines = (int)std::count(val.begin(), val.end(), '\n');
                if (lp.type != TokenType::COMMENT) {
                    // Reclassify identifier as custom keyword if applicable
                    TokenType etype = lp.type;
                    if (etype == TokenType::IDENTIFIER && _customKw.count(val))
                        etype = TokenType::KW_CUSTOM;
                    ts.emplace_back(etype, val, line, col);
                    _m.typeCounts[tokenTypeName(etype)]++;
                }
                if (newlines > 0) { line += newlines; col = 1; }
                else              { col += (int)val.size(); }
                remaining = remaining.substr(val.size());
                matched = true;
                break;
            }
        }
        if (!matched) {
            std::cerr << "[RegexLexer] Unknown token '" << remaining[0]
                      << "' at line " << line << "\n";
            ++_m.errorCount;
            remaining = remaining.substr(1);
            ++col;
        }
    }

    ts.emplace_back(TokenType::EOF_TOK, "EOF", line, col);
    timer.stop();
    _m.elapsedMs   = timer.elapsedMs();
    _m.tokenCount  = (int)ts.size() - 1;
    _m.memoryBytes = ts.size() * sizeof(Token);
    return ts;
}
