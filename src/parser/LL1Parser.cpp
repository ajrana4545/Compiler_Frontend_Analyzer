#include "LL1Parser.h"
#include "../utils/Timer.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stack>

LL1Parser::LL1Parser(Grammar& g) : _g(g) {}

void LL1Parser::buildTable() {
    _table.clear();
    _conflictCount = 0;
    const auto& prods = _g.productions();

    for (size_t i = 0; i < prods.size(); ++i) {
        const auto& p = prods[i];
        // For each terminal in FIRST(α)
        auto firstAlpha = _g.firstOfSequence(p.rhs);
        for (auto& a : firstAlpha) {
            if (a == "ε") continue;
            auto& cell = _table[p.lhs][a];
            if (cell.prod.lhs.empty()) {
                cell.prod = p;
            } else {
                cell.hasConflict = true;
                ++_conflictCount;
                if (!_silent) std::cerr << "[LL1] FIRST/FIRST conflict at M[" << p.lhs << "," << a << "]\n";
            }
        }
        // If ε ∈ FIRST(α), add entry for each terminal in FOLLOW(A)
        if (firstAlpha.count("ε")) {
            for (auto& b : _g.follow(p.lhs)) {
                auto& cell = _table[p.lhs][b];
                if (cell.prod.lhs.empty()) {
                    cell.prod = p;
                } else {
                    cell.hasConflict = true;
                    ++_conflictCount;
                    if (!_silent) std::cerr << "[LL1] FIRST/FOLLOW conflict at M[" << p.lhs << "," << b << "]\n";
                }
            }
        }
    }
}

ParseResult LL1Parser::parse(const std::vector<std::string>& tokens) {
    Timer timer; timer.start();
    ParseResult r;
    r.srConflicts = 0;
    r.rrConflicts = 0;

    std::vector<std::string> input = tokens;
    if (input.empty() || input.back() != "$") input.push_back("$");

    std::stack<std::string> stk;
    stk.push("$");
    stk.push(_g.startSymbol());

    size_t ip = 0; // input pointer
    int step   = 0;

    auto log = [&](const std::string& msg) { r.log.push_back(msg); };

    log("  Stack                        | Input          | Action");
    log("  " + std::string(70, '-'));

    while (true) {
        ++step;
        // Build stack string for display
        std::stack<std::string> tmp = stk;
        std::vector<std::string> stItems;
        while (!tmp.empty()) { stItems.push_back(tmp.top()); tmp.pop(); }
        std::string stackStr;
        for (int k = (int)stItems.size() - 1; k >= 0; --k) stackStr += stItems[k] + " ";
        std::string inputStr;
        for (size_t k = ip; k < input.size() && k < ip + 4; ++k) inputStr += input[k] + " ";

        if (stk.empty()) break;
        std::string top = stk.top();

        if (top == "$" && ip < input.size() && input[ip] == "$") {
            log("  [" + std::to_string(step) + "] ACCEPT");
            r.success = true;
            break;
        }

        if (top == input[ip]) {
            log("  [" + std::to_string(step) + "] MATCH '" + top + "'  |  " + stackStr + "| " + inputStr);
            stk.pop(); ++ip;
        } else if (_g.isTerminal(top) || top == "$") {
            std::string msg = "  [" + std::to_string(step) + "] ERROR: stack top '" + top
                             + "', input '" + input[ip] + "'";
            log(msg); r.errorMsgs.push_back(msg); ++r.errors;
            ++ip; // skip input symbol (error recovery)
            if (ip >= input.size()) break;
        } else {
            // Non-terminal: look up table
            auto itA = _table.find(top);
            if (itA == _table.end() || itA->second.find(input[ip]) == itA->second.end()) {
                // Also try wildcard via FOLLOW
                std::string msg = "  [" + std::to_string(step) + "] ERROR: no rule for M["
                                 + top + "," + input[ip] + "]";
                log(msg); r.errorMsgs.push_back(msg); ++r.errors;
                stk.pop(); // pop and ignore (error recovery)
            } else {
                auto& cell = itA->second.at(input[ip]);
                stk.pop();
                // Push rhs in reverse
                const auto& rhs = cell.prod.rhs;
                for (int k = (int)rhs.size() - 1; k >= 0; --k) {
                    if (rhs[k] != "ε") stk.push(rhs[k]);
                }
                std::string action = top + " -> ";
                for (auto& s : rhs) action += s + " ";
                log("  [" + std::to_string(step) + "] APPLY " + action + "  |  " + stackStr + "| " + inputStr);
            }
        }
        ++r.steps;
        if (step > 10000) { log("  [ABORT] Exceeded step limit"); break; }
    }

    timer.stop();
    r.elapsedMs  = timer.elapsedMs();
    r.memoryBytes= tokens.size() * sizeof(std::string) * 2 + r.log.size() * 80;
    return r;
}

void LL1Parser::printTable() const {
    std::cout << "\n=== LL(1) Parse Table ===\n";
    // Collect all terminals
    std::set<std::string> cols;
    for (auto it = _table.begin(); it != _table.end(); ++it)
        for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2)
            cols.insert(it2->first);

    // Print header
    std::cout << std::setw(15) << "NT" << " | ";
    for (auto& c : cols) std::cout << std::setw(14) << c << " | ";
    std::cout << '\n' << std::string(15 + (int)cols.size() * 17, '-') << '\n';

    for (auto tblIt = _table.begin(); tblIt != _table.end(); ++tblIt) {
        const std::string& nt = tblIt->first;
        const std::map<std::string, TableCell>& row = tblIt->second;
        std::cout << std::setw(15) << nt << " | ";
        for (auto& c : cols) {
            auto it = row.find(c);
            if (it == row.end()) { std::cout << std::setw(14) << "" << " | "; continue; }
            std::string cellStr = it->second.prod.lhs + "->";
            for (auto& s : it->second.prod.rhs) cellStr += s;
            if (it->second.hasConflict) cellStr += "!";
            std::cout << std::setw(14) << cellStr << " | ";
        }
        std::cout << '\n';
    }
}
