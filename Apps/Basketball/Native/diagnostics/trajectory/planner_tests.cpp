// Regression tests for planner.hpp and scenarios.hpp.
#include "check.hpp"
#include "scenarios.hpp"
#include <string>
using namespace planner;

namespace {
const World kMetres=World::metres();

Agent receiver(Vec3 pos,Vec3 vel={}){return {"WR",pos,vel,MoverParams::receiver(),Uncertainty::receiver()};}
Agent defender(std::string name,Vec3 pos,Vec3 vel={}){return {std::move(name),pos,vel,MoverParams::defender(),Uncertainty::defender()};}
Scene scene(Agent r,std::vector<Agent> defenders={}){
 Scene s;s.world=kMetres;s.passer={0,0,0};s.receiver=std::move(r);s.defenders=std::move(defenders);
 return s;
}
const PlannerConfig& config(){static const PlannerConfig c=PlannerConfig::defaults(kMetres);return c;}
// Runs until the grid has been fully searched twice (so refinement has settled).
const Decision& settle(Planner& p,const Scene& s,int maxUpdates=400){
 const Decision* d=&p.update(s);
 for(int i=0;i<maxUpdates&&d->completedSweeps<2;++i)d=&p.update(s);
 return *d;
}
const Evaluation* find(const std::vector<Evaluation>& all,double T,double depth,double lateral,int mode){
 for(const Evaluation& e:all)if(sameCandidate(e.candidate,{T,depth,lateral,mode}))return &e;
 return nullptr;
}

// The launch velocity is solved exactly, and what is displayed is that arc.
void launchExactness(){
 Scene s=scene(receiver({12,0,-6},{6,0,3}),{defender("CB",{15,0,-3},{4,0,2}),defender("S",{25,0,0})});
 int feasible=0;
 for(const Evaluation& e:Planner(config()).evaluateGrid(s)){
  if(!e.feasible())continue;
  ++feasible;
  const double T=e.candidate.flightTime;
  const auto path=displayPath(kMetres,e,30);
  const double scale=projectile::positionScale(e.launch,kMetres.gravityVector(),T)+projectile::maxAbs(e.destination);
  CHECK(path.front().position.x==e.launch.position.x&&path.back().time==T);
  CHECK(check::near(path.back().position.x,e.destination.x,scale)&&check::near(path.back().position.y,e.destination.y,scale)&&check::near(path.back().position.z,e.destination.z,scale));
  CHECK(e.speed>=config().launch.minSpeed&&e.speed<=config().launch.maxSpeed);
  CHECK(e.elevationDeg>=config().launch.minElevationDeg&&e.elevationDeg<=config().launch.maxElevationDeg);
  CHECK(e.landingTime>=T&&e.threats.size()==4);
  CHECK(std::fabs(e.costTerms.total()-e.cost)<1e-12);
 }
 CHECK(feasible>100);
}

// A scene in yards ranks and judges candidates like the same scene in metres.
void unitsInvariance(){
 const World y=World::yards();
 auto toYards=[&](Vec3 v){return Vec3{y.fromMetres(v.x),y.fromMetres(v.y),y.fromMetres(v.z)};};
 Scene m=scene(receiver({14,0,4},{5,0,2}),{defender("CB",{17,0,7},{3,0,1}),defender("LB",{8,0,1})});
 Scene yd=m;
 yd.world=y;
 auto convert=[&](Agent& a){a.position=toYards(a.position);a.velocity=toYards(a.velocity);a.params=a.params.scaledTo(y);a.uncertainty=a.uncertainty.scaledTo(y);};
 convert(yd.receiver);
 for(Agent& d:yd.defenders)convert(d);
 const PlannerConfig cy=PlannerConfig::defaults(y);
 const auto em=Planner(config()).evaluateGrid(m);
 const auto ey=Planner(cy).evaluateGrid(yd);
 CHECK(em.size()==ey.size());
 int same=0,compared=0;
 double worstSpeed=0,worstRisk=0;
 for(size_t i=0;i<em.size()&&i<ey.size();++i){
  ++compared;
  if(em[i].verdict==ey[i].verdict&&em[i].reject==ey[i].reject)++same;
  if(em[i].feasible()&&ey[i].feasible()){
   worstSpeed=std::fmax(worstSpeed,std::fabs(y.toMetres(ey[i].speed)-em[i].speed)/em[i].speed);
   worstRisk=std::fmax(worstRisk,std::fabs(ey[i].preCatchRisk-em[i].preCatchRisk));
  }
 }
 CHECK(same>=compared-2);  // allow a knife-edge candidate to round the other way
 CHECK(worstSpeed<1e-12&&worstRisk<1e-9);
 auto best=[](const std::vector<Evaluation>& all){
  const Evaluation* b=nullptr;
  for(const Evaluation& e:all)if(e.feasible()&&(!b||e.cost<b->cost))b=&e;
  return b;
 };
 CHECK(best(em)&&best(ey)&&best(em)->label==best(ey)->label&&std::fabs(best(em)->cost-best(ey)->cost)<1e-9);
}

void receiverReachability(){
 const Agent still=receiver({0,0,0});
 // Reaction 0.25 s then 6.5 m/s^2: 3 m is out of reach in 1.0 s, in reach in 1.5 s.
 CHECK(!innerReach(still,{3,0,0},1.0).feasible);
 const ReachPlan ok=innerReach(still,{3,0,0},1.5);
 CHECK(ok.feasible&&ok.effort<=1);
 CHECK(horizontalLength(planPosition(ok,1.5)-Vec3{3,0,0})<1e-12);
 // Nothing moves before the reaction time.
 CHECK(!innerReach(still,{.2,0,0},.2).feasible);
 CHECK(innerReach(receiver({0,0,0},{5,0,0}),{1,0,0},.2).feasible);
 // Turning and braking limits: at full speed the receiver cannot reverse 2 m in 0.6 s...
 const Agent fast=receiver({0,0,0},{9,0,0});
 CHECK(!innerReach(fast,{-2,0,0},.6).feasible);
 // ...continues along its heading, and can veer 1.5 m by giving up some depth...
 CHECK(innerReach(fast,{9,0,0},1.0).feasible);
 CHECK(innerReach(fast,{8,0,1.5},1.0).feasible);
 // ...but not veer while keeping full depth: that would exceed top speed.
 CHECK(!innerReach(fast,{9,0,1.5},1.0).feasible);
 // Catch modes: a jump costs time, a dive adds reach.
 const auto modes=catchModes(still.params);
 CHECK(receiverReach(still,{1.5,0,0},modes[2],.6).feasible&&!receiverReach(still,{1.5,0,0},modes[0],.6).feasible);
 CHECK(!receiverReach(still,{0,0,0},modes[1],.2).feasible);
}

// Regression for "zero velocity must not imply stationary": a defender standing
// still 1 m from the ball's path can still get there after reacting.
void zeroVelocityDefender(){
 const projectile::State ball{{0,1.8,0},{15,3,0}};  // passes x=10 at t=0.667 s, 1.6 m high
 long work=0;
 const Threat near=sweepThreat(kMetres,defender("LB",{10,0,1}),ball,0,1,1./60,Phase::BeforeCatch,work);
 CHECK(near.intercept&&near.interceptTime>MoverParams::defender().reactionTime);
 // A stationary model (reach = arm only) would have said 1.0 - 0.6 > 0: unreachable.
 CHECK(1.0-MoverParams::defender().armReach>0);
 const Threat far=sweepThreat(kMetres,defender("LB",{10,0,3}),ball,0,1,1./60,Phase::BeforeCatch,work);
 CHECK(!far.intercept&&far.gap>0&&far.probability>0&&far.probability<.5);
 // A defender that will not react in time cannot get there.
 Agent asleep=defender("LB",{10,0,1});
 asleep.params.reactionTime=5;
 asleep.uncertainty={0,0,0};
 CHECK(!sweepThreat(kMetres,asleep,ball,0,1,1./60,Phase::BeforeCatch,work).intercept);
}

void heightMatters(){
 long work=0;
 const Agent lb=defender("LB",{10,0,0});
 // Same horizontal path, different heights over the defender at t ~ 0.67 s.
 const projectile::State low{{0,1.8,0},{15,3,0}};
 const projectile::State high{{0,1.8,0},{15,9,0}};
 const Threat a=sweepThreat(kMetres,lb,low,0,.667,1./60,Phase::BeforeCatch,work);
 const Threat b=sweepThreat(kMetres,lb,high,0,.667,1./60,Phase::BeforeCatch,work);
 CHECK(a.intercept);
 CHECK(!b.intercept);
 // Over the defender the high ball is out of jumping reach.
 CHECK(projectile::advance(high,kMetres,.667).position.y>lb.params.jumpReachHeight);
}

// The defender gets to the destination, but only after the catch: the throw is
// not contested, the risk shows up after a miss.
void arrivalTimeAndMiss(){
 Scene s=scene(receiver({15,0,0}),{defender("S",{19,0,0})});
 const Evaluation e=evaluate(s,config(),{.9,0,0,0});
 CHECK(e.feasible());
 CHECK(e.threats.size()==2&&!e.threats[0].intercept&&e.threats[1].intercept);
 CHECK(e.threats[1].interceptTime>.9&&e.verdict!=Verdict::Contested);
 CHECK(e.postMissRisk>.5&&e.preCatchRisk<.1);
}

void interceptionTimePrecision(){
 Agent post=defender("post",{10,0,.3});
 post.params.maxSpeed=1e-12;post.params.accel=post.params.decel=post.params.turnAccel=1e-12;
 post.params.reactionTime=0;post.params.diveReach=0;post.params.diveMaxHeight=.1;
 post.uncertainty={0,0,0};
 long work=0;
 const Threat t=sweepThreat(kMetres,post,{{0,1.5,0},{20,3,0}},0,1,1./60,Phase::BeforeCatch,work);
 const double exact=(10-std::sqrt(.6*.6-.3*.3))/20;
 CHECK(t.intercept&&std::fabs(t.interceptTime-exact)<1e-9);
 CHECK(std::fabs(t.interceptPoint.x-20*exact)<1e-8);
}

// A defender between passer and receiver blocks the flat throw; the joint search
// finds a higher (or re-aimed) trajectory that is not contested.
void lobOverDefender(){
 Scene s=scene(receiver({20,0,0}),{defender("LB",{10,0,0})});
 const auto all=Planner(config()).evaluateGrid(s);
 const Evaluation* flat=nullptr;
 for(const Evaluation& e:all)
  if(e.feasible()&&e.candidate.mode==0&&e.candidate.depth==0&&e.candidate.lateral==0&&(!flat||e.candidate.flightTime<flat->candidate.flightTime))flat=&e;
 CHECK(flat&&flat->verdict==Verdict::Contested);
 Planner p(config());
 const Decision& d=settle(p,s);
 CHECK(d.hasSelection&&d.selected.verdict!=Verdict::Contested&&d.selected.verdict!=Verdict::Invalid);
 CHECK(flat&&d.selected.elevationDeg>flat->elevationDeg+5);
 CHECK(d.selected.threats[0].minSigmas>0);
}

// With nothing in the way, the fastest valid throw wins.
void fastWhenOpen(){
 Scene s=scene(receiver({15,0,0}));
 double fastestClear=kInf;
 for(const Evaluation& e:Planner(config()).evaluateGrid(s))
  if(e.verdict==Verdict::PredictedClear)fastestClear=std::fmin(fastestClear,e.candidate.flightTime);
 Planner p(config());
 const Decision& d=settle(p,s);
 CHECK(d.hasSelection&&d.selected.verdict==Verdict::PredictedClear);
 CHECK(d.selected.candidate.flightTime<=fastestClear+config().search.timeStep+1e-9);
 CHECK(d.selected.speed>=.75*config().launch.maxSpeed);
}

// A deep defender sits on the route: the planner adjusts the destination
// (comeback or lateral) rather than throwing into him.
void adjustAwayFromCoverage(){
 Scene s=scene(receiver({20,0,0},{8,0,0}),{defender("S",{23.5,0,0},{6,0,0})});
 Planner p(config());
 const Decision& d=settle(p,s);
 CHECK(d.hasSelection&&d.selected.verdict!=Verdict::Contested);
 CHECK(d.selected.candidate.depth<0||d.selected.candidate.lateral!=0);
 const Evaluation onRoute=evaluate(s,config(),{d.selected.candidate.flightTime,0,0,d.selected.candidate.mode});
 CHECK(!onRoute.feasible()||onRoute.cost>d.selected.cost);
}

void neverSafeWhenUnsupported(){
 Scene s=scene(receiver({15,0,0}),{defender("CB",{18,0,4})});
 const Evaluation tooFast=evaluate(s,config(),{.25,0,0,0});
 CHECK(tooFast.reject==Reject::Speed&&tooFast.verdict==Verdict::Invalid&&!tooFast.why.empty());
 const Evaluation unreachable=evaluate(s,config(),{.9,4,3,0});
 CHECK(unreachable.reject==Reject::ReceiverUnreachable&&unreachable.verdict==Verdict::Invalid);
 Scene close=scene(receiver({3,0,0}));
 const Evaluation steep=evaluate(close,config(),{3.4,0,0,0});
 CHECK(steep.reject==Reject::Elevation&&steep.verdict==Verdict::Invalid);
 const Evaluation badMode=evaluate(s,config(),{1,0,0,7});
 CHECK(badMode.reject==Reject::BadInput);
 Scene broken=s;
 broken.defenders[0].position.x=std::numeric_limits<double>::quiet_NaN();
 CHECK(evaluate(broken,config(),{1,0,0,0}).reject==Reject::BadInput);
 Planner p(config());
 CHECK(!p.update(broken).hasSelection);
 Scene badParams=s;
 badParams.receiver.params.maxSpeed=-1;
 CHECK(evaluate(badParams,config(),{1,0,0,0}).verdict==Verdict::Invalid);

 // Over every scenario's full grid: each label is backed by its conditions.
 int clear=0,contested=0;
 for(const auto& spec:scenarios::standardScenarios()){
  const auto r=scenarios::run(spec,config());
  for(const Evaluation& e:Planner(config()).evaluateGrid(r.throwScene)){
   CHECK(!e.why.empty());
   if(e.verdict==Verdict::PredictedClear){
    ++clear;
    bool ok=e.feasible()&&e.receiverPlan.feasible&&e.receiverMarginSigmas>=config().score.receiverSigmas&&e.receiverPlan.effort<=config().score.maxEffortClear;
    for(const Threat& t:e.threats)if(t.phase==Phase::BeforeCatch)ok=ok&&!t.intercept&&t.minSigmas>=config().score.clearSigmas;
    CHECK(ok);
   }
   if(e.verdict==Verdict::Contested){
    ++contested;
    bool any=false;
    for(const Threat& t:e.threats)any=any||(t.phase==Phase::BeforeCatch&&t.intercept);
    CHECK(any);
   }
   if(!e.feasible())CHECK(e.verdict==Verdict::Invalid);
  }
 }
 CHECK(clear>0&&contested>0);
}

void uncertaintyWidensRisk(){
 Agent sure=defender("CB",{17,0,3.5}),unsure=sure;
 sure.uncertainty.velocity=.2;
 unsure.uncertainty.velocity=2;
 const Candidate c{.8,0,0,0};
 const Evaluation a=evaluate(scene(receiver({15,0,0}),{sure}),config(),c);
 const Evaluation b=evaluate(scene(receiver({15,0,0}),{unsure}),config(),c);
 CHECK(a.feasible()&&b.feasible());
 CHECK(b.minClearanceSigmas<a.minClearanceSigmas);
 CHECK(b.preCatchRisk>a.preCatchRisk);
 CHECK(b.verdict<=a.verdict);
}

// Two mirror-image options and noisy observations: the choice must not flap.
void hysteresisPreventsFlapping(){
 auto flips=[](PlannerConfig cfg){
  Planner p(cfg);
  scenarios::Rng rng(5);
  int sideChanges=0,lastSide=0;
  for(int i=0;i<180;++i){
   Scene s=scene(receiver({15,0,0}),{defender("S",{19,0,0})});
   s.defenders[0].position.z+=rng.normal()*.05;
   s.defenders[0].velocity.z+=rng.normal()*.15;
   const Decision& d=p.update(s);
   if(!d.hasSelection||i<20)continue;
   const int side=d.selected.candidate.lateral>1e-9?1:d.selected.candidate.lateral< -1e-9?-1:0;
   if(lastSide!=0&&side!=0&&side!=lastSide)++sideChanges;
   if(side!=0)lastSide=side;
  }
  return sideChanges;
 };
 PlannerConfig off=config();
 off.score.switchMargin=0;off.score.continuityWeight=0;off.score.costSmoothing=1;off.score.noiseSigmas=0;off.score.blend=false;
 const int with=flips(config()),without=flips(off);
 CHECK(with<=1);
 CHECK(without>=with);
}

// When the selection stops being valid, the planner drops it in the same update and
// never keeps showing it. (Candidates are relative to the receiver's predicted path,
// so a change of direction moves the destination with the receiver; going out of
// range is what invalidates one.)
void promptSwitchWhenInvalid(){
 Scene s=scene(receiver({15,0,0},{6,0,0}));
 Planner p(config());
 const Decision& before=settle(p,s);
 CHECK(before.hasSelection);
 const Candidate old=before.selected.candidate;
 s.receiver.position={45,0,0};
 CHECK(evaluate(s,config(),old).reject==Reject::Speed);
 const Decision& after=p.update(s);
 CHECK(after.reason.find("invalid")!=std::string::npos);
 CHECK(!after.hasSelection||(after.selected.feasible()&&!sameCandidate(after.selected.candidate,old)));
 int updates=1;
 while(!p.decision().hasSelection&&updates<20){p.update(s);++updates;}
 CHECK(p.decision().hasSelection&&p.decision().selected.feasible());
 // A contested selection is replaced in the same update by any uncontested
 // alternative evaluated in that update, and the planner gets there quickly.
 Scene t=scene(receiver({20,0,0}));
 Planner q(config());
 const Candidate first=settle(q,t).selected.candidate;
 t.defenders.push_back(defender("LB",{10,0,0}));  // steps into the passing lane
 CHECK(evaluate(t,config(),first).verdict==Verdict::Contested);
 const Decision& d=q.update(t);
 bool alternative=false;
 for(const Evaluation& e:d.considered)alternative=alternative||(e.feasible()&&e.verdict>Verdict::Contested);
 CHECK(alternative);
 if(alternative){
  CHECK(d.hasSelection&&!sameCandidate(d.selected.candidate,first)&&d.selected.verdict>Verdict::Contested);
  CHECK(d.reason.find("contested")!=std::string::npos);
 }
}

void boundedWorkAndContinuation(){
 PlannerConfig cfg=config();
 cfg.search.workBudget=3000;
 Scene s=scene(receiver({12,0,-4},{6,0,2}),{defender("CB",{15,0,-2},{4,0,2}),defender("S",{24,0,0})});
 long maxSingle=0;
 const Planner fresh(cfg);
 for(const Candidate& c:fresh.grid()){long used=0;evaluate(s,cfg,c,&used);maxSingle=std::max(maxSingle,used);}
 Planner p(cfg);
 double lastPosition=-1;
 bool monotone=true,bounded=true;
 int updates=0;
 while(p.decision().completedSweeps<1&&updates<2000){
  s.receiver.position=s.receiver.position+s.receiver.velocity*(1./60);  // the scene keeps changing
  const Decision& d=p.update(s);
  ++updates;
  bounded=bounded&&d.work<=cfg.search.workBudget+3*maxSingle;
  const double position=d.completedSweeps+d.sweepProgress;
  monotone=monotone&&position>lastPosition;
  lastPosition=position;
 }
 CHECK(bounded);
 CHECK(monotone);  // the pass resumes where it stopped rather than restarting
 CHECK(p.decision().completedSweeps==1&&updates>3);
 // Optional wall-clock cap: still makes progress every update.
 PlannerConfig timed=config();
 timed.search.workBudget=2000000000;timed.search.timeBudgetMs=.2;
 Planner q(timed);
 const Decision& d1=q.update(s);
 CHECK(d1.considered.size()>=1&&d1.considered.size()<q.grid().size());
}

// Every frame of every scenario: the selection was evaluated in that update, is
// feasible, and its displayed arc ends exactly where the evaluation says.
void selectionInvariantsAcrossScenarios(){
 int frames=0,blends=0,bad=0;
 for(const auto& spec:scenarios::standardScenarios()){
  scenarios::Rng rng(spec.seed);
  scenarios::Actor rx=spec.receiver;
  auto ds=spec.defenders;
  Planner p(config());
  const double dt=1./60;
  for(int i=0;i*dt<=spec.duration;++i){
   const Scene s=scenarios::observe(spec,rx,ds,rng);
   const Decision& d=p.update(s);
   ++frames;
   int rejected=0;
   for(int k:d.rejectCounts)rejected+=k;
   if(rejected!=static_cast<int>(d.considered.size()))++bad;
   if(d.hasSelection){
    const Evaluation& e=d.selected;
    if(e.validatedUpdate!=d.update||!e.feasible())++bad;
    const auto path=displayPath(s.world,e,20);
    if(path.back().position.x!=projectile::advance(e.launch,s.world,e.candidate.flightTime).position.x)++bad;
    if(horizontalLength(path.back().position-e.destination)>1e-9)++bad;
    if(d.blending)++blends;
   }
   scenarios::stepActor(rx,rx,i*dt,dt);
   for(auto& a:ds)scenarios::stepActor(a,rx,i*dt,dt);
  }
 }
 CHECK(bad==0);
 CHECK(frames>1000);
 CHECK(blends>0);  // the validated-blend path is exercised
}

// Against the scripted truth, a "predicted clear" throw is never defended.
void predictedClearHoldsInSimulation(){
 int clear=0;
 for(const auto& spec:scenarios::standardScenarios()){
  const auto r=scenarios::run(spec,config());
  CHECK(r.throwDecision.hasSelection);
  if(r.throwDecision.hasSelection&&r.throwDecision.selected.verdict==Verdict::PredictedClear){
   ++clear;
   CHECK(r.outcome.result=="caught");
  }
  CHECK(r.outcome.result!="not thrown");
 }
 CHECK(clear>=2);
}

void diagnostics(){
 CHECK(riskAssumptions(config()).size()>=6);
 Scene s=scene(receiver({15,0,0}),{defender("CB",{18,0,4})});
 Planner p(config());
 const Decision& d=p.update(s);
 CHECK(!d.reason.empty()&&d.hasSelection);
 CHECK(d.selected.costTerms.describe().find("risk")!=std::string::npos);
 for(const Evaluation& e:d.considered){
  CHECK(!e.why.empty());
  if(e.feasible())CHECK(e.threats.size()==2&&!e.label.empty());
 }
 CHECK(std::string(toString(Verdict::PredictedClear))=="predicted clear");
 CHECK(std::string(toString(Reject::ReceiverUnreachable))=="receiver cannot reach");
}
}

int main(){
 check::run("launchExactness",launchExactness);
 check::run("unitsInvariance",unitsInvariance);
 check::run("receiverReachability",receiverReachability);
 check::run("zeroVelocityDefender",zeroVelocityDefender);
 check::run("heightMatters",heightMatters);
 check::run("arrivalTimeAndMiss",arrivalTimeAndMiss);
 check::run("interceptionTimePrecision",interceptionTimePrecision);
 check::run("lobOverDefender",lobOverDefender);
 check::run("fastWhenOpen",fastWhenOpen);
 check::run("adjustAwayFromCoverage",adjustAwayFromCoverage);
 check::run("neverSafeWhenUnsupported",neverSafeWhenUnsupported);
 check::run("uncertaintyWidensRisk",uncertaintyWidensRisk);
 check::run("hysteresisPreventsFlapping",hysteresisPreventsFlapping);
 check::run("promptSwitchWhenInvalid",promptSwitchWhenInvalid);
 check::run("boundedWorkAndContinuation",boundedWorkAndContinuation);
 check::run("selectionInvariantsAcrossScenarios",selectionInvariantsAcrossScenarios);
 check::run("predictedClearHoldsInSimulation",predictedClearHoldsInSimulation);
 check::run("diagnostics",diagnostics);
 return check::finish("planner");
}
