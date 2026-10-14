import struct

with open('out/boot-linked.bin', 'rb') as f:
    hdr = f.read(56)
    magic, img_len, entry, n_segments, n_relocs, n_names, flags = struct.unpack('<8s6Q', hdr)
    procparam, = struct.unpack('<Q', f.read(8))
    tls = struct.unpack('<4Q', f.read(32))
    n_table, = struct.unpack('<Q', f.read(8))
    f.seek(n_table * 56, 1)
    n_bindings, = struct.unpack('<Q', f.read(8))
    f.seek(n_bindings * 24, 1)
    f.seek(n_segments * 24, 1)
    f.seek(n_names * 128, 1)
    f.seek(n_relocs * 32, 1)
    image_offset = f.tell()
    f.seek(image_offset)
    image = f.read(img_len)

print(f"Loaded image of {len(image):,} bytes at offset 0x{image_offset:x}")

def get_str(addr):
    if addr >= len(image): return '<out of bounds>'
    s = []
    while addr < len(image) and image[addr] != 0:
        c = image[addr]
        if 32 <= c <= 126:
            s.append(chr(c))
        else:
            s.append(f'\\x{c:02x}')
        addr += 1
    return ''.join(s)

addrs = [0x161aec8, 0x161aed0, 0x161aed8, 0x161aee0, 0x161aee8, 0x161aef0, 0x161aef8, 0x161af00]
print("Strings from table:")
for a in addrs:
    print(f'  0x{a:x}: "{get_str(a)}"')

# Search for RIP-relative references to 0x16b3418 in code
target_ptr = 0x16b3418
print(f"\nSearching for all instructions referencing 0x{target_ptr:x}...")
# Scan up to 0x15b9ce8 (end of main eboot code segment)
for i in range(0, min(len(image), 0x15b9ce8) - 4):
    disp = struct.unpack('<i', image[i:i+4])[0]
    # instruction ending at i+4
    if i + 4 + disp == target_ptr:
        prefix = image[max(0, i-4):i]
        print(f"  Target ref at pc_after=0x{i+4:x} (disp at 0x{i:x}): prefix {prefix.hex()}")
