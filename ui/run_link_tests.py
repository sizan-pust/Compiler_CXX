from compiler_bridge import CompilerBridge
import json, pprint

cb = CompilerBridge(r"d:\\compiler\\bin\\compiler.exe")
files = [r"d:\\compiler\\test_printf_with_arg.c", r"d:\\compiler\\test_printf_missing_arg.c"]
for f in files:
    print("FILE:", f)
    res = cb.run_link(f)
    print("RETURNCODE:", res['returncode'], "SUCCESS:", res['success'])
    print("STDERR:")
    print(res['stderr'])
    print("STDOUT:")
    print(res['stdout'])
    try:
        data = json.loads(res['stdout']) if res['stdout'] else None
        print("PARSED:", data)
    except Exception as e:
        print("JSON ERR:", e)
    print("="*60)
