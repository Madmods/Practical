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
 assert(baseline.solution.valid&&fit.solution.valid&&fit.coverageKnown);assert(fit.margin>=baseline.margin-1e-6);assert(length(fit.offset)<=6);
 auto unknown=planThrow({0,8,0},{60,7,0},{},28,.05,0,true,35,true,95,false,true,{});assert(!unknown.coverageKnown&&length(unknown.offset)==0);
 auto manual=planThrow({0,8,0},{60,7,0},{},28,.05,0,false,35,true,95,false,false,{});assert(manual.solution.valid&&manual.solution.angle==35);
 assert(!planThrow({0,8,0},{340,7,0},{},28,.05,0,false,5,true,95,false,false,{}).solution.valid);
 for(bool visible:{false,true})for(bool right:{false,true})for(bool first:{false,true})assert(cursorModeAllowed(visible,right,first)==(visible&&!right&&!first));
 const float identity[]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};float tilted[]={1,0,0,0,0,.8f,.6f,0,0,0,1,0,0,0,0,1};
 assert(cameraStable(identity,identity));assert(!cameraStable(identity,tilted));tilted[4]=NAN;assert(!cameraStable(identity,tilted));
 assert(visiblePower(1,2,"95")==95);assert(visiblePower(1,0,"75")==0);assert(visiblePower(0,2,"95")==0);
 assert(visiblePower(1,2,"96")==-1);assert(visiblePower(1,2,"0")==-1);assert(visiblePower(1,2,"95x")==-1);assert(visiblePower(1,2,"")==-1);
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
