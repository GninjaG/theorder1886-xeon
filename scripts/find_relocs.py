import struct

with open("out/boot-linked.bin", "rb") as f:
    hdr = f.read(56)
    magic, img_len, entry, n_segments, n_relocs, n_names, flags = struct.unpack("<8s6Q", hdr)
    procparam, = struct.unpack("<Q", f.read(8))
    tls = struct.unpack("<4Q", f.read(32))
    n_table, = struct.unpack("<Q", f.read(8))
    f.seek(n_table * 56, 1)
    n_bindings, = struct.unpack("<Q", f.read(8))
    f.seek(n_bindings * 24, 1)
    f.seek(n_segments * 24, 1)
    names = [f.read(128).strip(b"\0").decode("latin1") for _ in range(n_names)]
    relocs = [struct.unpack("<QQqq", f.read(32)) for _ in range(n_relocs)]

target_sym = 0x2983a0
for r in relocs:
    target, kind, sym, addend = r
    if sym == target_sym:
        print(f"Reloc at 0x{target:x} points to 0x{target_sym:x}")
