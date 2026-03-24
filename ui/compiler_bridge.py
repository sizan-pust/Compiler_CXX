"""
Compiler Bridge
Handles communication with the C++ compiler via subprocess
"""

import subprocess
import sys
import os
from pathlib import Path


class CompilerBridge:
    """Bridge to communicate with the C++ compiler"""
    
    def __init__(self, compiler_path):
        """
        Initialize the compiler bridge
        
        Args:
            compiler_path: Path to the compiler executable
        """
        self.compiler_path = compiler_path
        self.validate_compiler()
        
    def validate_compiler(self):
        """Check if compiler exists"""
        if not os.path.exists(self.compiler_path):
            raise FileNotFoundError(f"Compiler not found: {self.compiler_path}")
        
    def run_phase(self, source_file, phase):
        """
        Run a specific compiler phase
        
        Args:
            source_file: Path to the source code file
            phase: Compiler phase (--lex, --parse, --semantic, --icg, --optimize, --codegen)
            
        Returns:
            dict with 'stdout' and 'stderr' keys
        """
        if not os.path.exists(source_file):
            raise FileNotFoundError(f"Source file not found: {source_file}")
        
        try:
            # Run compiler with specified phase
            result = subprocess.run(
                [self.compiler_path, phase, source_file],
                capture_output=True,
                text=True,
                timeout=30
            )
            
            return {
                'stdout': result.stdout,
                'stderr': result.stderr,
                'returncode': result.returncode,
                'success': result.returncode == 0
            }
        except subprocess.TimeoutExpired:
            return {
                'stdout': '',
                'stderr': f'Compilation timeout (30 seconds)',
                'returncode': -1,
                'success': False
            }
        except Exception as e:
            return {
                'stdout': '',
                'stderr': f'Execution error: {str(e)}',
                'returncode': -1,
                'success': False
            }
    
    def compile_all(self, source_file):
        """
        Run all phases in sequence
        
        Args:
            source_file: Path to the source code file
            
        Returns:
            dict with compilation results for each phase
        """
        phases = ["--lex", "--parse", "--semantic", "--icg", "--optimize", "--codegen"]
        results = {}
        
        for phase in phases:
            results[phase] = self.run_phase(source_file, phase)
            if not results[phase]['success'] and phase not in ["--optimize", "--codegen"]:
                # Stop on critical errors (but allow optimization to fail gracefully)
                break
        
        return results
    
    def get_compiler_info(self):
        """Get compiler information"""
        return {
            'path': self.compiler_path,
            'exists': os.path.exists(self.compiler_path),
            'phases': [
                '--lex',
                '--parse',
                '--semantic',
                '--icg',
                '--optimize',
                '--codegen'
            ]
        }
