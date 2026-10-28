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
