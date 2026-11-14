import ctypes, pathlib, unittest
P=pathlib.Path(__file__).parent
SBOX=[]
def mul(a,b):
    v=0
    for _ in range(8):
        if b&1:v^=a
        a=((a<<1)^ (0x11b if a&128 else 0))&255;b>>=1
    return v
for i in range(256):
    value=1
    for _ in range(254):value=mul(value,i)
    if not i:value=0
    rot=lambda n:((value<<n)|(value>>(8-n)))&255
    SBOX.append(value^rot(1)^rot(2)^rot(3)^rot(4)^0x63)
def expand(key):
    out=bytearray(key);r=1
    for p in range(32,240,4):
        t=out[p-4:p]
        if p%32==0:t=bytearray([SBOX[t[1]]^r,SBOX[t[2]],SBOX[t[3]],SBOX[t[0]]]);r=mul(r,2)
        elif p%32==16:t=bytearray(SBOX[x] for x in t)
        out.extend(out[p-32+i]^t[i] for i in range(4))
    return bytes(out)
class Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.dll=ctypes.CDLL(str(P/'scan_aes_schedules.dll'));cls.dll.ScanBuffer.argtypes=[ctypes.c_void_p,ctypes.c_size_t,ctypes.c_void_p,ctypes.c_uint];cls.dll.ScanBuffer.restype=ctypes.c_uint
    def scan(self,data):
        inp=ctypes.create_string_buffer(bytes(data));out=ctypes.create_string_buffer(32*512);count=self.dll.ScanBuffer(inp,len(data),out,512);return [out.raw[i*32:i*32+32] for i in range(count)]
    def test_zero_and_noise_rejected(self): self.assertEqual(self.scan(bytes(1024*1024)),[]);self.assertEqual(self.scan(bytes(range(256))*1024),[])
    def test_forward_schedule(self):
        k=bytes(range(32));self.assertEqual(self.scan(bytes(64)+expand(k)+bytes(64)),[k])
    def test_reversed_words(self):
        k=bytes(reversed(range(32)));s=expand(k);s=b''.join(s[i:i+4][::-1] for i in range(0,240,4));self.assertEqual(self.scan(s),[k])
    def test_corrupt_rejected(self):
        s=bytearray(expand(bytes(range(32))));s[180]^=1;self.assertEqual(self.scan(s),[])
    def test_deduplicated(self):
        k=bytes(range(32));self.assertEqual(self.scan(expand(k)*2),[k])
if __name__=='__main__':unittest.main(verbosity=2)
