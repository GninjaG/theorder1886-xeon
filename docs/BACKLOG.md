# Backlog (The Order: 1886 port)

Ordered by what a player notices. Each item: what is known, the next step, how it is checked.
Status: `todo`, `doing`, `done (commit)`, `parked (why)`. Details and history: docs/PROGRESS.md.

Measuring scene: the London rooftop from the user's save (THEORDER_USER_DIR=out/user_gameplay,
default `--pad`), ~1430 draws/frame; at 0.3.2 it runs 28-30 FPS at 30, 14-17 us/draw.

## GitHub issues (2026-10-17)
- #3 (01.00 eboot crashed at start): fixed in 0.3.3 (01.02-only hooks and patch sites); asked
  the reporter to test.
- #1 (Intel Arc, black/missing textures): 0.3.3 stops speculative SRT walks from patching the
  walker for good; not reproducible here, asked the reporter to test.
- #2 (Linux): answered, no Linux build yet; left open to track it.

## 1. GPU thread time per draw in heavy scenes — doing
The 60 FPS mode drops far below 60 in gameplay: the GPU command thread is the limit
(1430 draws x 14-17 us = 20-24 ms per frame of its own CPU time, plus waits).
- Stage B (port:DrawRec) is the limit in the 60 FPS mode. The mapped range check's lock was a
  fifth of it: per-thread copies (0.3.4) took the rooftop from ~57k to ~71k draws/s (~33 -> ~39 FPS).
- After 0.3.4 stage B is idle about half the time: the GPU command thread's waits (drains,
  item 3's Finish) set the pace; stage B's cost per draw still sets how long a drain waits.
- Image refreshes (THEORDER_FRAME_STATS "Image refreshes by bytes"): 0x1053e00000 (1728x1080 and
  864x540 at one address) is GPU-dirty ~110 times/s, ~800 MB/s of buffer-to-image copies on the
  GPU (written as a buffer, read as a texture); 0x1057eb0000 (1024x1024, GPU-modified and
  CPU-dirty) ~20 times/s has every mip hashed, ~650 MB/s of xxhash on stage B.
- Next: hash only the pages the CPU wrote (MaybeCpuDirty/CpuDirty refresh); look at why the
  1728x1080 target is written as a buffer.
- Check: us/draw and FPS on the rooftop, 60 FPS mode, interleaved runs.

## 2. Draw pipe drains at WaitRegMem — done (finding, 2026-10-17)
They wait for labels the compute queues write; the compute queues drain before every packet
anyway. Handing compute work to the recording thread removed the drains but lost ~15% (it moved
work onto stage B, the limit). Details: docs/PROGRESS.md 2026-10-17 (later).

## 3. GPU readback waits for the whole GPU — todo (cause found, 2026-10-17)
BufferCache::DownloadMemory -> Scheduler::Finish: ~20% of the GPU command thread on the rooftop
(60 FPS mode, 0.3.4). Once a frame, the CPU writes a 4 KiB page that holds two 192-byte storage
buffers a compute dispatch writes (0x106ea85360, 0x106ea85440) next to a compute ReleaseMem label
(+0x520) and the compute command buffer the CPU fills (+0x5a0, sceGnmDispatchInitDefaultHardwareState
and the eboot at 0xcc8791): the page is GPU-modified, so the write faults and a 512 KiB window is
downloaded after a Finish (~4.6 ms). The CPU never needs those bytes; the download only keeps the
page consistent (a later upload of the CPU-modified page would overwrite the GPU's bytes).
Without readbacks (THEORDER_READBACKS=0) the game loads the scene later with fewer draws: they are
needed elsewhere. Writing the label through the backing view alone would move the download to the
next CPU write of the page.
- Next: a download that waits only for the tick of the last GPU write to the range (per-range
  ticks; read from a host-visible arena or an end-of-submission copy) instead of Finish.
- Tools: THEORDER_FRAME_STATS prints "Buffer downloads by time" (window, CPU writes, instruction);
  THEORDER_GPU_WRITE_WATCH=<hex> prints the writable bindings in that address's 64 KiB.

## 4. Scene-load pauses (pipeline restores) — todo
~95 stored pipelines restore at the prologue start (260-390 ms), 30-pipeline bursts 100-180 ms;
about 4-5 ms each (file read ~2 ms, driver 1.2 ms). Early restores from workers did not work
(their programs are not loaded yet).
- Next: let workers load cached programs, or prefetch binaries via an emulator key -> file index.

## 5. Rare startup fault in the task queue (0x20aa22) — closed (not reproduced, 2026-10-17)
Seen once in ~40 starts before 0.3.1; none in the ~100 batch runs of 2026-10-16/17. The fault
report prints the queue header if it returns.

## 6. One-frame black triangle — parked (no reproduction)
The engine's fullscreen vertex buffer is static; the cause is on the port's side.
- Next: a detector that compares the vertex data a draw read with guest memory.

## 7. Settings file leftovers from God of War — done (9b79d7f)
The in-game menu already rewrote the file without them; the launcher and run.py kept them.
Both writers now drop them (graphics_options.clean_lines).

## 8. `-startlevel` ignored — parked (testing convenience only)
Reaches the title screen even with the gate patched; see docs/ENGINE_RE.md.

## 9. Coverage — parked (needs saves or routes)
Most of the campaign, sound, AMD and Intel GPUs are unchecked (no AMD/Intel hardware here).
