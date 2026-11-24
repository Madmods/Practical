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
    def test_bulk_xor_matches_bytewise_reference(self):
        for size in (0,1,12,16,31,1200,4096,65535):
            a=bytes(i%256 for i in range(size));b=bytes((255-i)%256 for i in range(size))
            self.assertEqual(xor(a,b),bytes(x^y for x,y in zip(a,b)))
        with self.assertRaises(DecodeError):xor(b'a',b'ab')
    def test_prepared_nonce_verifier_reuses_setup_and_remains_authenticated(self):
        verifier=NonceVerifier(self.key)
        for size in (1,16,31,1200,32000,64000,500,1200):
            plain=bytes(size);u=unprotect(packet(self.hp,self.key,self.iv,100+size,plain),self.hp,31)
            nonce,decoded=verifier.recover(u);self.assertEqual(decoded,plain);self.assertEqual(nonce_for(nonce,u.packet_number),self.iv)
            self.assertLessEqual(verifier.stream_bytes,65536);self.assertLessEqual(len(verifier.streams),4)
            damaged=Unprotected(u.header,u.encrypted[:-1]+bytes([u.encrypted[-1]^1]),u.packet_number,u.cid,u.key_phase)
            self.assertIsNone(verifier.recover(damaged))
        self.assertIsNone(NonceVerifier(bytes(32)).recover(u))
    def test_discovery_handles_packet_number_width_changes_and_wraps(self):
        from validate_live_transport import discover
        for wrapper in (23,31):
            for base,widths in ((100000,(1,2)),(100000,(1,3)),(100000,(2,1)),(65535,(2,2)),(255,(1,1))):
                with self.subTest(wrapper=wrapper,base=base,widths=widths):
                    rows=[dict(payload=packet(self.hp,self.key,self.iv,base+i,self.plain,wrapper,pnlen=width)) for i,width in enumerate(widths)]
                    context=discover(rows,[self.hp,self.key],wrapper);self.assertIsNotNone(context)
                    for row in rows:
                        u,plain=decrypt(row['payload'],context['hp'],context['key'],context['iv'],wrapper,context['largest'])
                        self.assertEqual(plain,self.plain)

    def test_discovery_skips_unavailable_old_epoch_and_damaged_first_packet(self):
        from validate_live_transport import discover
        current=[dict(payload=packet(self.hp,self.key,self.iv,100+i,self.plain)) for i in range(2)]
        old=dict(payload=packet(self.hp,bytes([9])*32,self.iv,50,self.plain))
        damaged=bytearray(current[0]['payload']);damaged[-1]^=1
        for prefix in (old,dict(payload=bytes(damaged))):
            self.assertIsNotNone(discover([prefix]+current,[self.hp,self.key],31))

    def test_discovery_still_requires_two_distinct_authenticated_datagrams(self):
        from validate_live_transport import discover
        good=dict(payload=packet(self.hp,self.key,self.iv,100,self.plain))
        wrong=dict(payload=packet(self.hp,bytes([9])*32,self.iv,101,self.plain))
        other_cid=dict(payload=packet(self.hp,self.key,self.iv,101,self.plain,cid=bytes([7])*20))
        for rows in ([good],[good,good],[wrong,good],[good,other_cid]):
            self.assertIsNone(discover(rows,[self.hp,self.key],31))
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
