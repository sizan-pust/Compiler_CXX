"""
Syntax Highlighter
Provides syntax highlighting for C/C++ code
"""

import re
import tkinter as tk


class SyntaxHighlighter:
    """Provides syntax highlighting for code editor"""
    
    def __init__(self, text_widget, colors):
        """
        Initialize syntax highlighter
        
        Args:
            text_widget: TextView/Text widget to highlight
            colors: Dictionary of color mappings
        """
        self.text = text_widget
        self.colors = colors
        
        # Configure tags
        self.setup_tags()
        
        # Keywords
        self.keywords = {
            'int', 'float', 'double', 'char', 'void', 'long', 'short',
            'signed', 'unsigned', 'if', 'else', 'for', 'while', 'do',
            'switch', 'case', 'default', 'break', 'continue', 'return',
            'struct', 'typedef', 'union', 'enum', 'sizeof', 'const',
            'static', 'extern', 'auto', 'register', 'volatile',
            'main', 'include', 'define'
        }
        
    def setup_tags(self):
        """Configure text tags for highlighting"""
        self.text.tag_config("keyword", foreground=self.colors.get('keyword', '#0000FF'))
        self.text.tag_config("string", foreground=self.colors.get('string', '#00AA00'))
        self.text.tag_config("comment", foreground=self.colors.get('comment', '#888888'))
        self.text.tag_config("number", foreground=self.colors.get('number', '#FF0000'))
        self.text.tag_config("function", foreground=self.colors.get('function', '#0066CC'))
        self.text.tag_config("preprocessor", foreground="#FF00FF")
        
    def highlight(self):
        """Perform syntax highlighting on entire document"""
        # Remove all tags
        for tag in ["keyword", "string", "comment", "number", "function", "preprocessor"]:
            self.text.tag_remove(tag, "1.0", "end")
        
        content = self.text.get("1.0", "end-1c")
        
        # Highlight preprocessor directives
        self.highlight_pattern(r'#\s*(include|define|ifdef|ifndef|endif|pragma)', "preprocessor")
        
        # Highlight strings
        self.highlight_pattern(r'"[^"]*"', "string")
        self.highlight_pattern(r"'[^']*'", "string")
        
        # Highlight comments
        self.highlight_pattern(r'//[^\n]*', "comment")
        self.highlight_pattern(r'/\*.*?\*/', "comment", re.DOTALL)
        
        # Highlight numbers
        self.highlight_pattern(r'\b\d+(\.\d+)?\b', "number")
        
        # Highlight keywords
        keyword_pattern = r'\b(' + '|'.join(self.keywords) + r')\b'
        self.highlight_pattern(keyword_pattern, "keyword")
        
        # Highlight function calls
        self.highlight_pattern(r'\b([a-zA-Z_]\w*)\s*(?=\()', "function")
        
    def highlight_pattern(self, pattern, tag, flags=0):
        """Highlight all occurrences of a pattern"""
        content = self.text.get("1.0", "end-1c")
        
        for match in re.finditer(pattern, content, flags):
            start = match.start()
            end = match.end()
            
            # Convert character position to line.column format
            start_idx = self.text.index(f"1.0+{start}c")
            end_idx = self.text.index(f"1.0+{end}c")
            
            self.text.tag_add(tag, start_idx, end_idx)


class IndentHelper:
    """Helper for intelligent indentation"""
    
    @staticmethod
    def get_indent_level(text, line_num):
        """Calculate indent level for a line"""
        lines = text.split('\n')
        if line_num >= len(lines):
            return 0
        
        line = lines[line_num]
        indent = len(line) - len(line.lstrip())
        return indent // 4  # Assuming 4-space indents
    
    @staticmethod
    def should_indent_next(text, line_num):
        """Check if next line should be indented"""
        lines = text.split('\n')
        if line_num >= len(lines):
            return False
        
        line = lines[line_num].strip()
        # Indent after opening braces or colons
        return line.endswith('{') or line.endswith(':')
    
    @staticmethod
    def should_dedent(text, line_num):
        """Check if current line should be dedented"""
        lines = text.split('\n')
        if line_num >= len(lines):
            return False
        
        line = lines[line_num].strip()
        # Dedent closing braces or case labels
        return line.startswith('}') or line.startswith('case ') or line.startswith('default:')
