#!/usr/bin/env python3
"""Offset-free extraction of the FWFILE payload from an Endgame Gear updater .exe.
Stdlib only -> runs on Linux. No hardcoded file offsets, no hardcoded length."""
import struct, sys, os, hashlib, math
sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
from pelib import PE, resources, entropy

def find_resource_id_in_code(p, type_name='FWFILE'):
    """Recover the resource ID the binary actually passes to FindResourceW,
    by locating   PUSH <VA of L"FWFILE">  ;  PUSH imm32  ;  PUSH 0  ;  CALL [FindResourceW]
    Returns list of (code_rva, id)."""
    d = p.data
    lit = (type_name+'\0').encode('utf-16-le')
    # every raw offset where the wide literal appears, mapped to a VA
    vas = []
    i = d.find(lit)
    while i != -1:
        rva = p.off2rva(i)
        if rva is not None: vas.append(p.imagebase + rva)
        i = d.find(lit, i+1)
    hits = []
    for va in vas:
        pat = b'\x68' + struct.pack('<I', va)          # PUSH imm32 (the string VA)
        j = d.find(pat)
        while j != -1:
            # next: 68 id32   6A 00   FF 15 <thunk>
            if d[j+5] == 0x68 and d[j+10:j+12] == b'\x6a\x00' and d[j+12:j+14] == b'\xff\x15':
                rid = struct.unpack_from('<I', d, j+6)[0]
                hits.append((p.off2rva(j), rid, struct.unpack_from('<I',d,j+14)[0]))
            j = d.find(pat, j+1)
    return hits

def main(path, outdir='.'):
    p = PE(path)
    print('file      : %s (%d bytes)' % (os.path.basename(path), len(p.data)))
    res = [r for r in resources(p) if r[0][0] == '"FWFILE"']
    print('FWFILE res: %s' % ([ (r[0][1], r[0][2], r[2]) for r in res ]))
    code = find_resource_id_in_code(p)
    print('code sites : %s' % ([('rva 0x%X'%a, 'id %d'%i, 'thunk 0x%X'%t) for a,i,t in code]))
    ids_used = sorted({i for _,i,_ in code})
    if len(ids_used) != 1:
        print('!! could not pin a single resource ID from code -> DO NOT GUESS'); return 2
    want = ids_used[0]
    match = [r for r in res if r[0][1] == str(want)]
    if len(match) != 1:
        print('!! resource id %d not found exactly once' % want); return 2
    pl, rva, sz, cp, off = match[0]
    blob = p.data[off:off+sz]
    assert len(blob) == sz
    print('SELECTED   : type=FWFILE id=%d lang=%s  size=%d (0x%X)  rva=0x%X'%(want,pl[2],sz,sz,rva))
    print('  sha256   : %s' % hashlib.sha256(blob).hexdigest())
    # ---- validation ----
    H = entropy(blob)
    recs = [blob[i:i+1024] for i in range(0, len(blob), 1024)]
    import collections
    c = collections.Counter(recs)
    dup = max(c.values())
    print('  entropy  : %.4f bits/byte  (expect ~7.98 = encrypted; <6 or MZ header => WRONG blob)' % H)
    print('  looks-PE : %s' % (blob[:2] == b'MZ'))
    print('  records  : %d x 1024 (+%d remainder, zero-padded by the tool)' % (sz//1024, sz%1024))
    print('  distinct : %d, most-repeated record x%d' % (len(c), dup))
    print('  rec0 sum16 = 0x%04X   (device-checked additive checksum, cmd 0xA0/0x06 offset +4)'
          % (sum(recs[0]) & 0xFFFF))
    ok = (H > 7.5) and blob[:2] != b'MZ' and sz % 4 == 0
    print('  VERDICT  : %s' % ('plausible encrypted firmware image' if ok else 'REJECT'))
    out = os.path.join(outdir, 'FWFILE_%d.bin' % want)
    open(out,'wb').write(blob); print('  wrote    : %s' % out)
    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv[1], sys.argv[2] if len(sys.argv)>2 else '.'))
