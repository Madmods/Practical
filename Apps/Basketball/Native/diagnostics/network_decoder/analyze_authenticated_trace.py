import collections,json,pathlib
from network_transport_lab import *
BASE=pathlib.Path(__file__).parent
trace=json.loads((BASE/'authenticated-packet-trace.json').read_text())
mapping=json.loads((BASE/'replication-type-maps.json').read_text())['3']
groups={};errors=collections.Counter()
for row in trace:
    try:
        for frame in frames(bytes.fromhex(row['plaintextHex'])):groups.setdefault((row['direction'],frame.stream),[]).append((frame,row['frame']))
    except DecodeError as exc:errors[str(exc)]+=1
def leb(data,p):
    value=0
    for i in range(5):
        if p>=len(data):raise DecodeError('Truncated reference')
        b=data[p];p+=1;value|=(b&127)<<(i*7)
        if not b&128:return value,p
    raise DecodeError('Reference exceeds 32 bits')
report={'authenticatedPacketCount':len(trace),'frameParseErrors':dict(errors),'streams':[],'eventHeaders':[],'remoteCallsDecoded':0,'requiresJoinSchema':True}
records_out=[]
for (direction,stream_id),segments in groups.items():
    segments.sort(key=lambda x:x[0].offset);origin=segments[0][0].offset;window=Stream(origin=origin);stream_bytes=bytearray();provenance=[]
    for frame,packet_frame in segments:
        stream_bytes.extend(window.add(frame.offset,frame.data,frame.fin));provenance.append((frame.offset,frame.offset+len(frame.data),packet_frame))
    parser=Records();records=parser.feed(stream_bytes);types=collections.Counter();opcodes=collections.Counter();pos=4
    for flag,body in records:
        types[f'0x{body[0]:02x}']+=1
        frame_numbers=sorted({p for a,b,p in provenance if a<origin+pos+len(body) and b>origin+pos-4})
        records_out.append(dict(direction=direction,stream=stream_id,offset=origin+pos,sourceFrames=frame_numbers,bodyHex=body.hex(),recordFlag=flag))
        if body[0]==0x83 and len(body)>1:
            opcode=mapping[body[1]];opcodes[str(opcode)]+=1
            if opcode==7:
                peer,p=leb(body,2)
                instance=None
                if peer:
                    if p+4>len(body):raise DecodeError('Truncated instance reference')
                    instance=int.from_bytes(body[p:p+4],'little');p+=4
                if p+2>len(body):raise DecodeError('Truncated event identifier')
                event_id=int.from_bytes(body[p:p+2],'little');p+=2
                report['eventHeaders'].append(dict(direction=direction,peer=peer,instance=instance,eventId=event_id,argumentBytes=len(body)-p,sourceFrames=frame_numbers,status='Member schema and complete invocation boundary not validated'))
        pos+=4+len(body)
    report['streams'].append(dict(direction=direction,id=stream_id,observedOrigin=origin,originIsJoin=origin==0,contiguousBytes=len(stream_bytes),records=len(records),partialRecordBytes=len(parser.buffer),applicationTypes=dict(types),leadingReplicationOpcodes=dict(opcodes)))
(BASE/'authenticated-records.json').write_text(json.dumps(records_out),encoding='utf-8')
out=BASE.parent/'outputs'/'decoder-live-test'/'record-validation.json';out.write_text(json.dumps(report,indent=2),encoding='utf-8');print(json.dumps(report,indent=2))
