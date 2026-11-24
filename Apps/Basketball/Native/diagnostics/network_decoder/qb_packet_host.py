"""Explicit QB mode: warm up read-only, then edit only the owned game flow.

The normal game generates the throw. No event is synthesized or replayed.
Original datagrams are forwarded on every processing error. Already accepted
stream edits remain pinned during disable/drain so retransmissions stay stable.
"""
import ctypes,json,pathlib,struct,socket,threading,time,collections,sys
from network_transport_lab import DecodeError,frames,unprotect,recover_nonce
from live_network_decoder import LiveDecoder
from qb_packet_editor import Editor,authenticated_plain,encrypt_changed
from validate_live_transport import candidates
import network_schema_cache
from transport_recovery import TransportHealth, RecoveryBudget, preparation_presentation

GAME_FILTER='ip and udp and !loopback and (udp.Payload32[0] == 0x01000017 or udp.Payload32[0] == 0x0100001f) and udp.Payload16[4b] == 0x0111 and udp.Payload[6] == 2'
def dependency_path():
    base=pathlib.Path(sys.executable).resolve().parent if getattr(sys,'frozen',False) else pathlib.Path(__file__).resolve().parent
    path=base/'WinDivert.dll'
    if not path.is_file() or not (base/'WinDivert64.sys').is_file():
        raise FileNotFoundError('Networking files are missing. Extract the entire Vanta ZIP and keep its folders together')
    return path

class Driver:
    def __init__(self):
        self.dll=ctypes.WinDLL(str(dependency_path()),use_last_error=True);d=self.dll
        d.WinDivertOpen.argtypes=[ctypes.c_char_p,ctypes.c_int,ctypes.c_short,ctypes.c_ulonglong];d.WinDivertOpen.restype=ctypes.c_void_p
        d.WinDivertRecv.argtypes=[ctypes.c_void_p,ctypes.c_void_p,ctypes.c_uint,ctypes.POINTER(ctypes.c_uint),ctypes.c_void_p];d.WinDivertRecv.restype=ctypes.c_int
        d.WinDivertSend.argtypes=d.WinDivertRecv.argtypes;d.WinDivertSend.restype=ctypes.c_int
        d.WinDivertShutdown.argtypes=[ctypes.c_void_p,ctypes.c_int];d.WinDivertClose.argtypes=[ctypes.c_void_p]
        d.WinDivertSetParam.argtypes=[ctypes.c_void_p,ctypes.c_int,ctypes.c_ulonglong];d.WinDivertSetParam.restype=ctypes.c_int
        d.WinDivertHelperCalcChecksums.argtypes=[ctypes.c_void_p,ctypes.c_uint,ctypes.c_void_p,ctypes.c_ulonglong];d.WinDivertHelperCalcChecksums.restype=ctypes.c_int
        d.WinDivertHelperCompileFilter.argtypes=[ctypes.c_char_p,ctypes.c_int,ctypes.c_void_p,ctypes.c_uint,ctypes.POINTER(ctypes.c_char_p),ctypes.POINTER(ctypes.c_uint)];d.WinDivertHelperCompileFilter.restype=ctypes.c_int
    def validate(self,filter):
        error=ctypes.c_char_p();position=ctypes.c_uint()
        return bool(self.dll.WinDivertHelperCompileFilter(filter.encode('ascii'),0,None,0,ctypes.byref(error),ctypes.byref(position)))
    def open(self,filter,sniff=False,priority=None):
        level=priority if priority is not None else (1100 if sniff else 1000)
        handle=self.dll.WinDivertOpen(filter.encode('ascii'),0,level,5 if sniff else 0)
        if handle in (None,ctypes.c_void_p(-1).value):raise OSError(ctypes.get_last_error(),'Cannot open QB transport')
        if sniff:
            # Key discovery can take seconds during a join. Retain the initial
            # dictionary burst while the read-only decoder authenticates it.
            for parameter,value in ((0,16384),(1,16000),(2,33554432)):
                if not self.dll.WinDivertSetParam(handle,parameter,value):
                    self.dll.WinDivertClose(handle);raise OSError(ctypes.get_last_error(),'Cannot configure QB setup buffering')
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

def current_flow(live,observed,owned):
    # Rejoins retain old authenticated groups. Choose only the newest observed
    # connection on a socket still owned by this process, with both current CIDs.
    recent=[r for r in observed[-512:] if r['connection'][0][1] in owned]
    if not recent:return None
    connection=recent[-1]['connection']
    if connection not in live.schemas:return None
    contexts=[]
    for direction in ('outgoing','incoming'):
        latest=next((r for r in reversed(recent) if r['connection']==connection and r['direction']==direction),None)
        if latest is None:return None
        state=live.groups.get((connection,direction,latest['cid']))
        if not state or not state['context']:return None
        contexts.append(state['context'])
    return connection,*contexts

def setup_phase(live,observed,owned):
    recent=[r for r in observed[-512:] if r['connection'][0][1] in owned]
    if not recent:return 'waiting'
    connection=recent[-1]['connection']
    for direction in ('outgoing','incoming'):
        latest=next((r for r in reversed(recent) if r['connection']==connection and r['direction']==direction),None)
        if latest is None or not live.groups.get((connection,direction,latest['cid']),{}).get('context'):
            return 'keys-'+direction
    return 'rejoin' if connection not in live.schemas else 'checking'

def setup_status(live,observed,owned):
    phase=setup_phase(live,observed,owned)
    if phase=='waiting':return 'Waiting for the current game connection'
    if phase.startswith('keys-'):return 'Preparing keys for '+phase[5:]+' traffic on this game connection'
    if phase=='rejoin':return 'Rejoin FF3 without holding a football. Keep Vanta open while you rejoin'
    return 'Checking current QB connection'

def setup_failure(live,observed,owned,key_count,scan_result):
    recent=[r for r in observed[-512:] if r['connection'][0][1] in owned]
    if scan_result.get('error'):return 'QB cannot read this game process. Retry setup; normal throwing remains available'
    if not recent:return 'No supported traffic from this Roblox client. Join FF3, then retry QB setup'
    if not key_count:return 'No supported transport keys found for this client. QB assistance is unavailable; retry setup for a fresh diagnostic'
    stage=setup_status(live,observed,owned)
    if 'Preparing keys' in stage:return 'Transport authentication failed for this client. QB assistance is unavailable; retry setup'
    if setup_phase(live,observed,owned)=='rejoin':return 'Join schema missing. Retry setup, then rejoin FF3 without holding a football. Keep Vanta open'
    return 'QB setup stopped before the current connection became ready. Retry QB setup'

def scan_incrementally(pid,keys,result,cancel,stop_path,start_address=0):
    # Publish candidate keys after short slices; preserve the total scan budget.
    started=time.monotonic();address=start_address;total=0
    while not cancel.is_set() and not stop_path.exists() and time.monotonic()-started<4 and total<1024*1024*1024:
        found,stats=candidates(pid,250,start_address=address,stop_path=stop_path,max_bytes=1024*1024*1024-total)
        keys.extend(k for k in found if k not in keys);found.clear();total+=stats.get('bytesRead',0);result.update(stats);result['bytesRead']=total
        following=stats.get('nextAddress',0)
        if stats.get('error') or not stats.get('limitReached') or following<=address:break
        address=following

def control_status(editor):
    if editor.recovering:return 'Resynchronizing match throw data',False
    # Setup has already authenticated this connection and loaded its schema.
    # Each actual throw validates its own contract before replacement; it does
    # not need a separate, unassisted calibration throw first.
    return 'QB throw control ready',True

def transport_sample(observed):
    # Encrypted datagrams only. Keep both directions despite asymmetric traffic.
    recent=[]
    for direction in ('outgoing','incoming'):
        recent.extend([r for r in observed if r['direction']==direction][-12:])
    return dict(packets=[dict(frame=r['frame'],direction=r['direction'],connection=r['connection'],cid=r['cid'].hex(),payload=r['payload'][:2048].hex(),truncated=len(r['payload'])>2048) for r in recent],keysWritten=False)

def run_recovering(folder,pid,parent,seconds=None,seed_provider=None):
    from network_decoder_host import publish
    budget=RecoveryBudget();began=time.monotonic()
    while True:
        remaining=None if seconds is None else max(0,seconds-(time.monotonic()-began))
        options=dict(recovery_attempt=len(budget.attempts))
        if seed_provider is not None:options['seed_provider']=seed_provider
        result=run(folder,pid,parent,remaining,**options)
        try:state=json.loads((folder/'state.json').read_text(encoding='utf-8'))
        except (OSError,ValueError):return result
        reason=state.get('recoveryReason','')
        if (folder/'stop').exists() or (seconds is not None and time.monotonic()-began>=seconds):return result
        delay=budget.next_delay(time.monotonic(),reason)
        if delay is None:
            if reason=='authentication':
                state.update(capture='Stopped',phase='update-required',ready=False,retryable=False,updateRequired=True,status='Automatic key recovery could not authenticate this client. A compatible helper update is required; normal throwing remains available',error='Transport authentication remains unsupported after bounded automatic recovery')
                publish(folder/'state.json',state)
            return result
        deadline=time.monotonic()+delay
        while time.monotonic()<deadline:
            state.update(capture='Preparing',phase='recovering',ready=False,requiresRejoin=False,automaticRecovery=True,recoveryAttempt=len(budget.attempts),status='Refreshing connection setup automatically (attempt '+str(len(budget.attempts))+' of 3)',error='',heartbeat=int(ctypes.windll.kernel32.GetTickCount64()))
            publish(folder/'state.json',state)
            if (folder/'stop').exists():return result
            time.sleep(min(.1,max(0,deadline-time.monotonic())))

def run(folder,pid,parent,seconds=None,recovery_attempt=0,seed_provider=None):
    from network_decoder_host import publish
    folder.mkdir(parents=True,exist_ok=True);state_path=folder/'state.json';stop=folder/'stop';plan_path=folder/'plan.json';setup_only=folder/'setup-only';cache_path=folder.parent/'schema-cache.json'
    kernel=ctypes.windll.kernel32;kernel.GetTickCount64.restype=ctypes.c_ulonglong
    kernel.OpenProcess.argtypes=[ctypes.c_ulong,ctypes.c_int,ctypes.c_ulong];kernel.OpenProcess.restype=ctypes.c_void_p
    kernel.WaitForSingleObject.argtypes=[ctypes.c_void_p,ctypes.c_ulong];kernel.CloseHandle.argtypes=[ctypes.c_void_p]
    parent_handle=kernel.OpenProcess(0x100000,False,parent) if parent else None
    owner=network_schema_cache.identity(pid);keys=[];live=LiveDecoder(keys,network_schema_cache.load(folder.parent/'schema-cache.json',owner,max_age=None))
    driver=None;handles=[];ending=threading.Event();scan_done=threading.Event();scan_result={};editor=None;mode='Preparing';error='';seen=0;started=time.monotonic();draining=None;last_edit=0;pending={};acked=set();last_publish=0;last_plan=0;plan={};flow=None;out_context=None;in_context=None
    scan_rounds=0;scan_groups=set();last_scan=0;last_receive=0;last_port_check=0;scan_cancel=threading.Event();saved_flow=None;owned_packets=0;scan_resume=0;observed=[];setup_deadline=float('inf');last_diagnostic=0;last_transport_sample=-10
    scan_done.set();perf_started=time.perf_counter();traffic_started=None;timings={};recovery_reason='';health=None;seed_queue=collections.deque();seeded_flows=set();seed_retry={};seed_hashes=set();seed_count=0
    publish_lock=threading.RLock()
    def status(message,ready=False):
        with publish_lock:_status(message,ready)
    def _status(message,ready=False):
        phase=setup_phase(live,observed,ports(pid))
        rejoin=phase=='rejoin' and not error and not stop.exists()
        if mode=='Preparing':
            phase,recovery_message=preparation_presentation(phase,0 if traffic_started is None else time.perf_counter()-traffic_started)
            if recovery_message:message=recovery_message
        if mode=='Draining' and recovery_reason:phase='recovering';ready=False
        if health and health.suspect:ready=False

        publish(state_path,dict(capture=mode,phase='ready' if ready else ('failed' if mode=='Stopped' else phase),requiresRejoin=rejoin,decoder=message,status=message,calls=[],ready=ready,error=error,stats={**dict(live.counts),**(dict(editor.stats) if editor else {}),'observedPackets':seen,'scanRounds':scan_rounds,'ownedPackets':owned_packets,'bufferedJoinPackets':seed_count,'keyCandidateCount':len(keys),'scanBytes':scan_result.get('bytesRead',0),'scanError':scan_result.get('error',0),'authenticatedOutgoingGroups':sum(bool(s['context']) for g,s in live.groups.items() if g[1]=='outgoing'),'authenticatedIncomingGroups':sum(bool(s['context']) for g,s in live.groups.items() if g[1]=='incoming'),'setupTimingsMs':dict(timings)},last=editor.last if editor else None,lastDecline=editor.last_decline if editor else None,history=editor.decision_history() if editor else [],heartbeat=int(kernel.GetTickCount64()),recoveryReason=recovery_reason,recoveryAttempt=recovery_attempt,initialReadyDeadlineMs=5000,automaticRecovery=True,localPort=flow[0][1] if flow else None,keysWritten=False))
    def wants_stop():return stop.exists() or (mode!='Preparing' and setup_only.exists()) or (parent_handle and kernel.WaitForSingleObject(parent_handle,0)==0) or (seconds is not None and time.monotonic()-started>=seconds)
    def scan():
        nonlocal scan_resume
        try:
            scan_incrementally(pid,keys,scan_result,scan_cancel,stop,start_address=scan_resume)
            scan_resume=scan_result.get('nextAddress',0) if scan_result.get('limitReached') else 0
        except Exception as exc:scan_result['error']=str(exc)
        finally:scan_done.set()
    scan_thread=None;watcher=None;active_handle=[None]
    def watch():
        nonlocal mode,draining,error,last_port_check,recovery_reason
        last_preparing_status=0
        while not ending.wait(.1):
            if mode=='Preparing' and traffic_started is not None and time.perf_counter()-traffic_started>=5 and time.monotonic()-last_preparing_status>=.5 and setup_phase(live,observed,ports(pid)).startswith('keys-'):
                status(setup_status(live,observed,ports(pid)));last_preparing_status=time.monotonic()
            if mode=='Running':
                stamp=time.monotonic()
                if stamp-last_receive>3:
                    recovery_reason='traffic-stalled';error='QB traffic stopped; restoring normal traffic before automatic recovery';mode='Draining';draining=stamp
                elif flow and stamp-last_port_check>1:
                    last_port_check=stamp
                    if flow[0][1] not in ports(pid):recovery_reason='connection-changed';error='Game connection changed; preparing automatic recovery';mode='Draining';draining=stamp
                if wants_stop():mode='Draining';draining=stamp
                if mode=='Draining' and recovery_reason:status('Connection authentication or traffic stopped; restoring normal traffic before automatic recovery',False)
            drained=mode=='Draining' and draining is not None and ((not pending or all(clicked in acked for clicked in list(pending.values()))) and time.monotonic()-last_edit>=2 or time.monotonic()-draining>10)
            if active_handle[0] and ((mode=='Preparing' and (wants_stop() or time.monotonic()>setup_deadline)) or drained):
                driver.shutdown(active_handle[0]);return
    try:
        if not owner or (parent and not parent_handle):raise RuntimeError('QB game or app process is unavailable')
        driver=Driver();warm=driver.open(GAME_FILTER,True);handles.append(warm);active_handle[0]=warm;status('Listening for the game connection')
        watcher=threading.Thread(target=watch);watcher.start()
        observed=[];owned=ports(pid);last_ports=time.monotonic();last_decode=0;warm_connection=None
        while not wants_stop() and time.monotonic()<setup_deadline:
            from_seed=bool(seed_queue);received=seed_queue.popleft() if from_seed else driver.recv(warm)
            if received is None:break
            raw,_=received
            if not from_seed and raw in seed_hashes:continue
            seen+=1;r=row(raw,seen)
            if not r:continue
            if time.monotonic()-last_ports>.2 or r['connection'][0][1] not in owned:owned=ports(pid);last_ports=time.monotonic()
            if r['connection'][0][1] not in owned:continue
            owned_packets+=1
            if not from_seed and seed_provider is not None and r['connection'] not in seeded_flows and time.monotonic()>=seed_retry.get(r['connection'],0):
                seed_retry[r['connection']]=time.monotonic()+.5
                try:prior=seed_provider(r['connection'])[:4352]
                except (OSError,ValueError,TypeError):prior=[]
                scoped=[]
                for item in prior:
                    parsed=row(item[0],0)
                    if parsed and parsed['connection']==r['connection']:scoped.append(item)
                if scoped:
                    if len(seeded_flows)>=8:seeded_flows.clear()
                    seeded_flows.add(r['connection']);seed_count+=len(scoped);seed_hashes.clear();seed_hashes.update(item[0] for item in scoped);seed_queue.extend(scoped)
                    if raw not in seed_hashes:seed_queue.append(received)
                    continue
            if traffic_started is None:
                traffic_started=time.perf_counter();timings['listenerWait']=round((traffic_started-perf_started)*1000);setup_deadline=time.monotonic()+120
            if warm_connection and warm_connection!=r['connection'] and warm_connection[0][1] not in owned:
                # A real rejoin closed the old socket. Its queued packets,
                # decoder limits and key retry count must not starve the join.
                observed=[];live=LiveDecoder(keys,network_schema_cache.load(cache_path,owner,max_age=None))
                scan_rounds=0;scan_groups=set();last_scan=0;last_decode=0;flow=None;saved_flow=None;scan_resume=0;setup_deadline=time.monotonic()+120;traffic_started=time.perf_counter();timings={}
            warm_connection=r['connection']
            observed.append(r)
            if time.monotonic()-last_transport_sample>=2:
                publish(folder/'transport-diagnostic.json',transport_sample(observed));last_transport_sample=time.monotonic()
            # Keep a rolling setup sample while the prepared helper waits for
            # QBA. Decoder streams keep their own bounded reassembly state.
            if len(observed)>4096:del observed[:-4096]
            if hasattr(live,'seen') and len(live.seen)>8192:live.seen.intersection_update(r['frame'] for r in observed)
            signature=(r['connection'],r['cid'])
            if not flow and scan_done.is_set() and scan_rounds<3 and (scan_rounds==0 or time.monotonic()-last_scan>.25) and (signature not in scan_groups or 'Preparing keys' in setup_status(live,observed,owned)):
                if scan_thread:scan_thread.join()
                scan_groups={(item['connection'],item['cid']) for item in observed};scan_done.clear();scan_cancel.clear();scan_result.clear();scan_rounds+=1;last_scan=time.monotonic();scan_thread=threading.Thread(target=scan);scan_thread.start()
            if scan_done.is_set() and scan_result.get('error'):raise RuntimeError('QB transport keys are unavailable for this client')
            if scan_done.is_set() and scan_rounds>=3 and setup_phase(live,observed,owned).startswith('keys-'):
                # Keep the listener open through a short join transition;
                # a fresh socket resets this exhausted batch before retrying.
                setup_deadline=min(setup_deadline,time.monotonic()+5)
            if keys and time.monotonic()-last_decode>.25:
                timings.setdefault('firstCandidates',round((time.perf_counter()-traffic_started)*1000))
                if scan_result.get('error'):raise RuntimeError('QB transport keys are unavailable for this client')
                live.ingest(observed);last_decode=time.monotonic()
                selected=current_flow(live,observed,ports(pid))
                if selected:flow,out_context,in_context=selected
                else:flow=None
                if flow:
                    timings.setdefault('dictionaryReady',round((time.perf_counter()-traffic_started)*1000))
                    scan_cancel.set()
                    if saved_flow!=flow:network_schema_cache.save(cache_path,owner,live);saved_flow=flow
                    if setup_only.exists():
                        setup_deadline=time.monotonic()+120
                        status('Setup ready. Press Continue; QB aim will resume automatically' if (folder/'resume-after-join').exists() else 'Networking ready. QBA is off',True)
                        continue
                    network_schema_cache.save(cache_path,owner,live)
                    break
                status(setup_status(live,observed,ports(pid)))
            elif not keys and time.monotonic()-last_publish>.5:
                last_publish=time.monotonic();status('Reading current transport keys' if not scan_done.is_set() else 'No supported keys found yet; waiting for this connection')
        if wants_stop():
            mode='Stopped';status('QB setup stopped');return 0
        if not flow:
            if setup_phase(live,observed,ports(pid)).startswith('keys-') and not scan_result.get('error'):recovery_reason='authentication'
            mode='Stopped';status('QB setup stopped' if wants_stop() else setup_failure(live,observed,ports(pid),len(keys),scan_result));return 1
        if setup_only.exists():
            mode='Stopped';status('Join setup saved. Enable QB aim in Football Fusion 3');return 0
        editor=Editor(live.schemas[flow]);active_schema=live.schema_packets.get(flow)
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
        driver.close(warm);handles.remove(warm);active_handle[0]=active;last_receive=time.monotonic();mode='Running';health=TransportHealth(time.monotonic());message,usable=control_status(editor);status(message,usable)
        while True:
            if wants_stop() and draining is None:draining=time.monotonic();mode='Draining';status('Finishing the previous throw')
            if draining is not None and (not pending or all(clicked in acked for clicked in pending.values())) and time.monotonic()-last_edit>=2:break
            received=driver.recv(active)
            if received is None:break
            raw,address=received;last_receive=time.monotonic();changed=raw;seen+=1;requires_edit=False;transport_authenticated=False;r=None
            try:
                r=row(raw,seen)
                if r and r['connection']==flow:
                    if r['direction']=='outgoing':
                        u,nonce,plain=authenticated_plain(r['payload'],out_context);transport_authenticated=True
                        # A click publishes immediately before game input continues.
                        # Do not reuse a pre-click plan for the first throw packet.
                        try:
                            meta=plan_path.stat()
                            if meta.st_size>65536:plan={}
                            elif meta.st_mtime_ns!=last_plan:
                                plan=json.loads(plan_path.read_text(encoding='utf-8'));last_plan=meta.st_mtime_ns
                        except (OSError,ValueError):plan={};last_plan=0
                        if plan.get('pid')!=pid:plan={}
                        before=editor.stats['modifiedThrows'];updated=editor.process_plain(plain,plan,int(kernel.GetTickCount64()),draining is None and not health.suspect)
                        if editor.diagnostic_sequence!=last_diagnostic:
                            publish(folder/'throw-diagnostic.json',dict(records=list(editor.throw_diagnostics),keysWritten=False));last_diagnostic=editor.diagnostic_sequence
                        if updated!=plain:
                            requires_edit=True
                            payload=encrypt_changed(r['payload'],u,nonce,updated,out_context);changed=raw[:r['udp_offset']]+payload+raw[r['udp_offset']+len(payload):]
                            for clicked in editor.covered:pending[u.packet_number]=clicked
                            if editor.stats['modifiedThrows']>before:last_edit=time.monotonic()
                    else:
                        # A match can teach a fresh descriptor dictionary after
                        # setup. Keep the decoder attached to incoming schema
                        # records instead of pinning the warm-up dictionary.
                        if len(getattr(live,'seen',()))>8192:live.seen.clear()
                        live.ingest([r])
                        fresh_schema=live.schema_packets.get(flow)
                        if fresh_schema and fresh_schema!=active_schema:
                            editor.update_schema(live.schemas[flow],int(kernel.GetTickCount64()));active_schema=fresh_schema
                            network_schema_cache.save(cache_path,owner,live)
                        u=unprotect(r['payload'],in_context['hp'],23,in_context['largest']);recovered=recover_nonce(u,in_context['key'])
                        if recovered:
                            transport_authenticated=True
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
            if r and r['connection']==flow:
                health.record(r['direction'],transport_authenticated,time.monotonic())
                if health.needs_refresh(time.monotonic()) and draining is None:
                    recovery_reason='authentication';mode='Draining';draining=time.monotonic()
                    status('Transport authentication changed; packet edits paused before automatic key recovery',False)
            if time.monotonic()-last_publish>.2:
                message,usable=control_status(editor)
                if health.suspect:message,usable='Authentication changed; packet edits paused while the connection is checked',False

                status(message if draining is None else 'Finishing the previous throw',draining is None and usable);last_publish=time.monotonic()
        mode='Stopped';status('QB throw control stopped');return 0
    except Exception as exc:
        error=str(exc);mode='Stopped';status('QB throw control unavailable');return 1
    finally:
        ending.set();scan_cancel.set()
        for handle in handles:
            try:driver.shutdown(handle);driver.close(handle)
            except Exception:pass
        if watcher:watcher.join()
        if scan_thread:scan_thread.join()
        live.groups.clear();keys.clear()
        if parent_handle:kernel.CloseHandle(parent_handle)
