import os
import subprocess
import sys

timeout_sec = int(sys.argv[1]) if len(sys.argv) > 1 else 45

env = dict(os.environ)
env['THEORDER_GAME_DIR'] = r'C:\Users\Mateus\Downloads\games\CUSA00035'
env['GOW3_GAME_DIR'] = r'C:\Users\Mateus\Downloads\games\CUSA00035'
env['THEORDER_TIMEOUT'] = str(timeout_sec)
env['GOW3_TIMEOUT'] = str(timeout_sec)
env['THEORDER_FRAME_DUMP_INTERVAL'] = '100'
env['GOW3_FRAME_DUMP_INTERVAL'] = '100'

print(f"Launching game with THEORDER_TIMEOUT={timeout_sec}s, dumping frames every 100 flips...")
try:
    with open('run_video_test.log', 'w', encoding='utf-8', errors='replace') as f:
        result = subprocess.run(
            [sys.executable, 'run.py'] + sys.argv[2:],
            env=env,
            stdout=f,
            stderr=subprocess.STDOUT,
            timeout=timeout_sec + 10
        )
    print(f"Game finished with exit code {result.returncode}")
except subprocess.TimeoutExpired:
    print(f"Subprocess timeout expired after {timeout_sec + 10} seconds; process terminated.")
