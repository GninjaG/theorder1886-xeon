import struct

with open(r'./out/eboot.elf', 'rb') as f:
    data = f.read()

for off in range(0x1753200, 0x1753550, 0x18):
    fn = struct.unpack('<Q', data[off:off+8])[0]
    str_ptr = struct.unpack('<Q', data[off+8:off+16])[0]
    s = ""
    if str_ptr < len(data) - 8:
        # maybe str_ptr points to a char* pointer
        p2 = struct.unpack('<Q', data[str_ptr:str_ptr+8])[0]
        if p2 < len(data):
            end = data.find(b'\x00', p2)
            if end != -1 and end - p2 < 100:
                s = data[p2:end].decode('latin1', 'replace')
    print(f'off 0x{off:x}: fn=0x{fn:x}, str_ptr=0x{str_ptr:x}, name="{s}"')
