#pragma once
#include "Token.h"
#include "../utils/Timer.h"
#include <string>
#include <set>
#include <unordered_map>

// ─── Lexer Metrics ─────────────────────────────────────────────────────────
struct LexerMetrics {
    double   elapsedMs   = 0.0;
    int      tokenCount  = 0;
    int      errorCount  = 0;
    size_t   memoryBytes = 0;      // sizeof(Token) * tokenCount
    std::unordered_map<std::string, int> typeCounts; // type-name → count
};

// ─── DFA-based Lexer ──────────────────────────────────────────────────────
class DFALexer {
public:
    explicit DFALexer(const std::string& source);

    void setCustomKeywords(const std::set<std::string>& kw) { _customKw = kw; }
    TokenStream tokenize();
    const LexerMetrics& metrics() const { return _m; }

private:
    std::string  _src;
    size_t       _pos  = 0;
    int          _line = 1;
    int          _col  = 1;
    LexerMetrics _m;
    std::set<std::string> _customKw;

    char  peek(int offset = 0) const;
    char  advance();
    void  skipWhitespace();
    void  skipLineComment();
    void  skipBlockComment();

    Token readIdentifierOrKeyword();
    Token readNumber();
    Token readCharLiteral();
    Token readStringLiteral();
    Token readOperatorOrDelimiter();
};
