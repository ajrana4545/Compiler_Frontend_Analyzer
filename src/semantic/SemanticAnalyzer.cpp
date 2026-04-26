#include "SemanticAnalyzer.h"
#include "../utils/Timer.h"
#include <iostream>
#include <algorithm>
#include <cctype>

// ─── Helpers ──────────────────────────────────────────────────────────────────
static bool isTypeTok(const Token& t) {
    return t.type == TokenType::KW_INT   || t.type == TokenType::KW_FLOAT ||
           t.type == TokenType::KW_CHAR  || t.type == TokenType::KW_VOID;
}
static std::string typeStr(const Token& t) {
    if (t.type == TokenType::KW_INT)   return "int";
    if (t.type == TokenType::KW_FLOAT) return "float";
    if (t.type == TokenType::KW_CHAR)  return "char";
    if (t.type == TokenType::KW_VOID)  return "void";
    return t.value;
}

// ─── SinglePassAnalyzer ───────────────────────────────────────────────────────
void SinglePassAnalyzer::reportError(const std::string& msg, int line) {
    _errors.push_back("[Line " + std::to_string(line) + "] " + msg);
}

SemanticResult SinglePassAnalyzer::analyze(const TokenStream& ts) {
    Timer timer; timer.start();
    _st = SymbolTable();
    _errors.clear(); _warnings.clear();
    _st.enterScope(); // global scope (depth 0)

    int braceDepth = 0; // tracks { } nesting across the whole file

    for (size_t i = 0; i < ts.size(); ++i) {
        const Token& tok = ts[i];

        // ── Brace-based scope tracking ──────────────────────────────────────
        if (tok.type == TokenType::LBRACE) {
            ++braceDepth;
            if (braceDepth == 1) {
                // Top-level brace = entering a function body
                _st.enterScope();
            }
        }
        if (tok.type == TokenType::RBRACE) {
            if (braceDepth == 1) {
                // Leaving function body
                _st.exitScope();
            }
            if (braceDepth > 0) --braceDepth;
        }

        // ── Variable / function declaration ────────────────────────────────
        if (isTypeTok(tok)) {
            std::string declType = typeStr(tok);
            if (i + 1 < ts.size() && ts[i+1].type == TokenType::IDENTIFIER) {
                std::string name = ts[i+1].value;
                int ln = ts[i+1].line;
                bool isFunc = (i + 2 < ts.size() && ts[i+2].type == TokenType::LPAREN);
                Symbol sym;
                sym.name   = name;
                sym.type   = declType;
                sym.scope  = _st.currentScope();
                sym.line   = ln;
                sym.isFunc = isFunc;
                if (!_st.insert(sym))
                    reportError("Redeclaration of '" + name + "'", ln);
            }
        }

        // ── Identifier usage ────────────────────────────────────────────────
        if (tok.type == TokenType::IDENTIFIER) {
            bool isDecl = (i > 0 && isTypeTok(ts[i-1]));
            if (!isDecl) {
                if (!_st.lookup(tok.value))
                    reportError("Undeclared identifier '" + tok.value + "'", tok.line);
            }
        }

        // ── Type mismatch: float literal → int variable ────────────────────
        if (tok.type == TokenType::ASSIGN && i >= 1 && i + 1 < ts.size()) {
            const Token& lhsTok = ts[i-1];
            const Token& rhsTok = ts[i+1];
            if (lhsTok.type == TokenType::IDENTIFIER) {
                const Symbol* sym = _st.lookup(lhsTok.value);
                if (sym) {
                    if (sym->type == "int" && rhsTok.type == TokenType::FLOAT_LITERAL)
                        _warnings.push_back("[Line " + std::to_string(tok.line)
                            + "] Precision loss: assigning float to int '" + sym->name + "'");
                    if (sym->type == "void")
                        reportError("Cannot assign to void '" + lhsTok.value + "'", tok.line);
                }
            }
        }
    }

    _st.exitScope(); // global scope
    timer.stop();

    SemanticResult r;
    r.success   = _errors.empty();
    r.errors    = _errors;
    r.warnings  = _warnings;
    r.passCount = 1;
    r.elapsedMs = timer.elapsedMs();
    return r;
}

// ─── MultiPassAnalyzer ────────────────────────────────────────────────────────
void MultiPassAnalyzer::reportError(const std::string& msg, int line) {
    _errors.push_back("[Line " + std::to_string(line) + "] " + msg);
}

void MultiPassAnalyzer::pass1_collectDeclarations(const TokenStream& ts) {
    _st.enterScope(); // global scope
    for (size_t i = 0; i < ts.size(); ++i) {
        if (isTypeTok(ts[i]) && i + 1 < ts.size() && ts[i+1].type == TokenType::IDENTIFIER) {
            // Only global-level (braceDepth == 0)
            Symbol sym;
            sym.name   = ts[i+1].value;
            sym.type   = typeStr(ts[i]);
            sym.scope  = 0;
            sym.line   = ts[i+1].line;
            sym.isFunc = (i + 2 < ts.size() && ts[i+2].type == TokenType::LPAREN);
            _st.insert(sym); // ignore redecl in pass 1
        }
    }
}

void MultiPassAnalyzer::pass2_typeCheck(const TokenStream& ts) {
    int braceDepth = 0;

    for (size_t i = 0; i < ts.size(); ++i) {
        const Token& tok = ts[i];

        if (tok.type == TokenType::LBRACE) {
            ++braceDepth;
            if (braceDepth == 1) _st.enterScope(); // function body
        }
        if (tok.type == TokenType::RBRACE) {
            if (braceDepth == 1) _st.exitScope();
            if (braceDepth > 0) --braceDepth;
        }

        // Declarations inside function bodies
        if (braceDepth > 0 && isTypeTok(tok)
            && i+1 < ts.size() && ts[i+1].type == TokenType::IDENTIFIER) {
            bool isFunc = (i + 2 < ts.size() && ts[i+2].type == TokenType::LPAREN);
            if (!isFunc) {
                Symbol sym;
                sym.name  = ts[i+1].value;
                sym.type  = typeStr(tok);
                sym.scope = _st.currentScope();
                sym.line  = ts[i+1].line;
                sym.isFunc= false;
                if (!_st.insert(sym))
                    reportError("Redeclaration of local variable '" + sym.name + "'", sym.line);
            }
        }

        // Identifier usage
        if (tok.type == TokenType::IDENTIFIER) {
            bool isDecl = (i > 0 && isTypeTok(ts[i-1]));
            if (!isDecl && !_st.lookup(tok.value))
                reportError("Use of undeclared identifier '" + tok.value + "'", tok.line);
        }

        // Type mismatch: float literal to int
        if (tok.type == TokenType::ASSIGN && i >= 1 && i+1 < ts.size()) {
            const Token& lhs = ts[i-1];
            const Token& rhs = ts[i+1];
            if (lhs.type == TokenType::IDENTIFIER) {
                const Symbol* sym = _st.lookup(lhs.value);
                if (sym && sym->type == "int" && rhs.type == TokenType::FLOAT_LITERAL)
                    reportError("Type mismatch: assigning float to int '" + sym->name + "'", tok.line);
            }
        }
    }
}

SemanticResult MultiPassAnalyzer::analyze(const TokenStream& ts) {
    Timer timer; timer.start();
    _st = SymbolTable();
    _errors.clear(); _warnings.clear();

    pass1_collectDeclarations(ts);
    pass2_typeCheck(ts);
    _st.exitScope(); // global scope

    timer.stop();
    SemanticResult r;
    r.success   = _errors.empty();
    r.errors    = _errors;
    r.warnings  = _warnings;
    r.passCount = 2;
    r.elapsedMs = timer.elapsedMs();
    return r;
}
