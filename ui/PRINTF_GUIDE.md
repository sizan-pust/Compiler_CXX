# Using Printf and Console Output in CompilerIDE

## Overview
CompilerIDE now includes an integrated **Console** tab that simulates program output. The console can display output from:
1. `printf()` statements in C code
2. Calculated values returned by functions
3. Variable values and program flow simulation

## How to Use Printf

### Basic Printf Examples

**Example 1: Simple Text Output**
```c
#include <stdio.h>

int main() {
    printf("Hello, World!\n");
    return 0;
}
```

When you click **"▶️ Run Program"** or use **Build & Run**, the console will show:
```
============================================================
Program Output:
============================================================

Hello, World!

============================================================
Program finished
```

**Example 2: Printing Variables**
```c
#include <stdio.h>

int main() {
    int x = 10;
    int y = 20;
    printf("x = %d\n", x);
    printf("y = %d\n", y);
    printf("Sum: %d\n", x + y);
    return 0;
}
```

Console output:
```
x = 10
y = 20
Sum: 30
```

**Example 3: Function Results**
```c
int add(int a, int b) {
    return a + b;
}

int main() {
    int result = add(5, 3);
    printf("5 + 3 = %d\n", result);
    return 0;
}
```

Console output:
```
5 + 3 = 8
```

## Printf Format Specifiers

| Specifier | Meaning | Example |
|-----------|---------|---------|
| `%d` | Integer | `printf("%d", 42);` → `42` |
| `%i` | Integer | `printf("%i", 42);` → `42` |
| `%f` | Float | `printf("%f", 3.14);` → `3.14` |
| `%s` | String | `printf("%s", "hi");` → `hi` |
| `%c` | Character | `printf("%c", 'A');` → `A` |

## Escape Sequences

| Sequence | Meaning |
|----------|---------|
| `\n` | Newline |
| `\t` | Tab |
| `\\` | Backslash |
| `\"` | Double quote |

### Example:
```c
printf("Name:\tJohn\nAge:\t25\n");
```

Output:
```
Name:   John
Age:    25
```

## Working Your Example

Your code:
```c
int add(int a, int b) {
    return a + b;
}

int main() {
    int result = add(5, 3);
    return result;
}
```

To see output, modify it to use `printf`:
```c
#include <stdio.h>

int add(int a, int b) {
    return a + b;
}

int main() {
    int result = add(5, 3);
    printf("Result: %d\n", result);
    return result;
}
```

Now click **Build & Run** and check the **Console** tab:
```
============================================================
Program Output:
============================================================

Result: 8

============================================================
Program finished
```

## Console Features

### Running the Program
1. **Automatic:** Click **Build & Run** (Ctrl+B) to build and auto-run
2. **Manual:** Click **▶️ Run Program** in the Console tab anytime

### Clearing Console
- Click **🗑️ Clear Console** to clear all console output

### Console Colors
- **Green:** Program output
- **Red:** Errors or messages
- **Blue:** Info messages

## Common Mistakes

### ❌ Missing `#include <stdio.h>`
```c
// This won't work - stdio.h is needed for printf
int main() {
    printf("Hello\n");  // Error: printf undefined
    return 0;
}
```

**Fix:**
```c
#include <stdio.h>

int main() {
    printf("Hello\n");  // Works!
    return 0;
}
```

### ❌ Wrong Format Specifier
```c
int x = 10;
printf("%f\n", x);  // Wrong! %f expects float, x is int
```

**Fix:**
```c
int x = 10;
printf("%d\n", x);  // Correct format specifier
```

### ❌ Missing Newline
```c
printf("Line 1");
printf("Line 2");
// Output: Line 1Line 2 (no line break)
```

**Fix:**
```c
printf("Line 1\n");
printf("Line 2\n");
// Output:
// Line 1
// Line 2
```

## Advanced Examples

### Loop Output
```c
#include <stdio.h>

int main() {
    for (int i = 1; i <= 5; i++) {
        printf("Number: %d\n", i);
    }
    return 0;
}
```

Output:
```
Number: 1
Number: 2
Number: 3
Number: 4
Number: 5
```

### Conditional Output
```c
#include <stdio.h>

int main() {
    int x = 15;
    if (x > 10) {
        printf("x is greater than 10\n");
    } else {
        printf("x is 10 or less\n");
    }
    return 0;
}
```

Output:
```
x is greater than 10
```

### Function with Multiple Outputs
```c
#include <stdio.h>

int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    for (int i = 1; i <= 5; i++) {
        printf("Factorial of %d = %d\n", i, factorial(i));
    }
    return 0;
}
```

Output:
```
Factorial of 1 = 1
Factorial of 2 = 2
Factorial of 3 = 6
Factorial of 4 = 24
Factorial of 5 = 120
```

## How the Console Simulator Works

The console uses **static analysis** of your code to simulate output:

1. **Finds all printf statements** in your code
2. **Extracts format strings** and arguments
3. **Resolves variable values** from assignments
4. **Formats output** based on format specifiers
5. **Displays in console** with colors

### Limitations
- Cannot execute loops (only static analysis)
- Cannot evaluate complex expressions
- Cannot call external functions
- Cannot handle random data

For actual execution, you would need to compile to machine code and run the executable (beyond scope of this compiler).

## Tips for Effective Output

### 1. Label Your Output
```c
printf("Input value: %d\n", value);
printf("Processing...\n");
printf("Result: %d\n", result);
```

### 2. Use Separators
```c
printf("=== Program Start ===\n");
printf("Data: %d\n", data);
printf("=== Program End ===\n");
```

### 3. Print Intermediate Values
```c
printf("Before: x = %d\n", x);
x = x * 2;
printf("After doubling: x = %d\n", x);
```

### 4. Use Descriptive Messages
```c
// Good
printf("Sum of %d and %d is %d\n", a, b, sum);

// Not helpful
printf("%d\n", sum);
```

## Resources

- **C Standard Library:** [stdio.h reference](https://en.cppreference.com/w/c/io)
- **Printf Man Page:** `man 3 printf`
- **C Tutorial:** Search "C printf tutorial" online

## Troubleshooting

### Console shows "Program output:" but nothing else
- Check if you have `printf` statements
- Ensure statements are inside `main()` function
- Verify format specifiers match variable types

### Console shows wrong values
- Variable values might not be resolved from all branches
- Check your variable assignments in the code
- The simulator is static - it won't execute conditions

### "Program finished" but no output
- Your program might not have any printf calls
- Try adding: `printf("Hello World\n");`

## Next Steps

1. **Modify your code** to add printf statements
2. **Save the file** (Ctrl+S)
3. **Click "Build & Run"** - builds and auto-runs
4. **Check the Console tab** for output
5. **Adjust output** to see what you need

---

**Happy Printing!** 🖨️
