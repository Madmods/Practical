import collections,json,pathlib,tempfile,unittest
from unittest.mock import patch,Mock
from types import SimpleNamespace
from test_network_wire_lab import fixture_schema
from network_wire_lab import Schema
import qb_packet_host as host

class SetupTests(unittest.TestCase):
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
