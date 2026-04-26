#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <sstream>

// Formats and prints a 2-D table (header + rows) as an ASCII box table.
class TablePrinter {
public:
    using Row = std::vector<std::string>;

    static void print(const Row& headers, const std::vector<Row>& rows,
                      std::ostream& out = std::cout)
    {
        // Determine column widths.
        std::vector<size_t> w(headers.size(), 0);
        for (size_t c = 0; c < headers.size(); ++c)
            w[c] = headers[c].size();
        for (auto& row : rows)
            for (size_t c = 0; c < row.size() && c < w.size(); ++c)
                w[c] = std::max(w[c], row[c].size());

        auto sep = [&]() {
            out << '+';
            for (auto cw : w) out << std::string(cw + 2, '-') << '+';
            out << '\n';
        };

        auto printRow = [&](const Row& r, bool bold = false) {
            out << '|';
            for (size_t c = 0; c < w.size(); ++c) {
                std::string cell = (c < r.size()) ? r[c] : "";
                out << ' ' << std::left << std::setw((int)w[c]) << cell << " |";
            }
            out << '\n';
        };

        sep();
        printRow(headers, true);
        sep();
        for (auto& row : rows) printRow(row);
        sep();
    }
};
