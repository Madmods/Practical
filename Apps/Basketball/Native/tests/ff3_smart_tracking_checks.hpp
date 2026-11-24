#pragma once
template<class Check> void smartTrackingChecks(Check check){
 using namespace ff3Assist;
 check(catchFormation({-1,.5,-1.5},{1,.5,-1.5},{2,2,1}),"both forward hands are a measured catch formation");
 check(catchFormation({-.5,2.1,-.2},{.5,2.1,-.2},{2,2,1}),"an overhead catch is included in the hands-out gate");
 check(!catchFormation({-1.5,-1,0},{1.5,-1,0},{2,2,1})&&!catchFormation({-1,.5,-1.5},{1,-1,0},{2,2,1}),"lowered arms and one swinging running arm cannot open the gate");
 check(!catchFormation({NAN,1,-2},{1,1,-2},{2,2,1})&&!catchFormation({-1,.5,1},{1,.5,1},{2,2,1}),"unknown or behind-the-torso hands cannot authorize catch-only boost");
 CatchPoseGate gate;CatchPose observed;observed.measured=observed.handsOut=true;
 check(gate.update(observed,10),"an observed catch pose opens immediately without adding a takeoff delay");observed.handsOut=false;
 check(gate.update(observed,10.02)&&!gate.update(observed,10.041),"brief animation flicker has a bounded 40 ms bridge, then hands-down closes the gate");observed.handsOut=true;gate.update(observed,11);observed.measured=false;check(!gate.update(observed,11.001),"missing pose geometry closes the gate immediately");
 auto target=[](double at,double point,double needed=4.25){DistanceDiveTarget t{true,100,{0,8,point},at,at-10,point,DiveTargetKind::Intercept};t.catchable=true;t.requiredExtra=needed;t.forwardClearance=1;return t;};
 for(double frame:{.008,.016,.033})for(double latency:{0.,.016,.032}){
  DirectionalBoost control;auto t=target(10.3,17,4.1);check(control.beginTracked({0,-5,43},{0,8,0},t,10,12),"tracked Smart control starts with its hard limit and a timed contact");
  Vec position{0,8,0},velocity{0,-5,43};std::vector<std::pair<double,Vec>> queue;double time=10;bool ended=false;
  while(time<10.3-1e-6){auto command=ended?VelocitySample{true,true,{},velocity}:control.sampleTracked(position,velocity,time);check(command.valid&&command.velocity.y==-5&&std::abs(command.velocity.x)<1e-6&&command.velocity.z>=0&&command.velocity.z<=120.001,"Smart feedback preserves vertical velocity and the original forward ray at different frame rates");
   if(!ended){queue.push_back({time+latency,command.velocity});ended=command.complete;}
   const double dt=std::min(frame,10.3-time);for(const auto& pending:queue)if(pending.first<=time+.00001)velocity=pending.second;position=position+flat(velocity)*dt;time+=dt;
  }
  check(std::abs(position.z-17)<1.45&&control.spent<=12.05,"tracked propulsion arrives within a physical catch envelope under frame and actuator delay variation");
 }
 {DirectionalBoost c;auto t=target(10.3,17);c.beginTracked({0,-5,43},{0,8,0},t,10,12);auto first=c.sampleTracked({0,8,0},{0,-5,43},10);auto faster=c.sampleTracked({0,8,9},{0,-5,90},10.12);check(first.valid&&faster.valid&&faster.velocity.z<first.velocity.z,"observed forward progress reduces the command instead of continuing a fixed boost budget past the catch");auto passed=c.sampleTracked({0,8,19},faster.velocity,10.2);check(passed.complete&&passed.velocity.z>=0&&passed.velocity.z<=43.001,"a passed Smart contact removes extra propulsion without reversing the native dive");}
 {DirectionalBoost c;auto t=target(10.3,17);c.beginTracked({0,-5,43},{0,8,0},t,10,12);for(int i=0;i<9;++i)c.sampleTracked({0,8,0},{0,-5,43},10+i*.008);check(c.spent==0,"polling the same physics position never charges commanded motion as observed travel");check(c.sampleTracked({0,8,0},{0,-5,43},10.1).complete,"a prolonged stale position stops Smart propulsion rather than accelerating blindly");}
 {DirectionalBoost c;auto t=target(10.3,12.9,0);check(c.beginTracked({0,-5,43},{0,8,0},t,10,12),"a native-speed catch remains armed even when it initially needs zero extra boost");auto command=c.sampleTracked({0,8,0},{0,-5,43},10);check(command.valid&&command.velocity.z<=43.001&&!command.complete,"tracking a native catch does not add unnecessary speed");auto changed=target(10.3,16,3.1);check(c.retargetTracked(changed,10.05),"a later ball prediction can request extra travel during the same armed dive");auto adapt=c.sampleTracked({0,8,2.15},{0,-5,43},10.05);check(adapt.valid&&adapt.velocity.z>43,"Smart can begin boosting after a zero-extra initial prediction");auto other=changed;other.ball=200;check(!c.retargetTracked(other,10.06),"an active Smart dive cannot switch to another football");c.missingTarget(10.07);auto coast=c.sampleTracked({0,8,3},{0,-5,60},10.07);check(coast.valid&&!coast.complete&&coast.velocity.z<=43.001,"a brief missing prediction coasts at native speed instead of adding stale boost");c.missingTarget(10.1);check(c.sampleTracked({0,8,4},{0,-5,43},10.1).complete,"missing predictions end the action after the short coast interval");}
 {DirectionalBoost c;auto t=target(10.3,17);c.beginTracked({0,-5,43},{0,8,0},t,10,12);auto issued=c.sampleTracked({0,8,0},{0,-5,43},10);c.retargetTracked(target(10.3,12.9,0),10.02);c.sampleTracked({0,8,1.2},issued.velocity,10.02);auto stop=c.sampleTracked({0,8,18},issued.velocity,10.04);check(stop.complete&&stop.velocity.z<=43.001,"finishing removes a recent in-flight boost command even if a newer lower command has not reached physics yet");}
 {DirectionalBoost c;auto t=target(10.3,17,20);check(!c.beginTracked({0,0,43},{0,8,0},t,10,12),"a contact outside the maximum extra-distance limit cannot arm Smart Boost");}
 ff3Play::Scene s;s.valid=s.focused=true;s.menu=s.typing=false;s.pid=1;s.model=2;s.own=3;s.player=4;s.at=10;s.head={0,10,0};s.rootPosition={0,8,0};s.groundY=0;s.velocity={0,0,25};s.worldGravity=196.2;
 ff3qb::BallObservation closeBall{100,{5,10,2},{},196.2,false,99,true},forwardBall{200,{0,10,10},{},196.2,false,99,true};s.looseBalls={closeBall,forwardBall};std::array<ContactReach,2> hands{{{{},.7,true},{{},.7,true}}};
 auto chosen=selectSmartBoostTarget(s,true,10,40,12,{0,0,1},25,hands);check(chosen.valid&&chosen.ball==200,"an uncatchable closer off-path ball does not mask a reachable pass on the dive ray");
 check(!selectSmartBoostTarget(s,true,10,40,12,{0,0,1},25,hands,100).valid,"a pinned pass cannot be silently replaced when it becomes unreachable");
 s.looseBalls={forwardBall};s.looseBalls[0].position.x=1.5;hands={ContactReach{{-1.5,0,0},.7,true},ContactReach{{1.5,0,0},.7,true}};
 {auto bad=s;bad.looseBalls[0].radius=NAN;check(!selectSmartBoostTarget(bad,true,10,40,12,{0,0,1},25,hands).valid,"unknown football size cannot authorize a Smart contact");}
 auto right=selectSmartBoostTarget(s,true,10,40,12,{0,0,1},25,hands);check(right.valid,"Smart tests both measured hands instead of rejecting a catch reachable by the other hand");
}
