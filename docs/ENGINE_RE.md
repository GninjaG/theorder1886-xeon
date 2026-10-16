# The Order: 1886 engine notes (eboot 01.02)

Facts found in the executable (`out/eboot.elf`, image base 0: addresses are eboot offsets) while
porting. Tools: `llvm-objdump -d` (MSYS2 clang64), Rizin 0.9.1 (`rz-bin -zz` for strings; its
string addresses are file offsets, 0x4000 above the virtual addresses past 0x1400000), and short
Python scripts that resolve rip-relative constants from the ELF segments.

## Decompiling with REA (Ghidra)

REA admits only PE images to Ghidra on Windows, so `python scripts/eboot_pe.py <game folder>`
wraps the eboot in one, `out/eboot_ghidra.exe`:
- eboot address A is at **0x100000000 + A** (the PE headers need room below eboot address 0);
- every relocation is applied; the 679 import slots are IAT entries named from
  `src/import_names.inc` (26 of the 355 imports keep their NID);
- the 27958 `.eh_frame_hdr` function starts are exported as `sub_<A>`, A in eboot coordinates,
  plus `_init`, `_start` and `main`.

The PE loader would pick the Windows x64 calling convention; the eboot uses System V, so the REA
server needs `REA_GHIDRA_LANGUAGE_ID=x86:LE:64:default` and `REA_GHIDRA_COMPILER_SPEC_ID=gcc`.
The first query of a session imports and analyzes the whole image (12 min on this PC, 654 s of
them auto-analysis; nothing is kept between sessions), past REA's 330 s default:
`REA_GHIDRA_STARTUP_TIMEOUT_MS=3600000`, `GHIDRA_HEADLESS_MAXMEM=8G`. REA must
run from the published npm package (`C:\Users\Mateus\tools\rea`): the Windows Ghidra boundary
needs its bundled native addon, which a source build lacks (`npx rea-agents@6.5.0` resolves to
the global `npm link` of one).

## Start-up

- `_start` (0xa0) calls `_init_env` and `atexit`, then the initializer at 0x20 (`.init_array` at
  0x166b928.., then the `.ctors` list from 0x166ac90 downwards), then `main` (0x2981b0). The
  loader must not call 0x20 itself: every global would be constructed twice (it did until
  8c2de5f, mapping the engine's 1.6 GiB of memory pools twice).
- The engine's pools at start: 8 + 24 + 810 + 654 + 52 MiB of direct memory. Loading the first
  gameplay save adds ~0.8 GiB of 2 MiB streaming chunks; 4.2 of 5.3 GB are in use there.

## Video setup (0x44ce86, called by the game-options function at 0x446080)

- The picture's aspect ratio is the float stored at 0x44ce8c (2.4); the engine centers a picture
  of that shape in its display buffer and sizes the scene targets to it (1920x800 at 2.4).
- Display mode: the flags at `0x7dc(game)` select 720p (bit 7), 1080p (bit 8) or 4K (bit 9), in
  that order, with 0x500/0x2d0, 0x780/0x438 and 0xf00/0x870 immediates (0x44cef7.., 0x44cf3d..,
  0x44cf80..). The 1080p bit is set on this setup, so `-use4K` alone changes nothing.
- The configuration is passed to 0xa82860; 0x8a10a0 copies it (from offset 0x48, the aspect) to
  the display device at 0xe0 (global 0x16b1df0).

## Field of view

Cameras keep a horizontal FOV defined for 16:9 and turn it into a vertical one with
`2*atan(tan(h/2) * 0.5625)`; the 0.5625 is a double in each function's constant pool: 0x147f2b0,
0x147f2d0, 0x147f3c0 / 0x147f3d0 (projection builders around 0x8c6a44 / 0x8c7a42), 0x147f5d0
(converted back with 2.4 at 0x147f5d8), 0x147f5e8, 0x147f610, 0x147f670. The aspect ratio
patches scale all of them but 0x147f5d0 by 2.4/aspect (Vert+).

## Command line (cr7game.cpp)

The retail executable still parses developer options: names are hashed (lower case, '-'
skipped, table at 0x15c04a0) and looked up by binary search in the parsed argument list; the
game-options function (0x446080) reads them. The port passes them with `THEORDER_ARGS` (split on
spaces). Help texts at 0x148a000.. (file offset 0x148e000..):

| Option | Help text | Tried |
|---|---|---|
| `-cores numcores` | Set number of cores to use (default 6, stored at 0x7c8, passed to the renderer config) | 8 and 10: same threads, same FPS |
| `-use720p`, `-use1080p`, `-use4K` | resolution in fullscreen mode (flag bits 7-9) | `-use4K`: 1080p bit wins |
| `-fixedfps` | run at a fixed framerate | |
| `-syncinterval sync_interval_num` | the VSYNC interval to use | 1: still 30 FPS (the port paces presentation) |
| `-noadaptivesync` | disable adaptive VSYNC | |
| `-msaa msaa_mode` | the MSAA mode to use | |
| `-disablecloth`, `-nocloth` | disable cloth on game characters | |
| `-startlevel`, `-startcheckpoint checkpoint_name` | load a level and spawn at a checkpoint | `wch_020` + `wch_020_checkpoint_025`: still the title screen |
| `-printfps` | print the frame time in the TTY when over budget | |
| `-photomode`, `-nogameui`, `-disableuiindicators`, `-gamechaptersonly`, `-usedlc`, `-enablesubtitles`, `-gamma` | | |

Checkpoint names follow `<level>_<nnn>_checkpoint_<nnn>`: `pro` (prologue), `wch`
(Whitechapel), `bwy`, `air`, `frl`, `tsl`, `eih`, `cat`, `may`, `wmp`.

## Rendering facts used by fixes

- Fullscreen clear: VS e5979e2b (x = s0 + {-1, 3, -1}, y = {1, 1, -3}, z = 1) with PS 9e98cf77
  (writes four constants). Its user data register must stay at the push-constant slot the
  pipeline gives it (91aa3ac).
- Final composite to the display buffer: VS b9c87fc1 (positions from the vertex index), PS
  a41511a2 (samples the picture at `fragcoord - (display - picture) / 2`, encodes sRGB).
- Tonemap and other fullscreen passes: VS 0ece32eb reads position and UV from a 60-byte vertex
  buffer (stride 20: (-1,1,1,0,0) (3,1,1,2,0) (-1,-3,1,0,2)) at 0x1059c18988 in every captured
  frame. `THEORDER_WATCH=1059c18988:60` showed it written once (zeros, then the triangle) and
  never again in 80 s: the guest data is static, so the one-frame black triangle comes from the
  port's side (buffer cache or a different V#), not from the engine rewriting it.

## Task system queue (startup fault at 0x20aa22)

`sub_20a950(scheduler)` runs one pending job (165 direct callers: every "help while waiting"
path). For each queue in `scheduler+0x168` (count at `+0x190`) it pops a Michael-Scott queue of
64-byte nodes addressed by index (`queue+0x148` node array; header with head `+0`, tail `+0x40`,
free list `+0x80`, count `+0xc0`; 64-bit words: index in the low 32 bits, tag above, `next ==
-1` is null) and pushes the old head node onto a Treiber free list. The fault is the read of
`node[head].next`: head already held an index outside the array, so the corruption happens
earlier (push side `sub_20b010` or node reuse). Node fields: `+8` job function, `+0x30` a
dependency counter pointer, `+0x38` a completion counter decremented after the job runs.
- Analytic in-scattering of light volumes (Sun et al.): PS 15c552f8, a2925aa8, 64b3f066, fd7df5bc
  into a 960x400 R16G16B16A16 buffer, F table in a 512x512 R32 texture.
