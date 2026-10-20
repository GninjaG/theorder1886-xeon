"""Symbolized fault stacks of game logs: python tools/fault_stacks.py out/batch/*.log

For each log with a 'Fault:' report, prints the faulting thread and the unwound host frames,
resolving theorder-probe.exe offsets with llvm-symbolizer (MSYS2 CLANG64).
"""
import re
import shutil
import subprocess
import argparse
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EXE = ROOT / 'out' / 'theorder-probe.exe'
IMAGE_BASE = 0x140000000


def symbolizer():
    found = shutil.which('llvm-symbolizer')
    if found:
        return found
    candidate = Path(r'C:\msys64\clang64\bin\llvm-symbolizer.exe')
    return str(candidate) if candidate.exists() else None


def symbolize(offsets, exe=EXE):
    tool = symbolizer()
    if not tool or not offsets:
        return {}
    text = '\n'.join(hex(IMAGE_BASE + o) for o in offsets)
    out = subprocess.run([tool, f'--obj={exe}', '-f', '-C'], input=text, capture_output=True,
                         text=True).stdout.strip().split('\n\n')
    names = {}
    for offset, block in zip(offsets, out):
        lines = block.split('\n')
        where = lines[1].replace(str(ROOT).replace('\\', '/') + '/gpu/shadps4/', '') if len(lines) > 1 else ''
        names[offset] = f'{lines[0][:90]}  {where[:70]}'
    return names


def parse_fault_log(log, path=''):
    fault = re.search(r'^Fault: .*$', log, re.M)
    if not fault:
        return None
    rest = log[fault.start():]
    thread = re.search(r'thread name: (.*)', rest[:400])
    frames = re.findall(r'^\s+\d+ (\S+)', rest.split('globals:')[0], re.M)
    offsets = [int(match.group(1), 16) for frame in frames
               for match in [re.match(r'theorder-probe\.exe\+0x([0-9a-f]+)', frame)] if match]
    flips = re.findall(r'Flip #(\d+)', log[:fault.start()])
    return {
        'log': str(path) if path else None,
        'fault': fault.group(0)[7:110],
        'thread': thread.group(1) if thread else None,
        'flip': int(flips[-1]) if flips else 0,
        'frames': frames,
        'offsets': offsets,
    }


def inspect_log(log, path='', resolve_symbols=True, exe=EXE):
    result = parse_fault_log(log, path)
    if result and resolve_symbols:
        result['symbols'] = symbolize(result['offsets'], exe)
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--json', type=Path, help='write structured fault summaries')
    parser.add_argument('--no-symbols', action='store_true', help='skip llvm-symbolizer')
    parser.add_argument('--exe', type=Path, default=EXE, help='executable used for this log')
    parser.add_argument('logs', nargs='*', type=Path)
    args = parser.parse_args()
    results = []
    for path in args.logs:
        log = Path(path).read_text(encoding='utf-8', errors='replace')
        result = inspect_log(log, path, resolve_symbols=not args.no_symbols, exe=args.exe)
        if not result:
            continue
        results.append(result)
        names = result.get('symbols', {})
        print(f'== {path.name}: {result["fault"]} [{result["thread"] or "?"}] '
              f'after flip {result["flip"]}')
        for frame in result['frames'][:12]:
            match = re.match(r'theorder-probe\.exe\+0x([0-9a-f]+)', frame)
            print('    ' + (names.get(int(match.group(1), 16), frame) if match else frame))
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps(results, indent=2) + '\n', encoding='utf-8')
    if args.logs and not results:
        print('No faults found.')


if __name__ == '__main__':
    main()
