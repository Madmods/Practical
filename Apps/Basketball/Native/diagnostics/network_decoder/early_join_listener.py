"""Passive, bounded encrypted join buffer. No game inputs, key scans or edits."""
import collections,ctypes,json,pathlib,secrets,threading,time

MAX_SEED_BYTES=8*1024*1024
MAX_SEED_PACKETS=4352

class JoinBuffer:
    def __init__(self,max_bytes=32*1024*1024,max_flows=8,prefix=4096,recent=256,ttl=120):
        self.max_bytes=max_bytes;self.max_flows=max_flows;self.prefix=prefix;self.recent=recent;self.ttl=ttl
        self.flows=collections.OrderedDict();self.bytes=0;self.sequence=0;self.lock=threading.RLock()

    def add(self,raw,connection,monotonic,wall):
        if not raw or len(raw)>65535 or len(raw)>self.max_bytes:return
        with self.lock:
            self.sequence+=1
            entry=(self.sequence,monotonic,wall,bytes(raw))
            if connection not in self.flows:self.flows[connection]=[[],collections.deque(),monotonic]
            flow=self.flows[connection];flow[2]=monotonic;self.flows.move_to_end(connection)
            if len(flow[0])<self.prefix:flow[0].append(entry)
            else:
                flow[1].append(entry)
                if len(flow[1])>self.recent:self.bytes-=len(flow[1].popleft()[3])
            self.bytes+=len(raw)
            for key in list(self.flows):
                if monotonic-self.flows[key][2]>self.ttl:self._remove(key)
            while len(self.flows)>self.max_flows or self.bytes>self.max_bytes:
                self._remove(next(iter(self.flows)))

    def _remove(self,connection):
        flow=self.flows.pop(connection)
        self.bytes-=sum(len(row[3]) for group in flow[:2] for row in group)

    def snapshot(self,owner,owned_ports,now,connection=None):
        if not owner or not owned_ports:return []
        born=owner['created']/10000000-11644473600
        with self.lock:
            choices=[key for key,flow in self.flows.items() if key[0][1] in owned_ports and now-flow[2]<=self.ttl and (connection is None or key==connection)]
            if not choices:return []
            selected=choices[-1];flow=self.flows[selected];out=[];size=0
            for row in sorted((*flow[0],*flow[1]),key=lambda row:row[0]):
                if row[2]<born-.1 or now-row[1]>self.ttl:continue
                if size+len(row[3])>MAX_SEED_BYTES:break
                out.append(row[3]);size+=len(row[3])
            return out

def atomic_json(path,value):
    temporary=path.with_suffix('.tmp');temporary.write_text(json.dumps(value,separators=(',',':')),encoding='utf-8')
    for attempt in range(5):
        try:temporary.replace(path);return
        except PermissionError:
            if attempt==4:raise
            time.sleep(.002)

def request_seed(folder,pid,connection=None,timeout=.4,identity=None):
    import network_schema_cache
    owner=(identity or network_schema_cache.identity)(pid)
    if not owner:return []
    nonce=secrets.token_hex(16);response=folder/('seed-'+nonce+'.json')
    try:
        atomic_json(folder/'request.json',dict(nonce=nonce,owner=owner,connection=connection))
        deadline=time.monotonic()+timeout
        while time.monotonic()<deadline:
            try:
                if response.stat().st_size>20*1024*1024:return []
                state=json.loads(response.read_text(encoding='utf-8'))
                if state.get('owner')!=owner or state.get('nonce')!=nonce or (identity or network_schema_cache.identity)(pid)!=owner:return []
                packets=[];size=0
                for value in state.get('packets',[])[:MAX_SEED_PACKETS]:
                    if not isinstance(value,str) or len(value)>131070:return []
                    packet=bytes.fromhex(value);size+=len(packet)
                    if size>MAX_SEED_BYTES:return []
                    packets.append((packet,None))
                return packets
            except FileNotFoundError:time.sleep(.01)
        return []
    except (OSError,ValueError,TypeError,KeyError):return []
    finally:
        try:response.unlink(missing_ok=True)
        except OSError:pass

def run(folder,parent):
    from qb_packet_host import Driver,GAME_FILTER,row,ports
    import network_schema_cache
    folder.mkdir(parents=True,exist_ok=True);buffer=JoinBuffer();stop=threading.Event();driver=None;handle=None;reader=None;last_nonce='';last_status=0;error=''
    kernel=ctypes.windll.kernel32
    kernel.OpenProcess.argtypes=[ctypes.c_ulong,ctypes.c_int,ctypes.c_ulong];kernel.OpenProcess.restype=ctypes.c_void_p
    kernel.WaitForSingleObject.argtypes=[ctypes.c_void_p,ctypes.c_ulong];kernel.CloseHandle.argtypes=[ctypes.c_void_p];kernel.GetTickCount64.restype=ctypes.c_ulonglong
    parent_handle=kernel.OpenProcess(0x100000,False,parent) if parent else None
    def state(ready):atomic_json(folder/'state.json',dict(listenerReady=ready,capture='Listening' if ready else 'Stopped',status='Join listener ready' if ready else 'Join listener stopped',error=error,bufferedBytes=buffer.bytes,flows=len(buffer.flows),heartbeat=int(kernel.GetTickCount64()),keysWritten=False,packetEdits=False))
    def receive():
        nonlocal error
        count=0
        try:
            while not stop.is_set():
                received=driver.recv(handle)
                if received is None:break
                count+=1;packet=row(received[0],count)
                if packet:buffer.add(received[0],packet['connection'],time.monotonic(),time.time())
        except OSError as exc:error=str(exc)
        finally:stop.set()
    try:
        if parent and not parent_handle:raise RuntimeError('Vanta parent is unavailable')
        # Overlapping WinDivert handles must use distinct priorities:
        # https://reqrypt.org/windivert-doc.html (WinDivertOpen).
        driver=Driver();handle=driver.open(GAME_FILTER,True,priority=1200);reader=threading.Thread(target=receive);reader.start();state(True)
        while not stop.wait(.05):
            if (folder/'stop').exists() or (parent_handle and kernel.WaitForSingleObject(parent_handle,0)==0):break
            try:
                request_path=folder/'request.json'
                if request_path.stat().st_size<=4096:
                    request=json.loads(request_path.read_text(encoding='utf-8'));nonce=request.get('nonce','');owner=request.get('owner')
                    if len(nonce)==32 and all(c in '0123456789abcdef' for c in nonce) and nonce!=last_nonce:
                        last_nonce=nonce;pid=owner['pid'];current=network_schema_cache.identity(pid);connection=request.get('connection');connection=tuple(tuple(part) for part in connection) if connection else None
                        packets=buffer.snapshot(owner,ports(pid),time.monotonic(),connection) if current==owner else []
                        atomic_json(folder/('seed-'+nonce+'.json'),dict(nonce=nonce,owner=owner,packets=[packet.hex() for packet in packets],keysWritten=False))
            except (OSError,ValueError,TypeError,KeyError):pass
            if time.monotonic()-last_status>=.5:state(True);last_status=time.monotonic()
            # Only nonce-shaped replies produced by this listener are retired.
            for reply in folder.glob('seed-*.json'):
                name=reply.stem[5:]
                if len(name)==32 and all(c in '0123456789abcdef' for c in name):
                    try:
                        if time.time()-reply.stat().st_mtime>20:reply.unlink()
                    except OSError:pass
        return 0
    except Exception as exc:error=str(exc);return 1
    finally:
        stop.set()
        if driver and handle:
            driver.shutdown(handle)
            if reader:reader.join()
            driver.close(handle)
        buffer.flows.clear();buffer.bytes=0
        try:state(False)
        except OSError:pass
        for reply in folder.glob('seed-*.json'):
            name=reply.stem[5:]
            if len(name)==32 and all(c in '0123456789abcdef' for c in name):
                try:reply.unlink()
                except OSError:pass
        if parent_handle:kernel.CloseHandle(parent_handle)
