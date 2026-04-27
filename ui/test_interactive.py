from compiler_bridge import CompilerBridge
import time

cb = CompilerBridge(r"d:\\compiler\\bin\\compiler.exe")
source = r"d:\\compiler\\test_input.c"

# Create a small program that uses scanf
with open(source, 'w') as f:
    content = (
        '#include <stdio.h>\n'
        'int main() {\n'
        '  int x;\n'
        '  if (scanf("%d", &x) == 1) {\n'
        '    printf("You entered: %d\\n", x);\n'
        '  } else {\n'
        '    printf("No input\\n");\n'
        '  }\n'
        '  return 0;\n'
        '}\n'
    )
    f.write(content)

print('Starting program...')
res = cb.start_program(source, stdout_callback=lambda s: print('STDOUT:', s, end=''), stderr_callback=lambda s: print('STDERR:', s, end=''), exit_callback=lambda rc: print('\nExited:', rc))
print('start_program returned:', res)

# Wait a moment for program to be ready for input
time.sleep(0.5)
print('Sending input...')
sent = cb.send_input('42\n')
print('sent', sent)

# Wait for program to exit
time.sleep(1)
print('Done')
