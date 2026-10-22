# Graphics regression checkpoint

2026-10-22: preserve earlier correctness fixes while establishing measurement.

| Area | Evidence / remaining gate |
| --- | --- |
| Arena download / depth-store barriers | Existing regression tests preserved and passing |
| Visible pending graphics pipelines | Existing key-scoped waits preserved; warm sample records stalls rather than deleting the waits |
| Main-menu output on NVIDIA | Display capture in `out\checkpoint-20261022\captures` inspected |
| Combat, grenade particles, volumetrics, solar/cascade shadows | NOT TESTED in this checkpoint |
| AMD/Intel, wave32/wave64, LDS divergence | NOT TESTED; do not change shader semantics on menu evidence |
| Z-fighting, vertex explosions, black polygons | No new scene-specific fix or visual-correctness claim |

The prior RCA in `PROGRESS.md` distinguishes confirmed synchronization hazards
from gated shader hypotheses. Keep that distinction. Validation-layer absence in
a normal run is not a synchronization-validation pass.

Need a reproducible gameplay checkpoint, matching camera/input replay, before/after
captures, resource/draw IDs and vendor-specific hardware evidence before accepting
additional graphics fixes. Do not blanket-clear caches or disable effects.

## 2026-10-22 release investigation

Reviewed the screenshot in #45 (washed-out character and crushed surroundings on
an RTX 4080 Laptop), the attached runtime logs in #36 (RX 6700 XT) and #63
(RX 9070, Chapters 8/9), and retained `out\vsm` / `out\htile2` comparisons.
The logs contain HTILE-read markers, but no Vulkan validation or synchronization
hazard identifying the bad lighting pass. Absence of those messages is not a
validation pass. The older comparisons show depth/debug views or transitional
frames, not matching camera checkpoints for these reports.

No new lighting/shadow default is changed in v0.7.7. In particular, do not replace
the reported issue with a gamma/exposure workaround, force all shadow tiles
nonempty, or enable every experimental lighting flag. The release does include
previously merged preservation of shader-written HTILE, custom-border-color
sampler cache keys, resolved descriptors after texture rebind, and per-target
additive blending. These are reasons to request a retest, not proof that #26,
#36, #39, #45 or #63 is resolved.

The remaining gate is a chapter/checkpoint plus repeatable camera/input and
before/after final-output captures on the reporting GPU. Include `user\last_run.log`,
driver version, output resolution, upscaler and graphics settings. Existing
`THEORDER_LIGHTING_TEST` tokens (`shading`, `msdepth`, `dref`, `bias`) are diagnostic
only: test one token at a time against an unchanged baseline, preserving saves
and caches. Shader-changing tokens use distinct cache versions. A menu/prologue
smoke test cannot validate the Chapter 5/8/9 defects.
