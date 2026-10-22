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
