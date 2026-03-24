# Quick Start Guide

## Installation & Setup

### 1. Prerequisites
- Python 3.7 or higher
- Windows, macOS, or Linux
- C++ compiler already built (`d:\compiler\bin\compiler.exe`)

### 2. Install Dependencies
```bash
cd d:\compiler\ui
pip install -r requirements.txt
```

### 3. Launch the IDE
```bash
python main.py
```

## First Steps

### 1. Create a New File
- Click **File → New File** or press `Ctrl+N`
- Or use the **📄 New** button in the toolbar

### 2. Write Code
Open the sample file or write your own C code:
```c
int main() {
    int x = 5;
    int y = 10;
    return x + y;
}
```

### 3. Save Your File
- Press `Ctrl+S` or click **💾 Save**
- Choose a location and filename (e.g., `program.c`)

### 4. Compile and Run

#### Option A: Step-by-Step Debugging
Click each phase button to see intermediate results:
1. **⚙️ Lex** (F1) - See tokens
2. **📝 Parse** (F2) - See abstract syntax tree
3. **✓ Semantic** (F3) - See symbol table
4. **🔧 ICG** (F4) - See three-address code
5. **⚡ Optimize** (F5) - See optimized instructions
6. **🏗️ CodeGen** (F6) - See x86-64 assembly

#### Option B: Full Build
Click **▶️ Build & Run** or press `Ctrl+B` to run all phases

### 5. View Results
Check the output in the tabs:
- **Output** - Compilation messages
- **Assembly** - Generated x86-64 code
- **TAC** - Three-address code
- **Symbols** - Variable/function table

## Common Tasks

### Opening an Existing File
```
File → Open File (Ctrl+O)
```

### Understanding Compilation Phases

| Phase | Purpose | Output |
|-------|---------|--------|
| Lex   | Break code into tokens | Token list |
| Parse | Build syntax tree | AST (JSON) |
| Semantic | Check types, resolve names | Symbol table |
| ICG   | Generate intermediate code | TAC instructions |
| Optimize | Improve performance | Optimized TAC |
| CodeGen | Generate machine code | x86-64 assembly |

### Fixing Compile Errors
1. Check the **Output** tab for error messages
2. Click on the line number in the error
3. Edit the code and save
4. Re-run the phase

### Viewing Generated Code
1. Select the **Assembly** tab for x86-64 code
2. Select the **TAC** tab for three-address code
3. Select the **Symbols** tab for variables/functions

## Example: Complete Workflow

### Step 1: Create sample.c
```c
int add(int a, int b) {
    return a + b;
}

int main() {
    int result = add(5, 3);
    return result;
}
```

### Step 2: Run Lex
Click **⚙️ Lex** →Output shows tokens:
```
KW_INT value=int line=1 col=1
IDENTIFIER value=add line=1 col=5
...
```

### Step 3: Run Parse
Click **📝 Parse** → Shows AST in JSON format

### Step 4: Run Semantic
Click **✓ Semantic** → Shows symbol table:
```
Name      Type     Kind      Scope  Line
add       int      function  0      1
result    int      variable  1      7
```

### Step 5: Run All Phases
Click **▶️ Build & Run** → Full compilation

### Step 6: Review Assembly
Check **Assembly** tab → x86-64 code ready for execution

## Tips & Tricks

### Keyboard Shortcuts
| Key | Action |
|-----|--------|
| Ctrl+N | New file |
| Ctrl+O | Open file |
| Ctrl+S | Save file |
| Ctrl+B | Build all |
| F1-F6 | Individual phases |

### Auto-Indent
- Code automatically indents when you press Enter
- Respects opening/closing braces

### Syntax Highlighting
- **Blue** = Keywords (int, if, while, etc.)
- **Green** = Strings
- **Red** = Numbers
- **Gray** = Comments
- **Purple** = Preprocessor directives (#include, #define)

### Recent Files
- Recently opened files are remembered in **File → Recent**
- Stored in `~/.compileride/recent.txt`

## Troubleshooting

### "Compiler not found" Error
```
Error: Compiler not found at d:\compiler\bin\compiler.exe
```
**Solution:** Build the compiler first:
```bash
cd d:\compiler
cmake -B build
cmake --build build
```

### Syntax Highlighting Not Working
- Ensure file ends with `.c` or `.cpp` extension
- Try clicking in the editor and typing more code
- Highlighting updates on key release

### Files Won't Save
- Check if file is read-only
- Ensure directory has write permissions
- Try "Save As" to a different location

### Compilation Timeout
- Current timeout is 30 seconds
- Check if compiler is stuck on large files
- Try running individual phases instead of all at once

## Getting Help

### Check Error Messages
- Red text in **Output** tab shows errors
- Orange text shows warnings
- Read the full error message

### Run Individual Phases
- Helps isolate which phase is failing
- Compare outputs between phases
- Use samples to test functionality

### Sample Files
- `sample.c` in the UI folder
- Contains examples of all C language features
- Use as reference for syntax

## Next Steps

1. **Explore Sample Code** - Open and compile `sample.c`
2. **Understand Each Phase** - Run phases individually
3. **Write Your Programs** - Create custom C source files
4. **Debug Compilation** - Use phase output to understand compilation
5. **Study Generated Code** - Review assembly to understand compilation

## Resources

- **C Language Reference** - Search "C standard library"
- **Compiler Theory** - Read about lexing, parsing, semantic analysis
- **x86-64 Assembly** - Learn assembly instruction set
- **Optimization Techniques** - Understand code optimization

---

**Happy Compiling!** 🚀
