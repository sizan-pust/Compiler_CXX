#!/usr/bin/env python3
"""
IDE Test & Validation Script
Validates setup and provides diagnostic information
"""

import sys
import os
from pathlib import Path


def check_python_version():
    """Check if Python version is sufficient"""
    version = sys.version_info
    print(f"✓ Python {version.major}.{version.minor}.{version.micro}", end=" ")
    
    if version.major >= 3 and version.minor >= 7:
        print("(OK)")
        return True
    else:
        print("(FAIL - Need 3.7+)")
        return False


def check_tkinter():
    """Check if tkinter is available"""
    try:
        import tkinter
        print("✓ tkinter", end=" ")
        print("(OK)")
        return True
    except ImportError:
        print("✗ tkinter (MISSING)")
        print("  Install: python -m pip install tk")
        return False


def check_compiler():
    """Check if C++ compiler exists"""
    compiler_path = Path(r"d:\compiler\bin\compiler.exe")
    if compiler_path.exists():
        print(f"✓ Compiler: {compiler_path}", end=" ")
        print("(OK)")
        return True
    else:
        print(f"✗ Compiler not found: {compiler_path}", end=" ")
        print("(FAIL)")
        print("  Build with: cmake -B build && cmake --build build")
        return False


def check_files():
    """Check if all IDE files exist"""
    ui_dir = Path(__file__).parent
    required_files = [
        'main.py',
        'compiler_bridge.py',
        'file_manager.py',
        'syntax_highlighter.py',
        'config.py',
        'requirements.txt',
    ]
    
    all_present = True
    for fname in required_files:
        fpath = ui_dir / fname
        if fpath.exists():
            print(f"✓ {fname}")
        else:
            print(f"✗ {fname} (MISSING)")
            all_present = False
    
    return all_present


def main():
    """Run diagnostic checks"""
    print("=" * 60)
    print("CompilerIDE - System Diagnostics")
    print("=" * 60)
    print()
    
    checks = [
        ("Python Version", check_python_version),
        ("tkinter Module", check_tkinter),
        ("C++ Compiler", check_compiler),
        ("IDE Files", check_files),
    ]
    
    results = {}
    for name, check_fn in checks:
        print(f"{name}:")
        try:
            results[name] = check_fn()
        except Exception as e:
            print(f"  Error: {e}")
            results[name] = False
        print()
    
    # Summary
    print("=" * 60)
    print("Summary")
    print("=" * 60)
    
    all_ok = all(results.values())
    
    for name, result in results.items():
        status = "✓ PASS" if result else "✗ FAIL"
        print(f"{status}: {name}")
    
    print()
    
    if all_ok:
        print("✓ All checks passed! Ready to launch.")
        print()
        print("Start the IDE with:")
        print("  python main.py")
        print()
        return 0
    else:
        print("✗ Some checks failed. Please fix the issues above.")
        print()
        return 1


if __name__ == "__main__":
    sys.exit(main())
