#include "../src/ff3_qb_logic.hpp"
#include <iostream>
#include <cstdlib>
using namespace ff3qb;
int main(){
 unsigned checks=0;auto check=[&](bool ok,const char* why){++checks;if(!ok){std::cerr<<"FAIL "<<checks<<": "<<why<<'\n';std::exit(1);}};
 FieldBounds field{true,{0,3,0},{1,0,0},{0,0,1},80,180};
 std::vector<FieldLine> lines={{{-82,2.5,0},{4,1,360},{1,0,0},{0,0,1}},{{82,2.5,0},{4,1,360},{1,0,0},{0,0,1}},{{0,2.5,-182},{168,1,4},{1,0,0},{0,0,1}},{{0,2.5,182},{168,1,4},{1,0,0},{0,0,1}}};
 auto measured=fieldFromLines({0,2.55,0},{1,0,0},{0,0,1},lines);
 check(measured.valid()&&std::abs(measured.halfWidth-80)<1e-6&&std::abs(measured.halfLength-180)<1e-6,"inner white-line edges include both end zones");
 auto missing=lines;missing.pop_back();check(!fieldFromLines({},field.right,field.forward,missing).valid(),"incomplete perimeter fails closed");
 auto malformed=lines;malformed[0].size.z=40;check(!fieldFromLines({},field.right,field.forward,malformed).valid(),"unrelated short stripes rejected");
 malformed=lines;malformed[0].right={NAN,0,0};check(!fieldFromLines({},field.right,field.forward,malformed).valid(),"bad orientation rejected");
 for(double angle:{0.,.45,1.57079632679,3.14159265359}){
  Vec right{std::cos(angle),0,std::sin(angle)},forward{-std::sin(angle),0,std::cos(angle)},translation{130,3,-90};
  auto transform=[&](Vec p){return translation+right*p.x+Vec{0,p.y-3,0}+forward*p.z;};
  FieldBounds rotated{true,translation,right,forward,80,180};
  auto transformed=lines;for(auto& l:transformed){l.position=transform(l.position);l.right=right;l.forward=forward;}
  auto derived=fieldFromLines(transform({0,2.55,0}),right,forward,transformed);check(derived.valid()&&std::abs(derived.halfWidth-80)<1e-6,"translated rotated fields measured");
  for(int axis:{0,1})for(double sign:{-1.,1.}){
   Vec body=axis?Vec{0,8,sign*178}:Vec{sign*78,8,0};Defender receiver;receiver.position=transform(body);receiver.speed=0;receiver.jumpHeight=0;receiver.diveReach=0;
   auto make=[&](Vec p){return Solution{true,transform({0,8,0}),{},transform(p),1,0};};
   Vec edge=axis?Vec{0,8,sign*181}:Vec{sign*81,8,0};
   check(inBoundsCatch(make(edge),receiver,.05,rotated),"reachable over-line ball allowed with body in bounds");
   Vec far=axis?Vec{0,8,sign*190}:Vec{sign*90,8,0};
   check(!inBoundsCatch(make(far),receiver,.05,rotated),"stadium/deep-out pass rejected");
   receiver.position=transform(axis?Vec{0,8,sign*181}:Vec{sign*81,8,0});check(!inBoundsCatch(make(edge),receiver,.05,rotated),"out-of-bounds receiver cannot justify a throw");
   receiver.position=transform(body);receiver.velocity=(axis?forward:right)*(sign*20);
   check(!inBoundsCatch(make(edge),receiver,.05,rotated),"route running out before arrival rejected");
  }
 }
 Defender receiver;receiver.position={78,8,178};receiver.speed=0;receiver.jumpHeight=0;
 check(!inBoundsCatch({true,{0,8,0},{},{82,8,182},1,0},receiver,.05,field),"corner shares one diagonal reach budget");
 check(!inBoundsCatch({true,{0,8,0},{},{78,35,178},1,0},receiver,.05,field),"in-bounds but vertically unreachable throw rejected");
 auto unknown=field;unknown.known=false;check(!inBoundsCatch({true,{0,8,0},{},{78,8,178},1,0},receiver,.05,unknown),"missing field blocks assistance");
 for(double angle:{0.,.45,1.57079632679}){
  Vec right{std::cos(angle),0,std::sin(angle)},forward{-std::sin(angle),0,std::cos(angle)},translation{130,3,-90};
  auto transform=[&](Vec p){return translation+right*p.x+Vec{0,p.y-3,0}+forward*p.z;};
  FieldBounds rotated{true,translation,right,forward,80,180};
  for(int axis:{0,1})for(double sign:{-1.,1.}){
   auto point=[&](double distance){return transform(axis?Vec{0,8,sign*(180+distance)}:Vec{sign*(80+distance),8,0});};
   for(double margin:{0.,.5,2.,4.,10.,20.}){
    check(boundaryAllows(point(margin),rotated,margin),"slider includes its exact edge on rotated sidelines and end zones");
    check(!boundaryAllows(point(margin+.1),rotated,margin),"slider rejects beyond its edge on rotated sidelines and end zones");
   }
  }
  check(!boundaryAllows(transform({81,8,181}),rotated,1.4)&&boundaryAllows(transform({81,8,181}),rotated,1.5),"positive corner distance uses one radial budget");
 }
 for(double invalid:{double(NAN),double(INFINITY),-.5,20.5})check(!boundaryAllows({0,8,0},field,invalid),"invalid boundary limit rejected");
 receiver.position={78,8,0};receiver.velocity={};
 auto nearEdge=Solution{true,{0,8,0},{},{83,8,0},1,0};
 check(!inBoundsCatch(nearEdge,receiver,.05,field,2)&&inBoundsCatch(nearEdge,receiver,.05,field,4),"extended limit permits a reachable outside catch");
 receiver.position={81,8,0};check(!inBoundsCatch(nearEdge,receiver,.05,field,20),"extended limit does not permit out-of-bounds receiver bodies");
 for(double margin:{0.,.5,2.,10.,20.})check(boundaryAllows({79.9,8,0},field,margin),"slider never adds an inward catch inset");
 receiver.position={77,8,0};receiver.redirect=false;
 auto recovered=planThrow({0,8,0},{84,8,0},{},28,.05,0,false,20,true,95,false,false,{},20,{},false,5,3,50,&field,&receiver);
 check(recovered.solution.valid&&inBoundsCatch(recovered.solution,receiver,.05,field)&&recovered.offset.x<0,"planner brings excessive lead back to a legal reachable catch even with Smart fit off");
 receiver.position={78,8,0};receiver.velocity={22,0,0};
 auto blocked=planCoverageThrow({0,8,0},receiver.position,receiver.velocity,28,.05,{},true,true,7,196.2,{},95,false,NAN,true,.1,&receiver,0,nullptr,0,3,85,&field);
 check(!blocked.solution.valid,"no plan when every arrival puts receiver out of bounds");
 for(int x=-100;x<=100;x+=4)for(int z=-200;z<=200;z+=8){
  receiver.position={std::clamp(double(x),-78.,78.),8,std::clamp(double(z),-178.,178.)};receiver.velocity={};
  Solution s{true,{0,8,0},{},{double(x),8,double(z)},1,0};
  if(inBoundsCatch(s,receiver,.05,field)){auto delta=s.catchPoint-field.closest(s.catchPoint);delta.y=0;check(length(delta)<=3.000001,"accepted sample stays in constrained catch strip");}
 }
 std::cout<<checks<<" field discovery, sideline, end-zone, rotation, reach and planner checks passed\n";
}
