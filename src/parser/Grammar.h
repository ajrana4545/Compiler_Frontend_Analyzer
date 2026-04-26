#pragma once
#include <string>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>

// ─── Grammar Types ────────────────────────────────────────────────────────────
static const std::string EPSILON = "ε";
static const std::string END_MARKER = "$";

struct Production {
    std::string              lhs;
    std::vector<std::string> rhs; // empty rhs = epsilon
};

class Grammar {
public:
    // Load grammar from text (one rule per line: "A -> B C | D E | ε")
    bool loadFromString(const std::string& text);
    bool loadFromFile(const std::string& path);

    // Grammar transformations
    void removeLeftRecursion();
    void leftFactor();

    // Validation
    bool isLL1Compatible() const;
    bool validate(std::vector<std::string>& errors) const;

    // FIRST / FOLLOW
    void computeFirst();
    void computeFollow();

    const std::set<std::string>& first(const std::string& sym) const;
    const std::set<std::string>& follow(const std::string& sym) const;
    std::set<std::string> firstOfSequence(const std::vector<std::string>& seq) const;

    // Accessors
    const std::vector<Production>& productions() const { return _prods; }
    const std::vector<std::string>& nonTerminals() const { return _nonTerminals; }
    const std::set<std::string>&    terminals()    const { return _terminals; }
    const std::string&              startSymbol()  const { return _start; }

    bool isNonTerminal(const std::string& s) const;
    bool isTerminal(const std::string& s) const;
    bool isNullable(const std::string& s) const;

    // Pretty print
    void print() const;
    void printFirstFollow() const;

private:
    std::vector<Production>    _prods;
    std::vector<std::string>   _nonTerminals;  // ordered list (preserves insertion order)
    std::set<std::string>      _terminals;
    std::string                _start;

    std::unordered_map<std::string, std::set<std::string>> _first;
    std::unordered_map<std::string, std::set<std::string>> _follow;
    std::unordered_set<std::string>                        _nullable;

    void _updateNonTerminals();
    void _computeNullable();
    bool _parseLine(const std::string& line);
};
