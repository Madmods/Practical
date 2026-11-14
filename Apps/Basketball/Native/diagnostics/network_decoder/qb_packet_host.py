"""Explicit QB mode: warm up read-only, then edit only the owned game flow.

The normal game generates the throw. No event is synthesized or replayed.
Original datagrams are forwarded on every processing error. Already accepted
stream edits remain pinned during disable/drain so retransmissions stay stable.
"""
import os,sys,ctypes,json,pathlib,struct,socket,threading,time,collections
from network_transport_lab import DecodeError,frames,unprotect,recover_nonce
from live_network_decoder import LiveDecoder
from qb_packet_editor import Editor,authenticated_plain,encrypt_changed
from validate_live_transport import candidates
import network_schema_cache

GAME_FILTER='ip and udp and !loopback and (udp.Payload32[0] == 0x01000017 or udp.Payload32[0] == 0x0100001f) and udp.Payload16[4b] == 0x0111 and udp.Payload[6] == 2'
DEPENDENCY=os.environ.get('VANTA_WINDIVERT_DLL') or str((pathlib.Path(sys.executable).parent if getattr(sys,'frozen',False) else pathlib.Path(__file__).resolve().parents[2]/'third_party'/'windivert'/'bin')/'WinDivert.dll')

class Driver:
    def __init__(self):
        self.dll=ctypes.WinDLL(DEPENDENCY,use_last_error=True);d=self.dll
        d.WinDivertOpen.argtypes=[ctypes.c_char_p,ctypes.c_int,ctypes.c_short,ctypes.c_ulonglong];d.WinDivertOpen.restype=ctypes.c_void_p
        d.WinDivertRecv.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_uint,ctypes.POINTER(ctypes.c_uint),ctypes.c_void_p];d.WinDivertRecv.restype=ctypes.c_int
        d.WinDivertSend.argtypes=d.WinDivertRecv.argtypes;d.WinDivertSend.restype=ctypes.c_int
        d.WinDivertShutdown.argtypes=[ctypes.c_void_p,ctypes.c_int];d.WinDivertClose.argtypes=[ctypes.c_void_p]
        d.WinDivertHelperCalcChecksums.argtypes=[ctypes.c_void_p,ctypes.c_uint,ctypes.c_void_p,ctypes.c_ulonglong];d.WinDivertHelperCalcChecksums.restype=ctypes.c_int
        d.WinDivertHelperCompileFilter.argtypes=[ctypes.c_char_p,ctypes.c_int,ctypes.c_void_p,ctypes.c_uint,ctypes.POINTER(ctypes.c_char_p),ctypes.POINTER(ctypes.c_uint)];d.WinDivertHelperCompileFilter.restype=ctypes.c_int
    def validate(self,filter):
        error=ctypes.c_char_p();position=ctypes.c_uint()
        return bool(self.dll.WinDivertHelperCompileFilter(filter.encode('ascii'),0,None,0,ctypes.byref(error),ctypes.byref(position)))
    def open(self,filter,sniff=False):
        handle=self.dll.WinDivertOpen(filter.encode('ascii'),0,1000,5 if sniff else 0)
        if handle in (None,ctypes.c_void_p(-1).value):raise OSError(ctypes.get_last_error(),'Cannot open QB transport')
        return handle
    def recv(self,handle):
        packet=ctypes.create_string_buffer(65535);address=ctypes.create_string_buffer(80);size=ctypes.c_uint()
        if not self.dll.WinDivertRecv(handle,packet,len(packet),ctypes.byref(size),address):return None
        return packet.raw[:size.value],address
    def send(self,handle,raw,address,changed=False):
        packet=ctypes.create_string_buffer(raw,len(raw));sent=ctypes.c_uint()
        if changed and not self.dll.WinDivertHelperCalcChecksums(packet,len(raw),address,0):raise OSError('QB packet checksum failed')
        if not self.dll.WinDivertSend(handle,packet,len(raw),ctypes.byref(sent),address) or sent.value!=len(raw):raise OSError(ctypes.get_last_error(),'QB forwarding failed')
    def shutdown(self,h):self.dll.WinDivertShutdown(h,1)
    def close(self,h):self.dll.WinDivertClose(h)

def ports(pid):
    api=ctypes.WinDLL('iphlpapi');f=api.GetExtendedUdpTable;f.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_ulong),ctypes.c_int,ctypes.c_ulong,ctypes.c_int,ctypes.c_ulong]
    size=ctypes.c_ulong();f(None,ctypes.byref(size),False,2,1,0)
    if not 4<=size.value<=1024*1024:return set()
    data=ctypes.create_string_buffer(size.value)
    if f(data,ctypes.byref(size),False,2,1,0):return set()
    count=struct.unpack_from('<I',data.raw)[0]
    if 4+count*12>size.value:return set()
    return {socket.ntohs(port&65535) for address,port,owner in struct.iter_unpack('<III',data.raw[4:4+count*12]) if owner==pid}

def row(raw,index):
    if len(raw)<28 or raw[0]>>4!=4 or raw[9]!=17:return None
    ihl=(raw[0]&15)*4;total=int.from_bytes(raw[2:4],'big')
    if ihl<20 or total>len(raw) or total<ihl+8 or int.from_bytes(raw[6:8],'big')&0x3fff:return None
    source_port,destination_port,size=struct.unpack_from('>HHH',raw,ihl)
    if size<8 or size>total-ihl:return None
    payload=raw[ihl+8:ihl+size]
    if len(payload)<7 or payload[:3]!=b'\x01\0\0' or payload[4:7]!=b'\x01\x11\x02' or payload[3] not in (23,31):return None
    wrapper=payload[3]
    if len(payload)<wrapper+41 or payload[wrapper]&0xc0!=0x40:return None
    outgoing=wrapper==31;source=socket.inet_ntoa(raw[12:16]);destination=socket.inet_ntoa(raw[16:20]);local=(source,source_port) if outgoing else (destination,destination_port);server=(destination,destination_port) if outgoing else (source,source_port)
    return dict(frame=index,time=time.time(),payload=payload,direction='outgoing' if outgoing else 'incoming',connection=(local,server),cid=payload[wrapper+1:wrapper+21],udp_offset=ihl+8)

def run(folder,pid,parent,seconds=None):
    from network_decoder_host import publish
    folder.mkdir(parents=True,exist_ok=True);state_path=folder/'state.json';stop=folder/'stop';plan_path=folder/'plan.json'
    kernel=ctypes.windll.kernel32;kernel.GetTickCount64.restype=ctypes.c_ulonglong
    kernel.OpenProcess.argtypes=[ctypes.c_ulong,ctypes.c_int,ctypes.c_ulong];kernel.OpenProcess.restype=ctypes.c_void_p
    kernel.WaitForSingleObject.argtypes=[ctypes.c_void_p,ctypes.c_ulong];kernel.CloseHandle.argtypes=[ctypes.c_void_p]
    parent_handle=kernel.OpenProcess(0x100000,False,parent) if parent else None
    owner=network_schema_cache.identity(pid);keys=[];live=LiveDecoder(keys,network_schema_cache.load(folder.parent/'schema-cache.json',owner,max_age=None))
    driver=None;handles=[];ending=threading.Event();scan_done=threading.Event();scan_result={};editor=None;mode='Preparing';error='';seen=0;started=time.monotonic();draining=None;last_edit=0;pending={};acked=set();last_publish=0;last_plan=0;plan={};flow=None;out_context=None;in_context=None
    def status(message,ready=False):
        publish(state_path,dict(capture=mode,decoder=message,status=message,calls=[],ready=ready,error=error,stats=dict(editor.stats) if editor else {},last=editor.last if editor else None,keysWritten=False))
    def wants_stop():return stop.exists() or (parent_handle and kernel.WaitForSingleObject(parent_handle,0)==0) or (seconds is not None and time.monotonic()-started>=seconds)
    def scan():
        try:
            found,stats=candidates(pid,10000,stop_path=stop);keys.extend(found);found.clear();scan_result.update(stats)
        finally:scan_done.set()
    scan_thread=None;watcher=None;active_handle=[None]
    def watch():
        nonlocal mode,draining
        while not ending.wait(.1):
            if mode=='Running' and wants_stop():mode='Draining';draining=time.monotonic()
            drained=mode=='Draining' and draining is not None and ((not pending or all(clicked in acked for clicked in list(pending.values()))) and time.monotonic()-last_edit>=2 or time.monotonic()-draining>10)
            if active_handle[0] and ((mode=='Preparing' and (wants_stop() or time.monotonic()-started>60)) or drained):
                driver.shutdown(active_handle[0]);return
    try:
        if not owner or (parent and not parent_handle):raise RuntimeError('QB game or app process is unavailable')
        driver=Driver();warm=driver.open(GAME_FILTER,True);handles.append(warm);active_handle[0]=warm;status('Preparing QB throw control')
        scan_thread=threading.Thread(target=scan);scan_thread.start();watcher=threading.Thread(target=watch);watcher.start()
        observed=[];owned=ports(pid);last_ports=time.monotonic();last_decode=0
        while not wants_stop() and time.monotonic()-started<60:
            received=driver.recv(warm)
            if received is None:break
            raw,_=received;seen+=1;r=row(raw,seen)
            if not r:continue
            if time.monotonic()-last_ports>.2 or r['connection'][0][1] not in owned:owned=ports(pid);last_ports=time.monotonic()
            if r['connection'][0][1] not in owned:continue
            observed.append(r)
            if len(observed)>20000:raise RuntimeError('QB warm-up packet budget reached; retry setup')
            if scan_done.is_set() and time.monotonic()-last_decode>.5:
                if scan_result.get('error'):raise RuntimeError('QB transport keys are unavailable for this client')
                live.ingest(observed);last_decode=time.monotonic()
                for group,state in live.groups.items():
                    if group[1]=='outgoing' and state['context'] and group[0] in live.schemas:
                        incoming=next((s['context'] for g,s in live.groups.items() if g[0]==group[0] and g[1]=='incoming' and s['context']),None)
                        if incoming:flow=group[0];out_context=state['context'];in_context=incoming;break
                if flow:break
                status('Rejoin once to finish QB setup' if live.counts['authenticatedPackets'] else 'Preparing QB transport keys')
        if not flow:
            mode='Stopped';status('QB setup stopped' if wants_stop() else 'QB setup needs a fresh join');return 1
        editor=Editor(live.schemas[flow])
        def process_warm(r):
            if r['connection']!=flow:return
            if r['direction']=='outgoing':
                try:u,nonce,plain=authenticated_plain(r['payload'],out_context);editor.process_plain(plain,{},0,False)
                except DecodeError:pass
            else:
                try:
                    u=unprotect(r['payload'],in_context['hp'],23,in_context['largest']);recovered=recover_nonce(u,in_context['key'])
                    if recovered:
                        ranges=[];frames(recovered[1],ack_ranges=ranges)
                        if ranges:out_context['largest']=max(out_context['largest'],max(hi for lo,hi in ranges))
                except DecodeError:pass
        for r in observed:process_warm(r)
        local,server=flow
        exact=f'ip and udp and !loopback and ((outbound and ip.SrcAddr == {local[0]} and udp.SrcPort == {local[1]} and ip.DstAddr == {server[0]} and udp.DstPort == {server[1]}) or (inbound and ip.SrcAddr == {server[0]} and udp.SrcPort == {server[1]} and ip.DstAddr == {local[0]} and udp.DstPort == {local[1]}))'
        active=driver.open(exact);handles.append(active);driver.shutdown(warm)
        while True:
            received=driver.recv(warm)
            if received is None:break
            seen+=1;r=row(received[0],seen)
            if r:process_warm(r)
        driver.close(warm);handles.remove(warm);active_handle[0]=active;mode='Running';status('QB throw control ready',True)
        while True:
            if wants_stop() and draining is None:draining=time.monotonic();mode='Draining';status('Finishing the previous throw')
            if draining is not None and (not pending or all(clicked in acked for clicked in pending.values())) and time.monotonic()-last_edit>=2:break
            received=driver.recv(active)
            if received is None:break
            raw,address=received;changed=raw;seen+=1;requires_edit=False
            try:
                r=row(raw,seen)
                if r and r['connection']==flow:
                    if r['direction']=='outgoing':
                        u,nonce,plain=authenticated_plain(r['payload'],out_context)
                        if time.monotonic()-last_plan>.015:
                            try:
                                if plan_path.stat().st_size<=16384:plan=json.loads(plan_path.read_text(encoding='utf-8'))
                            except (OSError,ValueError):plan={}
                            last_plan=time.monotonic()
                        if plan.get('pid')!=pid:plan={}
                        before=editor.stats['modifiedThrows'];updated=editor.process_plain(plain,plan,int(kernel.GetTickCount64()),draining is None)
                        if updated!=plain:
                            requires_edit=True
                            payload=encrypt_changed(r['payload'],u,nonce,updated,out_context);changed=raw[:r['udp_offset']]+payload+raw[r['udp_offset']+len(payload):]
                            for clicked in editor.covered:pending[u.packet_number]=clicked
                            if editor.stats['modifiedThrows']>before:last_edit=time.monotonic()
                    else:
                        u=unprotect(r['payload'],in_context['hp'],23,in_context['largest']);recovered=recover_nonce(u,in_context['key'])
                        if recovered:
                            ranges=[];frames(recovered[1],ack_ranges=ranges)
                            for lo,hi in ranges:
                                out_context['largest']=max(out_context['largest'],hi)
                                for number,clicked in pending.items():
                                    if lo<=number<=hi:acked.add(clicked)
            except (DecodeError,KeyError,ValueError,TypeError):
                if editor:editor.stats['passedUnchanged']+=1
            finally:
                try:
                    if requires_edit and changed==raw:editor.stats['withheldForRetry']+=1
                    else:driver.send(active,changed,address,changed!=raw)
                except OSError:
                    if changed!=raw:
                        editor.stats['sendRetries']+=1
                        driver.send(active,changed,address,True)
                    else:raise
            if time.monotonic()-last_publish>.2:
                status('QB throw control ready' if draining is None else 'Finishing the previous throw',draining is None);last_publish=time.monotonic()
        mode='Stopped';status('QB throw control stopped');return 0
    except Exception as exc:
        error=str(exc);mode='Stopped';status('QB throw control unavailable');return 1
    finally:
        ending.set()
        for handle in handles:
            try:driver.shutdown(handle);driver.close(handle)
            except Exception:pass
        if watcher:watcher.join()
        if scan_thread:scan_thread.join()
        live.groups.clear();keys.clear()
        if parent_handle:kernel.CloseHandle(parent_handle)
