#include "Grammar.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <stdexcept>

// ─── Utility ──────────────────────────────────────────────────────────────────
static std::string trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    size_t b = s.find_last_not_of(" \t\r\n");
    return (a == std::string::npos) ? "" : s.substr(a, b - a + 1);
}

static std::vector<std::string> split(const std::string& s, const std::string& delim) {
    std::vector<std::string> parts;
    size_t start = 0, pos;
    while ((pos = s.find(delim, start)) != std::string::npos) {
        parts.push_back(trim(s.substr(start, pos - start)));
        start = pos + delim.size();
    }
    parts.push_back(trim(s.substr(start)));
    return parts;
}

static std::vector<std::string> tokenizeRHS(const std::string& rhs) {
    std::istringstream iss(rhs);
    std::vector<std::string> syms;
    std::string tok;
    while (iss >> tok) syms.push_back(tok);
    return syms;
}

// ─── Parsing ──────────────────────────────────────────────────────────────────
bool Grammar::_parseLine(const std::string& line) {
    // Expected format: LHS -> alt1 | alt2 | ε
    auto arrowPos = line.find("->");
    if (arrowPos == std::string::npos) return false;
    std::string lhs = trim(line.substr(0, arrowPos));
    std::string rhsPart = trim(line.substr(arrowPos + 2));
    if (lhs.empty()) return false;

    if (_start.empty()) _start = lhs;

    auto alts = split(rhsPart, "|");
    for (auto& alt : alts) {
        Production p;
        p.lhs = lhs;
        std::string altTrim = trim(alt);
        if (altTrim == EPSILON || altTrim.empty())
            p.rhs = {}; // epsilon
        else
            p.rhs = tokenizeRHS(altTrim);
        _prods.push_back(std::move(p));
    }
    return true;
}

bool Grammar::loadFromString(const std::string& text) {
    _prods.clear(); _nonTerminals.clear(); _terminals.clear(); _start.clear();
    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        if (!_parseLine(line)) {
            std::cerr << "[Grammar] Could not parse: " << line << "\n";
        }
    }
    _updateNonTerminals();
    return !_prods.empty();
}

bool Grammar::loadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) { std::cerr << "[Grammar] Cannot open " << path << "\n"; return false; }
    std::ostringstream oss; oss << f.rdbuf();
    return loadFromString(oss.str());
}

void Grammar::_updateNonTerminals() {
    std::unordered_set<std::string> seen;
    _nonTerminals.clear();
    _terminals.clear();
    // Collect all LHS as non-terminals in order of appearance
    for (auto& p : _prods)
        if (seen.insert(p.lhs).second)
            _nonTerminals.push_back(p.lhs);
    // Everything in RHS that is not a non-terminal is a terminal
    for (auto& p : _prods)
        for (auto& sym : p.rhs)
            if (seen.find(sym) == seen.end() && sym != EPSILON)
                _terminals.insert(sym);
}

bool Grammar::isNonTerminal(const std::string& s) const {
    for (auto& nt : _nonTerminals) if (nt == s) return true;
    return false;
}
bool Grammar::isTerminal(const std::string& s) const { return _terminals.count(s) > 0; }
bool Grammar::isNullable(const std::string& s) const { return _nullable.count(s) > 0; }

// ─── Nullable ─────────────────────────────────────────────────────────────────
void Grammar::_computeNullable() {
    _nullable.clear();
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& p : _prods) {
            if (_nullable.count(p.lhs)) continue;
            bool allNull = p.rhs.empty();
            if (!allNull) {
                allNull = true;
                for (auto& sym : p.rhs) {
                    if (!_nullable.count(sym)) { allNull = false; break; }
                }
            }
            if (allNull) { _nullable.insert(p.lhs); changed = true; }
        }
    }
}

// ─── FIRST ────────────────────────────────────────────────────────────────────
void Grammar::computeFirst() {
    _computeNullable();
    _first.clear();
    // Init: terminals map to themselves
    for (auto& t : _terminals) _first[t] = { t };
    _first[EPSILON] = { EPSILON };
    _first[END_MARKER] = { END_MARKER };
    for (auto& nt : _nonTerminals) _first[nt] = {};

    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& p : _prods) {
            auto& F = _first[p.lhs];
            size_t before = F.size();
            if (p.rhs.empty()) { F.insert(EPSILON); }
            else {
                bool allNullable = true;
                for (auto& sym : p.rhs) {
                    auto& fs = _first[sym];
                    for (auto& x : fs) if (x != EPSILON) F.insert(x);
                    if (!_nullable.count(sym)) { allNullable = false; break; }
                }
                if (allNullable) F.insert(EPSILON);
            }
            if (F.size() > before) changed = true;
        }
    }
}

std::set<std::string> Grammar::firstOfSequence(const std::vector<std::string>& seq) const {
    std::set<std::string> result;
    bool allNullable = true;
    for (auto& sym : seq) {
        auto it = _first.find(sym);
        if (it != _first.end()) {
            for (auto& x : it->second) if (x != EPSILON) result.insert(x);
            if (!_nullable.count(sym)) { allNullable = false; break; }
        } else {
            result.insert(sym); // terminal
            allNullable = false; break;
        }
    }
    if (allNullable) result.insert(EPSILON);
    return result;
}

const std::set<std::string>& Grammar::first(const std::string& sym) const {
    static const std::set<std::string> empty;
    auto it = _first.find(sym);
    return (it != _first.end()) ? it->second : empty;
}

// ─── FOLLOW ───────────────────────────────────────────────────────────────────
void Grammar::computeFollow() {
    _follow.clear();
    for (auto& nt : _nonTerminals) _follow[nt] = {};
    _follow[_start].insert(END_MARKER);

    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& p : _prods) {
            for (size_t i = 0; i < p.rhs.size(); ++i) {
                if (!isNonTerminal(p.rhs[i])) continue;
                auto& F = _follow[p.rhs[i]];
                size_t before = F.size();
                // Compute FIRST of β (rest of rhs after position i)
                std::vector<std::string> beta(p.rhs.begin() + i + 1, p.rhs.end());
                auto fb = firstOfSequence(beta);
                for (auto& x : fb) if (x != EPSILON) F.insert(x);
                if (fb.count(EPSILON) || beta.empty())
                    for (auto& x : _follow[p.lhs]) F.insert(x);
                if (F.size() > before) changed = true;
            }
        }
    }
}

const std::set<std::string>& Grammar::follow(const std::string& sym) const {
    static const std::set<std::string> empty;
    auto it = _follow.find(sym);
    return (it != _follow.end()) ? it->second : empty;
}

// ─── Remove Left Recursion ────────────────────────────────────────────────────
void Grammar::removeLeftRecursion() {
    // Algorithm: for each NT A, eliminate direct left recursion.
    // Indirect left recursion requires inter-NT substitution (handled with ordering).
    std::vector<Production> newProds;

    for (size_t i = 0; i < _nonTerminals.size(); ++i) {
        const std::string& A = _nonTerminals[i];
        // Substitute earlier NTs to handle indirect
        for (size_t j = 0; j < i; ++j) {
            const std::string& B = _nonTerminals[j];
            std::vector<Production> toAdd;
            for (auto& p : _prods) {
                if (p.lhs == A && !p.rhs.empty() && p.rhs[0] == B) {
                    // Replace A -> B α with A -> γ α for each B -> γ
                    for (auto& q : _prods) {
                        if (q.lhs == B) {
                            Production np;
                            np.lhs = A;
                            np.rhs = q.rhs;
                            for (size_t k = 1; k < p.rhs.size(); ++k)
                                np.rhs.push_back(p.rhs[k]);
                            toAdd.push_back(np);
                        }
                    }
                    // Remove original rule (mark lhs as empty temporarily)
                    // We'll rebuild below
                }
            }
        }

        // Collect direct left-recursive and non-left-recursive productions for A
        std::vector<std::vector<std::string>> alpha, beta;
        for (auto& p : _prods) {
            if (p.lhs != A) continue;
            if (!p.rhs.empty() && p.rhs[0] == A)
                alpha.push_back(std::vector<std::string>(p.rhs.begin() + 1, p.rhs.end()));
            else
                beta.push_back(p.rhs);
        }

        if (alpha.empty()) continue; // No direct left recursion

        // Remove all productions for A from _prods
        _prods.erase(std::remove_if(_prods.begin(), _prods.end(),
            [&](const Production& p) { return p.lhs == A; }), _prods.end());

        std::string A1 = A + "'";
        // Add A' to NT list
        _nonTerminals.insert(_nonTerminals.begin() + i + 1, A1);

        // A -> β A' for each β
        for (auto& b : beta) {
            Production p; p.lhs = A;
            p.rhs = b;
            p.rhs.push_back(A1);
            _prods.push_back(p);
        }
        if (beta.empty()) { // A was only left-recursive: add A -> A'
            Production p; p.lhs = A; p.rhs = { A1 }; _prods.push_back(p);
        }

        // A' -> α A' for each α
        for (auto& a : alpha) {
            Production p; p.lhs = A1;
            p.rhs = a;
            p.rhs.push_back(A1);
            _prods.push_back(p);
        }
        // A' -> ε
        Production ep; ep.lhs = A1; ep.rhs = {};
        _prods.push_back(ep);
    }
    _updateNonTerminals();
}

// ─── Left Factoring ───────────────────────────────────────────────────────────
void Grammar::leftFactor() {
    bool changed = true;
    int counter = 1;
    while (changed) {
        changed = false;
        std::vector<Production> result;
        for (auto& A : _nonTerminals) {
            // Group productions for A by first symbol
            std::map<std::string, std::vector<Production>> groups;
            std::vector<std::string> order;
            for (auto& p : _prods) {
                if (p.lhs != A) continue;
                std::string key = p.rhs.empty() ? EPSILON : p.rhs[0];
                if (groups.find(key) == groups.end()) order.push_back(key);
                groups[key].push_back(p);
            }
            for (auto& key : order) {
                auto& prods = groups[key];
                if (prods.size() == 1) {
                    result.push_back(prods[0]); continue;
                }
                // Factor out common prefix
                // Find longest common prefix
                size_t prefixLen = prods[0].rhs.size();
                for (auto& p : prods)
                    for (size_t k = 0; k < prefixLen && k < p.rhs.size(); ++k)
                        if (p.rhs[k] != prods[0].rhs[k]) { prefixLen = k; break; }
                if (prefixLen == 0) { for (auto& p : prods) result.push_back(p); continue; }

                std::string A1 = A + std::to_string(counter++);
                _nonTerminals.push_back(A1);

                Production factored; factored.lhs = A;
                factored.rhs = std::vector<std::string>(prods[0].rhs.begin(),
                                                        prods[0].rhs.begin() + prefixLen);
                factored.rhs.push_back(A1);
                result.push_back(factored);

                for (auto& p : prods) {
                    Production np; np.lhs = A1;
                    np.rhs = std::vector<std::string>(p.rhs.begin() + prefixLen, p.rhs.end());
                    result.push_back(np);
                }
                changed = true;
            }
        }
        for (auto& p : _prods)
            if (std::find_if(_nonTerminals.begin(), _nonTerminals.end(),
                [&](auto& n){ return n == p.lhs; }) == _nonTerminals.end())
                result.push_back(p);
        _prods = result;
    }
    _updateNonTerminals();
}

// ─── LL(1) compatibility check ────────────────────────────────────────────────
bool Grammar::isLL1Compatible() const {
    for (auto& A : _nonTerminals) {
        std::vector<Production> ps;
        for (auto& p : _prods) if (p.lhs == A) ps.push_back(p);
        for (size_t i = 0; i < ps.size(); ++i) {
            auto fi = firstOfSequence(ps[i].rhs);
            for (size_t j = i + 1; j < ps.size(); ++j) {
                auto fj = firstOfSequence(ps[j].rhs);
                // FIRST/FIRST conflict
                for (auto& x : fi) if (x != EPSILON && fj.count(x)) return false;
                // FIRST/FOLLOW conflict
                if (fi.count(EPSILON))
                    for (auto& x : follow(A)) if (fj.count(x)) return false;
            }
        }
    }
    return true;
}

bool Grammar::validate(std::vector<std::string>& errors) const {
    errors.clear();
    if (_prods.empty()) { errors.push_back("No productions defined."); return false; }
    // Check all RHS symbols are defined
    std::unordered_set<std::string> ntSet(_nonTerminals.begin(), _nonTerminals.end());
    for (auto& p : _prods)
        for (auto& sym : p.rhs)
            if (sym != EPSILON && !ntSet.count(sym) && !_terminals.count(sym))
                errors.push_back("Undefined symbol '" + sym + "' in rule for " + p.lhs);
    return errors.empty();
}

void Grammar::print() const {
    std::cout << "\n=== Grammar Productions ===\n";
    for (auto& p : _prods) {
        std::cout << "  " << p.lhs << " -> ";
        if (p.rhs.empty()) std::cout << EPSILON;
        else for (size_t i = 0; i < p.rhs.size(); ++i) {
            if (i) std::cout << ' ';
            std::cout << p.rhs[i];
        }
        std::cout << '\n';
    }
}

void Grammar::printFirstFollow() const {
    std::cout << "\n=== FIRST Sets ===\n";
    for (auto& nt : _nonTerminals) {
        std::cout << "  FIRST(" << nt << ") = { ";
        bool first = true;
        for (auto& x : _first.at(nt).empty() ? std::set<std::string>{} : _first.at(nt)) {
            if (!first) std::cout << ", "; first = false;
            std::cout << x;
        }
        std::cout << " }\n";
    }
    std::cout << "\n=== FOLLOW Sets ===\n";
    for (auto& nt : _nonTerminals) {
        std::cout << "  FOLLOW(" << nt << ") = { ";
        bool first = true;
        auto it = _follow.find(nt);
        if (it != _follow.end())
            for (auto& x : it->second) { if (!first) std::cout << ", "; first = false; std::cout << x; }
        std::cout << " }\n";
    }
}
