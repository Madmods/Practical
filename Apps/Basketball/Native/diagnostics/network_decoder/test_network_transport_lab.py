import unittest
from network_transport_lab import *

def packet(hp,key,iv,pn,plain,wrapper=31,cid=bytes(range(20)),pnlen=4):
    header=bytes([0x40|(pnlen-1)])+cid+(pn&((1<<(8*pnlen))-1)).to_bytes(pnlen,'big')
    protected=bytearray(header+AESGCM(key).encrypt(nonce_for(iv,pn),plain,header))
    mask=ecb(hp,protected[25:41]);protected[0]^=mask[0]&0x1f
    for i in range(pnlen): protected[21+i]^=mask[i+1]
    return bytes(wrapper)+protected

class Transport(unittest.TestCase):
    hp=bytes(range(32));key=bytes(reversed(range(32)));iv=bytes(range(12))
    plain=b'\x0a\x00\x0bhello world'+b'\0'*24
    def test_aes256_nist(self):
        key=bytes.fromhex('000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f');plain=bytes.fromhex('00112233445566778899aabbccddeeff')
        self.assertEqual(ecb(key,plain).hex(),'8ea2b7ca516745bfeafc49904b496089');self.assertEqual(ecb(key,ecb(key,plain),True),plain)
    def test_authenticated_decode(self):
        for wrapper in (23,31):
            for width in (1,2,3,4):
                data=packet(self.hp,self.key,self.iv,77,self.plain,wrapper,pnlen=width)
                u,p=decrypt(data,self.hp,self.key,self.iv,wrapper,76);self.assertEqual(p,self.plain);self.assertEqual(u.packet_number,77)
    def test_tampering_rejected(self):
        data=bytearray(packet(self.hp,self.key,self.iv,77,self.plain));data[-1]^=1
        with self.assertRaises(DecodeError): decrypt(data,self.hp,self.key,self.iv,31)
    def test_wrong_keys_rejected(self):
        data=packet(self.hp,self.key,self.iv,77,self.plain)
        with self.assertRaises(DecodeError): decrypt(data,self.hp,bytes(32),self.iv,31)
    def test_derive_and_independently_validate_iv(self):
        ivs=[]
        for pn in (77,78):
            u=unprotect(packet(self.hp,self.key,self.iv,pn,self.plain),self.hp,31)
            nonce,plain=recover_nonce(u,self.key);self.assertEqual(plain,self.plain);ivs.append(nonce_for(nonce,pn))
        self.assertEqual(ivs,[self.iv,self.iv])
    def test_wrong_candidate_not_authenticated(self):
        u=unprotect(packet(self.hp,self.key,self.iv,77,self.plain),self.hp,31)
        self.assertIsNone(recover_nonce(u,bytes(32)))
    def test_pn_wrap(self):
        self.assertEqual(reconstruct(1,8,255),257);self.assertEqual(reconstruct(255,8,257),255)
    def test_short_packets_rejected(self):
        for n in range(72):
            with self.assertRaises(DecodeError): unprotect(bytes(n),self.hp,31)
    def test_stream_frames(self):
        f=frames(self.plain);self.assertEqual(f,[StreamFrame(0,0,b'hello world',False,3)])
    def test_frame_bounds(self):
        for p in (b'\x0a',b'\x0a\0\x3fX',b'\x02\0\0\0\x01',b'\x1a\0',b'\x3f'):
            with self.assertRaises(DecodeError): frames(p)
    def test_out_of_order_and_retransmit(self):
        s=Stream();self.assertEqual(s.add(3,b'def',True),b'');self.assertEqual(s.add(0,b'abc'),b'abcdef');self.assertEqual(s.add(0,b'abcdef'),b'')
    def test_overlap_conflict(self):
        s=Stream();s.add(0,b'abcd')
        with self.assertRaises(DecodeError): s.add(2,b'X')
    def test_bounded_window_with_large_stream_offset(self):
        s=Stream(limit=16,origin=100_000_000)
        self.assertEqual(s.add(100_000_003,b'def'),b'')
        self.assertEqual(s.add(100_000_000,b'abc'),b'abcdef')
        self.assertEqual(len(s.buffer),6)
        with self.assertRaises(DecodeError): s.add(0,b'x')
    def test_stream_budget_and_final(self):
        s=Stream(4)
        with self.assertRaises(DecodeError): s.add(4,b'X')
        s.add(0,b'ab',True)
        with self.assertRaises(DecodeError): s.add(2,b'c')
    def test_split_records(self):
        r=Records();encoded=b'\0\0\0\x05hello';self.assertEqual(r.feed(encoded[:6]),[]);self.assertEqual(r.feed(encoded[6:]),[(False,b'hello')])
    def test_bad_record_length(self):
        for value in (b'\0\0\0\0',b'\x7f\xff\xff\xff'):
            with self.assertRaises(DecodeError): Records().feed(value)

if __name__=='__main__': unittest.main(verbosity=2)
