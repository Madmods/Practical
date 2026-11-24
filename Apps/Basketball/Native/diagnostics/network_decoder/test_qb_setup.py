import collections,json,pathlib,tempfile,unittest
from unittest.mock import patch,Mock
from types import SimpleNamespace
from test_network_wire_lab import fixture_schema
from network_wire_lab import Schema
import qb_packet_host as host

class SetupTests(unittest.TestCase):
    def test_listener_waits_for_a_late_join_without_scanning_or_timing_out(self):
        connection=(('10.0.0.1',1000),('10.0.0.2',2000));cid=bytes(20);received=[0];tick=[0.];scanned_at=[]
        class Live:
            def __init__(self,keys,cached):self.groups={};self.schemas={};self.schema_packets={};self.counts=collections.Counter()
            def ingest(self,rows):
                self.schemas[connection]=Schema.from_packet(fixture_schema());self.schema_packets[connection]=fixture_schema()
                self.groups={(connection,d,cid):{'context':True} for d in ('outgoing','incoming')}
        class Thread:
            def __init__(self,target):self.target=target
            def start(self):
                if self.target.__name__=='scan':self.target()
            def join(self):pass
        driver=Mock();driver.open.return_value=123;kernel=Mock();kernel.GetTickCount64=Mock(return_value=1000)
        def clock():tick[0]+=3;return tick[0]
        def scan(*args,**kwargs):scanned_at.append(received[0]);return [bytes(32)],{}
        def row(raw,index):
            if received[0]<=50:return None
            return dict(frame=index,payload=b'',connection=connection,cid=cid,direction='outgoing' if index%2 else 'incoming')
        with tempfile.TemporaryDirectory() as folder:
            session=pathlib.Path(folder);(session/'setup-only').touch()
            def receive(handle):
                received[0]+=1
                state=json.loads((session/'state.json').read_text())
                if received[0]<=50:self.assertFalse(scanned_at)
                if state['ready']:(session/'stop').touch();return None
                if received[0]>70:raise AssertionError('Late join did not prepare')
                return b'packet',None
            driver.recv.side_effect=receive
            with patch.object(host.ctypes,'windll',SimpleNamespace(kernel32=kernel)),patch.object(host,'Driver',return_value=driver),patch.object(host,'LiveDecoder',Live),patch.object(host,'candidates',side_effect=scan),patch.object(host,'ports',return_value={1000}),patch.object(host,'row',side_effect=row),patch.object(host.time,'monotonic',side_effect=clock),patch.object(host.threading,'Thread',Thread),patch.object(host.network_schema_cache,'identity',return_value={'pid':10,'created':20}):
                self.assertEqual(host.run(session,10,0),0)
            self.assertGreater(tick[0],120);self.assertTrue(scanned_at);self.assertTrue(all(n>50 for n in scanned_at))
    def test_transport_diagnostic_keeps_both_directions_and_is_bounded(self):
        rows=[dict(frame=i,direction='outgoing' if i<30 else 'incoming',connection=(('local',1),('server',2)),cid=bytes(20),payload=bytes(3000)) for i in range(60)]
        snapshot=host.transport_sample(rows);self.assertEqual(len(snapshot['packets']),24)
        self.assertEqual(sum(r['direction']=='incoming' for r in snapshot['packets']),12)
        self.assertTrue(all(len(r['payload'])==4096 and r['truncated'] for r in snapshot['packets']));self.assertFalse(snapshot['keysWritten'])
    def test_prepared_editor_does_not_require_a_calibration_throw(self):
        e=SimpleNamespace(recovering=False,format_verified=False)
        message,ready=host.control_status(e);self.assertTrue(ready);self.assertEqual(message,'QB throw control ready')
        e.format_verified=True;self.assertTrue(host.control_status(e)[1])
        e.recovering=True;self.assertFalse(host.control_status(e)[1])
    def test_active_host_assists_the_first_user_throw(self):
        from test_qb_packet_editor import payload,plan
        connection=(('10.0.0.1',1000),('10.0.0.2',2000));cid=bytes(20);warm_stopped=[False];active_reads=[0];warm_reads=[0]
        class Live:
            def __init__(self,keys,cached):self.groups={};self.schemas={};self.schema_packets={};self.counts=collections.Counter()
            def ingest(self,rows):
                self.schemas[connection]=Schema.from_packet(fixture_schema());self.schema_packets[connection]=fixture_schema()
                self.groups={(connection,d,cid):{'context':{'hp':bytes(32),'key':bytes(32),'largest':0}} for d in ('incoming','outgoing')}
        class Thread:
            def __init__(self,target):self.target=target
            def start(self):
                if self.target.__name__=='scan':self.target()
            def join(self):pass
        driver=Mock();driver.open.side_effect=lambda filter,sniff=False:1 if sniff else 2
        driver.shutdown.side_effect=lambda handle:warm_stopped.__setitem__(0,True) if handle==1 else None
        def receive(handle):
            if handle==1:
                if warm_stopped[0]:return None
                warm_reads[0]+=1
                if warm_reads[0]>50:raise AssertionError('Fixture did not finish setup')
                return b'warm',None
            active_reads[0]+=1
            if active_reads[0]>1:return None
            state=json.loads((session/'state.json').read_text())
            self.assertTrue(state['ready']);self.assertEqual(state['status'],'QB throw control ready')
            return b'throw',None
        driver.recv.side_effect=receive
        def row(raw,index):return dict(frame=index,payload=raw,udp_offset=0,connection=connection,cid=cid,direction='outgoing' if raw==b'throw' or index%2 else 'incoming')
        tick=[0.]
        def clock():tick[0]+=.1;return tick[0]
        kernel=Mock();kernel.GetTickCount64.return_value=1010
        with tempfile.TemporaryDirectory() as folder:
            session=pathlib.Path(folder)/'session';session.mkdir();(session/'plan.json').write_text(json.dumps({**plan(),'pid':10}),encoding='utf-8')
            with patch.object(host.ctypes,'windll',SimpleNamespace(kernel32=kernel)),patch.object(host,'Driver',return_value=driver),patch.object(host,'LiveDecoder',Live),patch.object(host,'candidates',return_value=([bytes(32)],{})),patch.object(host,'ports',return_value={1000}),patch.object(host,'row',side_effect=row),patch.object(host,'authenticated_plain',side_effect=lambda raw,ctx:(SimpleNamespace(packet_number=1),None,payload() if raw==b'throw' else b'')),patch.object(host,'unprotect',side_effect=host.DecodeError('fixture')),patch.object(host,'encrypt_changed',side_effect=lambda raw,u,nonce,changed,ctx:changed),patch.object(host.network_schema_cache,'identity',return_value={'pid':10,'created':20}),patch.object(host.time,'monotonic',side_effect=clock),patch.object(host.threading,'Thread',Thread):
                self.assertEqual(host.run(session,10,0),0,(session/'state.json').read_text())
            driver.send.assert_called_once()
            self.assertNotEqual(driver.send.call_args.args[1],payload());self.assertTrue(driver.send.call_args.args[3])
            state=json.loads((session/'state.json').read_text());self.assertEqual(state['stats']['modifiedThrows'],1)
    def test_scan_can_resume_where_previous_round_stopped(self):
        import threading
        result={}
        with patch.object(host,'candidates',return_value=([],dict(bytesRead=200,limitReached=0,nextAddress=2345,error=0))) as scan:
            host.scan_incrementally(10,[],result,threading.Event(),pathlib.Path('unused-stop-marker'),start_address=1234)
        self.assertEqual(scan.call_args.kwargs['start_address'],1234)
        self.assertEqual(result['bytesRead'],200)

    def test_failure_distinguishes_keys_authentication_and_schema(self):
        connection=(('10.0.0.1',1000),('10.0.0.2',2000));cid=bytes(20)
        rows=[dict(connection=connection,direction=d,cid=cid) for d in ('outgoing','incoming')]
        live=SimpleNamespace(groups={},schemas={})
        self.assertIn('No supported traffic',host.setup_failure(live,rows,set(),0,{}))
        self.assertIn('No supported transport keys',host.setup_failure(live,rows,{1000},0,{}))
        self.assertIn('authentication failed',host.setup_failure(live,rows,{1000},3,{}))
        live.groups={(connection,d,cid):{'context':True} for d in ('outgoing','incoming')}
        self.assertIn('Join schema missing',host.setup_failure(live,rows,{1000},3,{}))
        self.assertIn('cannot read',host.setup_failure(live,rows,{1000},3,{'error':5}))

    def test_incremental_scan_exposes_keys_and_keeps_total_budget(self):
        import threading
        keys=[];result={};cancel=threading.Event();calls=[]
        def scan(pid,ms,**kwargs):
            calls.append(kwargs)
            if len(calls)==1:return [b'a'*32],dict(bytesRead=100,limitReached=1,nextAddress=1234,error=0)
            self.assertEqual(keys,[b'a'*32]);self.assertEqual(kwargs['start_address'],1234);self.assertEqual(kwargs['max_bytes'],1024*1024*1024-100)
            return [b'b'*32],dict(bytesRead=200,limitReached=0,nextAddress=2345,error=0)
        with tempfile.TemporaryDirectory() as folder,patch.object(host,'candidates',side_effect=scan):
            host.scan_incrementally(10,keys,result,cancel,pathlib.Path(folder)/'stop')
        self.assertEqual(len(keys),2);self.assertEqual(result['bytesRead'],300)
        cancel.set()
        with patch.object(host,'candidates') as scan:host.scan_incrementally(10,[],{},cancel,pathlib.Path('unused-stop-marker'));scan.assert_not_called()
    def test_continue_reuses_prepared_helper_without_a_second_scan(self):
        connection=(('10.0.0.1',1000),('10.0.0.2',2000));cid=bytes(20)
        class Live:
            def __init__(self,keys,cached):self.groups={};self.schemas={};self.schema_packets={};self.counts=collections.Counter()
            def ingest(self,rows):
                self.schemas[connection]=Schema.from_packet(fixture_schema());self.schema_packets[connection]=fixture_schema()
                self.groups={(connection,d,cid):{'context':{'hp':bytes(32),'key':bytes(32),'largest':0}} for d in ('incoming','outgoing')}
        class Thread:
            def __init__(self,target):self.target=target
            def start(self):
                if self.target.__name__=='scan':self.target()
            def join(self):pass
        for cancel,resume,delay in ((False,True,0),(True,True,0),(False,False,600),(True,False,0)):
            with tempfile.TemporaryDirectory() as folder:
                session=pathlib.Path(folder)/'session';session.mkdir();(session/'setup-only').touch();continued=[False];opened=[];halt=[False];ready_packets=[0]
                if resume:(session/'resume-after-join').touch()
                def open_handle(filter,sniff=False):opened.append(sniff);return 1 if sniff else 2
                def receive(handle):
                    if halt[0] or handle==2:return None
                    state=json.loads((session/'state.json').read_text())
                    if state['ready']:
                        self.assertEqual(state['phase'],'ready');self.assertFalse(state['requiresRejoin']);ready_packets[0]+=1
                        if ready_packets[0]<=delay:return b'packet',None
                        self.assertEqual(opened,[True]);continued[0]=True
                        if cancel:(session/'stop').touch()
                        else:(session/'setup-only').unlink()
                    return b'packet',None
                driver=Mock();driver.open.side_effect=open_handle;driver.recv.side_effect=receive;driver.shutdown.side_effect=lambda h:halt.__setitem__(0,True)
                tick=[0.]
                def clock():tick[0]+=.5;return tick[0]
                kernel=Mock();kernel.GetTickCount64=Mock(return_value=1000)
                with patch.object(host.ctypes,'windll',SimpleNamespace(kernel32=kernel)),patch.object(host,'Driver',return_value=driver),patch.object(host,'LiveDecoder',Live),patch.object(host,'candidates',return_value=([bytes(32)],{})) as scan,patch.object(host,'ports',return_value={1000}),patch.object(host,'row',side_effect=lambda raw,index:dict(frame=index,payload=b'',connection=connection,cid=cid,direction='outgoing' if index%2 else 'incoming')),patch.object(host,'authenticated_plain',return_value=(None,None,b'')),patch.object(host,'unprotect',side_effect=host.DecodeError('fixture')),patch.object(host.network_schema_cache,'identity',return_value={'pid':10,'created':20}),patch.object(host.time,'monotonic',side_effect=clock),patch.object(host.threading,'Thread',Thread):
                    self.assertEqual(host.run(session,10,0),0,(session/'state.json').read_text())
                self.assertTrue(continued[0]);self.assertEqual(scan.call_count,1);self.assertEqual(opened,[True] if cancel else [True,False])
    def test_setup_status_does_not_request_rejoin_for_missing_current_keys(self):
        c=(('10.0.0.1',1000),('10.0.0.2',2000));cid=bytes(20)
        rows=[dict(connection=c,direction=d,cid=cid) for d in ('outgoing','incoming')]
        live=SimpleNamespace(schemas={c:True},groups={})
        self.assertIn('Preparing keys',host.setup_status(live,rows,{1000}));self.assertNotEqual(host.setup_phase(live,rows,{1000}),'rejoin')
        live.groups={(c,d,cid):{'context':True} for d in ('outgoing','incoming')}
        live.schemas.clear();self.assertEqual(host.setup_phase(live,rows,{1000}),'rejoin');self.assertIn('without holding a football',host.setup_status(live,rows,{1000}))
    def test_new_socket_gets_fresh_retries_after_old_socket_exhausts_them(self):
        old=(('10.0.0.1',1000),('10.0.0.2',2000));new=(('10.0.0.1',3000),('10.0.0.3',4000));cid=bytes(20);scans=[];joined=[False];created=[]
        class Live:
            def __init__(self,keys,cached):
                self.keys=keys;self.groups={};self.schemas={};self.schema_packets={};self.counts=collections.Counter();created.append(self)
            def ingest(self,rows):
                if len(self.keys)<4:return
                self.assert_current=all(r['connection']==new for r in rows)
                self.schemas[new]=Schema.from_packet(fixture_schema());self.schema_packets[new]=fixture_schema()
                self.groups={(new,d,cid):{'context':{'cid':cid}} for d in ('incoming','outgoing')}
        class Thread:
            def __init__(self,target):self.target=target
            def start(self):
                if self.target.__name__=='scan':self.target()
            def join(self):pass
        def scan(*args,**kwargs):scans.append(len(scans)+1);return [bytes([len(scans)])*32],{}
        def row(raw,index):
            if len(scans)>=3:joined[0]=True
            return dict(frame=index,payload=b'',connection=new if joined[0] else old,cid=cid,direction='outgoing' if index%2 else 'incoming')
        tick=[0.]
        def clock():tick[0]+=.2;return tick[0]
        driver=Mock();driver.open.return_value=123
        prepared=[]
        def receive(handle):
            state=json.loads((session/'state.json').read_text())
            if state['ready']:prepared.append(state);(session/'stop').touch()
            return b'packet',None
        driver.recv.side_effect=receive
        kernel=Mock();kernel.GetTickCount64=Mock(return_value=1000)
        with tempfile.TemporaryDirectory() as folder:
            session=pathlib.Path(folder)/'session';session.mkdir();(session/'setup-only').touch()
            with patch.object(host.ctypes,'windll',SimpleNamespace(kernel32=kernel)),patch.object(host,'Driver',return_value=driver),patch.object(host,'LiveDecoder',Live),patch.object(host,'candidates',side_effect=scan),patch.object(host,'ports',side_effect=lambda pid:{3000} if joined[0] else {1000}),patch.object(host,'row',side_effect=row),patch.object(host.time,'monotonic',side_effect=clock),patch.object(host.threading,'Thread',Thread),patch.object(host.network_schema_cache,'identity',return_value={'pid':10,'created':20}):
                self.assertEqual(host.run(session,10,0),0,(session/'state.json').read_text())
            self.assertEqual(len(scans),4);self.assertEqual(len(created),2);self.assertTrue(created[-1].assert_current)
            driver.open.assert_called_once_with(host.GAME_FILTER,True)
    def test_idle_active_connection_is_closed(self):
        import threading,time
        connection=(('10.0.0.1',1000),('10.0.0.2',2000));cid=bytes(20)
        class Live:
            def __init__(self,keys,cached):
                self.groups={};self.schemas={};self.schema_packets={};self.counts=collections.Counter()
            def ingest(self,rows):
                self.schemas[connection]=Schema.from_packet(fixture_schema());self.schema_packets[connection]=fixture_schema()
                self.groups={(connection,d,cid):{'context':{'hp':bytes(32),'key':bytes(32),'largest':0}} for d in ('incoming','outgoing')}
        class Driver:
            def __init__(self):self.warm_stopped=False;self.closed=[];self.halt=threading.Event()
            def open(self,filter,sniff=False):return 1 if sniff else 2
            def recv(self,handle):
                if handle==1:
                    if self.warm_stopped:return None
                    time.sleep(.01);return b'packet',None
                if not self.halt.wait(5):raise AssertionError('Idle connection was not shut down')
                return None
            def shutdown(self,handle):
                if handle==1:self.warm_stopped=True
                else:self.halt.set()
            def close(self,handle):self.closed.append(handle)
        driver=Driver();kernel=Mock();kernel.GetTickCount64=Mock(return_value=1000)
        with tempfile.TemporaryDirectory() as folder:
            session=pathlib.Path(folder)/'session'
            with patch.object(host.ctypes,'windll',SimpleNamespace(kernel32=kernel)),patch.object(host,'Driver',return_value=driver),patch.object(host,'LiveDecoder',Live),patch.object(host,'candidates',return_value=([bytes(32)],{})),patch.object(host,'ports',return_value={1000}),patch.object(host,'row',side_effect=lambda raw,index:dict(frame=index,payload=b'',connection=connection,cid=cid,direction='outgoing' if index%2 else 'incoming')),patch.object(host,'authenticated_plain',return_value=(None,None,b'')),patch.object(host,'unprotect',side_effect=host.DecodeError('fixture')),patch.object(host.network_schema_cache,'identity',return_value={'pid':10,'created':20}):
                self.assertEqual(host.run(session,10,0),0,(session/'state.json').read_text())
            state=json.loads((session/'state.json').read_text())
            self.assertEqual(state['capture'],'Stopped');self.assertFalse(state['ready']);self.assertIn('traffic stopped',state['error']);self.assertIn(2,driver.closed)
    def test_closed_old_socket_cannot_win_over_rejoined_connection(self):
        old=(('10.0.0.1',55109),('10.0.0.2',2000));new=(('10.0.0.1',49879),('10.0.0.2',2000));cid=bytes(20)
        live=SimpleNamespace(schemas={old:True,new:True},groups={(c,d,cid):{'context':{'flow':c}} for c in (old,new) for d in ('outgoing','incoming')})
        rows=[dict(connection=c,direction=d,cid=cid) for c in (old,new) for d in ('outgoing','incoming')]
        self.assertEqual(host.current_flow(live,rows,{49879})[0],new)
        self.assertIsNone(host.current_flow(live,rows,set()))
        live.groups[(new,'incoming',cid)]['context']=None
        self.assertIsNone(host.current_flow(live,rows,{55109,49879}))
    def test_latest_connection_id_must_be_authenticated(self):
        flow=(('10.0.0.1',49879),('10.0.0.2',2000));old=bytes(20);new=bytes([1])*20
        live=SimpleNamespace(schemas={flow:True},groups={(flow,d,old):{'context':True} for d in ('incoming','outgoing')})
        rows=[dict(connection=flow,direction=d,cid=old) for d in ('incoming','outgoing')]
        rows.append(dict(connection=flow,direction='outgoing',cid=new))
        self.assertIsNone(host.current_flow(live,rows,{49879}))
    def test_setup_and_active_handles_use_distinct_priorities(self):
        driver=host.Driver.__new__(host.Driver);driver.dll=Mock();driver.dll.WinDivertOpen.return_value=123
        driver.open('ip',True);driver.open('ip',False)
        calls=driver.dll.WinDivertOpen.call_args_list
        self.assertNotEqual(calls[0].args[2],calls[1].args[2])
        self.assertEqual(driver.dll.WinDivertSetParam.call_count,3)
        self.assertEqual([c.args[1:] for c in driver.dll.WinDivertSetParam.call_args_list],[(0,16384),(1,16000),(2,33554432)])
    def test_startup_listener_setup_and_editor_do_not_compete_at_one_priority(self):
        driver=host.Driver.__new__(host.Driver);driver.dll=Mock();driver.dll.WinDivertOpen.return_value=123
        driver.open('ip',True,priority=1200);driver.open('ip',True);driver.open('ip',False)
        self.assertEqual([call.args[2] for call in driver.dll.WinDivertOpen.call_args_list],[1200,1100,1000])
        self.assertEqual([call.args[3] for call in driver.dll.WinDivertOpen.call_args_list],[5,5,0])
    def test_setup_buffer_failure_closes_the_read_only_handle(self):
        driver=host.Driver.__new__(host.Driver);driver.dll=Mock();driver.dll.WinDivertOpen.return_value=123;driver.dll.WinDivertSetParam.return_value=0
        with self.assertRaises(OSError):driver.open('ip',True)
        driver.dll.WinDivertClose.assert_called_once_with(123)
    def test_rejoin_rescans_and_saves_schema_without_enabling_interception(self):
        connection=(('10.0.0.1',1000),('10.0.0.2',2000));cid=bytes(20)
        class Live:
            def __init__(self,keys,cached):
                self.keys=keys;self.groups={};self.schemas={};self.schema_packets={};self.counts=collections.Counter(authenticatedPackets=1)
            def ingest(self,rows):
                if len(self.keys)<2:return
                self.schemas[connection]=Schema.from_packet(fixture_schema());self.schema_packets[connection]=fixture_schema()
                self.groups={(connection,d,cid):{'context':{'cid':cid}} for d in ('incoming','outgoing')}
        driver=Mock();driver.open.return_value=123
        prepared=[]
        def receive(handle):
            state=json.loads((session/'state.json').read_text())
            if state['ready']:prepared.append(state);(session/'stop').touch()
            return b'packet',None
        driver.recv.side_effect=receive
        class Thread:
            def __init__(self,target):self.target=target
            def start(self):
                if self.target.__name__=='scan':self.target()
            def join(self):pass
        tick=[0.]
        def clock():tick[0]+=.2;return tick[0]
        kernel=Mock();kernel.GetTickCount64=Mock(return_value=1000)
        owner={'pid':10,'created':20}
        with tempfile.TemporaryDirectory() as folder:
            session=pathlib.Path(folder)/'session';session.mkdir();(session/'setup-only').touch()
            with patch.object(host.ctypes,'windll',SimpleNamespace(kernel32=kernel)),patch.object(host,'Driver',return_value=driver),patch.object(host,'LiveDecoder',Live),patch.object(host,'candidates',side_effect=[([b'a'*32],{}),([b'b'*32],{})]) as scan,patch.object(host,'ports',return_value={1000}),patch.object(host,'row',side_effect=lambda raw,index:dict(frame=index,payload=b'',connection=connection,cid=cid,direction='outgoing' if index%2 else 'incoming')),patch.object(host.time,'monotonic',side_effect=clock),patch.object(host.threading,'Thread',Thread),patch.object(host.network_schema_cache,'identity',return_value=owner):
                self.assertEqual(host.run(session,10,0),0,(session/'state.json').read_text())
            self.assertEqual(scan.call_count,2)
            driver.open.assert_called_once_with(host.GAME_FILTER,True)
            self.assertEqual(len(host.network_schema_cache.load(pathlib.Path(folder)/'schema-cache.json',owner)),1)
            state=json.loads((session/'state.json').read_text());self.assertFalse(state['ready']);self.assertTrue(prepared);self.assertEqual(prepared[-1]['phase'],'ready');self.assertIn('QBA is off',prepared[-1]['status'])
            self.assertNotIn('aaaaaaaa',json.dumps(state))

if __name__=='__main__':unittest.main()
