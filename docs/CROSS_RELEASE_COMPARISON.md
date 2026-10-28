# Cross-release comparison: v0.6.1 versus v0.7.x

This workflow extends tools/regression_report.py. It compares manifests from
**different port executable builds**, without claiming that numerical improvements
prove visual correctness. Related investigations: #43, #47, #24 and #63.

## Capture the two builds separately

Use tools/batch_runs.py with a matching fixed checkpoint, input script, game
update, output resolution, upscaler, driver, and repetition count. Each side
must be run with a *different* executable and writable profile/data paths.
Never delete your real saves or shader caches.

The runner supports --cond, --setting, --scene, --pad, --cusa,
--game-version, --profile-root, --user-template, --cache-state, --data-root,
--game-dir and --manifest. Inspect its --help for additional capture flags.

Example PowerShell setup (substitute valid paths and the same real game dump;
the release executables must already be installed):

~~~powershell
$env:THEORDER_PROBE = 'D:\triage\old\theorder-probe.exe'
python tools/batch_runs.py --runs 5 --secs 120 --cond base= --issue 43 --scene 'Issue 43 fixed boot camera' --game-dir 'D:\CUSA00035' --cusa CUSA00035 --game-version 01.02 --profile-root 'D:\triage\profiles-old' --data-root 'D:\triage\data-old' --cache-state cold --setting upscaler=off --setting preset=1 --setting output_res=1920x1080 --setting aspect_ratio=original --setting engine_fps=60 --setting fps_limit=0 --setting vsync=1 --setting async_shaders=1 --setting deferred_readback=1 --setting stale_readback=1 --setting frames_queued=1 --manifest 'D:\triage\old.json'

$env:THEORDER_PROBE = 'D:\triage\new\theorder-probe.exe'
python tools/batch_runs.py --runs 5 --secs 120 --cond base= --issue 43 --scene 'Issue 43 fixed boot camera' --game-dir 'D:\CUSA00035' --cusa CUSA00035 --game-version 01.02 --profile-root 'D:\triage\profiles-new' --data-root 'D:\triage\data-new' --cache-state cold --setting upscaler=off --setting preset=1 --setting output_res=1920x1080 --setting aspect_ratio=original --setting engine_fps=60 --setting fps_limit=0 --setting vsync=1 --setting async_shaders=1 --setting deferred_readback=1 --setting stale_readback=1 --setting frames_queued=1 --manifest 'D:\triage\new.json'

python tools/regression_report.py 'D:\triage\new.json' --compare-with 'D:\triage\old.json' --out 'D:\triage\comparison'
~~~

A checkpoint requiring a saved game needs a separate, manually verified
--user-template copied under each isolated profile root. Use the same scripted
input route and --frame-trace in **both** runs when raw pacing is the focus.
For visual correctness, capture the same final scene with the same camera
separately; logging/capture overhead can distort frametimes.

## Interpreting the output

The tool writes the candidate's original report.md and report.html plus
cross-release.json and cross-release.md. The comparator now checks the entire **redacted effective THEORDER_* runtime
environment** across runs and builds. Options such as THEORDER_NO_MSAA,
THEORDER_DRAW_PIPE and inherited renderer toggles can affect the output even
when the chosen INI keys are identical. Missing or differing runtime flags
block numerical deltas rather than producing a misleading matched comparison.
The manifest filters the executable location, player names, config/capture
paths, and filesystem-looking diagnostic values. Existing reports with private
paths should be redacted before sharing; this change does not retroactively
rewrite past files.

Both executable SHA-256 hashes must
be collected and different. CUSA, game version, eboot SHA, GPU, driver, scene,
OS/architecture, duration, input, cache state and all selected INI settings
must be present and equal across all runs. One condition is permitted in each
manifest. A missing or different value **blocks** comparison deltas.

The new cross-release status can be "blocked" or "review-required" only; no
"fixed" status exists. With fewer than five repetitions, the report warns about
weak evidence. Port cache isolation does **not** control the OS/driver cache.

P95/P99 from --frame-trace are summarized as medians of each independent
run's whole-trace percentile; they are **not** pooled percentiles across all
frames or proof of less hitching in every scene. A 0.6.1 binary lacking newer
instrumentation may have unavailable telemetry: the report does not invent it.

If an old release cannot be executed by the newer harness (ABI/probe
mismatch), do not force the mix and do not claim comparability. Use
separate compatible checkouts and manually verify the resulting manifests.
**Privacy boundary:** the changes in this PR redact the *environment
provenance field*, not the complete manifest. Raw `batch_runs.py` manifests
still contain machine-local `log_path`, `data_dir`, `user_profile`,
`capture_dir`, capture references and potentially arbitrary scene/reference
notes. They are local diagnostic artifacts, **not files safe for automatic
publication**. Review and remove private paths, names, notes and source material
before attaching any raw manifest or rendered report to a public issue.

Only share manually redacted logs/manifests; avoid personal paths, private saves,
copyrighted binaries or game assets.
