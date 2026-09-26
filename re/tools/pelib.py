import struct, os, math
def u16(b,o): return struct.unpack_from('<H',b,o)[0]
def u32(b,o): return struct.unpack_from('<I',b,o)[0]
class PE:
    def __init__(self, path):
        self.path=path
        self.data=open(path,'rb').read(); d=self.data
        e=u32(d,0x3c); assert d[e:e+4]==b'PE\0\0'
        fh=e+4; self.machine=u16(d,fh); self.nsec=u16(d,fh+2); self.sizeopt=u16(d,fh+16)
        self.opt=fh+20; self.magic=u16(d,self.opt); p32p=(self.magic==0x20b); self.pe32p=p32p
        self.numdd=u32(d,self.opt+(0x6c if p32p else 0x5c))
        ddoff=self.opt+(0x70 if p32p else 0x60)
        self.dd=[(u32(d,ddoff+i*8),u32(d,ddoff+i*8+4)) for i in range(self.numdd)]
        so=self.opt+self.sizeopt; self.secs=[]
        for i in range(self.nsec):
            b=so+i*40
            self.secs.append(dict(name=d[b:b+8].rstrip(b'\0').decode('latin1'),
                vsize=u32(d,b+8),vaddr=u32(d,b+12),rsize=u32(d,b+16),raddr=u32(d,b+20)))
        self.imagebase=u32(d,self.opt+28) if not p32p else struct.unpack_from('<Q',d,self.opt+24)[0]
    def rva2off(self,rva):
        for s in self.secs:
            if s['vaddr']<=rva<s['vaddr']+max(s['vsize'],s['rsize']):
                o=s['raddr']+(rva-s['vaddr'])
                return o if o < len(self.data) else None
        return None
    def off2rva(self,off):
        for s in self.secs:
            if s['rsize'] and s['raddr']<=off<s['raddr']+s['rsize']:
                return s['vaddr']+(off-s['raddr'])
        return None
TYPES={1:'CURSOR',2:'BITMAP',3:'ICON',4:'MENU',5:'DIALOG',6:'STRING',7:'FONTDIR',8:'FONT',
9:'ACCEL',10:'RCDATA',11:'MSGTABLE',12:'GRP_CURSOR',14:'GRP_ICON',16:'VERSION',
17:'DLGINCLUDE',19:'PLUGPLAY',20:'VXD',21:'ANICURSOR',22:'ANIICON',23:'HTML',24:'MANIFEST',
240:'DLGINIT',241:'TOOLBAR'}
def _walk(p,base,off,level,path,out,seen):
    if off in seen: return
    seen.add(off)
    d=p.data
    nn=u16(d,off+12); ni=u16(d,off+14)
    for i in range(nn+ni):
        e=off+16+i*8
        nv=u32(d,e); ov=u32(d,e+4)
        if nv&0x80000000:
            no=base+(nv&0x7fffffff); ln=u16(d,no)
            nm='"'+d[no+2:no+2+ln*2].decode('utf-16-le',errors='replace')+'"'
        else:
            nm=(TYPES.get(nv,'#%d'%nv)+'(%d)'%nv) if level==0 else str(nv)
        if ov&0x80000000:
            _walk(p,base,base+(ov&0x7fffffff),level+1,path+[nm],out,seen)
        else:
            do=base+ov
            out.append((path+[nm],u32(d,do),u32(d,do+4),u32(d,do+8),p.rva2off(u32(d,do))))
def resources(p):
    rva,sz=p.dd[2]
    if not rva: return []
    base=p.rva2off(rva); out=[]
    _walk(p,base,base,0,[],out,set())
    return out
def entropy(b):
    if not b: return 0.0
    c=[0]*256
    for x in b: c[x]+=1
    n=len(b); h=0.0
    for v in c:
        if v: q=v/n; h-=q*math.log2(q)
    return h
