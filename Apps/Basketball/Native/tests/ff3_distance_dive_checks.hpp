#pragma once
template<class Check> void distanceDiveChecks(Check check){
 using namespace ff3Assist;
 ff3Play::Scene s;s.valid=s.focused=s.grounded=true;s.typing=s.menu=false;s.pid=1;s.model=2;s.own=3;s.player=4;s.at=10;s.groundY=0;s.head={0,5,0};s.rootPosition={0,3.5,0};s.standingHeadHeight=5;s.standingRootHeight=3.5;s.velocity={0,0,-25};s.speed=22;s.worldGravity=196.2;s.jumpVelocity=50;s.naturalShape={2,2,3,true};s.cameraForward={0,0,-1};s.cameraRight={1,0,0};
 ff3qb::BallObservation ball{100,{0,5,-8},{},28,false,99,true};ContactReach contact{{},1.25,true};
 auto target=distanceDiveTarget(s,ball,10,30,7,25,contact);check(target.valid&&target.kind==DiveTargetKind::Intercept,"distance-based targeting finds a reachable ball interception");
 check(!diveLaunchReady(10,10.03,8,0,{0,0,20}),"an E press plus 25 milliseconds cannot mistake running for a launched dive");
 check(diveLaunchReady(10,10.03,14,1,{5.09,-8.42,42.7}),"native dive velocity can start assistance on the first update");
 check(diveLaunchReady(10,10.004,5,0,{0,35,20}),"a fresh E activation starts steering while airborne before the ragdoll flag arrives");
 {auto air=s;air.grounded=false;air.rootPosition.y=8;check(diveAirborne(air),"a jumping root can start immediate in-air dive assistance");air.rootPosition.y=1.2;check(!diveAirborne(air),"a prone body on the field cannot activate late even when FloorMaterial or ragdoll flags remain airborne");}
 check(!diveLaunchReady(10,10.32,14,1,{0,0,43})&&!diveLaunchReady(0,10.1,5,0,{0,0,43})&&!diveLaunchReady(10,10.1,14,1,{0,0,43},false)&&!diveLaunchReady(10,10.1,14,1,{}),"an old press, missing press, grounded ragdoll or stationary body cannot start late assistance");
 auto retained=target;retained.at=10.5;check(retainDiveTarget(retained,10,10.06)&&!retainDiveTarget(retained,10,10.11)&&!retainDiveTarget(retained,10,retained.at),"a brief missing ball sample retains only the existing future intercept for at most 100 milliseconds");
 check(!distanceDiveTarget(s,ball,10,3,7,25,contact).valid,"Assist distance rejects a target outside the limit");
 auto high=ball;high.position={0,20,-8};auto landing=distanceDiveTarget(s,high,10,30,7,25,contact);check(landing.valid&&landing.kind==DiveTargetKind::Approach,"a high pass supplies a near-term direction before hand-height interception or distant landing is possible");
 auto loose=ball;loose.position={0,.6,-9};loose.velocity={};auto floor=distanceDiveTarget(s,loose,10,30,7,25,contact);check(floor.valid&&floor.kind==DiveTargetKind::Loose,"a loose floor ball produces a floor target instead of an imaginary airborne catch");
 auto thrown=ball;thrown.position={10,20,-12};thrown.velocity={10,7,-4};const double eta=diveLandingTime(s,thrown,10);auto point=divePoint(s,thrown,eta,10);check(std::isfinite(eta)&&std::abs(point.y-.6)<.1,"landing time and landing position solve the same ballistic trajectory");
 for(int bad=0;bad<6;++bad){auto scene=s;auto b=ball;if(bad==0)scene.focused=false;if(bad==1)b.anchored=true;if(bad==2)b.verifiedBall=false;if(bad==3)scene.possessed=true;if(bad==4)b.position.x=NAN;if(bad==5)scene.at=9;check(!distanceDiveTarget(scene,b,10,30,7,25,contact).valid,"invalid or stale ball scenes cannot drive dive motion");}
 // Assist turns even an incorrectly aimed dive, without adding horizontal speed.
 DistanceDive assist;DistanceDiveTarget right{true,100,{10,3.5,0},10.4,.4,10,DiveTargetKind::Intercept};
 check(!assist.begin(s.rootPosition,{-30,-8,0},right,10,true,false,0,30),"Assist rejects a target behind the player's activation movement heading");
 check(assist.begin(s.rootPosition,{0,-8,-30},right,10,true,false,0,30),"Assist can target either side without reversing the activation heading");
 auto redirected=assist.sample(s.rootPosition,{0,-8,-30},10.05,right);check(redirected.valid&&!redirected.complete&&redirected.velocity.x>29.9&&redirected.velocity.z<=.001&&std::abs(ff3qb::length(flat(redirected.velocity))-30)<1e-6&&redirected.velocity.y==-8,"Assist steers sideways while preserving speed, vertical motion and the forward half-plane");
 DistanceDive boost;check(boost.begin(s.rootPosition,{0,-8,-25},right,10,false,true,2,30),"Boost operates with Assist disabled");
 auto pushed=boost.sample(s.rootPosition,{0,-8,-25},10.05,right);check(pushed.valid&&pushed.velocity.x>0&&pushed.velocity.z<0&&pushed.velocity.y==-8,"Boost adds a ball-directed component without rotating all of the original dive velocity");
 const double commandedExtra=boost.appliedExtra;auto sparse=boost.sample(s.rootPosition,pushed.velocity,10.058,right);
 check(sparse.valid&&std::abs(boost.spent-commandedExtra*.008)<1e-8,"a repeated physics position sample charges issued extra velocity, not the normal dive's unsampled displacement");
 DistanceDive both;check(both.begin(s.rootPosition,{0,-8,-25},right,10,true,true,7,30),"Assist and Boost compose into one movement controller");auto combined=both.sample(s.rootPosition,{0,-8,-25},10.05,right);check(combined.velocity.x>25&&std::abs(combined.velocity.z)<.01&&combined.velocity.y==-8,"combined mode steers the dive and adds distance along the target direction");
 {DistanceDive early;check(early.begin(s.rootPosition,{0,20,-43},right,10,true,true,7,30),"first-update motion fixture starts");auto first=early.sample(s.rootPosition,{0,20,-43},10,right);check(first.valid&&first.velocity.x>0&&early.appliedExtra>0&&first.velocity.y==20,"the first command already steers and boosts instead of spending its first update unchanged");}
 {DistanceDive proactive;DistanceDiveTarget close{true,100,{8,3.5,0},10.4,.4,8,DiveTargetKind::Intercept};check(proactive.begin(s.rootPosition,{43,0,0},close,10,false,true,7,30),"already-reachable target boost fixture starts");auto first=proactive.sample(s.rootPosition,{43,0,0},10,close);check(first.velocity.x>43&&proactive.appliedExtra>0,"Boost adds selected extra travel even when normal speed could reach the target by its predicted time");}
 // The distance budget integrates issued extra velocity, not position-sample
 // jitter. Target updates cannot reset it or extend the action clock.
 for(double budget:{1.,3.,7.,15.}){
  DistanceDive control;DistanceDiveTarget distantTarget{true,100,{0,3.5,-40},10.5,.5,40,DiveTargetKind::Landing};Vec position=s.rootPosition,velocity{0,-4,-25};check(control.begin(position,velocity,distantTarget,10,true,true,budget,60),"distance budget fixture starts");const double deadline=control.until;double extraTravel=0;bool ended=false;
  for(int i=1;i<=90;++i){const double time=10+i*.008;auto refreshed=distantTarget;refreshed.at=time+.3;check(control.retarget(refreshed,position,time)&&control.until==deadline,"live retargeting does not restart the boost duration");auto command=control.sample(position,velocity,time,refreshed);check(command.valid&&command.velocity.y==velocity.y&&ff3qb::length(flat(command.velocity))<=120.001,"composed dive output preserves vertical motion and bounds speed");
   if(command.complete){check(ff3qb::length(flat(command.velocity))<=25.001,"completion removes boost contribution while preserving ordinary dive speed");ended=true;break;}
   extraTravel+=std::max(0.,ff3qb::length(flat(command.velocity))-25)*.008;position=position+command.velocity*.008;velocity=command.velocity;
  }
  check(ended&&extraTravel<=budget+.5&&control.spent<=budget+.5,"a boost cannot repeatedly spend its extra travel distance");
 }
 // Passing the chosen target never produces a positional pull backward.
 DistanceDive passed;check(passed.begin(s.rootPosition,{0,-5,-25},target,10,true,true,7,30),"passed-target fixture starts");auto end=passed.sample({0,3.5,-30},{0,-5,-35},10.1,target);check(end.complete&&end.velocity.z<0&&end.velocity.y==-5,"passing the target ends assistance without reversing the dive");
 auto missing=right;missing.valid=false;check(!assist.retarget(missing,s.rootPosition,10.06),"a removed target ends steering instead of selecting a random replacement");auto stopped=assist.sample(s.rootPosition,{30,-8,0},10.07,missing);check(stopped.complete&&stopped.velocity.x==30&&stopped.velocity.y==-8,"loss of a target preserves the underlying normal dive");
 check(!both.begin(s.rootPosition,{0,0,-25},right,10,true,true,NAN,30)&&!both.begin(s.rootPosition,{0,0,-25},right,10,true,true,7,3),"nonfinite distance and out-of-range starts are rejected");
 // A centered route must not stop a full arm length away from the ball.
 for(bool steering:{false,true}){DistanceDive control;DistanceDiveTarget diagonal{true,100,{20,3.5,0},10.5,.5,20,DiveTargetKind::Landing};Vec position=s.rootPosition,velocity{0,0,-25};check(control.begin(position,velocity,diagonal,10,steering,true,3,60),"sideways boost budget fixture starts");double extraTravel=0;
  for(int i=1;i<=90;++i){const double time=10+i*.008;auto movingTarget=diagonal;movingTarget.at=time+.3;control.retarget(movingTarget,position,time);auto command=control.sample(position,velocity,time,movingTarget);check(command.valid,"sideways boost produces a finite command");if(command.complete)break;extraTravel+=ff3qb::length(flat(command.velocity)-control.lastBase)*.008;position=position+command.velocity*.008;velocity=command.velocity;}
  check(extraTravel<=3.5&&control.spent<=3.5,"extra distance is bounded for sideways Boost-only and combined steering, not just straight dives");}
 auto rotated=s;rotated.head.x=2;auto rotatedFloor=distanceDiveTarget(rotated,loose,10,30,7,25,contact);check(rotatedFloor.valid&&std::abs(rotatedFloor.point.x+rotated.head.x-rotated.rootPosition.x-loose.position.x)<1e-6,"dive targets account for the current head-to-root offset instead of steering beside the ball");
}
