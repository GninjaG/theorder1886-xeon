import struct
import json
from pathlib import Path

to = Path("c:/projetos/fork/theorder1886-port")
raw = (to / "out/boot-linked.bin").read_bytes()

pos = 0
magic, size, entry, ns, nr, ni, flags = struct.unpack_from('<8s6Q', raw, pos)
pos += 56
procparam, = struct.unpack_from('<Q', raw, pos)
pos += 8
main_tls = struct.unpack_from('<4Q', raw, pos)
pos += 32
num_modules, = struct.unpack_from('<Q', raw, pos)
pos += 8
modules = [struct.unpack_from('<7Q', raw, pos + i * 56) for i in range(num_modules)]
pos += num_modules * 56
num_bindings, = struct.unpack_from('<Q', raw, pos)
pos += 8
bindings = [struct.unpack_from('<3Q', raw, pos + i * 24) for i in range(num_bindings)]
pos += num_bindings * 24
segments = [struct.unpack_from('<3Q', raw, pos + i * 24) for i in range(ns)]
pos += ns * 24
names = [raw[pos + i * 128:pos + (i + 1) * 128].split(b'\0')[0].decode('ascii', errors='replace') for i in range(ni)]
pos += ni * 128
relocs = [struct.unpack_from('<QQqq', raw, pos + i * 32) for i in range(nr)]
pos += nr * 32
image = raw[pos:pos + size]

target_nids = [
    "Xs9hdiD7sAA#libScePosix",
    "0Cq8ipKr9n0#libkernel",
    "FXPWHNk8Of0#libkernel",
    "m5-2bsNfv7s#libkernel",
    "waPcxYiR3WA#libkernel",
    "NLq2b1jOaN0#libkernel"
]

print("Scanning for call sites of target NIDs...")
for tnid in target_nids:
    if tnid not in names:
        print(f"NID {tnid} not found in names table!")
        continue
    idx = names.index(tnid)
    # find GOT relocation
    got_targets = [r[0] for r in relocs if r[1] != 0 and r[2] == idx]
    print(f"\nTarget {tnid} (index {idx}): GOT slot(s) at {[hex(g) for g in got_targets]}")
    for g in got_targets:
        # Search for instructions referencing `g` in code (e.g. rip-relative call/jmp or mov)
        # Search for FF 15 [rel32] (call qword ptr [rip + offset]) or FF 25 [rel32] (jmp qword ptr [rip + offset])
        # rel32 = g - (insn_addr + 6)
        print(f"  Searching for references to GOT slot {hex(g)} in image...")
        refs = []
        for offset in range(0, min(len(image) - 6, 0x1700000)): # search main eboot text
            # check call [rip+rel32] (FF 15) or jmp [rip+rel32] (FF 25) or mov reg, [rip+rel32] (48 8B ..)
            b0, b1 = image[offset], image[offset + 1]
            if (b0 == 0xff and b1 in (0x15, 0x25)) or (b0 == 0x48 and b1 == 0x8b):
                rel32, = struct.unpack_from('<i', image, offset + (2 if b0 == 0xff else 3))
                insn_len = 6 if b0 == 0xff else 7
                if offset + insn_len + rel32 == g:
                    refs.append(offset)
        print(f"  Found {len(refs)} reference(s): {[hex(r) for r in refs[:5]]}")
        for r in refs[:3]:
            context = image[max(0, r - 32):r + 48]
            print(f"    Context at {hex(r)}: {context.hex()}")
