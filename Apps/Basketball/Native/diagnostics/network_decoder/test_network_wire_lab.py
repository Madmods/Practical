import struct,unittest
from network_wire_lab import *
def v(n):
    out=bytearray()
    while n>=128:out.append((n&127)|128);n>>=7
    return bytes(out+bytes([n]))
def s(t):b=t.encode();return v(len(b))+b
def fixture_schema():
    raw=b'\x03'+bytes(8)+b'\0'+v(1)+v(0)+v(1)+s('RemoteEvent')+b'\0\0\0'+v(1)+s('FireServer')+v(1)+b'\x1d\0\0'+b'\0\0'
    compressed=zstandard.ZstdCompressor().compress(raw);return len(compressed).to_bytes(4,'little')+len(raw).to_bytes(4,'little')+compressed
class Tests(unittest.TestCase):
    def combined_event(self):
        schema=Schema.from_packet(fixture_schema());schema.properties=[dict(name='CFrame',className='Part',kind=27)]
        body=b'\x83\x23'+bytes(4)+b'\x0a\x01'+(123).to_bytes(4,'little')+bytes(2)+b'\x01'+v(658)+bytes(4)+b'\x07\x01'+(456).to_bytes(4,'little')+bytes(2)+v(1)+b'\x02'+s('Clicked')+b'\0'
        return schema,body
    def test_combined_acknowledgements_before_event(self):
        schema,body=self.combined_event();calls=Wire(schema).events(body,'outgoing')
        self.assertEqual(len(calls),1);self.assertEqual(calls[0]['args'],['Clicked']);self.assertEqual(calls[0]['consumed'],len(body)-1)
    def test_unknown_prefix_is_not_scanned_for_events(self):
        schema,body=self.combined_event();self.assertEqual(Wire(schema).events(b'\x83\xfe'+body[1:],'outgoing'),[])
    def test_acknowledgement_requires_matching_property_schema(self):
        schema,body=self.combined_event();schema.properties[0]['className']='Camera'
        with self.assertRaisesRegex(DecodeError,'acknowledgement schema'):Wire(schema).events(body,'outgoing')
    def test_schema(self):self.assertEqual(Schema.from_packet(fixture_schema()).events[0]['name'],'FireServer')
    def test_schema_truncated(self):
        with self.assertRaises((DecodeError,zstandard.ZstdError)):Schema.from_packet(fixture_schema()[:-1])
    def test_schema_trailing(self):
        with self.assertRaises(DecodeError):Schema.from_packet(fixture_schema()+b'X')
    def test_schema_expanded_length(self):
        data=bytearray(fixture_schema());data[4:8]=(1).to_bytes(4,'little')
        with self.assertRaisesRegex(DecodeError,'expanded length'):Schema.from_packet(data)
    def test_schema_v3_extension_retained(self):
        data=fixture_schema();raw=zstandard.ZstdDecompressor().decompress(data[8:])+bytes(4)+bytes.fromhex('0203010401070200')
        compressed=zstandard.ZstdCompressor().compress(raw);schema=Schema.from_packet(len(compressed).to_bytes(4,'little')+len(raw).to_bytes(4,'little')+compressed)
        self.assertEqual(schema.opaque_extension,bytes.fromhex('0203010401070200'));self.assertEqual(schema.events[0]['name'],'FireServer')
    def test_event(self):
        wire=Wire(Schema.from_packet(fixture_schema()));body=b'\x83\x07\x01'+(123).to_bytes(4,'little')+b'\0\0'+v(3)+b'\x02'+s('Clicked')+b'\x16'+struct.pack('<fff',1,2,3)+b'\x0c'+struct.pack('<d',95)
        event=wire.leading_event(body,'outgoing');self.assertEqual(event['args'][0],'Clicked');self.assertEqual(event['args'][2],95);self.assertEqual(event['consumed'],len(body));self.assertFalse(event['pathResolved'])
    def test_wrong_direction(self):
        with self.assertRaises(DecodeError):Wire(Schema.from_packet(fixture_schema())).leading_event(b'\x83\x07\0\0\0\0','incoming')
    def test_missing_schema(self):
        with self.assertRaises(DecodeError):Wire().leading_event(b'\x83\x07\0\0\0\0','outgoing')
    def test_nonfinite_rejected(self):
        with self.assertRaises(DecodeError):Wire().typed(Reader(struct.pack('<d',float('nan'))),12)
    def test_unknown_type_rejected(self):
        with self.assertRaises(DecodeError):Wire().typed(Reader(b''),255)
    def test_cached_string_requires_context(self):
        with self.assertRaises(DecodeError):Wire().typed(Reader(b'\x04'),1)
    def test_dictionary(self):
        result=Wire().typed(Reader(v(1)+s('tool')+b'\x02'+s('Football')),31);self.assertEqual(result,{'tool':'Football'})
    def test_duplicate_key_rejected(self):
        with self.assertRaises(DecodeError):Wire().typed(Reader(v(2)+(s('a')+b'\x09\x01')*2),31)
    def test_oversized_container(self):
        with self.assertRaises(DecodeError):Wire().typed(Reader(v(129)),29)
if __name__=='__main__':unittest.main(verbosity=2)
