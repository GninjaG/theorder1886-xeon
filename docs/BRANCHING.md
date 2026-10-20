# Branching & Release Convention

Single-trunk model. This document is the standing rule for how work lands and how releases
are cut, so the v0.7.3 ↔ v0.7.5 divergence that split the project across `master` and a
long-lived `upstream-0.20` branch does not happen again.

## The trunk

- **`master` is the single trunk and is always publishable.** Every release is cut from it.
- `origin/master` is the source of truth. Keep it in sync; do not let a parallel line
  accumulate features that `master` lacks.

## How work lands

- **Small, self-contained change:** commit directly to `master`.
- **Larger or riskier change:** use a **short-lived** work branch named for the task
  (e.g. `investigate/flicker-rca`, `fix/<thing>`). Merge it into `master` as soon as it is
  validated, then **delete it immediately** (local and remote). A work branch should live
  hours-to-days, not weeks.
- Do **not** create long-lived parallel branches such as `upstream-0.20`. That branch was the
  root cause of the release confusion and has been retired; its content was merged into
  `master` at v0.7.6-rc.1.

## Validation gate before merge

- Build cleanly and run the full suite single-run: `bash tools/run_tests.sh`
  (all C/C++ suites + the Python suite; environment skips excepted).
- Do not merge onto a red suite.

## Releases

- Tags and releases are cut **from `master`**, using the `windows-v<VERSION>` tag convention.
- `launcher.VERSION` (in `launcher/port_launcher_win.py`) and the release tag must match the
  same `<VERSION>`; `tests/test_launcher_controls.py::test_the_order_release_version` enforces this.
- Pre-releases (release candidates) are published with the GitHub **pre-release** flag and an
  `-rc.N` suffix; promote to a normal/latest release only when intended.
- Release notes follow the established format: a descriptive title, numbered highlights that
  cite issues and files, Package Contents, How to Play, a bilingual EN + PT-BR section, and the
  Ko-fi support footer (`https://ko-fi.com/mateusmeneses`).
- **Honesty rule for release notes:** do not claim an issue is fixed without the evidence that
  backs it. Distinguish confirmed root-cause fixes (with regression tests) from changes whose
  visual/runtime effect is still pending hardware capture. v0.7.5 announced flicker fixes that
  users reported as unresolved; subsequent notes keep a "still pending verification" section
  rather than repeat that.

## Publishing & destructive git — require explicit approval

- Pushing to `origin`, creating a tag/release, and deleting a remote branch are impactful and
  are done only with the user's explicit go-ahead, one decision at a time.
- Never `git config` changes, force push, or skip hooks (`--no-verify`) unless the user
  explicitly asks. Prefer `git branch -d` (merged check) over `-D`.

## Reference remotes (fetch-only, never merged blindly)

- `origin` — this fork (`mateussmeneses/theorder1886-port`).
- `deft96` — the `Deft96/order1886_pc` reference fork, for comparison only.
