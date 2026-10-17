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
- Next: profile again; then BindTextures (21% inclusive), GetBarriers, the descriptor hashing.
- Check: us/draw and FPS on the rooftop, 60 FPS mode, interleaved runs.

## 2. Draw pipe drains at WaitRegMem — done (finding, 2026-10-17)
They wait for labels the compute queues write; the compute queues drain before every packet
anyway. Handing compute work to the recording thread removed the drains but lost ~15% (it moved
work onto stage B, the limit). Details: docs/PROGRESS.md 2026-10-17 (later).

## 3. GPU readback waits for the whole GPU — todo
BufferCache::DownloadMemory -> Scheduler::Finish: 11% of the GPU thread on the rooftop.
- Next: find which guest reads trigger it; wait for the tick that wrote the range instead.
- Check: sampler share, FPS.

## 4. Scene-load pauses (pipeline restores) — todo
~95 stored pipelines restore at the prologue start (260-390 ms), 30-pipeline bursts 100-180 ms;
about 4-5 ms each (file read ~2 ms, driver 1.2 ms). Early restores from workers did not work
(their programs are not loaded yet).
- Next: let workers load cached programs, or prefetch binaries via an emulator key -> file index.

## 5. Rare startup fault in the task queue (0x20aa22) — todo
1 in ~40 starts (not seen in 40 recent ones). The fault report now prints the queue header.
- Next: wait for a report; if it never shows, close as not reproduced.

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
