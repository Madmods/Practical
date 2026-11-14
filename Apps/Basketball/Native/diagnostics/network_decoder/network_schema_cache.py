"""Cache validated, non-secret schema bytes, bound to one process and connection."""
import ctypes,json,pathlib,time
from network_wire_lab import Schema

def identity(pid):
    kernel=ctypes.windll.kernel32
    kernel.OpenProcess.argtypes=[ctypes.c_ulong,ctypes.c_int,ctypes.c_ulong];kernel.OpenProcess.restype=ctypes.c_void_p
    kernel.GetProcessTimes.argtypes=[ctypes.c_void_p,*([ctypes.c_void_p]*4)];kernel.CloseHandle.argtypes=[ctypes.c_void_p]
    process=kernel.OpenProcess(0x1000,False,pid)
    if not process:return None
    values=[ctypes.c_ulonglong() for _ in range(4)]
    try:
        if not kernel.GetProcessTimes(process,*(ctypes.byref(v) for v in values)):return None
        return {'pid':pid,'created':values[0].value}
    finally:kernel.CloseHandle(process)

def load(path,owner,max_age=1800):
    try:
        if not owner or path.stat().st_size>512*1024:return {}
        root=json.loads(path.read_text(encoding='utf-8'))
        age=time.time()-root['saved']
        if root.get('owner')!=owner or age<0 or (max_age is not None and age>max_age):return {}
        result={}
        for entry in root.get('schemas',[])[:4]:
            connection=tuple(tuple(endpoint) for endpoint in entry['connection']);raw=bytes.fromhex(entry['body']);schema=Schema.from_packet(raw)
            cids={bytes.fromhex(cid) for cid in entry['cids']}
            if len(cids)>16 or any(len(cid)!=20 for cid in cids):continue
            result[connection]=(schema,cids,raw)
        return result
    except Exception:return {}

def save(path,owner,decoder):
    if not owner:return
    entries=[]
    for connection,raw in list(decoder.schema_packets.items())[-4:]:
        cids=[group[2].hex() for group,state in decoder.groups.items() if group[0]==connection and state['context']]
        entries.append(dict(connection=connection,body=raw.hex(),cids=cids))
    if not entries:return
    payload=json.dumps(dict(owner=owner,saved=time.time(),schemas=entries))
    if len(payload)>512*1024:return
    path.parent.mkdir(parents=True,exist_ok=True);temp=path.with_suffix('.tmp');temp.write_text(payload,encoding='utf-8');temp.replace(path)
