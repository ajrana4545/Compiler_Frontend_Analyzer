#pragma once
#include <string>
#include <vector>

// Shared result type returned by every parser.
struct ParseResult {
    bool        success     = false;
    int         steps       = 0;
    int         errors      = 0;
    double      elapsedMs   = 0.0;
    size_t      memoryBytes = 0;
    int         srConflicts = 0;
    int         rrConflicts = 0;
    std::vector<std::string> log;
    std::vector<std::string> errorMsgs;
};
