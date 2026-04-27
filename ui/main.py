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
import threading
from queue import Queue

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
        # Index in the console text where user input may begin (protect earlier output)
        self.console_lock_index = "1.0"
        
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
        ttk.Button(toolbar, text="🔨 Build", width=12, command=self.build_only).pack(side=tk.LEFT, padx=2)
        # ttk.Button(toolbar, text="▶️ Build & Run", width=12, command=self.build_and_run).pack(side=tk.LEFT, padx=2)
        
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
            console_frame, font=('Courier New', 10), state=tk.NORMAL,
            background='#1e1e1e', foreground='#00FF00'
        )
        self.console_text.config(insertbackground="#00FF00")
        self.console_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        # Bind console for inline input (user types directly into the console)
        self.console_text.bind('<KeyPress>', self.on_console_keypress)
        self.console_text.bind('<Button-1>', self.on_console_click)
        # Ensure copy/paste still works
        self.console_text.bind('<Control-c>', lambda e: self.console_text.event_generate('<<Copy>>'))
        self.console_text.bind('<Control-v>', lambda e: self.console_text.event_generate('<<Paste>>'))
        
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
        """Run a specific compiler phase (in background thread)"""
        if not self.current_file:
            messagebox.showwarning("No File", "Please save your file first")
            return
        
        if self.file_modified:
            self.save_file()
        
        # Run in background thread to prevent UI freezing
        thread = threading.Thread(target=self._run_phase_thread, args=(phase,), daemon=True)
        thread.start()
    
    def _run_phase_thread(self, phase):
        """Background thread for running a specific phase"""
        try:
            self.root.after(0, lambda p=phase: self.update_status(f"Running {p}..."))
            self.root.after(0, self.clear_output)
            
            result = self.compiler.run_phase(self.current_file, phase)
            self.root.after(0, lambda p=phase, r=result: self.display_phase_output(p, r))
        except Exception as e:
            self.root.after(0, lambda err=str(e): self.append_output(f"ERROR: {err}", "error"))
            
    def build_all(self, run_after=False):
        """Run all phases in sequence (in background thread)

        Args:
            run_after: if True, run the program (--link) after build regardless of build success
        """
        if not self.current_file:
            messagebox.showwarning("No File", "Please save your file first")
            return

        if self.file_modified:
            self.save_file()

        # Run in background thread to prevent UI freezing
        thread = threading.Thread(target=self._build_all_thread, args=(run_after,), daemon=True)
        thread.start()
    
    def _build_all_thread(self, run_after=False):
        """Background thread for building all phases"""
        try:
            # Update UI on main thread
            self.root.after(0, self.clear_output)
            
            phases = ["--lex", "--parse", "--semantic", "--icg", "--optimize", "--codegen"]
            
            build_success = True
            for phase in phases:
                self.root.after(0, lambda p=phase: self.update_status(f"Building with {p}..."))
                try:
                    result = self.compiler.run_phase(self.current_file, phase)
                    self.root.after(0, lambda p=phase, r=result: self.display_phase_output(p, r))
                    
                    if not result['success'] and phase in ["--lex", "--parse", "--semantic"]:
                        build_success = False
                        break
                except Exception as e:
                    self.root.after(0, lambda p=phase, err=str(e): self.append_output(f"ERROR in {p}: {err}", "error"))
                    build_success = False
                    break
            # Run program automatically after successful build, or if requested regardless
            if build_success:
                self.root.after(0, lambda: self.update_status("Build complete - Running program..."))
                # Schedule program run on main thread
                self.root.after(500, self.run_program)
            else:
                if run_after:
                    self.root.after(0, lambda: self.update_status("Build failed, running program (--link) anyway..."))
                    self.root.after(500, self.run_program)
                else:
                    self.root.after(0, lambda: self.update_status("Build failed"))
        except Exception as e:
            self.root.after(0, lambda: self.update_status(f"Build error: {str(e)}"))
        
    def display_phase_output(self, phase, result):
        """Display output from a compiler phase"""
        if result['stderr']:
            self.append_output(f"\n[{phase}] Errors/Warnings:\n", "info")
            self.append_output(result['stderr'], "error" if "ERROR" in result['stderr'] else "warning")
        
        if result['stdout']:
            self.append_output(f"\n[{phase}] Output:\n", "info")
            output = result['stdout']
            # Try to parse JSON output; strip any leading non-JSON text
            def extract_json_text(s):
                first = s.find('{')
                last = s.rfind('}')
                if first != -1 and last != -1 and last > first:
                    return s[first:last+1]
                return s

            json_text = extract_json_text(output)
            try:
                data = json.loads(json_text)
                self.append_output(json.dumps(data, indent=2), "info")

                # Display in appropriate tab
                if phase == "--codegen" and 'asm' in json_text:
                    self.display_assembly(json_text)
                elif phase in ["--icg", "--optimize"] and 'tac' in json_text:
                    self.display_tac(json_text)
                elif phase == "--semantic" and 'symbols' in json_text:
                    self.display_symbols(json_text)
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
        """Clear console output and terminate current execution"""
        # Terminate any running compiler/process
        try:
            self.compiler.terminate_current()
        except Exception:
            pass

        self.console_text.delete(1.0, tk.END)
        # reset input lock index
        self.console_lock_index = "1.0"

        self.update_status("Console cleared")

        # Notify user
        self.append_console("Execution terminated.", "info")
    
    def append_console(self, text, tag="output"):
        """Append text to console"""
        # Insert text at the end and keep the console editable for inline input
        self.console_text.insert(tk.END, text, tag)
        self.console_text.see(tk.END)
        # Update the protected index so earlier output cannot be edited
        try:
            self.console_lock_index = self.console_text.index(tk.END + "-1c")
        except Exception:
            self.console_lock_index = self.console_text.index(tk.END)
    
    def run_program(self):
        """Run the compiled program and show output (in background thread)"""
        if not self.current_file:
            messagebox.showwarning("No File", "Please save your file first")
            return
        
        # Run in background thread to prevent UI freezing
        thread = threading.Thread(target=self._run_program_thread, daemon=True)
        thread.start()
    
    def _run_program_thread(self):
        """Background thread for running program"""
        try:
            # Clear console on main thread
            self.root.after(0, self.clear_console)
            self.root.after(0, lambda: self.append_console("=" * 60 + "\n", "info"))
            self.root.after(0, lambda: self.append_console("Program Output:\n", "info"))
            self.root.after(0, lambda: self.append_console("=" * 60 + "\n\n", "info"))
            # Start program in async mode with streaming callbacks
            def stdout_cb(line):
                self.root.after(0, lambda l=line: self.append_console(l, 'output'))

            def stderr_cb(line):
                self.root.after(0, lambda l=line: self.append_console(l, 'error'))

            def exit_cb(returncode):
                self.root.after(0, lambda: self.append_console("\n" + "=" * 60 + "\n", "info"))
                self.root.after(0, lambda: self.append_console(f"Program finished (exit code {returncode})\n", "info"))
                self.root.after(0, lambda: self.output_tabs.select(4))  # Switch to Console tab

            res = self.compiler.start_program(self.current_file, stdout_callback=stdout_cb, stderr_callback=stderr_cb, exit_callback=exit_cb)

            # If compilation failed, show compile stderr
            if not res.get('started'):
                err = res.get('stderr', '')
                if err:
                    self.root.after(0, lambda e=err: self.append_console(e, 'error'))
                self.root.after(0, lambda: self.append_console("\n" + "=" * 60 + "\n", "info"))
                return
            # ensure input area is at the end and locked to not permit editing earlier output
            try:
                self.console_text.mark_set(tk.INSERT, tk.END)
                self.console_lock_index = self.console_text.index(tk.END + "-1c")
            except Exception:
                pass
        except Exception as e:
            self.root.after(0, lambda: self.append_console(f"Error: {str(e)}\n", "error"))

    def _index_before(self, idx1, idx2):
        """Return True if idx1 < idx2 in text index order."""
        try:
            l1, c1 = map(int, str(idx1).split('.'))
            l2, c2 = map(int, str(idx2).split('.'))
            return (l1 < l2) or (l1 == l2 and c1 < c2)
        except Exception:
            return False

    def on_console_click(self, event):
        self.console_text.focus_set()
        self.console_text.mark_set(tk.INSERT, tk.END)
        self.console_text.see(tk.END)
        return "break"

    def on_console_keypress(self, event):
        """Handle inline console keypresses: prevent editing before lock, handle Enter to send input."""
        try:
            # Protect earlier output: if caret is before lock index, move it to end
            if not self.console_text.compare(tk.INSERT, '>=', self.console_lock_index):
                self.console_text.mark_set(tk.INSERT, tk.END)
                return 'break'
        except Exception:
            pass

        # Handle Enter: send the current input line(s) from lock index
        if event.keysym == 'Return':
            try:
                input_text = self.console_text.get(self.console_lock_index, tk.END)
                # strip trailing newline inserted by user if any
                if input_text.endswith('\n'):
                    input_text = input_text[:-1]
                self.console_text.insert(tk.END, '\n')
                sent = False
                try:
                    sent = self.compiler.send_input(input_text + '\n')
                except Exception as e:
                    self.append_console(f"Send input error: {e}\n", 'error')

                if not sent:
                    self.append_console("No running program to send input to.\n", 'error')
                # Ensure newline present as echo
                if not self.console_text.get(self.console_lock_index, tk.END).endswith('\n'):
                    self.console_text.insert(tk.END, '\n')
                self.console_text.see(tk.END)
                # update lock index to new end
                self.console_lock_index = self.console_text.index(tk.END + '-1c')
            except Exception:
                pass
            return 'break'

        # Prevent BackSpace / Left moving before lock
        if event.keysym in ('BackSpace', 'Left', 'Home'):
            try:
                if not self.console_text.compare(tk.INSERT, '>', self.console_lock_index):
                    return 'break'
            except Exception:
                return 'break'

        # Allow other keys
        return

    def send_console_input(self):
        """Send the text in the console input entry to the running program's stdin."""
        # legacy entry removed; keep method for compatibility but do nothing
        return

    def build_only(self):
        """Compile the current file (compile-only)"""
        if not self.current_file:
            messagebox.showwarning("No File", "Please save your file first")
            return

        if self.file_modified:
            self.save_file()

        thread = threading.Thread(target=self._build_only_thread, daemon=True)
        thread.start()

    def _build_only_thread(self):
        try:
            self.root.after(0, lambda: self.update_status("Compiling (build)..."))
            self.root.after(0, self.clear_output)

            result = self.compiler.compile_file(self.current_file)

            # Show compile stderr (warnings/errors)
            if result.get('stderr'):
                self.root.after(0, lambda s=result.get('stderr'): self.append_output(s, 'error'))

            # Show compile stdout
            if result.get('stdout'):
                self.root.after(0, lambda s=result.get('stdout'): self.append_output(s, 'info'))

            if result.get('success'):
                self.root.after(0, lambda: self.update_status("Build succeeded"))
            else:
                self.root.after(0, lambda: self.update_status("Build failed"))
        except Exception as e:
            self.root.after(0, lambda: self.append_output(f"Build error: {str(e)}", 'error'))

    # def build_and_run(self):
    #     """Compile the current file and run it if compilation succeeds"""
    #     if not self.current_file:
    #         messagebox.showwarning("No File", "Please save your file first")
    #         return

    #     if self.file_modified:
    #         self.save_file()

    #     thread = threading.Thread(target=self._build_and_run_thread, daemon=True)
    #     thread.start()

    # def _build_and_run_thread(self):
    #     try:
    #         self.root.after(0, lambda: self.update_status("Building and running..."))
    #         self.root.after(0, self.clear_output)

    #         # Use the bridge's compile+run helper which compiles and runs in one cancellable step
    #         result = self.compiler.run_link(self.current_file)

    #         # Show stderr (compile or runtime errors)
    #         if result.get('stderr'):
    #             self.root.after(0, lambda s=result.get('stderr'): self.append_output(s, 'error'))

    #         # Try to parse JSON stdout
    #         stdout = result.get('stdout', '')
    #         if stdout:
    #             first = stdout.find('{')
    #             last = stdout.rfind('}')
    #             json_text = stdout[first:last+1] if (first != -1 and last != -1 and last > first) else stdout
    #             try:
    #                 data = json.loads(json_text)
    #                 out_text = data.get('output', '')
    #                 if out_text:
    #                     self.root.after(0, lambda t=out_text: self.append_console(t, 'output'))
    #                 else:
    #                     if not data.get('success', False):
    #                         err = data.get('error', '')
    #                         if err:
    #                             self.root.after(0, lambda e=err: self.append_console(e, 'error'))
    #             except json.JSONDecodeError:
    #                 self.root.after(0, lambda s=stdout: self.append_console(s, 'output'))
    #     except Exception as e:
    #         self.root.after(0, lambda: self.append_output(f"Build & Run error: {str(e)}", 'error'))
    
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
        
        # If it's an expression (with or without parentheses), try to evaluate it
        # Check if it contains operators or is wrapped in parentheses
        if any(op in arg for op in ['+', '-', '*', '/', '%']) or re.match(r'\(.*\)', arg):
            # Remove outer parentheses if present
            expr = arg
            expr_match = re.match(r'\((.*)\)', expr)
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
