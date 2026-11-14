"""Length-preserving edits of authenticated, schema-validated FF3 throw records.

No driver, process access or networking in this module. All other bytes remain
unchanged. Reliable stream edits are pinned for retransmissions, even when the
current aim plan is inactive. Fragmented fields are left to the game's aim.
"""
import math,struct,collections
from cryptography.hazmat.primitives.ciphers.aead import AESGCM
from network_transport_lab import DecodeError,Stream,frames,unprotect,recover_nonce,ecb
from network_wire_lab import Reader,Wire
from live_network_decoder import record_alignment,MAPS

def vector(value):
    if not isinstance(value,(list,tuple)) or len(value)!=3 or any(isinstance(x,bool) or not isinstance(x,(int,float)) or not math.isfinite(x) or abs(x)>1e7 for x in value):raise DecodeError('Invalid plan vector')
    return tuple(float(x) for x in value)

def length(value):return math.sqrt(sum(x*x for x in value))

def velocity(origin,target,movement,power,gravity,latency,lob=False):
    origin,target,movement=map(vector,(origin,target,movement))
    if not 1<=power<=95 or not 1<=gravity<=300 or not 0<=latency<=.25 or length(movement)>40:raise DecodeError('Ballistic input bounds')
    distance=length(tuple(b-a for a,b in zip(origin,target)))
    if not 3<=distance<=600:raise DecodeError('Receiver distance bounds')
    def at(t):return tuple((target[i]+movement[i]*(t+latency)-origin[i]+(.5*gravity*t*t if i==1 else 0))/t for i in range(3))
    def error(t):return sum(v*v for v in at(t))-power*power
    roots=[];previous=.03;prior=error(previous)
    for i in range(1,801):
        next_time=.03+i*.01;current=error(next_time)
        if (prior>0 and current<=0) or (prior<0 and current>=0):
            low,high=previous,next_time;positive=prior>0
            for _ in range(32):
                mid=(low+high)*.5
                if (error(mid)>0)==positive:low=mid
                else:high=mid
            roots.append((low+high)*.5)
        previous,prior=next_time,current
    if not roots:raise DecodeError('Receiver outside throw range')
    flight=roots[-1] if lob else roots[0];result=at(flight)
    if not .06<=flight<=8 or abs(length(result)-power)>.01:raise DecodeError('Invalid ballistic solution')
    return result,flight

def angle_velocity(origin,target,movement,angle,gravity,latency):
    origin,target,movement=map(vector,(origin,target,movement))
    if not all(math.isfinite(x) for x in (angle,gravity,latency)) or not 5<=angle<=75 or not 1<=gravity<=300 or not 0<=latency<=.25 or length(movement)>40:raise DecodeError('Angle plan bounds')
    if not 3<=length(tuple(b-a for a,b in zip(origin,target)))<=600:raise DecodeError('Receiver distance bounds')
    tangent=math.tan(math.radians(angle))
    def error(t):
        d=[target[i]+movement[i]*(t+latency)-origin[i] for i in range(3)]
        return math.hypot(d[0],d[2])*tangent-d[1]-.5*gravity*t*t
    previous=.06;prior=error(previous)
    for i in range(1,401):
        next_time=.06+i*.02;current=error(next_time)
        if prior>=0 and current<=0:
            low,high=previous,next_time
            for _ in range(32):
                mid=(low+high)*.5
                if error(mid)>0:low=mid
                else:high=mid
            t=(low+high)*.5
            launch=tuple((target[j]+movement[j]*(t+latency)-origin[j]+(.5*gravity*t*t if j==1 else 0))/t for j in range(3))
            power=length(launch)
            if t<=8 and 1<=power<=95:return launch,t,power
        previous,prior=next_time,current
    raise DecodeError('Selected angle exceeds available power')

def throw_fields(body,call):
    if call['method']!='FireServer' or len(call['args'])!=4 or call['args'][0]!='Clicked':return None
    r=Reader(body);r.pos=call['recordOffset']+1;r.ref();r.integer(2)
    if r.uint()!=4 or r.byte()!=2 or r.string()!='Clicked':return None
    if r.byte()!=22:return None
    origin=struct.unpack('<fff',r.take(12))
    if r.byte()!=22:return None
    aim_offset=r.pos;r.take(12)
    if r.byte()!=12:return None
    power_offset=r.pos;power=struct.unpack('<d',r.take(8))[0]
    if r.pos!=call['consumed'] or not math.isfinite(power) or not 1<=power<=95:return None
    return origin,aim_offset,power_offset,power

def replacement(body,call,plan,tick):
    fields=throw_fields(body,call)
    if not fields:return None
    if plan.get('active') is not True or plan.get('mode')!='ff3-qb':return None
    issued,expires,clicked=plan.get('issued',0),plan.get('expires',0),plan.get('clickAt',0)
    if not 0<=tick-issued<=150 or not issued<=tick<=expires<=issued+200 or not 0<=tick-clicked<=350 or clicked<=0:return None
    origin,aim_offset,power_offset,old_power=fields
    if length(tuple(a-b for a,b in zip(vector(origin),vector(plan['head']))))>12:raise DecodeError('Throw origin disagrees with held character')
    power=95.0 if plan.get('autoPower') is True else old_power
    if plan.get('anglePower') is True and plan.get('autoPower') is True:
        launch,flight,power=angle_velocity(origin,plan['targetPosition'],plan['targetVelocity'],float(plan['angle']),float(plan['gravity']),float(plan['latency']))
    else:
        launch,flight=velocity(origin,plan['targetPosition'],plan['targetVelocity'],power,float(plan['gravity']),float(plan['latency']),bool(plan.get('lob')))
    aim=tuple(origin[i]+launch[i]*(10000/power) for i in range(3))
    return [(aim_offset,struct.pack('<fff',*aim)),(power_offset,struct.pack('<d',power))],dict(oldPower=old_power,newPower=power,origin=origin,aim=aim,flight=flight,instance=call['instance'],clickAt=clicked)

class RecordTracker:
    def __init__(self,schema):self.schema=schema;self.streams={}
    def add(self,frame):
        if frame.stream not in self.streams:
            if len(self.streams)>=32:raise DecodeError('QB stream count budget')
            self.streams[frame.stream]={'stream':Stream(8*1024*1024,frame.offset),'buffer':bytearray(),'start':frame.offset,'ready':False,'blocked':False}
        state=self.streams[frame.stream]
        if state['blocked']:return []
        origin=state['stream'].origin
        if frame.offset<origin:
            skip=origin-frame.offset
            if skip>=len(frame.data):return []
            data=state['stream'].add(origin,frame.data[skip:],frame.fin)
        else:data=state['stream'].add(frame.offset,frame.data,frame.fin)
        state['buffer'].extend(data);buffer=state['buffer']
        if not state['ready']:
            if origin==0:
                if len(buffer)<7:return []
                if buffer[:3]!=bytes.fromhex('060104') or buffer[3:7]==b'ctrl':state['blocked']=True;return []
                del buffer[:7];state['start']+=7;state['ready']=True
            else:
                aligned=record_alignment(buffer,self.schema,MAPS['3'],'outgoing')
                if aligned is None:
                    if len(buffer)>65536:state['blocked']=True
                    return []
                del buffer[:aligned];state['start']+=aligned;state['ready']=True
        records=[]
        while len(buffer)>=4:
            size=int.from_bytes(buffer[:4],'big')&0x7fffffff
            if not 1<=size<=1024*1024:state['blocked']=True;raise DecodeError('QB record size bound')
            if len(buffer)<4+size:break
            records.append((frame.stream,state['start']+4,bytes(buffer[4:4+size])))
            del buffer[:4+size];state['start']+=4+size
        window=state['stream'];consumed=state['start']-window.origin
        if consumed>=65536:
            window.buffer=window.buffer[consumed:];window.present=window.present[consumed:];window.contiguous-=consumed;window.delivered-=consumed;window.origin+=consumed
            if window.final is not None:window.final-=consumed
        return records

class Editor:
    def __init__(self,schema):
        self.schema=schema;self.wire=Wire(schema,MAPS['3']);self.tracker=RecordTracker(schema);self.ledger=[];self.claimed=set();self.stats=collections.Counter();self.last=None;self.covered=set()
    def process_plain(self,plain,plan,tick,allow=True):
        spans=frames(plain);candidate=bytearray(plain);records=[]
        for frame in spans:
            try:records.extend(self.tracker.add(frame))
            except DecodeError:self.stats['trackingFailures']+=1
        if allow:
            for stream,start,body in records:
                if body[:1]!=b'\x83':continue
                try:
                    for call in self.wire.events(body,'outgoing'):
                        proposal=replacement(body,call,plan,tick)
                        if not proposal:continue
                        edits,detail=proposal
                        if detail['clickAt'] in self.claimed or len(self.ledger)+len(edits)>2048:continue
                        planned=[]
                        for offset,value in edits:
                            absolute=start+offset;positions={}
                            for frame in spans:
                                if frame.stream!=stream:continue
                                for at in range(max(absolute,frame.offset),min(absolute+len(value),frame.offset+len(frame.data))):positions.setdefault(at,[]).append(frame.plain_offset+at-frame.offset)
                            if any(at not in positions for at in range(absolute,absolute+len(value))):break
                            planned.append((stream,absolute,value,detail['clickAt']))
                        else:
                            self.ledger.extend(planned);self.claimed.add(detail['clickAt']);self.last=detail;self.stats['modifiedThrows']+=1
                except (DecodeError,UnicodeError,KeyError,TypeError,ValueError):self.stats['declinedRecords']+=1
        touched=False;coverage=collections.defaultdict(list);self.covered=set()
        for stream,start,value,clicked in self.ledger:
            present=set()
            for frame in spans:
                if frame.stream!=stream:continue
                lo=max(start,frame.offset);hi=min(start+len(value),frame.offset+len(frame.data))
                if lo<hi:
                    at=frame.plain_offset+lo-frame.offset;candidate[at:at+hi-lo]=value[lo-start:hi-start];touched=True
                    present.update(range(lo,hi))
            coverage[clicked].append(len(present)==len(value))
        self.covered={clicked for clicked,parts in coverage.items() if all(parts)}
        if touched:self.stats['rewrittenPackets']+=1
        return bytes(candidate)

def authenticated_plain(udp,context):
    u=unprotect(udp,context['hp'],31,context['largest'])
    if u.key_phase!=context['phase']:raise DecodeError('QB connection changed')
    recovered=recover_nonce(u,context['key'])
    if recovered is None:raise DecodeError('QB packet authentication failed')
    context['cid']=u.cid
    context['largest']=max(context['largest'],u.packet_number)
    return u,*recovered

def encrypt_changed(udp,u,nonce,plain,context):
    encrypted=AESGCM(context['key']).encrypt(nonce,plain,u.header);packet=bytearray(u.header+encrypted)
    if len(packet)!=len(udp)-31:raise DecodeError('QB packet length changed')
    mask=ecb(context['hp'],bytes(packet[25:41]));packet[0]^=mask[0]&31
    for index in range(len(u.header)-21):packet[21+index]^=mask[index+1]
    return udp[:31]+packet
