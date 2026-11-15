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
    def test_timed_jump_and_nonduplicated_dive(self):
        jump=2500/(2*196.2)
        self.assertLess(jump_rise(jump,196.2,.01),.51)
        self.assertAlmostEqual(jump_rise(jump,196.2,.3),jump)
        reach=math.hypot(3.65,.5*math.hypot(1.4,1.4))+.6
        d=dict(position=[0,0,0],velocity=[0,0,0],speed=20,jump=0,dive=12,radius=1.5+reach,above=reach-.5,below=reach+.5,worldGravity=196.2)
        self.assertAlmostEqual(reach_gap(d,[d['radius']+20*.2+25*.1,-.5,0],.2),0)
        self.assertGreater(reach_gap(d,[d['radius'],d['above'],0],0),0)
    def test_diagonal_close_window_uses_shared_geometry(self):
        reach=math.hypot(3.65,.5*math.hypot(1.4,1.4))+.6
        d=dict(position=[0,7.4883728,-5.84384155],velocity=[0,0,0],speed=20,jump=2500/(2*196.2),dive=12,radius=1.5+reach,above=reach-.5,below=reach+.5,worldGravity=196.2)
        origin=[11.3648453,7.489995,-15.778595];end=[11.9801,13.82776,11.9801]
        launch,flight=velocity(origin,end,[0,0,0],95,28,.0624565,False,5)
        margin=catch_path_margin(origin,launch,flight,95,28,.0624565,[d])
        self.assertGreater(margin,1.0)
        wr=dict(d,position=[0,7.4883728,0])
        self.assertLess(reach_gap(wr,end,flight+.0624565),-.25)
        self.assertLess(catch_path_margin(origin,launch,flight,95,28,.0624565,[wr]),0)
    def test_sender_revalidates_receiver_reach(self):
        p=plan();p.update(smartFit=True,coverageKnown=True,catchers=[],receiver=dict(position=[70,5,0],velocity=[0,0,0],speed=20,jump=6.37,dive=12,radius=5.88,above=3.88,below=4.88,worldGravity=196.2))
        self.assertNotEqual(editor().process_plain(payload(),p,1010),payload())
        p['receiver']['position']=[200,5,0]
        self.assertEqual(editor().process_plain(payload(),p,1010),payload())
        for key,value in (('radius',0),('above',float('nan')),('worldGravity',0)):
            p['receiver'].update(position=[70,5,0],radius=5.88,above=3.88,worldGravity=196.2);p['receiver'][key]=value
            self.assertEqual(editor().process_plain(payload(),p,1010),payload())
    def test_near_tangent_roots_are_not_lost_between_samples(self):
        origin,target,moving=[0,8,0],[100,8,0],[0,0,0]
        def required(angle):return angle_velocity(origin,target,moving,angle,28,.05,5)[2]
        lo,hi=30.,60.
        for _ in range(80):
            a,b=lo+(hi-lo)/3,hi-(hi-lo)/3
            if required(a)<required(b):hi=b
            else:lo=a
        power=required((lo+hi)/2)*(1+1e-10)
        low,t0=velocity(origin,target,moving,power,28,.05,False,5)
        high,t1=velocity(origin,target,moving,power,28,.05,True,5)
        self.assertLess(t1-t0,.01);self.assertGreater(t1,t0)
        for launch,t in ((low,t0),(high,t1)):
            landing=[origin[i]+launch[i]*(t+5/power)-(.5*28*t*t if i==1 else 0) for i in range(3)]
            self.assertLess(length(tuple(landing[i]-target[i] for i in range(3))),1e-5)
    def test_signed_clearance_distinguishes_inside_catch_radius(self):
        defender=dict(position=[70,5,0],velocity=[0,0,0],speed=22,jump=9,dive=12)
        central=catch_path_margin([65,5,0],[95,0,0],0,95,28,.05,[defender])
        beyond=catch_path_margin([68,5,0],[95,0,0],0,95,28,.05,[defender])
        self.assertGreater(beyond,central);self.assertLess(beyond,0)
    def test_beam_uses_low_root_and_exact_power_at_actual_origin(self):
        for distance,height in ((15,0),(35,5),(150,5),(220,5)):
            p=plan();p.update(beam95=True,plannedPower=95,lob=False,anglePower=True,angle=75,targetPosition=[distance,5+height,0])
            e=editor();args=arguments(e.process_plain(payload(),p,1010))
            self.assertEqual(e.stats['modifiedThrows'],1);self.assertEqual(args[3],95)
            aim=args[2]['components'];angle=math.degrees(math.atan2(aim[1]-5,math.hypot(aim[0],aim[2])))
            self.assertLessEqual(angle,35)
            if distance==15:self.assertLess(angle,5)
            direction=tuple(aim[i]-args[1]['components'][i] for i in range(3));norm=length(direction);t=e.last['flight']
            landed=[args[1]['components'][i]+direction[i]/norm*(5+95*t)-(.5*28*t*t if i==1 else 0) for i in range(3)]
            self.assertLess(length(tuple(landed[i]-p['targetPosition'][i] for i in range(3))),.001)
    def test_beam_can_transmit_reduced_power_for_close_jump_catch(self):
        p=plan();p.update(beam95=True,plannedPower=45,targetPosition=[25,9,0])
        e=editor();args=arguments(e.process_plain(payload(),p,1010))
        self.assertEqual(e.stats['modifiedThrows'],1);self.assertEqual(args[3],45)
        aim=args[2]['components'];self.assertLessEqual(math.degrees(math.atan2(aim[1]-5,math.hypot(aim[0],aim[2]))),35)
    def test_beam_transmits_selected_high_arc_and_power(self):
        p=plan();p.update(beam95=True,plannedPower=65,lob=True,targetPosition=[150,5,0])
        e=editor();args=arguments(e.process_plain(payload(),p,1010))
        self.assertEqual(e.stats['modifiedThrows'],1);self.assertEqual(args[3],65)
        direction=tuple(a-b for a,b in zip(args[2]['components'],args[1]['components']))
        angle=math.degrees(math.atan2(direction[1],math.hypot(direction[0],direction[2])))
        self.assertGreater(angle,35);self.assertLessEqual(angle,85)
        t=e.last['flight'];norm=length(direction)
        landed=[args[1]['components'][i]+direction[i]/norm*(5+65*t)-(.5*28*t*t if i==1 else 0) for i in range(3)]
        self.assertLess(length(tuple(landed[i]-p['targetPosition'][i] for i in range(3))),.001)
    def test_smart_fit_withholds_rewrite_when_contested(self):
        p=plan();p.update(smartFit=True,coverageKnown=True,catchers=[])
        self.assertNotEqual(editor().process_plain(payload(),p,1010),payload())
        for position in ([70,5,0],[15,5,0]):
            p['catchers']=[dict(position=position,velocity=[0,0,0],speed=22,jump=9,dive=12)]
            e=editor();self.assertEqual(e.process_plain(payload(),p,1010),payload());self.assertEqual(e.stats.get('modifiedThrows',0),0)
        p['catchers']=[dict(position=[35,5,100],velocity=[0,0,0],speed=22,jump=9,dive=12)]
        e=editor();self.assertNotEqual(e.process_plain(payload(),p,1010),payload());self.assertFalse(e.last['contested'])
        p['coverageKnown']=False;self.assertEqual(editor().process_plain(payload(),p,1010),payload())
    def test_smart_fit_rejects_invalid_coverage_metadata(self):
        for catchers in (None,[None],[{}],[dict(position=[70,5,0],velocity=[0,0,0],speed=22,jump=float('nan'),dive=12)], [{}]*129):
            p=plan();p.update(smartFit=True,coverageKnown=True,catchers=catchers)
            self.assertEqual(editor().process_plain(payload(),p,1010),payload())
    def test_selected_power_and_jump_target_reach_packet(self):
        for selected in (60,75,90,95):
            p=plan();p.update(plannedPower=selected,targetPosition=[70,10,0])
            e=editor();args=arguments(e.process_plain(payload(),p,1010));self.assertEqual(args[3],selected)
            origin=args[1]['components'];direction=tuple(a-b for a,b in zip(args[2]['components'],origin));norm=length(direction);direction=tuple(v/norm for v in direction);t=e.last['flight']
            landed=[origin[i]+direction[i]*(5+selected*t)-(.5*28*t*t if i==1 else 0) for i in range(3)]
            self.assertLess(length(tuple(a-b for a,b in zip(landed,p['targetPosition']))),.001)
        for invalid in (True,0,96,float('nan'),'95'):
            p=plan();p['plannedPower']=invalid;self.assertEqual(editor().process_plain(payload(),p,1010),payload())
    def test_transmitted_throw_lands_after_server_spawn_offset(self):
        for angle in (None,12,35,55):
            for moving in ([0,0,0],[8,0,2]):
                p=plan();p.update(targetPosition=[70,7,0],targetVelocity=moving)
                if angle is not None:p.update(anglePower=True,angle=angle)
                e=editor();args=arguments(e.process_plain(payload(),p,1010))
                self.assertEqual(e.stats['modifiedThrows'],1)
                origin=args[1]['components'];aim=args[2]['components'];power=args[3]
                direction=tuple(a-b for a,b in zip(aim,origin));norm=length(direction);direction=tuple(v/norm for v in direction)
                t=e.last['flight']
                landed=[origin[i]+direction[i]*(5+power*t)-(.5*28*t*t if i==1 else 0) for i in range(3)]
                expected=[p['targetPosition'][i]+moving[i]*(t+.05) for i in range(3)]
                self.assertLess(length(tuple(a-b for a,b in zip(landed,expected))),.001)
                if angle is None:self.assertEqual(power,95)
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
