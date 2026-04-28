// Test file for compiler lexer
#include <stdio.h>

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    int x = 10;
    int y = 20;
    float pi = 3.14159;
    char str[] = "Hello World";
    
    int sum = x + y;
    int product = x * y;
    int diff = y - x;
    int quotient = y / x;
    
    if (sum > 20) {
        printf("Sum is greater than 20\n");
    } else {
        printf("Sum is less than or equal to 20\n");
    }
    
    for (int i = 0; i < 5; i++) {
        printf("Factorial of %d is %d\n", i, factorial(i));
    }
    
    return 0;
}
