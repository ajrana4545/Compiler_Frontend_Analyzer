#pragma once
#include <string>
#include <vector>
#include <map>

// Unified result record for any algorithm (lexer OR parser)
struct AlgorithmResult {
    std::string name;
    bool        success      = false;
    double      elapsedMs    = 0.0;
    size_t      memoryBytes  = 0;
    int         steps        = 0;
    int         errors       = 0;
    int         srConflicts  = 0;
    int         rrConflicts  = 0;
    std::vector<std::string> log;
    std::vector<std::string> errorMsgs;
    std::string category; // "LEXER" | "PARSER" | "SEMANTIC"
};

class ComparisonEngine {
public:
    void addResult(const AlgorithmResult& r);
    void printComparisonTable() const;
    void printLogs(bool verbose = false) const;

    const std::vector<AlgorithmResult>& results() const { return _results; }
    std::vector<AlgorithmResult> resultsByCategory(const std::string& cat) const;

private:
    std::vector<AlgorithmResult> _results;
};
