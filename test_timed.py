import os
import subprocess
import sys


def _parse_timeout(argv):
    if not argv:
        return 45, argv

    if argv[0].startswith('--timeout='):
        return int(argv[0].split('=', 1)[1]), argv[1:]
    if argv[0] in ('--timeout', '-t'):
        if len(argv) < 2:
            raise SystemExit('missing timeout value after --timeout/-t')
        return int(argv[1]), argv[2:]
    if argv[0].lstrip('-').isdigit():
        return int(argv[0]), argv[1:]
    return 45, argv


def main(argv=None):
    argv = sys.argv[1:] if argv is None else list(argv)
    timeout_sec, run_args = _parse_timeout(argv)

    env = dict(os.environ)
    env['THEORDER_GAME_DIR'] = r'C:\Users\Mateus\Downloads\games\CUSA00035'
    env['THEORDER_TIMEOUT'] = str(timeout_sec)
    env['THEORDER_FRAME_DUMP_INTERVAL'] = '100'

    print(f"Launching game with THEORDER_TIMEOUT={timeout_sec}s, dumping frames every 100 flips...")
    try:
        with open('run_video_test.log', 'w', encoding='utf-8', errors='replace') as f:
            result = subprocess.run(
                [sys.executable, 'run.py'] + run_args,
                env=env,
                stdout=f,
                stderr=subprocess.STDOUT,
                timeout=timeout_sec + 10
            )
        print(f"Game finished with exit code {result.returncode}")
        return result.returncode
    except subprocess.TimeoutExpired:
        print(f"Subprocess timeout expired after {timeout_sec + 10} seconds; process terminated.")
        return 124


if __name__ == '__main__':
    raise SystemExit(main())
