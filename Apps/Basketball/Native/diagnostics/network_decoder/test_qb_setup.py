import collections,json,pathlib,tempfile,unittest
from unittest.mock import patch,Mock
from types import SimpleNamespace
from test_network_wire_lab import fixture_schema
from network_wire_lab import Schema
import qb_packet_host as host

class SetupTests(unittest.TestCase):
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
            with patch.object(host.ctypes,'windll',SimpleNamespace(kernel32=kernel)),patch.object(host,'Driver',return_value=driver),patch.object(host,'LiveDecoder',Live),patch.object(host,'candidates',side_effect=[([b'a'*32],{}),([b'b'*32],{})]) as scan,patch.object(host,'ports',return_value={1000}),patch.object(host,'row',side_effect=lambda raw,index:dict(frame=index,connection=connection,cid=cid)),patch.object(host.time,'monotonic',side_effect=clock),patch.object(host.threading,'Thread',Thread),patch.object(host.network_schema_cache,'identity',return_value=owner):
                self.assertEqual(host.run(session,10,0),0)
            self.assertEqual(scan.call_count,2)
            driver.open.assert_called_once_with(host.GAME_FILTER,True)
            self.assertEqual(len(host.network_schema_cache.load(pathlib.Path(folder)/'schema-cache.json',owner)),1)
            state=json.loads((session/'state.json').read_text());self.assertFalse(state['ready']);self.assertIn('setup saved',state['status'])
            self.assertNotIn('aaaaaaaa',json.dumps(state))

if __name__=='__main__':unittest.main()
