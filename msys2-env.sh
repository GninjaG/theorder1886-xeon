# Sourced by Windows build/test/run scripts. Prefer the active MSYS2
# virtual /clang64 root (GitHub Actions may install under D:/a/_temp/msys64)
# instead of a stale C:/msys64 from another installation.
# MSYS2_ROOT remains an explicit override for Git Bash and external shells.
if [[ -n ${MSYS2_ROOT:-} && -d ${MSYS2_ROOT}/clang64/bin ]]; then
    export PATH="${MSYS2_ROOT}/clang64/bin:${MSYS2_ROOT}/usr/bin:$PATH"
    export MSYSTEM=CLANG64
elif [[ -d /clang64/bin && -d /usr/bin ]]; then
    export PATH="/clang64/bin:/usr/bin:$PATH"
    export MSYSTEM=CLANG64
elif [[ -d /c/msys64/clang64/bin ]]; then
    export PATH="/c/msys64/clang64/bin:/c/msys64/usr/bin:$PATH"
    export MSYSTEM=CLANG64
fi
