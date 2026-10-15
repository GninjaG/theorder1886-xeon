import re

with open('out/eboot.elf', 'rb') as f:
    data = f.read()

terms = [b'skip', b'cutscene', b'cinematic', b'movie', b'bink']
for t in terms:
    matches = list(re.finditer(re.escape(t), data, re.IGNORECASE))
    print(f'=== Term: {t.decode()} (found {len(matches)}) ===')
    seen = set()
    for m in matches:
        s0 = data.rfind(b'\x00', 0, m.start()) + 1
        s1 = data.find(b'\x00', m.end())
        if 0 < s1 - s0 < 200:
            snippet = data[s0:s1]
            if snippet not in seen:
                seen.add(snippet)
                try:
                    s_str = snippet.decode('latin1')
                    if any(c.isalnum() for c in s_str):
                        print(f'  0x{s0:x}: {s_str}')
                except:
                    pass
        if len(seen) >= 20:
            break
