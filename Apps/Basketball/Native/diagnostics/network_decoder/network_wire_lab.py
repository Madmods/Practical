"""Independent bounded Roblox wire reader. Unknown schema/types fail closed.

Protocol references (not vendored source):
https://github.com/userinfected/raknet-decryption
https://github.com/topologyorgboi/Roblox-RakNet-Decompilation-Project
Current record opcode map is extracted as protocol data from the user-provided Yuna.exe.
"""
import math,struct,sys,pathlib,json
sys.path.insert(0,str(pathlib.Path(__file__).parent/'network_lab_deps'))
import zstandard
from network_transport_lab import DecodeError

class Reader:
    def __init__(self,data): self.data=data;self.pos=0
    def take(self,n):
        if n<0 or n>len(self.data)-self.pos:raise DecodeError('Truncated wire field')
        out=self.data[self.pos:self.pos+n];self.pos+=n;return out
    def integer(self,n,order='little'):return int.from_bytes(self.take(n),order)
    def byte(self):return self.integer(1)
    def uint(self):
        value=0
        for shift in range(0,70,7):
            b=self.byte()
            if shift==63 and b>1:raise DecodeError('Oversized varint')
            value|=(b&127)<<shift
            if not b&128:return value
        raise DecodeError('Oversized varint')
    def count(self,maximum):
        n=self.uint()
        if n>maximum:raise DecodeError('Wire collection budget exceeded')
        return n
    def string(self,maximum=8192):return self.take(self.count(maximum)).decode('utf-8',errors='strict')
    def float(self,n):
        value=struct.unpack('<f' if n==4 else '<d',self.take(n))[0]
        if not math.isfinite(value):raise DecodeError('Non-finite wire number')
        return value
    def ref(self):
        peer=self.uint()
        if peer>0xffffffff:raise DecodeError('Reference peer exceeds bound')
        return (peer,self.integer(4)) if peer else (0,0)

class Schema:
    def __init__(self):self.enums=[];self.classes=[];self.properties=[];self.events=[];self.content=[];self.strings=[];self.replication_properties={};self.opaque_extension=b''
    @classmethod
    def from_packet(cls,data):
        r=Reader(data);n=r.integer(4);expanded=r.integer(4)
        if n>16*1024*1024 or expanded>32*1024*1024:raise DecodeError('Schema compression budget exceeded')
        raw=zstandard.ZstdDecompressor().decompress(r.take(n),max_output_size=32*1024*1024)
        if len(raw)!=expanded:raise DecodeError('Schema expanded length mismatch')
        if r.pos!=len(data):raise DecodeError('Trailing schema bytes')
        q=Reader(raw);version=q.byte()
        if version>3:raise DecodeError('Unknown schema version')
        if version>=3:q.take(8)
        s=cls()
        for i in range(q.count(10000)):s.enums.append(dict(id=i,name=q.string(),bits=q.byte()))
        class_count=q.count(20000);property_count=q.count(100000);event_count=q.count(100000)
        for i in range(class_count):
            c=dict(id=i,name=q.string(),properties=[],events=[])
            for _ in range(q.count(10000)):
                name=q.string();kind=q.byte();enum=q.integer(2,'big') if kind==7 else 0
                if kind==60:q.byte()
                prop=dict(id=len(s.properties),name=name,kind=kind,enum=enum,className=c['name']);s.properties.append(prop);c['properties'].append(prop)
            c['flags']=q.integer(2,'big')
            for _ in range(q.count(10000)):
                name=q.string();args=[(q.byte(),q.integer(2,'big')) for _ in range(q.count(64))]
                event=dict(id=len(s.events),name=name,args=args,className=c['name']);s.events.append(event);c['events'].append(event)
            s.classes.append(c)
        if len(s.properties)!=property_count or len(s.events)!=event_count:raise DecodeError('Schema declared counts disagree with parsed counts')
        s.content=[q.string() for _ in range(q.count(10000))];s.strings=[q.string() for _ in range(q.count(100000))]
        if version==3 and q.pos<len(raw):
            count=q.integer(4)
            if count>class_count:raise DecodeError('Schema extension class count')
            for _ in range(count):
                class_id=q.integer(4);size=q.integer(4)
                if class_id>=class_count or class_id in s.replication_properties or size>property_count:raise DecodeError('Schema extension class bounds')
                ids=[q.integer(4) for _ in range(size)]
                if len(set(ids))!=len(ids) or any(p>=property_count or s.properties[p]['className']!=s.classes[class_id]['name'] for p in ids):raise DecodeError('Schema extension property owner mismatch')
                s.replication_properties[class_id]=ids
            # Current v3 has an eight-byte ancillary trailer. It is retained,
            # never interpreted as event/type metadata or used for argument reads.
            if len(raw)-q.pos!=8:raise DecodeError('Unknown schema extension trailer size')
            s.opaque_extension=q.take(8)
        if q.pos!=len(raw):raise DecodeError('Unrecognized trailing schema fields')
        return s

class Wire:
    def __init__(self,schema=None,mapping=None):self.schema=schema;self.mapping=mapping or list(range(256));self.cache={};self.instances={}
    def typed(self,r,kind,enum=0,depth=0):
        if depth>16:raise DecodeError('Argument nesting limit')
        if kind==0:return None
        if kind==1:
            slot=r.byte()
            if slot==0:return ''
            if slot<128:
                if slot not in self.cache:raise DecodeError('Missing cached string')
                return self.cache[slot]
            size=r.integer(4,'big')
            if size>8192:raise DecodeError('Cached string budget exceeded')
            value=r.take(size).decode('utf-8',errors='strict');self.cache[slot&127]=value;return value
        if kind==2:return r.string()
        if kind==7:
            if not self.schema or enum>=len(self.schema.enums):raise DecodeError('Enum schema unavailable')
            bits=self.schema.enums[enum]['bits'];n=(bits+7)//8
            if not 1<=n<=8:raise DecodeError('Enum width unsupported')
            return {'type':'Enum','enum':self.schema.enums[enum]['name'],'value':r.integer(n,'big')}
        if kind==9:
            value=r.byte()
            if value>1:raise DecodeError('Invalid boolean')
            return bool(value)
        if kind in (10,44):
            value=r.uint();return (value>>1)^-(value&1)
        if kind in (11,12):return r.float(4 if kind==11 else 8)
        if kind in (19,21,22):return {'type':{19:'Color3',21:'Vector2',22:'Vector3'}[kind],'components':[r.float(4) for _ in range(2 if kind==21 else 3)]}
        if kind==20:return {'type':'Color3uint8','components':list(r.take(3))}
        if kind==28:
            peer,ident=r.ref();return None if not peer else {'type':'Instance','peer':peer,'id':ident,'resolved':False}
        if kind in (29,30):return [self.variant(r,depth+1) for _ in range(r.count(128))]
        if kind in (31,32):
            result={}
            for _ in range(r.count(128)):
                key=r.string(256)
                if key in result:raise DecodeError('Duplicate dictionary key')
                result[key]=self.variant(r,depth+1)
            return result
        if kind==49:
            h=r.byte()
            if h<128:return r.take(h if h<127 else r.count(8192)).decode('utf-8',errors='strict')
            i=(h&127) if (h&127)<127 else r.uint()
            if not self.schema or i>=len(self.schema.strings):raise DecodeError('Optimized string schema unavailable')
            return self.schema.strings[i]
        raise DecodeError(f'Unsupported wire type {kind}')
    def variant(self,r,depth):
        kind=r.byte();enum=r.integer(2,'big') if kind==7 else 0;return self.typed(r,kind,enum,depth)
    def leading_event(self,body,direction):
        if len(body)<2 or body[0]!=0x83 or self.mapping[body[1]]!=7:return None
        r=Reader(body);r.take(2);peer,ident=r.ref();event_id=r.integer(2)
        if not self.schema or event_id>=len(self.schema.events):raise DecodeError('Join/member schema unavailable')
        event=self.schema.events[event_id]
        methods={'FireServer':('outgoing','FireServer'),'OnRemoteServerEvent':('outgoing','FireServer'),'OnClientEvent':('incoming','OnClientEvent'),'InvokeServer':('outgoing','InvokeServer'),'OnClientInvoke':('incoming','OnClientInvoke'),'RemoteOnInvokeServer':('outgoing','InvokeServer'),'RemoteOnInvokeClient':('incoming','OnClientInvoke'),'RemoteOnInvokeSuccess':('incoming','InvokeResult'),'RemoteOnInvokeError':('incoming','InvokeError')}
        if event['name'] not in methods:return None
        expected,method=methods[event['name']]
        if expected!=direction:raise DecodeError('Method direction disagrees with schema')
        if event['className'] not in ('RemoteEvent','UnreliableRemoteEvent','RemoteFunction','BaseRemoteEvent'):raise DecodeError('Member does not belong to a remote class')
        args=[self.typed(r,kind,enum) for kind,enum in event['args']]
        if len(event['args'])==1 and event['args'][0][0]==29:args=args[0]
        invocation_id=None
        if event['name'] in ('RemoteOnInvokeServer','RemoteOnInvokeClient'):
            invocation_id=args[0];args=args[-1]
        if len(args)>64:raise DecodeError('Invocation argument limit')
        return dict(path=f'Unresolved instance {peer}:{ident}',method=method,args=args,peer=peer,instance=ident,eventId=event_id,invocationId=invocation_id,source='Authenticated traffic',consumed=r.pos,recordBytes=len(body),pathResolved=False)
    def events(self,body,direction):
        if not body or body[0]!=0x83:return []
        r=Reader(body);r.byte();calls=[]
        while r.pos<len(body) and len(calls)<64:
            if not any(body[r.pos:]):break
            start=r.pos;op=self.mapping[r.byte()]
            if op==35:r.take(4)
            elif op==10:
                peer,ident=r.ref();property_id=r.integer(2);versioned=r.byte()
                if not peer or not self.schema or property_id>=len(self.schema.properties) or versioned>1:raise DecodeError('Invalid property acknowledgement header')
                prop=self.schema.properties[property_id]
                if prop['name']!='CFrame' or prop['className'] not in ('Part','MeshPart') or prop['kind']!=27:raise DecodeError('Unsupported property acknowledgement schema')
                if versioned and r.uint()>0xffffffff:raise DecodeError('Property acknowledgement version bound')
                r.take(4)
            elif op==7:
                call=self.leading_event(b'\x83'+body[start:],direction)
                if call is None:break
                r.pos=start+call['consumed']-1;call.update(consumed=r.pos,recordBytes=len(body),recordOffset=start);calls.append(call)
            else:break
        return calls
