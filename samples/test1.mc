/* test1.mc - Valid Mini-C program
   Tests: variable declarations, arithmetic, function definition,
          if/else, while loop, return statement */

int factorial(int n) {
    int result;
    result = 1;
    while (n > 1) {
        result = result * n;
        n = n - 1;
    }
    return result;
}

float average(int a, int b) {
    float sum;
    sum = a + b;
    return sum / 2;
}

int main() {
    int x;
    int y;
    float avg;
    x = 5;
    y = 10;
    avg = average(x, y);
    if (x > 0) {
        x = factorial(x);
    } else {
        x = 0;
    }
    return 0;
}
