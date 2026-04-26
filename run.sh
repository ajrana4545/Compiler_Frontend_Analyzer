#!/bin/bash
# =========================================================
#  Compiler Analyzer - All commands for Git Bash / VS Code
# =========================================================

# ---------- BUILD ----------
build() {
    echo "Building..."
    g++ -std=c++17 -O2 -o compiler_analyzer.exe $(find src -name "*.cpp")
    echo "Build done."
}

# ---------- RUN MODULES ----------
mod1() { ./compiler_analyzer.exe --input samples/test1.mc --grammar samples/grammar.txt --module 1; }
mod2() { ./compiler_analyzer.exe --input samples/test1.mc --grammar samples/grammar.txt --module 2; }
mod3() { ./compiler_analyzer.exe --input samples/test1.mc --grammar samples/grammar.txt --module 3; }
mod4() { ./compiler_analyzer.exe --input samples/test1.mc --grammar samples/grammar.txt --module 4; }
mod5() { ./compiler_analyzer.exe --input samples/test1.mc --grammar samples/grammar.txt --module 5; }
mod6() { ./compiler_analyzer.exe --input samples/test1.mc --grammar samples/grammar.txt --module 6; }
all()  { ./compiler_analyzer.exe --input samples/test1.mc --grammar samples/grammar.txt; }

# ---------- TEST FILES ----------
test1() { ./compiler_analyzer.exe --input samples/test1.mc --grammar samples/grammar.txt; }
test2() { ./compiler_analyzer.exe --input samples/test2.mc --grammar samples/grammar.txt; }
test3() { ./compiler_analyzer.exe --input samples/test3.mc --grammar samples/grammar.txt; }

# ---------- CUSTOM KEYWORDS ----------
custom() { ./compiler_analyzer.exe --input samples/test1.mc --grammar samples/grammar.txt --tokens samples/tokens.txt; }

# ---------- USAGE ----------
echo "Available commands (source this file first: source run.sh)"
echo "  build       - compile the project"
echo "  mod1..mod6  - run a specific module"
echo "  all         - run all 6 modules"
echo "  test1/2/3   - run on different test files"
echo "  custom      - run with custom keywords (tokens.txt)"
