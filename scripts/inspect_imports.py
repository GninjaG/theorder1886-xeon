import json
import re
from pathlib import Path

gow3_dir = Path("c:/projetos/fork/gow3_pc")
to_dir = Path("c:/projetos/fork/theorder1886-port")

# Read known symbols from gow3 import_names.inc
syms = dict(re.findall(r'\{"([^"]+)","([^"]+)"\}', (gow3_dir / "src/import_names.inc").read_text(encoding="utf-8")))

link_data = json.loads((to_dir / "out/link.json").read_text(encoding="utf-8"))
unresolved = link_data["unresolved_imports"]

known = []
unknown = []

for u in unresolved:
    if u in syms:
        known.append((u, syms[u]))
    elif u.endswith("#libScePosix") and (u.replace("#libScePosix", "#libkernel") in syms):
        target = u.replace("#libScePosix", "#libkernel")
        known.append((u, syms[target] + " (via libkernel)"))
    else:
        unknown.append(u)

print(f"Total unresolved host imports: {len(unresolved)}")
print(f"Known symbol names: {len(known)}")
print(f"Unknown NIDs: {len(unknown)}")

# Group known by library
by_lib = {}
for u, name in known:
    lib = u.split("#")[-1]
    by_lib.setdefault(lib, []).append(name)

print("\n--- Known Symbols by Library ---")
for lib, funcs in sorted(by_lib.items()):
    print(f"  [{lib}] ({len(funcs)} functions):")
    for f in sorted(funcs)[:10]:
        print(f"    - {f}")
    if len(funcs) > 10:
        print(f"    ... e mais {len(funcs) - 10}")

if unknown:
    print("\n--- Unknown NIDs ---")
    for u in unknown[:20]:
        print(f"    - {u}")
    if len(unknown) > 20:
        print(f"    ... e mais {len(unknown) - 20}")
