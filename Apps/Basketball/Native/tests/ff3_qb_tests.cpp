#include "ff3_qb_logic.hpp"
#include "ff3_throw_state.hpp"
#include <cassert>
#include <iostream>
using namespace ff3qb;
int main(){int count=0;
 {RouteHistory still;for(int i=0;i<30;++i)still.update(i*.05,{0,8,0},{50+(i%2?.01:-.01),8,0},{i%2?1.:-1.,0,0});assert(length(still.movement)==0);}
 {BeamMemory memory;Plan low,high;low.solution.valid=high.solution.valid=true;low.power=95;low.solution.angle=20;high.power=60;high.solution.angle=65;high.lob=true;
  assert(memory.accept(low,{},1).power==95);
  assert(memory.accept(high,low,1.1).power==95);assert(memory.accept(low,low,1.2).power==95);
  assert(memory.accept(high,low,1.3).power==95);assert(memory.accept(high,low,1.4).power==95);assert(memory.accept(high,low,1.55).power==60);
 }
 {BeamMemory memory;Vec origin{0,8,0},target{100,8,0};auto prior=solve(origin,target,{},95,28,.05,false,5);
  memory.goal={solve(origin,target+Vec{0,1,0},{},95,28,.05,false,5),{0,1,0},95,10,false,false};Vec offset{};
  for(int i=0;i<7;++i){auto next=planCoverageThrow(origin,target,{},28,.05,{},true,true,7,196.2,offset,95,false,prior.angle,false,.016,nullptr,0,&memory,1+i*.016);
   assert(next.solution.valid&&next.offset.y>=offset.y&&next.offset.y-offset.y<=.192001);offset=next.offset;prior=next.solution;
  }assert(std::abs(offset.y-1)<1e-6);
  memory.goal={solve(origin,target,{},95,28,.05,true,5),{},95,10,false,true};double power=95;bool lob=false;prior=solve(origin,target,{},95,28,.05,false,5);
  for(int i=0;i<180;++i){auto next=planCoverageThrow(origin,target,{},28,.05,{},true,true,7,196.2,{},power,lob,prior.angle,false,.016,nullptr,0,&memory,2+i*.016);
   assert(next.solution.valid&&std::abs(next.solution.angle-prior.angle)<=.72001&&length(point(next.solution,28,next.solution.time)-target)<1e-6);
   prior=next.solution;power=next.power;lob=next.lob;
  }assert(power>94.9&&lob);
 }
 {Defender still{{100,8,0},{},20,6.37,12,5.88,3.88,4.88};
  assert(receiverGap(still,{104,8,0},1)<0);
  assert(receiverGap(still,{110,8,0},1)>0&&reachGap(still,{110,8,0},1)<0);
  auto catchArc=solve({0,8,0},{100,16,0},{},95,28,.05,false,5);auto full=groundFlight(catchArc,28,0);
  assert(full.valid&&full.time>catchArc.time&&full.catchPoint.x>catchArc.catchPoint.x);
  assert(std::abs(full.catchPoint.y-.6)<1e-7&&length(full.velocity-catchArc.velocity)<1e-9);
 }
 {Defender wr{{30,8,0},{},20,6.37,12,5.88,3.88,4.88};std::vector<Defender> overlapping{wr};
  auto p=planCoverageThrow({0,8,0},wr.position,{},28,.06,overlapping,true,true,6.37,196.2,{},35,true,80,true,.1,&wr);
  assert(p.solution.valid&&p.power==95&&p.solution.angle<20&&!estimatedClear(p,true));
  auto open=planCoverageThrow({0,8,0},{100,8,0},{},28,.06,{},true,true,6.37,196.2,{},95,true,80,true,.1);
  assert(open.solution.valid&&open.power==95&&!open.lob&&open.solution.angle<20);
 }
 auto divingReceiver=solve({0,8,0},{60,8,0},{0,0,43.5},95,28,.05,false,5);
 assert(divingReceiver.valid&&length(point(divingReceiver,28,divingReceiver.time)-divingReceiver.catchPoint)<1e-6);
 assert(!solve({0,8,0},{60,8,0},{0,0,46},95,28,.05,false,5).valid);
 // Pruning distant catchers must preserve the full sampled-path minimum.
 for(double distance:{15.,60.,150.})for(double speed:{45.,70.,95.})for(bool high:{false,true}){
  auto s=solve({0,8,0},{distance,10,0},{3,0,1},speed,28,.06,high,5);if(!s.valid)continue;
  std::vector<Defender> ds={{{-200,8,80},{},20,6.37,12},{{distance*.5,8,4},{5,0,0},20,6.37,12},{{distance+40,12,0},{},20,6.37,12}};
  double exhaustive=1000;int steps=std::clamp(int(std::ceil(s.time/.02)),32,400);
  for(const auto& d:ds)for(int i=0;i<=steps;++i)exhaustive=std::min(exhaustive,reachGap(d,point(s,28,s.time*i/steps),s.time*i/steps+.06));
  assert(std::abs(exhaustive-coverageMargin(s,28,ds,.06))<1e-9);
 }
 for(bool beam:{false,true})for(double height:{-3.,0.,4.,8.}){
  Defender wr{{100,8,0},{},20,6.37,12,5.88,3.88,4.88};
  auto p=planCoverageThrow({0,8,0},{100,8+height,0},{},28,.05,{},true,beam,6.37,196.2,{},0,false,NAN,true,.1,&wr,height);
  assert(p.solution.valid&&std::abs(p.solution.catchPoint.y-(8+height))<1e-6);
  assert(reachGap(wr,p.solution.catchPoint,p.solution.time+.05)<=-.25);
  if(beam)assert(p.power==95);
 }
 ResumeIntent resume;resume.suspend(true,10);resume.suspend(false,10);assert(resume.take(10));assert(!resume.take(10));resume.suspend(true,10);resume.cancel();assert(!resume.take(10));resume.suspend(true,10);assert(!resume.take(11));
 for(double angle:{12.,25.,35.,55.,75.})for(double distance:{15.,50.,90.,150.})for(double drift:{-10.,0.,10.}){
  Vec origin{0,8,0},target{distance,7,0},movement{drift,0,4};auto s=solveAngle(origin,target,movement,angle,28,.06);
  if(s.valid){assert(length(point(s,28,s.time)-s.catchPoint)<1e-6);assert(length(s.velocity)<=95);assert(std::abs(std::atan2(s.velocity.y,std::hypot(s.velocity.x,s.velocity.z))*180/3.141592653589793-angle)<1e-6);++count;}
 }
 assert(!solveAngle({},{500,0,0},{},5,28,.05).valid);
 assert(!solveAngle({},{40,0,0},{},NAN,28,.05).valid);
 assert(presetAngle(0,{},{160,0,0},{10,0,0})>presetAngle(0,{},{30,0,0},{10,0,0}));
 assert(std::string(routeName({},{80,0,0},{0,0,10}))=="Crossing");
 assert(selectionDistance(100,100,100,120,100,80,8,false)==0);
 assert(selectionDistance(120,100,100,120,100,80,8,true)<selectionDistance(120,100,100,120,100,80,8,false));
 std::vector<Defender> defenders={{{62,7,3},{0,0,0}}};
 auto baseline=planThrow({0,8,0},{60,7,0},{},28,.05,0,true,35,true,95,false,false,defenders);
 auto fit=planThrow({0,8,0},{60,7,0},{},28,.05,0,true,35,true,95,false,true,defenders);
 assert(baseline.solution.valid&&fit.solution.valid&&fit.margin<=catchSafetyMargin); // Coverage does not veto a reachable throw.
 assert(fit.margin>=baseline.margin-1e-6); // Do not worsen a deeply overlapping reach volume.
 auto past=planCoverageThrow({0,8,0},{60,8,0},{10,0,0},28,.05,{{{55,8,0},{},22,9,12}},true,false,7,196.2);
 assert(past.solution.valid&&past.margin>=coverageMargin(solve({0,8,0},{60,8,0},{10,0,0},95,28,.05,false,5),28,{{{55,8,0},{},22,9,12}},.05)-.025); // Fit can clear a trailing DB above or beyond the WR.
 assert(receiverCanReach(past.offset,{10,0,0},past.solution.time,5.95));
 auto still=planCoverageThrow({0,8,0},{60,8,0},{},28,.05,{{{57,8,0},{},22,9,12}},true,false,7,196.2);
 assert(still.solution.valid&&std::hypot(still.offset.x,still.offset.z)<=4.001);
 auto unknown=planThrow({0,8,0},{60,7,0},{},28,.05,0,true,35,true,95,false,true,{});assert(!unknown.coverageKnown&&length(unknown.offset)==0);
 auto manual=planThrow({0,8,0},{60,7,0},{},28,.05,0,false,35,true,95,false,false,{});assert(manual.solution.valid&&manual.solution.angle==35);
 assert(!planThrow({0,8,0},{340,7,0},{},28,.05,0,false,5,true,95,false,false,{}).solution.valid);
 AngleTracker smoother;assert(smoother.update(20,0)==20);assert(std::abs(smoother.update(55,.1)-22.4)<1e-6);assert(smoother.update(22.5,.2)==22.4);
 RouteHistory history;for(int i=0;i<30;i++){double t=i*.05;history.update(t,{0,7,0},{60+10*t,7,0},{10,0,0});}
 assert(history.route=="Vertical stem");
 for(int i=30;i<60;i++){double t=i*.05;history.update(t,{0,7,0},{74.5,7,12*(t-1.45)},{0,0,12});}
 assert(history.route=="Crossing / out"&&history.mode==4);
 auto high=solveAngle({0,8,0},{60,7,0},{},55,28,.05);assert(high.valid);
 std::vector<Defender> standing={{{45,7,14},{},22,0,0}},athletic={{{45,7,14},{},22,9,12}};
 assert(coverageMargin(high,28,athletic,.05)<=coverageMargin(high,28,standing,.05));
 auto stable=planThrow({0,8,0},{60,7,0},{},28,.05,0,true,35,true,95,false,true,std::vector<Defender>{{{45,7,100},{},22,9,12}},32);
 assert(stable.solution.valid&&stable.solution.angle==32); // Fit does not jump +/-10 degrees.
 // A stopped receiver loses historical lead and fit never moves the head target.
 history.update(3.1,{0,7,0},{74.5,7,18},{0,0,0});history.update(3.2,{0,7,0},{74.5,7,18},{0,0,0});assert(length(history.movement)==0);
 for(double distance:{20.,60.,150.,250.})for(Vec movement:{Vec{},Vec{8,0,2}}){
  const Vec head{distance,8,0};
  auto fast=planThrow({0,8,0},head,movement,28,.05,0,true,35,true,60,false,false,athletic,48,{},true,5);
  assert(fast.solution.valid&&std::abs(fast.power-95)<1e-6);
  assert(length(point(fast.solution,28,fast.solution.time)-fast.solution.catchPoint)<1e-5);
  auto timed=solveTimed({0,8,0},head,movement,fast.solution.time,28,.05);
  assert(timed.valid&&length(point(timed,28,timed.time)-timed.catchPoint)<1e-5&&std::abs(length(timed.velocity)-95)<1e-5);
  assert(std::abs(length(fast.solution.origin-Vec{0,8,0})-5)<1e-6);
  if(length(movement)==0){assert(length(fast.offset)==0);assert(length(fast.solution.catchPoint-head)==0);}
  auto angled=solveAngle({0,8,0},head,movement,35,28,.05,5);
  if(angled.valid)assert(length(point(angled,28,angled.time)-angled.catchPoint)<1e-6);
 }
 auto directAuto=planThrow({0,8,0},{150,8,0},{},28,.05,0,true,35,true,60,false,false,athletic,48,{},true,5);
 assert(directAuto.solution.angle<20); // No arbitrary distance-based lob.
 auto jump95=planCoverageThrow({0,8,0},{100,8,0},{},28,.05,{},true,true,7,196.2);
 assert(jump95.solution.valid&&jump95.power==95&&jump95.offset.y==0);
 assert(jump95.offset.x==0&&jump95.offset.z==0);
 assert(length(point(jump95.solution,28,jump95.solution.time)-jump95.solution.catchPoint)<1e-5);
 auto headBeam=planCoverageThrow({0,8,0},{100,8,0},{},28,.05,{},true,true,0,196.2);
 assert(headBeam.solution.valid&&headBeam.power==95&&headBeam.offset.y==0);
 auto shortBeam=planCoverageThrow({0,8,0},{15,8,0},{},28,.05,{},true,true,7,196.2);
 assert(shortBeam.solution.valid&&shortBeam.solution.angle<5&&!shortBeam.lob&&shortBeam.power==95);
 auto closeBeam=planCoverageThrow({0,8,0},{35,8,0},{},28,.05,{},true,true,7,196.2);
 assert(closeBeam.solution.valid&&closeBeam.power>=90&&closeBeam.solution.angle<=beamMaxAngle);
 auto closeCovered=planCoverageThrow({0,8,0},{35,8,0},{},28,.05,{{{35,8,3},{},22,9,12}},true,true,7,196.2,shortBeam.offset,shortBeam.power,shortBeam.lob,shortBeam.solution.angle);
 for(int i=0;i<12;i++)closeCovered=planCoverageThrow({0,8,0},{35,8,0},{},28,.05,{{{35,8,3},{},22,9,12}},true,true,7,196.2,closeCovered.offset,closeCovered.power,closeCovered.lob,closeCovered.solution.angle);
 assert(closeCovered.solution.valid&&closeCovered.margin<=catchSafetyMargin&&closeCovered.power<=95);
 auto fiftyYards=planCoverageThrow({0,8,0},{150,8,0},{22,0,0},28,.05,{{{80,8,5},{},22,9,12}},true,true,7,196.2);
 assert(fiftyYards.solution.valid&&fiftyYards.solution.angle<=beamMaxAngle);
 // Recompute moving geometry without power roundoff or a retained high root.
 Plan beam;
 for(int distance=20;distance<=400;distance+=2){
  beam=planCoverageThrow({0,8,0},{double(distance),8,0},{8,0,2},28,.05,{{{70,8,8},{},22,9,12}},true,true,7,196.2,beam.offset,beam.power,true,70);
  if(!beam.solution.valid)continue;
  assert(beam.power>=15&&beam.power<=95&&beam.solution.angle<=beamMaxAngle);
  auto low=solve({0,8,0},Vec{double(distance),8,0}+beam.offset,{8,0,2},beam.power,28,.05,beam.lob,5);
  if(!low.valid||std::abs(low.time-beam.solution.time)>=1e-6)std::cerr<<"Arc mismatch at "<<distance<<" power "<<beam.power<<" lob "<<beam.lob<<" time "<<beam.solution.time<<" rebuilt "<<low.time<<" angle "<<beam.solution.angle<<'\n';
  assert(low.valid&&std::abs(low.time-beam.solution.time)<1e-6);
  assert(length(point(beam.solution,28,beam.solution.time)-beam.solution.catchPoint)<1e-5);
 }
 assert(!planCoverageThrow({0,8,0},{500,8,0},{},28,.05,{},true,true,7,196.2).solution.valid);
 auto noJump=planCoverageThrow({0,8,0},{100,8,0},{},28,.05,{},true,false,0,196.2);
 assert(noJump.solution.valid&&noJump.power==95&&noJump.offset.y==0);
 std::vector<Defender> underneath={{{45,8,20},{},22,10,12}};
 auto adaptive=planCoverageThrow({0,8,0},{150,8,0},{},28,.05,underneath,true,false,7,196.2);
 auto forced95=planCoverageThrow({0,8,0},{150,8,0},{},28,.05,underneath,true,true,7,196.2);
 assert(adaptive.solution.valid&&forced95.solution.valid);
 assert(forced95.margin>catchSafetyMargin&&forced95.solution.angle<=beamMaxAngle);
 assert(adaptive.power<=95&&adaptive.margin>catchSafetyMargin);
 auto retained=planCoverageThrow({0,8,0},{150.1,8,0},{},28,.05,underneath,true,false,7,196.2,adaptive.offset,adaptive.power,adaptive.lob,adaptive.solution.angle);
 assert(retained.solution.valid&&std::abs(retained.solution.angle-adaptive.solution.angle)<.2);
 std::vector<Defender> onReceiver={{{100,8,0},{},22,9,12}};
 auto contested=planCoverageThrow({0,8,0},{100,8,0},{},28,.05,onReceiver,true,false,7,196.2);
 assert(contested.solution.valid&&contested.margin<=catchSafetyMargin); // Assist remains active without claiming exclusivity.
 assert(contested.power<=95&&contested.offset.y<=7);
 auto contestedHeld=planCoverageThrow({0,8,0},{100.01,8,0},{},28,.05,onReceiver,true,false,7,196.2,contested.offset,contested.power,contested.lob,contested.solution.angle);
 assert(contestedHeld.solution.valid&&std::abs(contestedHeld.solution.angle-contested.solution.angle)<.1);
 // A newly blocked held path must yield to a materially better placement.
 auto redirected=planCoverageThrow({0,8,0},{150,8,0},{},28,.05,underneath,true,false,7,196.2,forced95.offset,forced95.power,forced95.lob,forced95.solution.angle);
 assert(redirected.solution.valid&&redirected.margin>catchSafetyMargin);
 // A moving receiver still gets an assisted, reachable fit under close coverage.
 for(Vec defender:{Vec{100,8,8},Vec{85,8,15},Vec{60,8,5}}){
  std::vector<Defender> coverage{{defender,{},22,9,12}};
  auto plain=planCoverageThrow({0,8,0},{100,8,0},{8,0,0},28,.05,coverage,false,false,7,196.2);
  auto smart=planCoverageThrow({0,8,0},{100,8,0},{8,0,0},28,.05,coverage,true,false,7,196.2);
  assert(plain.solution.valid&&smart.solution.valid);
  assert(smart.margin>=plain.margin-.025);
  assert(length(point(smart.solution,28,smart.solution.time)-smart.solution.catchPoint)<1e-5);
  assert(receiverCanReach(smart.offset,{8,0,0},smart.solution.time,5.95)&&smart.offset.y<=7);
 }
 auto closeLob=planThrow({0,8,0},{25,8,0},{8,0,0},28,.05,2,true,55,true,95,true,true,{},55,{},false,5);
 assert(closeLob.solution.valid&&closeLob.power<50&&closeLob.solution.angle==55);
 auto continuous=adaptive;
 for(int i=1;i<=80;i++){
  double oldAngle=continuous.solution.angle;auto oldOffset=continuous.offset;
  continuous=planCoverageThrow({0,8,0},{150+i*.01,8,.03*std::sin(i)},{},28,.05,underneath,true,false,7,196.2,continuous.offset,continuous.power,continuous.lob,continuous.solution.angle);
  assert(continuous.solution.valid&&std::abs(continuous.solution.angle-oldAngle)<=4.01&&length(continuous.offset-oldOffset)<=1.21);
 }
 auto nearLaunch=jump95.solution.origin;
 assert(coverageMargin(jump95.solution,28,{{nearLaunch,{},22,0,0}},.05)<=catchSafetyMargin);
 // Open regular throws prioritize power; close covered throws can loft at lower power.
 auto open=planCoverageThrow({0,8,0},{100,8,0},{},28,.05,{},true,false,7,196.2);
 assert(open.solution.valid&&open.power>=90&&open.solution.angle<20);
 auto touch=planCoverageThrow({0,8,0},{20,8,0},{},28,.05,{{{19,8,0},{},22,9,12}},true,true,7,196.2);
 assert(touch.solution.valid&&touch.margin<=catchSafetyMargin&&!estimatedClear(touch,true));
 auto closeRegular=planCoverageThrow({0,8,0},{20,8,0},{},28,.05,{{{19,8,0},{},22,9,12}},true,false,7,196.2);
 assert(closeRegular.solution.valid&&closeRegular.margin<=catchSafetyMargin&&!estimatedClear(closeRegular,true));
 auto resetArc=planCoverageThrow({0,8,0},{100,8,0},{},28,.05,{},true,false,7,196.2,{},50,true,60);
 for(int i=0;i<30;i++){double before=resetArc.solution.angle;resetArc=planCoverageThrow({0,8,0},{100,8,0},{},28,.05,{},true,false,7,196.2,resetArc.offset,resetArc.power,resetArc.lob,resetArc.solution.angle);assert(resetArc.solution.valid&&std::abs(resetArc.solution.angle-before)<=4.01);}
 if(!(resetArc.power>=90&&resetArc.solution.angle<20))std::cerr<<"Reset arc: "<<resetArc.power<<" / "<<resetArc.solution.angle<<'\n';
 assert(resetArc.power>=90&&resetArc.solution.angle<20);
 // A coverage ranking change must not bypass the actual throw-angle limiter.
 auto switching=solveAngle({0,8,0},{150,8,0},{},35,28,.05,5);
 Plan smoothSwitch{switching,{},length(switching.velocity),0,true,false};
 for(int i=0;i<24;i++){
  const double before=smoothSwitch.solution.angle;
  smoothSwitch=planCoverageThrow({0,8,0},{150,8,0},{},28,.05,{{{45,8,i%2?12.:30.},{},22,9,12}},true,false,7,196.2,smoothSwitch.offset,smoothSwitch.power,smoothSwitch.lob,before,true,.05);
  assert(smoothSwitch.solution.valid&&std::abs(smoothSwitch.solution.angle-before)<=1.201);
  assert(length(point(smoothSwitch.solution,28,smoothSwitch.solution.time)-smoothSwitch.solution.catchPoint)<1e-6);
 }
 // Beam selects power before visual continuity, even from a previous weak lob.
 auto powerBeam=planCoverageThrow({0,8,0},{100,8,0},{},28,.05,{},true,true,7,196.2,{},50,true,60,true,.016);
 assert(powerBeam.solution.valid&&powerBeam.power==95&&!powerBeam.lob);
 auto elevatedBeam=planCoverageThrow({0,8,0},{150,8,0},{},28,.05,underneath,true,true,7,196.2,{},40,false,35,true,.016);
 assert(elevatedBeam.power==forced95.power&&estimatedClear(elevatedBeam,true));
 {
  ThrowActor wr;wr.id=4;wr.name="WR";wr.valid=wr.sideKnown=true;wr.reach={{100,8,0},{},20,15,12};
  ThrowActor db=wr;db.id=6;db.name="DB";db.opponent=true;db.reach.jumpHeight=0;db.reach.speed=0;db.reach.diveReach=0;
  BallFlight ownBall;ownBall.id=88;ownBall.thrower=3;ownBall.airborne=true;ownBall.born=10.1;ownBall.at=10.2;ownBall.position={0,60,0};ownBall.gravity=28;ownBall.model={true,10.2,28,0,ownBall.position,{50,10,0},10.2};ownBall.fieldY=0;
  ThrowView metrics;flightMetrics(metrics,ownBall,0,{wr,db},4,true);
  assert(metrics.coverageKnown&&!metrics.intable&&std::isfinite(metrics.toSpot)&&std::isfinite(metrics.nearD)); // Overlapping WR/DB positions alone do not mean interceptable.
  auto lowBall=ownBall;lowBall.position={0,8,0};lowBall.model.origin=lowBall.position;lowBall.model.velocity={50,8,0};db.reach.position={50,8,0};
  flightMetrics(metrics,lowBall,0,{wr,db},4,true);assert(metrics.intable&&metrics.margin<=catchSafetyMargin);
  db.opponent=false;flightMetrics(metrics,lowBall,0,{wr,db},4,true);assert(!metrics.intable&&std::isnan(metrics.nearD));
  db.sideKnown=false;flightMetrics(metrics,lowBall,0,{wr,db},4,true);assert(metrics.coverageKnown&&metrics.intable); // Teamless practice players remain potential catchers.
  db.sideKnown=db.opponent=true;
  {auto late=ownBall;late.position={0,40,0};late.model.origin=late.position;late.model.velocity={30,0,0};auto farWr=wr;farWr.reach={{200,8,0},{},0,0,0};auto lateDb=db;lateDb.reach={{45,5,0},{},0,0,0};
   flightMetrics(metrics,late,0,{farWr,lateDb},4,true,35);
   assert(metrics.intable&&metrics.eta>1.5); // A missed intended height cannot hide a DB later in the actual path.
  }
  ThrowIntent intent{10,1,2,3,4,5,10,95,35,2,100,12,"WR","Beam"};
  ThrowTracker shot;shot.begin(intent,{}, {wr,db});
  auto foreign=ownBall;foreign.id=99;foreign.thrower=900;
  shot.update(10.2,1,2,{foreign},{wr,db},0,true);assert(shot.view.phase==ThrowView::Pending&&!shot.ball);
  shot.update(10.25,1,2,{foreign,ownBall},{wr,db},0,true);assert(shot.view.phase==ThrowView::Flight&&shot.ball==88&&shot.view.name=="WR");
  auto caught=wr;caught.ball=88;
  shot.update(10.3,1,2,{}, {caught,db},0,true);assert(shot.view.phase==ThrowView::Done&&shot.view.result=="Caught");
  shot.update(13,1,2,{}, {wr,db},0,true);assert(shot.view.phase==ThrowView::Hidden);
  {auto predicted=intent;predicted.predictionKnown=predicted.predictedIntable=true;ThrowTracker retained;retained.begin(predicted,{}, {wr,db});
   retained.update(10.2,1,2,{ownBall},{wr},0,true);assert(retained.view.coverageKnown&&retained.view.intable); // Passing the DB cannot erase the pre-throw forecast.
  }
  shot.begin(intent,{}, {wr,db});shot.update(10.2,1,2,{ownBall},{wr,db},0,true);db.ball=88;
  shot.update(10.3,1,2,{}, {wr,db},0,true);assert(shot.view.result=="Intercepted");db.ball=0;
  shot.begin(intent,{}, {wr,db});shot.update(10.2,1,2,{ownBall},{wr,db},0,true);
  auto groundBall=ownBall;groundBall.landed=groundBall.ended=true;groundBall.position={100,.6,0};
  shot.update(11,1,2,{groundBall},{wr,db},0,true);shot.update(11.2,1,2,{groundBall},{wr,db},0,true);assert(shot.view.result=="Incomplete");
  shot.begin(intent,{}, {wr,db});shot.update(10.2,1,2,{ownBall},{wr,db},0,true);shot.update(11,1,2,{groundBall},{wr,db},0,true);
  shot.update(11.3,1,2,{}, {caught,db},0,true);assert(shot.view.result=="Incomplete"); // Picking it up after ground contact is not a completed pass.
  shot.begin(intent,{}, {wr,db});shot.update(10.2,1,2,{ownBall},{wr,db},0,true);shot.update(10.3,1,2,{}, {wr,db},0,true);shot.update(11,1,2,{}, {wr,db},0,true);assert(shot.view.result=="Result unavailable");
  shot.begin(intent,{ownBall},{wr,db});shot.update(10.2,1,2,{ownBall},{wr,db},0,true);assert(!shot.ball); // Existing nearby own balls are not a new pass.
  shot.begin(intent,{}, {wr,db});auto another=ownBall;another.id=89;shot.update(10.2,1,2,{ownBall,another},{wr,db},0,true);assert(!shot.ball); // Ambiguous ownership does not pick an arbitrary ball.
  shot.update(10.3,7,2,{}, {wr,db},0,true);assert(shot.view.phase==ThrowView::Hidden);
 }
 // Script-driven balls can retain their launch velocity and be anchored.
 // Forecast must use changing positions, and a stationary stale-velocity ball stays hidden.
 FlightTracker flights;
 for(int i=0;i<=25;i++){double t=i*.05;flights.update({{1,{40*t,10+20*t-14*t*t,0},{40,20,0},28,true},{2,{0,3,0},{95,60,0},28,false}},1+t);}
 auto observed=flights.visible();assert(observed.size()==1&&observed[0].id==1);
 auto fullForecast=forecast(observed[0],0);assert(fullForecast.valid&&fullForecast.path.size()==81);
 assert(std::abs(fullForecast.landing.y-.6)<1e-7&&fullForecast.remaining>.5);
 const double absoluteLanding=1.25+fullForecast.remaining;
 assert(std::abs(fullForecast.landing.x-40*absoluteLanding)<.02);
 assert(!forecast(observed[0],NAN).valid);
 FlightTracker contact=flights;contact.update({{1,{72,.6,0},{40,-30,0},28,true}},2.3,0);assert(contact.visible().empty());
 auto noGravity=observed[0];noGravity.gravity=0;assert(!forecast(noGravity,0).valid);
 assert(length(leadAdjustment({0,8,0},{100,8,0},{0,0,20},-5)-Vec{0,0,-5})<1e-9);
 assert(length(leadAdjustment({0,8,0},{100,8,0},{},-5)-Vec{-5,0,0})<1e-9);
 assert(length(leadAdjustment({0,8,0},{100,8,0},{},0))==0);
 assert(std::abs(observed[0].velocity.x-40)<.01&&std::abs(observed[0].velocity.y-(20-28*1.25))<.01);
 assert(observed[0].trail.size()>10); // Survives the apex with constant raw velocity.
 auto landed=observed[0].position;
 for(int i=1;i<=8;i++)flights.update({{1,landed,{40,20,0},28,true}},2.25+i*.05);
 assert(flights.visible().empty());
 flights.update({},3);assert(flights.flights.empty()); // Catch/removal clears history.
 for(int i=0;i<8;i++)flights.update({{3,{i*1.25,1,0},{25,-20,0},28,false}},4+i*.05);
 assert(flights.visible().empty()); // Level rolling motion is no longer airborne.
 // Replication jitter must not rotate the full predicted landing each frame.
 FlightTracker jitter;Vec fixedLanding{};bool lockedForecast=false;int forecastCount=0;
 for(int i=0;i<=100;i++){
  double t=i*.02,stamp=std::floor(t/.05)*.05;Vec p{70*stamp,20+45*stamp-14*stamp*stamp,12*stamp};
  jitter.update({{77,p,{70,45,12},28,true}},10+t,0);
  for(const auto& ball:jitter.visible()){
   auto prediction=forecast(ball,std::sin(t*7)*.6);
   if(prediction.valid){if(!lockedForecast){fixedLanding=prediction.landing;lockedForecast=true;}else assert(length(prediction.landing-fixedLanding)<1e-5);++forecastCount;}
  }
 }
 assert(forecastCount>50);
 // Delivery times can accelerate/decelerate independently of actual motion.
 // Geometry must recover the true landing despite those changing delays.
 std::vector<FlightSample> delayed;
 for(int i=0;i<=12;i++){double t=i*.04,arrival=20+t+.035*std::sin(i*.55);delayed.push_back({arrival,{70*t,20+45*t-14*t*t,12*t}});}
 auto spatial=fitFlight(delayed,28);assert(spatial.valid);
 BallFlight delayedBall;delayedBall.model=spatial;delayedBall.gravity=28;delayedBall.at=delayed.back().at;delayedBall.fieldY=0;
 auto actual=forecast(delayedBall,0);double landingTime=(45+std::sqrt(45*45+56*19.4))/28;
 assert(actual.valid&&length(actual.landing-Vec{70*landingTime,.6,12*landingTime})<.01);
 auto originalPath=actual.path;delayedBall.at+=.2;auto advanced=forecast(delayedBall,0);
 for(size_t i=0;i<originalPath.size();i++)assert(length(originalPath[i]-advanced.path[i])<1e-7);
 FlightTracker shortFlight;shortFlight.update({{78,{0,10,0},{},28,true}},0,0);shortFlight.update({{78,{5,12,0},{},28,true}},.05,0);
 assert(!forecast(shortFlight.visible().front(),0).valid); // No two-sample long-range projection.
 auto prior=jitter.visible().front();jitter.update({{77,prior.position+Vec{50,0,0},{},28,true}},12.05,0);assert(jitter.visible().empty()||!forecast(jitter.visible().front(),0).valid);
 // Reproduce the observed 65->70->75 staircase with a receiver running away.
 Plan chase;
 for(int i=0;i<30;i++){double distance=100+i*2.;chase=planCoverageThrow({0,8,0},{distance,8,0},{20,0,0},28,.05,{{{distance-2,8,0},{20,0,0},23,8.37,12}},true,true,6.37,196.2,chase.offset,chase.power,chase.lob,chase.solution.angle);
  assert(chase.solution.valid&&chase.power<=95&&chase.solution.angle<=beamMaxAngle&&chase.margin<=catchSafetyMargin);
 }

 // Previously Beam rejected every arc above 35 degrees and sent a contested
 // 95-power pass. This scene has a clear 65-power high arc over the DB.
 std::vector<Defender> middle{{{40,8,0},{},22,9,12}};
 auto blockedLow=solve({0,8,0},{150,8,0},{},95,28,.05,false,5);
 assert(coverageMargin(blockedLow,28,middle,.05)<=catchSafetyMargin);
 auto over=planCoverageThrow({0,8,0},{150,8,0},{},28,.05,middle,true,true,7,196.2);
 assert(over.solution.valid&&over.power<95&&over.solution.angle>35&&over.margin>catchSafetyMargin&&over.lob);
 assert(length(point(over.solution,28,over.solution.time)-over.solution.catchPoint)<1e-5);
 // Increasing flight time cannot make identical receiver/defender reach
 // exclusive. This must never become an estimated-clear result.
 for(bool beamMode:{false,true}){
  auto overlap=planCoverageThrow({0,8,0},{25,8,0},{},28,.05,{{{25,8,0},{},22,9,12}},true,beamMode,7,196.2);
  assert(overlap.solution.valid&&!estimatedClear(overlap,true));
 }
 SelectionLatch select;assert(select.update(1,false,0)==1);
 // Dimensions from the bounded R6 catch-part check, shared by WR and DB.
 auto geometry=catchGeometry({2,2,1},{2,1,1},{1,2,1},{1.4,1.65,1.4},.6);
 assert(geometry.measured&&geometry.radius>5.8&&geometry.radius<5.9&&geometry.above>3.8&&geometry.above<3.9);
 assert(!catchGeometry({NAN,2,1},{2,1,1},{1,2,1},{1.4,1.65,1.4},.6).measured);
 double actualJump=2500./(2*196.2);
 assert(jumpRise(actualJump,196.2,.01)<.51&&std::abs(jumpRise(actualJump,196.2,.3)-actualJump)<1e-8);
 Defender reach{{0,0,0},{},20,0,12,geometry.radius,geometry.above,geometry.below,196.2};
 assert(std::abs(reachGap(reach,{geometry.radius+20*.2+25*.1,-.5,0},.2))<1e-6); // Dive replaces sprint, not both at full speed.
 assert(reachGap(reach,{geometry.radius,geometry.above,0},0)>0); // Cannot extend an arm fully in two directions.
 Vec closeOrigin{11.364845275879,7.489995002747,-15.778594970703},closeTarget{0,7.488372802734,0};
 Defender wr{closeTarget,{},20,actualJump,12,geometry.radius,geometry.above,geometry.below,196.2};
 Defender db=wr;db.position.z=-5.843841552735;Defender other=db;other.position={1.52346,7.48837,34.29739};
 std::vector<Defender> closeScene{db,other};
 auto window=planCoverageThrow(closeOrigin,closeTarget,{},28,.0624565,closeScene,true,true,actualJump,196.2,{},0,false,NAN,true,.1,&wr);
 assert(window.solution.valid&&placementAllowed(window.offset,{},window.solution.time));
 assert(std::hypot(window.offset.x,window.offset.z)<=3.000001&&reachGap(wr,window.solution.catchPoint,window.solution.time+.0624565)<-.25);
 CoverageTrace threat;coverageMargin(window.solution,28,closeScene,.0624565,&threat);assert(threat.catcher>=0&&threat.time>=0&&threat.time<=window.solution.time);
 auto rebuilt=solve(closeOrigin,closeTarget+window.offset,{},window.power,28,.0624565,window.lob,5);
 assert(rebuilt.valid&&length(rebuilt.catchPoint-window.solution.catchPoint)<1e-5&&std::abs(rebuilt.time-window.solution.time)<1e-6);
 auto heldWindow=planCoverageThrow(closeOrigin,closeTarget,{},28,.0624565,closeScene,true,true,actualJump,196.2,window.offset,window.power,window.lob,window.solution.angle,true,.1,&wr);
 assert(length(heldWindow.offset-window.offset)<1e-6&&heldWindow.power==window.power);
 auto overlapping=planCoverageThrow(closeOrigin,closeTarget,{},28,.0624565,{wr},true,true,actualJump,196.2,{},0,false,NAN,true,.1,&wr);
 assert(!estimatedClear(overlapping,true));
 // The former "clear" fit required an uncommanded 17-stud sprint/dive.
 // It must not survive retention, even though it is theoretically reachable.
 auto corrected=planCoverageThrow(closeOrigin,closeTarget,{},28,.0624565,closeScene,true,true,actualJump,196.2,{11.9801,6.33939,11.9801},95,false,14.553,true,.1,&wr);
 assert(corrected.solution.valid&&placementAllowed(corrected.offset,{},corrected.solution.time));
 assert(!placementAllowed({11.9801,6.33939,11.9801},{},.25));
 for(Vec movement:{Vec{},Vec{0,0,20}})for(double distance:{20.,60.,150.}){
  Defender receiver{{distance,8,0},movement,20,actualJump,12,geometry.radius,geometry.above,geometry.below,196.2};
  auto defender=receiver;defender.position.z-=6;
  auto bounded=planCoverageThrow({0,8,0},receiver.position,movement,28,.06,{defender},true,true,actualJump,196.2,{30,0,40},95,false,15,true,.1,&receiver);
  if(bounded.solution.valid)assert(placementAllowed(bounded.offset,movement,bounded.solution.time)&&std::hypot(bounded.offset.x,bounded.offset.z)<=6.000001);
 }
 // Go-route sketch: two DBs to the right and one underneath-left. Adjust the
 // landing slightly left/short while clearing every DB, not just the nearest.
 Defender routeWr{{0,8,100},{0,0,20},20,actualJump,12,geometry.radius,geometry.above,geometry.below,196.2};
 auto rightDb=routeWr,trailDb=routeWr,underDb=routeWr;
 rightDb.position={40,8,100};trailDb.position={50,8,80};underDb.position={-35,8,45};
 std::vector<Defender> routeCoverage{rightDb,trailDb,underDb};
 auto originalLanding=solve({0,8,0},routeWr.position,routeWr.velocity,95,28,.06,false,5);
 assert(coverageMargin(originalLanding,28,routeCoverage,.06)<=catchSafetyMargin);
 auto sideFit=planCoverageThrow({0,8,0},routeWr.position,routeWr.velocity,28,.06,routeCoverage,true,true,actualJump,196.2,{},0,false,NAN,true,.1,&routeWr);
 assert(estimatedClear(sideFit,true)&&sideFit.power==95&&sideFit.offset.x<0&&sideFit.offset.z<0);
 assert(std::hypot(sideFit.offset.x,sideFit.offset.z)<=3.01&&sideFit.solution.angle<20);
 assert(receiverGap(routeWr,sideFit.solution.catchPoint,sideFit.solution.time+.06)<-.25);
 assert(select.update(2,true,.05)==1);assert(select.update(1,true,.1)==1);
 assert(select.update(2,true,.15)==1);assert(select.update(2,true,.32)==2);
 assert(select.update(3,true,.33,true)==3);assert(select.update(4,false,.34)==4);
 CurveTracker display;auto lowDisplay=solve({0,8,0},{80,8,0},{},95,28,.05,false,5);
 auto highDisplay=solve({0,8,0},{80,8,0},{},60,28,.05,true,5);
 {CurveTracker anchored;Vec head{0,8,0};anchored.update(lowDisplay,28,1,1,head);auto start=anchored.points.front();
  anchored.update(highDisplay,28,1,1.016,head);
  assert(length(anchored.points.front()-start)<length(highDisplay.origin-start)*.2); // Angle changes cannot teleport the launch point.
 }
 display.update(lowDisplay,28,1,0);auto initial=display.points[32];
 display.update(highDisplay,28,1,.016);assert(display.settling&&length(display.points[32]-initial)<length(point(highDisplay,28,highDisplay.time*.5)-initial)*.3);
 for(int i=2;i<120;i++)display.update(highDisplay,28,1,i*.016);
 assert(!display.settling&&length(display.points[32]-point(highDisplay,28,highDisplay.time*.5))<.01);
 display.update(lowDisplay,28,2,1);assert(!display.settling&&length(display.points[32]-initial)<1e-6);
 // Continuous receiver/QB translation must not leave the display perpetually
 // settling. Launch translation stays attached while new placement eases.
 for(int i=1;i<=120;i++){
  auto moving=lowDisplay;Vec shift{0,0,20*i*.016};moving.origin=moving.origin+shift;moving.catchPoint=moving.catchPoint+shift;
  display.update(moving,28,2,1+i*.016);
  assert(!display.settling&&length(display.points[32]-point(moving,28,moving.time*.5))<1e-6);
 }
 display.update(highDisplay,28,2,2.936);
 assert(length(display.points.front()-highDisplay.origin)<1e-6);
 auto before=display.points[32];display.update({},28,2,2.95);assert(length(display.points[32]-before)<1e-9);
 auto shifted=highDisplay;shifted.velocity.z+=15;shifted.catchPoint=point(shifted,28,shifted.time);
 auto oldEnd=display.points.back();display.update(shifted,28,2,2.966);
 assert(length(display.points.back()-oldEnd)<length(point(shifted,28,shifted.time)-oldEnd)*.2);


 for(bool visible:{false,true})for(bool right:{false,true})for(bool first:{false,true})assert(cursorModeAllowed(visible,right,first)==(visible&&!right&&!first));
 const float identity[]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};float tilted[]={1,0,0,0,0,.8f,.6f,0,0,0,1,0,0,0,0,1};
 assert(cameraStable(identity,identity));assert(!cameraStable(identity,tilted));tilted[4]=NAN;assert(!cameraStable(identity,tilted));
 // Real repro: hidden old wheel 70 and current visible wheel 75.
 assert(readPower(true,{{1,0,"70"},{1,2,"75"}})==75);
 assert(readPower(true,{{1,2,"75"},{1,0,"70"}})==75);
 assert(readPower(true,{{1,0,"70"}})==0);
 assert(readPower(false,{{1,2,"75"}})==0);
 assert(readPower(true,{{0,2,"75"},{1,2," 95 "}})==95);
 assert(readPower(true,{{1,2,"70"},{1,2,"75"}})==-2);
 for(const auto* text:{"96","0","95x","","nan","-5","100"})assert(readPower(true,{{1,2,text}})==-1);
 assert(powerAvailable(true,true,0));assert(powerAvailable(true,true,-2));assert(!powerAvailable(true,false,0));assert(!powerAvailable(true,false,-2));assert(powerAvailable(true,false,75));assert(!powerAvailable(false,true,75));
 assert(readPower(true,{{1,2,"45"}})==45);assert(readPower(false,{{1,2,"45"}})==0);

 for(double speed:{50.,75.,95.})for(double gravity:{28.,40.})for(double distance:{20.,50.,80.})for(double drift:{-12.,0.,12.})for(bool lob:{false,true}){
  Vec origin{0,8,0},target{distance,7,10},v{0,0,drift};auto s=solve(origin,target,v,speed,gravity,.05,lob);
  if(s.valid){assert(length(point(s,gravity,s.time)-s.catchPoint)<1e-5);assert(length(s.catchPoint-(target+v*(s.time+.05)))<1e-5);assert(std::abs(length(s.velocity)-speed)<1e-5);++count;}
 }
 auto direct=solve({0,8,0},{60,7,0},{},95,28,.03),lob=solve({0,8,0},{60,7,0},{},95,28,.03,true);
 for(double speed:{20.,50.,95.})for(double height:{-20.,0.,12.})for(double drift:{-10.,0.,10.}){
  Solution paired;auto low=solve({0,8,0},{60,8+height,0},{drift,0,0},speed,28,.03,false,5,&paired);
  auto separateLow=solve({0,8,0},{60,8+height,0},{drift,0,0},speed,28,.03,false,5);
  auto separateHigh=solve({0,8,0},{60,8+height,0},{drift,0,0},speed,28,.03,true,5);
  assert(low.valid==separateLow.valid&&paired.valid==separateHigh.valid);
  if(low.valid)assert(std::abs(low.time-separateLow.time)<1e-9);
  if(paired.valid)assert(std::abs(paired.time-separateHigh.time)<1e-9);
 }
 assert(direct.valid&&lob.valid&&lob.angle>direct.angle&&lob.time>direct.time);
 assert(!solve({},{500,5,0},{},20,28,0).valid);
 assert(!solve({NAN,0,0},{20,5,0},{},95,28,0).valid);
 assert(!solve({},{20,5,0},{50,0,0},95,28,0).valid);
 assert(!solve({},{20,5,0},{},95,-28,0).valid);
 assert(!solve({},{20,5,0},{},95,28,NAN).valid);
 ClickGate gate;assert(!gate.down(false,1));assert(!gate.up());assert(gate.down(true,2));assert(gate.down(true,2.01));assert(gate.up());assert(gate.take(2.02,true));assert(!gate.take(2.03,true));assert(!gate.up());
 assert(gate.down(true,3));assert(!gate.take(3.2,true));assert(gate.up());
 assert(gate.down(true,4));gate.cancel();assert(!gate.take(4.01,true));assert(gate.up());
 assert(gate.down(true,5));assert(!gate.take(5.01,false));assert(gate.up());
 std::cout<<count<<" ballistic interception cases and invalid-data/click ownership checks passed\n";
}
