"""Offline authenticated QUIC decoding research. No packet capture or process reads on import."""
from __future__ import annotations
from dataclasses import dataclass
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives.ciphers.aead import AESGCM

class DecodeError(ValueError): pass

def ecb(key: bytes, block: bytes, decrypt=False):
    if len(key)!=32 or len(block)!=16: raise DecodeError('AES-256 dimensions')
    cipher=Cipher(algorithms.AES(key),modes.ECB())
    ctx=cipher.decryptor() if decrypt else cipher.encryptor()
    return ctx.update(block)+ctx.finalize()

def xor(a,b):
    if len(a)!=len(b): raise DecodeError('XOR dimensions')
    return bytes(x^y for x,y in zip(a,b))

def nonce_for(iv: bytes, pn: int):
    if len(iv)!=12 or not 0<=pn<(1<<62): raise DecodeError('Nonce dimensions')
    return xor(iv,pn.to_bytes(12,'big'))

def reconstruct(truncated: int, bits: int, largest: int|None):
    if bits not in (8,16,24,32): raise DecodeError('Packet number width')
    if largest is None: return truncated
    expected=largest+1; window=1<<bits; half=window//2
    candidate=(expected&~(window-1))|truncated
    if candidate+half<=expected and candidate+window<(1<<62): candidate+=window
    elif candidate>expected+half and candidate>=window: candidate-=window
    return candidate

@dataclass(frozen=True)
class Unprotected:
    header: bytes
    encrypted: bytes
    packet_number: int
    cid: bytes
    key_phase: int

def unprotect(udp: bytes, hp: bytes, wrapper: int, largest=None, cid_bytes=20):
    if len(udp)>65535 or wrapper<0 or wrapper>len(udp): raise DecodeError('Datagram bounds')
    q=udp[wrapper:]; pn_offset=1+cid_bytes
    sample_offset=pn_offset+4
    if len(q)<sample_offset+16 or q[0]&0x80 or not q[0]&0x40: raise DecodeError('Not a complete QUIC short header')
    mask=ecb(hp,q[sample_offset:sample_offset+16]); first=q[0]^(mask[0]&0x1f)
    if first&0x18: raise DecodeError('Reserved bits')
    n=(first&3)+1
    if len(q)<pn_offset+n+16: raise DecodeError('Missing authentication tag')
    pn=bytes(q[pn_offset+i]^mask[i+1] for i in range(n))
    full=reconstruct(int.from_bytes(pn,'big'),8*n,largest)
    return Unprotected(bytes([first])+q[1:pn_offset]+pn,q[pn_offset+n:],full,q[1:pn_offset],(first>>2)&1)

def decrypt(udp: bytes, hp: bytes, payload: bytes, iv: bytes, wrapper: int, largest=None):
    u=unprotect(udp,hp,wrapper,largest)
    try: plain=AESGCM(payload).decrypt(nonce_for(iv,u.packet_number),u.encrypted,u.header)
    except Exception as exc: raise DecodeError('Packet authentication failed') from exc
    return u,plain

def recover_nonce(u: Unprotected, payload_key: bytes):
    """Validate a 96-bit GCM nonce candidate using known ciphertext/AAD/tag.

    This is a key-candidate verifier, not key recovery: the AES key is required.
    An independent packet must authenticate under the same derived session IV.
    """
    c,tag=u.encrypted[:-16],u.encrypted[-16:]
    if not c or len(payload_key)!=32: return None
    aes=AESGCM(payload_key); zero=bytes(12)
    keystream=aes.encrypt(zero,bytes(len(c)),b'')[:-16]
    synthetic_plain=xor(c,keystream)
    synthetic=aes.encrypt(zero,synthetic_plain,u.header)
    if synthetic[:-16]!=c: raise DecodeError('GCM verification construction failed')
    j0=ecb(payload_key,xor(xor(tag,synthetic[-16:]),ecb(payload_key,bytes(15)+b'\x01')),True)
    if j0[12:]!=b'\0\0\0\x01': return None
    nonce=j0[:12]
    try: plain=aes.decrypt(nonce,u.encrypted,u.header)
    except Exception: return None
    return nonce,plain

def varint(data: bytes, pos: int):
    if pos<0 or pos>=len(data): raise DecodeError('Truncated varint')
    n=1<<(data[pos]>>6)
    if pos+n>len(data): raise DecodeError('Truncated varint')
    return int.from_bytes(data[pos:pos+n],'big')&((1<<(8*n-2))-1),pos+n

@dataclass(frozen=True)
class StreamFrame:
    stream: int
    offset: int
    data: bytes
    fin: bool
    plain_offset: int=0

def frames(plain: bytes, datagrams=None,ack_ranges=None):
    out=[];p=0;count=0
    while p<len(plain):
        count+=1
        if count>4096: raise DecodeError('Frame count bound')
        kind,p=varint(plain,p)
        if kind in (0,1,0x1e): continue
        if 8<=kind<=15:
            stream,p=varint(plain,p); offset=0
            if kind&4: offset,p=varint(plain,p)
            if kind&2: length,p=varint(plain,p)
            else: length=len(plain)-p
            if length>len(plain)-p: raise DecodeError('Truncated STREAM data')
            out.append(StreamFrame(stream,offset,plain[p:p+length],bool(kind&1),p));p+=length
        elif kind in (2,3):
            largest,p=varint(plain,p);delay,p=varint(plain,p);ranges,p=varint(plain,p);first,p=varint(plain,p)
            if ranges>1024 or first>largest: raise DecodeError('Invalid ACK ranges')
            smallest=largest-first
            if ack_ranges is not None:ack_ranges.append((smallest,largest))
            for _ in range(ranges):
                gap,p=varint(plain,p);size,p=varint(plain,p)
                if smallest<gap+2 or size>smallest-gap-2: raise DecodeError('ACK range underflow')
                high=smallest-gap-2;smallest=high-size
                if ack_ranges is not None:ack_ranges.append((smallest,high))
            if kind==3:
                for _ in range(3): _,p=varint(plain,p)
        elif kind in (4,5,6,7,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d):
            if kind==4:
                for _ in range(3): _,p=varint(plain,p)
            elif kind in (5,0x11,0x15):
                for _ in range(2): _,p=varint(plain,p)
            elif kind in (6,7):
                if kind==6: _,p=varint(plain,p)
                n,p=varint(plain,p)
                if n>len(plain)-p: raise DecodeError('Truncated CRYPTO/token data')
                p+=n
            elif 0x10<=kind<=0x17 or kind==0x19: _,p=varint(plain,p)
            elif kind==0x18:
                seq,p=varint(plain,p);retire,p=varint(plain,p)
                if retire>seq or p>=len(plain): raise DecodeError('Invalid connection ID')
                n=plain[p];p+=1
                if not 1<=n<=20 or p+n+16>len(plain): raise DecodeError('Truncated connection ID')
                p+=n+16
            elif kind in (0x1a,0x1b):
                if p+8>len(plain): raise DecodeError('Truncated path challenge')
                p+=8
            else:
                _,p=varint(plain,p)
                if kind==0x1c: _,p=varint(plain,p)
                n,p=varint(plain,p)
                if n>len(plain)-p: raise DecodeError('Truncated close reason')
                p+=n
        elif kind in (0x30,0x31):
            if kind&1: n,p=varint(plain,p)
            else: n=len(plain)-p
            if n>len(plain)-p: raise DecodeError('Truncated DATAGRAM')
            # Keep these separate until their application framing is validated.
            if datagrams is not None:datagrams.append(plain[p:p+n])
            p+=n
        else: raise DecodeError(f'Unsupported QUIC frame type {kind}')
    return out

class Stream:
    """Bounded, overlap-checked reassembly from stream offset zero."""
    def __init__(self,limit=2*1024*1024,origin=0):
        if origin<0: raise DecodeError('Invalid stream origin')
        self.limit=limit;self.origin=origin;self.buffer=bytearray();self.present=bytearray();self.contiguous=0;self.delivered=0;self.final=None
    def add(self,offset,data,fin=False):
        offset-=self.origin
        if offset<0 or offset+len(data)>self.limit: raise DecodeError('Stream budget exceeded')
        end=offset+len(data)
        if self.final is not None and end>self.final: raise DecodeError('Data beyond stream final size')
        if fin:
            if self.final is not None and self.final!=end: raise DecodeError('Conflicting final size')
            if len(self.present)>end and any(self.present[end:]): raise DecodeError('Final size precedes existing data')
            self.final=end
        if end>len(self.buffer): self.buffer.extend(bytes(end-len(self.buffer)));self.present.extend(bytes(end-len(self.present)))
        for i,v in enumerate(data,offset):
            if self.present[i] and self.buffer[i]!=v: raise DecodeError('Conflicting retransmission')
        self.buffer[offset:end]=data;self.present[offset:end]=bytes([1])*len(data)
        while self.contiguous<len(self.present) and self.present[self.contiguous]: self.contiguous+=1
        out=bytes(self.buffer[self.delivered:self.contiguous]);self.delivered=self.contiguous;return out

class Records:
    """Profile-based framing only; does not invent a Roblox remote-call schema."""
    def __init__(self,limit=1024*1024): self.buffer=bytearray();self.limit=limit
    def feed(self,data):
        if len(self.buffer)+len(data)>self.limit+4: raise DecodeError('Record queue budget exceeded')
        self.buffer.extend(data);out=[]
        while len(self.buffer)>=4:
            word=int.from_bytes(self.buffer[:4],'big');size=word&0x7fffffff
            if size>self.limit or size==0: raise DecodeError('Invalid record size')
            if len(self.buffer)<size+4: break
            out.append((bool(word&0x80000000),bytes(self.buffer[4:4+size])));del self.buffer[:4+size]
        return out
