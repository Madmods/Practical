"""Explicit live-test runner. Saves evidence counts only; never writes session keys."""
import ctypes,json,pathlib,struct,sys,time,ipaddress,collections,threading
from network_transport_lab import *

BASE=pathlib.Path(__file__).parent
class Stats(ctypes.Structure):
    _fields_=[('bytesRead',ctypes.c_uint64),('regions',ctypes.c_uint32),('error',ctypes.c_uint32),('milliseconds',ctypes.c_uint32),('limitReached',ctypes.c_uint32),('nextAddress',ctypes.c_uint64)]

def candidates(pid,max_ms=10000,start_address=0,stop_path=None):
    dll=ctypes.CDLL(str(BASE/('scan_aes_schedules_v2.dll' if (BASE/'scan_aes_schedules_v2.dll').exists() else 'scan_aes_schedules.dll')))
    dll.ScanProcess.argtypes=[ctypes.c_uint32,ctypes.c_void_p,ctypes.c_uint,ctypes.c_uint64,ctypes.c_uint,ctypes.POINTER(ctypes.c_long),ctypes.POINTER(Stats)]
    dll.ScanProcess.restype=ctypes.c_uint
    data=ctypes.create_string_buffer(32*512);cancel=ctypes.c_long(0);stats=Stats();stats.nextAddress=start_address
    finished=threading.Event();watcher=None
    if stop_path:
        def check_stop():
            while not finished.wait(.1):
                if pathlib.Path(stop_path).exists():cancel.value=1;return
        watcher=threading.Thread(target=check_stop);watcher.start()
    try:count=dll.ScanProcess(pid,data,512,1024*1024*1024,max_ms,ctypes.byref(cancel),ctypes.byref(stats))
    finally:
        finished.set()
        if watcher:watcher.join()
    keys=[bytes(data.raw[i*32:i*32+32]) for i in range(count)];ctypes.memset(data,0,len(data))
    return keys,{name:getattr(stats,name) for name,_ in Stats._fields_}

def packets(filename):
    b=filename.read_bytes();p=0;endian='<';links=[];frame=0;out=[]
    while p+12<=len(b):
        if b[p:p+4]==b'\x0a\x0d\x0d\x0a':endian='<' if b[p+8:p+12]==b'\x4d\x3c\x2b\x1a' else '>';links=[]
        u32=lambda off:struct.unpack_from(endian+'I',b,off)[0]
        kind,size=u32(p),u32(p+4)
        if size<12 or p+size>len(b) or u32(p+size-4)!=size:raise DecodeError('Invalid PCAPNG block')
        if kind==1:links.append(struct.unpack_from(endian+'H',b,p+8)[0])
        if kind==6:
            frame+=1;iface,hi,lo,n,wire=struct.unpack_from(endian+'IIIII',b,p+8);raw=b[p+28:p+28+n]
            if iface<len(links) and links[iface]==1 and len(raw)>=42:
                etype=int.from_bytes(raw[12:14],'big');i=14
                while etype in (0x8100,0x88a8) and i+4<=len(raw):etype=int.from_bytes(raw[i+2:i+4],'big');i+=4
                if etype==0x0800 and i+20<=len(raw) and raw[i+9]==17:
                    total=int.from_bytes(raw[i+2:i+4],'big');ihl=(raw[i]&15)*4;u=i+ihl
                    if ihl>=20 and total>=ihl+8 and i+total<=len(raw) and not int.from_bytes(raw[i+6:i+8],'big')&0x3fff:
                        src,dst=struct.unpack_from('>HH',raw,u);length=int.from_bytes(raw[u+4:u+6],'big')
                        if 8<=length<=total-ihl:
                            out.append(dict(frame=frame,time=(hi*4294967296+lo)*1e-6,source=str(ipaddress.ip_address(raw[i+12:i+16])),destination=str(ipaddress.ip_address(raw[i+16:i+20])),sourcePort=src,destinationPort=dst,payload=raw[u+8:u+length]))
        p+=size
    return out

def discover(sample,keys,wrapper):
    if len(sample)<2:return None
    # Each accepted candidate must authenticate TWO different captured datagrams.
    for hp in keys:
        try:first=unprotect(sample[0]['payload'],hp,wrapper)
        except DecodeError:continue
        for key in keys:
            recovered=recover_nonce(first,key)
            if recovered is None:continue
            nonce,plain=recovered;iv=nonce_for(nonce,first.packet_number)
            for other in sample[1:12]:
                try:
                    u,p=decrypt(other['payload'],hp,key,iv,wrapper,first.packet_number)
                    if u.cid!=first.cid or u.key_phase!=first.key_phase or u.packet_number==first.packet_number:continue
                    # The context is now independently authenticated; malformed frames are still rejected separately.
                    return dict(hp=hp,key=key,iv=iv,cid=first.cid,phase=first.key_phase,largest=first.packet_number)
                except DecodeError:continue
    return None

def main():
    if len(sys.argv)!=3 or sys.argv[1] not in ('--explicit-live-test','--analyze-existing'):raise SystemExit('Requires an explicit test mode and CAPTURE_DIRECTORY')
    folder=pathlib.Path(sys.argv[2]);state=folder/'capture-state.json';deadline=time.monotonic()+20
    while sys.argv[1]=='--explicit-live-test' and time.monotonic()<deadline:
        try:s=json.loads(state.read_text(encoding='utf-8-sig'))
        except (OSError,ValueError):time.sleep(.1);continue
        if s['status']=='capturing':break
        if s['status']=='failed':raise SystemExit(s['message'])
        time.sleep(.1)
    else:
        if sys.argv[1]=='--explicit-live-test':raise SystemExit('Capture did not become active; no process read performed')
    info=json.loads((folder/'capture-info.json').read_text(encoding='utf-8-sig'))
    if len(info['processes'])!=1:raise SystemExit('Expected exactly one captured Roblox process')
    pid=info['processes'][0]['Id'];keys,stats=candidates(pid,10000)
    deadline=time.monotonic()+30
    while time.monotonic()<deadline:
        s=json.loads(state.read_text(encoding='utf-8-sig'))
        if s['status']=='complete':break
        if s['status']=='failed':raise SystemExit(s['message'])
        time.sleep(.1)
    else:raise SystemExit('Capture did not finish')
    ports={v['LocalPort'] for v in info['endpoints']};data=packets(folder/'game-packets.pcapng');flows={}
    for p in data:
        outgoing=p['sourcePort'] in ports
        flow=(p['source'],p['sourcePort'],p['destination'],p['destinationPort'],outgoing)
        flows.setdefault(flow,[]).append(p)
    report=dict(stage='authenticated transport validation',scan=stats,keyCandidateCount=len(keys),packetCount=len(data),contexts=[],remoteCallsDecoded=0,keysWritten=False)
    traces=[]
    for flow,sample in sorted(flows.items(),key=lambda kv:-len(kv[1]))[:4]:
        outgoing=flow[-1];wrapper=31 if outgoing else 23
        usable=[p for p in sample if len(p['payload'])>=wrapper+41 and p['payload'][wrapper]&0xc0==0x40]
        context=discover(usable,keys,wrapper)
        entry=dict(direction='outgoing' if outgoing else 'incoming',flow=list(flow[:-1]),packets=len(sample),profileCompatiblePackets=len(usable),authenticated=False)
        if context:
            entry['authenticated']=True;entry['authenticatedPackets']=0;entry['frameParseFailures']=0;entry['streams']=[];streams={};lengths={};reasons=collections.Counter();prefixes=[]
            for p in usable:
                try:
                    u,plain=decrypt(p['payload'],context['hp'],context['key'],context['iv'],wrapper,context['largest'])
                    if u.cid!=context['cid'] or u.key_phase!=context['phase']:continue
                    context['largest']=max(context['largest'],u.packet_number);entry['authenticatedPackets']+=1
                    traces.append(dict(frame=p['frame'],direction=entry['direction'],packetNumber=u.packet_number,plaintextHex=plain.hex()))
                    for f in frames(plain):
                        out=streams.setdefault(f.stream,Stream()).add(f.offset,f.data,f.fin);lengths[f.stream]=lengths.get(f.stream,0)+len(out)
                except DecodeError as exc:
                    entry['frameParseFailures']+=1;reasons[str(exc)]+=1
                    if len(prefixes)<8:prefixes.append(dict(frame=p['frame'],reason=str(exc),prefixHex=plain[:48].hex()))
            entry['streams']=[dict(id=i,contiguousBytes=n) for i,n in lengths.items()]
            entry['frameParseReasons']=dict(reasons);entry['structuralSamples']=prefixes
        report['contexts'].append(entry)
    # No plaintext or key material is included in the evidence file.
    keys.clear();name='transport-validation.json' if sys.argv[1]=='--explicit-live-test' else 'transport-trace-validation.json'
    (folder/name).write_text(json.dumps(report,indent=2),encoding='utf-8')
    if traces:(BASE/'authenticated-packet-trace.json').write_text(json.dumps(traces),encoding='utf-8')
    print(json.dumps(report,indent=2))

if __name__=='__main__':main()

