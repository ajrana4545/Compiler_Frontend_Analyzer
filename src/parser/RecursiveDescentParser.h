#pragma once
#include "Grammar.h"
#include "ParserTypes.h"
#include <string>
#include <vector>

// ─── Recursive Descent Parser ─────────────────────────────────────────────────
class RecursiveDescentParser {
public:
    // Uses the Mini-C grammar hardcoded (tokens are Mini-C token types)
    // tokens: vector of token strings (type names or values)
    ParseResult parse(const std::vector<std::string>& tokens);

private:
    std::vector<std::string> _tokens;
    size_t _pos;
    int    _steps;
    std::vector<std::string> _log;
    std::vector<std::string> _errors;

    const std::string& cur() const;
    const std::string& peek(int offset = 0) const;
    bool consume(const std::string& expected);
    bool at(const std::string& tok) const;
    void syncTo(const std::vector<std::string>& sync);

    // Grammar rules (Mini-C subset)
    bool parseProgram();
    bool parseDeclList();
    bool parseDecl();
    bool parseType();
    bool parseParams();
    bool parseParamList();
    bool parseStmtList();
    bool parseStmt();
    bool parseExpr();
    bool parseAddExpr();
    bool parseAddExprPrime();
    bool parseMulExpr();
    bool parseMulExprPrime();
    bool parseUnary();
};
