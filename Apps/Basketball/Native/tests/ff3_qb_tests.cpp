#include "ff3_qb_logic.hpp"
#include <cassert>
#include <iostream>
using namespace ff3qb;
int main(){int count=0;
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
 assert(baseline.solution.valid&&fit.solution.valid&&fit.coverageKnown);assert(fit.margin>=baseline.margin-1e-6);assert(length(fit.offset)<=10);
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
 auto stable=planThrow({0,8,0},{60,7,0},{},28,.05,0,true,35,true,95,false,true,athletic,32);
 assert(stable.solution.valid&&stable.solution.angle==32); // Fit does not jump +/-10 degrees.
 // A stopped receiver loses historical lead and fit never moves the head target.
 history.update(3.1,{0,7,0},{74.5,7,18},{0,0,0});history.update(3.2,{0,7,0},{74.5,7,18},{0,0,0});assert(length(history.movement)==0);
 for(double distance:{20.,60.,150.,250.})for(Vec movement:{Vec{},Vec{8,0,2}}){
  const Vec head{distance,8,0};
  auto fast=planThrow({0,8,0},head,movement,28,.05,0,true,35,true,60,false,true,athletic,48,{},true,5);
  assert(fast.solution.valid&&std::abs(fast.power-95)<1e-6);
  assert(length(point(fast.solution,28,fast.solution.time)-fast.solution.catchPoint)<1e-5);
  assert(std::abs(length(fast.solution.origin-Vec{0,8,0})-5)<1e-6);
  if(length(movement)==0){assert(length(fast.offset)==0);assert(length(fast.solution.catchPoint-head)==0);}
  auto angled=solveAngle({0,8,0},head,movement,35,28,.05,5);
  if(angled.valid)assert(length(point(angled,28,angled.time)-angled.catchPoint)<1e-6);
 }
 auto directAuto=planThrow({0,8,0},{150,8,0},{},28,.05,0,true,35,true,60,false,true,athletic,48,{},true,5);
 assert(directAuto.solution.angle<20); // No arbitrary distance-based lob.
 auto jump95=planCoverageThrow({0,8,0},{100,8,0},{},28,.05,{},true,true,7,196.2);
 assert(jump95.solution.valid&&jump95.power==95&&jump95.offset.y>=3&&jump95.offset.y<=7);
 assert(jump95.offset.x==0&&jump95.offset.z==0);
 assert(length(point(jump95.solution,28,jump95.solution.time)-jump95.solution.catchPoint)<1e-5);
 assert(!planCoverageThrow({0,8,0},{100,8,0},{},28,.05,{},true,true,0,196.2).solution.valid);
 auto noJump=planCoverageThrow({0,8,0},{100,8,0},{},28,.05,{},true,false,0,196.2);
 assert(noJump.solution.valid&&noJump.power<95&&noJump.offset.y==0);
 std::vector<Defender> underneath={{{45,8,0},{},22,10,12}};
 auto adaptive=planCoverageThrow({0,8,0},{150,8,0},{},28,.05,underneath,true,false,7,196.2);
 auto forced95=planCoverageThrow({0,8,0},{150,8,0},{},28,.05,underneath,true,true,7,196.2);
 assert(adaptive.solution.valid&&forced95.solution.valid);
 assert(adaptive.power<95&&adaptive.margin>.15&&forced95.margin<=.15);
 auto retained=planCoverageThrow({0,8,0},{150.1,8,0},{},28,.05,underneath,true,false,7,196.2,adaptive.offset,adaptive.power,adaptive.lob);
 assert(retained.power==adaptive.power&&retained.lob==adaptive.lob);
 std::vector<Defender> onReceiver={{{100,8,0},{},22,9,12}};
 auto contested=planCoverageThrow({0,8,0},{100,8,0},{},28,.05,onReceiver,true,false,7,196.2);
 assert(contested.solution.valid&&contested.margin<=.15); // A jump does not create guaranteed exclusivity.
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
