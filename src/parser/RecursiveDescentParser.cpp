#include "RecursiveDescentParser.h"
#include "../lexer/Token.h"
#include "../utils/Timer.h"
#include <iostream>
#include <sstream>
#include <cctype>

// ─── Helpers ──────────────────────────────────────────────────────────────────
const std::string& RecursiveDescentParser::cur() const {
    static const std::string eof = "EOF";
    return (_pos < _tokens.size()) ? _tokens[_pos] : eof;
}
const std::string& RecursiveDescentParser::peek(int offset) const {
    static const std::string eof = "EOF";
    size_t p = _pos + (size_t)offset;
    return (p < _tokens.size()) ? _tokens[p] : eof;
}

bool RecursiveDescentParser::at(const std::string& tok) const { return cur() == tok; }

bool RecursiveDescentParser::consume(const std::string& expected) {
    ++_steps;
    if (cur() == expected) {
        _log.push_back("  [" + std::to_string(_steps) + "] MATCH '" + expected + "'");
        ++_pos;
        return true;
    }
    std::string msg = "  [" + std::to_string(_steps) + "] ERROR: expected '"
                    + expected + "', got '" + cur() + "'";
    _log.push_back(msg);
    _errors.push_back(msg);
    return false;
}

void RecursiveDescentParser::syncTo(const std::vector<std::string>& sync) {
    while (!at("EOF")) {
        for (auto& s : sync) if (at(s)) return;
        ++_pos;
    }
}

// ─── Grammar rules ────────────────────────────────────────────────────────────
bool RecursiveDescentParser::parseProgram() {
    _log.push_back("► program");
    return parseDeclList();
}

bool RecursiveDescentParser::parseDeclList() {
    _log.push_back("► decl_list");
    while (at("int") || at("float") || at("void") || at("char")) {
        size_t before = _pos;
        if (!parseDecl()) { syncTo({"int","float","void","char","EOF"}); }
        if (_pos == before) ++_pos; // safety advance
    }
    return true;
}

bool RecursiveDescentParser::parseDecl() {
    _log.push_back("► decl");
    if (!parseType()) return false;
    bool ok = consume("ID");
    if (at("(")) {
        consume("("); parseParams(); consume(")");
        consume("{"); parseStmtList(); consume("}");
    } else {
        consume(";");
    }
    return ok;
}

bool RecursiveDescentParser::parseType() {
    _log.push_back("► type");
    if (at("int"))   { ++_steps; _log.push_back("  ["+std::to_string(_steps)+"] MATCH 'int'");   ++_pos; return true; }
    if (at("float")) { ++_steps; _log.push_back("  ["+std::to_string(_steps)+"] MATCH 'float'"); ++_pos; return true; }
    if (at("void"))  { ++_steps; _log.push_back("  ["+std::to_string(_steps)+"] MATCH 'void'");  ++_pos; return true; }
    if (at("char"))  { ++_steps; _log.push_back("  ["+std::to_string(_steps)+"] MATCH 'char'");  ++_pos; return true; }
    std::string msg = "ERROR: expected type specifier, got '" + cur() + "'";
    _log.push_back(msg); _errors.push_back(msg);
    return false;
}

bool RecursiveDescentParser::parseParams() {
    _log.push_back("► params");
    if (at(")")) return true;
    if (!parseType()) return true; // empty params
    consume("ID");
    return parseParamList();
}

bool RecursiveDescentParser::parseParamList() {
    _log.push_back("► param_list");
    while (at(",")) {
        consume(","); parseType(); consume("ID");
    }
    return true;
}

bool RecursiveDescentParser::parseStmtList() {
    _log.push_back("► stmt_list");
    while (!at("}") && !at("EOF")) {
        size_t prevPos = _pos;
        parseStmt();
        if (_pos == prevPos) {
            // Didn't consume anything — skip to avoid infinite loop
            _log.push_back("  [skip] unknown token '" + cur() + "'");
            ++_pos;
        }
    }
    return true;
}

bool RecursiveDescentParser::parseStmt() {
    _log.push_back("► stmt");
    if (at("{")) {
        consume("{"); parseStmtList(); consume("}");
    } else if (at("if")) {
        consume("if"); consume("("); parseExpr(); consume(")");
        parseStmt();
        if (at("else")) { consume("else"); parseStmt(); }
    } else if (at("while")) {
        consume("while"); consume("("); parseExpr(); consume(")"); parseStmt();
    } else if (at("for")) {
        consume("for"); consume("(");
        parseExpr(); consume(";"); parseExpr(); consume(";"); parseExpr();
        consume(")"); parseStmt();
    } else if (at("return")) {
        consume("return");
        if (!at(";")) parseExpr();
        consume(";");
    } else if (at("int") || at("float") || at("char") || at("void")) {
        // local variable declaration
        parseType(); consume("ID");
        if (at("=")) { consume("="); parseExpr(); }
        consume(";");
    } else {
        parseExpr(); consume(";");
    }
    return true;
}

bool RecursiveDescentParser::parseExpr() {
    _log.push_back("► expr");
    // Lookahead: ID = expr
    if (at("ID") && peek(1) == "=") {
        consume("ID"); consume("="); return parseExpr();
    }
    bool ok = parseAddExpr();
    // Handle relational operators
    while (at("==") || at("!=") || at("<") || at(">") || at("<=") || at(">=")) {
        std::string op = cur();
        ++_steps;
        _log.push_back("  [" + std::to_string(_steps) + "] MATCH '" + op + "'");
        ++_pos;
        parseAddExpr();
    }
    return ok;
}

bool RecursiveDescentParser::parseAddExpr() {
    _log.push_back("► add_expr");
    if (!parseMulExpr()) return false;
    return parseAddExprPrime();
}

bool RecursiveDescentParser::parseAddExprPrime() {
    _log.push_back("► add_expr'");
    while (at("+") || at("-")) {
        std::string op = cur();
        ++_steps;
        _log.push_back("  [" + std::to_string(_steps) + "] MATCH '" + op + "'");
        ++_pos;
        parseMulExpr();
    }
    return true;
}

bool RecursiveDescentParser::parseMulExpr() {
    _log.push_back("► mul_expr");
    if (!parseUnary()) return false;
    return parseMulExprPrime();
}

bool RecursiveDescentParser::parseMulExprPrime() {
    _log.push_back("► mul_expr'");
    while (at("*") || at("/")) {
        std::string op = cur();
        ++_steps;
        _log.push_back("  [" + std::to_string(_steps) + "] MATCH '" + op + "'");
        ++_pos;
        parseUnary();
    }
    return true;
}

bool RecursiveDescentParser::parseUnary() {
    _log.push_back("► unary");
    if (at("(")) {
        consume("(");
        parseExpr();
        return consume(")");
    }
    if (at("ID")) {
        ++_steps;
        _log.push_back("  [" + std::to_string(_steps) + "] MATCH 'ID' (" + cur() + ")");
        ++_pos;
        // Function call
        if (at("(")) {
            consume("(");
            while (!at(")") && !at("EOF")) {
                parseExpr();
                if (at(",")) consume(",");
                else break;
            }
            consume(")");
        }
        return true;
    }
    if (at("NUM")) {
        ++_steps; _log.push_back("  [" + std::to_string(_steps) + "] MATCH 'NUM'"); ++_pos; return true;
    }
    // Try numeric literal by content
    if (!cur().empty() && cur() != "EOF") {
        char c = cur()[0];
        if (std::isdigit((unsigned char)c)) {
            ++_steps; _log.push_back("  [" + std::to_string(_steps) + "] MATCH NUM '" + cur() + "'"); ++_pos; return true;
        }
    }
    std::string msg = "ERROR: expected expression, got '" + cur() + "'";
    _log.push_back(msg); _errors.push_back(msg);
    return false;
}

// ─── Entry point ──────────────────────────────────────────────────────────────
ParseResult RecursiveDescentParser::parse(const std::vector<std::string>& tokens) {
    _tokens = tokens;
    _pos    = 0;
    _steps  = 0;
    _log.clear();
    _errors.clear();

    Timer t; t.start();
    parseProgram();
    if (!at("EOF")) {
        std::string msg = "ERROR: extra tokens after program, got '" + cur() + "'";
        _log.push_back(msg); _errors.push_back(msg);
    }
    t.stop();

    ParseResult r;
    r.success    = _errors.empty();
    r.steps      = _steps;
    r.errors     = (int)_errors.size();
    r.elapsedMs  = t.elapsedMs();
    r.memoryBytes= _tokens.size() * sizeof(std::string) + _log.size() * 64;
    r.log        = _log;
    r.errorMsgs  = _errors;
    return r;
}
