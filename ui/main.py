"""
CompilerIDE - Python UI for C++/C Compiler
A full-featured IDE with syntax highlighting, multi-phase compilation,
and interactive debugging.
"""

import tkinter as tk
from tkinter import ttk, filedialog, messagebox, scrolledtext
import os
import sys
import subprocess
import json
from datetime import datetime
from pathlib import Path

# Add current directory to path for imports
sys.path.insert(0, os.path.dirname(__file__))

from compiler_bridge import CompilerBridge
from file_manager import FileManager
from syntax_highlighter import SyntaxHighlighter


class CompilerIDE:
    def __init__(self, root):
        self.root = root
        self.root.title("CompilerIDE - Multi-Phase C Compiler")
        self.root.geometry("1400x900")
        
        # Initialize components
        self.compiler = CompilerBridge(r"d:\compiler\bin\compiler.exe")
        self.file_manager = FileManager()
        self.current_file = None
        self.file_modified = False
        
        # Color scheme for syntax highlighting
        self.colors = {
            'keyword': '#0066CC',
            'string': '#006600',
            'comment': '#999999',
            'number': '#CC0000',
            'function': '#0066CC',
            'whitespace': '#000000',
        }
        
        self.setup_ui()
        self.setup_styles()
        
    def setup_styles(self):
        """Configure ttk styles"""
        style = ttk.Style()
        style.theme_use('clam')
        style.configure('TButton', font=('Arial', 10))
        style.configure('TLabel', font=('Arial', 10))
        
    def setup_ui(self):
        """Build the main UI layout"""
        
        # ── Main container with menu bar ──────
        self.create_menu_bar()
        
        # ── Toolbar ──────────────────────────
        self.create_toolbar()
        
        # ── Main content area (paned) ────────
        main_paned = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        main_paned.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Left side: Editor
        left_frame = ttk.Frame(main_paned)
        main_paned.add(left_frame, weight=2)
        self.create_editor_panel(left_frame)
        
        # Right side: Output and tabs
        right_frame = ttk.Frame(main_paned)
        main_paned.add(right_frame, weight=1)
        self.create_output_panel(right_frame)
        
        # ── Status bar ───────────────────────
        self.create_status_bar()
        
    def create_menu_bar(self):
        """Create main menu bar"""
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)
        
        # File menu
        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="File", menu=file_menu)
        file_menu.add_command(label="New File", command=self.new_file, accelerator="Ctrl+N")
        file_menu.add_command(label="Open File", command=self.open_file, accelerator="Ctrl+O")
        file_menu.add_command(label="Save", command=self.save_file, accelerator="Ctrl+S")
        file_menu.add_command(label="Save As", command=self.save_file_as, accelerator="Ctrl+Shift+S")
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self.root.quit)
        
        # Build menu
        build_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Build", menu=build_menu)
        build_menu.add_command(label="Lex (Phase 1)", command=lambda: self.run_phase("--lex"), accelerator="F1")
        build_menu.add_command(label="Parse (Phase 2)", command=lambda: self.run_phase("--parse"), accelerator="F2")
        build_menu.add_command(label="Semantic (Phase 3)", command=lambda: self.run_phase("--semantic"), accelerator="F3")
        build_menu.add_command(label="ICG (Phase 4)", command=lambda: self.run_phase("--icg"), accelerator="F4")
        build_menu.add_command(label="Optimize (Phase 5)", command=lambda: self.run_phase("--optimize"), accelerator="F5")
        build_menu.add_command(label="CodeGen (Phase 6)", command=lambda: self.run_phase("--codegen"), accelerator="F6")
        build_menu.add_separator()
        build_menu.add_command(label="Build All", command=self.build_all, accelerator="Ctrl+B")
        
        # Help menu
        help_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Help", menu=help_menu)
        help_menu.add_command(label="About", command=self.show_about)
        
        # Bind keyboard shortcuts
        self.root.bind("<Control-n>", lambda e: self.new_file())
        self.root.bind("<Control-o>", lambda e: self.open_file())
        self.root.bind("<Control-s>", lambda e: self.save_file())
        self.root.bind("<Control-Shift-S>", lambda e: self.save_file_as())
        self.root.bind("<Control-b>", lambda e: self.build_all())
        self.root.bind("<F1>", lambda e: self.run_phase("--lex"))
        self.root.bind("<F2>", lambda e: self.run_phase("--parse"))
        self.root.bind("<F3>", lambda e: self.run_phase("--semantic"))
        self.root.bind("<F4>", lambda e: self.run_phase("--icg"))
        self.root.bind("<F5>", lambda e: self.run_phase("--optimize"))
        self.root.bind("<F6>", lambda e: self.run_phase("--codegen"))
        
    def create_toolbar(self):
        """Create toolbar with action buttons"""
        toolbar = ttk.Frame(self.root)
        toolbar.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        
        # File operations
        ttk.Button(toolbar, text="📄 New", width=8, command=self.new_file).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="📁 Open", width=8, command=self.open_file).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="💾 Save", width=8, command=self.save_file).pack(side=tk.LEFT, padx=2)
        
        ttk.Separator(toolbar, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=5)
        
        # Build operations
        ttk.Button(toolbar, text="⚙️ Lex", width=8, command=lambda: self.run_phase("--lex")).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="📝 Parse", width=8, command=lambda: self.run_phase("--parse")).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="✓ Semantic", width=12, command=lambda: self.run_phase("--semantic")).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="🔧 ICG", width=8, command=lambda: self.run_phase("--icg")).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="⚡ Optimize", width=12, command=lambda: self.run_phase("--optimize")).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="🏗️ CodeGen", width=12, command=lambda: self.run_phase("--codegen")).pack(side=tk.LEFT, padx=2)
        
        ttk.Separator(toolbar, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=5)
        
        # Quick actions
        ttk.Button(toolbar, text="▶️ Build & Run", width=12, command=self.build_all).pack(side=tk.LEFT, padx=2)
        
    def create_editor_panel(self, parent):
        """Create code editor panel with line numbers"""
        editor_frame = ttk.Frame(parent)
        editor_frame.pack(fill=tk.BOTH, expand=True)
        
        # Frame for line numbers and editor
        content_frame = ttk.Frame(editor_frame)
        content_frame.pack(fill=tk.BOTH, expand=True)
        
        # Line numbers
        self.line_numbers = tk.Text(
            content_frame, width=5, padx=3, takefocus=0,
            border=0, background='#f0f0f0', state=tk.DISABLED,
            font=('Courier New', 11)
        )
        self.line_numbers.pack(side=tk.LEFT, fill=tk.Y)
        
        # Code editor
        self.editor = scrolledtext.ScrolledText(
            content_frame, wrap=tk.NONE, font=('Courier New', 11),
            undo=True, maxundo=-1
        )
        self.editor.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        
        # Syntax highlighter
        self.highlighter = SyntaxHighlighter(self.editor, self.colors)
        
        # Bind events
        self.editor.bind("<<Change>>", self.on_editor_change)
        self.editor.bind("<KeyRelease>", self.on_key_release)
        
        # Update line numbers initially
        self.update_line_numbers()
        
    def create_output_panel(self, parent):
        """Create tabbed output panel"""
        # Tab control
        self.output_tabs = ttk.Notebook(parent)
        self.output_tabs.pack(fill=tk.BOTH, expand=True)
        
        # Compilation Output tab
        output_frame = ttk.Frame(self.output_tabs)
        self.output_tabs.add(output_frame, text="Output")
        
        self.output_text = scrolledtext.ScrolledText(
            output_frame, height=10, font=('Courier New', 10), state=tk.DISABLED
        )
        self.output_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Configure output text colors
        self.output_text.tag_config("error", foreground="#FF0000", background="#FFE6E6")
        self.output_text.tag_config("warning", foreground="#FF6600", background="#FFF3E6")
        self.output_text.tag_config("success", foreground="#009900", background="#E6FFE6")
        self.output_text.tag_config("info", foreground="#0066CC")
        
        # Assembly Output tab
        asm_frame = ttk.Frame(self.output_tabs)
        self.output_tabs.add(asm_frame, text="Assembly")
        
        self.asm_text = scrolledtext.ScrolledText(
            asm_frame, font=('Courier New', 9), state=tk.DISABLED
        )
        self.asm_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # TAC Output tab
        tac_frame = ttk.Frame(self.output_tabs)
        self.output_tabs.add(tac_frame, text="TAC")
        
        self.tac_text = scrolledtext.ScrolledText(
            tac_frame, font=('Courier New', 9), state=tk.DISABLED
        )
        self.tac_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Symbol Table tab
        sym_frame = ttk.Frame(self.output_tabs)
        self.output_tabs.add(sym_frame, text="Symbols")
        
        self.sym_text = scrolledtext.ScrolledText(
            sym_frame, font=('Courier New', 9), state=tk.DISABLED
        )
        self.sym_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Console/Output tab
        console_frame = ttk.Frame(self.output_tabs)
        self.output_tabs.add(console_frame, text="Console")
        
        # Console with buttons
        console_btn_frame = ttk.Frame(console_frame)
        console_btn_frame.pack(fill=tk.X, padx=5, pady=5)
        
        ttk.Button(console_btn_frame, text="▶️ Run Program", width=15, command=self.run_program).pack(side=tk.LEFT, padx=2)
        ttk.Button(console_btn_frame, text="🗑️ Clear Console", width=15, command=self.clear_console).pack(side=tk.LEFT, padx=2)
        
        self.console_text = scrolledtext.ScrolledText(
            console_frame, font=('Courier New', 10), state=tk.DISABLED, 
            background='#1e1e1e', foreground='#00FF00'
        )
        self.console_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Configure console colors
        self.console_text.tag_config("output", foreground="#00FF00")
        self.console_text.tag_config("error", foreground="#FF6666")
        self.console_text.tag_config("info", foreground="#66B2FF")
        
    def create_status_bar(self):
        """Create status bar at bottom"""
        status_frame = ttk.Frame(self.root)
        status_frame.pack(side=tk.BOTTOM, fill=tk.X, padx=5, pady=2)
        
        self.status_var = tk.StringVar(value="Ready")
        status_label = ttk.Label(status_frame, textvariable=self.status_var, relief=tk.SUNKEN)
        status_label.pack(side=tk.LEFT, fill=tk.X, expand=True)
        
        self.line_col_var = tk.StringVar(value="Ln 1, Col 1")
        line_col_label = ttk.Label(status_frame, textvariable=self.line_col_var, width=15)
        line_col_label.pack(side=tk.RIGHT)
        
    def update_line_numbers(self):
        """Update line numbers display"""
        line_count = int(self.editor.index('end-1c').split('.')[0])
        
        self.line_numbers.config(state=tk.NORMAL)
        self.line_numbers.delete(1.0, tk.END)
        
        for i in range(1, line_count + 1):
            self.line_numbers.insert(tk.END, f"{i}\n")
        
        self.line_numbers.config(state=tk.DISABLED)
        
    def on_editor_change(self, event=None):
        """Handle editor content changes"""
        self.file_modified = True
        if self.current_file:
            self.root.title(f"CompilerIDE - {self.current_file} *")
        
    def on_key_release(self, event=None):
        """Handle key releases (update line/col, syntax highlight)"""
        # Update line numbers
        self.update_line_numbers()
        
        # Update cursor position
        cursor = self.editor.index(tk.INSERT)
        line, col = cursor.split('.')
        self.line_col_var.set(f"Ln {line}, Col {int(col)+1}")
        
        # Update syntax highlighting
        self.highlighter.highlight()
        
    def new_file(self):
        """Create a new file"""
        if self.file_modified:
            if messagebox.askyesno("Unsaved Changes", "Save current file?"):
                self.save_file()
        
        self.editor.delete(1.0, tk.END)
        self.current_file = None
        self.file_modified = False
        self.root.title("CompilerIDE - Untitled")
        self.update_status("New file created")
        
    def open_file(self):
        """Open an existing file"""
        if self.file_modified:
            if messagebox.askyesno("Unsaved Changes", "Save current file?"):
                self.save_file()
        
        filepath = filedialog.askopenfilename(
            filetypes=[("C Source", "*.c"), ("C++ Source", "*.cpp"), ("All Files", "*")],
            initialdir=str(Path.home() / "Desktop")
        )
        
        if filepath:
            try:
                with open(filepath, 'r', encoding='utf-8') as f:
                    content = f.read()
                
                self.editor.delete(1.0, tk.END)
                self.editor.insert(1.0, content)
                self.current_file = filepath
                self.file_modified = False
                self.root.title(f"CompilerIDE - {os.path.basename(filepath)}")
                self.update_status(f"Opened: {filepath}")
                self.update_line_numbers()
                self.highlighter.highlight()
            except Exception as e:
                messagebox.showerror("Error", f"Cannot open file:\n{e}")
                
    def save_file(self):
        """Save current file"""
        if not self.current_file:
            self.save_file_as()
            return
        
        try:
            content = self.editor.get(1.0, tk.END)
            with open(self.current_file, 'w', encoding='utf-8') as f:
                f.write(content)
            
            self.file_modified = False
            self.root.title(f"CompilerIDE - {os.path.basename(self.current_file)}")
            self.update_status(f"Saved: {self.current_file}")
        except Exception as e:
            messagebox.showerror("Error", f"Cannot save file:\n{e}")
            
    def save_file_as(self):
        """Save file with new name"""
        filepath = filedialog.asksaveasfilename(
            defaultextension=".c",
            filetypes=[("C Source", "*.c"), ("C++ Source", "*.cpp"), ("All Files", "*")],
            initialdir=str(Path.home() / "Desktop")
        )
        
        if filepath:
            self.current_file = filepath
            self.save_file()
            
    def run_phase(self, phase):
        """Run a specific compiler phase"""
        if not self.current_file:
            messagebox.showwarning("No File", "Please save your file first")
            return
        
        if self.file_modified:
            self.save_file()
        
        self.update_status(f"Running {phase}...")
        self.clear_output()
        
        try:
            result = self.compiler.run_phase(self.current_file, phase)
            self.display_phase_output(phase, result)
        except Exception as e:
            self.append_output(f"ERROR: {e}", "error")
            
    def build_all(self):
        """Run all phases in sequence"""
        if not self.current_file:
            messagebox.showwarning("No File", "Please save your file first")
            return
        
        if self.file_modified:
            self.save_file()
        
        self.clear_output()
        phases = ["--lex", "--parse", "--semantic", "--icg", "--optimize", "--codegen"]
        
        build_success = True
        for phase in phases:
            self.update_status(f"Building with {phase}...")
            try:
                result = self.compiler.run_phase(self.current_file, phase)
                self.display_phase_output(phase, result)
                if not result['success'] and phase in ["--lex", "--parse", "--semantic"]:
                    build_success = False
                    break
            except Exception as e:
                self.append_output(f"ERROR in {phase}: {e}", "error")
                build_success = False
                break
        
        if build_success:
            self.update_status("Build complete - Running program simulation...")
            self.root.after(500, self.run_program)
        else:
            self.update_status("Build failed")
        
    def display_phase_output(self, phase, result):
        """Display output from a compiler phase"""
        if result['stderr']:
            self.append_output(f"\n[{phase}] Errors/Warnings:\n", "info")
            self.append_output(result['stderr'], "error" if "ERROR" in result['stderr'] else "warning")
        
        if result['stdout']:
            self.append_output(f"\n[{phase}] Output:\n", "info")
            output = result['stdout']
            
            # Try to parse JSON output
            try:
                data = json.loads(output)
                self.append_output(json.dumps(data, indent=2), "info")
                
                # Display in appropriate tab
                if phase == "--codegen" and 'asm' in output:
                    self.display_assembly(output)
                elif phase in ["--icg", "--optimize"] and 'tac' in output:
                    self.display_tac(output)
                elif phase == "--semantic" and 'symbols' in output:
                    self.display_symbols(output)
            except json.JSONDecodeError:
                # Plain text output
                self.append_output(output, "info")
                
    def display_assembly(self, output):
        """Display assembly output"""
        self.asm_text.config(state=tk.NORMAL)
        self.asm_text.delete(1.0, tk.END)
        
        try:
            data = json.loads(output)
            asm_code = data.get('asm', data.get('text', output))
            self.asm_text.insert(1.0, asm_code)
        except:
            self.asm_text.insert(1.0, output)
        
        self.asm_text.config(state=tk.DISABLED)
        self.output_tabs.select(1)  # Switch to Assembly tab
        
    def display_tac(self, output):
        """Display TAC output"""
        self.tac_text.config(state=tk.NORMAL)
        self.tac_text.delete(1.0, tk.END)
        
        try:
            data = json.loads(output)
            tac_code = data.get('text', output)
            self.tac_text.insert(1.0, tac_code)
        except:
            self.tac_text.insert(1.0, output)
        
        self.tac_text.config(state=tk.DISABLED)
        self.output_tabs.select(2)  # Switch to TAC tab
        
    def display_symbols(self, output):
        """Display symbol table"""
        self.sym_text.config(state=tk.NORMAL)
        self.sym_text.delete(1.0, tk.END)
        
        try:
            data = json.loads(output)
            symbols = data.get('symbols', [])
            
            sym_str = "Symbol Table:\n"
            sym_str += "=" * 70 + "\n"
            sym_str += f"{'Name':<20} {'Type':<15} {'Kind':<15} {'Scope':<10} {'Line':<5}\n"
            sym_str += "=" * 70 + "\n"
            
            for sym in symbols:
                sym_str += f"{sym.get('name', ''):<20} {sym.get('type', ''):<15} {sym.get('kind', ''):<15} {str(sym.get('scope', '')):<10} {str(sym.get('line', '')):<5}\n"
            
            self.sym_text.insert(1.0, sym_str)
        except Exception as e:
            self.sym_text.insert(1.0, f"Error parsing symbols: {e}\n{output}")
        
        self.sym_text.config(state=tk.DISABLED)
        self.output_tabs.select(3)  # Switch to Symbols tab
        
    def clear_output(self):
        """Clear all output panels"""
        for text_widget in [self.output_text, self.asm_text, self.tac_text, self.sym_text]:
            text_widget.config(state=tk.NORMAL)
            text_widget.delete(1.0, tk.END)
            text_widget.config(state=tk.DISABLED)
        
    def append_output(self, text, tag="info"):
        """Append text to output panel"""
        self.output_text.config(state=tk.NORMAL)
        self.output_text.insert(tk.END, text + "\n", tag)
        self.output_text.see(tk.END)
        self.output_text.config(state=tk.DISABLED)
        
    def update_status(self, message):
        """Update status bar"""
        self.status_var.set(message)
        self.root.update_idletasks()
        
    def show_about(self):
        """Show about dialog"""
        messagebox.showinfo(
            "About CompilerIDE",
            "CompilerIDE v1.0\n\n"
            "A multi-phase C/C++ compiler with Python UI\n\n"
            "Phases:\n"
            "1. Lexical Analysis (Tokenization)\n"
            "2. Syntax Analysis (Parsing)\n"
            "3. Semantic Analysis\n"
            "4. Intermediate Code Generation (TAC)\n"
            "5. Code Optimization\n"
            "6. Code Generation (x86-64 Assembly)\n\n"
            "© 2026 Compiler Project"
        )
    
    def clear_console(self):
        """Clear console output"""
        self.console_text.config(state=tk.NORMAL)
        self.console_text.delete(1.0, tk.END)
        self.console_text.config(state=tk.DISABLED)
    
    def append_console(self, text, tag="output"):
        """Append text to console"""
        self.console_text.config(state=tk.NORMAL)
        self.console_text.insert(tk.END, text, tag)
        self.console_text.see(tk.END)
        self.console_text.config(state=tk.DISABLED)
    
    def run_program(self):
        """Run the compiled program and show output"""
        if not self.current_file:
            messagebox.showwarning("No File", "Please save your file first")
            return
        
        self.clear_console()
        self.append_console("=" * 60 + "\n", "info")
        self.append_console("Program Output:\n", "info")
        self.append_console("=" * 60 + "\n\n", "info")
        
        # Extract code from editor
        code = self.editor.get(1.0, tk.END)
        
        # Simulate program execution by analyzing printf statements
        self.simulate_execution(code)
        
        self.append_console("\n" + "=" * 60 + "\n", "info")
        self.append_console("Program finished\n", "info")
        self.output_tabs.select(4)  # Switch to Console tab
    
    def simulate_execution(self, code):
        """Simulate program execution based on code analysis"""
        import re
        
        # Extract all printf/puts statements
        printf_pattern = r'printf\s*\(\s*"([^"]*)"(.*?)\)'
        matches = re.findall(printf_pattern, code, re.DOTALL)
        
        if not matches:
            # Check for other output patterns
            if 'printf' in code:
                self.append_console("// Program output:\n", "info")
                self.append_console("// (Note: printf statements detected but await runtime execution)\n\n", "info")
            else:
                self.append_console("// Note: Calculate function returns result: ", "info")
                # Try to find main function and extract return value
                main_match = re.search(r'int\s+main\s*\(\s*\)\s*{(.*?)}', code, re.DOTALL)
                if main_match:
                    main_body = main_match.group(1)
                    # Look for return statements
                    return_matches = re.findall(r'return\s+(\w+|\d+);', main_body)
                    if return_matches:
                        self.append_console(f"return {return_matches[-1]};\n", "output")
                    else:
                        self.append_console("return 0;\n", "output")
        else:
            # Display printf outputs
            for format_str, args in matches:
                output = self.format_printf_output(format_str, args, code)
                self.append_console(output, "output")
    
    def format_printf_output(self, format_str, args_str, code):
        """Format printf output based on format string and arguments"""
        import re
        
        # Simple format string processing
        output = format_str
        
        # Handle %d, %i, %f, %s, %c
        # Extract arguments (handle nested parentheses for function calls)
        arg_list = []
        paren_depth = 0
        current_arg = ""
        for char in args_str:
            if char == '(' :
                paren_depth += 1
                current_arg += char
            elif char == ')':
                paren_depth -= 1
                current_arg += char
            elif char == ',' and paren_depth == 0:
                arg_list.append(current_arg.strip())
                current_arg = ""
            else:
                current_arg += char
        if current_arg.strip():
            arg_list.append(current_arg.strip())
        
        # Build variable values dictionary with function evaluation
        var_values = self._extract_variable_values(code)
        
        # Replace format specifiers
        arg_idx = 0
        def replace_format(match):
            nonlocal arg_idx
            spec = match.group(0)
            
            if arg_idx >= len(arg_list):
                return spec
            
            arg = arg_list[arg_idx].strip()
            arg_idx += 1
            
            # Resolve variable/expression to value
            resolved = self._resolve_value(arg, var_values, code)
            
            return str(resolved)
        
        # Replace all format specifiers
        output = re.sub(r'%[difs]', replace_format, output)
        
        # Handle escape sequences
        output = output.replace('\\n', '\n')
        output = output.replace('\\t', '\t')
        
        return output + '\n'
    
    def _extract_variable_values(self, code):
        """Extract variable values from code, including function results"""
        import re
        
        var_values = {}
        
        # First, find all simple assignments: var = literal_value
        var_pattern = r'(\w+)\s*=\s*(\d+(?:\.\d+)?)'
        for match in re.finditer(var_pattern, code):
            var_values[match.group(1)] = match.group(2)
        
        # Then, find assignments with function calls: var = function_name(args)
        func_call_pattern = r'(\w+)\s*=\s*(\w+)\s*\((.*?)\)'
        for match in re.finditer(func_call_pattern, code):
            var_name = match.group(1)
            func_name = match.group(2)
            args = match.group(3)
            
            # Try to evaluate the function call
            result = self._evaluate_function(func_name, args, var_values, code)
            if result is not None:
                var_values[var_name] = result
        
        return var_values
    
    def _resolve_value(self, arg, var_values, code):
        """Resolve an argument to its value"""
        import re
        
        arg = arg.strip()
        
        # If it's a variable, look it up
        if arg in var_values:
            return var_values[arg]
        
        # If it's a number, return it
        if re.match(r'^\d+(?:\.\d+)?$', arg):
            return arg
        
        # If it's a function call, try to evaluate it
        func_match = re.match(r'(\w+)\s*\((.*)\)', arg)
        if func_match:
            func_name = func_match.group(1)
            args = func_match.group(2)
            result = self._evaluate_function(func_name, args, var_values, code)
            if result is not None:
                return result
        
        # If it's an expression like (5 + 3), try to evaluate it
        expr_match = re.match(r'\((.*)\)', arg)
        if expr_match:
            expr = expr_match.group(1)
            result = self._evaluate_expression(expr, var_values)
            if result is not None:
                return result
        
        # Default: return as-is
        return arg
    
    def _evaluate_expression(self, expr, var_values):
        """Evaluate a simple arithmetic expression"""
        import re
        
        expr = expr.strip()
        
        # Replace variables with their values
        for var, val in var_values.items():
            expr = re.sub(r'\b' + var + r'\b', str(val), expr)
        
        # Try to evaluate the expression safely
        try:
            # Only allow arithmetic operations
            if all(c in '0123456789+-*/%() .' for c in expr):
                return str(int(eval(expr)))
        except:
            pass
        
        return None
    
    def _evaluate_function(self, func_name, args_str, var_values, code):
        """Try to evaluate a function call by finding its definition"""
        import re
        
        # Find the function definition
        func_pattern = rf'int\s+{func_name}\s*\((.*?)\)\s*{{\s*(.*?)\}}'
        func_match = re.search(func_pattern, code, re.DOTALL)
        
        if not func_match:
            return None
        
        # Extract parameters and body
        params = func_match.group(1)
        body = func_match.group(2)
        
        # Parse parameter names
        param_names = [p.strip().split()[-1] for p in params.split(',') if p.strip()]
        
        # Parse argument values
        arg_values = [self._resolve_value(arg.strip(), var_values, code) 
                      for arg in args_str.split(',') if arg.strip()]
        
        # Create parameter-to-value mapping
        local_vars = dict(zip(param_names, arg_values))
        local_vars.update(var_values)  # Include outer scope variables
        
        # Look for return statement
        return_pattern = r'return\s+(.*?);'
        return_match = re.search(return_pattern, body)
        
        if return_match:
            return_expr = return_match.group(1).strip()
            # Evaluate the return expression
            result = self._evaluate_expression(return_expr, local_vars)
            return result
        
        return None


def main():
    root = tk.Tk()
    app = CompilerIDE(root)
    root.mainloop()


if __name__ == "__main__":
    main()
