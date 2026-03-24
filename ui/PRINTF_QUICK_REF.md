# Printf Quick Reference Card

## Single Page Cheat Sheet

### Basic Template
```c
#include <stdio.h>

int main() {
    printf("Hello, World!\n");
    return 0;
}
```

### Format Specifiers
```c
int x = 10;
float f = 3.14;
char c = 'A';

printf("%d", x);      // Integer: 10
printf("%i", x);      // Integer: 10
printf("%f", f);      // Float: 3.140000
printf("%c", c);      // Character: A
printf("%s", "Hi");   // String: Hi
```

### Common Escape Sequences
```c
\n    Newline
\t    Tab
\\    Backslash
\"    Double quote
```

### Examples with Output

**Print Variables**
```c
int age = 25;
printf("I am %d years old\n", age);
// Output: I am 25 years old
```

**Multiple Variables**
```c
int x = 5, y = 3;
printf("%d + %d = %d\n", x, y, x + y);
// Output: 5 + 3 = 8
```

**Function Return**
```c
int add(int a, int b) { return a + b; }

int main() {
    int result = add(5, 3);
    printf("Result: %d\n", result);
    // Output: Result: 8
    return 0;
}
```

**Loop Output**
```c
for (int i = 1; i <= 3; i++) {
    printf("Count: %d\n", i);
}
// Output:
// Count: 1
// Count: 2
// Count: 3
```

**Multiple Prints**
```c
printf("Line 1\n");
printf("Line 2\n");
// Output:
// Line 1
// Line 2
```

### Format Specifier Reference

| Type | Specifier | Example |
|------|-----------|---------|
| Integer | `%d` or `%i` | `printf("%d", 42);` → 42 |
| Float | `%f` | `printf("%f", 3.1);` → 3.100000 |
| Character | `%c` | `printf("%c", 'X');` → X |
| String | `%s` | `printf("%s", "Hi");` → Hi |

### Common Patterns

**Debug Output**
```c
int x = 10;
printf("DEBUG: x = %d\n", x);
```

**Formatted Table**
```c
printf("Name\tAge\tGrade\n");
printf("John\t20\tA\n");
printf("Jane\t19\tB\n");
```

**Status Messages**
```c
printf("Starting...\n");
// ... do work ...
printf("Done!\n");
```

**Warning Message**
```c
if (x < 0) {
    printf("WARNING: x is negative!\n");
}
```

### Common Mistakes ❌

| Mistake | Problem | Fix |
|---------|---------|-----|
| No `#include` | `printf` undefined | Add `#include <stdio.h>` |
| Wrong format | `3.5` printed as `3` | Use `%f` for float, `%d` for int |
| Missing `\n` | Output on one line | Add `\n` for newline |
| No quotes | Compilation error | Use `printf("text", var);` |

### Using in CompilerIDE

1. **Write code with printf**
2. **Click Build & Run** (Ctrl+B)  
3. **Check Console tab** for output
4. Output shows as GREEN text

### Tips
- Always include `#include <stdio.h>` at the top
- Always use `\n` to end lines
- Match format specifier to variable type
- Use descriptive messages
- Can mix text and variables in one printf

### More Info
See **PRINTF_GUIDE.md** for detailed documentation
