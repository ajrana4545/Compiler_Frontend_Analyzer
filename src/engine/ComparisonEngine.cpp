#include "ComparisonEngine.h"
#include "../utils/TablePrinter.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

void ComparisonEngine::addResult(const AlgorithmResult& r) {
    _results.push_back(r);
}

std::vector<AlgorithmResult> ComparisonEngine::resultsByCategory(const std::string& cat) const {
    std::vector<AlgorithmResult> out;
    for (auto& r : _results) if (r.category == cat) out.push_back(r);
    return out;
}

void ComparisonEngine::printComparisonTable() const {
    std::cout << "\n";
    std::cout << "======================================================================\n";
    std::cout << "                 ALGORITHM COMPARISON REPORT\n";
    std::cout << "======================================================================\n\n";

    TablePrinter::Row hdr = {
        "Algorithm", "Category", "Result", "Time(ms)", "Memory(B)",
        "Steps", "Errors", "S/R Cnf", "R/R Cnf"
    };
    std::vector<TablePrinter::Row> rows;
    for (auto& r : _results) {
        rows.push_back({
            r.name,
            r.category,
            r.success ? "OK" : "FAIL",
            std::to_string((int)r.elapsedMs) + "." + std::to_string((int)(r.elapsedMs * 10) % 10),
            std::to_string(r.memoryBytes),
            std::to_string(r.steps),
            std::to_string(r.errors),
            std::to_string(r.srConflicts),
            std::to_string(r.rrConflicts)
        });
    }
    TablePrinter::print(hdr, rows);
}

void ComparisonEngine::printLogs(bool verbose) const {
    for (auto& r : _results) {
        std::cout << "\n--- " << r.name << " Log ---\n";
        int maxLines = verbose ? (int)r.log.size() : std::min((int)r.log.size(), 30);
        for (int i = 0; i < maxLines; ++i) std::cout << r.log[i] << '\n';
        if (!verbose && (int)r.log.size() > 30)
            std::cout << "  ... (" << r.log.size() - 30 << " more lines, use --verbose)\n";
        if (!r.errorMsgs.empty()) {
            std::cout << "  Errors:\n";
            for (auto& e : r.errorMsgs) std::cout << "    " << e << '\n';
        }
    }
}
