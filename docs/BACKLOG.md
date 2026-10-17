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
- 0.3.8: #7/#9 (CUSA00785 stopped at the AppContent service label) fixed; #1/#10 (Intel black
  characters) VK_EXT_vertex_input_dynamic_state off on Intel; #4/#5 all 30 PS4 languages in the
  launcher. Asked to test. #8 (RTX 5070 Ti: black dots on faces, stretched black triangles from the
  rooftop's poles and wires; not seen here at 1080p): asked for the log and settings. #10 also: subtitles
  miss letters ("didn t"), MSAA on goes black (HD 630).
- 0.4.0: #8's black dots are probably NaN pixels spread by the depth of field (bokeh hexagons in the
  shadPS4 reports); GCN NaN rules ported (shadPS4 #5257/#5346), not reproduced here. #11 (RX Vega 64,
  chapter IX, ErrorOutOfHostMemory; the same in shadPS4 on that PC): the reporter's chapter IX save
  runs here; Check() now names the failing call, asked for a new log, RAM size and page file.

## 0. UI past the screen edges with an aspect ratio patch — todo (found 2026-10-17)
The 3D view is right (Vert+), but the Iggy UI is laid out for the taller picture: glyphs ~1.07x,
positions spread with the height (~1.33x), the chapter title's right edge 35 px past the screen at
16:9 (the player calls it "a zoom"). Next: find where the UI gets its display size (aspect at display
device +0xe0, from the video config copied at +0x98) and give the UI the original 2.4 picture.
Check: full-res frame dumps of the chapter I title (rooftop save, ~58 s) at original and 16:9.

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
needed elsewhere. Writing the label through the backing view (tried, 3+3 runs) only moved the download to
the next CPU write of the page (sceGnmDispatchInitDefaultHardwareState), no gain.
- Tried (2026-10-17, not kept): copies of small written bindings at each submission end (shadows)
  plus the compute label through the backing view. The command buffer page's downloads went from
  ~170 to ~2 ms/s, but the eboot's window 0x105b500000 then paid the wait (~250 ms/s): its 1024-byte
  binding is written again in the current, unsubmitted command buffer just before the CPU writes
  next to it ("stale"), so its data needs that submission. Total unchanged; and the label through
  the backing view hung the game (a GFX wait on 0x1066181700 saw an older value: the GPU's copy of
  the label stays old and a later download brings it back).
- Next: only a byte-precise page model (CPU writes that miss the GPU bytes need no download)
  avoids it; the uploads of CPU-modified pages would then have to skip GPU-written bytes.
- Tools: THEORDER_FRAME_STATS prints "Buffer downloads by time" (window, CPU writes, instruction);
  THEORDER_GPU_WRITE_WATCH=<hex> prints the writable bindings in that address's 64 KiB.

## 4. Scene-load pauses (pipeline restores) — doing
~95 stored pipelines restore at the prologue start (260-390 ms), 30-pipeline bursts 100-180 ms;
about 4-5 ms each (file read ~1.3 ms, driver ~0.8 ms). Early restores from workers did not work
(their programs are not loaded yet).
0.3.5: pipelines with a permutation made this session build in the background (they were taken
as known and compiled on the GPU thread, 130-190 ms each, 4-8 a run). Left: 2-4 a run
(unskippable compute dispatches; stored keys whose index a later session reused), and the
permutation count itself (hundreds per material shader: image type and sRGB per texture).
0.3.6: past 8 ms of known-pipeline builds in a 33 ms window the rest build in the background:
the scene start burst is gone (prologue 67 ms, rooftop load 300-399 ms instead of 833 ms).
0.3.7: skippable compute dispatches always build in the background (no compute freeze left).
0.3.9: a known graphics pipeline only restores its stored binary on the GPU thread; a miss
(another session's permutation at that index) builds in the background. Prologue worst frames
99-119 ms, 0-1 slow synchronous compiles a run; rooftop none.
- Next: permutations identified by their specialization instead of the order they appear
  (cache format change), or fewer specializations (2D views of array images).

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
