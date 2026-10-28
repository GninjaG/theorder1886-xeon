import os
import subprocess
import sys
import threading
import time

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

    dump_dir = os.path.abspath('out/dump')
    if os.path.exists(dump_dir):
        for f in os.listdir(dump_dir):
            try:
                os.remove(os.path.join(dump_dir, f))
            except OSError:
                pass

    pad_file = os.path.abspath('pad_inject.txt')
    with open(pad_file, 'w') as f:
        f.write('')

    env = dict(os.environ)
    env['THEORDER_GAME_DIR'] = r'C:\Users\Mateus\Downloads\games\CUSA00035'
    env['THEORDER_TIMEOUT'] = str(timeout_sec)
    env['THEORDER_FRAME_DUMP_INTERVAL'] = '60'
    env['THEORDER_USER_DIR'] = os.path.abspath('out/user_gameplay')
    env['THEORDER_PAD_FILE'] = pad_file

    stop_event = threading.Event()

    def pad_pusher():
        time.sleep(12)
        while not stop_event.is_set():
            try:
                with open(pad_file, 'w') as f:
                    f.write('cross\n')
                time.sleep(0.3)
                with open(pad_file, 'w') as f:
                    f.write('\n')
                time.sleep(2.0)
            except Exception:
                pass

    t_pad = threading.Thread(target=pad_pusher, daemon=True)
    t_pad.start()

    samples = []
    def memory_sampler():
        import ctypes
        import ctypes.wintypes
        psapi = ctypes.windll.psapi
        kernel32 = ctypes.windll.kernel32

        class PROCESS_MEMORY_COUNTERS(ctypes.Structure):
            _fields_ = [
                ('cb', ctypes.wintypes.DWORD),
                ('PageFaultCount', ctypes.wintypes.DWORD),
                ('PeakWorkingSetSize', ctypes.c_size_t),
                ('WorkingSetSize', ctypes.c_size_t),
                ('QuotaPeakPagedPoolUsage', ctypes.c_size_t),
                ('QuotaPagedPoolUsage', ctypes.c_size_t),
                ('QuotaPeakNonPagedPoolUsage', ctypes.c_size_t),
                ('QuotaNonPagedPoolUsage', ctypes.c_size_t),
                ('PagefileUsage', ctypes.c_size_t),
                ('PeakPagefileUsage', ctypes.c_size_t),
            ]

        while not stop_event.is_set():
            time.sleep(2.0)
            ram_mb = 0
            commit_mb = 0
            vram_mb = 0

            # 1. Query VRAM via nvidia-smi
            try:
                vram_out = subprocess.check_output(
                    ['nvidia-smi', '--query-gpu=memory.used', '--format=csv,noheader,nounits'],
                    text=True, stderr=subprocess.DEVNULL
                ).strip()
                vram_mb = int(vram_out.split('\n')[0])
            except Exception:
                pass

            # 2. Query RAM of theorder-probe.exe
            try:
                arr = (ctypes.wintypes.DWORD * 2048)()
                cb_needed = ctypes.wintypes.DWORD()
                if psapi.EnumProcesses(ctypes.byref(arr), ctypes.sizeof(arr), ctypes.byref(cb_needed)):
                    count = cb_needed.value // ctypes.sizeof(ctypes.wintypes.DWORD)
                    for i in range(count):
                        pid = arr[i]
                        if pid == 0:
                            continue
                        h = kernel32.OpenProcess(0x0400 | 0x0010, False, pid)
                        if h:
                            buf = (ctypes.c_char * 260)()
                            if psapi.GetProcessImageFileNameA(h, buf, 260) > 0:
                                name = buf.value.decode('ascii', errors='ignore').lower()
                                if 'theorder-probe' in name:
                                    pmc = PROCESS_MEMORY_COUNTERS()
                                    pmc.cb = ctypes.sizeof(pmc)
                                    if psapi.GetProcessMemoryInfo(h, ctypes.byref(pmc), ctypes.sizeof(pmc)):
                                        ram_mb = pmc.WorkingSetSize // (1024 * 1024)
                                        commit_mb = pmc.PagefileUsage // (1024 * 1024)
                                        kernel32.CloseHandle(h)
                                        break
                            kernel32.CloseHandle(h)
            except Exception:
                pass

            if ram_mb > 0 or vram_mb > 0:
                samples.append((ram_mb, commit_mb, vram_mb))
                print(f"[Memory Monitor] Probe RAM (Working Set): {ram_mb} MB | Commit: {commit_mb} MB | Total GPU VRAM: {vram_mb} MB", flush=True)

    t_mem = threading.Thread(target=memory_sampler, daemon=True)
    t_mem.start()

    print(f"Launching game with THEORDER_TIMEOUT={timeout_sec}s, dumping frames every 60 flips...")
    try:
        with open('run_video_test.log', 'w', encoding='utf-8', errors='replace') as f:
            result = subprocess.run(
                [sys.executable, 'run.py'] + run_args,
                env=env,
                stdout=f,
                stderr=subprocess.STDOUT,
                timeout=timeout_sec + 20
            )
        stop_event.set()
        print(f"Game finished with exit code {result.returncode}")
        if samples:
            peak_ram = max(s[0] for s in samples)
            peak_commit = max(s[1] for s in samples)
            peak_vram = max(s[2] for s in samples)
            avg_ram = sum(s[0] for s in samples) / len(samples)
            avg_vram = sum(s[2] for s in samples) / len(samples)
            print("="*60)
            print(f"[SUMMARY] Peak RAM (Working Set): {peak_ram} MB (~{peak_ram/1024:.2f} GB)")
            print(f"[SUMMARY] Peak RAM (Commit):       {peak_commit} MB (~{peak_commit/1024:.2f} GB)")
            print(f"[SUMMARY] Peak VRAM:              {peak_vram} MB (~{peak_vram/1024:.2f} GB)")
            print(f"[SUMMARY] Avg RAM:                {avg_ram:.0f} MB | Avg VRAM: {avg_vram:.0f} MB")
            print("="*60)
        subprocess.run([sys.executable, 'tools/convert_dumps.py'])
        return result.returncode
    except subprocess.TimeoutExpired:
        stop_event.set()
        print(f"Subprocess timeout expired after {timeout_sec + 20} seconds; process terminated.")
        if samples:
            peak_ram = max(s[0] for s in samples)
            peak_vram = max(s[2] for s in samples)
            print(f"[SUMMARY] Peak RAM: {peak_ram} MB | Peak VRAM: {peak_vram} MB")
        subprocess.run([sys.executable, 'tools/convert_dumps.py'])
        return 124

if __name__ == '__main__':
    raise SystemExit(main())
