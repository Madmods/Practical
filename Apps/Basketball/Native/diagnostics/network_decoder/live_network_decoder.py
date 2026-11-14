"""Incremental authenticated transport reader. Never sends or modifies packets."""
import collections,json,pathlib,time
from network_transport_lab import DecodeError,Stream,Records,frames,decrypt,unprotect,recover_nonce
from network_wire_lab import Schema,Wire,zstandard
from validate_live_transport import discover

MAPS=json.loads((pathlib.Path(__file__).parent/'replication-type-maps.json').read_text())
SUPPORTED_METHODS={'FireServer','InvokeServer','OnClientEvent','OnClientInvoke'}

def record_alignment(data,schema,mapping,direction):
    """Require three complete framed records and one schema-validated remote event."""
    for start in range(max(0,len(data)-11)):
        if data[start]!=0x80:continue
        pos=start;event=False;wire=Wire(schema,mapping)
        for _ in range(3):
            if pos+5>len(data) or data[pos]!=0x80:break
            size=int.from_bytes(data[pos:pos+4],'big')&0x7fffffff
            if not 1<=size<=65536 or pos+4+size>len(data):break
            body=bytes(data[pos+4:pos+4+size])
            if body[0] not in (0x83,0xf8,0xe5):break
            if body[0]==0x83:
                if len(body)<2 or mapping[body[1]]>35:break
                try:event=bool(wire.events(body,direction)) or event
                except (DecodeError,UnicodeError):break
            pos+=4+size
        else:
            if event:return start
    return None

class LiveDecoder:
    def __init__(self,keys,cached=None):
        self.keys=keys;self.groups={};self.streams={};self.schemas={};self.wires={};self.pending=collections.defaultdict(list)
        self.calls=collections.deque(maxlen=500);self.highlights=collections.deque(maxlen=16);self.counts=collections.Counter();self.errors=collections.Counter();self.seen=set()
        self.cached=cached or {};self.schema_packets={}
    def ingest(self,rows):
        for row in rows:
            if row['frame'] in self.seen:continue
            self.seen.add(row['frame'])
            if len(self.seen)>20000:raise DecodeError('Capture packet budget exceeded')
            group=(row['connection'],row['direction'],row['cid'])
            if group not in self.groups:
                if len(self.groups)>=16:continue
                self.groups[group]={'context':None,'pending':[],'attempt':None}
            state=self.groups[group]
            if state['context']:self.packet(row,state['context'])
            elif len(state['pending'])<10000:state['pending'].append(row)
        for group,state in self.groups.items():
            if state['context'] or len(state['pending'])<2:continue
            attempt=(len(self.keys),min(len(state['pending']),12))
            if state['attempt']==attempt:continue
            state['attempt']=attempt;wrapper=31 if group[1]=='outgoing' else 23
            context=discover(state['pending'],self.keys,wrapper)
            if context:
                state['context']=context;waiting=state['pending'];state['pending']=[]
                cached=self.cached.get(group[0])
                if cached and group[2] in cached[1]:
                    self.schemas[group[0]]=cached[0];self.schema_packets[group[0]]=cached[2];self.counts['cachedSchemaConnections']=len(self.schemas)
                for row in waiting:self.packet(row,context)
    def packet(self,row,context):
        try:
            wrapper=31 if row['direction']=='outgoing' else 23
            try:u,plain=decrypt(row['payload'],context['hp'],context['key'],context['iv'],wrapper,context['largest'])
            except DecodeError:
                u=unprotect(row['payload'],context['hp'],wrapper,context['largest']);recovered=recover_nonce(u,context['key'])
                if recovered is None:raise DecodeError('Packet authentication failed')
                _,plain=recovered
            if u.cid!=context['cid'] or u.key_phase!=context['phase']:raise DecodeError('Unsupported key phase')
            context['largest']=max(context['largest'],u.packet_number);self.counts['authenticatedPackets']+=1
            for frame in frames(plain):self.segment(row,frame)
        except DecodeError as exc:self.errors[str(exc)]+=1
    def segment(self,row,frame):
        key=(row['connection'],row['direction'],frame.stream)
        if key not in self.streams:
            if len(self.streams)>=64:raise DecodeError('Stream count budget exceeded')
            if frame.offset and row['connection'] not in self.schemas:return
            self.streams[key]={'stream':Stream(limit=8*1024*1024,origin=frame.offset),'header':bytearray(),'ready':False,'control':False,'blocked':False,'records':Records()}
        state=self.streams[key];data=state['stream'].add(frame.offset,frame.data,frame.fin)
        if state['blocked']:return
        if not state['ready']:
            state['header'].extend(data)
            if len(state['header'])>65536:state['blocked']=True;raise DecodeError('Record alignment budget exceeded; rejoin required')
            if state['stream'].origin:
                aligned=record_alignment(state['header'],self.schemas[row['connection']],MAPS['3'],row['direction'])
                if aligned is None:return
                data=bytes(state['header'][aligned:]);state['header'].clear();state['ready']=True;self.counts['resynchronizedStreams']+=1
            else:
                if len(state['header'])<7:return
                header=bytes(state['header'][:7])
                if header[:3] not in (bytes.fromhex('060104'),bytes.fromhex('060100')):state['blocked']=True;raise DecodeError('Unknown channel header')
                state['control']=header[3:]==b'ctrl';data=bytes(state['header'][7:]);state['header'].clear();state['ready']=True
        if state['control']:return
        for offset in range(0,len(data),65536):
            for flag,body in state['records'].feed(data[offset:offset+65536]):self.application(row['connection'],row['direction'],body,row['time'])
    def application(self,connection,direction,body,stamp):
        self.counts['applicationRecords']+=1
        if not body:return
        if body[0]==0x97:
            try:
                self.schemas[connection]=Schema.from_packet(body[1:]);self.schema_packets[connection]=body[1:];self.counts['validatedSchemas']+=1
                waiting=self.pending.pop(connection,[])
                for d,b,t in waiting:self.replication(connection,d,b,t)
            except (DecodeError,ValueError,UnicodeError,zstandard.ZstdError) as exc:self.errors['Schema: '+str(exc)]+=1
        elif body[0] in (0x83,0xb7,0xe8):
            if connection in self.schemas:self.replication(connection,direction,body,stamp)
            elif len(self.pending[connection])<4096:self.pending[connection].append((direction,body,stamp))
    def replication(self,connection,direction,body,stamp):
        mode={0x83:'3',0xb7:'1',0xe8:'2'}[body[0]];key=(connection,direction,mode)
        wire=self.wires.setdefault(key,Wire(self.schemas[connection],MAPS[mode]))
        try:
            for call in wire.events(b'\x83'+body[1:],direction):
                if call['method'] not in SUPPORTED_METHODS:continue
                call['timestamp']=time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime(stamp));call['captureTime']=stamp;call['recordBoundaryValidated']=call['consumed']==len(body) or not any(body[call['consumed']:])
                self.calls=collections.deque(sorted([*self.calls,call],key=lambda c:c['captureTime'])[-500:],maxlen=500);self.counts['decodedCalls']+=1
                if call['method']=='FireServer' and call['args'] and call['args'][0]=='Clicked':self.counts['clickedCalls']+=1;self.highlights.append(call)
        except (DecodeError,UnicodeError) as exc:self.errors[str(exc)]+=1
    def state(self,capture='Running'):
        schema=bool(self.schemas)
        decoder='Decoding captured calls' if schema else ('Authenticated transport; rejoin to collect the schema' if self.counts['authenticatedPackets'] else 'Waiting for supported transport keys')
        recent=[c for c in self.calls if c not in self.highlights];calls=sorted([*self.highlights,*recent[-(500-len(self.highlights)):]],key=lambda c:c.get('captureTime',0))
        return dict(capture=capture,decoder=decoder,status='Limited decoder: some call formats are skipped and instance paths are unresolved.',calls=calls,stats=dict(self.counts),errors=dict(self.errors),coverage='events-with-validated-acknowledgements',keysWritten=False)
