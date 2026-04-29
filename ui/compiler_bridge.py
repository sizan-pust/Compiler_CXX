"""
Compiler Bridge
Handles communication with the C++ compiler via subprocess
"""

import subprocess
import sys
import os
import re
import json
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
        self._current_proc = None
        
    def validate_compiler(self):
        """Check if compiler exists"""
        if not os.path.exists(self.compiler_path):
            raise FileNotFoundError(f"Compiler not found: {self.compiler_path}")
        

    def _make_unbuffered_source(self, source_file):
        """Create temp C file and inject stdout unbuffering inside main()."""
        
        dirpath = os.path.dirname(source_file) or "."
        filename = os.path.basename(source_file)
        temp_path = os.path.join(dirpath, "__run_unbuffered_" + filename)

        with open(source_file, "r", encoding="utf-8") as f:
            code = f.read()
        code = "#include <stdio.h>\n#define print printf\n" + code
        # Find: int main(...) {
        pattern = r'(int\s+main\s*\([^)]*\)\s*\{)'
    # wrap top-level mini-C code inside main if user did not write main()
        if "main" not in code:
            code = code + "\n"
            code = code.replace("#define print printf\n", "#define print printf\n\nint main(){\n", 1)
            code += "\nreturn 0;\n}\n"

        # disable stdout buffering
        code = code.replace(
            "int main(){",
            "int main(){\n    setvbuf(stdout, NULL, _IONBF, 0);",
            1
        )

        code = code.replace(
            "int main() {",
            "int main() {\n    setvbuf(stdout, NULL, _IONBF, 0);",
            1
        )
        replacement = r'\1\n    setvbuf(stdout, NULL, _IONBF, 0);'

        new_code, count = re.subn(pattern, replacement, code, count=1)

        if count == 0:
            new_code = code

        with open(temp_path, "w", encoding="utf-8") as f:
            f.write(new_code)

        return temp_path


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
            # Run compiler with specified phase (blocking)
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

    def terminate_current(self):
        """Terminate any currently running subprocess launched by this bridge."""
        try:
            if self._current_proc and self._current_proc.poll() is None:
                self._current_proc.kill()
        except Exception:
            pass
        finally:
            self._current_proc = None

    def compile_file(self, source_file, timeout=60):
        """Compile the source file using system gcc in the file's directory.

        Returns same dict shape as `run_phase`.
        """
        if not os.path.exists(source_file):
            raise FileNotFoundError(f"Source file not found: {source_file}")

        dirpath = os.path.dirname(source_file) or '.'
        filename = os.path.basename(source_file)
        exe_name = os.path.splitext(filename)[0] + ('_exec.exe' if os.name == 'nt' else '_exec')
        exe_path = os.path.join(dirpath, exe_name)

        try:
            # Use absolute paths to avoid cwd / PATH lookup issues on Windows
            proc = subprocess.Popen(['gcc', '-o', exe_path, source_file],
                                    stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            self._current_proc = proc
            try:
                stdout, stderr = proc.communicate(timeout=timeout)
                returncode = proc.returncode
                return {'stdout': stdout, 'stderr': stderr, 'returncode': returncode, 'success': returncode == 0}
            except subprocess.TimeoutExpired:
                proc.kill()
                stdout, stderr = proc.communicate()
                return {'stdout': stdout, 'stderr': f'Compile timeout ({timeout}s)', 'returncode': -1, 'success': False}
        except Exception as e:
            return {'stdout': '', 'stderr': f'Execution error: {str(e)}', 'returncode': -1, 'success': False}
        finally:
            self._current_proc = None

    def run_link(self, source_file, compile_timeout=60, run_timeout=60):
        """Compile and run the source file using system gcc, return JSON-like stdout string.

        The returned dict has keys 'stdout' (JSON string), 'stderr', 'returncode', 'success'.
        """
        if not os.path.exists(source_file):
            raise FileNotFoundError(f"Source file not found: {source_file}")

        dirpath = os.path.dirname(source_file) or '.'
        filename = os.path.basename(source_file)
        exe_name = os.path.splitext(filename)[0] + ('_exec.exe' if os.name == 'nt' else '_exec')
        exe_path = os.path.join(dirpath, exe_name)

        # Compile
        try:
            # Use absolute paths to avoid cwd / PATH lookup issues on Windows
            proc = subprocess.Popen(['gcc', '-o', exe_path, source_file],
                                    stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            self._current_proc = proc
            try:
                c_stdout, c_stderr = proc.communicate(timeout=compile_timeout)
            except subprocess.TimeoutExpired:
                proc.kill()
                c_stdout, c_stderr = proc.communicate()
                j = json.dumps({'success': False, 'error': 'compile timeout', 'status': 1})
                return {'stdout': j, 'stderr': c_stderr, 'returncode': -1, 'success': False}

            if proc.returncode != 0:
                j = json.dumps({'success': False, 'error': c_stderr, 'status': 1})
                return {'stdout': j, 'stderr': c_stderr, 'returncode': proc.returncode, 'success': False}

            # Run the produced executable using its absolute path
            run_proc = subprocess.Popen([exe_path], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            self._current_proc = run_proc
            try:
                r_stdout, r_stderr = run_proc.communicate(timeout=run_timeout)
            except subprocess.TimeoutExpired:
                run_proc.kill()
                r_stdout, r_stderr = run_proc.communicate()
                j = json.dumps({'success': False, 'error': 'run timeout', 'status': -1})
                return {'stdout': j, 'stderr': r_stderr, 'returncode': -1, 'success': False}

            # Success iff returncode == 0
            success = (run_proc.returncode == 0)
            j = json.dumps({'success': success, 'output': r_stdout, 'exec_status': run_proc.returncode, 'status': 0 if success else run_proc.returncode})
            return {'stdout': j, 'stderr': r_stderr, 'returncode': run_proc.returncode, 'success': success}
        except Exception as e:
            return {'stdout': '', 'stderr': f'Execution error: {str(e)}', 'returncode': -1, 'success': False}
        finally:
            self._current_proc = None

    def start_program(self, source_file, stdout_callback=None, stderr_callback=None, exit_callback=None, compile_timeout=60):
        """Compile and start the program asynchronously.

        - Compiles the source_file using `gcc`.
        - If compilation succeeds, launches the produced executable with pipes for stdin/stdout/stderr.
        - Spawns reader threads that invoke the provided callbacks for stdout/stderr lines.
        - Calls `exit_callback(returncode)` when the process exits.

        Returns a dict: if compilation fails returns {'started': False, 'stderr': <compile stderr>},
        otherwise returns {'started': True, 'exe_path': <path>, 'pid': <pid>}.
        """
        if not os.path.exists(source_file):
            raise FileNotFoundError(f"Source file not found: {source_file}")

        dirpath = os.path.dirname(source_file) or '.'
        filename = os.path.basename(source_file)
        exe_name = os.path.splitext(filename)[0] + ('_exec.exe' if os.name == 'nt' else '_exec')
        exe_path = os.path.join(dirpath, exe_name)

        # Compile first
        try:
            run_source = self._make_unbuffered_source(source_file)

            compile_proc = subprocess.Popen(
            ['gcc', '-o', exe_path, run_source],
                                            stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            self._current_proc = compile_proc
            try:
                c_stdout, c_stderr = compile_proc.communicate(timeout=compile_timeout)
            except subprocess.TimeoutExpired:
                compile_proc.kill()
                c_stdout, c_stderr = compile_proc.communicate()
                return {'started': False, 'stderr': 'compile timeout', 'returncode': -1}

            if compile_proc.returncode != 0:
                return {'started': False, 'stderr': c_stderr, 'returncode': compile_proc.returncode}

        except Exception as e:
            return {'started': False, 'stderr': f'Compilation error: {e}', 'returncode': -1}
        finally:
            self._current_proc = None

        # Launch executable with pipes for interactive I/O
        try:
            run_proc = subprocess.Popen([exe_path], stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE, text=True, bufsize=1)
            self._current_proc = run_proc
            self._output_lines = []
            self._stderr_lines = []

            import threading

            def _reader(stream, callback, store_list):
                try:
                    while True:
                        ch = stream.read(1)
                        if ch == '':
                            break
                        store_list.append(ch)
                        if callback:
                            try:
                                callback(ch)
                            except Exception:
                                pass
                finally:
                    try:
                        stream.close()
                    except Exception:
                        pass

            t_out = threading.Thread(target=_reader, args=(run_proc.stdout, stdout_callback, self._output_lines), daemon=True)
            t_err = threading.Thread(target=_reader, args=(run_proc.stderr, stderr_callback, self._stderr_lines), daemon=True)
            t_out.start()
            t_err.start()

            def _waiter():
                rc = run_proc.wait()
                # give reader threads a moment to drain
                try:
                    t_out.join(timeout=0.1)
                except Exception:
                    pass
                try:
                    t_err.join(timeout=0.1)
                except Exception:
                    pass
                # Call exit callback
                if exit_callback:
                    try:
                        exit_callback(rc)
                    except Exception:
                        pass
                # clear current proc reference
                self._current_proc = None

            waiter = threading.Thread(target=_waiter, daemon=True)
            waiter.start()

            return {'started': True, 'exe_path': exe_path, 'pid': run_proc.pid}

        except Exception as e:
            self._current_proc = None
            return {'started': False, 'stderr': f'Execution error: {e}', 'returncode': -1}

    def send_input(self, text):
        """Send text to the stdin of the currently running process."""
        try:
            if (
                self._current_proc
                and self._current_proc.poll() is None
                and self._current_proc.stdin
                and not self._current_proc.stdin.closed
            ):
                self._current_proc.stdin.write(text)
                self._current_proc.stdin.flush()
                return True
        except Exception:
            return False
        return False
    
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
