#include "SLR1Parser.h"
#include "../utils/Timer.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stack>
#include <algorithm>

SLR1Parser::SLR1Parser(Grammar& g) : _g(g) {}

// ─── Dot helpers ─────────────────────────────────────────────────────────────
std::string SLR1Parser::dotSymbol(const LRItem& item) const {
    const auto& p = _g.productions()[item.prodIdx];
    if (item.dot < (int)p.rhs.size()) return p.rhs[item.dot];
    return "";
}

bool SLR1Parser::dotAtEnd(const LRItem& item) const {
    const auto& p = _g.productions()[item.prodIdx];
    return item.dot >= (int)p.rhs.size() ||
           (p.rhs.size() == 1 && p.rhs[0] == "ε");
}

// ─── Closure ─────────────────────────────────────────────────────────────────
ItemSet SLR1Parser::closure(const ItemSet& items) const {
    ItemSet result = items;
    bool changed = true;
    while (changed) {
        changed = false;
        ItemSet toAdd;
        for (auto& item : result) {
            std::string B = dotSymbol(item);
            if (B.empty() || !_g.isNonTerminal(B)) continue;
            const auto& prods = _g.productions();
            for (int i = 0; i < (int)prods.size(); ++i) {
                if (prods[i].lhs == B) {
                    LRItem ni{i, 0};
                    if (!result.count(ni)) { toAdd.insert(ni); changed = true; }
                }
            }
        }
        result.insert(toAdd.begin(), toAdd.end());
    }
    return result;
}

// ─── Goto ─────────────────────────────────────────────────────────────────────
ItemSet SLR1Parser::gotoSet(const ItemSet& I, const std::string& X) const {
    ItemSet J;
    for (auto& item : I) {
        if (dotSymbol(item) == X) {
            J.insert({item.prodIdx, item.dot + 1});
        }
    }
    return closure(J);
}

int SLR1Parser::stateIndex(const ItemSet& s) const {
    for (int i = 0; i < (int)_states.size(); ++i)
        if (_states[i] == s) return i;
    return -1;
}

// ─── Build Canonical LR(0) Collection ────────────────────────────────────────
void SLR1Parser::buildCanonical() {
    _states.clear();
    _gotoTable.clear();
    _actionTable.clear();

    const auto& prods = _g.productions();
    if (prods.empty()) return;

    // Start item: production 0, dot at 0
    ItemSet I0 = closure({{0, 0}});
    _states.push_back(I0);

    // Collect all grammar symbols
    std::vector<std::string> allSymbols;
    for (auto& nt : _g.nonTerminals()) allSymbols.push_back(nt);
    for (auto& t  : _g.terminals())    allSymbols.push_back(t);

    bool changed = true;
    while (changed) {
        changed = false;
        for (int i = 0; i < (int)_states.size(); ++i) {
            for (auto& X : allSymbols) {
                ItemSet J = gotoSet(_states[i], X);
                if (J.empty()) continue;
                int j = stateIndex(J);
                if (j < 0) {
                    j = (int)_states.size();
                    _states.push_back(J);
                    changed = true;
                }
                _gotoTable[i][X] = j;
            }
        }
    }
}

// ─── Build SLR Tables ─────────────────────────────────────────────────────────
void SLR1Parser::buildTable() {
    _srConflicts = _rrConflicts = 0;
    const auto& prods = _g.productions();

    auto setAction = [&](int state, const std::string& sym, Action a) {
        auto& cell = _actionTable[state][sym];
        if (cell.kind == Action::ERROR) { cell = a; return; }
        // Conflict
        if (cell.kind == Action::SHIFT && a.kind == Action::REDUCE)  { ++_srConflicts; if (!_silent) std::cerr << "[SLR] S/R conflict state " << state << " on '" << sym << "'\n"; }
        else if (cell.kind == Action::REDUCE && a.kind == Action::SHIFT)  { ++_srConflicts; if (!_silent) std::cerr << "[SLR] S/R conflict state " << state << " on '" << sym << "'\n"; }
        else if (cell.kind == Action::REDUCE && a.kind == Action::REDUCE) { ++_rrConflicts; if (!_silent) std::cerr << "[SLR] R/R conflict state " << state << " on '" << sym << "'\n"; }
    };

    for (int i = 0; i < (int)_states.size(); ++i) {
        for (auto& item : _states[i]) {
            const auto& p = prods[item.prodIdx];
            std::string B = dotSymbol(item);

            if (!B.empty() && B != "ε") {
                // Shift
                if (_gotoTable.count(i) && _gotoTable.at(i).count(B)) {
                    int j = _gotoTable.at(i).at(B);
                    if (_g.isTerminal(B) || B == "$")
                        setAction(i, B, {Action::SHIFT, j});
                }
            } else {
                // Reduce or Accept
                if (item.prodIdx == 0 && item.dot == (int)prods[0].rhs.size()) {
                    setAction(i, "$", {Action::ACCEPT, -1});
                } else {
                    for (auto& a : _g.follow(p.lhs))
                        setAction(i, a, {Action::REDUCE, item.prodIdx});
                }
            }
        }
    }
}

// ─── Parse ────────────────────────────────────────────────────────────────────
ParseResult SLR1Parser::parse(const std::vector<std::string>& tokens) {
    Timer timer; timer.start();
    ParseResult r;
    r.srConflicts = _srConflicts;
    r.rrConflicts = _rrConflicts;

    const auto& prods = _g.productions();
    std::vector<std::string> input = tokens;
    if (input.empty() || input.back() != "$") input.push_back("$");

    std::stack<int>         stateStack;
    std::stack<std::string> symStack;
    stateStack.push(0);
    size_t ip = 0;

    auto log = [&](const std::string& msg) { r.log.push_back(msg); };
    log("  Step | State | Input Sym | Action");
    log("  " + std::string(50, '-'));

    int step = 0;
    while (true) {
        ++step;
        if (step > 50000) { log("  [ABORT] Step limit exceeded"); break; }

        int state = stateStack.top();
        std::string sym = (ip < input.size()) ? input[ip] : "$";

        // Log
        log("  [" + std::to_string(step) + "] state=" + std::to_string(state)
            + " sym='" + sym + "'");

        auto itS = _actionTable.find(state);
        if (itS == _actionTable.end() || !itS->second.count(sym)) {
            std::string msg = "  ERROR: no action for state=" + std::to_string(state)
                            + " input='" + sym + "'";
            log(msg); r.errorMsgs.push_back(msg); ++r.errors;
            // Error recovery: skip input
            if (sym == "$") break;
            ++ip; continue;
        }

        Action act = itS->second.at(sym);
        if (act.kind == Action::SHIFT) {
            log("    SHIFT to state " + std::to_string(act.value));
            stateStack.push(act.value);
            symStack.push(sym);
            ++ip;
        } else if (act.kind == Action::REDUCE) {
            const auto& p = prods[act.value];
            std::string rule = p.lhs + " ->";
            for (auto& s : p.rhs) rule += " " + s;
            log("    REDUCE by " + rule);
            // Pop rhs symbols (handle epsilon)
            int popCount = (p.rhs.size() == 1 && p.rhs[0] == "ε") ? 0 : (int)p.rhs.size();
            for (int k = 0; k < popCount; ++k) {
                if (!stateStack.empty()) stateStack.pop();
                if (!symStack.empty())   symStack.pop();
            }
            // Goto
            int topState = stateStack.top();
            if (_gotoTable.count(topState) && _gotoTable.at(topState).count(p.lhs)) {
                stateStack.push(_gotoTable.at(topState).at(p.lhs));
                symStack.push(p.lhs);
            } else {
                std::string msg = "  ERROR: no goto for state=" + std::to_string(topState)
                                + " NT='" + p.lhs + "'";
                log(msg); r.errorMsgs.push_back(msg); ++r.errors;
                break;
            }
        } else if (act.kind == Action::ACCEPT) {
            log("    ACCEPT");
            r.success = true;
            break;
        } else {
            std::string msg = "  ERROR: state=" + std::to_string(state) + " input='" + sym + "'";
            log(msg); r.errorMsgs.push_back(msg); ++r.errors;
            if (sym == "$") break;
            ++ip;
        }
        ++r.steps;
    }

    timer.stop();
    r.elapsedMs  = timer.elapsedMs();
    r.memoryBytes= _states.size() * 128 + tokens.size() * sizeof(std::string);
    return r;
}

void SLR1Parser::printTable() const {
    std::cout << "\n=== SLR(1) Action Table (first 10 states) ===\n";
    int limit = std::min((int)_states.size(), 10);
    std::cout << std::setw(6) << "State";
    std::set<std::string> cols;
    for (auto it = _actionTable.begin(); it != _actionTable.end(); ++it)
        for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
            cols.insert(it2->first);
    for (auto& c : cols) std::cout << " | " << std::setw(10) << c;
    std::cout << '\n' << std::string(6 + (int)cols.size() * 14, '-') << '\n';

    for (int i = 0; i < limit; ++i) {
        std::cout << std::setw(6) << i;
        auto it = _actionTable.find(i);
        for (auto& c : cols) {
            std::string cell = "";
            if (it != _actionTable.end()) {
                auto it2 = it->second.find(c);
                if (it2 != it->second.end()) {
                    if (it2->second.kind == Action::SHIFT)  cell = "s" + std::to_string(it2->second.value);
                    if (it2->second.kind == Action::REDUCE) cell = "r" + std::to_string(it2->second.value);
                    if (it2->second.kind == Action::ACCEPT) cell = "acc";
                }
            }
            std::cout << " | " << std::setw(10) << cell;
        }
        std::cout << '\n';
    }
    if ((int)_states.size() > 10)
        std::cout << "  ... (" << _states.size() - 10 << " more states)\n";
}
