"""Decode an explicit capture with in-memory key candidates. Does not start capture."""
import collections,json,pathlib,struct,time
from network_transport_lab import *
from network_wire_lab import Schema,Wire
from validate_live_transport import discover
BASE=pathlib.Path(__file__).parent
MAPS=json.loads((BASE/'replication-type-maps.json').read_text())

def read_pcap(path):
    data=pathlib.Path(path).read_bytes()
    if len(data)>128*1024*1024 or len(data)<24:raise DecodeError('Capture bounds')
    if data[:4]!=bytes.fromhex('d4c3b2a1') or struct.unpack_from('<I',data,20)[0]!=101:raise DecodeError('Expected raw-IP little-endian PCAP')
    p=24;rows=[];i=0
    while p+16<=len(data):
        sec,micro,n,wire=struct.unpack_from('<IIII',data,p);p+=16
        if n>65535 or p+n>len(data):break
        raw=data[p:p+n];p+=n;i+=1
        if len(raw)<28 or raw[0]>>4!=4 or raw[9]!=17:continue
        ihl=(raw[0]&15)*4;total=int.from_bytes(raw[2:4],'big')
        if ihl<20 or total>len(raw) or total<ihl+8 or int.from_bytes(raw[6:8],'big')&0x3fff:continue
        sp,dp,length=struct.unpack_from('>HHH',raw,ihl)
        if length<8 or length>total-ihl:continue
        payload=raw[ihl+8:ihl+length]
        if len(payload)<7 or payload[:3]!=b'\x01\x00\x00' or payload[4:7]!=b'\x01\x11\x02' or payload[3] not in (23,31):continue
        source='.'.join(map(str,raw[12:16]));destination='.'.join(map(str,raw[16:20]));outgoing=payload[3]==31;wrapper=payload[3]
        if len(payload)<wrapper+41 or payload[wrapper]&0xc0!=0x40:continue
        local=(source,sp) if outgoing else (destination,dp);server=(destination,dp) if outgoing else (source,sp)
        rows.append(dict(frame=i,time=sec+micro/1e6,source=source,sourcePort=sp,destination=destination,destinationPort=dp,payload=payload,direction='outgoing' if outgoing else 'incoming',connection=(local,server),cid=payload[wrapper+1:wrapper+21]))
    return rows

def channel_records(data,origin):
    candidates=[0]
    if origin==0 and (data.startswith(bytes.fromhex('060104')) or data.startswith(bytes.fromhex('0601006374726c'))):candidates=[7]
    accepted=[]
    for skip in candidates:
        try:
            reader=Records();records=[]
            for start in range(skip,len(data),65536):records.extend(reader.feed(data[start:start+65536]))
            if (records or skip==len(data)==7) and all(body for flag,body in records):accepted.append((skip,records,len(reader.buffer)))
        except DecodeError:pass
    if len(accepted)!=1:raise DecodeError('Record boundary/channel header not validated')
    return accepted[0]

def decode_capture(path,keys):
    rows=read_pcap(path);groups={}
    for row in rows:groups.setdefault((row['connection'],row['direction'],row['cid']),[]).append(row)
    contexts=[];stream_groups={};stats=collections.Counter();failures=collections.Counter()
    for (connection,direction,cid),sample in sorted(groups.items(),key=lambda kv:-len(kv[1]))[:16]:
        wrapper=31 if direction=='outgoing' else 23;context=discover(sample,keys,wrapper)
        status=dict(connection=connection,direction=direction,packets=len(sample),authenticated=bool(context));contexts.append(status)
        if not context:continue
        status['authenticatedPackets']=0
        for row in sample:
            try:
                try:u,plain=decrypt(row['payload'],context['hp'],context['key'],context['iv'],wrapper,context['largest'])
                except DecodeError:
                    u=unprotect(row['payload'],context['hp'],wrapper,context['largest'])
                    recovered=recover_nonce(u,context['key'])
                    if recovered is None:raise DecodeError('Packet authentication failed')
                    nonce,plain=recovered;stats['nonceRecoveredPackets']+=1
                if u.cid!=context['cid'] or u.key_phase!=context['phase']:continue
                context['largest']=max(context['largest'],u.packet_number);status['authenticatedPackets']+=1;stats['authenticatedPackets']+=1
                for frame in frames(plain):stream_groups.setdefault((connection,direction,frame.stream),[]).append((frame,row))
            except DecodeError as exc:failures[str(exc)]+=1
    applications=[];streams=[];incomplete_schemas=[]
    for (connection,direction,stream_id),segments in stream_groups.items():
        segments.sort(key=lambda pair:pair[0].offset);origin=segments[0][0].offset;window=Stream(limit=8*1024*1024,origin=origin);data=bytearray()
        try:
            for frame,row in segments:data.extend(window.add(frame.offset,frame.data,frame.fin))
            skip,records,partial=channel_records(data,origin);position=skip+4
            for flag,body in records:
                contributors=[row for frame,row in segments if frame.offset<origin+position+len(body) and frame.offset+len(frame.data)>origin+position-4]
                stamp=max((r['time'] for r in contributors),default=segments[0][1]['time'])
                applications.append(dict(connection=connection,direction=direction,body=body,time=stamp,frames=sorted({r['frame'] for r in contributors})))
                position+=4+len(body)
            if partial>=5:
                tail=bytes(data[position-4:])
                if tail[4]==0x97:incomplete_schemas.append(dict(direction=direction,stream=stream_id,expectedRecordBytes=int.from_bytes(tail[:4],'big')&0x7fffffff,availableRecordBytes=len(tail)-4))
            streams.append(dict(connection=connection,direction=direction,id=stream_id,origin=origin,bytes=len(data),records=len(records),partialRecordBytes=partial))
        except DecodeError as exc:
            failures[str(exc)]+=1;streams.append(dict(connection=connection,direction=direction,id=stream_id,origin=origin,bytes=len(data),error=str(exc),prefixHex=bytes(data[:32]).hex()))
    schemas={};decoders={};calls=[];kinds=collections.Counter();schema_errors=[]
    for app in sorted(applications,key=lambda a:a['time']):
        body=app['body'];connection=app['connection'];direction=app['direction'];kinds[hex(body[0])]+=1
        if body[0]==0x97:
            try:
                schema=Schema.from_packet(body[1:]);schemas[connection]=schema
                for decoder_key in [k for k in decoders if k[0]==connection]:del decoders[decoder_key]
            except Exception as exc:schema_errors.append(dict(frames=app['frames'],error=str(exc)))
        mode={0x83:'3',0xb7:'1',0xe8:'2'}.get(body[0])
        if mode and connection in schemas:
            decoder_key=(connection,direction,mode);wire=decoders.setdefault(decoder_key,Wire(schemas[connection],MAPS[mode]))
            normalized=b'\x83'+body[1:]
            try:
                call=wire.leading_event(normalized,direction)
                if call:
                    call['timestamp']=time.strftime('%Y-%m-%dT%H:%M:%SZ',time.gmtime(app['time']));call['packetFrames']=app['frames'];call['recordBoundaryValidated']=call['consumed']==len(body) or not any(body[call['consumed']:]);calls.append(call)
            except DecodeError as exc:failures[str(exc)]+=1
    report=dict(packetRows=len(rows),stats=dict(stats),contexts=contexts,streams=streams,applicationRecords=len(applications),applicationTypes=dict(kinds),schemas=[dict(connection=c,classes=len(s.classes),properties=len(s.properties),events=len(s.events),remoteMembers=[e['name'] for e in s.events if e['className'] in ('RemoteEvent','RemoteFunction','BaseRemoteEvent','UnreliableRemoteEvent')]) for c,s in schemas.items()],incompleteSchemas=incomplete_schemas,schemaErrors=schema_errors,errors=dict(failures),decodedCalls=len(calls),keysWritten=False)
    folder=pathlib.Path(path).parent
    (folder/'session-decoder-report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
    (folder/'decoded-calls.json').write_text(json.dumps({'calls':calls},indent=2),encoding='utf-8')
    # Authenticated record bodies stay in the private workspace for protocol debugging.
    (BASE/'join-authenticated-records.json').write_text(json.dumps([{**{k:v for k,v in a.items() if k!='body'},'bodyHex':a['body'].hex()} for a in applications]),encoding='utf-8')
    return report
