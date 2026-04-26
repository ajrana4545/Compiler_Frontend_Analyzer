#pragma once
#include <string>
#include <vector>
#include <unordered_map>

// ─── Symbol ───────────────────────────────────────────────────────────────────
struct Symbol {
    std::string name;
    std::string type;        // "int", "float", "char", "void", etc.
    int         scope    = 0;
    int         line     = -1;
    bool        isFunc   = false;
    std::vector<std::string> paramTypes;
};

// ─── SymbolTable ──────────────────────────────────────────────────────────────
class SymbolTable {
public:
    void enterScope();
    void exitScope();
    int  currentScope() const { return _scope; }

    bool insert(const Symbol& sym);          // false if already declared in current scope
    const Symbol* lookup(const std::string& name) const;  // walks up scopes
    const Symbol* lookupCurrent(const std::string& name) const;

    void print() const;

private:
    // Each scope level maps name → Symbol
    std::vector<std::unordered_map<std::string, Symbol>> _tables;
    int _scope = -1;
};
