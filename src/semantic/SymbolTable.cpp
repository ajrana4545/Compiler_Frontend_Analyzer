#include "SymbolTable.h"
#include <iostream>
#include <iomanip>
#include <stdexcept>

void SymbolTable::enterScope() {
    ++_scope;
    if (_scope >= (int)_tables.size())
        _tables.emplace_back();
    else
        _tables[_scope].clear();
}

void SymbolTable::exitScope() {
    if (_scope < 0) return;
    _tables[_scope].clear();
    --_scope;
}

bool SymbolTable::insert(const Symbol& sym) {
    if (_scope < 0) return false;
    auto& tbl = _tables[_scope];
    if (tbl.count(sym.name)) return false; // redeclaration
    tbl[sym.name] = sym;
    return true;
}

const Symbol* SymbolTable::lookup(const std::string& name) const {
    for (int s = _scope; s >= 0; --s) {
        if (s < (int)_tables.size()) {
            auto it = _tables[s].find(name);
            if (it != _tables[s].end()) return &it->second;
        }
    }
    return nullptr;
}

const Symbol* SymbolTable::lookupCurrent(const std::string& name) const {
    if (_scope < 0 || _scope >= (int)_tables.size()) return nullptr;
    auto it = _tables[_scope].find(name);
    return (it != _tables[_scope].end()) ? &it->second : nullptr;
}

void SymbolTable::print() const {
    std::cout << "\n=== Symbol Table ===\n";
    std::cout << std::left
              << std::setw(20) << "Name"
              << std::setw(10) << "Type"
              << std::setw(8)  << "Scope"
              << std::setw(6)  << "Line"
              << "Function?\n";
    std::cout << std::string(60, '-') << '\n';
    for (int s = 0; s <= _scope; ++s) {
        if (s >= (int)_tables.size()) break;
        for (auto it2 = _tables[s].begin(); it2 != _tables[s].end(); ++it2) {
            const Symbol& sym = it2->second;
            std::cout << std::left
                      << std::setw(20) << sym.name
                      << std::setw(10) << sym.type
                      << std::setw(8)  << sym.scope
                      << std::setw(6)  << sym.line
                      << (sym.isFunc ? "yes" : "no") << '\n';
        }
    }
}
