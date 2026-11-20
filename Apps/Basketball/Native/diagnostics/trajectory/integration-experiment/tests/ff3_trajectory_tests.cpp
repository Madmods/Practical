#include "../src/ff3_trajectory_adapter.hpp"
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <iomanip>
int checks=0;
void check(bool value,const char* message){++checks;if(!value){std::cerr<<message<<'\n';std::exit(1);}}
int main(){using namespace ff3qb;
 std::ofstream cross("build/joint-dispatch-fixtures.csv");cross<<std::setprecision(17)<<"tx,ty,tz,mx,my,mz,power,lob,time,vx,vy,vz\n";
 double worst=0;
 for(bool beam:{false,true})for(double distance:{15.,50.,150.,250.})for(double lateral:{0.,16.}){
  ff3Trajectory::Adapter adapter;Defender wr{{distance,8,0},{0,0,lateral},20,6.37,12,5.88,3.88,4.88,196.2,true};
  ff3Trajectory::Result r;
  for(int i=0;i<12;++i){auto start=std::chrono::steady_clock::now();r=adapter.update({0,8,0},wr.position,wr.velocity,28,.06,0,wr,{},beam,{},true);worst=std::max(worst,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());if(r.plan.solution.valid)break;}
  if(!r.plan.solution.valid){std::cerr<<"beam "<<beam<<" distance "<<distance<<" lateral "<<lateral<<'\n';if(auto d=adapter.decision()){std::cerr<<d->reason<<" selected="<<d->hasSelection<<" speed="<<d->selected.speed<<" time="<<d->selected.candidate.flightTime<<" why="<<d->selected.why<<'\n';for(int i=0;i<7;++i)std::cerr<<i<<":"<<d->rejectCounts[i]<<" ";std::cerr<<'\n';}}
  check(r.plan.solution.valid,"open scene has no result");const auto& p=r.plan;
  check(p.power<=95&&p.power>=1,"power limit");
  check(receiverGap(wr,p.solution.catchPoint,p.solution.time+.06)<=-.25,"receiver gate");
  auto sent=solve({0,8,0},wr.position+p.offset,wr.velocity,p.power,28,.06,p.lob,5);
  check(sent.valid&&length(sent.velocity-p.solution.velocity)<1e-4,"dispatch mismatch");
  check(length(point(p.solution,28,p.solution.time)-p.solution.catchPoint)<1e-6,"endpoint mismatch");
  auto target=wr.position+p.offset;cross<<target.x<<','<<target.y<<','<<target.z<<','<<wr.velocity.x<<','<<wr.velocity.y<<','<<wr.velocity.z<<','<<p.power<<','<<p.lob<<','<<p.solution.time<<','<<p.solution.velocity.x<<','<<p.solution.velocity.y<<','<<p.solution.velocity.z<<'\n';
  if(beam&&lateral==0)check(p.power>94.99,"beam missed feasible 95-power seed");
 }
 {ff3Trajectory::Adapter adapter;Defender wr{{70,8,0},{},20,6.37,12,5.88,3.88,4.88,196.2,true};Defender db=wr;db.redirect=false;db.speed=22;
  auto r=adapter.update({0,8,0},wr.position,{},28,.06,0,wr,{db},true,{});
  if(r.plan.solution.valid)check(r.diagnostics->decision.selected.verdict!=planner::Verdict::PredictedClear,"overlap falsely clear");
 }
 {ff3Trajectory::Adapter adapter;Defender wr{{70,8,0},{},20,6.37,12,5.88,3.88,4.88,196.2,true};
  auto first=adapter.update({0,8,0},wr.position,{},28,.06,0,wr,{},true,{});
  check(first.plan.solution.valid,"stability setup");
  for(int i=0;i<20;++i){wr.position.z=(i%2?.01:-.01);auto p=adapter.update({0,8,0},wr.position,{},28,.06,0,wr,{},true,{},false);check(p.plan.solution.valid&&std::abs(p.plan.solution.angle-first.plan.solution.angle)<.1,"stationary angle chatter");}
 }
 std::cout<<checks<<" integration checks passed; worst measured search "<<worst<<" ms\n";
}
