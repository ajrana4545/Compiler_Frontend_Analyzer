#pragma once
#include "Grammar.h"
#include "ParserTypes.h"
#include <map>
#include <set>
#include <string>
#include <vector>
#include <utility>

// ─── LR(0) Item ───────────────────────────────────────────────────────────────
struct LRItem {
    int    prodIdx;  // index into Grammar::productions()
    int    dot;      // dot position in rhs

    bool operator<(const LRItem& o) const {
        return prodIdx < o.prodIdx || (prodIdx == o.prodIdx && dot < o.dot);
    }
    bool operator==(const LRItem& o) const {
        return prodIdx == o.prodIdx && dot == o.dot;
    }
};

using ItemSet = std::set<LRItem>;

// ─── SLR(1) Parser ────────────────────────────────────────────────────────────
class SLR1Parser {
public:
    struct Action {
        enum Kind { SHIFT, REDUCE, ACCEPT, ERROR } kind = ERROR;
        int value = -1; // state# for SHIFT, prod# for REDUCE
    };

    explicit SLR1Parser(Grammar& g);

    void setSilent(bool s) { _silent = s; }
    void buildCanonical();     // build LR(0) collection
    void buildTable();         // build SLR action/goto tables
    ParseResult parse(const std::vector<std::string>& tokens);
    void printTable() const;

    bool hasConflicts() const  { return _srConflicts + _rrConflicts > 0; }
    int  srConflicts()  const  { return _srConflicts; }
    int  rrConflicts()  const  { return _rrConflicts; }

private:
    Grammar& _g;

    std::vector<ItemSet>             _states;      // canonical collection
    std::map<int, std::map<std::string, int>>      _gotoTable;
    std::map<int, std::map<std::string, Action>>   _actionTable;
    int _srConflicts = 0;
    int _rrConflicts = 0;
    bool _silent = false;

    ItemSet  closure(const ItemSet& items) const;
    ItemSet  gotoSet(const ItemSet& I, const std::string& X) const;
    int      stateIndex(const ItemSet& s) const;

    std::string dotSymbol(const LRItem& item) const; // symbol after dot
    bool        dotAtEnd(const LRItem& item) const;
};
