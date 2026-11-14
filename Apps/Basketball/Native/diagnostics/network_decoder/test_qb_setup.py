import collections,json,pathlib,tempfile,unittest
from unittest.mock import patch,Mock
from types import SimpleNamespace
from test_network_wire_lab import fixture_schema
from network_wire_lab import Schema
import qb_packet_host as host

class SetupTests(unittest.TestCase):
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
        for cancel in (False,True):
            with tempfile.TemporaryDirectory() as folder:
                session=pathlib.Path(folder)/'session';session.mkdir();(session/'setup-only').touch();(session/'resume-after-join').touch();continued=[False];opened=[];halt=[False]
                def open_handle(filter,sniff=False):opened.append(sniff);return 1 if sniff else 2
                def receive(handle):
                    if halt[0] or handle==2:return None
                    state=json.loads((session/'state.json').read_text())
                    if state['status'].startswith('Setup ready.'):
                        self.assertEqual(opened,[True]);continued[0]=True
                        if cancel:(session/'stop').touch()
                        else:(session/'setup-only').unlink()
                    return b'packet',None
                driver=Mock();driver.open.side_effect=open_handle;driver.recv.side_effect=receive;driver.shutdown.side_effect=lambda h:halt.__setitem__(0,True)
                tick=[0.]
                def clock():tick[0]+=.05;return tick[0]
                kernel=Mock();kernel.GetTickCount64=Mock(return_value=1000)
                with patch.object(host.ctypes,'windll',SimpleNamespace(kernel32=kernel)),patch.object(host,'Driver',return_value=driver),patch.object(host,'LiveDecoder',Live),patch.object(host,'candidates',return_value=([bytes(32)],{})) as scan,patch.object(host,'ports',return_value={1000}),patch.object(host,'row',side_effect=lambda raw,index:dict(frame=index,payload=b'',connection=connection,cid=cid,direction='outgoing' if index%2 else 'incoming')),patch.object(host,'authenticated_plain',return_value=(None,None,b'')),patch.object(host,'unprotect',side_effect=host.DecodeError('fixture')),patch.object(host.network_schema_cache,'identity',return_value={'pid':10,'created':20}),patch.object(host.time,'monotonic',side_effect=clock),patch.object(host.threading,'Thread',Thread):
                    self.assertEqual(host.run(session,10,0),0,(session/'state.json').read_text())
                self.assertTrue(continued[0]);self.assertEqual(scan.call_count,1);self.assertEqual(opened,[True] if cancel else [True,False])
    def test_setup_status_does_not_request_rejoin_for_missing_current_keys(self):
        c=(('10.0.0.1',1000),('10.0.0.2',2000));cid=bytes(20)
        rows=[dict(connection=c,direction=d,cid=cid) for d in ('outgoing','incoming')]
        live=SimpleNamespace(schemas={c:True},groups={})
        self.assertIn('Preparing keys',host.setup_status(live,rows,{1000}))
        live.groups={(c,d,cid):{'context':True} for d in ('outgoing','incoming')}
        live.schemas.clear();self.assertIn('Rejoin once',host.setup_status(live,rows,{1000}))
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
        driver=Mock();driver.recv.return_value=(b'packet',None);driver.open.return_value=123
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
    def test_rejoin_rescans_and_saves_schema_without_enabling_interception(self):
        connection=(('10.0.0.1',1000),('10.0.0.2',2000));cid=bytes(20)
        class Live:
            def __init__(self,keys,cached):
                self.keys=keys;self.groups={};self.schemas={};self.schema_packets={};self.counts=collections.Counter(authenticatedPackets=1)
            def ingest(self,rows):
                if len(self.keys)<2:return
                self.schemas[connection]=Schema.from_packet(fixture_schema());self.schema_packets[connection]=fixture_schema()
                self.groups={(connection,d,cid):{'context':{'cid':cid}} for d in ('incoming','outgoing')}
        driver=Mock();driver.recv.return_value=(b'packet',None);driver.open.return_value=123
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
            state=json.loads((session/'state.json').read_text());self.assertFalse(state['ready']);self.assertIn('setup saved',state['status'])
            self.assertNotIn('aaaaaaaa',json.dumps(state))

if __name__=='__main__':unittest.main()
