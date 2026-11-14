"""Explicitly started, bounded Vanta inspector host. No automatic capture on import."""
import argparse,ctypes,json,os,pathlib,subprocess,sys,time
from decode_saved_session import read_pcap
from live_network_decoder import LiveDecoder
from validate_live_transport import candidates
import network_schema_cache

BASE=pathlib.Path(sys.executable).parent if getattr(sys,'frozen',False) else pathlib.Path(__file__).parent

def publish(path,state):
    fields=('path','method','args','timestamp','source','pathResolved')
    state={**state,'calls':[{k:c[k] for k in fields if k in c} for c in state.get('calls',[])]}
    while len(json.dumps(state,ensure_ascii=False).encode('utf-8'))>60000 and state['calls']:
        removable=next((i for i,c in enumerate(state['calls']) if not (c.get('method')=='FireServer' and c.get('args') and c['args'][0]=='Clicked')),0)
        state['calls'].pop(removable)
    temporary=path.with_suffix('.tmp');temporary.write_text(json.dumps(state,ensure_ascii=False),encoding='utf-8')
    for attempt in range(5):
        try:os.replace(temporary,path);break
        except PermissionError:
            if attempt==4:raise
            time.sleep(.002)

def run(folder,pid,seconds,parent):
    folder.mkdir(parents=True,exist_ok=True);state_path=folder/'state.json';capture=folder/'packets.pcap';keys=[]
    owner=network_schema_cache.identity(pid);cache_path=folder.parent/'schema-cache.json';decoder=LiveDecoder(keys,network_schema_cache.load(cache_path,owner))
    started=time.monotonic();last_scan=0;scanned_flows=set();child=None
    kernel=ctypes.windll.kernel32;kernel.OpenProcess.argtypes=[ctypes.c_ulong,ctypes.c_int,ctypes.c_ulong];kernel.OpenProcess.restype=ctypes.c_void_p
    kernel.WaitForSingleObject.argtypes=[ctypes.c_void_p,ctypes.c_ulong];kernel.CloseHandle.argtypes=[ctypes.c_void_p]
    parent_handle=kernel.OpenProcess(0x00100000,False,parent) if parent else None
    if parent and not parent_handle:raise RuntimeError('Inspector parent is no longer available')
    try:
        if capture.exists():raise RuntimeError('Session destination must be new')
        publish(state_path,decoder.state('Starting'))
        child=subprocess.Popen([str(BASE/'capture-network-lab.exe'),'--capture',str(capture),'0',str(seconds)],creationflags=0x08000000)
        while time.monotonic()-started<seconds+15:
            if (folder/'stop').exists() or (parent_handle and kernel.WaitForSingleObject(parent_handle,0)==0):
                capture.with_suffix('.pcap.stop').touch();break
            if (folder/'clear').exists():decoder.calls.clear();decoder.highlights.clear();(folder/'clear').unlink()
            if capture.exists():
                try:rows=read_pcap(capture)
                except (OSError,ValueError):rows=[]
                flows={r['connection'] for r in rows}
                if flows-scanned_flows and time.monotonic()-last_scan>10:
                    publish(state_path,{**decoder.state(),'decoder':'Reading supported transport keys (up to 10 seconds)'})
                    new,_=candidates(pid,10000,stop_path=folder/'stop');keys.extend(k for k in new if k not in keys);new.clear();scanned_flows|=flows;last_scan=time.monotonic()
                decoder.ingest(rows);publish(state_path,decoder.state())
            if child.poll() is not None:break
            time.sleep(.4)
        capture.with_suffix('.pcap.stop').touch()
        if child:
            try:child.wait(timeout=5)
            except subprocess.TimeoutExpired:child.terminate();child.wait(timeout=3)
        if capture.exists():decoder.ingest(read_pcap(capture))
        final=decoder.state('Stopped');final['captureExitCode']=child.returncode if child else None;publish(state_path,final)
        try:publish(BASE.parent/'last-capture.json',final)
        except OSError:pass
        network_schema_cache.save(cache_path,owner,decoder)
        return 0
    except Exception as exc:
        publish(state_path,{**decoder.state('Stopped'),'error':str(exc)});return 1
    finally:
        capture.with_suffix('.pcap.stop').touch()
        if child and child.poll() is None:
            try:child.wait(timeout=3)
            except subprocess.TimeoutExpired:child.terminate();child.wait(timeout=3)
        decoder.groups.clear();keys.clear()
        if parent_handle:kernel.CloseHandle(parent_handle)

def main():
    if sys.argv[1:]==['--self-test']:
        from network_transport_lab import ecb
        assert len(ecb(bytes(32),bytes(16)))==16
        assert LiveDecoder([]).state('Stopped')['calls']==[]
        found,stats=candidates(0,0);assert not found and stats['bytesRead']==0
        from qb_packet_host import Driver,GAME_FILTER
        assert Driver().validate(GAME_FILTER)
        print('Host imports and AES runtime validated; no capture or process scan started')
        return
    parser=argparse.ArgumentParser();parser.add_argument('--session',type=pathlib.Path,required=True);parser.add_argument('--pid',type=int,required=True);parser.add_argument('--parent',type=int,default=0);parser.add_argument('--seconds',type=int,default=60);parser.add_argument('--qb-mode',action='store_true');args=parser.parse_args()
    if not 5<=args.seconds<=90 or args.pid<=0:parser.error('Requires a Roblox PID and 5–90 seconds')
    if args.qb_mode:
        from qb_packet_host import run as qb_run
        raise SystemExit(qb_run(args.session.resolve(),args.pid,args.parent,args.seconds if '--seconds' in sys.argv else None))
    raise SystemExit(run(args.session.resolve(),args.pid,args.seconds,args.parent))

if __name__=='__main__':main()
