# Intelligent Compiler Front-End Analyzer

A modular **C++17 compiler front-end** that implements multiple lexical, syntax, and semantic analysis algorithms, runs them on the same input, compares their performance, and recommends the best algorithm.

Includes a **web-based dashboard** powered by a Python backend for interactive analysis.

---

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Modules](#modules)
- [Project Structure](#project-structure)
- [Prerequisites](#prerequisites)
- [Build & Run](#build--run)
- [Web Interface](#web-interface)
- [Test Files](#test-files)
- [Grammar Format](#grammar-format)
- [Command Line Options](#command-line-options)
- [Technologies Used](#technologies-used)

---

## Overview

This project implements a **complete compiler front-end pipeline**:

```
Source Code (.mc)
       │
       ▼
┌──────────────┐     ┌──────────────┐     ┌──────────────┐
│   MODULE 1   │     │   MODULE 2   │     │   MODULE 3   │
│   Lexical    │────▶│   Grammar    │────▶│   Syntax     │
│   Analysis   │     │  Processing  │     │   Analysis   │
│              │     │              │     │              │
│ • DFA Lexer  │     │ • Load CFG   │     │ • Recursive  │
│ • Regex Lexer│     │ • FIRST sets │     │   Descent    │
│              │     │ • FOLLOW sets│     │ • LL(1)      │
│              │     │ • Validation │     │ • SLR(1)     │
└──────────────┘     └──────────────┘     └──────────────┘
                                                 │
       ┌─────────────────────────────────────────┘
       ▼
┌──────────────┐     ┌──────────────┐     ┌──────────────┐
│   MODULE 4   │     │   MODULE 5   │     │   MODULE 6   │
│   Semantic   │────▶│  Comparison  │────▶│  Recommend   │
│   Analysis   │     │   Engine     │     │   Engine     │
│              │     │              │     │              │
│ • Single-Pass│     │ • Side-by-   │     │ • Score each │
│ • Multi-Pass │     │   side table │     │   algorithm  │
│              │     │ • Performance│     │ • Rank best  │
│              │     │   metrics    │     │   choices    │
└──────────────┘     └──────────────┘     └──────────────┘
```

---

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                    Web Browser (UI)                      │
│                     index.html                          │
└─────────────────┬───────────────────────────────────────┘
                  │  HTTP / JSON API
┌─────────────────▼───────────────────────────────────────┐
│               Python Server (server.py)                 │
│           Serves frontend + calls C++ backend           │
└─────────────────┬───────────────────────────────────────┘
                  │  subprocess (stdin/stdout)
┌─────────────────▼───────────────────────────────────────┐
│            C++ Backend (compiler_analyzer.exe)           │
│                                                         │
│  src/lexer/     ── DFA & Regex tokenizers               │
│  src/parser/    ── Grammar, RD, LL(1), SLR(1) parsers   │
│  src/semantic/  ── Symbol table, type checking           │
│  src/engine/    ── Comparison & recommendation           │
│  src/utils/     ── Timer, table printer                  │
└─────────────────────────────────────────────────────────┘
```

---

## Modules

### Module 1: Lexical Analysis
Converts source code into a stream of tokens using two approaches:

| Algorithm | Approach | Description |
|-----------|----------|-------------|
| **DFA Lexer** | State Machine | Hand-written character-by-character scanner. Fast and predictable. |
| **Regex Lexer** | Pattern Matching | Uses C++ `<regex>` to match token patterns. Easier to maintain. |

**Tokens recognized:** Keywords (`int`, `if`, `return`...), identifiers, literals (int, float, char, string), operators (`+`, `==`, `<=`...), delimiters (`{`, `;`, `,`...), comments.

### Module 2: Grammar Processing
Loads a Context-Free Grammar (CFG) and prepares it for parsing:
- Computes **FIRST** and **FOLLOW** sets
- Validates grammar for undefined symbols
- Checks **LL(1) compatibility**
- Supports left recursion removal and left factoring

### Module 3: Syntax Analysis (Parsing)
Parses the token stream using three different algorithms:

| Parser | Type | Description |
|--------|------|-------------|
| **Recursive Descent** | Top-down, hand-written | One function per grammar rule. Simple and fast. |
| **LL(1)** | Top-down, table-driven | Uses a parse table built from FIRST/FOLLOW sets. |
| **SLR(1)** | Bottom-up, table-driven | Shift-reduce parser using LR(0) item sets. |

### Module 4: Semantic Analysis
Checks if the code is **meaningful** (not just syntactically correct):
- **Single-Pass:** One scan through tokens — checks declarations and types.
- **Multi-Pass:** Pass 1 collects all declarations, Pass 2 verifies types and usages. Supports forward references.

**Checks performed:**
- Undeclared variable usage
- Variable redeclaration in the same scope
- Type mismatch (e.g., assigning `float` to `int`)
- Assignment to `void` type

### Module 5: Comparison Engine
Displays a side-by-side **performance comparison** of all algorithms:

```
+---------------------+----------+--------+----------+---------+-------+--------+
| Algorithm           | Category | Result | Time(ms) | Mem (B) | Steps | Errors |
+---------------------+----------+--------+----------+---------+-------+--------+
| DFA-Lexer           | LEXER    | OK     | 0.0      | 2160    | 60    | 0      |
| Regex-Lexer         | LEXER    | OK     | 1.2      | 2160    | 60    | 0      |
| RecursiveDescent    | PARSER   | OK     | 0.1      | 5120    | 45    | 0      |
| LL(1)               | PARSER   | OK     | 0.2      | 8960    | 52    | 14     |
| SLR(1)              | PARSER   | FAIL   | 0.5      | 3200    | 30    | 12     |
+---------------------+----------+--------+----------+---------+-------+--------+
```

### Module 6: Recommendation Engine
Scores each algorithm using a weighted formula and recommends the best:

| Criterion | Points |
|-----------|--------|
| Parsing succeeded | +40 |
| Very fast (< 1ms) | +20 |
| Low memory (< 4KB) | +10 |
| Few steps (< 50) | +5 |
| Per shift-reduce conflict | -15 |
| Per reduce-reduce conflict | -20 |
| Per error | -5 |

---

## Project Structure

```
compiler_analyzer_project/
│
├── src/                          # C++ source code
│   ├── main.cpp                  # Entry point (runs all 6 modules)
│   │
│   ├── lexer/                    # Module 1: Lexical Analysis
│   │   ├── Token.h               # Token types and structure
│   │   ├── DFALexer.h / .cpp     # DFA-based tokenizer
│   │   └── RegexLexer.h / .cpp   # Regex-based tokenizer
│   │
│   ├── parser/                   # Modules 2 & 3: Grammar + Parsing
│   │   ├── Grammar.h / .cpp      # CFG representation, FIRST/FOLLOW
│   │   ├── ParserTypes.h         # Shared ParseResult structure
│   │   ├── RecursiveDescentParser.h / .cpp
│   │   ├── LL1Parser.h / .cpp    # LL(1) table-driven parser
│   │   └── SLR1Parser.h / .cpp   # SLR(1) bottom-up parser
│   │
│   ├── semantic/                 # Module 4: Semantic Analysis
│   │   ├── SymbolTable.h / .cpp  # Scoped symbol table
│   │   └── SemanticAnalyzer.h / .cpp  # Single & multi-pass analyzers
│   │
│   ├── engine/                   # Modules 5 & 6: Comparison + Recommendation
│   │   ├── ComparisonEngine.h / .cpp
│   │   └── Recommender.h / .cpp
│   │
│   └── utils/                    # Utilities
│       ├── Timer.h               # High-resolution stopwatch
│       └── TablePrinter.h        # ASCII table formatter
│
├── samples/                      # Test input files
│   ├── grammar.txt               # Mini-C grammar definition
│   ├── tokens.txt                # Custom keywords file
│   ├── test1.mc                  # Valid program (no errors)
│   ├── test2.mc                  # Syntax errors
│   ├── test3.mc                  # Semantic errors
│   ├── test4.mc                  # Nested loops & control flow
│   ├── test5.mc                  # Complex arithmetic expressions
│   ├── test6.mc                  # Multiple functions
│   ├── test7.mc                  # Lexer edge cases (operators)
│   ├── test8.mc                  # Minimal program
│   ├── test9.mc                  # Heavy semantic errors
│   └── test10.mc                 # Large program (performance test)
│
├── server.py                     # Python HTTP server (web backend)
├── index.html                    # Web dashboard (frontend)
├── run.sh                        # Build & run commands
├── compiler_analyzer.exe         # Compiled binary
└── README.md                     # This file
```

---

## Prerequisites

| Tool | Version | Purpose |
|------|---------|---------|
| **g++** | C++17 support | Compiling the C++ backend |
| **Python 3** | 3.6+ | Running the web server |
| **Git Bash** or **Terminal** | Any | Running shell commands |
| **Web Browser** | Modern (Chrome, Edge, Firefox) | Viewing the dashboard |

---

## Build & Run

### 1. Build the C++ Backend

```bash
# Using Git Bash or terminal
g++ -std=c++17 -O2 -o compiler_analyzer.exe $(find src -name "*.cpp")
```

Or using the helper script:
```bash
source run.sh
build
```

### 2. Run via Command Line

```bash
# Run all modules on the default test file
./compiler_analyzer.exe --input samples/test1.mc --grammar samples/grammar.txt

# Run a specific module (e.g., module 3 = parsing)
./compiler_analyzer.exe --input samples/test1.mc --grammar samples/grammar.txt --module 3

# Run with verbose output and parse tables
./compiler_analyzer.exe --input samples/test1.mc --grammar samples/grammar.txt --verbose --table
```

### 3. Run via Web Interface

```bash
# Start the web server
python server.py

# Open in browser
# http://localhost:8080
```

### 4. Using the Helper Script

```bash
source run.sh      # Load commands
build              # Compile
test1              # Run test1.mc through all modules
test2              # Run test2.mc (syntax errors)
mod3               # Run only module 3 (parsing)
all                # Run all modules
```

---

## Web Interface

The web dashboard provides an interactive GUI for the analyzer:

- **Source code editor** on the left panel
- **Module buttons** (1-Lexical, 2-Grammar, FIRST/FOLLOW, 3-Parsing, 4-Semantic, 5-Comparison, 6-Recommend)
- **Formatted output** with color-coded results
- **Parser comparison table** with status, steps, time, errors, and conflicts
- **Parse tables** (LL(1) and SLR(1)) displayed automatically

To start:
```bash
python server.py
```
Then open **http://localhost:8080** in your browser.

---

## Test Files

| File | Description | Lexer | Parser | Semantic |
|------|-------------|:-----:|:------:|:--------:|
| `test1.mc` | Valid program — factorial, average, if/else, while | ✅ | ✅ | ✅ |
| `test2.mc` | Missing `;`, missing `)`, extra `}` | ✅ | ❌ | — |
| `test3.mc` | Redeclaration, type mismatch, undeclared var | ✅ | ✅ | ❌ |
| `test4.mc` | Nested loops, multiple functions, control flow | ✅ | ✅ | ✅ |
| `test5.mc` | Operator precedence, nested parentheses | ✅ | ✅ | ✅ |
| `test6.mc` | Multiple functions (square, cube, divide, void) | ✅ | ✅ | ✅ |
| `test7.mc` | All operators (`==`, `!=`, `<=`, `>=`), comments | ✅ | ✅ | ✅ |
| `test8.mc` | Minimal: `int main() { return 0; }` | ✅ | ✅ | ✅ |
| `test9.mc` | Many semantic errors — undeclared, redeclared, type mismatch | ✅ | ✅ | ❌ |
| `test10.mc` | Large: 9 functions (fibonacci, gcd, isPrime, etc.) | ✅ | ✅ | ✅ |

---

## Grammar Format

The grammar file uses this format:
```
# Comments start with #
# Format: NonTerminal -> symbol1 symbol2 | alternative | ε

program -> decl_list
decl_list -> decl decl_list | ε
type -> int | float | void | char
stmt -> if ( expr ) stmt | while ( expr ) stmt | return expr ;
expr -> ID = expr | add_expr
```

**Rules:**
- `->` separates the left-hand side from alternatives
- `|` separates alternatives
- `ε` represents the empty string (epsilon)
- Symbols matching a production LHS are **non-terminals**
- Everything else is a **terminal**

---

## Command Line Options

| Flag | Short | Description |
|------|-------|-------------|
| `--input <file>` | `-i` | Source code file (default: `samples/test1.mc`) |
| `--grammar <file>` | `-g` | Grammar file (default: `samples/grammar.txt`) |
| `--tokens <file>` | `-k` | Custom keywords file (one per line) |
| `--module <1-6>` | `-m` | Run only a specific module |
| `--verbose` | `-v` | Show detailed step-by-step logs |
| `--table` | `-t` | Show LL(1) and SLR(1) parse tables |
| `--help` | `-h` | Show usage information |

### Examples

```bash
# Run only the lexer (module 1)
./compiler_analyzer.exe -i samples/test1.mc -g samples/grammar.txt -m 1

# Run parser with verbose output and parse tables
./compiler_analyzer.exe -i samples/test1.mc -g samples/grammar.txt -m 3 -v -t

# Run with custom keywords
./compiler_analyzer.exe -i samples/test1.mc -g samples/grammar.txt -k samples/tokens.txt

# Run all modules on a file with errors
./compiler_analyzer.exe -i samples/test2.mc -g samples/grammar.txt
```

---

## Technologies Used

| Component | Technology | Purpose |
|-----------|-----------|---------|
| Backend Core | **C++17** | Compiler algorithms implementation |
| Web Server | **Python 3** | HTTP server + JSON API bridge |
| Frontend | **HTML / CSS / JavaScript** | Interactive web dashboard |
| Build | **g++ (MinGW/GCC)** | C++ compilation |

---

## Mini-C Language

The analyzer works on **Mini-C**, a simplified subset of the C language:

### Supported Features
- **Data types:** `int`, `float`, `char`, `void`
- **Variables:** Declaration and assignment
- **Functions:** Definition, parameters, return values, calls
- **Control flow:** `if`, `if-else`, `while`, `for`
- **Operators:** `+`, `-`, `*`, `/`, `=`, `==`, `!=`, `<`, `>`, `<=`, `>=`
- **Literals:** Integer (`42`), float (`3.14`), char (`'a'`), string (`"hello"`)
- **Comments:** Single-line (`//`) and block (`/* */`)

### Example Program
```c
int factorial(int n) {
    int result;
    result = 1;
    while (n > 1) {
        result = result * n;
        n = n - 1;
    }
    return result;
}

int main() {
    int x;
    x = factorial(5);
    return 0;
}
```

---
