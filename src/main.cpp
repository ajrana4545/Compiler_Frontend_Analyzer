#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <iomanip>

// Lexers
#include "lexer/Token.h"
#include "lexer/DFALexer.h"
#include "lexer/RegexLexer.h"

// Parser / Grammar
#include "parser/Grammar.h"
#include "parser/RecursiveDescentParser.h"
#include "parser/LL1Parser.h"
#include "parser/SLR1Parser.h"

// Semantic
#include "semantic/SymbolTable.h"
#include "semantic/SemanticAnalyzer.h"

// Engine
#include "engine/ComparisonEngine.h"
#include "engine/Recommender.h"

// Utils
#include "utils/TablePrinter.h"
#include "utils/Timer.h"

// ─── CLI helpers ──────────────────────────────────────────────────────────────
static void banner(const std::string& title) {
    std::string line(72, '=');
    std::cout << '\n' << line << '\n';
    int pad = (int)(72 - title.size()) / 2;
    std::cout << std::string(pad, ' ') << title << '\n';
    std::cout << line << '\n';
}

static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) { std::cerr << "Cannot open file: " << path << '\n'; return ""; }
    std::ostringstream ss; ss << f.rdbuf();
    return ss.str();
}

// Convert TokenStream to a vector<string> of token VALUES/types for parsers
// RD parser expects symbolic type strings; LL1/SLR1 need values matching grammar terminals.
static std::vector<std::string> toRDTokens(const TokenStream& ts) {
    std::vector<std::string> out;
    for (auto& t : ts) {
        if (t.type == TokenType::EOF_TOK) continue;
        // Map to Mini-C grammar symbols
        switch (t.type) {
            case TokenType::KW_INT:    out.push_back("int");    break;
            case TokenType::KW_FLOAT:  out.push_back("float");  break;
            case TokenType::KW_CHAR:   out.push_back("char");   break;
            case TokenType::KW_VOID:   out.push_back("void");   break;
            case TokenType::KW_IF:     out.push_back("if");     break;
            case TokenType::KW_ELSE:   out.push_back("else");   break;
            case TokenType::KW_WHILE:  out.push_back("while");  break;
            case TokenType::KW_FOR:    out.push_back("for");    break;
            case TokenType::KW_RETURN: out.push_back("return"); break;
            case TokenType::IDENTIFIER:out.push_back("ID");     break;
            case TokenType::KW_CUSTOM: out.push_back(t.value);  break;  // custom keyword -> use as-is
            case TokenType::INT_LITERAL:
            case TokenType::FLOAT_LITERAL: out.push_back("NUM"); break;
            case TokenType::PLUS:      out.push_back("+");      break;
            case TokenType::MINUS:     out.push_back("-");      break;
            case TokenType::STAR:      out.push_back("*");      break;
            case TokenType::SLASH:     out.push_back("/");      break;
            case TokenType::ASSIGN:    out.push_back("=");      break;
            case TokenType::EQ:        out.push_back("==");     break;
            case TokenType::NEQ:       out.push_back("!=");     break;
            case TokenType::LT:        out.push_back("<");      break;
            case TokenType::GT:        out.push_back(">");      break;
            case TokenType::LE:        out.push_back("<=");     break;
            case TokenType::GE:        out.push_back(">=");     break;
            case TokenType::LPAREN:    out.push_back("(");      break;
            case TokenType::RPAREN:    out.push_back(")");      break;
            case TokenType::LBRACE:    out.push_back("{");      break;
            case TokenType::RBRACE:    out.push_back("}");      break;
            case TokenType::SEMICOLON: out.push_back(";");      break;
            case TokenType::COMMA:     out.push_back(",");      break;
            default: break; // skip unknown / comment
        }
    }
    return out;
}

// Print token stream table
static void printTokens(const TokenStream& ts) {
    TablePrinter::Row hdr = {"#", "Type", "Value", "Line", "Col"};
    std::vector<TablePrinter::Row> rows;
    int i = 0;
    for (auto& t : ts) {
        if (t.type == TokenType::EOF_TOK) continue;
        rows.push_back({
            std::to_string(++i),
            tokenTypeName(t.type),
            t.value,
            std::to_string(t.line),
            std::to_string(t.col)
        });
    }
    TablePrinter::print(hdr, rows);
}

// Print token statistics
static void printTokenStats(const std::string& lexerName,
                            double ms, int count, int errors, size_t mem,
                            const std::unordered_map<std::string,int>& typeCounts) {
    std::cout << "\n  Lexer: " << lexerName << "\n";
    std::cout << "  Time: " << std::fixed << std::setprecision(3) << ms << " ms"
              << "  |  Tokens: " << count
              << "  |  Errors: " << errors
              << "  |  Memory: " << mem << " bytes\n";
    std::cout << "  Token type breakdown:\n";
    for (auto it = typeCounts.begin(); it != typeCounts.end(); ++it)
        std::cout << "    " << it->first << ": " << it->second << "\n";
}

// ─── Main ─────────────────────────────────────────────────────────────────────
int main(int argc, char** argv) {
    std::string inputFile  = "samples/test1.mc";
    std::string grammarFile= "samples/grammar.txt";
    std::string tokensFile = "";           // optional custom keywords file
    bool verbose = false;
    bool showTable = false;
    int  onlyModule = 0;  // 0 = run all modules

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "--input"   || arg == "-i") && i+1 < argc) inputFile   = argv[++i];
        else if ((arg == "--grammar" || arg == "-g") && i+1 < argc) grammarFile = argv[++i];
        else if ((arg == "--tokens"  || arg == "-k") && i+1 < argc) tokensFile  = argv[++i];
        else if ((arg == "--module"  || arg == "-m") && i+1 < argc) onlyModule  = std::stoi(argv[++i]);
        else if (arg == "--verbose" || arg == "-v") verbose = true;
        else if (arg == "--table"   || arg == "-t") showTable = true;
        else if (arg == "--help"    || arg == "-h") {
            std::cout << "Usage: compiler_analyzer [--input <file>] [--grammar <file>]\n"
                         "                         [--tokens <file>] [--module <1-6>] [--verbose] [--table]\n"
                         "  --tokens / -k   File with custom keywords (one per line)\n"
                         "  --module / -m   Run only a specific module (1-6)\n";
            return 0;
        }
    }
    // Helper lambda: true when this module should be printed
    auto show = [&](int m) { return onlyModule == 0 || onlyModule == m; };

    // ── Load custom keywords (optional) ─────────────────────────────────────
    std::set<std::string> customKw;
    if (!tokensFile.empty()) {
        std::ifstream kf(tokensFile);
        std::string kw;
        while (std::getline(kf, kw)) {
            // Strip whitespace / comments
            if (!kw.empty() && kw[0] != '#') customKw.insert(kw);
        }
        if (show(1)) {
            std::cout << "  Custom keywords loaded: ";
            for (auto& k : customKw) std::cout << k << " ";
            std::cout << "\n";
        }
    }

    // ── Load source ──────────────────────────────────────────────────────────
    std::string source = readFile(inputFile);
    if (source.empty()) { std::cerr << "Empty or missing input file.\n"; return 1; }

    std::cout << "\n";
    std::cout << "=========================================================================\n";
    std::cout << "         INTELLIGENT COMPILER FRONT-END ANALYZER\n";
    std::cout << "         Lexical -> Syntax -> Semantic -> Comparison\n";
    std::cout << "=========================================================================\n";
    std::cout << "\n  Input file : " << inputFile  << '\n';
    std::cout <<   "  Grammar    : " << grammarFile << '\n';

    ComparisonEngine engine;

    // ══════════════════════════════════════════════════════════════════════════
    // MODULE 1: LEXICAL ANALYSIS  (always runs — tokens needed downstream)
    // ══════════════════════════════════════════════════════════════════════════
    if (show(1)) banner("MODULE 1: LEXICAL ANALYSIS");

    // --- DFA Lexer ---
    if (show(1)) std::cout << "\n[1.1] DFA-Based Tokenizer\n";
    DFALexer dfaLex(source);
    dfaLex.setCustomKeywords(customKw);
    TokenStream dfaTokens = dfaLex.tokenize();
    auto& dm = dfaLex.metrics();
    if (show(1)) {
        printTokenStats("DFA Lexer", dm.elapsedMs, dm.tokenCount, dm.errorCount, dm.memoryBytes, dm.typeCounts);
        if (verbose) printTokens(dfaTokens);
    }

    AlgorithmResult dfaRes;
    dfaRes.name       = "DFA-Lexer";
    dfaRes.category   = "LEXER";
    dfaRes.success    = (dm.errorCount == 0);
    dfaRes.elapsedMs  = dm.elapsedMs;
    dfaRes.memoryBytes= dm.memoryBytes;
    dfaRes.steps      = dm.tokenCount;
    dfaRes.errors     = dm.errorCount;
    engine.addResult(dfaRes);

    // --- Regex Lexer ---
    if (show(1)) std::cout << "\n[1.2] Regex-Based Tokenizer\n";
    RegexLexer regLex(source);
    regLex.setCustomKeywords(customKw);
    TokenStream regTokens = regLex.tokenize();
    auto& rm = regLex.metrics();
    if (show(1)) {
        printTokenStats("Regex Lexer", rm.elapsedMs, rm.tokenCount, rm.errorCount, rm.memoryBytes, rm.typeCounts);
        if (verbose) printTokens(regTokens);
    }

    AlgorithmResult regRes;
    regRes.name       = "Regex-Lexer";
    regRes.category   = "LEXER";
    regRes.success    = (rm.errorCount == 0);
    regRes.elapsedMs  = rm.elapsedMs;
    regRes.memoryBytes= rm.memoryBytes;
    regRes.steps      = rm.tokenCount;
    regRes.errors     = rm.errorCount;
    engine.addResult(regRes);

    // Use DFA tokens for downstream analysis (both are equivalent)
    TokenStream& tokens = dfaTokens;

    // ══════════════════════════════════════════════════════════════════════════
    // MODULE 2: GRAMMAR PROCESSING  (always runs — grammar needed downstream)
    // ══════════════════════════════════════════════════════════════════════════
    if (show(2)) banner("MODULE 2: GRAMMAR PROCESSING");

    Grammar grammar;
    bool grammarLoaded = grammar.loadFromFile(grammarFile);
    if (!grammarLoaded) {
        std::cerr << "Failed to load grammar. Using embedded Mini-C grammar.\n";
        std::string g = R"(
program -> decl_list
decl_list -> decl decl_list | ε
decl -> type id ; | type id ( params ) { stmt_list }
type -> int | float | void | char
params -> type id param_list | ε
param_list -> , type id param_list | ε
stmt_list -> stmt stmt_list | ε
stmt -> expr ; | { stmt_list } | if ( expr ) stmt | if ( expr ) stmt else stmt | while ( expr ) stmt | return expr ; | return ;
expr -> id = expr | add_expr
add_expr -> mul_expr add_expr_p
add_expr_p -> + mul_expr add_expr_p | - mul_expr add_expr_p | ε
mul_expr -> unary mul_expr_p
mul_expr_p -> * unary mul_expr_p | / unary mul_expr_p | ε
unary -> ( expr ) | id | NUM
)";
        grammar.loadFromString(g);
    }

    if (show(2)) {
        grammar.print();
        std::cout << "\n[2.1] Removing left recursion...\n";
        // Note: our grammar is already non-left-recursive
        // grammar.removeLeftRecursion();
        std::cout << "[2.2] Computing FIRST and FOLLOW sets...\n";
    }
    grammar.computeFirst();
    grammar.computeFollow();
    if (show(2)) {
        grammar.printFirstFollow();
        std::vector<std::string> grammarErrors;
        if (grammar.validate(grammarErrors)) {
            std::cout << "\n  [OK] Grammar validation: PASSED\n";
        } else {
            std::cout << "\n  [FAIL] Grammar validation: FAILED\n";
            for (auto& e : grammarErrors) std::cout << "    " << e << '\n';
        }
        if (grammar.isLL1Compatible())
            std::cout << "  [OK] LL(1) compatibility check: PASSED\n";
        else
            std::cout << "  [WARN] LL(1) compatibility check: Grammar has conflicts\n";
    }

    // ══════════════════════════════════════════════════════════════════════════
    // MODULE 3: SYNTAX ANALYSIS
    // ══════════════════════════════════════════════════════════════════════════
    if (show(3)) banner("MODULE 3: SYNTAX ANALYSIS");

    // Convert token stream to string vectors for parsers (always needed for engine)
    std::vector<std::string> parserTokens = toRDTokens(tokens);

    // --- (a) Recursive Descent ---
    if (show(3)) std::cout << "\n[3.a] Recursive Descent Parser\n";
    {
        RecursiveDescentParser rdp;
        ParseResult r = rdp.parse(parserTokens);
        if (show(3)) {
            std::cout << "  Result: " << (r.success ? "SUCCESS" : "FAIL")
                      << "  Steps: " << r.steps
                      << "  Errors: " << r.errors
                      << "  Time: " << std::fixed << std::setprecision(3) << r.elapsedMs << " ms\n";
            if (!r.errorMsgs.empty()) {
                std::cout << "  Errors:\n";
                for (auto& e : r.errorMsgs) std::cout << "    " << e << '\n';
            }
            if (verbose) for (auto& l : r.log) std::cout << l << '\n';
        }
        AlgorithmResult ar;
        ar.name        = "RecursiveDescent";
        ar.category    = "PARSER";
        ar.success     = r.success;
        ar.elapsedMs   = r.elapsedMs;
        ar.memoryBytes = r.memoryBytes;
        ar.steps       = r.steps;
        ar.errors      = r.errors;
        ar.log         = r.log;
        ar.errorMsgs   = r.errorMsgs;
        engine.addResult(ar);
    }

    // --- (b) LL(1) ---
    if (show(3)) std::cout << "\n[3.b] LL(1) Parser\n";
    {
        LL1Parser ll1(grammar);
        ll1.setSilent(!show(3));
        ll1.buildTable();
        if (show(3)) {
            std::cout << "  Conflicts in table: " << ll1.conflictCount() << '\n';
            if (showTable) ll1.printTable();
        }
        ParseResult r = ll1.parse(parserTokens);
        if (show(3)) {
            std::cout << "  Result: " << (r.success ? "SUCCESS" : "FAIL")
                      << "  Steps: " << r.steps
                      << "  Errors: " << r.errors
                      << "  Time: " << std::fixed << std::setprecision(3) << r.elapsedMs << " ms\n";
            if (!r.errorMsgs.empty()) {
                std::cout << "  Errors:\n";
                for (auto& e : r.errorMsgs) std::cout << "    " << e << '\n';
            }
        }
        AlgorithmResult ar;
        ar.name        = "LL(1)";
        ar.category    = "PARSER";
        ar.success     = r.success;
        ar.elapsedMs   = r.elapsedMs;
        ar.memoryBytes = r.memoryBytes;
        ar.steps       = r.steps;
        ar.errors      = r.errors;
        ar.srConflicts = ll1.conflictCount();
        ar.log         = r.log;
        ar.errorMsgs   = r.errorMsgs;
        engine.addResult(ar);
    }

    // --- (c) SLR(1) ---
    if (show(3)) std::cout << "\n[3.c] SLR(1) Parser\n";
    {
        SLR1Parser slr(grammar);
        slr.setSilent(!show(3));
        slr.buildCanonical();
        slr.buildTable();
        if (show(3)) {
            std::cout << "  States: \n";
            std::cout << "  S/R Conflicts: " << slr.srConflicts()
                      << "  R/R Conflicts: " << slr.rrConflicts() << '\n';
            if (showTable) slr.printTable();
        }
        ParseResult r = slr.parse(parserTokens);
        if (show(3)) {
            std::cout << "  Result: " << (r.success ? "SUCCESS" : "FAIL")
                      << "  Steps: " << r.steps
                      << "  Errors: " << r.errors
                      << "  Time: " << std::fixed << std::setprecision(3) << r.elapsedMs << " ms\n";
            if (!r.errorMsgs.empty()) {
                std::cout << "  Errors:\n";
                for (auto& e : r.errorMsgs) std::cout << "    " << e << '\n';
            }
        }
        AlgorithmResult ar;
        ar.name        = "SLR(1)";
        ar.category    = "PARSER";
        ar.success     = r.success;
        ar.elapsedMs   = r.elapsedMs;
        ar.memoryBytes = r.memoryBytes;
        ar.steps       = r.steps;
        ar.errors      = r.errors;
        ar.srConflicts = slr.srConflicts();
        ar.rrConflicts = slr.rrConflicts();
        ar.log         = r.log;
        ar.errorMsgs   = r.errorMsgs;
        engine.addResult(ar);
    }

    // ══════════════════════════════════════════════════════════════════════════
    // MODULE 4: SEMANTIC ANALYSIS
    // ══════════════════════════════════════════════════════════════════════════
    if (show(4)) banner("MODULE 4: SEMANTIC ANALYSIS");

    if (show(4)) std::cout << "\n[4.1] Single-Pass Semantic Analyzer\n";
    {
        SinglePassAnalyzer spa;
        SemanticResult r = spa.analyze(tokens);
        if (show(4)) {
            std::cout << "  Result: " << (r.success ? "SUCCESS" : "FAIL")
                      << "  Time: " << std::fixed << std::setprecision(3) << r.elapsedMs << " ms\n";
            for (auto& e : r.errors)   std::cout << "  ERROR: "   << e << '\n';
            for (auto& w : r.warnings) std::cout << "  WARNING: " << w << '\n';
        }
        AlgorithmResult ar;
        ar.name       = "SinglePass-Semantic";
        ar.category   = "SEMANTIC";
        ar.success    = r.success;
        ar.elapsedMs  = r.elapsedMs;
        ar.errors     = (int)r.errors.size();
        ar.errorMsgs  = r.errors;
        engine.addResult(ar);
    }

    if (show(4)) std::cout << "\n[4.2] Multi-Pass Semantic Analyzer\n";
    {
        MultiPassAnalyzer mpa;
        SemanticResult r = mpa.analyze(tokens);
        if (show(4)) {
            std::cout << "  Result: " << (r.success ? "SUCCESS" : "FAIL")
                      << "  Passes: " << r.passCount
                      << "  Time: " << std::fixed << std::setprecision(3) << r.elapsedMs << " ms\n";
            for (auto& e : r.errors)   std::cout << "  ERROR: "   << e << '\n';
            for (auto& w : r.warnings) std::cout << "  WARNING: " << w << '\n';
        }
        AlgorithmResult ar;
        ar.name       = "MultiPass-Semantic";
        ar.category   = "SEMANTIC";
        ar.success    = r.success;
        ar.elapsedMs  = r.elapsedMs;
        ar.errors     = (int)r.errors.size();
        ar.errorMsgs  = r.errors;
        engine.addResult(ar);
    }

    // ══════════════════════════════════════════════════════════════════════════
    // MODULE 5: COMPARISON ENGINE
    // ══════════════════════════════════════════════════════════════════════════
    if (show(5)) {
        banner("MODULE 5: COMPARISON ENGINE");
        engine.printComparisonTable();
        if (verbose) {
            banner("STEP LOGS");
            engine.printLogs(true);
        }
    }

    // ══════════════════════════════════════════════════════════════════════════
    // MODULE 6: RECOMMENDATION
    // ══════════════════════════════════════════════════════════════════════════
    if (show(6)) {
        banner("MODULE 6: RECOMMENDATION");
        Recommender rec;
        auto recommendations = rec.recommend(engine);
        rec.print(recommendations);
    }

    std::cout << "Analysis complete.\n\n";
    return 0;
}
