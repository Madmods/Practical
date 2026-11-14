import tempfile,pathlib,json,unittest
from live_network_decoder import LiveDecoder,record_alignment
from network_decoder_host import publish
from test_network_wire_lab import fixture_schema,v,s

class Tests(unittest.TestCase):
    def test_midstream_alignment_requires_three_records(self):
        from network_wire_lab import Schema
        schema=Schema.from_packet(fixture_schema());body=b'\x83\x07\x01'+(123).to_bytes(4,'little')+b'\0\0'+v(1)+b'\x02'+s('Clicked');framed=(0x80000000+len(body)).to_bytes(4,'big')+body
        self.assertIsNone(record_alignment(framed*2,schema,list(range(256)),'outgoing'))
        self.assertEqual(record_alignment(b'junk'+framed*3,schema,list(range(256)),'outgoing'),4)
        self.assertIsNone(record_alignment(framed*3,schema,list(range(256)),'incoming'))
    def test_schema_cache_rejects_process_reuse_and_expiry(self):
        import network_schema_cache as cache
        from types import SimpleNamespace
        connection=(('10.0.0.1',1),('10.0.0.2',2));owner={'pid':123,'created':456}
        decoder=SimpleNamespace(schema_packets={connection:fixture_schema()},groups={(connection,'incoming',bytes(20)):{'context':True}})
        with tempfile.TemporaryDirectory() as folder:
            path=pathlib.Path(folder)/'cache.json';cache.save(path,owner,decoder)
            self.assertEqual(len(cache.load(path,owner)),1);self.assertFalse(cache.load(path,{'pid':123,'created':457}))
            root=json.loads(path.read_text());root['saved']-=1801;path.write_text(json.dumps(root));self.assertFalse(cache.load(path,owner))
    def test_schema_then_direction_mapped_event(self):
        decoder=LiveDecoder([]);connection=(('127.0.0.1',1),('127.0.0.2',2))
        decoder.application(connection,'incoming',b'\x97'+fixture_schema(),0)
        # This synthetic schema uses a literal FireServer member.
        event=b'\x83\x07\x01'+(123).to_bytes(4,'little')+b'\0\0'+v(1)+b'\x02'+s('Clicked')
        from network_wire_lab import Wire
        decoder.wires[(connection,'outgoing','3')]=Wire(decoder.schemas[connection])
        decoder.application(connection,'outgoing',event,0)
        self.assertEqual(decoder.counts['clickedCalls'],1);self.assertEqual(decoder.calls[0]['args'],['Clicked'])
    def test_connection_schema_isolation(self):
        decoder=LiveDecoder([]);decoder.application('one','incoming',b'\x97'+fixture_schema(),0)
        decoder.application('two','outgoing',b'\x83\x07\0',0)
        self.assertEqual(len(decoder.calls),0);self.assertEqual(len(decoder.pending['two']),1)
    def test_bad_schema_does_not_enable_calls(self):
        decoder=LiveDecoder([]);decoder.application('one','incoming',b'\x97'+fixture_schema()[:-1],0)
        self.assertFalse(decoder.schemas)
    def test_state_transport_never_exports_keys(self):
        decoder=LiveDecoder([b'secret'*5+b'xx']);state=decoder.state('Stopped')
        self.assertNotIn('secret',json.dumps(state));self.assertFalse(state['keysWritten'])
    def test_ipc_message_budget(self):
        with tempfile.TemporaryDirectory() as folder:
            path=pathlib.Path(folder)/'state.json';calls=[{'args':['x'*8192]} for _ in range(100)]
            publish(path,{'calls':calls,'capture':'Stopped'})
            self.assertLessEqual(path.stat().st_size,60000);self.assertGreater(len(json.loads(path.read_text())['calls']),0)
            self.assertEqual(len(calls),100)
    def test_clicked_call_survives_ipc_trimming(self):
        with tempfile.TemporaryDirectory() as folder:
            path=pathlib.Path(folder)/'state.json';clicked={'method':'FireServer','args':['Clicked',60]}
            publish(path,{'calls':[clicked]+[{'method':'OnClientEvent','args':['x'*8192]} for _ in range(100)]})
            self.assertEqual(json.loads(path.read_text())['calls'][0],clicked)

if __name__=='__main__':unittest.main(verbosity=2)
