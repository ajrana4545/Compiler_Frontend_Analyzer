#pragma once
#include "Token.h"
#include "../utils/Timer.h"
#include <string>
#include <vector>
#include <set>
#include <regex>
#include <unordered_map>

struct LexerMetrics; // forward – defined in DFALexer.h, shared via common header

// Pattern descriptor used by the regex lexer.
struct LexPattern {
    TokenType   type;
    std::regex  pattern;
    std::string name; // for display
};

// ─── Regex-based Lexer ────────────────────────────────────────────────────
class RegexLexer {
public:
    explicit RegexLexer(const std::string& source);

    void setCustomKeywords(const std::set<std::string>& kw) { _customKw = kw; }
    TokenStream tokenize();

    struct Metrics {
        double   elapsedMs   = 0.0;
        int      tokenCount  = 0;
        int      errorCount  = 0;
        size_t   memoryBytes = 0;
        std::unordered_map<std::string, int> typeCounts;
    };

    const Metrics& metrics() const { return _m; }

private:
    std::string          _src;
    std::vector<LexPattern> _patterns;
    Metrics              _m;
    std::set<std::string>   _customKw;

    void buildPatterns();
};
