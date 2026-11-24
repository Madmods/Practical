import unittest,struct
from unittest.mock import patch
import qb_packet_editor as qb_editor
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
    def alternative_plan(self):
        p=plan();p.update(targetPosition=[83,5,0],plannedPower=60,fitOffset=[5,0,0],smartFit=True,coverageKnown=True,catchers=[],boundaryRequired=True,boundaryMargin=0,
            field=dict(known=True,center=[0,3,0],right=[1,0,0],forward=[0,0,1],halfWidth=80,halfLength=180),
            receiver=dict(position=[78,5,0],velocity=[0,0,0],speed=0,jump=0,dive=0,radius=6,above=6,below=8),
            rankedThrows=[dict(offset=[0,0,0],power=60,angle=20,lob=False)])
        return p

    def test_alternative_revalidated_at_release_preserves_wire_contract(self):
        p=self.alternative_plan();e=editor();before=payload();after=e.process_plain(before,p,1010)
        self.assertEqual(e.stats['modifiedThrows'],1,e.last_decline)
        self.assertEqual(e.last['alternativeRank'],1);self.assertEqual(e.last['primaryDecline'],'Catch point exceeds OOB limit')
        self.assertEqual(e.last['target'],[78,5,0]);self.assertEqual(len(after),len(before))
        old,new=arguments(before),arguments(after)
        self.assertEqual(old[:2],new[:2]);self.assertEqual(len(old),len(new));self.assertEqual(new[3],int(new[3]))
        self.assertEqual(e.decision_history()[-1]['detail'],'Validated alternative 1')

    def test_alternatives_never_relax_expiry_schema_origin_or_reach(self):
        for change,expected in ((dict(expires=1005),'expired'),(dict(environment='match'),'arguments changed'),(dict(head=[100,5,0]),'origin')):
            p=self.alternative_plan();p.update(change);e=editor()
            with patch.object(qb_editor,'_replacement',wraps=qb_editor._replacement) as attempt:
                self.assertEqual(e.process_plain(payload(),p,1010),payload());self.assertEqual(attempt.call_count,1)
            self.assertIn(expected,e.last_decline['reason'].lower());self.assertFalse(e.claimed)
            self.assertEqual(e.decision_history()[-1]['detail'],e.last_decline['reason'])
        p=self.alternative_plan();p['rankedThrows'][0]['offset']=[30,0,0];e=editor()
        self.assertEqual(e.process_plain(payload(),p,1010),payload());self.assertFalse(e.claimed)

    def test_alternatives_are_bounded_and_do_not_extend_click_lease(self):
        p=self.alternative_plan();e=editor()
        with patch.object(qb_editor.time,'perf_counter',side_effect=(0,.026)):
            self.assertEqual(e.process_plain(payload(),p,1010),payload())
        self.assertFalse(e.claimed)
        p=self.alternative_plan();p['expires']=1010;e=editor()
        self.assertEqual(e.process_plain(payload(),p,1010),payload());self.assertFalse(e.claimed)

    def test_outside_boundary_margin_at_actual_release(self):
        field=dict(known=True,center=[0,3,0],right=[1,0,0],forward=[0,0,1],halfWidth=80,halfLength=180)
        for margin,target,receiver_x,accepted in ((0,80,78,True),(0,81,78,False),(2,83,78,False),(4,83,78,True),(0,79.5,78,True),(.5,80.5,78,True),(20,79.5,78,True),(20,83,81,False)):
            with self.subTest(margin=margin,target=target,receiver=receiver_x):
                receiver=dict(position=[receiver_x,5,0],velocity=[0,0,0],speed=0,jump=0,dive=0,radius=6,above=6,below=8)
                p=plan();p.update(boundaryRequired=True,boundaryMargin=margin,field=field,receiver=receiver,targetPosition=[target,5,0],smartFit=False)
                e=editor();after=e.process_plain(payload(),p,1010);self.assertEqual(e.stats['modifiedThrows'],int(accepted),e.last_decline)
                if not accepted:self.assertEqual(after,payload());self.assertFalse(e.claimed)
        for invalid in (None,True,'2',math.nan,math.inf,-.5,-5,20.5):
            p=plan();p.update(boundaryRequired=True,boundaryMargin=invalid,field=field,receiver=dict(position=[78,5,0],velocity=[0,0,0],speed=0,jump=0,dive=0,radius=6,above=6,below=8),targetPosition=[78,5,0])
            e=editor();self.assertEqual(e.process_plain(payload(),p,1010),payload());self.assertEqual(e.last_decline['reason'],'OOB limit bounds')
        for margin in (0,2,20):
            p=plan();p.update(environment='practice',boundaryRequired=False,boundaryMargin=margin,field=dict(known=False));self.assertNotEqual(editor().process_plain(payload(),p,1010),payload())

    def test_boundary_margin_rotations_end_zones_and_corners(self):
        for angle in (0,.45,math.pi/2):
            right=[math.cos(angle),0,math.sin(angle)];forward=[-math.sin(angle),0,math.cos(angle)]
            field=dict(known=True,center=[130,3,-90],right=right,forward=forward,halfWidth=80,halfLength=180)
            def point(x,z):return [130+right[0]*x+forward[0]*z,8,-90+right[2]*x+forward[2]*z]
            for axis in (0,1):
                for sign in (-1,1):
                    for margin in (0,.5,2,4,10,20):
                        edge=lambda delta:point(sign*(80+delta),0) if axis==0 else point(0,sign*(180+delta))
                        self.assertTrue(field_catch_allowed(field,edge(margin),margin));self.assertFalse(field_catch_allowed(field,edge(margin+.1),margin))
            self.assertFalse(field_catch_allowed(field,point(81,181),1.4));self.assertTrue(field_catch_allowed(field,point(81,181),1.5))

    def test_match_movement_prefix_and_dynamic_tail_edit_only_destination_and_power(self):
        schema=Schema.from_packet(fixture_schema());schema.properties=[dict(name='MoveDirectionInternal',className='Humanoid',kind=22)]
        for extra in ((60,1),(75,1),(75.5,-2)):
            base=body();base=base[:9]+v(6)+base[10:-1]+b'\x0c'+struct.pack('<d',extra[0])+b'\x0c'+struct.pack('<d',extra[1])+base[-1:]
            base=base.replace(b'\x16'+struct.pack('<fff',0,5,0),b'\x16'+struct.pack('<fff',-6000000,5,0),1)
            movement=b'\x0d\x01'+(123).to_bytes(4,'little')+bytes(2)+b'\x00'+struct.pack('<fff',.8,0,.6)
            raw=b'\xc3'+movement+bytes([MAPS['4'].index(7)])+base[2:]
            framed=(0x80000000+len(raw)).to_bytes(4,'big')+raw;before=stream(bytes.fromhex('06010400000001')+framed)
            e=Editor(schema);p=plan();p.update(environment='match',beam95=True,plannedPower=73.6)
            after=e.process_plain(before,p,1010);self.assertEqual(e.stats['modifiedThrows'],1,e.last_decline);self.assertTrue(e.format_verified)
            changed_body=frames(after)[0].data[11:];calls=Wire(schema,MAPS['4']).events(b'\x83'+changed_body[1:],'outgoing');args=calls[0]['args']
            self.assertEqual(args[1]['components'],[-6000000,5,0]);self.assertEqual(args[3],int(args[3]));self.assertEqual(args[4:],list(extra))
            self.assertEqual(changed_body[1:22],movement);self.assertEqual(len(before),len(after));self.assertEqual(after[-19:],before[-19:])
            aim=before.index(struct.pack('<fff',10000,5,0));power=aim+13;allowed=set(range(aim,aim+12))|set(range(power,power+8))
            self.assertTrue(all(a==b or i in allowed for i,(a,b) in enumerate(zip(before,after))))

    def test_first_throw_validates_and_assists_without_calibration(self):
        from qb_packet_host import control_status
        e=editor();p=plan();self.assertTrue(control_status(e)[1]);self.assertFalse(e.format_verified)
        first=payload();self.assertNotEqual(e.process_plain(first,p,1010,True),first)
        self.assertTrue(e.format_verified);self.assertTrue(control_status(e)[1]);self.assertEqual(e.stats['modifiedThrows'],1)
        self.assertEqual(e.claimed,{p['clickAt']})

    def test_parser_failure_stays_unready_and_diagnostic_is_bounded(self):
        from unittest.mock import patch
        from qb_packet_host import control_status
        e=editor();raw=b'\xc3'+b'x'*5000;framed=(0x80000000+len(raw)).to_bytes(4,'big')+raw
        before=stream(bytes.fromhex('06010400000001')+framed*32)
        with patch.object(e,'_calls',side_effect=DecodeError('fixture parser failure')):
            self.assertEqual(e.process_plain(before,plan(),1010,False),before)
        self.assertFalse(e.format_verified);self.assertFalse(control_status(e)[1]);self.assertFalse(e.ledger)
        self.assertEqual(len(e.throw_diagnostics),8);self.assertEqual(e.diagnostic_sequence,32)
        self.assertTrue(all(len(d['recordHex'])==8192 and d['truncated'] for d in e.throw_diagnostics))
        e=editor();e.process_plain(payload(),{},1010,False);self.assertFalse(e.throw_diagnostics)
        e=editor();e.process_plain(payload(),plan(),4000,False);self.assertFalse(e.throw_diagnostics)

    def test_throw_diagnostic_preserves_clicked_record_past_acknowledgements(self):
        e=editor();p=plan()
        for i in range(20):e._trace_throw(6,i,b'\xc3acknowledgement',p,1010)
        e._trace_throw(6,21,b'\xc3Clicked',p,1010)
        for i in range(22,50):e._trace_throw(6,i,b'\xc3acknowledgement',p,1010)
        self.assertEqual(len(e.throw_diagnostics),8);self.assertEqual(sum(d['containsClicked'] for d in e.throw_diagnostics),1)

    def test_packet_beam_changes_only_destination_and_whole_power(self):
        for environment in ('practice','match'):
            raw=body()
            if environment=='match':raw=raw[:9]+v(6)+raw[10:-1]+b'\x0c'+struct.pack('<d',60)+b'\x0c'+struct.pack('<d',1)+raw[-1:]
            before=stream(bytes.fromhex('06010400000001')+(0x80000000+len(raw)).to_bytes(4,'big')+raw)
            for selected in (73,73.6,94.9):
                with self.subTest(environment=environment,selected=selected):
                    p=plan();p.update(environment=environment,beam95=True,plannedPower=selected)
                    e=editor();after=e.process_plain(before,p,1010)
                    self.assertEqual(e.stats['modifiedThrows'],1,e.last_decline)
                    old,new=arguments(before),arguments(after)
                    self.assertEqual(len(new),len(old));self.assertEqual(new[:2],old[:2]);self.assertEqual(new[4:],old[4:])
                    self.assertEqual(new[3],int(new[3]));self.assertGreaterEqual(new[3],1);self.assertLessEqual(new[3],95)
                    self.assertNotEqual(new[2],old[2]);self.assertNotEqual(new[3],old[3])
                    aim=before.index(struct.pack('<fff',10000,5,0));power=aim+12+1
                    allowed=set(range(aim,aim+12))|set(range(power,power+8))
                    self.assertEqual(len(after),len(before));self.assertTrue(all(a==b or i in allowed for i,(a,b) in enumerate(zip(before,after))))
                    self.assertEqual(struct.unpack_from('<d',after,power)[0],int(new[3]))

    def test_wheel_confirmation_matches_actual_power_before_destination_edit(self):
        for environment in ('practice','match'):
            raw=body()
            if environment=='match':raw=raw[:9]+v(6)+raw[10:-1]+b'\x0c'+struct.pack('<d',60)+b'\x0c'+struct.pack('<d',1)+raw[-1:]
            before=stream(bytes.fromhex('06010400000001')+(0x80000000+len(raw)).to_bytes(4,'big')+raw)
            p=plan();p.update(environment=environment,requireWheelPower=True,confirmedPower=60,plannedPower=60,autoPower=False)
            e=editor();after=e.process_plain(before,p,1010)
            self.assertEqual(e.stats['modifiedThrows'],1,e.last_decline);self.assertEqual(arguments(after)[3],60)
            self.assertEqual(len(e.ledger),1) # Destination only; no power edit is staged.
            aim=before.index(struct.pack('<fff',10000,5,0));allowed=set(range(aim,aim+12))
            self.assertEqual(len(before),len(after));self.assertTrue(all(a==b or i in allowed for i,(a,b) in enumerate(zip(before,after))))
            for confirmed,planned in ((73,73),(60,73),(True,60),(None,60),(float('nan'),60),(60.5,60.5),(0,0),(96,96)):
                with self.subTest(environment=environment,confirmed=confirmed,planned=planned):
                    bad={**p,'confirmedPower':confirmed,'plannedPower':planned};e=editor()
                    self.assertEqual(e.process_plain(before,bad,1010),before);self.assertFalse(e.ledger);self.assertFalse(e.claimed)
                    self.assertEqual(e.decision_history()[-1]['outcome'],'normal');self.assertEqual(e.decision_history()[-1]['reason'],'Wheel not ready')

    def test_decision_history_distinguishes_skip_from_assist_and_deduplicates(self):
        e=editor();p=plan();p.update(requireWheelPower=True,confirmedPower=73,plannedPower=73,autoPower=False)
        before=payload();self.assertEqual(e.process_plain(before,p,1010),before)
        self.assertEqual(e.decision_history()[0]['reason'],'Wheel not ready')
        following=stream(record(),len(frames(before)[0].data));self.assertEqual(e.process_plain(following,p,1020),following)
        self.assertEqual(len(e.decision_history()),1)
        p.update(confirmedPower=60,plannedPower=60)
        following=stream(record(),len(frames(before)[0].data)+len(record()))
        self.assertNotEqual(e.process_plain(following,p,1030),following)
        self.assertEqual([x['outcome'] for x in e.decision_history()],['normal','assisted'])
        self.assertNotIn('_key',e.decision_history()[0]);self.assertEqual(e.decision_history()[-1]['power'],60)
        self.assertEqual(e.process_plain(payload(),{},9000,False),before);self.assertEqual(len(e.decision_history()),2)

    def test_history_is_bounded_and_expired_plan_has_clear_skip_reason(self):
        e=editor();offset=0
        for i in range(12):
            p=plan();p.update(clickAt=980+i,expires=1005)
            raw=record();data=(bytes.fromhex('06010400000001') if not i else b'')+raw
            before=stream(data,offset);offset+=len(data)
            self.assertEqual(e.process_plain(before,p,1010+i),before)
        history=e.decision_history();self.assertEqual(len(history),8)
        self.assertEqual(history[0]['sequence'],5);self.assertEqual(history[-1]['sequence'],12)
        self.assertTrue(all(item['outcome']=='normal' and item['reason']=='Plan expired' for item in history))

    def test_live_match_union_ack_does_not_hide_the_following_throw(self):
        schema=Schema.from_packet(fixture_schema());schema.properties=[dict(name='CFrame',className='UnionOperation',kind=27)]
        for versioned in (0,1):
            ack=bytes([MAPS['4'].index(10)])+b'\x01'+(123).to_bytes(4,'little')+bytes(2)+bytes([versioned])+(v(658) if versioned else b'')+bytes(4)
            raw=bytes([0xc3])+ack+bytes([MAPS['4'].index(7)])+body()[2:]
            framed=(0x80000000+len(raw)).to_bytes(4,'big')+raw;before=stream(bytes.fromhex('06010400000001')+framed)
            e=Editor(schema);after=e.process_plain(before,plan(),1010)
            self.assertEqual(e.stats['modifiedThrows'],1,e.last_decline);self.assertEqual(len(after),len(before))
            self.assertEqual(frames(after)[0].data[12:12+len(ack)],ack)
            self.assertEqual(e.stats['declinedRecords'],0)
    def test_warmup_primes_cached_arguments_without_editing_or_claiming_click(self):
        e=editor();event=body()[:9]
        warm_body=event+v(1)+b'\x01\x87'+(6).to_bytes(4,'big')+b'Cached'
        rec=lambda raw:(0x80000000+len(raw)).to_bytes(4,'big')+raw
        warm_data=bytes.fromhex('06010400000001')+rec(warm_body)
        before=stream(warm_data);self.assertEqual(e.process_plain(before,plan(),1010,False),before)
        self.assertEqual(e.wire.cache[7],'Cached');self.assertFalse(e.claimed);self.assertFalse(e.ledger)
        following=stream(rec(event+v(1)+b'\x01\x07'),len(warm_data));self.assertEqual(e.process_plain(following,plan(),1020),following)
        self.assertEqual(e.stats['decodedCalls'],2);self.assertEqual(e.stats['declinedRecords'],0)

    def test_parser_recovery_retains_accepted_edits_and_new_throw_ownership(self):
        schema=Schema.from_packet(fixture_schema());schema.properties=[dict(name='CFrame',className='Part',kind=27)]
        e=Editor(schema);first=payload();accepted=e.process_plain(first,plan(),1010);self.assertNotEqual(accepted,first)
        bad=Schema.from_packet(fixture_schema());bad.properties=[dict(name='CFrame',className='Camera',kind=27)];e.wire.schema=bad
        ack=bytes([MAPS['3'].index(10)])+b'\x01'+(123).to_bytes(4,'little')+bytes(2)+b'\x00'+bytes(4)
        raw=body()[:1]+ack+body()[1:];record=(0x80000000+len(raw)).to_bytes(4,'big')+raw;offset=len(frames(first)[0].data)
        for i in range(3):
            current=stream(record,offset);offset+=len(record);self.assertEqual(e.process_plain(current,plan(),6000+i),current)
        self.assertEqual(e.stats['parserRecoveries'],1);self.assertFalse(e.recovering);self.assertEqual(e.claimed,{990})
        self.assertEqual(e.process_plain(first,{},7000,False),accepted)
        next_plan=plan();next_plan.update(issued=10040,expires=10190,clickAt=10030)
        current=stream(record,offset);self.assertNotEqual(e.process_plain(current,next_plan,10050),current)
        self.assertEqual(e.stats['modifiedThrows'],2);self.assertEqual(e.claimed,{990,10030})

    def test_new_join_schema_replaces_reader_without_discarding_ledger(self):
        e=editor();first=payload();accepted=e.process_plain(first,plan(),1010);ledger=list(e.ledger)
        fresh=Schema.from_packet(fixture_schema());e.update_schema(fresh,2000)
        self.assertIs(e.schema,fresh);self.assertIs(e.tracker.schema,fresh);self.assertEqual(e.ledger,ledger);self.assertEqual(e.claimed,{990})
        self.assertEqual(e.process_plain(first,{},2010,False),accepted);self.assertEqual(e.stats['schemaRefreshes'],1)

    def test_every_validated_replication_profile_can_carry_a_throw(self):
        for prefix,mode in REPLICATION_MODES.items():
            with self.subTest(mode=mode):
                raw=bytes([prefix,MAPS[mode].index(7)])+body()[2:];rec=(0x80000000+len(raw)).to_bytes(4,'big')+raw
                before=stream(bytes.fromhex('06010400000001')+rec);e=editor();after=e.process_plain(before,plan(),1010)
                self.assertEqual(e.stats['modifiedThrows'],1,e.last_decline);self.assertEqual(len(after),len(before))
                decoded=frames(after)[0].data[11:];call=Wire(e.schema,MAPS[mode]).events(b'\x83'+decoded[1:],'outgoing')[0]
                self.assertEqual(call['args'][0],'Clicked');self.assertEqual(call['args'][3],95)

    def test_recovery_plaintext_history_has_a_memory_bound(self):
        e=editor()
        for i in range(300):e.process_plain(stream(bytes(6000),i*6000),{},1000+i,False)
        self.assertLessEqual(len(e.recent),256);self.assertLessEqual(e.recent_bytes,1024*1024);self.assertFalse(e.claimed)

    def test_open_away_floor_survives_actual_origin_and_retained_plan(self):
        for beam in (False,True):
            for angle_power in (False,True):
                p=plan();p.update(head=[1,5,0],targetPosition=[70,5,0],targetVelocity=[20,0,0],plannedPower=95,beam95=beam,anglePower=angle_power,angle=7,angleFallback=True,minAngle=25,maxAngle=50,sampleAgeMs=80)
                e=editor();after=e.process_plain(payload(),p,1010)
                self.assertEqual(e.stats['modifiedThrows'],1,e.last_decline)
                args=arguments(after);aim=args[2]['components'];origin=args[1]['components'];direction=[a-b for a,b in zip(aim,origin)];norm=length(direction);direction=[x/norm for x in direction]
                elevation=math.degrees(math.atan2(direction[1],math.hypot(direction[0],direction[2])))
                self.assertGreaterEqual(elevation,25-1e-4);self.assertLessEqual(elevation,50+1e-4)
                target=[71.8,5,0] # Measured route advanced by 80ms sample + 10ms dispatch.
                launch,t,power=angle_velocity(origin,target,[20,0,0],25,28,.05,5)
                self.assertEqual(args[3],int(args[3]));self.assertLess(abs(args[3]-power),1)
                t=e.last['flight']
                end=[origin[i]+direction[i]*(5+args[3]*t)-(.5*28*t*t if i==1 else 0) for i in range(3)]
                expected=[target[i]+[20,0,0][i]*(t+.05) for i in range(3)]
                self.assertLess(length([a-b for a,b in zip(end,expected)]),.001)

    def test_route_floor_does_not_force_invalid_or_manual_power_throw(self):
        for floor in (True,None,math.nan,-31,51):
            p=plan();p.update(minAngle=floor,maxAngle=50);e=editor()
            self.assertEqual(e.process_plain(payload(),p,1010),payload());self.assertEqual(e.stats['modifiedThrows'],0)
        p=plan();p.update(minAngle=25,maxAngle=50,autoPower=False);e=editor()
        self.assertEqual(e.process_plain(payload(),p,1010),payload());self.assertEqual(e.stats['modifiedThrows'],0)
        p=plan();p.update(targetPosition=[500,5,0],targetVelocity=[20,0,0],minAngle=25,maxAngle=50);e=editor()
        self.assertEqual(e.process_plain(payload(),p,1010),payload());self.assertEqual(e.stats['modifiedThrows'],0)

    def test_practice_and_match_keep_distinct_boundary_policies(self):
        p=plan();p.update(environment='practice',boundaryRequired=False,field=dict(known=False))
        practice=editor();after=practice.process_plain(payload(),p,1010)
        self.assertEqual(practice.stats['modifiedThrows'],1,practice.last_decline)
        self.assertNotEqual(after,payload())
        p.update(environment='match',boundaryRequired=True)
        match=editor();self.assertEqual(match.process_plain(payload(),p,1010),payload())
        self.assertEqual(match.stats['modifiedThrows'],0)
        self.assertIn('arguments changed',match.last_decline['reason'])

    def test_environment_contracts_are_not_interchangeable(self):
        raw=body();match=raw[:9]+v(6)+raw[10:-1]+b'\x0c'+struct.pack('<d',60)+b'\x0c'+struct.pack('<d',1)+raw[-1:]
        for environment,count,accepted in (('practice',4,True),('practice',6,False),('match',4,False),('match',6,True),('unknown',4,False)):
            with self.subTest(environment=environment,count=count):
                raw=body() if count==4 else match
                before=stream(bytes.fromhex('06010400000001')+(0x80000000+len(raw)).to_bytes(4,'big')+raw)
                p=plan();p['environment']=environment;e=editor();after=e.process_plain(before,p,1010)
                self.assertEqual(e.stats['modifiedThrows'],int(accepted),e.last_decline)
                if not accepted:
                    self.assertEqual(after,before);self.assertFalse(e.ledger);self.assertFalse(e.claimed)

    def test_changed_argument_layouts_pass_through_without_consuming_click(self):
        command=b'\x02'+s('Clicked');origin=b'\x16'+struct.pack('<fff',0,5,0)
        destination=b'\x16'+struct.pack('<fff',10000,5,0);power=b'\x0c'+struct.pack('<d',60)
        flag=b'\x0c'+struct.pack('<d',1)
        original=[command,origin,destination,power]
        changed=[original[:-1],original+[flag],original+[power,flag,flag],
                 [command,power,origin,destination],[command,destination,origin,power],
                 [command,origin,destination,b'\x0b'+struct.pack('<f',60)],
                 original+[power,b'\x0b'+struct.pack('<f',1)],
                 original+[b'\x09\x01',flag],
                 original+[power,b'\x02'+s('1')]]
        for args in changed:
            with self.subTest(count=len(args),args=args):
                raw=body()[:9]+v(len(args))+b''.join(args)+body()[-1:]
                changed_record=(0x80000000+len(raw)).to_bytes(4,'big')+raw
                data=bytes.fromhex('06010400000001')+changed_record;before=stream(data)
                e=editor();self.assertEqual(e.process_plain(before,plan(),1010),before)
                self.assertEqual(e.stats['modifiedThrows'],0);self.assertFalse(e.ledger);self.assertFalse(e.claimed)
                self.assertIsNotNone(e.last_decline)
                # A declined signature must not consume the user's throw intent.
                following=stream(record(),len(data))
                self.assertNotEqual(e.process_plain(following,plan(),1020),following)
                self.assertEqual(e.stats['modifiedThrows'],1,e.last_decline)

    def test_boundary_guard_at_actual_release_without_smart_fit(self):
        field=dict(known=True,center=[0,3,0],right=[1,0,0],forward=[0,0,1],halfWidth=80,halfLength=180)
        receiver=dict(position=[78,5,0],velocity=[0,0,0],speed=0,jump=0,dive=0,radius=6,above=6,below=8)
        for target,accepted in (([70,5,0],False),([78,5,0],True),([81,5,0],True),([85,5,0],False),([300,5,0],False),([78,40,0],False)):
            p=plan();p.update(boundaryRequired=True,field=field,receiver=receiver,targetPosition=target,smartFit=False)
            e=editor();after=e.process_plain(payload(),p,1010)
            self.assertEqual(e.stats['modifiedThrows'],int(accepted),(target,e.last_decline))
            if not accepted:self.assertEqual(after,payload())
        for bad in (None,{},dict(field,known=False),dict(field,halfWidth=math.nan),dict(field,right=[2,0,0])):
            p=plan();p.update(boundaryRequired=True,field=bad,receiver=receiver,targetPosition=[78,5,0]);e=editor()
            self.assertEqual(e.process_plain(payload(),p,1010),payload());self.assertEqual(e.stats['modifiedThrows'],0)

    def test_boundary_rechecks_route_after_sample_delay(self):
        field=dict(known=True,center=[0,3,0],right=[1,0,0],forward=[0,0,1],halfWidth=80,halfLength=180)
        receiver=dict(position=[78,5,0],velocity=[20,0,0],speed=20,jump=7,dive=12,radius=6,above=6,below=8)
        p=plan();p.update(boundaryRequired=True,field=field,receiver=receiver,targetPosition=[78,5,0],targetVelocity=[20,0,0],sampleAgeMs=80)
        e=editor();self.assertEqual(e.process_plain(payload(),p,1010),payload());self.assertEqual(e.stats['modifiedThrows'],0)

    def test_live_match_decodes_origin_but_preserves_wire_origin(self):
        raw=body();raw=raw[:9]+v(6)+raw[10:-1]+b'\x0c'+struct.pack('<d',60)+b'\x0c'+struct.pack('<d',1)+raw[-1:]
        raw=raw.replace(struct.pack('<fff',0,5,0),struct.pack('<fff',-6000000,5,0),1)
        before=stream(bytes.fromhex('06010400000001')+(0x80000000+len(raw)).to_bytes(4,'big')+raw)
        e=editor();after=e.process_plain(before,plan(),1010);args=arguments(after)
        self.assertEqual(e.stats['modifiedThrows'],1,e.last_decline)
        self.assertEqual(args[1]['components'],[-6000000,5,0])
        self.assertEqual(e.last['origin'],(0,5,0));self.assertEqual(args[4:],[60,1])
        direction=[args[2]['components'][i]-e.last['origin'][i] for i in range(3)]
        norm=length(direction);flight=e.last['flight'];power=args[3]
        landing=[e.last['origin'][i]+direction[i]/norm*(5+power*flight)-(.5*28*flight*flight if i==1 else 0) for i in range(3)]
        self.assertLess(length(tuple(a-b for a,b in zip(landing,[70,5,0]))),.001)
        invalid=plan();invalid['head']=[1000,5,0]
        rejected=editor();self.assertEqual(rejected.process_plain(before,invalid,1010),before)
        self.assertEqual(rejected.last_decline['reason'],'Unrecognized live-match origin encoding')

    def test_match_contract_changes_only_destination_and_integer_power(self):
        raw=body();raw=raw[:9]+v(6)+raw[10:-1]+b'\x0c'+struct.pack('<d',60)+b'\x0c'+struct.pack('<d',1)+raw[-1:]
        before=stream(bytes.fromhex('06010400000001')+(0x80000000+len(raw)).to_bytes(4,'big')+raw)
        for desired in (45.2,73.8,94.8477372608717):
            p=plan();p.update(plannedPower=desired,targetPosition=[40,5,0])
            e=editor();after=e.process_plain(before,p,1010)
            self.assertEqual(e.stats['modifiedThrows'],1,e.last_decline)
            original,updated=arguments(before),arguments(after)
            self.assertEqual(len(updated),6)
            self.assertEqual(updated[0],original[0]);self.assertEqual(updated[1],original[1])
            self.assertEqual(updated[4:],[60,1]);self.assertEqual(updated[4:],original[4:])
            self.assertEqual(updated[3],int(updated[3]));self.assertTrue(1<=updated[3]<=95)
            aim=before.index(struct.pack('<fff',10000,5,0));power=before.index(struct.pack('<d',60))
            allowed=set(range(aim,aim+12))|set(range(power,power+8))
            self.assertEqual(len(before),len(after))
            self.assertTrue(all(a==b or i in allowed for i,(a,b) in enumerate(zip(before,after))))

    def test_dynamic_match_trailing_values_are_preserved(self):
        for extra in ((60,1),(75,1),(61,1),(60,2),(60.5,1),(75.5,-2),(0,123456)):
            raw=body();raw=raw[:9]+v(6)+raw[10:-1]+b'\x0c'+struct.pack('<d',extra[0])+b'\x0c'+struct.pack('<d',extra[1])+raw[-1:]
            before=stream(bytes.fromhex('06010400000001')+(0x80000000+len(raw)).to_bytes(4,'big')+raw)
            e=editor();after=e.process_plain(before,plan(),1010)
            self.assertEqual(e.stats['modifiedThrows'],1,e.last_decline);self.assertEqual(arguments(after)[4:],list(extra))
            self.assertEqual(after[-19:],before[-19:]);self.assertEqual(len(after),len(before))

    def test_live_match_preserves_two_additional_arguments(self):
        raw=body();count_at=9
        self.assertEqual(raw[count_at],4)
        raw=raw[:count_at]+v(6)+raw[count_at+1:-1]+b'\x0c'+struct.pack('<d',60)+b'\x0c'+struct.pack('<d',1)+raw[-1:]
        before=stream(bytes.fromhex('06010400000001')+(0x80000000+len(raw)).to_bytes(4,'big')+raw)
        e=editor();after=e.process_plain(before,plan(),1010)
        self.assertEqual(e.stats['modifiedThrows'],1,e.last_decline)
        self.assertEqual(len(before),len(after))
        self.assertEqual(arguments(after)[4:],[60,1])
        self.assertEqual(after[-19:],before[-19:])
        self.assertEqual(e.process_plain(before,{},9000,allow=False),after)

    def test_physical_hold_uses_fresh_plan_after_old_click_deadline(self):
        p=plan();p.update(issued=5000,expires=5150,clickAt=1000,physicalHeld=True,releasedAt=0)
        e=editor();self.assertNotEqual(e.process_plain(payload(),p,5010),payload());self.assertEqual(e.stats['modifiedThrows'],1)
        p.update(physicalHeld=False,releasedAt=4990)
        e=editor();self.assertNotEqual(e.process_plain(payload(),p,5010),payload())
        for changes in (dict(physicalHeld=False,releasedAt=4500),dict(physicalHeld=True,releasedAt=4990),dict(physicalHeld=True,releasedAt=0,clickAt=0),dict(physicalHeld=True,releasedAt=0,issued=4800),dict(physicalHeld=True,releasedAt=0,clickAt=-1)):
            invalid={**p,**changes};self.assertEqual(editor().process_plain(payload(),invalid,5010),payload())

    def test_retained_beam_recalculates_power_when_receiver_leaves_selected_tier(self):
        launch,t=velocity([0,5,0],[71,5,0],[20,0,0],59,28,.05,spawn=5)
        angle=math.degrees(math.atan2(launch[1],math.hypot(launch[0],launch[2])))
        with self.assertRaises(DecodeError):velocity([0,5,0],[74.28,5,0],[20,0,0],59,28,.05,spawn=5)
        p=plan();p.update(beam95=True,plannedPower=59,targetPosition=[71,5,0],targetVelocity=[20,0,0],angle=angle,sampleAgeMs=154,clickPlanRetained=True)
        e=editor();args=arguments(e.process_plain(payload(),p,1010));self.assertEqual(e.stats['modifiedThrows'],1,e.last_decline)
        self.assertGreater(args[3],59);self.assertLessEqual(args[3],95)
        origin=args[1]['components'];aim=args[2]['components'];direction=tuple(a-b for a,b in zip(aim,origin));norm=length(direction);direction=tuple(v/norm for v in direction)
        self.assertAlmostEqual(math.degrees(math.atan2(direction[1],math.hypot(direction[0],direction[2]))),angle,delta=.5);self.assertEqual(args[3],int(args[3]))
        flight=e.last['flight'];landing=[origin[i]+direction[i]*(5+args[3]*flight)-(.5*28*flight*flight if i==1 else 0) for i in range(3)]
        self.assertLess(length(tuple(a-b for a,b in zip(landing,[74.28+20*(flight+.05),5,0]))),.001)
        p.update(targetPosition=[500,5,0]);self.assertEqual(editor().process_plain(payload(),p,1010),payload())

    def test_fit_preserves_catch_height_but_can_adjust_lead(self):
        p=plan();p.update(targetPosition=[70,8,3],routePosition=[70,5,0],targetVelocity=[0,0,20],leadDistance=3,preserveCatchHeight=True,heightAdjustment=3)
        e=editor();self.assertNotEqual(e.process_plain(payload(),p,1010),payload());self.assertEqual(e.last['routeOffset'],(0,3,0))
        for target in ([70,7.5,3],):
            p['targetPosition']=target;e=editor()
            self.assertEqual(e.process_plain(payload(),p,1010),payload())
            self.assertEqual(e.last_decline['reason'],'Fit lowers configured catch height')
        p['targetPosition']=[70,8,2];self.assertNotEqual(editor().process_plain(payload(),p,1010),payload())
        p['targetPosition']=[73,9,3];self.assertNotEqual(editor().process_plain(payload(),p,1010),payload())
        p.update(leadDistance=-3,targetPosition=[70,8,-4]);self.assertNotEqual(editor().process_plain(payload(),p,1010),payload())
        p['targetPosition']=[70,8,-2];self.assertNotEqual(editor().process_plain(payload(),p,1010),payload())

    def test_declined_raw_press_keeps_input_diagnostics(self):
        p=plan();p.update(clickAt=0,input=dict(action='physical-down-held',blocked=True,owned=True))
        e=editor();self.assertEqual(e.process_plain(payload(),p,1010),payload())
        self.assertEqual(e.last_decline['input'],p['input'])

    def test_widening_stays_relative_to_the_running_route(self):
        wr=dict(position=[80,8,0],velocity=[0,0,20],speed=20,jump=6.37,dive=12,radius=5.88,above=3.88,below=4.88,worldGravity=196.2,redirect=True)
        self.assertLess(receiver_gap(wr,[92,8,40],2),-.25)
        self.assertLess(receiver_gap(wr,[68,8,40],2),-.25)
        self.assertGreater(receiver_gap(wr,[80,8,-20],2),0)
        self.assertGreater(receiver_gap(wr,[120,8,40],2),0)
        slowing={**wr,'velocity':[0,0,5]}
        self.assertGreater(receiver_gap(slowing,[96,8,10],2),0)
        wr.update(position=[70,5,0])
        p=plan();p.update(targetPosition=[80,5,0],targetVelocity=[0,0,20],routePosition=[70,5,0],leadDistance=0,plannedPower=95,smartFit=True,coverageKnown=True,allowContested=True,catchers=[],receiver=wr,anglePower=False)
        e=editor();self.assertNotEqual(e.process_plain(payload(),p,1010),payload())
        self.assertEqual(e.stats['modifiedThrows'],1)
        self.assertEqual(e.last['routeOffset'],(10,0,0))
        p['targetPosition']=[82,5,0]
        self.assertEqual(editor().process_plain(payload(),p,1010),payload())

    def current_profile_record(self):
        schema=Schema.from_packet(fixture_schema())
        schema.properties=[dict(name='CFrame',className='Part',kind=27)]
        prefix=b'\x45'+bytes(4)+b'\xfe\x01'+(123).to_bytes(4,'little')+bytes(2)+b'\x01'+v(658)+bytes(4)
        data=b'\xc3'+prefix+b'\x4f'+body()[2:]
        return schema,data

    def test_current_client_profile_preserves_original_throw_math(self):
        schema,data=self.current_profile_record()
        raw=stream(bytes.fromhex('06010400000001')+(0x80000000+len(data)).to_bytes(4,'big')+data)
        for selected in (60,75,95):
            with self.subTest(power=selected):
                p=plan();p['plannedPower']=selected
                old=editor();old.process_plain(payload(),p,1010)
                new=Editor(schema);changed=new.process_plain(raw,p,1010)
                self.assertNotEqual(changed,raw)
                self.assertEqual(new.stats['modifiedThrows'],1)
                for key in ('newPower','aim','origin','flight','target','movement'):
                    self.assertEqual(new.last[key],old.last[key])
                self.assertEqual(new.process_plain(raw,{},5000,False),changed)
                self.assertEqual(Editor(schema).process_plain(raw,{},1010),raw)

    def test_current_profile_midstream_requires_validated_record_boundaries(self):
        schema,data=self.current_profile_record()
        framed=(0x80000000+len(data)).to_bytes(4,'big')+data
        from live_network_decoder import record_alignment
        self.assertIsNone(record_alignment(framed*2,schema,MAPS['3'],'outgoing'))
        self.assertEqual(record_alignment(b'junk'+framed*3,schema,MAPS['3'],'outgoing'),4)
        raw=stream(framed*3,10000);e=Editor(schema)
        self.assertNotEqual(e.process_plain(raw,plan(),1010),raw)
        self.assertEqual(e.stats['modifiedThrows'],1)
        # Unverified opcodes and incorrect acknowledgement schemas fail closed.
        unknown=data[:1]+b'\x01'+data[2:]
        self.assertEqual(Wire(schema,MAPS['4']).events(b'\x83'+unknown[1:],'outgoing'),[])
        schema.properties[0]['className']='Camera'
        with self.assertRaises(DecodeError):Wire(schema,MAPS['4']).events(b'\x83'+data[1:],'outgoing')

    def test_moving_qb_recalculates_regular_power_at_release(self):
        _,_,power=angle_velocity([0,5,0],[50,5,0],[0,0,0],40,28,.05,5)
        with self.assertRaises(DecodeError):velocity([-8,5,0],[50,5,0],[0,0,0],power,28,.05,False,5)
        p=plan();p.update(head=[0,5,0],targetPosition=[50,5,0],plannedPower=power,angle=40,anglePower=True,angleFallback=True,maxAngle=50)
        raw=body().replace(struct.pack('<fff',0,5,0),struct.pack('<fff',-8,5,0),1)
        plain=stream(bytes.fromhex('06010400000001')+(0x80000000+len(raw)).to_bytes(4,'big')+raw)
        e=editor();self.assertNotEqual(e.process_plain(plain,p,1010),plain)
        self.assertGreater(e.last['newPower'],power);self.assertLessEqual(e.last['newPower'],95)
        self.assertEqual(e.last['target'],[50,5,0])

    def test_regular_power_ceiling_falls_back_without_disappearing(self):
        launch,_=velocity([0,5,0],[100,5,0],[0,0,0],95,28,.05,False,5)
        angle=math.degrees(math.atan2(launch[1],math.hypot(launch[0],launch[2])))
        p=plan();p.update(targetPosition=[100,5,0],plannedPower=95,angle=angle,anglePower=True,angleFallback=True,maxAngle=50)
        raw=body().replace(struct.pack('<fff',0,5,0),struct.pack('<fff',-.1,5,0),1)
        plain=stream(bytes.fromhex('06010400000001')+(0x80000000+len(raw)).to_bytes(4,'big')+raw)
        e=editor();self.assertNotEqual(e.process_plain(plain,p,1010),plain);self.assertAlmostEqual(e.last['newPower'],95)
    def test_non_beam_angle_cap_resolves_power_and_preserves_endpoint(self):
        _,_,power=angle_velocity([0,5,0],[70,5,0],[0,0,0],55,28,.05,5)
        p=plan();p.update(plannedPower=power,lob=True,anglePower=False,maxAngle=50)
        e=editor();self.assertNotEqual(e.process_plain(payload(),p,1010),payload())
        launch=[a-b for a,b in zip(e.last['aim'],e.last['origin'])]
        self.assertLessEqual(math.degrees(math.atan2(launch[1],math.hypot(launch[0],launch[2]))),50);self.assertEqual(e.last['newPower'],int(e.last['newPower']))
        direction=[v/length(launch) for v in launch];t=e.last['flight'];power=e.last['newPower']
        landing=[e.last['origin'][i]+direction[i]*(5+power*t)-(.5*28*t*t if i==1 else 0) for i in range(3)]
        self.assertLess(length([a-b for a,b in zip(landing,[70,5,0])]),.001)
        self.assertEqual(e.last['target'],[70,5,0])
        for bad in (True,0,90,float('nan')):
            p['maxAngle']=bad;self.assertEqual(editor().process_plain(payload(),p,1010),payload())
    def test_redirect_keeps_running_route_and_delayed_release(self):
        origin=[36.6445732116699,7.48977041244507,86.8559722900391]
        receiver=dict(position=[31.4579086303711,7.48992252349854,201.211013793945],velocity=[0,0,20.0915598076771],speed=20.0915598076771,jump=6.37105004812896,dive=12,radius=5.88186461588846,above=3.88186461588846,below=4.88186461588846,worldGravity=196.199996948242,redirect=True)
        for horizon in (.5,1,2,3):
            on_route=[p+v*horizon for p,v in zip(receiver['position'],receiver['velocity'])]
            self.assertLess(receiver_gap(receiver,on_route,horizon),0)
        p=plan();p.update(head=origin,targetPosition=[31.3662753479695,15.6968909162813,198.248969542281],targetVelocity=receiver['velocity'],routePosition=receiver['position'],receiver=receiver,leadDistance=3,beam95=True,plannedPower=94.9106000879478,smartFit=True,coverageKnown=True,allowContested=True,catchers=[],latency=.0654540004730225,sampleAgeMs=210,clickPlanRetained=True)
        raw=body().replace(struct.pack('<fff',0,5,0),struct.pack('<fff',*origin),1)
        plain=stream(bytes.fromhex('06010400000001')+(0x80000000+len(raw)).to_bytes(4,'big')+raw)
        e=editor();self.assertNotEqual(e.process_plain(plain,p,1010),plain)
        # Advance the observed route, but never pull the planned placement back.
        expected=[x+v*.22 for x,v in zip(p['targetPosition'],p['targetVelocity'])]
        for a,b in zip(e.last['target'],expected):self.assertAlmostEqual(a,b)
        self.assertEqual(e.last['newPower'],int(e.last['newPower']));self.assertLess(abs(e.last['newPower']-p['plannedPower']),1)

    def test_auto_fixed_power_survives_origin_change_at_95(self):
        launch,t=velocity([0,5,0],[100,5,0],[0,0,0],95,28,.05,False,5)
        angle=math.degrees(math.atan2(launch[1],math.hypot(launch[0],launch[2])))
        with self.assertRaises(DecodeError):angle_velocity([-.1,5,0],[100,5,0],[0,0,0],angle,28,.05,5)
        p=plan();p.update(head=[-.1,5,0],targetPosition=[100,5,0],plannedPower=95,angle=angle,anglePower=False)
        raw=body().replace(struct.pack('<fff',0,5,0),struct.pack('<fff',-.1,5,0),1)
        plain=stream(bytes.fromhex('06010400000001')+(0x80000000+len(raw)).to_bytes(4,'big')+raw)
        e=editor();self.assertNotEqual(e.process_plain(plain,p,1010),plain)
        self.assertAlmostEqual(e.last['newPower'],95)

    def test_dive_burst_does_not_become_permanent_running_speed(self):
        d=dict(position=[0,8,0],velocity=[43,0,0],speed=22,jump=7,dive=12,radius=5.88,above=3.88,below=4.88,worldGravity=196.2)
        self.assertAlmostEqual(horizontal_travel(d,.1),4.3)
        self.assertAlmostEqual(horizontal_travel(d,2),56)
        d['velocity']=[0,0,0]
        self.assertAlmostEqual(horizontal_travel(d,2),56)

    def test_missed_catch_checks_the_rest_of_the_flight(self):
        origin=[0,8,0];launch,flight=velocity(origin,[70,16,0],[0,0,0],95,28,.05,False,5)
        db=dict(position=[125,8,0],velocity=[0,0,0],speed=22,jump=7,dive=12,radius=5.88,above=3.88,below=4.88,worldGravity=196.2)
        self.assertGreater(catch_path_margin(origin,launch,flight,95,28,.05,[db]),1)
        self.assertLess(catch_path_margin(origin,launch,flight,95,28,.05,[db],0),0)
        # Zero current speed cannot shrink the missed-catch envelope.
        db['speed']=0
        self.assertAlmostEqual(catch_path_margin(origin,launch,flight,95,28,.05,[db],0),-4.38)
        with self.assertRaises(DecodeError):catch_path_margin(origin,launch,flight,95,28,.05,[db],float('nan'))

    def test_comeback_requires_reaction_time_and_receiver_reach(self):
        receiver=dict(position=[70,5,0],velocity=[0,0,0],speed=20,jump=6.37,dive=12,radius=5.88,above=3.88,below=4.88,worldGravity=196.2,redirect=True)
        self.assertLess(receiver_gap(receiver,[60,5,0],1),0)
        self.assertGreater(receiver_gap(receiver,[60,5,0],.1),0)
        p=plan();p.update(targetPosition=[60,5,0],routePosition=[70,5,0],smartFit=True,coverageKnown=True,allowContested=True,catchers=[],receiver=receiver,groundY=0,checkMissedCatch=True)
        self.assertEqual(editor().process_plain(payload(),p,1010),payload()) # Too far to redirect in this short flight.
        p['targetPosition']=[64,5,0]
        e=editor();self.assertNotEqual(e.process_plain(payload(),p,1010),payload())
        receiver['speed']=0;self.assertEqual(editor().process_plain(payload(),p,1010),payload())
        receiver['redirect']='true'
        with self.assertRaises(DecodeError):validate_reach(receiver)

    def test_receiver_has_jump_reaction_and_comfort_margin(self):
        wr=dict(position=[0,8,0],velocity=[0,0,0],speed=20,jump=6.37,dive=12,radius=5.88,above=3.88,below=4.88,worldGravity=196.2,redirect=True)
        self.assertGreater(receiver_gap(wr,[0,14,0],.1),0)
        self.assertGreater(receiver_gap(wr,[0,17.5,0],1),0)
        self.assertLess(receiver_gap(wr,[0,13,0],.6),0)
        self.assertGreater(receiver_gap(wr,[20,8,0],3),0)

    def test_actual_dispatch_rechecks_missed_catch_risk(self):
        db=dict(position=[125,5,0],velocity=[0,0,0],speed=22,jump=7,dive=12,radius=5.88,above=3.88,below=4.88,worldGravity=196.2)
        p=plan();p.update(targetPosition=[70,13,0],plannedPower=95,smartFit=True,coverageKnown=True,allowContested=False,catchers=[db],groundY=-3)
        self.assertNotEqual(editor().process_plain(payload(),p,1010),payload())
        p['checkMissedCatch']=True
        self.assertEqual(editor().process_plain(payload(),p,1010),payload())
        p['allowContested']=True;e=editor()
        self.assertNotEqual(e.process_plain(payload(),p,1010),payload())
        self.assertTrue(e.last['contested']);self.assertLess(e.last['coverageMargin'],0)

    def test_auto_route_chest_pass_uses_fixed_power_below_five_degrees(self):
        p=plan();p.update(targetPosition=[65,3.5,0],targetVelocity=[0,0,20],plannedPower=95,anglePower=False,beam95=False,routePosition=[65,5,0],leadDistance=0)
        e=editor();self.assertNotEqual(e.process_plain(payload(),p,1010),payload())
        self.assertAlmostEqual(e.last['newPower'],95)
        launch,flight=velocity([0,5,0],p['targetPosition'],p['targetVelocity'],95,28,.05,False,5)
        self.assertLess(math.degrees(math.atan2(launch[1],math.hypot(launch[0],launch[2]))),5)
        self.assertAlmostEqual(5+launch[1]*(flight+5/95)-14*flight*flight,3.5)

    def test_stationary_receiver_cannot_sprint_to_rescue_placement(self):
        receiver=dict(position=[70,5,0],velocity=[0,0,0],speed=20,jump=6.37,dive=12,radius=5.88,above=3.88,below=4.88,worldGravity=196.2)
        self.assertLess(receiver_gap(receiver,[74,5,0],1),0)
        self.assertGreater(receiver_gap(receiver,[80,5,0],1),0)
        p=plan();p.update(targetPosition=[76,5,0],routePosition=[70,5,0],smartFit=True,coverageKnown=True,allowContested=True,catchers=[],receiver=receiver)
        self.assertEqual(editor().process_plain(payload(),p,1010),payload())
    def test_observed_dive_speed_remains_targetable(self):
        p=plan();p.update(beam95=True,plannedPower=95,targetVelocity=[0,0,43.5])
        self.assertNotEqual(editor().process_plain(payload(),p,1010),payload())
        p['targetVelocity']=[0,0,46];self.assertEqual(editor().process_plain(payload(),p,1010),payload())
    def test_moving_target_advances_by_sample_and_dispatch_delay(self):
        p=plan();p.update(beam95=True,plannedPower=95,targetVelocity=[0,0,20],routePosition=[70,5,0],sampleAgeMs=80)
        before=list(p['targetPosition']);e=editor()
        self.assertNotEqual(e.process_plain(payload(),p,1020),payload())
        self.assertAlmostEqual(e.last['target'][2],2.0)
        self.assertEqual(e.last['routeOffset'],(0.,0.,0.))
        self.assertEqual(p['targetPosition'],before)
        for age in (-1,101,float('nan')):
            p['sampleAgeMs']=age;self.assertEqual(editor().process_plain(payload(),p,1020),payload())
        p.update(clickPlanRetained=True,sampleAgeMs=250);e=editor()
        self.assertNotEqual(e.process_plain(payload(),p,1020),payload())
        self.assertAlmostEqual(e.last['target'][2],5.4)
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
    def test_sender_rejects_route_runaway_even_if_reachable(self):
        for moving in ([0,0,0],[0,0,20]):
            p=plan();p.update(routePosition=[70,5,0],targetVelocity=moving,targetPosition=[87,5,0])
            self.assertEqual(editor().process_plain(payload(),p,1010),payload())
            p['targetPosition']=[72,5,0]
            e=editor();self.assertNotEqual(e.process_plain(payload(),p,1010),payload());self.assertEqual(e.last['routeOffset'],(2.,0.,0.))
        self.assertEqual(placement_limit([0,0,0],8),6)
        self.assertLessEqual(placement_limit([0,0,20],8),6)
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
    def test_contested_assist_is_applied_when_explicitly_enabled(self):
        p=plan();p.update(smartFit=True,allowContested=True,coverageKnown=True,catchers=[dict(position=[70,5,0],velocity=[0,0,0],speed=22,jump=9,dive=12)])
        e=editor();out=e.process_plain(payload(),p,1010)
        self.assertNotEqual(out,payload());self.assertEqual(e.stats['modifiedThrows'],1)
        self.assertTrue(e.last['contested']);self.assertLessEqual(e.last['coverageMargin'],1)
        p['strictFit']=True
        self.assertEqual(editor().process_plain(payload(),p,1010),payload())
        p['strictFit']=False;p['routePosition']=[40,5,0]
        self.assertEqual(editor().process_plain(payload(),p,1010),payload())
        p['routePosition']=[70,5,0];p['coverageKnown']=False
        self.assertEqual(editor().process_plain(payload(),p,1010),payload())
    def test_signed_lead_is_applied_without_bypassing_fit_bound(self):
        for movement,bias in (([0,0,0],[-5,0,0]),([0,0,20],[0,0,-5])):
            p=plan();p.update(routePosition=[70,5,0],targetVelocity=movement,leadDistance=-5,targetPosition=[70+bias[0],5,bias[2]])
            e=editor();self.assertNotEqual(e.process_plain(payload(),p,1010),payload());self.assertEqual(e.last['routeOffset'],(0,0,0))
            self.assertEqual(e.last['target'],p['targetPosition'])
            for invalid in (13,-13,True,float('nan')):
                p['leadDistance']=invalid;self.assertEqual(editor().process_plain(payload(),p,1010),payload())
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
        aim=args[2]['components'];self.assertAlmostEqual(math.degrees(math.atan2(aim[1]-5,math.hypot(aim[0],aim[2]))),35,delta=.5);self.assertEqual(args[3],int(args[3]))
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
    def test_missed_assist_records_the_actual_gate(self):
        for change,reason in [(dict(active=False,selectionReason='Equip a football'),'No active'),(dict(clickAt=0),'physical click expired')]:
            p=plan();p.update(change);e=editor()
            self.assertEqual(e.process_plain(payload(),p,1010),payload())
            self.assertIn(reason,e.last_decline['reason'])
            self.assertEqual(e.last_decline['clickAt'],p['clickAt'])
            self.assertEqual(e.stats['declinedRecords'],1)
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
