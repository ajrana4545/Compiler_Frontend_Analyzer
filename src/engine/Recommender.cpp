// ============================================================
// Recommender.cpp — Recommendation Engine (Module 6)
// ============================================================

#include "Recommender.h"
#include <iostream>
#include <algorithm>
#include <iomanip>

double Recommender::scoreResult(const AlgorithmResult& r) {
    double score = 0.0;
    if (r.success) score += 40.0;
    // Conflicts are major negatives
    score -= r.srConflicts * 15.0;
    score -= r.rrConflicts * 20.0;
    // Fewer errors is better
    score -= r.errors * 5.0;
    // Speed: bonus for < 1ms, penalty for > 10ms
    if (r.elapsedMs < 1.0)  score += 20.0;
    else if (r.elapsedMs < 5.0)  score += 10.0;
    else if (r.elapsedMs > 10.0) score -= 10.0;
    // Memory: smaller is better
    if (r.memoryBytes < 4096) score += 10.0;
    // Steps: fewer steps can mean simpler / more direct parsing
    if (r.steps < 50) score += 5.0;
    return score;
}

std::string Recommender::buildReason(const AlgorithmResult& r, double score) {
    std::string reason;
    if (!r.success) reason += "Parsing failed. ";
    if (r.srConflicts > 0)
        reason += std::to_string(r.srConflicts) + " shift-reduce conflict(s) detected. ";
    if (r.rrConflicts > 0)
        reason += std::to_string(r.rrConflicts) + " reduce-reduce conflict(s) detected. ";
    if (r.success && r.srConflicts == 0 && r.rrConflicts == 0)
        reason += "No conflicts, clean parse. ";
    if (r.elapsedMs < 1.0)  reason += "Very fast (" + std::to_string(r.elapsedMs).substr(0,5) + " ms). ";
    else                    reason += "Moderate speed (" + std::to_string((int)r.elapsedMs) + " ms). ";
    if (r.category == "PARSER") {
        if (r.name == "RecursiveDescent")
            reason += "Recursive Descent is ideal for simple/moderate grammars and is easy to hand-write. ";
        else if (r.name == "LL(1)")
            reason += "LL(1) is efficient for LL-compatible grammars; requires grammar pre-processing. ";
        else if (r.name == "SLR(1)")
            reason += "SLR(1) handles a broader class of grammars than LL(1) with bottom-up parsing. ";
    }
    if (score >= 50.0)  reason += "-> HIGHLY RECOMMENDED.";
    else if (score >= 20.0) reason += "-> ACCEPTABLE.";
    else reason += "-> NOT RECOMMENDED for this grammar.";
    return reason;
}

std::vector<Recommendation> Recommender::recommend(const ComparisonEngine& engine) {
    std::vector<Recommendation> recs;
    for (auto& r : engine.results()) {
        Recommendation rec;
        rec.algorithm = r.name;
        rec.category  = r.category;
        rec.score     = scoreResult(r);
        rec.reason    = buildReason(r, rec.score);
        recs.push_back(rec);
    }
    // Sort descending by score
    std::sort(recs.begin(), recs.end(),
        [](const Recommendation& a, const Recommendation& b){ return a.score > b.score; });
    return recs;
}

void Recommender::print(const std::vector<Recommendation>& recs) {
    std::cout << "\n";
    std::cout << "======================================================================\n";
    std::cout << "                RECOMMENDATION ENGINE OUTPUT\n";
    std::cout << "======================================================================\n\n";

    int rank = 1;
    for (auto& rec : recs) {
        std::cout << "  #" << rank++ << " [" << rec.category << "] " << rec.algorithm
                  << "  (score: " << std::fixed << std::setprecision(1) << rec.score << ")\n";
        std::cout << "     " << rec.reason << "\n\n";
    }

    if (!recs.empty()) {
        std::cout << "* BEST OVERALL: [" << recs[0].category << "] "
                  << recs[0].algorithm << "\n";
        for (auto& rec : recs) {
            if (rec.category == "PARSER") {
                std::cout << "* BEST PARSER : " << rec.algorithm << "\n";
                break;
            }
        }
        for (auto& rec : recs) {
            if (rec.category == "LEXER") {
                std::cout << "* BEST LEXER  : " << rec.algorithm << "\n";
                break;
            }
        }
    }
    std::cout << '\n';
}

