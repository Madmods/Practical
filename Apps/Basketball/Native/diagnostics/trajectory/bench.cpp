// Benchmarks: numerical accuracy, worst-case update time, prediction sensitivity and
// frame-to-frame stability (with and without hysteresis). Deterministic inputs;
// timings naturally vary by machine.
#include "scenarios.hpp"
#include <algorithm>
#include <chrono>
#include <iostream>

using namespace planner;
using scenarios::Rng;

namespace {
double ms(std::chrono::steady_clock::time_point a){
 return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-a).count();
}
double percentile(std::vector<double> v,double p){
 if(v.empty())return 0;
 std::sort(v.begin(),v.end());
 return v[std::min(v.size()-1,static_cast<size_t>(p*static_cast<double>(v.size()-1)+.5))];
}
Agent randomAgent(Rng& rng,bool defender){
 Agent a;
 a.name="A";
 a.params=defender?MoverParams::defender():MoverParams::receiver();
 a.uncertainty=defender?Uncertainty::defender():Uncertainty::receiver();
 a.position={rng.uniform()*40-20,0,rng.uniform()*40-20};
 const double speed=rng.uniform()*a.params.maxSpeed,angle=rng.uniform()*2*kPi;
 a.velocity={speed*std::cos(angle),0,speed*std::sin(angle)};
 return a;
}

void accuracy(){
 std::cout<<"== Accuracy\n";
 Rng rng(101);
 const World w=World::metres();
 // Contact: residual at the computed time, in units of rounding at the problem scale.
 double worstContact=0;
 for(int i=0;i<200000;++i){
  const double h=std::pow(10.,rng.uniform()*12-6),v=(rng.uniform()*2-1)*std::pow(10.,rng.uniform()*8-4);
  const projectile::State s{{0,h,0},{1,v,0}};
  const auto c=projectile::groundContact(s,w);
  const double residual=std::fabs(projectile::advance(s,w,c.time).position.y);
  worstContact=std::fmax(worstContact,residual/projectile::roundingTolerance(projectile::positionScale(s,w.gravityVector(),c.time),1));
 }
 std::cout<<format("  ground contact residual, 200k random states (1e-6..1e6 heights): max %.1f ulps of the term scale\n",worstContact);

 // Launch solutions: the arc passes exactly through the destination.
 double worstLaunch=0;
 long launches=0;
 for(const auto& spec:scenarios::standardScenarios(w)){
  const auto r=scenarios::run(spec,PlannerConfig::defaults(w));
  for(const Evaluation& e:Planner(PlannerConfig::defaults(w)).evaluateGrid(r.throwScene)){
   if(e.reject==Reject::BadInput||e.reject==Reject::FlightTime)continue;
   if(!projectile::finite(e.launch.velocity))continue;
   const Vec3 end=projectile::advance(e.launch,w,e.candidate.flightTime).position;
   const double scale=projectile::positionScale(e.launch,w.gravityVector(),e.candidate.flightTime)+projectile::maxAbs(e.destination);
   worstLaunch=std::fmax(worstLaunch,projectile::maxAbs(end-e.destination)/projectile::roundingTolerance(scale,1));
   ++launches;
  }
 }
 std::cout<<format("  launch solutions: %.0f arcs, endpoint error max %.1f ulps of the term scale\n",static_cast<double>(launches),worstLaunch);

 // Receiver plans: replay each feasible plan and check every limit.
 double worstArrive=0,worstSpeed=0,worstAccel=0;
 int feasible=0;
 for(int i=0;i<20000;++i){
  const Agent a=randomAgent(rng,false);
  const double t=.2+rng.uniform()*2.5;
  const Vec3 target=a.position+a.velocity*t+Vec3{rng.normal(),0,rng.normal()}*(3+3*t);
  const ReachPlan p=innerReach(a,target,t);
  if(!p.feasible)continue;
  ++feasible;
  worstArrive=std::fmax(worstArrive,horizontalLength(planPosition(p,t)-horizontal(target)));
  const double cap=std::fmax(a.params.maxSpeed,horizontalLength(a.velocity));
  worstSpeed=std::fmax(worstSpeed,horizontalLength(p.cruiseVelocity)/cap);
  const double sp=horizontalLength(a.velocity);
  double use=horizontalLength(p.acceleration)/a.params.accel;
  if(sp>1e-6*a.params.maxSpeed){
   const Vec3 h=horizontal(a.velocity)/sp;
   const double along=dot(p.acceleration,h);
   use=std::hypot(along/(along>=0?a.params.accel:a.params.decel),horizontalLength(p.acceleration-h*along)/a.params.turnAccel);
  }
  worstAccel=std::fmax(worstAccel,use);
 }
 std::cout<<format("  receiver plans: %.0f feasible of 20000; arrival error max %.2e units, speed use max %.6f, accel use max %.6f (must be <= 1)\n",feasible,worstArrive,worstSpeed,worstAccel);

 // Defender bound: random limit-respecting manoeuvres never leave it.
 long checks=0,violations=0;
 double closest=kInf;
 for(int i=0;i<3000;++i){
  Agent a=randomAgent(rng,true);
  const Agent start=a;
  const double dt=1./240;
  Vec3 desired=a.velocity;
  for(int k=1;k<=int(2.5/dt);++k){
   const double t=k*dt;
   if(k%24==0){const double s=rng.uniform()*a.params.maxSpeed,an=rng.uniform()*2*kPi;desired={s*std::cos(an),0,s*std::sin(an)};}
   const Vec3 old=a.velocity;
   if(t-dt>=a.params.reactionTime)a.velocity=scenarios::steer(a,desired,dt);
   scenarios::integrate(a,old,dt);
   const double gap=outerReachGap(start,a.position,t);
   closest=std::fmin(closest,-gap);
   ++checks;
   if(gap>1e-9)++violations;
  }
 }
 std::cout<<format("  defender outer bound: %.0f positions from 3000 random limited manoeuvres, %.0f outside the bound (must be 0); tightest inside margin %.2e\n",
                   static_cast<double>(checks),static_cast<double>(violations),closest);

 // Interception time against a closed form: a defender who cannot move, ball along z=0.
 World flat=w;
 Agent post;post.name="post";post.position={10,0,.3};
 post.params.maxSpeed=1e-12;post.params.accel=post.params.decel=post.params.turnAccel=1e-12;
 post.params.reactionTime=0;post.params.diveReach=0;post.params.diveMaxHeight=.1;post.params.armReach=.6;
 post.uncertainty={0,0,0};
 const projectile::State ball{{0,1.5,0},{20,3,0}};
 long work=0;
 const Threat th=sweepThreat(flat,post,ball,0,1,1./60,Phase::BeforeCatch,work);
 const double exact=(10-std::sqrt(.6*.6-.3*.3))/20;
 std::cout<<format("  first-interception time vs closed form: error %.2e s\n",std::fabs(th.interceptTime-exact));
}

struct StabilityStats { double switchesPerSecond=0,oscillations=0,meanJump=0,maxJump=0,verdictFlips=0,largeJumps=0; };
StabilityStats stability(const scenarios::RunResult& r){
 StabilityStats s;
 std::vector<Candidate> recent;
 double jumps=0;int n=0;
 for(size_t i=1;i<r.frames.size();++i){
  const auto& a=r.frames[i-1];const auto& b=r.frames[i];
  if(!(a.has&&b.has))continue;
  const double j=horizontalLength(b.destination-a.destination);
  jumps+=j;++n;s.maxJump=std::fmax(s.maxJump,j);
  if(j>1)++s.largeJumps;  // destination moved more than 1 unit in one frame
  if(a.verdict!=b.verdict)++s.verdictFlips;
  if(!sameCandidate(a.candidate,b.candidate)){
   // A->B->A within half a second: switching back to something just abandoned.
   for(const Candidate& c:recent)if(sameCandidate(c,b.candidate)){++s.oscillations;break;}
   recent.push_back(a.candidate);
   if(recent.size()>30)recent.erase(recent.begin());
  }
 }
 s.meanJump=n?jumps/n:0;
 const double seconds=r.frames.back().time-r.frames.front().time;
 s.switchesPerSecond=r.frames.back().switches/seconds;
 return s;
}

void timingAndStability(){
 std::cout<<"\n== Computation per update (60 Hz, default budget) and stability\n";
 const World w=World::metres();
 const PlannerConfig cfg=PlannerConfig::defaults(w);
 PlannerConfig noHysteresis=cfg;
 noHysteresis.score.switchMargin=0;noHysteresis.score.continuityWeight=0;noHysteresis.score.blend=false;noHysteresis.score.costSmoothing=1;noHysteresis.score.noiseSigmas=0;
 std::vector<double> all;
 long maxWork=0,maxSingle=0;
 StabilityStats sum,sumOff;
 int scenarioCount=0;
 std::cout<<"  scenario             mean ms  p99 ms  max ms  full-grid ms | switch/s osc >1u  jump mean/max | no hysteresis: switch/s osc >1u jump mean/max\n";
 for(const auto& spec:scenarios::standardScenarios(w)){
  const auto r=scenarios::run(spec,cfg);
  const auto off=scenarios::run(spec,noHysteresis);
  std::vector<double> times;
  for(const auto& f:r.frames){times.push_back(f.ms);all.push_back(f.ms);maxWork=std::max(maxWork,f.work);}
  const auto start=std::chrono::steady_clock::now();
  const auto grid=Planner(cfg).evaluateGrid(r.throwScene);
  const double gridMs=ms(start);
  const Planner fresh(cfg);
  for(const Candidate& c:fresh.grid()){long used=0;evaluate(r.throwScene,cfg,c,&used);maxSingle=std::max(maxSingle,used);}
  const StabilityStats s=stability(r),o=stability(off);
  sum.switchesPerSecond+=s.switchesPerSecond;sum.oscillations+=s.oscillations;sum.largeJumps+=s.largeJumps;sum.meanJump+=s.meanJump;sum.maxJump=std::fmax(sum.maxJump,s.maxJump);
  sumOff.switchesPerSecond+=o.switchesPerSecond;sumOff.oscillations+=o.oscillations;sumOff.largeJumps+=o.largeJumps;sumOff.meanJump+=o.meanJump;sumOff.maxJump=std::fmax(sumOff.maxJump,o.maxJump);
  ++scenarioCount;
  std::string name=spec.name;name.resize(20,' ');
  std::cout<<"  "<<name<<format(" %7.2f %7.2f %7.2f %10.1f    | %6.1f %4.0f %4.0f  %5.2f/%5.2f",r.meanMs,percentile(times,.99),r.maxMs,gridMs,s.switchesPerSecond,s.oscillations,s.largeJumps,s.meanJump,s.maxJump)
           <<format("    | %6.1f %4.0f %4.0f  %5.2f/%5.2f\n",o.switchesPerSecond,o.oscillations,o.largeJumps,o.meanJump,o.maxJump);
 }
 std::cout<<format("  all updates: p50 %.2f ms, p99 %.2f ms, max %.2f ms; max work %.0f (budget %.0f + one evaluation up to %.0f)\n",
                   percentile(all,.5),percentile(all,.99),percentile(all,1),static_cast<double>(maxWork),static_cast<double>(cfg.search.workBudget),static_cast<double>(maxSingle));
 const double k=1./scenarioCount;
 std::cout<<format("  average per scenario: with hysteresis %.1f switches/s, %.1f oscillations, %.1f jumps >1 unit, mean jump %.2f\n"
                   "                        without         %.1f switches/s, %.1f oscillations, %.1f jumps >1 unit, mean jump %.2f\n",
                   sum.switchesPerSecond*k,sum.oscillations*k,sum.largeJumps*k,sum.meanJump*k,sumOff.switchesPerSecond*k,sumOff.oscillations*k,sumOff.largeJumps*k,sumOff.meanJump*k);
}

void sensitivity(){
 std::cout<<"\n== Prediction sensitivity (selected throw re-evaluated under 400 perturbed defender states drawn from the planner's own uncertainty)\n";
 std::cout<<"  scenario             verdict at throw   verdict kept  contested frac  model P(before catch)  mean |dRisk|\n";
 const World w=World::metres();
 const PlannerConfig cfg=PlannerConfig::defaults(w);
 for(const auto& spec:scenarios::standardScenarios(w)){
  const auto r=scenarios::run(spec,cfg);
  if(!r.throwDecision.hasSelection)continue;
  const Evaluation& sel=r.throwDecision.selected;
  Rng rng(spec.seed*7919);
  int kept=0,contested=0;
  double dRisk=0;
  const int trials=400;
  for(int i=0;i<trials;++i){
   Scene s=r.throwScene;
   for(Agent& d:s.defenders){
    // Perturb by the planner's own sigma model: position error now and velocity
    // error (the reaction-time term is folded into velocity here).
    d.position=d.position+Vec3{rng.normal(),0,rng.normal()}*d.uncertainty.position;
    d.velocity=d.velocity+Vec3{rng.normal(),0,rng.normal()}*std::hypot(d.uncertainty.velocity,d.params.maxSpeed*d.uncertainty.reactionTime/std::fmax(sel.candidate.flightTime,.1));
   }
   const Evaluation e=evaluate(s,cfg,sel.candidate);
   if(e.verdict==sel.verdict)++kept;
   if(e.verdict==Verdict::Contested)++contested;
   dRisk+=std::fabs(e.preCatchRisk-sel.preCatchRisk);
  }
  std::string name=spec.name;name.resize(20,' ');
  std::string verdict=toString(sel.verdict);verdict.resize(16,' ');
  std::cout<<"  "<<name<<" "<<verdict<<format("   %10.3f  %14.3f  %21.3f  %12.4f\n",double(kept)/trials,double(contested)/trials,sel.preCatchRisk,dRisk/trials);
 }
 std::cout<<"  (contested frac vs model P compares the Gaussian risk estimate with sampling the same error model)\n";
}
}

int main(){
 accuracy();
 timingAndStability();
 sensitivity();
}
