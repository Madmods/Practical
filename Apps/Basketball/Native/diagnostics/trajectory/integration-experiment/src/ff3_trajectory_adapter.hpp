#pragma once
#include "ff3_qb_logic.hpp"
#include "trajectory/planner.hpp"
#include <memory>

namespace ff3Trajectory {
inline projectile::Vec3 vector(ff3qb::Vec v){return {v.x,v.y,v.z};}
inline ff3qb::Vec vector(projectile::Vec3 v){return {v.x,v.y,v.z};}
struct Diagnostics {
 planner::Scene scene;
 planner::Decision decision;
 std::vector<ff3qb::Vec> alternatives;
 ff3qb::RiskForecast risk;
 double clearance=1000;
 bool calibrated=false; // Acceleration/reaction estimates still need measurement.
};
struct Result {ff3qb::Plan plan;std::shared_ptr<Diagnostics> diagnostics;};

class Adapter {
 std::unique_ptr<planner::Planner> planner_;
 bool beam_=false;double gravity_=0,latency_=0,ground_=0,height_=0;
 ff3qb::Vec origin_,target_,movement_,bias_;
 ff3qb::Defender receiver_;
 std::vector<ff3qb::Defender> defenders_;
 planner::Scene scene_;
 unsigned pilot_=0;
 std::vector<ff3qb::Vec> alternatives_;
 static planner::Agent agent(const ff3qb::Defender& d,double ground,double latency,bool receiver,size_t index){
  planner::Agent a;a.name=std::to_string(index);
  a.position=vector(d.position+ff3qb::Vec{d.velocity.x,0,d.velocity.z}*latency);
  a.velocity=vector(ff3qb::Vec{d.velocity.x,0,d.velocity.z});
  auto& p=a.params;
  p.maxSpeed=std::clamp(d.speed,1.,45.);
  // Explicit estimates in studs/s^2, not the package's metre-based defaults.
  p.accel=receiver?90:220;p.decel=receiver?110:220;p.turnAccel=receiver?75:220;
  p.reactionTime=receiver?.18:0;
  p.chestHeight=std::max(.8,d.position.y-ground-1.5);
  p.reachHeight=std::max(p.chestHeight,d.position.y-ground+d.above);
  p.jumpReachHeight=p.reachHeight+std::max(0.,d.jumpHeight);
  p.jumpPrepTime=receiver?.18:0;
  p.armReach=std::max(.5,d.radius-.5);p.diveReach=std::max(0.,d.diveReach);
  p.diveMaxHeight=std::max(1.,d.position.y-ground+1);
  a.uncertainty=receiver?planner::Uncertainty{.25,.8,.03}:planner::Uncertainty{.4,1.2,.05};
  return a;
 }
 ff3qb::Solution solution(const planner::Evaluation& e)const{
  if(!e.feasible())return {};
  return {true,vector(e.launch.position),vector(e.launch.velocity),vector(e.destination),e.candidate.flightTime,e.elevationDeg};
 }
 ff3qb::Vec offset(projectile::Vec3 destination,double time)const{
  return vector(destination)-target_-movement_*(time+latency_)-bias_;
 }
 bool compatible(projectile::Vec3 destination,double time)const{
  return ff3qb::placementAllowed(offset(destination,time),movement_,time,receiver_.redirect)
   &&ff3qb::receiverGap(receiver_,vector(destination),time+latency_)<=-.25;
 }
 void adjust(planner::Evaluation& e)const{
  const auto arc=solution(e);
  const double margin=defenders_.empty()?1000:ff3qb::coverageMargin(arc,gravity_,defenders_,latency_,nullptr,-INFINITY,ground_);
  // The helper's measured catch envelope remains an additional conservative
  // constraint. Rank it here as well, rather than merely flagging it afterward.
  if(margin<=ff3qb::catchSafetyMargin){
   if(e.verdict!=planner::Verdict::Contested)e.verdict=margin<=0?planner::Verdict::Contested:planner::Verdict::Uncertain;
   e.why="Measured catch envelope overlaps the full flight";
  }
  e.costTerms.verdict=e.verdict==planner::Verdict::Contested?1000:e.verdict==planner::Verdict::Uncertain?100:0;
  e.costTerms.risk+=std::max(0.,ff3qb::catchSafetyMargin-margin)*20;
  e.costTerms.time+=(95-e.speed)*(beam_?.20:.035);
  e.cost=e.costTerms.total();
 }
 planner::PlannerConfig configuration() {
  planner::PlannerConfig c;
  c.launch.releaseHeight=c.launch.releaseForward=c.launch.releaseSide=0;
  c.launch.releaseAlongLaunch=5;c.launch.minSpeed=1;c.launch.maxSpeed=95;
  c.launch.minElevationDeg=-30;c.launch.maxElevationDeg=75;
  c.launch.minFlightTime=.06;c.launch.maxFlightTime=5.56;
  c.search.timeStep=.1;c.search.depthOffsets={-22,-14,-7,0,7,14,22};
  c.search.lateralOffsets={-16,-8,0,8,16};c.search.refineOffset=1;c.search.offsetMajor=true;
  c.search.workBudget=60000;c.search.timeBudgetMs=5;c.search.poolSize=2;c.search.refineCount=3;c.search.pruneDistantThreats=true;
  c.score.uncertainCost=100;c.score.contestedCost=1000;c.score.blendTimeStep=.025;c.score.blendOffsetStep=.5;
  c.score.switchMargin=.15;c.score.noiseSigmas=1.5;c.score.adjustWeight=.06;c.score.preserveBlendCost=true;
  c.requireAfterMissClear=true;
  c.destinationAllowed=[this](projectile::Vec3 p,double time){return compatible(p,time);};
  c.adjustEvaluation=[this](planner::Evaluation& e){adjust(e);};
  return c;
 }
public:
 const planner::Decision* decision()const{return planner_?&planner_->decision():nullptr;}
 void reset(){planner_.reset();pilot_=0;alternatives_.clear();}
 Result update(ff3qb::Vec origin,ff3qb::Vec target,ff3qb::Vec movement,double gravity,double latency,
               double ground,const ff3qb::Defender& receiver,const std::vector<ff3qb::Defender>& defenders,
               bool beam,ff3qb::Vec bias,bool search=true){
  Result out;
  if(!ff3qb::finite(origin)||!ff3qb::finite(target)||!ff3qb::finite(movement)||!std::isfinite(ground)||gravity<1||gravity>300)return out;
  if(!planner_||beam_!=beam||std::abs(gravity_-gravity)>.001||height_!=bias.y){
   beam_=beam;gravity_=gravity;height_=bias.y;pilot_=0;alternatives_.clear();planner_=std::make_unique<planner::Planner>(configuration());
  }
  origin_=origin;target_=target;movement_=movement;latency_=latency;ground_=ground;
  receiver_=receiver;defenders_=defenders;bias_=bias;
  scene_={};scene_.world={1,gravity,ground+.6};scene_.passer=vector(origin);
  scene_.destinationBias=vector(bias);
  scene_.receiver=agent(receiver,scene_.world.groundY,latency,true,0);
  for(size_t i=0;i<defenders.size();++i)scene_.defenders.push_back(agent(defenders[i],scene_.world.groundY,latency,false,i));
  // Seed exact maximum-power roots so the time grid cannot miss a narrow 95
  // window. The general search remains free to choose lower speeds/higher arcs.
  std::vector<planner::Candidate> seeds;
  const auto modes=planner::catchModes(scene_.receiver.params);
  for(size_t mode=0;mode<modes.size();++mode){
   ff3qb::Vec aim=target+bias;aim.y=scene_.world.groundY+modes[mode].height+bias.y;
   auto root=ff3qb::solve(origin,aim,movement,95,gravity,latency,false,5);
   if(root.valid)seeds.push_back({root.time,0,0,int(mode)});
  }
  if(planner_->decision().hasSelection&&planner_->decision().selected.verdict==planner::Verdict::Contested){
   seeds.clear();
   const double fast=std::clamp((ff3qb::length(target-origin)-5)/95.,.12,2.);
   // Probe the outer placements early instead of waiting for the fine grid to
   // finish every small adjustment while the current lane remains covered.
   const planner::Candidate probes[]={{fast*2.5,22,0,1},{fast*3,22,0,1},{fast*2.5,-22,0,1},{fast*3,-22,0,1},
    {fast*1.75,0,16,1},{fast*2.5,0,16,1},{fast*1.75,0,-16,1},{fast*2.5,0,-16,1},
    {fast*1.35,14,0,0},{fast*1.75,14,0,0},{fast*1.35,-14,0,0},{fast*1.75,-14,0,0}};
   for(int i=0;i<2;++i)seeds.push_back(probes[(pilot_++)%12]);
  }
  auto e=planner_->revalidate(scene_);
  planner::Decision decision;
  if(search||!e.feasible())decision=planner_->update(scene_,seeds);
  else{
   const auto& previous=planner_->decision();decision.selected=e;decision.hasSelection=true;
   decision.reason=previous.reason;decision.update=previous.update;
   decision.completedSweeps=previous.completedSweeps;decision.sweepProgress=previous.sweepProgress;
   decision.switches=previous.switches;
  }
  if(!decision.hasSelection)return out;
  e=decision.selected;
  const auto arc=solution(e);
  if(!arc.valid||!compatible(e.destination,e.candidate.flightTime))return out;
  const auto fittedOffset=offset(e.destination,e.candidate.flightTime);
  // Verify the same ballistic root will be reconstructed by the existing
  // dispatch helper, including latency and the five-stud launch displacement.
  auto high=ff3qb::solve(origin,target+bias+fittedOffset,movement,e.speed,gravity,latency,true,5);
  const bool lob=high.valid&&std::abs(high.time-arc.time)<1e-5;
  auto sent=ff3qb::solve(origin,target+bias+fittedOffset,movement,e.speed,gravity,latency,lob,5);
  if(!sent.valid||ff3qb::length(sent.velocity-arc.velocity)>1e-4)return out;
  auto d=std::make_shared<Diagnostics>();d->scene=scene_;d->decision=std::move(decision);
  d->risk=ff3qb::riskForecast(arc,gravity,defenders,latency,ground);
  d->clearance=d->risk.margin;
  // The new forecast can add uncertainty; it cannot erase measured overlap.
  d->risk.possible=d->risk.possible||(!d->risk.intable&&e.verdict!=planner::Verdict::PredictedClear);
  if(!d->decision.considered.empty()){
   alternatives_.clear();for(const auto& candidate:d->decision.considered)if(candidate.feasible()&&alternatives_.size()<12)alternatives_.push_back(vector(candidate.destination));
  }
  d->alternatives=alternatives_;
  out.plan={arc,fittedOffset,std::min(95.,e.speed),d->clearance,!defenders.empty(),lob};out.diagnostics=std::move(d);
  return out;
 }
};
}
