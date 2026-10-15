"""Repeated timed runs of the game with A/B conditions, summarized as failure rates.

    python tools/batch_runs.py --runs 8 --secs 40 --cond base= --cond nomsaa=GOW3_NO_MSAA=1

Each --cond is LABEL=[VAR=VALUE[,VAR=VALUE...]]. Conditions are interleaved run by run, so a
drift (shader cache warming, temperature) affects all of them alike. Logs land in out/batch/;
a run fails on a fault or when it stops presenting before the timeout (fewer stats windows).
"""
import argparse
import os
import re
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GAME_DIR = os.environ.get('THEORDER_GAME_DIR', r'C:\Users\Mateus\Downloads\games\CUSA00035')


def kill_orphans():
    # A run killed by the timeout can leave the game running and the .exe locked (stale builds).
    subprocess.run(['taskkill', '/F', '/IM', 'theorder-probe.exe'], capture_output=True)
    subprocess.run(['taskkill', '/F', '/IM', 'gow3-probe.exe'], capture_output=True)


def summarize(log: str, secs: int):
    flips = re.findall(r'Flip #(\d+)', log)
    fault = re.search(r'^Fault: .*? at (\S+)', log, re.M)
    windows = log.count('Frame stats:')
    compile_ms = sum(float(m) for m in re.findall(r'compiles ([0-9.]+) ms', log))
    hung = not fault and windows < max(1, secs // 5 - 2)
    return {
        'flip': int(flips[-1]) if flips else 0,
        'fault': fault.group(1) if fault else '',
        'hung': hung,
        'compile_s': compile_ms / 1000,
        'windows': windows,
    }


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--runs', type=int, default=8)
    parser.add_argument('--secs', type=int, default=40)
    parser.add_argument('--cond', action='append', default=[], help='LABEL=VAR=VALUE,...')
    args = parser.parse_args()
    conds = []
    for c in args.cond or ['base=']:
        label, _, assigns = c.partition('=')
        env = dict(a.split('=', 1) for a in assigns.split(',') if a)
        conds.append((label, env))

    exe = ROOT / 'out' / 'theorder-probe.exe'
    print(f'exe {exe.name} built {time.ctime(exe.stat().st_mtime)}')
    out = ROOT / 'out' / 'batch'
    out.mkdir(parents=True, exist_ok=True)
    results = {label: [] for label, _ in conds}
    for i in range(args.runs):
        for label, extra in conds:
            kill_orphans()
            env = dict(os.environ)
            env.update({'THEORDER_GAME_DIR': GAME_DIR, 'GOW3_GAME_DIR': GAME_DIR,
                        'THEORDER_TIMEOUT': str(args.secs), 'GOW3_TIMEOUT': str(args.secs)})
            env.update(extra)
            log_path = out / f'{label}-{i + 1}.log'
            with open(log_path, 'w', encoding='utf-8', errors='replace') as f:
                try:
                    subprocess.run([sys.executable, str(ROOT / 'run.py')], env=env, cwd=ROOT,
                                   stdout=f, stderr=subprocess.STDOUT, timeout=args.secs + 40)
                except subprocess.TimeoutExpired:
                    pass
            r = summarize(log_path.read_text(encoding='utf-8', errors='replace'), args.secs)
            results[label].append(r)
            state = f'FAULT {r["fault"]}' if r['fault'] else ('HUNG' if r['hung'] else 'ok')
            print(f'{label:>10} {i + 1:2}: flip {r["flip"]:5} compile {r["compile_s"]:5.1f}s {state}',
                  flush=True)
    kill_orphans()
    print('summary:')
    for label, rs in results.items():
        failed = sum(1 for r in rs if r['fault'] or r['hung'])
        flips = sorted(r['flip'] for r in rs)
        print(f'  {label:>10}: {failed}/{len(rs)} failed '
              f'({sum(1 for r in rs if r["fault"])} faults, {sum(1 for r in rs if r["hung"])} hung), '
              f'median flip {flips[len(flips) // 2]}, '
              f'mean compile {sum(r["compile_s"] for r in rs) / len(rs):.1f}s')


if __name__ == '__main__':
    main()
