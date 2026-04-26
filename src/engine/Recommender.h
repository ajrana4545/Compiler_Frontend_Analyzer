// ============================================================
// Recommender.h — Recommendation Engine (Module 6)
// ============================================================

#pragma once
#include "ComparisonEngine.h"
#include <string>
#include <vector>

struct Recommendation {
    std::string algorithm;
    std::string category;
    double      score;
    std::string reason;
};

class Recommender {
public:
    // Analyze all results and produce ranked recommendations
    std::vector<Recommendation> recommend(const ComparisonEngine& engine);
    void print(const std::vector<Recommendation>& recs);

private:
    double scoreResult(const AlgorithmResult& r);
    std::string buildReason(const AlgorithmResult& r, double score);
};
