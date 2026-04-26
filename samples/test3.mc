/* test3.mc - Mini-C program with SEMANTIC ERRORS
   Errors:
     1. Use of undeclared variable 'z'
     2. Type mismatch: assigning float literal to int 'count'
     3. Redeclaration of 'x' in same scope */

int main() {
    int x;
    int x;
    int count;
    float result;
    x = 5;
    count = 3.14;
    result = z + x;
    return 0;
}
