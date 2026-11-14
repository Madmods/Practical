import unittest,struct
from cryptography.hazmat.primitives.ciphers.aead import AESGCM
from network_wire_lab import Schema,Wire
from network_transport_lab import frames,decrypt,ecb,nonce_for
from test_network_wire_lab import fixture_schema,v,s
from qb_packet_editor import *

def qv(n):
    for count,prefix in ((1,0),(2,0x4000),(4,0x80000000)):
        if n<1<<(count*8-2):return (n|prefix).to_bytes(count,'big')
    raise ValueError(n)
def stream(data,offset=0):return b'\x0e'+qv(6)+qv(offset)+qv(len(data))+data
def body():
    op=MAPS['3'].index(7)
    return b'\x83'+bytes([op])+b'\x01'+(123).to_bytes(4,'little')+bytes(2)+v(4)+b'\x02'+s('Clicked')+b'\x16'+struct.pack('<fff',0,5,0)+b'\x16'+struct.pack('<fff',10000,5,0)+b'\x0c'+struct.pack('<d',60)+b'\0'
def record():b=body();return (0x80000000+len(b)).to_bytes(4,'big')+b
def plan():return dict(mode='ff3-qb',active=True,issued=1000,expires=1150,clickAt=990,head=[0,5,0],targetPosition=[70,5,0],targetVelocity=[0,0,0],gravity=28,latency=.05,lob=False,autoPower=True)
def editor():return Editor(Schema.from_packet(fixture_schema()))
def payload():return stream(bytes.fromhex('06010400000001')+record())
def arguments(plain):
    data=frames(plain)[0].data;b=data[11:];return Wire(Schema.from_packet(fixture_schema()),MAPS['3']).events(b,'outgoing')[0]['args']

class Tests(unittest.TestCase):
    def test_angle_override_recalculates_power_from_actual_origin(self):
        p=plan();p.update(anglePower=True,angle=35)
        after=editor().process_plain(payload(),p,1010);args=arguments(after)
        self.assertLess(args[3],95);self.assertGreater(args[3],1)
        aim=args[2]['components'];self.assertAlmostEqual(math.degrees(math.atan2(aim[1]-5,math.hypot(aim[0],aim[2]))),35,places=4)
    def test_angle_solution_lands_on_receiver_and_stays_in_bounds(self):
        for angle in (12,35,55,75):
            launch,t,power=angle_velocity([0,5,0],[50,5,0],[4,0,2],angle,28,.05)
            actual=[launch[i]*t+(5-.5*28*t*t if i==1 else 0) for i in range(3)]
            expected=[50+4*(t+.05),5,2*(t+.05)]
            self.assertLess(length(tuple(a-b for a,b in zip(actual,expected))),1e-6)
            self.assertLessEqual(power,95)
    def test_unreachable_angle_does_not_change_throw(self):
        for angle in (float('nan'),-1,90,5):
            p=plan();p.update(anglePower=True,angle=angle,targetPosition=[340,5,0]);self.assertEqual(editor().process_plain(payload(),p,1010),payload())
    def test_power_and_aim_edit_preserves_origin_and_length(self):
        before=payload();e=editor();after=e.process_plain(before,plan(),1010)
        self.assertEqual(len(before),len(after));args=arguments(after)
        self.assertEqual(args[1]['components'],[0,5,0]);self.assertEqual(args[3],95)
        aim=args[2]['components'];self.assertGreater(aim[1],5);self.assertNotEqual(aim,[10000,5,0]);self.assertEqual(e.stats['modifiedThrows'],1)
    def test_retransmission_keeps_edit_after_disable(self):
        e=editor();first=e.process_plain(payload(),plan(),1010)
        again=e.process_plain(payload(),{},9000,allow=False)
        self.assertEqual(first,again);self.assertEqual(e.stats['modifiedThrows'],1)
    def test_retransmission_remains_stable_after_stream_compaction(self):
        e=editor();first=e.process_plain(payload(),plan(),1010);offset=7+len(record());telemetry=(0x8000000a).to_bytes(4,'big')+b'\xf8'+bytes(9)
        for _ in range(70):
            data=telemetry*80;e.process_plain(stream(data,offset),{},2000,False);offset+=len(data)
        self.assertGreater(e.tracker.streams[6]['stream'].origin,65536)
        self.assertEqual(e.process_plain(payload(),{},5000,False),first)
    def test_expired_or_unowned_plans_do_not_edit(self):
        for key,value in [('active',False),('clickAt',0),('expires',1005),('issued',0),('head',[100,5,0])]:
            p=plan();p[key]=value;e=editor();self.assertEqual(e.process_plain(payload(),p,1010),payload())
    def test_manual_power_is_preserved(self):
        p=plan();p['autoPower']=False;e=editor();after=e.process_plain(payload(),p,1010);self.assertEqual(arguments(after)[3],60)
    def test_fragmented_edit_range_is_not_partially_changed(self):
        data=bytes.fromhex('06010400000001')+record();cut=len(data)-16;e=editor()
        first=stream(data[:cut]);second=stream(data[cut:],cut)
        self.assertEqual(e.process_plain(first,plan(),1010),first);self.assertEqual(e.process_plain(second,plan(),1010),second);self.assertFalse(e.ledger)
    def test_one_physical_click_cannot_edit_multiple_throws(self):
        e=editor();e.process_plain(payload(),plan(),1010);next_record=stream(record(),7+len(record()))
        self.assertEqual(e.process_plain(next_record,plan(),1010),next_record);self.assertEqual(e.stats['modifiedThrows'],1)
    def test_ballistic_lead_hits_moving_receiver(self):
        origin=[0,5,0];target=[70,5,0];movement=[8,0,0];launch,t=velocity(origin,target,movement,95,28,.05)
        landed=[origin[i]+launch[i]*t-(.5*28*t*t if i==1 else 0) for i in range(3)]
        expected=[target[i]+movement[i]*(t+.05) for i in range(3)]
        self.assertLess(length(tuple(a-b for a,b in zip(landed,expected))),1e-6)
    def test_authenticated_packet_is_reencrypted_with_same_number(self):
        hp=bytes(range(32));key=bytes(range(32,64));iv=bytes(range(12));pn=7;header=b'\x40'+bytes(range(20))+bytes([pn]);nonce=nonce_for(iv,pn)
        original=payload();cipher=AESGCM(key).encrypt(nonce,original,header);packet=bytearray(header+cipher);mask=ecb(hp,bytes(packet[25:41]));packet[0]^=mask[0]&31;packet[21]^=mask[1];udp=bytes(31)+bytes(packet)
        context=dict(hp=hp,key=key,iv=iv,cid=bytes(range(20)),phase=0,largest=pn)
        u,n,plain=authenticated_plain(udp,context);changed=editor().process_plain(plain,plan(),1010);updated=encrypt_changed(udp,u,n,changed,context)
        decoded,recovered=decrypt(updated,hp,key,iv,31,pn)
        self.assertEqual(decoded.packet_number,pn);self.assertEqual(recovered,changed);self.assertEqual(updated[:31],udp[:31]);self.assertEqual(len(updated),len(udp))

if __name__=='__main__':unittest.main(verbosity=2)
