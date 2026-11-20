#pragma once
// Reproducible synthetic scenarios. Agents follow scripts under the same speed,
// acceleration, braking and turning limits the planner assumes; observations get
// seeded Gaussian noise; a throw is played out against the scripted "truth" to see
// whether the planner's verdict held.
#include "planner.hpp"
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace scenarios {
using namespace planner;

// splitmix64: identical sequences on every compiler and platform (unlike
// std::normal_distribution, whose output is implementation-defined).
struct Rng {
 std::uint64_t state;
 explicit Rng(std::uint64_t seed):state(seed){}
 std::uint64_t next(){
  std::uint64_t z=(state+=0x9E3779B97F4A7C15ull);
  z=(z^(z>>30))*0xBF58476D1CE4E5B9ull;
  z=(z^(z>>27))*0x94D049BB133111EBull;
  return z^(z>>31);
 }
 double uniform(){return static_cast<double>(next()>>11)*0x1p-53;}
 double normal(){
  const double u=std::fmax(uniform(),1e-300),v=uniform();
  return std::sqrt(-2*std::log(u))*std::cos(2*kPi*v);
 }
};

enum class Behaviour { Hold, Route, Pursue };
struct Script {
 Behaviour behaviour=Behaviour::Hold;
 std::vector<Vec3> waypoints;  // Route
 double speed=0;               // cruising speed
 double startTime=0;           // keeps its initial velocity until then
 Vec3 offset;                  // Pursue: cushion kept from the receiver
 double lead=.3;               // Pursue: aims this far ahead in time
};
struct Actor { Agent agent; Script script; size_t next=0; };

struct Spec {
 std::string name,description;
 World world;
 Vec3 passer;
 Actor receiver;
 bool routeKnown=true;     // whether the planner is told the receiver's waypoints
 std::vector<Actor> defenders;
 double snapshotTime=1;    // when the ball is thrown (s)
 double duration=2.5;      // how long the planner runs (s)
 double positionNoise=0,velocityNoise=0; // observation noise, 1 sigma (units, units/s)
 std::uint64_t seed=1;
};

// Velocity change limited like the planner's model: accel/decel along the heading,
// turnAccel across it, speed capped.
inline Vec3 steer(const Agent& a,Vec3 desired,double dt){
 const MoverParams& m=a.params;
 const Vec3 v=horizontal(a.velocity);
 const Vec3 dv=horizontal(desired)-v;
 const double speed=horizontalLength(v);
 Vec3 next;
 if(speed>1e-9){
  const Vec3 h=v/speed;
  const double along=std::clamp(dot(dv,h),-m.decel*dt,m.accel*dt);
  Vec3 across=dv-h*dot(dv,h);
  const double n=horizontalLength(across),cap=m.turnAccel*dt;
  if(n>cap)across=across*(cap/n);
  // Keep the combined change inside the ellipse the planner checks against.
  const double ap=along>=0?m.accel:m.decel;
  const double use=std::hypot(along/(ap*dt),horizontalLength(across)/cap);
  const double k=use>1?1/use:1;
  next=v+h*(along*k)+across*k;
 }else{
  const double n=horizontalLength(dv),cap=m.accel*dt;
  next=n>cap?v+dv*(cap/n):v+dv;
 }
 const double s=horizontalLength(next);
 return s>m.maxSpeed?next*(m.maxSpeed/s):next;
}

inline void moveToward(Actor& a,Vec3 target,double speed,double dt){
 const Vec3 to=horizontal(target)-horizontal(a.agent.position);
 const double dist=horizontalLength(to);
 const Vec3 desired=dist>1e-9?to*(std::fmin(speed,dist/.25)/dist):Vec3{};
 a.agent.velocity=steer(a.agent,desired,dt);
}

// Moves with the average of the old and new velocity: exact for the constant
// acceleration applied over the step, so agents never exceed the modelled limits.
inline void integrate(Agent& a,Vec3 oldVelocity,double dt){
 a.position=a.position+(oldVelocity+a.velocity)*(.5*dt);
}

inline void stepActor(Actor& a,const Actor& receiver,double t,double dt){
 const Script& s=a.script;
 const Vec3 oldVelocity=a.agent.velocity;
 if(t>=s.startTime){
  switch(s.behaviour){
   case Behaviour::Hold:moveToward(a,a.agent.position,0,dt);break;
   case Behaviour::Route:
    while(a.next<s.waypoints.size()&&horizontalLength(horizontal(s.waypoints[a.next])-horizontal(a.agent.position))<std::fmax(.4,s.speed*dt*2))++a.next;
    if(a.next<s.waypoints.size())moveToward(a,s.waypoints[a.next],s.speed,dt);
    else a.agent.velocity=steer(a.agent,unitOr(a.agent.velocity,{1,0,0})*s.speed,dt);
    break;
   case Behaviour::Pursue:
    moveToward(a,receiver.agent.position+receiver.agent.velocity*s.lead+s.offset,s.speed,dt);
    break;
  }
 }
 integrate(a.agent,oldVelocity,dt);
}

// What the planner sees: the truth plus seeded noise, and the route if known.
inline Scene observe(const Spec& spec,const Actor& receiver,const std::vector<Actor>& defenders,Rng& rng){
 auto noisy=[&](Agent a,double scale){
  a.position=a.position+Vec3{rng.normal(),0,rng.normal()}*(spec.positionNoise*scale);
  a.velocity=a.velocity+Vec3{rng.normal(),0,rng.normal()}*(spec.velocityNoise*scale);
  return a;
 };
 Scene s;
 s.world=spec.world;
 s.passer=spec.passer;
 s.receiver=noisy(receiver.agent,.5);
 if(spec.routeKnown)
  for(size_t i=receiver.next;i<receiver.script.waypoints.size();++i)s.receiverRoute.push_back(receiver.script.waypoints[i]);
 for(const Actor& d:defenders)s.defenders.push_back(noisy(d.agent,1));
 return s;
}

// ---------------------------------------------------------------- throw outcome

struct Outcome {
 std::string result="not thrown"; // caught / defended before catch / intercepted after miss / incomplete
 std::string by;
 double time=0;
 Vec3 point;
 double receiverMiss=0;           // horizontal distance from the catch reach at catch time (<= 0 caught)
};

// Plays the selected throw out against the scripted truth. The receiver replays the
// planned control inputs from its true state (so observation error shows up as
// drift); defenders chase the ball within their true limits after reacting.
inline Outcome simulateThrow(const Spec& spec,const Scene& observed,const Actor& receiverTruth,std::vector<Actor> defenders,const Evaluation& sel,double dt){
 Outcome out;
 const World& w=spec.world;
 const CatchMode mode=catchModes(receiverTruth.agent.params)[sel.candidate.mode];
 const double T=sel.candidate.flightTime;
 const Vec3 dp=horizontal(receiverTruth.agent.position)-horizontal(observed.receiver.position);
 const Vec3 dv=horizontal(receiverTruth.agent.velocity)-horizontal(observed.receiver.velocity);
 auto receiverAt=[&](double t){
  return planPosition(sel.receiverPlan,std::fmin(t,sel.receiverPlan.arrival))+dp+dv*t;
 };
 std::vector<double> times;
 for(double t=0;t<sel.landingTime;t+=dt)times.push_back(t);
 times.push_back(T);
 times.push_back(sel.landingTime);
 std::sort(times.begin(),times.end());
 std::vector<std::vector<std::pair<double,Vec3>>> history(defenders.size());
 bool missed=false;
 double prev=0;
 for(double t:times){
  const double step=t-prev;
  prev=t;
  const Vec3 ball=projectile::advance(sel.launch,w,t).position;
  const double h=ball.y-w.groundY;
  for(size_t i=0;i<defenders.size();++i){
   Actor& d=defenders[i];
   const MoverParams& m=d.agent.params;
   if(step>0){
    const Vec3 oldVelocity=d.agent.velocity;
    // Steering applies over [t-step, t], so it may only start once that interval
    // begins after the reaction time.
    if(t-step>=m.reactionTime){
     // Run to the future ball position with the most time to spare.
     double bestSpare=-kInf;
     Vec3 target=ball;
     for(double s=t;s<=sel.landingTime;s+=.05){
      const Vec3 b=projectile::advance(sel.launch,w,s).position;
      if(b.y-w.groundY>m.jumpReachHeight)continue;
      const double spare=(s-t)-horizontalLength(horizontal(b)-horizontal(d.agent.position))/m.maxSpeed;
      if(spare>bestSpare){bestSpare=spare;target=b;}
     }
     moveToward(d,target,m.maxSpeed,step);
    }
    integrate(d.agent,oldVelocity,step);
   }
   history[i].push_back({t,d.agent.position});
   const double dist=horizontalLength(horizontal(ball)-horizontal(d.agent.position));
   bool plays=false;
   if(h>=0&&h<=m.reachHeight&&dist<=m.armReach)plays=true;
   if(h>m.reachHeight&&h<=m.jumpReachHeight&&t>=m.jumpPrepTime){
    // A jump is played from where the defender was jumpPrepTime earlier.
    Vec3 takeoff=history[i].front().second;
    for(const auto& p:history[i])if(p.first<=t-m.jumpPrepTime+1e-12)takeoff=p.second;
    if(horizontalLength(horizontal(ball)-horizontal(takeoff))<=m.armReach)plays=true;
   }
   if(h>=0&&h<=m.diveMaxHeight&&dist<=m.armReach+m.diveReach)plays=true;
   if(plays){
    out.result=t<=T?"defended before catch":"intercepted after miss";
    out.by=d.agent.name;out.time=t;out.point=ball;
    return out;
   }
  }
  if(t==T&&!missed){
   out.receiverMiss=horizontalLength(horizontal(ball)-receiverAt(T))-mode.reach;
   if(out.receiverMiss<=0){out.result="caught";out.by=receiverTruth.agent.name;out.time=T;out.point=ball;return out;}
   missed=true;
  }
 }
 out.result="incomplete";
 out.time=sel.landingTime;
 out.point=sel.landingPoint;
 return out;
}

// ---------------------------------------------------------------- planner run

struct FrameLog {
 double time=0;
 bool has=false,switched=false,blending=false;
 Candidate candidate;
 Verdict verdict=Verdict::Invalid;
 Vec3 destination;
 double cost=0,ms=0;
 long work=0;
 int switches=0;
};
struct RunResult {
 std::vector<FrameLog> frames;
 Scene throwScene;
 Decision throwDecision;
 Outcome outcome;
 Actor receiverAtThrow;
 std::vector<Actor> defendersAtThrow;
 double maxMs=0,meanMs=0;
};

// Runs the planner every frame from t=0 to spec.duration on noisy observations and
// throws the selected solution at spec.snapshotTime.
inline RunResult run(const Spec& spec,const PlannerConfig& cfg,double hz=60){
 RunResult r;
 Rng rng(spec.seed);
 Actor receiver=spec.receiver;
 std::vector<Actor> defenders=spec.defenders;
 Planner planner(cfg);
 const double dt=1/hz;
 const int frames=static_cast<int>(std::lround(spec.duration*hz));
 bool thrown=false;
 double totalMs=0;
 for(int i=0;i<=frames;++i){
  const double t=i*dt;
  const Scene scene=observe(spec,receiver,defenders,rng);
  const auto start=std::chrono::steady_clock::now();
  const Decision& d=planner.update(scene);
  const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
  totalMs+=ms;
  r.maxMs=std::fmax(r.maxMs,ms);
  FrameLog f;
  f.time=t;f.has=d.hasSelection;f.switched=d.switched;f.blending=d.blending;f.ms=ms;f.work=d.work;f.switches=d.switches;
  if(d.hasSelection){f.candidate=d.selected.candidate;f.verdict=d.selected.verdict;f.destination=d.selected.destination;f.cost=d.selected.cost;}
  r.frames.push_back(f);
  if(!thrown&&t+1e-9>=spec.snapshotTime){
   thrown=true;
   r.throwScene=scene;
   r.throwDecision=d;
   r.receiverAtThrow=receiver;
   r.defendersAtThrow=defenders;
   if(d.hasSelection)r.outcome=simulateThrow(spec,scene,receiver,defenders,d.selected,dt);
  }
  stepActor(receiver,receiver,t,dt);
  for(Actor& a:defenders)stepActor(a,receiver,t,dt);
 }
 r.meanMs=totalMs/static_cast<double>(r.frames.size());
 return r;
}

// ---------------------------------------------------------------- library

// Scenario coordinates: passer at the origin, x downfield, z across the field.
// All numbers are metres and converted to the requested World units.
inline std::vector<Spec> standardScenarios(const World& w=World::metres()){
 const auto L=[&](double metres){return w.fromMetres(metres);};
 const auto P=[&](double x,double z){return Vec3{L(x),w.groundY,L(z)};};
 const MoverParams rp=MoverParams::receiver().scaledTo(w),dp=MoverParams::defender().scaledTo(w);
 const Uncertainty ru=Uncertainty::receiver().scaledTo(w),du=Uncertainty::defender().scaledTo(w);
 auto actor=[&](std::string name,Vec3 pos,Vec3 vel,bool isReceiver,Script s){
  for(Vec3& p:s.waypoints)p=P(p.x,p.z);
  s.speed=L(s.speed);
  s.offset=Vec3{L(s.offset.x),0,L(s.offset.z)};
  return Actor{Agent{std::move(name),P(pos.x,pos.z),Vec3{L(vel.x),0,L(vel.z)},isReceiver?rp:dp,isReceiver?ru:du},s,0};
 };
 auto route=[](std::vector<Vec3> pts,double speed,double start=0){Script s;s.behaviour=Behaviour::Route;s.waypoints=std::move(pts);s.speed=speed;s.startTime=start;return s;};
 auto pursue=[](double speed,Vec3 offset,double lead=.3,double start=0){Script s;s.behaviour=Behaviour::Pursue;s.speed=speed;s.offset=offset;s.lead=lead;s.startTime=start;return s;};
 const Script hold;
 auto base=[&](std::string name,std::string description){
  Spec s;s.name=std::move(name);s.description=std::move(description);s.world=w;s.passer=P(0,0);
  s.positionNoise=L(.05);s.velocityNoise=L(.15);
  return s;
 };
 std::vector<Spec> out;
 {
  Spec s=base("stationary-receiver","Receiver standing still; a zone defender sits nearby with zero velocity, so only the movement model says it can close.");
  s.receiver=actor("WR",{12,0,6},{0,0,0},true,hold);
  s.defenders.push_back(actor("CB",{15,0,8.5},{0,0,0},false,hold));
  s.seed=11;s.snapshotTime=.5;s.duration=1.5;
  out.push_back(s);
 }
 {
  Spec s=base("moving-go-route","Receiver running a go route with a trailing corner and a deep safety.");
  s.receiver=actor("WR",{8,0,-12},{8.5,0,0},true,route({{60,0,-12}},8.5));
  s.defenders.push_back(actor("CB",{6.5,0,-11},{8,0,0},false,pursue(8.3,{-1.2,0,.8})));
  s.defenders.push_back(actor("S",{30,0,-4},{0,0,0},false,route({{36,0,-9}},5,.3)));
  s.seed=12;s.snapshotTime=1.0;
  out.push_back(s);
 }
 {
  Spec s=base("crossing-route","Receiver crossing the field; a linebacker drops underneath and a defender crosses the other way.");
  s.receiver=actor("WR",{9,0,-14},{0,0,8},true,route({{9,0,18}},8));
  s.defenders.push_back(actor("LB",{6,0,0},{0,0,0},false,route({{9.5,0,1}},3)));
  s.defenders.push_back(actor("DB",{14,0,12},{0,0,-6},false,route({{14,0,-14}},6)));
  s.seed=13;s.snapshotTime=1.1;
  out.push_back(s);
 }
 {
  Spec s=base("close-throw","Quick slant at short range with a nickel defender in tight trail.");
  s.receiver=actor("WR",{4,0,-6},{3,0,4},true,route({{9,0,-1},{14,0,3}},7));
  s.defenders.push_back(actor("NB",{5.5,0,-5},{2,0,3},false,pursue(7,{-.8,0,-.6},.2)));
  s.seed=14;s.snapshotTime=.4;s.duration=1.6;
  out.push_back(s);
 }
 {
  Spec s=base("distant-throw","Deep throw to a receiver 30+ m downfield with a trailing corner and a safety rotating over the top.");
  s.receiver=actor("WR",{26,0,10},{9,0,0},true,route({{70,0,10}},9));
  s.defenders.push_back(actor("CB",{24.5,0,11.2},{8.5,0,0},false,pursue(8.5,{-1.4,0,.8})));
  s.defenders.push_back(actor("S",{42,0,0},{0,0,0},false,route({{50,0,8}},6,.2)));
  s.seed=15;s.snapshotTime=.6;s.duration=2.0;
  out.push_back(s);
 }
 {
  Spec s=base("multiple-defenders","Dig route into zone coverage: two linebackers, a trailing corner and a safety.");
  s.receiver=actor("WR",{10,0,-9},{7,0,0},true,route({{14,0,-9},{14,0,10}},7.5));
  s.defenders.push_back(actor("LB1",{6,0,-3},{0,0,0},false,route({{9.5,0,-4}},3)));
  s.defenders.push_back(actor("LB2",{6,0,4},{0,0,0},false,route({{9.5,0,5}},3)));
  s.defenders.push_back(actor("CB",{9,0,-10.5},{6,0,0},false,pursue(7.5,{-1,0,-1})));
  s.defenders.push_back(actor("S",{24,0,0},{0,0,0},false,route({{19,0,-2}},4,.4)));
  s.seed=16;s.snapshotTime=1.2;s.duration=2.6;
  out.push_back(s);
 }
 {
  Spec s=base("abrupt-cut","Receiver plants and comes back at ~1.1 s; the route is NOT known to the planner, which must notice from the velocity.");
  s.receiver=actor("WR",{6,0,-10},{8,0,0},true,route({{16,0,-10},{12,0,-13.5}},8));
  s.routeKnown=false;
  s.defenders.push_back(actor("CB",{5,0,-9},{7.5,0,0},false,pursue(8,{-1.5,0,.8},.4)));
  s.seed=17;s.snapshotTime=1.6;s.duration=2.4;
  out.push_back(s);
 }
 {
  Spec s=base("overlapping-reach","Receiver bracketed: a corner in tight coverage, a linebacker underneath and a safety over the top. Reach regions overlap the receiver's.");
  s.receiver=actor("WR",{12,0,0},{6,0,0},true,route({{20,0,0}},6));
  s.defenders.push_back(actor("CB",{12.6,0,.7},{6,0,0},false,pursue(6.5,{.3,0,.6})));
  s.defenders.push_back(actor("LB",{8.5,0,-.5},{0,0,0},false,hold));
  s.defenders.push_back(actor("S",{19,0,1},{0,0,0},false,hold));
  s.seed=18;s.snapshotTime=.8;s.duration=2.0;
  out.push_back(s);
 }
 return out;
}

}
