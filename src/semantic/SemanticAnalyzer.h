#pragma once
#include "SymbolTable.h"
#include "../lexer/Token.h"
#include <vector>
#include <string>

// Semantic analysis result
struct SemanticResult {
    bool                     success = true;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    int                      passCount = 0; // 1 for single-pass, 2 for multi-pass
    double                   elapsedMs = 0.0;
};

// ─── Single Pass Analyzer ─────────────────────────────────────────────────────
// Walks the token stream in one pass, tracking declarations and uses.
class SinglePassAnalyzer {
public:
    SemanticResult analyze(const TokenStream& tokens);

private:
    SymbolTable     _st;
    std::vector<std::string> _errors;
    std::vector<std::string> _warnings;

    void reportError(const std::string& msg, int line);

    // Mini-C semantic rules
    std::string currentType;
    bool        inFunctionBody = false;
    std::string returnType;

    void processDeclarations(const TokenStream& ts);
    void checkUsages(const TokenStream& ts);
};

// ─── Multi Pass Analyzer ──────────────────────────────────────────────────────
// Pass 1: Collect all top-level declarations (forward refs).
// Pass 2: Type checking + detailed analysis.
class MultiPassAnalyzer {
public:
    SemanticResult analyze(const TokenStream& tokens);

private:
    SymbolTable              _st;
    std::vector<std::string> _errors;
    std::vector<std::string> _warnings;

    void reportError(const std::string& msg, int line);
    void pass1_collectDeclarations(const TokenStream& ts);
    void pass2_typeCheck(const TokenStream& ts);
};
