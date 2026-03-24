#!/usr/bin/env python3
"""
CompilerIDE Launcher
Quick start script for the IDE
"""

import sys
import os

def main():
    """Launch the IDE"""
    # Add the UI directory to the path
    ui_dir = os.path.dirname(os.path.abspath(__file__))
    sys.path.insert(0, ui_dir)
    
    # Import and run main
    from main import main as run_ide
    run_ide()

if __name__ == "__main__":
    main()
