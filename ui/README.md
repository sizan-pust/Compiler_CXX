# CompilerIDE - Multi-Phase C/C++ Compiler IDE

A full-featured Python IDE for the multi-phase C/C++ compiler with real-time syntax highlighting, integrated debugger, and visual output for all compilation phases.

## Features

### Editor
- **Syntax Highlighting** - Real-time highlighting for C/C++ keywords, strings, comments, numbers
- **Line Numbers** - Left margin with line numbers
- **Auto-indentation** - Smart indentation based on code structure
- **Keyboard Shortcuts** - Common shortcuts for productivity
  - `Ctrl+N` - New File
  - `Ctrl+O` - Open File
  - `Ctrl+S` - Save File
  - `Ctrl+Shift+S` - Save As
  - `Ctrl+B` - Build All

### Compilation Phases
Run individual or all compilation phases with a single click:

1. **Lex (F1)** - Tokenization and lexical analysis
2. **Parse (F2)** - Syntax parsing and AST generation
3. **Semantic (F3)** - Type checking and semantic analysis
4. **ICG (F4)** - Intermediate Code Generation (Three-Address Code)
5. **Optimize (F5)** - Code optimization (constant folding, DCE, strength reduction)
6. **CodeGen (F6)** - Assembly code generation (x86-64)

### Output Panels
- **Output** - Compilation messages, errors, warnings
- **Assembly** - Generated x86-64 assembly code
- **TAC** - Three-Address Code representation
- **Symbols** - Symbol table and variable information

### File Management
- New/Open/Save file operations
- Recent files list
- File backup before compilation
- Support for .c and .cpp files

## Installation

### Prerequisites
- Python 3.7+
- tkinter (usually included with Python)
- Compiled C++ compiler executable at `d:\compiler\bin\compiler.exe`

### Setup
```bash
# Optional: Install packages
pip install -r requirements.txt
```

Note: tkinter is part of Python standard library. If missing:
```bash
# On Windows (Python 3.7+)
python -m pip install --upgrade pip
# Already included

# On Linux
sudo apt-get install python3-tk

# On macOS
brew install python-tk
```

## Usage

### Running the IDE
```bash
python main.py
```

### Workflow
1. **Create or Open** a C/C++ source file
2. **Edit** code with syntax highlighting
3. **Save** file (Ctrl+S)
4. **Build** using phase buttons or menu
   - Run individual phases for step-by-step debugging
   - Click "Build & Run" for full compilation
5. **View Output** in appropriate tabs
   - Compilation errors in Output tab
   - Generated code in Assembly/TAC tabs
   - Symbol information in Symbols tab

### Example
```c
#include <stdio.h>

int add(int a, int b) {
    return a + b;
}

int main() {
    int x = 5;
    int y = 3;
    int result = add(x, y);
    printf("Result: %d\n", result);
    return 0;
}
```

## Architecture

### Components

**main.py**
- Main GUI application
- Editor interface with syntax highlighting
- Toolbar and menu bar
- Output panels with tabs

**compiler_bridge.py**
- Subprocess communication with C++ compiler
- Phase execution and result parsing
- Error handling and timeout management

**file_manager.py**
- File I/O operations
- Recent files management
- File backup and metadata

**syntax_highlighter.py**
- C/C++ keyword highlighting
- String and comment detection
- Dynamic highlighting on key release

### Communication Model
```
Python UI (tkinter)
    ↓
Subprocess.run()
    ↓
C++ Compiler (compiler.exe)
    ↓
stdout/stderr capture
    ↓
JSON/Text parsing
    ↓
Display in UI panels
```

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl+N | New File |
| Ctrl+O | Open File |
| Ctrl+S | Save File |
| Ctrl+Shift+S | Save As |
| Ctrl+B | Build All |
| F1 | Run Lex phase |
| F2 | Run Parse phase |
| F3 | Run Semantic phase |
| F4 | Run ICG phase |
| F5 | Run Optimize phase |
| F6 | Run CodeGen phase |

## Output Format

### Compilation Messages
- **Errors** (red background) - Critical issues blocking compilation
- **Warnings** (orange) - Issues that may cause problems
- **Success** (green) - Successful compilation
- **Info** (blue) - Status messages

### TAC Output
Three-address code representation:
```
func main:
    t0 = 5
    t1 = 3
    t2 = t0 + t1
    copy t2, x
    return t2
endfunc main
```

### Assembly Output
x86-64 Intel syntax:
```asm
main:
    push rbp
    mov rbp, rsp
    sub rsp, 32
    mov rax, 5
    mov rbx, 3
    add rax, rbx
    leave
    ret
```

## Troubleshooting

### Compiler not found
- Check compiler path: `d:\compiler\bin\compiler.exe`
- Ensure C++ compiler is built: `cmake --build build`

### Port already in use
- Only one IDE instance can run at a time
- Close other IDE windows

### File permission errors
- Ensure file is not read-only
- Check directory permissions

### Syntax highlighting not updating
- Try pressing F5 to refresh
- Click in editor and continue typing

## Advanced Features

### Phase-by-Phase Debugging
Run each phase individually to see intermediate results:
1. Lex → View tokens
2. Parse → View AST
3. Semantic → View symbol table
4. ICG → View TAC
5. Optimize → See optimizations
6. CodeGen → View assembly

### Error Recovery
The compiler continues through phases when possible:
- Lex errors prevent all later phases
- Parse errors prevent semantic analysis
- Semantic errors don't block optimization
- Optimization always runs on valid TAC

## Configuration

Configuration files stored in `~/.compileride/`:
- `recent.txt` - List of recently opened files
- Future: User preferences, themes, etc.

## License

Part of the Multi-Phase Compiler project (2026)

## Support

For issues, check:
1. Compiler is built and executable
2. Python version is 3.7+
3. File paths use backslashes on Windows
4. tkinter is installed
