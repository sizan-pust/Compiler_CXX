import threading, time
from compiler_bridge import CompilerBridge

cb = CompilerBridge(r"d:\\compiler\\bin\\compiler.exe")
source = r"d:\\compiler\\long_run.c"

# Create a long-run C program
with open(source, 'w') as f:
    f.write('#include <stdio.h>\n')
    f.write('int main() {\n')
    f.write('  volatile unsigned long i = 0;\n')
    f.write('  while (1) {\n')
    f.write('    i++;\n')
    f.write('    if ((i % 100000000) == 0) printf("tick %lu\\n", i);\n')
    f.write('  }\n')
    f.write('  return 0;\n')
    f.write('}\n')

# Run compile+run in a thread
def run():
    print('Starting run_link...')
    res = cb.run_link(source, compile_timeout=10, run_timeout=60)
    print('run_link returned')
    print('STDOUT:', res.get('stdout'))
    print('STDERR:', res.get('stderr'))

thr = threading.Thread(target=run)
thr.start()

# Wait a bit then terminate
time.sleep(2)
print('Terminating process...')
cb.terminate_current()
thr.join()
print('Done')
