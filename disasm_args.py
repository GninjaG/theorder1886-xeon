import subprocess

with open(r'C:\Users\Mateus\Downloads\games\CUSA00035\eboot.bin', 'rb') as f:
    f.seek(0x453700)
    data = f.read(0x300)

with open('scratch_disasm.bin', 'wb') as f:
    f.write(data)

cmd = [
    r'C:\msys64\clang64\bin\x86_64-w64-mingw32-objdump.exe',
    '-D', '-b', 'binary', '-m', 'i386:x86-64',
    '--adjust-vma=0x453700',
    'scratch_disasm.bin'
]
res = subprocess.run(cmd, capture_output=True, text=True)
if res.returncode != 0:
    # try llvm-objdump --arch=x86-64
    cmd2 = [
        r'C:\msys64\clang64\bin\llvm-objdump.exe',
        '-D', '--arch=x86_64',
        'scratch_disasm.bin'
    ]
    res = subprocess.run(cmd2, capture_output=True, text=True)
print(res.stdout[:4000])
