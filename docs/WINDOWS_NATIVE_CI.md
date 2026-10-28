# Windows native CI validation

The workflow `.github/workflows/windows-native-validation.yml` builds and
tests the port on a clean GitHub-hosted Windows 2022 runner using MSYS2 CLANG64.
It runs on PRs touching the native runtime, renderer, build or tests. It can
also be launched explicitly with the GitHub Actions workflow-dispatch UI.

The job installs the dependencies listed in
`packaging/windows/README.md`, checks out submodules recursively, executes
`bash build.sh` and `bash tools/run_tests.sh`. It only uploads unit-test
**text logs** on failure (seven-day retention); no game files, save data,
private gameplay logs or shader caches are included.

The Windows-hosted build can consume significant GitHub Actions resources;
watch quota and runner availability. If package install or submodule download
fails, that is an infrastructure blocker, not a renderer regression.
Do not suppress build/test errors to obtain a green badge.

Passing means clean Windows compilation/linking and C/C++ and Python unit
tests passed. It is **not** GPU/driver/real gameplay evidence. Black-screen
issues #43/#47, graphics artifacts, scene-specific flicker, and #68's
cutscene stutters require separate matching hardware/capture validation.

## Selecting the correct MSYS2 installation

GitHub-hosted Windows runners can have multiple MSYS2 installations: a
preinstalled one under `C:\\msys64` and the current `msys2/setup-msys2`
instance under a temporary `D:\\a\\_temp\\msys64` path. The helper
`msys2-env.sh` must prepend the **active shell's** virtual `/clang64/bin`
and `/usr/bin` paths, rather than selecting the unrelated `C:\\msys64`
installation merely because it exists. Mixing those roots may make all
native tests fail silently due to incompatible dependent runtime DLLs.

An explicitly configured `MSYS2_ROOT` still takes priority (e.g. an
external Git Bash shell). Only when neither the override nor the current
virtual MSYS2 root is usable does the helper fall back to `C:\\msys64`.
The native CI must verify that unit-test binaries **execute** after the build;
compilation alone is not a passing result.
