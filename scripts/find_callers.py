import struct
from pathlib import Path

to = Path("c:/projetos/fork/theorder1886-port")
raw = (to / "out/boot-linked.bin").read_bytes()

# The image starts at offset in raw:
pos = 56
procparam, = struct.unpack_from('<Q', raw, pos); pos += 8
main_tls = struct.unpack_from('<4Q', raw, pos); pos += 32
num_modules, = struct.unpack_from('<Q', raw, pos); pos += 8
pos += num_modules * 56
num_bindings, = struct.unpack_from('<Q', raw, pos); pos += 8
pos += num_bindings * 24
ns, = struct.unpack_from('<Q', raw, 24)
pos += ns * 24
ni, = struct.unpack_from('<Q', raw, 40)
pos += ni * 128
nr, = struct.unpack_from('<Q', raw, 32)
pos += nr * 32
size, = struct.unpack_from('<Q', raw, 8)
image = raw[pos:pos + size]

plt_targets = {
    0xef3f00: "m5-2bsNfv7s",
    0xef3f10: "waPcxYiR3WA",
    0xef4730: "NLq2b1jOaN0",
    0xef3d90: "FXPWHNk8Of0",
    0xef3670: "Xs9hdiD7sAA"
}

for plt, name in plt_targets.items():
    print(f"\nSearching callers of PLT stub {hex(plt)} ({name})...")
    callers = []
    # E8 rel32 (call rel32)
    for off in range(0, 0xef0000):
        if image[off] == 0xe8:
            rel32, = struct.unpack_from('<i', image, off + 1)
            if off + 5 + rel32 == plt:
                callers.append(off)
    print(f"  Found {len(callers)} call sites: {[hex(c) for c in callers[:5]]}")
    for c in callers[:3]:
        snippet = image[max(0, c - 24):c + 24].hex()
        print(f"    Call at {hex(c)}: ...{snippet}...")
