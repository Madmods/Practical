"""Length-preserving edits of authenticated, schema-validated FF3 throw records.

No driver, process access or networking in this module. All other bytes remain
unchanged. Reliable stream edits are pinned for retransmissions, even when the
current aim plan is inactive. Fragmented fields are left to the game's aim.
"""
import math,struct,collections,time
from cryptography.hazmat.primitives.ciphers.aead import AESGCM
from network_transport_lab import DecodeError,Stream,frames,unprotect,recover_nonce,ecb
from network_wire_lab import Reader,Wire
from live_network_decoder import record_alignment,MAPS,REPLICATION_MODES

def vector(value):
    if not isinstance(value,(list,tuple)) or len(value)!=3 or any(isinstance(x,bool) or not isinstance(x,(int,float)) or not math.isfinite(x) or abs(x)>1e7 for x in value):raise DecodeError('Invalid plan vector')
    return tuple(float(x) for x in value)

def length(value):return math.sqrt(sum(x*x for x in value))

def velocity(origin,target,movement,power,gravity,latency,lob=False,spawn=0):
    origin,target,movement=map(vector,(origin,target,movement))
    if not 1<=power<=95 or not 1<=gravity<=300 or not 0<=latency<=.25 or length(movement)>45:raise DecodeError('Ballistic input bounds')
    distance=length(tuple(b-a for a,b in zip(origin,target)))
    if not 3<=distance<=600:raise DecodeError('Receiver distance bounds')
    if not math.isfinite(spawn) or not 0<=spawn<=5:raise DecodeError('Spawn offset bounds')
    def displacement(t):return tuple(target[i]+movement[i]*(t+latency)-origin[i]+(.5*gravity*t*t if i==1 else 0) for i in range(3))
    def at(t):return tuple(v*power/(power*t+spawn) for v in displacement(t))
    def error(t):return sum(v*v for v in displacement(t))-(power*t+spawn)**2
    def slope(t):return 2*sum(v*(movement[i]+(gravity*t if i==1 else 0)) for i,v in enumerate(displacement(t)))-2*power*(power*t+spawn)
    roots=[];previous=.03;prior=error(previous);prior_slope=slope(previous)
    def bracket(low,high,a,b):
        if not ((a>0 and b<=0) or (a<0 and b>=0)):return
        positive=a>0
        for _ in range(32):
            mid=(low+high)*.5
            if (error(mid)>0)==positive:low=mid
            else:high=mid
        root=(low+high)*.5
        if not roots or root-roots[-1]>1e-6:roots.append(root)
    for i in range(1,801):
        next_time=.03+i*.01;current=error(next_time);next_slope=slope(next_time)
        if prior_slope*next_slope<0:
            low,high=previous,next_time;positive=prior_slope>0
            for _ in range(36):
                mid=(low+high)*.5
                if (slope(mid)>0)==positive:low=mid
                else:high=mid
            mid=(low+high)*.5;value=error(mid);bracket(previous,mid,prior,value)
            if abs(value)<1e-7 and (not roots or mid-roots[-1]>1e-6):roots.append(mid)
            bracket(mid,next_time,value,current)
        else:bracket(previous,next_time,prior,current)
        previous,prior,prior_slope=next_time,current,next_slope
    if not roots:raise DecodeError('Receiver outside throw range')
    flight=roots[-1] if lob else roots[0];result=at(flight)
    if not .06<=flight<=8 or abs(length(result)-power)>.01:raise DecodeError('Invalid ballistic solution')
    return result,flight

def angle_velocity(origin,target,movement,angle,gravity,latency,spawn=0):
    origin,target,movement=map(vector,(origin,target,movement))
    if not all(math.isfinite(x) for x in (angle,gravity,latency)) or not 5<=angle<=85 or not 1<=gravity<=300 or not 0<=latency<=.25 or length(movement)>45:raise DecodeError('Angle plan bounds')
    if not 3<=length(tuple(b-a for a,b in zip(origin,target)))<=600:raise DecodeError('Receiver distance bounds')
    tangent=math.tan(math.radians(angle))
    if not math.isfinite(spawn) or not 0<=spawn<=5:raise DecodeError('Spawn offset bounds')
    def error(t):
        d=[target[i]+movement[i]*(t+latency)-origin[i] for i in range(3)]
        return math.hypot(d[0],d[2])*tangent-d[1]-.5*gravity*t*t
    previous=.06;prior=error(previous)
    for i in range(1,401):
        next_time=.06+i*.02;current=error(next_time)
        if prior>=0 and current<=0:
            low,high=previous,next_time
            for _ in range(32):
                mid=(low+high)*.5
                if error(mid)>0:low=mid
                else:high=mid
            t=(low+high)*.5
            launch=tuple((target[j]+movement[j]*(t+latency)-origin[j]+(.5*gravity*t*t if j==1 else 0))/t for j in range(3))
            raw_power=length(launch);power=raw_power-spawn/t
            if raw_power>0:launch=tuple(v*power/raw_power for v in launch)
            if t<=8 and 1<=power<=95:return launch,t,power
        previous,prior=next_time,current
    raise DecodeError('Selected angle exceeds available power')

def throw_fields(body,call,environment=None):
    if call['method']!='FireServer' or len(call['args']) not in (4,6) or call['args'][0]!='Clicked':return None
    # These are separate observed contracts, not interchangeable fallbacks.
    # A changed layout passes through; never infer or repair a new signature.
    expected={'practice':4,'match':6}.get(environment)
    if environment is not None and (expected is None or len(call['args'])!=expected):return None
    r=Reader(body);r.pos=call['recordOffset']+1;r.ref();r.integer(2)
    if r.uint()!=len(call['args']) or r.byte()!=2 or r.string()!='Clicked':return None
    if r.byte()!=22:return None
    origin=struct.unpack('<fff',r.take(12))
    if r.byte()!=22:return None
    aim_offset=r.pos;r.take(12)
    if r.byte()!=12:return None
    power_offset=r.pos;power=struct.unpack('<d',r.take(8))[0]
    # Live-match calls append two numeric fields. Keep their bytes untouched.
    for _ in range(len(call['args'])-4):
        if r.byte()!=12 or not math.isfinite(struct.unpack('<d',r.take(8))[0]):return None
    if r.pos!=call['consumed'] or not math.isfinite(power) or not 1<=power<=95:return None
    # Match tail values belong to the game and may vary. Their numeric wire
    # fields were validated above; preserve every byte without fixed constants.
    return origin,aim_offset,power_offset,power

def jump_rise(jump,gravity,horizon,vertical_speed=0):
    launch=math.sqrt(2*gravity*max(0,jump));t=min(max(0,horizon),launch/gravity)
    rise=launch*t-.5*gravity*t*t;t=min(max(0,horizon),max(0,vertical_speed)/gravity)
    return max(rise,max(0,vertical_speed)*t-.5*gravity*t*t)

def horizontal_travel(d,horizon):
    horizon=max(0,horizon);speed=max(0,min(45,d['speed']))
    burst=max(0,min(45,math.hypot(d['velocity'][0],d['velocity'][2]))-speed)*min(horizon,.35)
    future_dive=max(0,horizon-.1)*max(0,45-speed)
    return speed*horizon+min(d['dive'],max(burst,future_dive))

def reach_gap(d,p,horizon):
    position=d['position'];movement=d['velocity'];horizon=max(0,horizon)
    travel=horizontal_travel(d,horizon)
    vertical_radius=(d.get('above',6)+d.get('below',8))*.5;horizontal_radius=min(d.get('radius',6),vertical_radius)
    shoulder_radius=max(0,d.get('radius',6)-vertical_radius);shoulder_y=position[1]+(d.get('above',6)-d.get('below',8))*.5
    horizontal=max(0,math.hypot(p[0]-position[0],p[2]-position[2])-shoulder_radius-travel)
    top=shoulder_y+jump_rise(d['jump'],d.get('worldGravity',196.2),horizon,movement[1])
    bottom=shoulder_y+min(0,movement[1])*min(horizon,.25)
    vertical=max(0,p[1]-top,bottom-p[1])
    return (math.hypot(horizontal/horizontal_radius,vertical/vertical_radius)-1)*min(horizontal_radius,vertical_radius)

def validate_reach(d):
    if not isinstance(d,dict):raise DecodeError('Catcher metadata')
    if 'redirect' in d and not isinstance(d['redirect'],bool):raise DecodeError('Receiver redirect flag')
    vector(d['position']);movement=vector(d['velocity'])
    fields=((d['speed'],0,45),(d['jump'],0,42),(d['dive'],0,24),(d.get('radius',6),.1,12),(d.get('above',6),0,12),(d.get('below',8),0,12),(d.get('worldGravity',196.2),10,300))
    if any(isinstance(v,bool) or not isinstance(v,(int,float)) or not math.isfinite(v) or not lo<=v<=hi for v,lo,hi in fields):raise DecodeError('Catcher reach bounds')
    if d.get('above',6)+d.get('below',8)<.2:raise DecodeError('Empty catcher vertical extent')
    if math.hypot(movement[0],movement[2])>45:raise DecodeError('Catcher speed outside model')

def validate_field(field):
    if not isinstance(field,dict) or field.get('known') is not True:raise DecodeError('Field boundaries unavailable')
    vector(field.get('center'));right=vector(field.get('right'));forward=vector(field.get('forward'))
    for value,lo,hi in ((field.get('halfWidth'),30,150),(field.get('halfLength'),80,350)):
        if isinstance(value,bool) or not isinstance(value,(int,float)) or not math.isfinite(value) or not lo<=value<=hi:raise DecodeError('Field dimension bounds')
    if abs(right[1])>=.01 or abs(forward[1])>=.01 or abs(length(right)-1)>=.01 or abs(length(forward)-1)>=.01 or abs(sum(a*b for a,b in zip(right,forward)))>=.01:raise DecodeError('Field orientation bounds')

def field_contains(field,point,inset=0):
    delta=[p-c for p,c in zip(point,field['center'])]
    return abs(sum(a*b for a,b in zip(delta,field['right'])))<=field['halfWidth']-inset and abs(sum(a*b for a,b in zip(delta,field['forward'])))<=field['halfLength']-inset

def field_closest(field,point,inset=1):
    delta=[p-c for p,c in zip(point,field['center'])]
    x=max(-field['halfWidth']+inset,min(field['halfWidth']-inset,sum(a*b for a,b in zip(delta,field['right']))))
    z=max(-field['halfLength']+inset,min(field['halfLength']-inset,sum(a*b for a,b in zip(delta,field['forward']))))
    return [field['center'][0]+field['right'][0]*x+field['forward'][0]*z,point[1],field['center'][2]+field['right'][2]*x+field['forward'][2]*z]

def field_catch_allowed(field,point,margin=2):
    if isinstance(margin,bool) or not isinstance(margin,(int,float)) or not math.isfinite(margin) or not 0<=margin<=20:raise DecodeError('OOB limit bounds')
    nearest=field_closest(field,point,0)
    return math.hypot(point[0]-nearest[0],point[2]-nearest[2])<=margin+1e-6

def receiver_gap(receiver,point,horizon,field=None):
    if field is not None and not field_contains(field,receiver['position'],.5):return math.inf
    legal=lambda pose:field is None or field_contains(field,pose['position'],1)
    horizon=max(0,horizon)
    comfortable=dict(receiver);comfortable['jump']=comfortable['jump']*.85
    comfortable['radius']=max(.1,comfortable.get('radius',6)-.4);comfortable['above']=max(0,comfortable.get('above',6)-.25)
    jump_time=max(0,horizon-.18)
    pose=dict(comfortable);movement=receiver['velocity']
    following=dict(comfortable)
    following['position']=[receiver['position'][0]+movement[0]*horizon,receiver['position'][1],receiver['position'][2]+movement[2]*horizon]
    following.update(velocity=[0,movement[1],0],speed=0,dive=0)
    if math.hypot(movement[0],movement[2])>.5:following['radius']=following.get('radius',6)+min(3.0,max(0,horizon-.15)*4)
    route_gap=reach_gap(following,point,jump_time) if legal(following) else math.inf
    if receiver.get('redirect') is True:
        delay=.18+(.12 if math.hypot(movement[0],movement[2])>.5 else 0)
        pose['position']=[receiver['position'][0]+movement[0]*min(horizon,delay),receiver['position'][1],receiver['position'][2]+movement[2]*min(horizon,delay)]
        pose.update(velocity=[0,movement[1],0],speed=0,dive=0)
        speed=math.hypot(movement[0],movement[2])
        if speed>.5:
            available=max(0,horizon-delay)
            forward=(movement[0]/speed,0,movement[2]/speed);side=(-forward[2],0,forward[0])
            delta=[a-b for a,b in zip(field_closest(field,point) if field is not None else point,pose['position'])]
            dot=lambda a,b:sum(x*y for x,y in zip(a,b))
            progress=dot(delta,forward)
            if available<=0 or progress<speed*available*.55:return route_gap
            lateral=dot(delta,side);distance=math.hypot(progress,lateral)
            direction=[delta[0]/max(1e-9,distance),0,delta[2]/max(1e-9,distance)] if abs(lateral)<=progress*.7002075382097097 else [f*.8191520442889918+s*(-.573576436351046 if lateral<0 else .573576436351046) for f,s in zip(forward,side)]
            travel=max(0,min(max(0,min(45,min(receiver['speed'],speed)))*available,dot(delta,direction)))
            pose['position']=[p+d*travel for p,d in zip(pose['position'],direction)]
            return min(route_gap,reach_gap(pose,point,jump_time) if legal(pose) else math.inf)
        adjustment=min(8,max(0,horizon-delay)*max(0,min(45,receiver['speed']))*.5)
        if field is not None:
            delta=[a-b for a,b in zip(field_closest(field,point),pose['position'])];delta[1]=0
            scale=min(1,adjustment/max(1e-9,length(delta)));pose['position']=[p+d*scale for p,d in zip(pose['position'],delta)]
        else:pose['radius']=pose.get('radius',6)+adjustment
        return min(route_gap,reach_gap(pose,point,max(0,horizon-.18)) if legal(pose) else math.inf)
    return route_gap

def placement_limit(movement,flight,redirect=False):
    if redirect:return min(24,6+max(0,flight-.18)*14)
    return 6.0 if math.hypot(movement[0],movement[2])<.5 else 3.0+min(3.0,max(0,flight-.15)*4)

def catch_path_margin(origin,launch,flight,power,gravity,latency,catchers,ground=None):
    if not isinstance(catchers,list) or len(catchers)>128:raise DecodeError('Catcher coverage bounds')
    end=flight
    if ground is not None:
        if isinstance(ground,bool) or not isinstance(ground,(int,float)) or not math.isfinite(ground) or not -10000<=ground<=100000:raise DecodeError('Field height bounds')
        height=origin[1]+launch[1]*5/power-ground-.6
        disc=launch[1]*launch[1]+2*gravity*height
        if disc<0:raise DecodeError('No field intersection')
        end=(launch[1]+math.sqrt(disc))/gravity
        if not 0<end<=12:raise DecodeError('Field flight bounds')
    margin=1000.0;steps=max(32,min(400,math.ceil(end/.02)))
    for d in catchers:
        validate_reach(d)
        if ground is not None:d={**d,'speed':max(22,d['speed'])}
        for t in [flight]+[end*i/steps for i in range(steps+1)]:
            p=[origin[j]+launch[j]*(5/power+t)-(.5*gravity*t*t if j==1 else 0) for j in range(3)]
            margin=min(margin,reach_gap(d,p,t+latency))
    return margin

def _replacement(body,call,plan,tick):
    fields=throw_fields(body,call,plan.get('environment'))
    if not fields:
        if call['method']=='FireServer' and call['args'] and call['args'][0]=='Clicked':
            raise DecodeError('Throw arguments changed or unsupported; passed through unchanged')
        return None
    if plan.get('active') is not True or plan.get('mode')!='ff3-qb':raise DecodeError('No active assisted plan at throw')
    issued,expires,clicked=plan.get('issued',0),plan.get('expires',0),plan.get('clickAt',0)
    released=plan.get('releasedAt',0);held=plan.get('physicalHeld') is True
    if any(isinstance(v,bool) or not isinstance(v,(int,float)) or not math.isfinite(v) for v in (issued,expires,clicked,released)):raise DecodeError('Throw timing bounds')
    click_valid=clicked>0 and 0<=tick-clicked<=300000 and ((held and released==0) or (not held and ((released>=clicked and 0<=tick-released<=350) if released else tick-clicked<=350)))
    if not 0<=tick-issued<=150 or not issued<=tick<=expires<=issued+200 or not click_valid or clicked>issued or released>issued:raise DecodeError('Throw plan or physical click expired')
    # The target was sampled before planning, not when this packet arrived.
    # Advance only its measured route; retain old catcher positions and extend
    # their reach horizon by the same delay for conservative revalidation.
    sample_delay=0.0
    if 'sampleAgeMs' in plan:
        age=plan['sampleAgeMs']
        max_age=450 if plan.get('clickPlanRetained') is True else 100
        if isinstance(age,bool) or not isinstance(age,(int,float)) or not math.isfinite(age) or not 0<=age<=max_age:raise DecodeError('Target sample age bounds')
        sample_delay=(age+tick-issued)*.001
        movement=vector(plan['targetVelocity'])
        plan=dict(plan)
        for key in ('targetPosition','routePosition'):
            if key in plan:plan[key]=[p+v*sample_delay for p,v in zip(vector(plan[key]),movement)]
    origin,aim_offset,power_offset,old_power=fields
    if plan.get('requireWheelPower') is True:
        confirmed=plan.get('confirmedPower')
        if isinstance(confirmed,bool) or not isinstance(confirmed,(int,float)) or not math.isfinite(confirmed) or not 1<=confirmed<=95 or confirmed!=int(confirmed):
            raise DecodeError('Wheel power confirmation unavailable')
        if confirmed!=plan.get('plannedPower') or old_power!=confirmed:
            raise DecodeError('Wheel power differs from outgoing throw power')
    wire_origin=origin
    if len(call['args'])==6:
        head=vector(plan['head'])
        distance=lambda value:length(tuple(a-b for a,b in zip(value,head)))
        # Two independently observed live-match calls encoded X minus 6M.
        # Confirm against this client's native head before interpreting it.
        decoded=(origin[0]+6000000,origin[1],origin[2])
        if distance(origin)>16:
            if distance(decoded)>16:raise DecodeError('Unrecognized live-match origin encoding')
            origin=decoded

    if length(tuple(a-b for a,b in zip(vector(origin),vector(plan['head']))))>12:raise DecodeError('Throw origin disagrees with held character')
    power=plan.get('plannedPower',95.0) if plan.get('autoPower') is True else old_power
    if isinstance(power,bool) or not isinstance(power,(int,float)) or not math.isfinite(power) or not 1<=power<=95:raise DecodeError('Planned power bounds')
    beam=plan.get('beam95') is True
    if beam:
        if plan.get('autoPower') is not True:raise DecodeError('Beam requires calculated power')
        try:
            launch,flight=velocity(origin,plan['targetPosition'],plan['targetVelocity'],power,float(plan['gravity']),float(plan['latency']),bool(plan.get('lob')),spawn=5)
        except DecodeError:
            # Keep the selected elevation when a retained lower-power tier
            # loses range during release. Revalidate reach/coverage below.
            launch,flight,power=angle_velocity(origin,plan['targetPosition'],plan['targetVelocity'],float(plan.get('angle',math.nan)),float(plan['gravity']),float(plan['latency']),spawn=5)
        elevation=math.degrees(math.atan2(launch[1],math.hypot(launch[0],launch[2])))
        if not -30<=elevation<=85:raise DecodeError('Receiver outside planned Beam range')
    elif plan.get('anglePower') is True and plan.get('autoPower') is True:
        try:
            launch,flight,power=angle_velocity(origin,plan['targetPosition'],plan['targetVelocity'],float(plan['angle']),float(plan['gravity']),float(plan['latency']),spawn=5)
        except DecodeError:
            if plan.get('angleFallback') is not True:raise
            options=[]
            for high in (False,True):
                try:
                    candidate,t=velocity(origin,plan['targetPosition'],plan['targetVelocity'],95,float(plan['gravity']),float(plan['latency']),high,spawn=5)
                    elevation=math.degrees(math.atan2(candidate[1],math.hypot(candidate[0],candidate[2])))
                    if elevation<=plan.get('maxAngle',50):options.append((abs(elevation-float(plan['angle'])),candidate,t))
                except DecodeError:continue
            if not options:raise DecodeError('Receiver outside capped automatic throw range')
            _,launch,flight=min(options,key=lambda item:item[0]);power=95.0
    else:
        launch,flight=velocity(origin,plan['targetPosition'],plan['targetVelocity'],power,float(plan['gravity']),float(plan['latency']),bool(plan.get('lob')),spawn=5)
    max_angle=plan.get('maxAngle',85)
    if isinstance(max_angle,bool) or not isinstance(max_angle,(int,float)) or not math.isfinite(max_angle) or not 5<=max_angle<=85:raise DecodeError('Angle limit bounds')
    elevation=math.degrees(math.atan2(launch[1],math.hypot(launch[0],launch[2])))
    if elevation>max_angle+1e-6:
        if plan.get('autoPower') is not True:raise DecodeError('Throw exceeds angle limit')
        launch,flight,power=angle_velocity(origin,plan['targetPosition'],plan['targetVelocity'],max_angle,float(plan['gravity']),float(plan['latency']),spawn=5)
    min_angle=plan.get('minAngle',-30)
    if isinstance(min_angle,bool) or not isinstance(min_angle,(int,float)) or not math.isfinite(min_angle) or not -30<=min_angle<=max_angle:raise DecodeError('Minimum angle bounds')
    elevation=math.degrees(math.atan2(launch[1],math.hypot(launch[0],launch[2])))
    if elevation<min_angle-1e-6:
        if plan.get('autoPower') is not True or min_angle<5:raise DecodeError('Throw below route angle minimum')
        launch,flight,power=angle_velocity(origin,plan['targetPosition'],plan['targetVelocity'],min_angle,float(plan['gravity']),float(plan['latency']),spawn=5)
    # Select an integer power and solve again at that exact value. Merely
    # rounding the encoded number would leave destination/flight inconsistent.
    if plan.get('autoPower') is True:
        preferred=math.degrees(math.atan2(launch[1],math.hypot(launch[0],launch[2])))
        options=[]
        for integer in sorted({math.floor(power),math.ceil(power)}):
            if not 1<=integer<=95:continue
            for high in (False,True):
                try:
                    candidate,t=velocity(origin,plan['targetPosition'],plan['targetVelocity'],integer,float(plan['gravity']),float(plan['latency']),high,spawn=5)
                except DecodeError:continue
                angle=math.degrees(math.atan2(candidate[1],math.hypot(candidate[0],candidate[2])))
                if min_angle-1e-6<=angle<=max_angle+1e-6:
                    options.append((abs(angle-preferred),abs(integer-power),integer,candidate,t))
        if not options:raise DecodeError('No whole-power throw within angle bounds')
        _,_,power,launch,flight=min(options,key=lambda option:option[:2])
    elif power!=math.floor(power):
        raise DecodeError('Original power is not a whole number')
    offset=None
    anchor=plan.get('routePosition')
    if anchor is None and isinstance(plan.get('receiver'),dict):anchor=plan['receiver'].get('position')
    if anchor is not None:
        lead=plan.get('leadDistance',0)
        if isinstance(lead,bool) or not isinstance(lead,(int,float)) or not math.isfinite(lead) or not -12<=lead<=12:raise DecodeError('Lead distance bounds')
        base=vector(anchor);movement=vector(plan['targetVelocity']);direction=(movement[0],0,movement[2])
        if length(direction)<.5:direction=(base[0]-plan['head'][0],0,base[2]-plan['head'][2])
        norm=length(direction);bias=tuple(v*lead/norm for v in direction) if norm>.001 else (0,0,0)
        offset=tuple(a-b-c for a,b,c in zip(vector(plan['targetPosition']),base,bias))
        if plan.get('preserveCatchHeight') is True:
            height=plan.get('heightAdjustment')
            if isinstance(height,bool) or not isinstance(height,(int,float)) or not math.isfinite(height) or not -3<=height<=8:raise DecodeError('Configured catch height bounds')
            if offset[1]<height-1e-6:raise DecodeError('Fit lowers configured catch height')
        redirect=plan.get('smartFit') is True and isinstance(plan.get('receiver'),dict) and plan['receiver'].get('redirect') is True
        if math.hypot(offset[0],offset[2])>placement_limit(vector(plan['targetVelocity']),flight,redirect)+1e-6:raise DecodeError('Placement leaves receiver route')
    margin=None
    if plan.get('boundaryRequired') is True:
        field=plan.get('field');validate_field(field)
        validate_reach(plan.get('receiver'));receiver=dict(plan['receiver'])
        receiver['position']=[p+v*sample_delay for p,v in zip(vector(receiver['position']),vector(receiver['velocity']))]
        landing=[origin[i]+launch[i]*(5/power+flight)-(.5*float(plan['gravity'])*flight*flight if i==1 else 0) for i in range(3)]
        start=[origin[i]+launch[i]*5/power for i in range(3)]
        if not field_catch_allowed(field,landing,plan.get('boundaryMargin',2)):raise DecodeError('Catch point exceeds OOB limit')
        if not field_contains(field,start,-3) or receiver_gap(receiver,landing,flight+float(plan['latency']),field)>-.25:raise DecodeError('No receiver-reachable catch in bounds')
    if plan.get('smartFit') is True or plan.get('strictFit') is True:
        if plan.get('coverageKnown') is not True:raise DecodeError('Incomplete catcher coverage')
        # Recompute coverage at the actual origin. The app explicitly permits
        # contested fits; retain their measured risk instead of vetoing them.
        ground=plan['groundY'] if plan.get('checkMissedCatch') is True else None
        margin=catch_path_margin(origin,launch,flight,power,float(plan['gravity']),float(plan['latency'])+sample_delay,plan.get('catchers'),ground)
        if margin<=1.0 and (plan.get('allowContested') is not True or plan.get('strictFit') is True):raise DecodeError('No clear window at actual throw origin')
        if 'receiver' in plan:
            validate_reach(plan['receiver'])
            receiver=dict(plan['receiver'])
            receiver['position']=[p+v*sample_delay for p,v in zip(vector(receiver['position']),vector(receiver['velocity']))]
            landing=[origin[i]+launch[i]*(5/power+flight)-(.5*float(plan['gravity'])*flight*flight if i==1 else 0) for i in range(3)]
            if receiver_gap(receiver,landing,flight+float(plan['latency']))>-.25:raise DecodeError('Receiver cannot reach updated throw')
    aim=tuple(origin[i]+launch[i]*(10000/power) for i in range(3))
    edits=[(aim_offset,struct.pack('<fff',*aim))]
    if plan.get('autoPower') is True:edits.append((power_offset,struct.pack('<d',power)))
    return edits,dict(oldPower=old_power,newPower=power,origin=origin,wireOrigin=wire_origin,argumentCount=len(call['args']),aim=aim,flight=flight,spawnOffset=5,target=plan['targetPosition'],movement=plan['targetVelocity'],routeOffset=offset,instance=call['instance'],clickAt=clicked,coverageMargin=margin,contested=margin is not None and margin<=1.0,coverageUnits='studs')

# A changed release origin can invalidate the first native trajectory. Retry
# only the bounded alternatives already ranked by Smart Fit, each through the
# exact same contract, click expiry, angle, reach and boundary checks. Unknown
# arguments, parser failures and stale plans are never candidates for recovery.
_TRAJECTORY_DECLINES=frozenset((
    'Receiver outside throw range','Selected angle exceeds available power',
    'Receiver outside capped automatic throw range','Receiver outside planned Beam range',
    'No whole-power throw within angle bounds','Throw exceeds angle limit',
    'Throw below route angle minimum','Placement leaves receiver route',
    'Catch point exceeds OOB limit','No receiver-reachable catch in bounds',
    'No clear window at actual throw origin','Receiver cannot reach updated throw'))

def replacement(body,call,plan,tick):
    started=time.perf_counter()
    try:return _replacement(body,call,plan,tick)
    except DecodeError as original:
        if str(original) not in _TRAJECTORY_DECLINES or plan.get('smartFit') is not True or plan.get('autoPower') is not True:raise
        ranked=plan.get('rankedThrows')
        if not isinstance(ranked,list) or not 1<=len(ranked)<=3:raise
        try:
            target=vector(plan['targetPosition']);current=vector(plan['fitOffset'])
        except (DecodeError,KeyError,TypeError,ValueError):raise original
        attempted=set()
        for index,alternative in enumerate(ranked):
            elapsed=(time.perf_counter()-started)*1000
            if elapsed>=25:break
            if not isinstance(alternative,dict):continue
            power,angle,lob=alternative.get('power'),alternative.get('angle'),alternative.get('lob')
            if any(isinstance(v,bool) or not isinstance(v,(int,float)) or not math.isfinite(v) for v in (power,angle)):continue
            if not 1<=power<=95 or not -30<=angle<=85 or not isinstance(lob,bool):continue
            try:offset=vector(alternative['offset'])
            except (DecodeError,KeyError,TypeError,ValueError):continue
            identity=(offset,power,angle,lob)
            if identity in attempted:continue
            attempted.add(identity);retry=dict(plan)
            retry.update(targetPosition=[p-c+a for p,c,a in zip(target,current,offset)],fitOffset=list(offset),plannedPower=power,angle=angle,lob=lob)
            try:
                proposal=_replacement(body,call,retry,tick+int(math.ceil(elapsed)))
                if proposal is None:continue
                if (time.perf_counter()-started)*1000>=25:break
                edits,detail=proposal;detail.update(alternativeRank=index+1,primaryDecline=str(original));return edits,detail
            except (DecodeError,KeyError,TypeError,ValueError):continue
        raise original

class RecordTracker:
    def __init__(self,schema):self.schema=schema;self.streams={}
    def add(self,frame):
        if frame.stream not in self.streams:
            if len(self.streams)>=32:raise DecodeError('QB stream count budget')
            self.streams[frame.stream]={'stream':Stream(8*1024*1024,frame.offset),'buffer':bytearray(),'start':frame.offset,'ready':False,'blocked':False}
        state=self.streams[frame.stream]
        if state['blocked']:return []
        origin=state['stream'].origin
        if frame.offset<origin:
            skip=origin-frame.offset
            if skip>=len(frame.data):return []
            data=state['stream'].add(origin,frame.data[skip:],frame.fin)
        else:data=state['stream'].add(frame.offset,frame.data,frame.fin)
        state['buffer'].extend(data);buffer=state['buffer']
        if not state['ready']:
            if origin==0:
                if len(buffer)<7:return []
                if buffer[:3]!=bytes.fromhex('060104') or buffer[3:7]==b'ctrl':state['blocked']=True;return []
                del buffer[:7];state['start']+=7;state['ready']=True
            else:
                aligned=record_alignment(buffer,self.schema,MAPS['3'],'outgoing')
                if aligned is None:
                    if len(buffer)>65536:state['blocked']=True
                    return []
                del buffer[:aligned];state['start']+=aligned;state['ready']=True
        records=[]
        while len(buffer)>=4:
            size=int.from_bytes(buffer[:4],'big')&0x7fffffff
            if not 1<=size<=1024*1024:state['blocked']=True;raise DecodeError('QB record size bound')
            if len(buffer)<4+size:break
            records.append((frame.stream,state['start']+4,bytes(buffer[4:4+size])))
            del buffer[:4+size];state['start']+=4+size
        window=state['stream'];consumed=state['start']-window.origin
        if consumed>=65536:
            window.buffer=window.buffer[consumed:];window.present=window.present[consumed:];window.contiguous-=consumed;window.delivered-=consumed;window.origin+=consumed
            if window.final is not None:window.final-=consumed
        return records

class Editor:
    def __init__(self,schema):
        self.schema=schema;self._reset_reader();self.ledger=[];self.claimed=set();self.stats=collections.Counter();self.last=None;self.last_decline=None;self.covered=set()
        self.recent=collections.deque();self.recent_bytes=0;self.parser_failures=collections.Counter();self.last_recovery=-10000;self.recovering=False
        self.history=collections.deque(maxlen=8);self.decision_sequence=0
        self.format_verified=False;self.throw_diagnostics=collections.deque(maxlen=8);self.diagnostic_sequence=0;self.diagnostic_seen=collections.deque(maxlen=32)
    def _trace_throw(self,stream,start,body,plan,tick):
        clicked=plan.get('clickAt')
        if plan.get('mode')!='ff3-qb' or isinstance(clicked,bool) or not isinstance(clicked,(int,float)) or not math.isfinite(clicked) or clicked<=0 or not 0<=tick-clicked<=2000:return
        identity=(clicked,stream,start)
        if identity in self.diagnostic_seen:return
        self.diagnostic_seen.append(identity);self.diagnostic_sequence+=1
        context={k:plan[k] for k in ('environment','plannedPower','head','targetPosition','targetVelocity','angle','gravity','latency','clickAt','issued') if k in plan}
        entry=dict(sequence=self.diagnostic_sequence,at=tick,stream=stream,offset=start,recordMode=REPLICATION_MODES.get(body[0]) if body else None,recordBytes=len(body),recordHex=body[:4096].hex(),truncated=len(body)>4096,containsClicked=b'Clicked' in body,plan=context)
        # Preserve a throw-bearing record instead of replacing it with the next
        # stream of acknowledgements. Other records retain the earliest samples.
        if len(self.throw_diagnostics)<8:self.throw_diagnostics.append(entry)
        elif entry['containsClicked']:
            index=next((i for i,d in enumerate(self.throw_diagnostics) if not d['containsClicked']),0)
            del self.throw_diagnostics[index];self.throw_diagnostics.append(entry)
    def _decision(self,outcome,reason,plan,tick,call,power=None,detail=None):
        # One bounded summary per decision, never a copy of the wire arguments.
        clicked=plan.get('clickAt',0)
        if plan.get('mode')!='ff3-qb' or not isinstance(clicked,(int,float)) or isinstance(clicked,bool) or not math.isfinite(clicked) or clicked<=0:return
        key=(clicked,str(call.get('instance')),outcome,reason)
        if any(item['_key']==key for item in self.history):return
        self.decision_sequence+=1
        self.history.append(dict(_key=key,sequence=self.decision_sequence,at=tick,outcome=outcome,reason=reason,power=power,detail=str(detail)[:160] if detail else None))
    def decision_history(self):
        return [{k:v for k,v in item.items() if k!='_key'} for item in self.history]
    def _reset_reader(self):
        self.wires={mode:Wire(self.schema,MAPS[mode]) for mode in set(REPLICATION_MODES.values())}
        self.wire=self.wires['3'];self.profile_wire=self.wires['4'];self.tracker=RecordTracker(self.schema)
    def _calls(self,body):
        mode=REPLICATION_MODES.get(body[0]) if body else None
        return self.wires[mode].events(b'\x83'+body[1:],'outgoing') if mode else []
    def _recover(self,tick):
        # Reconstruct parser state from recent authenticated plaintext only.
        # Never replay a game input or evaluate an old plan. Accepted reliable
        # edits and their click ownership remain pinned for retransmissions.
        self._reset_reader();decoded=0
        for plain in self.recent:
            try:spans=frames(plain)
            except DecodeError:continue
            for frame in spans:
                try:records=self.tracker.add(frame)
                except DecodeError:continue
                for stream,start,body in records:
                    try:decoded+=len(self._calls(body))
                    except (DecodeError,UnicodeError,KeyError,ValueError,TypeError):pass
        self.parser_failures.clear();self.last_recovery=tick;self.recovering=not bool(decoded);self.stats['parserRecoveries']+=1
        self.stats['recoveredCalls']+=decoded
    def update_schema(self,schema,tick):
        self.schema=schema;self.format_verified=False;self._recover(tick);self.stats['schemaRefreshes']+=1
    def _decline(self,error,plan,tick,body,parser=False):
        self.stats['declinedRecords']+=1
        self.last_decline=dict(reason=str(error)[:160],at=tick,clickAt=plan.get('clickAt'),planActive=plan.get('active'),selectionReason=plan.get('selectionReason'),sampleAgeMs=plan.get('sampleAgeMs'),lockedTarget=plan.get('lockedTarget'),selectedTarget=plan.get('selectedTarget'),angle=plan.get('angle'),anglePower=plan.get('anglePower'),plannedPower=plan.get('plannedPower'),input=plan.get('input'),recordMode=REPLICATION_MODES.get(body[0]) if body else None,parserFailure=parser,property=getattr(error,'context',None))
    def process_plain(self,plain,plan,tick,allow=True):
        spans=frames(plain);candidate=bytearray(plain);records=[]
        if len(plain)<=1024*1024:
            self.recent.append(bytes(plain));self.recent_bytes+=len(plain)
            while len(self.recent)>256 or self.recent_bytes>1024*1024:self.recent_bytes-=len(self.recent.popleft())
        for frame in spans:
            try:records.extend(self.tracker.add(frame))
            except DecodeError:self.stats['trackingFailures']+=1
        recover=False
        for stream,start,body in records:
            self._trace_throw(stream,start,body,plan,tick)
            if not body or body[0] not in REPLICATION_MODES:continue
            try:calls=self._calls(body)
            except (DecodeError,UnicodeError,KeyError,TypeError,ValueError) as error:
                self.recovering=True
                self._decline(error,plan,tick,body,True);self.parser_failures[stream]+=1
                recover=recover or (self.parser_failures[stream]>=3 and tick-self.last_recovery>=5000)
                continue
            if calls:self.recovering=False
            for call in calls:
                self.stats['decodedCalls']+=1
                if call['method']=='FireServer' and call['args'] and call['args'][0]=='Clicked':self.stats['clickedCalls']+=1
                try:
                    if throw_fields(body,call,plan.get('environment')) is not None:self.format_verified=True;self.stats['validatedThrowCalls']+=1
                except (DecodeError,UnicodeError,KeyError,TypeError,ValueError):pass
                # Setup/draining still primes dictionaries, but never creates
                # an edit or consumes a physical click from a previous record.
                if not allow or self.recovering:continue
                try:
                    proposal=replacement(body,call,plan,tick)
                    if not proposal:continue
                    edits,detail=proposal
                    if detail['clickAt'] in self.claimed or len(self.ledger)+len(edits)>2048:continue
                    planned=[]
                    for offset,value in edits:
                        absolute=start+offset;positions={}
                        for frame in spans:
                            if frame.stream!=stream:continue
                            for at in range(max(absolute,frame.offset),min(absolute+len(value),frame.offset+len(frame.data))):positions.setdefault(at,[]).append(frame.plain_offset+at-frame.offset)
                        if any(at not in positions for at in range(absolute,absolute+len(value))):break
                        planned.append((stream,absolute,value,detail['clickAt']))
                    else:
                        self.ledger.extend(planned);self.claimed.add(detail['clickAt']);self.last=detail;self.stats['modifiedThrows']+=1
                        self._decision('assisted','Assist applied',plan,tick,call,detail['newPower'],('Validated alternative '+str(detail['alternativeRank'])) if 'alternativeRank' in detail else None)
                except (DecodeError,UnicodeError,KeyError,TypeError,ValueError) as error:
                    self._decline(error,plan,tick,body)
                    reason=str(error)
                    label=('OOB limit' if 'OOB limit' in reason else 'Arguments changed' if 'arguments changed' in reason else
                           'Wheel not ready' if reason.startswith('Wheel power') else
                           'Plan expired' if 'expired' in reason or 'sample age' in reason else
                           'Assist not ready' if reason.startswith('No active') else
                           'Ball position changed' if 'origin' in reason.lower() else 'No valid assisted throw')
                    power=call['args'][3] if len(call['args'])>3 and isinstance(call['args'][3],(int,float)) and not isinstance(call['args'][3],bool) and math.isfinite(call['args'][3]) else None
                    self._decision('normal',label,plan,tick,call,power,reason)
        if recover:self._recover(tick)
        touched=False;coverage=collections.defaultdict(list);self.covered=set()
        for stream,start,value,clicked in self.ledger:
            present=set()
            for frame in spans:
                if frame.stream!=stream:continue
                lo=max(start,frame.offset);hi=min(start+len(value),frame.offset+len(frame.data))
                if lo<hi:
                    at=frame.plain_offset+lo-frame.offset;candidate[at:at+hi-lo]=value[lo-start:hi-start];touched=True
                    present.update(range(lo,hi))
            coverage[clicked].append(len(present)==len(value))
        self.covered={clicked for clicked,parts in coverage.items() if all(parts)}
        if touched:self.stats['rewrittenPackets']+=1
        return bytes(candidate)

def authenticated_plain(udp,context):
    u=unprotect(udp,context['hp'],31,context['largest'])
    if u.key_phase!=context['phase']:raise DecodeError('QB connection changed')
    recovered=recover_nonce(u,context['key'])
    if recovered is None:raise DecodeError('QB packet authentication failed')
    context['cid']=u.cid
    context['largest']=max(context['largest'],u.packet_number)
    return u,*recovered

def encrypt_changed(udp,u,nonce,plain,context):
    encrypted=AESGCM(context['key']).encrypt(nonce,plain,u.header);packet=bytearray(u.header+encrypted)
    if len(packet)!=len(udp)-31:raise DecodeError('QB packet length changed')
    mask=ecb(context['hp'],bytes(packet[25:41]));packet[0]^=mask[0]&31
    for index in range(len(u.header)-21):packet[21+index]^=mask[index+1]
    return udp[:31]+packet
