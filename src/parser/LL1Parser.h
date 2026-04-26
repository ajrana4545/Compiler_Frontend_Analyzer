#pragma once
#include "Grammar.h"
#include "ParserTypes.h"
#include <map>
#include <string>
#include <vector>
#include <utility>

// LL(1) Parser
// Requires a Grammar object with FIRST/FOLLOW computed.
class LL1Parser {
public:
    struct TableCell {
        Production prod;
        bool hasConflict = false;
    };

    using ParseTable = std::map<std::string, std::map<std::string, TableCell>>;

    explicit LL1Parser(Grammar& g);

    void setSilent(bool s) { _silent = s; }
    void buildTable();
    ParseResult parse(const std::vector<std::string>& tokens);
    void printTable() const;

    bool hasConflicts() const { return _conflictCount > 0; }
    int  conflictCount() const { return _conflictCount; }

private:
    Grammar&   _g;
    ParseTable _table;
    int        _conflictCount = 0;
    bool       _silent = false;
};
