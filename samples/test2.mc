/* test2.mc - Mini-C program with SYNTAX ERRORS
   Errors:
     1. Missing semicolon after variable declaration
     2. Missing closing parenthesis in if condition
     3. Extra brace at end */

int main() {
    int x
    int y;
    x = 5;
    y = 10;
    if (x > 0 {
        y = x + 1;
    }
    return 0;
}
}
