import collections,json,pathlib,tempfile,threading,time,unittest,queue
from types import SimpleNamespace
from unittest.mock import Mock,patch
from early_join_listener import JoinBuffer,atomic_json,request_seed,run as run_listener
import qb_packet_host as host
from network_wire_lab import Schema
from test_network_wire_lab import fixture_schema

FLOW=(('10.0.0.1',1000),('10.0.0.2',2000))
OWNER=dict(pid=77,created=int((11644473600+100)*10000000))

class BufferTests(unittest.TestCase):
    def test_first_join_survives_later_traffic(self):
        buffer=JoinBuffer(prefix=3,recent=2)
        for i in range(20):buffer.add(('packet'+str(i)).encode(),FLOW,10+i*.01,101+i*.01)
        self.assertEqual(buffer.snapshot(OWNER,{1000},11),[b'packet0',b'packet1',b'packet2',b'packet18',b'packet19'])
        self.assertEqual(buffer.snapshot(OWNER,{999},11),[])
        self.assertEqual(buffer.snapshot(None,{1000},11),[])

    def test_process_start_and_current_ports_scope_the_handoff(self):
        buffer=JoinBuffer();buffer.add(b'old-process',FLOW,10,99);buffer.add(b'new-process',FLOW,11,101)
        self.assertEqual(buffer.snapshot(OWNER,{1000},12),[b'new-process'])
        self.assertEqual(buffer.snapshot(OWNER,{1000},200),[])
        other=(('10.0.0.1',5000),('10.0.0.9',2000));buffer.add(b'other-process',other,12,102)
        self.assertEqual(buffer.snapshot(OWNER,{1000},13),[b'new-process'])

    def test_memory_and_flow_counts_are_bounded(self):
        buffer=JoinBuffer(max_bytes=40,max_flows=2,prefix=2,recent=2)
        for i in range(100):buffer.add(bytes(10),(('local',i%5),('server',1)),i,100+i)
        self.assertLessEqual(buffer.bytes,40);self.assertLessEqual(len(buffer.flows),2)

    def test_initial_dictionary_after_a_thousand_datagrams_is_retained(self):
        buffer=JoinBuffer()
        for i in range(6000):buffer.add(('dictionary' if i==1500 else 'packet'+str(i)).encode(),FLOW,10+i*.0001,101+i*.0001)
        snapshot=buffer.snapshot(OWNER,{1000},12)
        self.assertIn(b'dictionary',snapshot);self.assertLessEqual(len(snapshot),4352)

    def test_specific_new_connection_never_returns_an_old_server(self):
        buffer=JoinBuffer();other=(('10.0.0.1',1000),('10.0.0.3',2000))
        buffer.add(b'first-server',FLOW,10,101);buffer.add(b'next-server',other,11,102)
        self.assertEqual(buffer.snapshot(OWNER,{1000},12,FLOW),[b'first-server'])
        self.assertEqual(buffer.snapshot(OWNER,{1000},12),[b'next-server'])

    def test_response_owner_nonce_and_process_reuse_are_validated(self):
        for changed in (False,True):
            with tempfile.TemporaryDirectory() as directory:
                folder=pathlib.Path(directory);calls=[0]
                def identity(pid):calls[0]+=1;return {**OWNER,'created':OWNER['created']+1} if changed and calls[0]>1 else OWNER
                def responder():
                    while not (folder/'request.json').exists():time.sleep(.001)
                    request=json.loads((folder/'request.json').read_text());atomic_json(folder/('seed-'+request['nonce']+'.json'),dict(owner=OWNER,nonce=request['nonce'],packets=[b'ciphertext'.hex()]))
                thread=threading.Thread(target=responder);thread.start();packets=request_seed(folder,77,FLOW,identity=identity);thread.join()
                self.assertEqual(packets,[] if changed else [(b'ciphertext',None)])
                self.assertFalse(list(folder.glob('seed-*.json')))

    def test_late_target_attachment_can_prepare_from_the_first_join(self):
        cid=bytes(20);driver=Mock();driver.open.return_value=1;received=[0];provider_calls=[]
        class Live:
            def __init__(self,keys,cached):self.keys=keys;self.groups={};self.schemas={};self.schema_packets={};self.counts=collections.Counter()
            def ingest(self,rows):
                if not self.keys or not any(r['payload']==b'seed-schema' for r in rows):return
                self.schemas[FLOW]=Schema.from_packet(fixture_schema());self.schema_packets[FLOW]=fixture_schema();self.groups={(FLOW,d,cid):{'context':{'cid':cid}} for d in ('outgoing','incoming')}
        class Thread:
            def __init__(self,target):self.target=target
            def start(self):
                if self.target.__name__=='scan':self.target()
            def join(self):pass
        kernel=Mock();kernel.GetTickCount64=Mock(return_value=1000);tick=[1.]
        def clock():tick[0]+=.02;return tick[0]
        with tempfile.TemporaryDirectory() as directory:
            folder=pathlib.Path(directory);(folder/'setup-only').touch()
            def receive(handle):
                received[0]+=1
                if json.loads((folder/'state.json').read_text())['ready']:(folder/'stop').touch();return None
                if received[0]>20:raise AssertionError('First-join schema was not recovered from the early buffer')
                return b'late-heartbeat',None
            def provider(connection):
                driver.open.assert_called_once_with(host.GAME_FILTER,True);provider_calls.append(connection)
                return [(b'seed-schema',None),(b'seed-incoming',None)]
            driver.recv.side_effect=receive
            def row(raw,index):return dict(frame=index,payload=raw,connection=FLOW,cid=cid,direction='incoming' if raw==b'seed-incoming' else 'outgoing')
            with patch.object(host.ctypes,'windll',SimpleNamespace(kernel32=kernel)),patch.object(host,'Driver',return_value=driver),patch.object(host,'LiveDecoder',Live),patch.object(host,'candidates',return_value=([bytes(32)],{})),patch.object(host,'ports',return_value={1000}),patch.object(host,'row',side_effect=row),patch.object(host.threading,'Thread',Thread),patch.object(host.network_schema_cache,'identity',return_value=OWNER),patch.object(host.time,'monotonic',side_effect=clock):
                self.assertEqual(host.run(folder,77,0,seconds=5,seed_provider=provider),0,(folder/'state.json').read_text())
            self.assertEqual(provider_calls,[FLOW]);driver.send.assert_not_called();state=json.loads((folder/'state.json').read_text());self.assertEqual(state['stats']['bufferedJoinPackets'],2)

    def test_listener_starts_without_a_pid_and_only_supplies_owned_ciphertext(self):
        for ending in ('stop-file','parent-exit'):
            driver=Mock();driver.open.return_value=1;packets=queue.Queue();dead=[False];result=[]
            driver.recv.side_effect=lambda handle:packets.get(timeout=2)
            driver.shutdown.side_effect=lambda handle:packets.put(None)
            kernel=Mock();kernel.OpenProcess.return_value=123;kernel.GetTickCount64.return_value=1000;kernel.WaitForSingleObject.side_effect=lambda *args:0 if dead[0] else 258
            with tempfile.TemporaryDirectory() as directory:
                folder=pathlib.Path(directory)
                with patch.object(host.ctypes,'windll',SimpleNamespace(kernel32=kernel)),patch.object(host,'Driver',return_value=driver),patch.object(host,'ports',return_value={1000}),patch.object(host,'row',side_effect=lambda raw,index:dict(connection=FLOW if raw==b'owned-ciphertext' else (('other',5000),('server',2000)))),patch.object(host.network_schema_cache,'identity',return_value=OWNER),patch.object(host,'candidates') as scan:
                    thread=threading.Thread(target=lambda:result.append(run_listener(folder,123)));thread.start()
                    deadline=time.monotonic()+1
                    while not (folder/'state.json').exists() and time.monotonic()<deadline:time.sleep(.005)
                    self.assertTrue(json.loads((folder/'state.json').read_text())['listenerReady'])
                    packets.put((b'other-process-ciphertext',None));packets.put((b'owned-ciphertext',None));time.sleep(.03)
                    self.assertEqual(request_seed(folder,77,FLOW,identity=lambda pid:OWNER),[(b'owned-ciphertext',None)])
                    driver.send.assert_not_called();scan.assert_not_called();driver.open.assert_called_once_with(host.GAME_FILTER,True,priority=1200)
                    if ending=='stop-file':(folder/'stop').touch()
                    else:dead[0]=True
                    thread.join(2);self.assertFalse(thread.is_alive());self.assertEqual(result,[0]);driver.close.assert_called_once_with(1)
                state=json.loads((folder/'state.json').read_text());self.assertFalse(state['listenerReady']);self.assertEqual(state['bufferedBytes'],0);self.assertFalse(state['keysWritten']);self.assertFalse(state['packetEdits'])

if __name__=='__main__':unittest.main()
