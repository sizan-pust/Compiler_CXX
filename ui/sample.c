// Sample C program for testing the compiler
// This demonstrates all major language features

#include <stdio.h>

// Function declarations
int add(int a, int b);
int factorial(int n);

// Simple calculation function
int add(int a, int b) {
    return a + b;
}

// Recursive factorial function
int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

// Main program
int main() {
    int x = 10;
    int y = 20;
    int sum = add(x, y);
    
    // Test control flow
    if (sum > 20) {
        printf("Sum is greater than 20\n");
    } else {
        printf("Sum is less than or equal to 20\n");
    }
    
    // Test loops
    int i = 0;
    while (i < 5) {
        printf("Loop iteration: %d\n", i);
        i = i + 1;
    }
    
    // Test for loop
    for (i = 1; i <= 5; i = i + 1) {
        int fact = factorial(i);
        printf("Factorial of %d is %d\n", i, fact);
    }
    
    // Test arithmetic operations
    int a = 15;
    int b = 4;
    int sum2 = a + b;
    int diff = a - b;
    int prod = a * b;
    int quot = a / b;
    int remain = a % b;
    
    printf("Sum: %d\n", sum2);
    printf("Difference: %d\n", diff);
    printf("Product: %d\n", prod);
    printf("Quotient: %d\n", quot);
    printf("Remainder: %d\n", remain);
    
    return 0;
}
