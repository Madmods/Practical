#pragma once
// Pass trajectory planner built on projectile.hpp.
//
// A candidate is (flight time, depth offset, lateral offset, catch mode) relative to
// where the receiver is predicted to be. From it the destination is fixed, and the
// launch velocity that reaches it in exactly that flight time follows in closed form,
// so speed, elevation, flight time and destination are searched jointly rather than
// by adjusting one angle at a time. Each candidate is then checked for:
//   - launch limits (speed, elevation, flight time),
//   - receiver reachability with an explicit, verifiable movement plan (inner bound),
//   - every defender over the whole flight, including after a missed catch, using a
//     conservative reach bound (outer bound) that depends on ball height and time,
//   - prediction uncertainty, reported as a margin in standard deviations.
// The Planner spends a bounded amount of work per update, keeps searching where it
// left off, and only changes its selection when a challenger is clearly better or the
// current choice has become invalid.
#include "projectile.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

namespace planner {
using projectile::State;
using projectile::Vec3;
using projectile::World;

constexpr double kPi=3.14159265358979323846;
constexpr double kInf=std::numeric_limits<double>::infinity();
inline double normalCdf(double x){return .5*std::erfc(-x/std::sqrt(2.));}
inline Vec3 unitOr(Vec3 v,Vec3 fallback){
 const double n=length(v);
 return n>0&&std::isfinite(n)?v/n:fallback;
}
// Lateral direction for a horizontal heading: (1,0,0) maps to (0,0,1). "Right" in
// labels means this direction.
inline Vec3 rightOf(Vec3 forward){return {-forward.z,0,forward.x};}
template<class... Numbers>
std::string format(const char* fmt,Numbers... values){
 char buffer[512];
 std::snprintf(buffer,sizeof buffer,fmt,static_cast<double>(values)...);
 return buffer;
}

// Movement and reach limits. Defaults are in metres and seconds and are placeholders
// to calibrate (see README); scaledTo() converts the lengths to World units.
struct MoverParams {
 double maxSpeed=9.0;         // m/s
 double accel=6.5;            // m/s^2 speeding up along the current heading
 double decel=8.5;            // m/s^2 braking along the current heading
 double turnAccel=7.5;        // m/s^2 across the current heading (turning)
 double reactionTime=.25;     // s before the agent reacts to the throw
 double chestHeight=1.3;      // m, natural catch height
 double reachHeight=2.3;      // m, highest ball reached without jumping
 double jumpReachHeight=3.0;  // m, highest ball reached with a jump
 double jumpPrepTime=.25;     // s to plant and leave the ground
 double armReach=.6;          // m, horizontal reach from the body centre
 double diveReach=1.2;        // m, extra horizontal reach in a dive
 double diveMaxHeight=1.2;    // m, a dive only reaches balls below this
 double missChest=.15,missHigh=.3,missDive=.45; // chance a reachable ball is not secured, by catch

 MoverParams scaledTo(const World& w) const {
  MoverParams m=*this;
  for(double* v:{&m.maxSpeed,&m.accel,&m.decel,&m.turnAccel,&m.chestHeight,&m.reachHeight,
                 &m.jumpReachHeight,&m.armReach,&m.diveReach,&m.diveMaxHeight})*v=w.fromMetres(*v);
  return m;
 }
 bool valid() const {
  for(double v:{maxSpeed,accel,decel,turnAccel,chestHeight,reachHeight,jumpReachHeight,armReach,diveMaxHeight})
   if(!std::isfinite(v)||v<=0)return false;
  for(double v:{reactionTime,jumpPrepTime,diveReach})if(!std::isfinite(v)||v<0)return false;
  return chestHeight<=reachHeight&&reachHeight<=jumpReachHeight;
 }
 static MoverParams receiver(){return {};}
 static MoverParams defender(){
  MoverParams m;
  m.maxSpeed=8.6;m.accel=6.5;m.decel=8.5;m.turnAccel=7.5;m.reactionTime=.3;
  return m;
 }
};

// One-standard-deviation errors in the movement prediction. They are combined into
// an isotropic Gaussian position error that grows with time (see sigmaAt).
struct Uncertainty {
 // The defender reach bound already allows any manoeuvre after the reaction time;
 // these cover measurement error and movement that starts before it (anticipation).
 double position=.25;     // m, current position error
 double velocity=.6;      // m/s, current velocity error: a "stationary" player may be moving
 double reactionTime=.05; // s, reaction-time error, converted at the agent's top speed
 Uncertainty scaledTo(const World& w) const {return {w.fromMetres(position),w.fromMetres(velocity),reactionTime};}
 double sigmaAt(double t,double maxSpeed) const {
  const double s=std::hypot(position,velocity*t,maxSpeed*reactionTime);
  return std::fmax(s,1e-12);
 }
 bool valid() const {
  return std::isfinite(position)&&position>=0&&std::isfinite(velocity)&&velocity>=0&&std::isfinite(reactionTime)&&reactionTime>=0;
 }
 // A teammate running a known route is better predicted than an opponent.
 static Uncertainty receiver(){return {.15,.4,.04};}
 static Uncertainty defender(){return {};}
};

struct Agent {
 std::string name;
 Vec3 position,velocity;   // position.y is ignored: movement is on the ground plane
 MoverParams params;
 Uncertainty uncertainty;
};

struct Scene {
 World world;
 Vec3 destinationBias{};
 Vec3 passer;                     // passer's feet
 Agent receiver;
 std::vector<Vec3> receiverRoute; // known upcoming waypoints, optional
 std::vector<Agent> defenders;
};

// ---------------------------------------------------------------- configuration

struct LaunchConfig {
 double releaseAlongLaunch=0; // optional offset along the full 3D launch direction
 double releaseHeight=1.9;   // m above the passer's feet
 double releaseForward=.3;   // m along the aim direction
 double releaseSide=.25;     // m to the right of the aim direction (throwing arm)
 double minSpeed=6, maxSpeed=27;               // m/s
 double minElevationDeg=-15, maxElevationDeg=60;
 double minFlightTime=.25, maxFlightTime=3.4;  // s
};

struct SearchConfig {
 bool pruneDistantThreats=false;
 bool offsetMajor=false; // explore all flight times before the next placement
 double timeStep=.1;                                       // s between grid flight times
 std::vector<double> depthOffsets{-4,-2.5,-1.25,0,1.25,2.5,4}; // m, + away from passer, - comeback
 std::vector<double> lateralOffsets{-3,-1.5,0,1.5,3};          // m, + right of the passer->receiver line
 double refineOffset=.6;     // m, local refinement step around the best candidate
 double sweepStep=1./60;     // s between interception checks along the flight
 int poolSize=8;             // best candidates re-validated every update
 int refineCount=6;          // refinement candidates per update
 long workBudget=40000;      // reach evaluations per update (bounds computation, deterministic)
 double timeBudgetMs=0;      // optional wall-clock cap on the grid pass per update; 0 = off
};

struct ScoreConfig {
 bool preserveBlendCost=false;
 double clearSigmas=2;       // "predicted clear" needs every defender gap >= clearSigmas * sigma
 double receiverSigmas=1;    // ...and the catch to survive this much receiver prediction error
 double maxEffortClear=.9;   // ...and the receiver plan to use at most this fraction of a limit
 double uncertainCost=1.5, contestedCost=4;
 double riskWeight=3;        // per unit interception probability
 double timeWeight=.4;       // per second of flight: prefers fast, flat throws
 double adjustWeight=.04;    // per metre the destination moves off the predicted route
 double effortWeight=.3;     // per fraction of a movement limit the receiver must use
 double marginWeight=.1;     // reward per sigma of receiver robustness (max 2)
 double switchMargin=.06;    // a challenger must be this much cheaper to replace the selection
 double continuityWeight=.02;// cost per grid step away from the current selection
 double costSmoothing=.35;   // weight of the newest cost in remembered candidates' ranking (1 = none)
 double noiseSigmas=2;       // ...and switching also needs this many std devs of measured cost noise
 bool blend=true;            // move toward a new choice in validated steps
 double blendTimeStep=.1, blendOffsetStep=.75;
};

struct Evaluation;
struct PlannerConfig {
 bool requireAfterMissClear=false;
 std::function<bool(Vec3,double)> destinationAllowed;
 std::function<void(Evaluation&)> adjustEvaluation;
 LaunchConfig launch;
 SearchConfig search;
 ScoreConfig score;
 // Converts the metre-based lengths above to World units. Costs per metre become
 // costs per unit, so a scene described in yards ranks candidates the same way.
 PlannerConfig scaledTo(const World& w) const {
  PlannerConfig c=*this;
  LaunchConfig& l=c.launch;
  for(double* v:{&l.releaseHeight,&l.releaseForward,&l.releaseSide,&l.releaseAlongLaunch,&l.minSpeed,&l.maxSpeed})*v=w.fromMetres(*v);
  for(double& v:c.search.depthOffsets)v=w.fromMetres(v);
  for(double& v:c.search.lateralOffsets)v=w.fromMetres(v);
  c.search.refineOffset=w.fromMetres(c.search.refineOffset);
  c.score.adjustWeight*=w.metresPerUnit;
  c.score.blendOffsetStep=w.fromMetres(c.score.blendOffsetStep);
  return c;
 }
 static PlannerConfig defaults(const World& w){return PlannerConfig{}.scaledTo(w);}
};

// ---------------------------------------------------------------- prediction

// Nominal receiver position after t seconds: along the known route at the current
// speed, or at the current velocity when there is no route.
inline Vec3 predictReceiver(const Scene& s,double t){
 Vec3 p=horizontal(s.receiver.position);
 const Vec3 v=horizontal(s.receiver.velocity);
 const double speed=horizontalLength(v);
 if(s.receiverRoute.empty()||speed<=0)return p+v*t;
 double remaining=speed*t;
 Vec3 heading=unitOr(v,{1,0,0});
 for(Vec3 waypoint:s.receiverRoute){
  const Vec3 leg=horizontal(waypoint)-p;
  const double len=horizontalLength(leg);
  if(len<=0)continue;
  heading=leg/len;
  if(len>=remaining)return p+heading*remaining;
  p=horizontal(waypoint);
  remaining-=len;
 }
 return p+heading*remaining;
}

// Necessary conditions on where an agent's body can be at time t, used for
// defenders (an outer bound, so it errs toward assuming they can get there). The
// agent coasts at its current velocity until it reacts, then may accelerate in any
// direction at its largest limit, with speed capped at max(maxSpeed, current speed).
// Zero velocity therefore does not mean stationary. Returns the horizontal distance
// outside the bound; <= 0 means the target may be reachable.
inline double outerReachGap(const Agent& a,Vec3 target,double t){
 if(!(t>=0))return kInf;
 const MoverParams& m=a.params;
 const double react=std::fmin(t,m.reactionTime);
 const double tau=t-react;
 const double amax=std::fmax(m.accel,std::fmax(m.decel,m.turnAccel));
 const Vec3 v=horizontal(a.velocity);
 const double vcap=std::fmax(m.maxSpeed,horizontalLength(v));
 const Vec3 p=horizontal(a.position),q=horizontal(target);
 const double accelGap=horizontalLength(q-(p+v*t))-amax*(.5*tau)*tau;
 const double speedGap=horizontalLength(q-(p+v*react))-vcap*tau;
 return std::fmax(accelGap,speedGap);
}

// An explicit receiver movement plan: coast through the reaction time, hold a
// constant acceleration for accelTime, then hold cruiseVelocity until arrival.
struct ReachPlan {
 bool feasible=false;
 double effort=kInf;      // largest fraction of any limit the plan uses (<= 1 if feasible)
 double reactionTime=0,accelTime=0,arrival=0;
 Vec3 start,initialVelocity,acceleration,cruiseVelocity,body;
};

// Horizontal position the plan puts the body at, time t after the throw.
inline Vec3 planPosition(const ReachPlan& r,double t){
 const double coast=std::fmin(t,r.reactionTime);
 Vec3 p=r.start+r.initialVelocity*coast;
 if(t<=r.reactionTime)return p;
 const double a=std::fmin(t-r.reactionTime,r.accelTime);
 p=p+r.initialVelocity*a+r.acceleration*(.5*a)*a;
 const double cruise=t-r.reactionTime-r.accelTime;
 return cruise>0?p+r.cruiseVelocity*cruise:p;
}

// Sufficient condition for the receiver (an inner bound, so it never claims a
// destination the receiver cannot reach under the movement model). Tries a family
// of explicit plans and checks each against the speed cap and an acceleration
// ellipse aligned with the current heading: accel/decel along it, turnAccel across
// it. The ellipse stays aligned with the initial heading during the manoeuvre.
inline ReachPlan innerReach(const Agent& a,Vec3 body,double t,int steps=24){
 ReachPlan best;
 if(!(t>=0))return best;
 const MoverParams& m=a.params;
 const Vec3 p=horizontal(a.position),v=horizontal(a.velocity),q=horizontal(body);
 best.start=p;best.initialVelocity=v;best.body=q;best.arrival=t;best.reactionTime=m.reactionTime;
 if(t<=m.reactionTime){
  const Vec3 end=p+v*t;
  const double tolerance=projectile::roundingTolerance(projectile::maxAbs(p)+projectile::maxAbs(v)*t+projectile::maxAbs(q));
  if(horizontalLength(q-end)<=tolerance){best.feasible=true;best.effort=0;best.reactionTime=t;best.cruiseVelocity=v;}
  return best;
 }
 const double tau=t-m.reactionTime;
 const Vec3 d=q-(p+v*m.reactionTime);
 const double speed0=horizontalLength(v);
 const bool hasHeading=speed0>1e-6*m.maxSpeed;
 const Vec3 heading=hasHeading?v/speed0:Vec3{};
 const double speedCap=std::fmax(m.maxSpeed,speed0);
 for(int k=1;k<=steps;++k){
  const double t1=tau*k/steps;
  const Vec3 w=(d-v*(.5*t1))/(tau-.5*t1);
  const Vec3 u=(w-v)/t1;
  double accelUse;
  if(hasHeading){
   const double along=dot(u,heading);
   const double across=horizontalLength(u-heading*along);
   accelUse=std::hypot(along/(along>=0?m.accel:m.decel),across/m.turnAccel);
  }else accelUse=horizontalLength(u)/m.accel;
  const double effort=std::fmax(accelUse,horizontalLength(w)/speedCap);
  if(effort<best.effort){
   best.effort=effort;best.accelTime=t1;best.acceleration=u;best.cruiseVelocity=w;
  }
 }
 best.feasible=best.effort<=1;
 return best;
}

// Catch heights the planner may aim for; height is above the ground plane.
// missProbability weights what defenders can do with the ball after a miss.
struct CatchMode { const char* name; double height,reach,timeCost,penalty,missProbability; };
inline std::vector<CatchMode> catchModes(const MoverParams& m){
 return {{"chest",m.chestHeight,m.armReach,0,0,m.missChest},
         {"high",m.reachHeight+.75*(m.jumpReachHeight-m.reachHeight),m.armReach,m.jumpPrepTime,.15,m.missHigh},
         {"dive",.5*m.diveMaxHeight,m.armReach+m.diveReach,0,.35,m.missDive}};
}

// The receiver's body must be within mode.reach of the destination, move-time
// earlier when the mode costs time (planting for a jump).
inline ReachPlan receiverReach(const Agent& r,Vec3 destination,const CatchMode& mode,double t){
 const double move=t-mode.timeCost;
 if(!(move>=0))return {};
 const Vec3 coast=horizontal(r.position)+horizontal(r.velocity)*move;
 const Vec3 offset=horizontal(destination)-coast;
 const double dist=horizontalLength(offset);
 const Vec3 body=dist<=mode.reach?coast:horizontal(destination)-offset*(mode.reach/dist);
 return innerReach(r,body,move);
}

// Robustness of the catch to error in the receiver prediction: the largest of
// 0.5, 1 and 2 sigma by which the receiver could be displaced (ahead, behind, left,
// right of its heading) with the destination still reachable. 0 if even 0.5 fails.
inline double receiverMarginSigmas(const Agent& r,Vec3 passer,Vec3 destination,const CatchMode& mode,double t,long& work){
 const double sigma=r.uncertainty.sigmaAt(t,r.params.maxSpeed);
 const Vec3 along=unitOr(horizontal(r.velocity),unitOr(horizontal(r.position)-horizontal(passer),{1,0,0}));
 const Vec3 dirs[]={along,-along,rightOf(along),-rightOf(along)};
 double margin=0;
 for(double k:{.5,1.,2.}){
  for(Vec3 dir:dirs){
   work+=24;
   if(!receiverReach(r,destination+dir*(k*sigma),mode,t).feasible)return margin;
  }
  margin=k;
 }
 return margin;
}

// Smallest horizontal gap between a defender's reach and the ball at time t, over
// the ways the defender can play a ball at that height.
struct ReachGap { double gap; const char* mode; };
inline ReachGap defenderGap(const Agent& a,Vec3 ball,double t,double groundY){
 const MoverParams& m=a.params;
 const double h=ball.y-groundY;
 ReachGap best{kInf,"none"};
 if(h<0)return best;
 auto consider=[&](const char* mode,double reach,double timeCost){
  const double gap=outerReachGap(a,ball,t-timeCost)-reach;
  if(gap<best.gap)best={gap,mode};
 };
 if(h<=m.reachHeight)consider("stand",m.armReach,0);
 else if(h<=m.jumpReachHeight)consider("jump",m.armReach,m.jumpPrepTime);
 if(h<=m.diveMaxHeight)consider("dive",m.armReach+m.diveReach,0);
 return best;
}

// ---------------------------------------------------------------- evaluation

enum class Phase { BeforeCatch, AfterMiss };
inline const char* toString(Phase p){return p==Phase::BeforeCatch?"before catch":"after miss";}

// One defender's opportunity against one phase of the flight.
struct Threat {
 std::string defender;
 Phase phase=Phase::BeforeCatch;
 double minSigmas=kInf;   // smallest gap / sigma over the phase (the most dangerous moment)
 double gap=kInf;         // gap at that moment; <= 0 means the defender can reach the ball
 double sigma=0;          // 1-sigma prediction error at that moment
 double time=0;           // when (s after release)
 Vec3 ball;               // where the ball is then
 const char* mode="none"; // stand / jump / dive
 double probability=0;    // P(reach) = Phi(-gap/sigma), Gaussian error assumption
 bool intercept=false;    // gap <= 0 at some time in the phase
 double interceptTime=0;  // first such time (bisected)
 Vec3 interceptPoint;
};

inline Threat sweepThreat(const World& w,const Agent& d,const State& launch,double t0,double t1,double step,Phase phase,long& work,bool pruneDistant=false){
 Threat th;
 th.defender=d.name;
 th.phase=phase;
 if(pruneDistant){
  const auto a=horizontal(projectile::advance(launch,w,t0).position),b=horizontal(projectile::advance(launch,w,t1).position);
  const auto delta=b-a;const double len2=dot(delta,delta);
  const double u=len2>0?std::clamp(dot(horizontal(d.position)-a,delta)/len2,0.,1.):0;
  const double cap=std::max(d.params.maxSpeed,horizontalLength(d.velocity));
  const double gap=horizontalLength(horizontal(d.position)-(a+delta*u))-cap*t1-d.params.armReach-d.params.diveReach;
  const double sigma=d.uncertainty.sigmaAt(t1,d.params.maxSpeed);
  // Every horizontal ball position lies on this segment, and total defender
  // travel cannot exceed cap*t1. This is a lower bound, not an optimistic sample.
  if(gap>8*sigma){++work;th.gap=gap;th.sigma=sigma;th.minSigmas=gap/sigma;th.probability=normalCdf(-th.minSigmas);th.time=t1;th.ball=projectile::advance(launch,w,t1).position;return th;}
 }
 struct Point { double t,gap,sigma; Vec3 ball; const char* mode; double z() const {return gap/sigma;} };
 auto at=[&](double t){
  ++work;
  const Vec3 ball=projectile::advance(launch,w,t).position;
  const ReachGap g=defenderGap(d,ball,t,w.groundY);
  return Point{t,g.gap,d.uncertainty.sigmaAt(t,d.params.maxSpeed),ball,g.mode};
 };
 const int n=std::max(1,static_cast<int>(std::ceil((t1-t0)/step)));
 std::vector<Point> pts;
 pts.reserve(n+1);
 for(int i=0;i<=n;++i)pts.push_back(at(i==n?t1:t0+(t1-t0)*i/n));
 size_t worst=0;
 for(size_t i=1;i<pts.size();++i)if(pts[i].z()<pts[worst].z())worst=i;
 Point best=pts[worst];
 if(std::isfinite(best.z())&&pts.size()>1){
  // Golden-section refinement of the most dangerous moment between neighbours.
  double a=pts[worst>0?worst-1:0].t,b=pts[std::min(worst+1,pts.size()-1)].t;
  const double r=(std::sqrt(5.)-1)/2;
  for(int i=0;i<24;++i){
   const double c=b-r*(b-a),e=a+r*(b-a);
   const Point pc=at(c),pe=at(e);
   if(pc.z()<best.z())best=pc;
   if(pe.z()<best.z())best=pe;
   if(pc.z()<pe.z())b=e;else a=c;
  }
 }
 th.minSigmas=best.z();th.gap=best.gap;th.sigma=best.sigma;th.time=best.t;th.ball=best.ball;th.mode=best.mode;
 th.probability=std::isfinite(th.gap)?normalCdf(-th.minSigmas):0;
 // First reachable moment: bisect between the last unreachable and first reachable time.
 auto firstReach=[&](double lo,Point hit){
  double hi=hit.t;
  for(int k=0;k<60&&hi-lo>1e-12*(1+hi);++k){
   const double mid=.5*(lo+hi);
   const Point pm=at(mid);
   if(pm.gap<=0){hi=mid;hit=pm;}else lo=mid;
  }
  th.intercept=true;th.interceptTime=hit.t;th.interceptPoint=hit.ball;
 };
 for(size_t i=0;i<pts.size()&&!th.intercept;++i){
  if(pts[i].gap>0)continue;
  if(i==0){th.intercept=true;th.interceptTime=pts[0].t;th.interceptPoint=pts[0].ball;}
  else firstReach(pts[i-1].t,pts[i]);
 }
 // A reachable dip narrower than the sample spacing, found only by refinement.
 if(!th.intercept&&best.gap<=0)firstReach(pts[worst>0?worst-1:0].t,best);
 return th;
}

enum class Verdict { Invalid, Contested, Uncertain, PredictedClear };
inline const char* toString(Verdict v){
 switch(v){
  case Verdict::Invalid:return "invalid";
  case Verdict::Contested:return "contested";
  case Verdict::Uncertain:return "uncertain";
  default:return "predicted clear";
 }
}
enum class Reject { None, BadInput, FlightTime, Speed, Elevation, Ground, ReceiverUnreachable };
inline const char* toString(Reject r){
 switch(r){
  case Reject::None:return "accepted";
  case Reject::BadInput:return "bad input";
  case Reject::FlightTime:return "flight time out of range";
  case Reject::Speed:return "launch speed out of range";
  case Reject::Elevation:return "elevation out of range";
  case Reject::Ground:return "ball meets the ground first";
  default:return "receiver cannot reach";
 }
}

struct Candidate {
 double flightTime=0,depth=0,lateral=0;
 int mode=0;
};
inline bool sameCandidate(const Candidate& a,const Candidate& b){
 return a.mode==b.mode&&std::fabs(a.flightTime-b.flightTime)<1e-9&&std::fabs(a.depth-b.depth)<1e-9&&std::fabs(a.lateral-b.lateral)<1e-9;
}

// The cost of a feasible candidate, term by term (lower is better).
struct CostTerms {
 double verdict=0,risk=0,afterMiss=0,time=0,adjust=0,catchMode=0,effort=0,robustness=0;
 double total() const {return verdict+risk+afterMiss+time+adjust+catchMode+effort+robustness;}
 std::string describe() const {
  return format("verdict %.2f + risk %.3f + after-miss %.3f + time %.3f + adjust %.3f + catch %.2f + effort %.3f + robustness %.2f",
                verdict,risk,afterMiss,time,adjust,catchMode,effort,robustness);
 }
};

struct Evaluation {
 Candidate candidate;
 std::string label;
 Reject reject=Reject::BadInput;
 Verdict verdict=Verdict::Invalid;
 std::string why;                // the deciding reason for the verdict or rejection
 State launch;                   // the exact launch state; display and checks both use it
 Vec3 destination,landingPoint;
 double speed=0,elevationDeg=0,landingTime=0;
 ReachPlan receiverPlan;
 double receiverMarginSigmas=0;  // 0, 0.5, 1 or 2: see receiverMarginSigmas()
 std::vector<Threat> threats;    // one per defender per phase
 double preCatchRisk=0,postMissRisk=0,minClearanceSigmas=kInf;
 double cost=kInf;
 CostTerms costTerms;
 std::uint64_t validatedUpdate=0;// Planner update that produced this evaluation
 bool feasible() const {return reject==Reject::None;}
 const Threat* worstBeforeCatch() const {
  const Threat* w=nullptr;
  for(const Threat& t:threats)if(t.phase==Phase::BeforeCatch&&(!w||t.minSigmas<w->minSigmas))w=&t;
  return w;
 }
};

inline bool sceneValid(const Scene& s){
 try{s.world.validate();}catch(const std::invalid_argument&){return false;}
 auto agentOk=[](const Agent& a){
  return projectile::finite(a.position)&&projectile::finite(a.velocity)&&a.params.valid()&&a.uncertainty.valid();
 };
 if(!projectile::finite(s.passer)||!agentOk(s.receiver))return false;
 for(const Agent& d:s.defenders)if(!agentOk(d))return false;
 for(const Vec3& p:s.receiverRoute)if(!projectile::finite(p))return false;
 return true;
}

inline std::string candidateLabel(const Candidate& c,const char* mode){
 std::string s=c.depth< -1e-9?"comeback":c.depth>1e-9?"deep":"on-route";
 if(c.lateral>1e-9)s+=" right";
 else if(c.lateral< -1e-9)s+=" left";
 return s+" / "+mode;
}

// Fully evaluates one candidate. `work` receives the number of reach evaluations.
inline Evaluation evaluate(const Scene& scene,const PlannerConfig& cfg,const Candidate& c,long* work=nullptr){
 Evaluation e;
 e.candidate=c;
 long used=1;
 auto finish=[&](Reject r,std::string why){
  e.reject=r;e.verdict=Verdict::Invalid;e.why=std::move(why);e.cost=kInf;
  if(work)*work=used;
  return e;
 };
 if(!sceneValid(scene))return finish(Reject::BadInput,"scene has non-finite or invalid values");
 const auto modes=catchModes(scene.receiver.params);
 if(c.mode<0||c.mode>=static_cast<int>(modes.size())||!std::isfinite(c.depth)||!std::isfinite(c.lateral)||!std::isfinite(c.flightTime))
  return finish(Reject::BadInput,"candidate has invalid parameters");
 const World& w=scene.world;
 const LaunchConfig& L=cfg.launch;
 const CatchMode& mode=modes[c.mode];
 e.label=candidateLabel(c,mode.name);
 const double T=c.flightTime;
 if(T<L.minFlightTime||T>L.maxFlightTime)return finish(Reject::FlightTime,format("flight time %.3f s outside [%.2f, %.2f]",T,L.minFlightTime,L.maxFlightTime));

 const Vec3 passer=horizontal(scene.passer);
 const Vec3 centre=predictReceiver(scene,T)+scene.destinationBias;
 const Vec3 along=unitOr(centre-passer,{1,0,0});
 Vec3 destination=centre+along*c.depth+rightOf(along)*c.lateral;
 destination.y=w.groundY+mode.height+scene.destinationBias.y;
 e.destination=destination;

 const Vec3 aim=unitOr(horizontal(destination)-passer,along);
 Vec3 release=passer+aim*L.releaseForward+rightOf(aim)*L.releaseSide;
 release.y=scene.passer.y+L.releaseHeight;
 if(release.y<=w.groundY)return finish(Reject::Ground,"release point is not above the ground");
 // Exact launch velocity reaching the destination at time T.
 const Vec3 displacement=destination-release-w.gravityVector()*(.5*T*T);
 const double distance=length(displacement);
 if(distance<=L.releaseAlongLaunch)return finish(Reject::Speed,"destination is inside the launch offset");
 const Vec3 direction=displacement/distance;
 const Vec3 v0=direction*((distance-L.releaseAlongLaunch)/T);
 release=release+direction*L.releaseAlongLaunch;
 e.launch={release,v0};
 if(!projectile::finite(v0))return finish(Reject::BadInput,"launch velocity is not finite");
 e.speed=length(v0);
 e.elevationDeg=std::atan2(v0.y,horizontalLength(v0))*180/kPi;
 if(e.speed>L.maxSpeed+1e-9||e.speed<L.minSpeed-1e-9)
  return finish(Reject::Speed,format("needs %.2f units/s, limits [%.2f, %.2f]",e.speed,L.minSpeed,L.maxSpeed));
 if(e.elevationDeg>L.maxElevationDeg||e.elevationDeg<L.minElevationDeg)
  return finish(Reject::Elevation,format("needs %.1f deg, limits [%.1f, %.1f]",e.elevationDeg,L.minElevationDeg,L.maxElevationDeg));
 // The arc is concave, so between two points above the ground it stays above it.
 const projectile::Contact landing=projectile::groundContact(e.launch,w);
 if(landing.kind!=projectile::ContactKind::Airborne||landing.time<T)return finish(Reject::Ground,"ball lands before the destination");
 e.landingTime=landing.time;
 e.landingPoint=landing.point;

 if(cfg.destinationAllowed&&!cfg.destinationAllowed(destination,T))return finish(Reject::ReceiverUnreachable,"destination fails integration reach limits");
 used+=24;
 e.receiverPlan=receiverReach(scene.receiver,destination,mode,T);
 if(!e.receiverPlan.feasible)
  return finish(Reject::ReceiverUnreachable,format("best movement plan needs %.0f%% of a limit",100*e.receiverPlan.effort));
 e.receiverMarginSigmas=receiverMarginSigmas(scene.receiver,scene.passer,destination,mode,T,used);

 double clearBefore=1,clearAfter=1;
 e.threats.reserve(2*scene.defenders.size());
 for(const Agent& d:scene.defenders){
  e.threats.push_back(sweepThreat(w,d,e.launch,0,T,cfg.search.sweepStep,Phase::BeforeCatch,used,cfg.search.pruneDistantThreats));
  clearBefore*=1-e.threats.back().probability;
  e.minClearanceSigmas=std::fmin(e.minClearanceSigmas,e.threats.back().minSigmas);
  e.threats.push_back(sweepThreat(w,d,e.launch,T,e.landingTime,cfg.search.sweepStep,Phase::AfterMiss,used,cfg.search.pruneDistantThreats));
  clearAfter*=1-e.threats.back().probability;
  if(cfg.requireAfterMissClear)e.minClearanceSigmas=std::fmin(e.minClearanceSigmas,e.threats.back().minSigmas);
 }
 e.preCatchRisk=1-clearBefore;
 e.postMissRisk=1-clearAfter;
 e.reject=Reject::None;

 const ScoreConfig& S=cfg.score;
 const Threat* first=nullptr;  // earliest interception before the catch
 for(const Threat& t:e.threats)if((cfg.requireAfterMissClear||t.phase==Phase::BeforeCatch)&&t.intercept&&(!first||t.interceptTime<first->interceptTime))first=&t;
 if(first){
  e.verdict=Verdict::Contested;
  e.why=first->defender+format(" can reach the ball at t=%.3f s, height %.2f",first->interceptTime,first->interceptPoint.y-w.groundY)+" ("+first->mode+")";
 }else if(e.minClearanceSigmas<S.clearSigmas){
  const Threat* t=e.worstBeforeCatch();
  if(cfg.requireAfterMissClear)for(const auto& candidate:e.threats)if(!t||candidate.minSigmas<t->minSigmas)t=&candidate;
  e.verdict=Verdict::Uncertain;
  e.why=t->defender+format(" misses by %.2f units = %.2f sigma at t=%.3f s (needs %.1f sigma)",t->gap,t->minSigmas,t->time,S.clearSigmas);
 }else if(e.receiverMarginSigmas<S.receiverSigmas){
  e.verdict=Verdict::Uncertain;
  e.why=format("receiver reach holds only to %.1f sigma of its prediction (needs %.1f)",e.receiverMarginSigmas,S.receiverSigmas);
 }else if(e.receiverPlan.effort>S.maxEffortClear){
  e.verdict=Verdict::Uncertain;
  e.why=format("receiver plan uses %.0f%% of a movement limit",100*e.receiverPlan.effort);
 }else{
  e.verdict=Verdict::PredictedClear;
  e.why=scene.defenders.empty()?std::string("no defenders"):format("every defender stays >= %.2f sigma away before the catch",e.minClearanceSigmas);
 }
 CostTerms& k=e.costTerms;
 k.verdict=e.verdict==Verdict::Contested?S.contestedCost:e.verdict==Verdict::Uncertain?S.uncertainCost:0;
 k.risk=S.riskWeight*e.preCatchRisk;
 k.afterMiss=S.riskWeight*(cfg.requireAfterMissClear?1:mode.missProbability)*e.postMissRisk;
 k.time=S.timeWeight*T;
 k.adjust=S.adjustWeight*std::hypot(c.depth,c.lateral);
 k.catchMode=mode.penalty;
 k.effort=S.effortWeight*e.receiverPlan.effort;
 k.robustness=-S.marginWeight*e.receiverMarginSigmas;
 e.cost=k.total();
 if(cfg.adjustEvaluation)cfg.adjustEvaluation(e);
 if(work)*work=used;
 return e;
}

// The displayed arc: same launch state and the same advance() as the evaluation, so
// the drawing is the selected solution, not an approximation of it.
inline std::vector<projectile::Sample> displayPath(const World& w,const Evaluation& e,int intervals,bool afterMiss=false){
 return projectile::sampleFlight(e.launch,w,0,afterMiss?e.landingTime:e.candidate.flightTime,intervals);
}

inline std::vector<std::string> riskAssumptions(const PlannerConfig& c){
 return {
  "Receiver reach is an inner bound: an explicit coast/accelerate/cruise plan within speed, accel, decel and turn limits (ellipse fixed to the initial heading).",
  "Defender reach is an outer bound: after the reaction time, any direction at the largest acceleration limit, speed capped at max(top speed, current speed).",
  "Defenders can play the ball standing below reachHeight, jumping below jumpReachHeight (after jumpPrepTime) and diving below diveMaxHeight.",
  "Prediction error is isotropic Gaussian: sigma(t) = hypot(position, velocity*t, topSpeed*reactionError); P(reach) = Phi(-gap/sigma).",
  "Defenders are treated independently: risk = 1 - product(1 - P_i).",
  format("Predicted clear requires every defender gap >= %.1f sigma, the catch to survive %.1f sigma of receiver error and <= %.0f%% receiver effort.",c.score.clearSigmas,c.score.receiverSigmas,100*c.score.maxEffortClear),
  "After-miss risk is weighted by the receiver's assumed miss probability for the catch type (MoverParams::missChest/High/Dive).",
  "No drag, spin, wind, deflections or blocking; the ball is a point."};
}

// ---------------------------------------------------------------- planner

struct Decision {
 bool hasSelection=false;
 Evaluation selected;               // always evaluated in this update
 std::string reason;                // why it was selected or kept
 bool switched=false;               // selection differs from the previous update's
 bool blending=false;               // selection is a validated step toward a better choice
 int switches=0;                    // selection changes so far (a multi-step blend counts once)
 long work=0;                       // reach evaluations spent this update
 std::uint64_t update=0;
 int completedSweeps=0;             // full passes over the candidate grid
 double sweepProgress=0;            // fraction of the current pass done
 std::vector<Evaluation> considered;// everything evaluated this update
 int rejectCounts[7]={};            // considered candidates per Reject value
};

class Planner {
public:
 explicit Planner(PlannerConfig cfg):cfg_(std::move(cfg)){buildGrid();}
 const PlannerConfig& config() const {return cfg_;}
 const std::vector<Candidate>& grid() const {return grid_;}
 const Decision& decision() const {return decision_;}

 // Bounded work: re-validates the current selection and the best remembered
 // candidates, refines around the best, then continues the grid pass where the
 // previous update stopped.
 const Decision& update(const Scene& scene,const std::vector<Candidate>& seeds={}){
  ++updates_;
  Decision d;
  d.update=updates_;
  d.switches=decision_.switches;
  std::vector<Evaluation>& evals=d.considered;
  // Every run() below adds at most one entry; reserving keeps pointers into evals valid.
  evals.reserve(grid_.size()+pool_.size()+seeds.size()+16);
  long work=0;
  auto run=[&](const Candidate& c)->int{
   for(size_t i=0;i<evals.size();++i)if(sameCandidate(evals[i].candidate,c))return static_cast<int>(i);
   long used=0;
   Evaluation e=evaluate(scene,cfg_,c,&used);
   e.validatedUpdate=updates_;
   work+=used;
   evals.push_back(std::move(e));
   return static_cast<int>(evals.size()-1);
  };
  const long budget=cfg_.search.workBudget;
  const auto started=std::chrono::steady_clock::now();
  auto withinTime=[&]{
   return cfg_.search.timeBudgetMs<=0||std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count()<cfg_.search.timeBudgetMs;
  };
  if(hasIncumbent_)run(incumbent_);
  for(const auto& seed:seeds)if(work<budget&&withinTime())run(seed);
  for(const PoolEntry& p:pool_)if(work<budget&&withinTime())run(p.candidate);
  if(const Evaluation* best=cheapest(evals,nullptr)){
   const Candidate b=best->candidate;
   const double dt=cfg_.search.timeStep/2,dx=cfg_.search.refineOffset;
   // Refinement stays inside the configured adjustment range.
   auto D=[&](double v){return std::clamp(v,depthRange_[0],depthRange_[1]);};
   auto Lat=[&](double v){return std::clamp(v,lateralRange_[0],lateralRange_[1]);};
   const Candidate moves[]={{b.flightTime-dt,b.depth,b.lateral,b.mode},{b.flightTime+dt,b.depth,b.lateral,b.mode},
                            {b.flightTime,D(b.depth-dx),b.lateral,b.mode},{b.flightTime,D(b.depth+dx),b.lateral,b.mode},
                            {b.flightTime,b.depth,Lat(b.lateral-dx),b.mode},{b.flightTime,b.depth,Lat(b.lateral+dx),b.mode}};
   for(int i=0;i<cfg_.search.refineCount&&i<6&&work<budget&&withinTime();++i)run(moves[(refineTurn_+i)%6]);
   refineTurn_=(refineTurn_+1)%6;
  }
  for(size_t n=0;n<grid_.size()&&(n==0||(work<budget&&withinTime()));++n){
   run(grid_[cursor_]);
   if(++cursor_==grid_.size()){cursor_=0;++sweeps_;}
  }

  const ScoreConfig& S=cfg_.score;
  const Evaluation* incumbent=hasIncumbent_?&evals[0]:nullptr;
  // Remembered candidates rank on an exponentially weighted mean of the continuous
  // part of their cost, and keep its variance, so observation noise neither flips
  // near-equal choices nor hides real differences. The verdict term is never
  // averaged: a candidate that gets worse is ranked worse at once.
  auto stats=[&](const Evaluation& e){
   const double now=e.cost-e.costTerms.verdict;
   const double a=S.costSmoothing;
   const PoolEntry* prior=nullptr;
   if(hasIncumbent_&&sameCandidate(e.candidate,incumbent_))prior=&incumbentStats_;
   else for(const PoolEntry& p:pool_)if(sameCandidate(p.candidate,e.candidate)){prior=&p;break;}
   if(!prior)return PoolEntry{e.candidate,now,noiseVariance_};
   const double delta=now-prior->mean;
   return PoolEntry{e.candidate,prior->mean+a*delta,(1-a)*(prior->variance+a*delta*delta)};
  };
  auto ranked=[&](const Evaluation& e){return e.feasible()?e.costTerms.verdict+stats(e).mean:kInf;};
  auto requiredGain=[&](const Evaluation& a,const Evaluation& b){
   return S.switchMargin+S.noiseSigmas*std::sqrt(stats(a).variance+stats(b).variance);
  };
  auto decisionCost=[&](const Evaluation& e){
   return ranked(e)+(hasIncumbent_?S.continuityWeight*distance(e.candidate,incumbent_):0);
  };
  const Evaluation* challenger=nullptr;
  for(const Evaluation& e:evals){
   if(!e.feasible()||&e==incumbent)continue;
   if(!challenger||decisionCost(e)<decisionCost(*challenger))challenger=&e;
  }
  const Evaluation* chosen=nullptr;
  bool forced=false;
  if(!incumbent||!incumbent->feasible()){
   chosen=challenger;forced=true;
   d.reason=!incumbent?"initial selection: lowest cost":"previous selection became invalid ("+incumbent->why+")";
  }else if(incumbent->verdict==Verdict::Contested&&challenger&&challenger->verdict>Verdict::Contested){
   chosen=challenger;forced=true;
   d.reason="previous selection became contested ("+incumbent->why+")";
  }else if(challenger&&decisionCost(*challenger)<ranked(*incumbent)-requiredGain(*incumbent,*challenger)){
   chosen=challenger;
   d.reason=format("challenger better by %.3f including continuity (needed %.3f: margin + noise)",ranked(*incumbent)-decisionCost(*challenger),requiredGain(*incumbent,*challenger));
  }else{
   chosen=incumbent;
   d.reason=challenger?format("kept: best challenger ranks %.3f vs %.3f (switching needs %.3f less: margin + noise)",decisionCost(*challenger),ranked(*incumbent),requiredGain(*incumbent,*challenger))
                      :std::string("kept: no feasible challenger");
  }
  Candidate newBlendTarget;
  if(chosen&&chosen!=incumbent&&!forced&&S.blend&&chosen->candidate.mode==incumbent_.mode){
   // Step toward the challenger. The intermediate is a full candidate, evaluated
   // like any other, and only used if it is no worse a class than the challenger.
   const Candidate a=incumbent_,b=chosen->candidate;
   Candidate mid=b;
   mid.flightTime=a.flightTime+std::clamp(b.flightTime-a.flightTime,-S.blendTimeStep,S.blendTimeStep);
   const double dd=b.depth-a.depth,dl=b.lateral-a.lateral,len=std::hypot(dd,dl);
   if(len>S.blendOffsetStep){mid.depth=a.depth+dd*(S.blendOffsetStep/len);mid.lateral=a.lateral+dl*(S.blendOffsetStep/len);}
   if(!sameCandidate(mid,b)){
    const Verdict target=chosen->verdict;
    const std::string targetLabel=chosen->label;
    const int idx=run(mid);  // may reallocate evals: re-find pointers by candidate
    if(evals[idx].feasible()&&evals[idx].verdict>=target&&(!S.preserveBlendCost||evals[idx].cost<=std::max(incumbent->cost,chosen->cost)+1e-9)){
     chosen=&evals[idx];
     d.blending=true;
     newBlendTarget=b;
     d.reason+="; blending toward "+targetLabel;
    }else{
     for(const Evaluation& e:evals)if(sameCandidate(e.candidate,b)){chosen=&e;break;}
     d.reason+="; blend step rejected, jumping";
    }
   }
  }
  if(chosen){
   d.hasSelection=true;
   d.selected=*chosen;
   d.switched=!hasIncumbent_||!sameCandidate(chosen->candidate,incumbent_);
   // A multi-step blend counts as one switch: later steps toward, and arrival at,
   // the same target are continuations.
   const bool continuing=blendingBefore_&&(sameCandidate(chosen->candidate,blendTarget_)||(d.blending&&sameCandidate(newBlendTarget,blendTarget_)));
   if(d.switched&&hasIncumbent_&&!continuing)++d.switches;
   blendingBefore_=d.blending;
   blendTarget_=newBlendTarget;
  }else{
   d.reason=evals.empty()?"nothing evaluated":"no feasible candidate yet";
  }
  // Remember the best-ranked distinct candidates for re-validation next update.
  std::vector<std::pair<double,const Evaluation*>> order;
  for(const Evaluation& e:evals)if(e.feasible())order.push_back({ranked(e),&e});
  std::sort(order.begin(),order.end(),[](const auto& x,const auto& y){return x.first<y.first;});
  std::vector<PoolEntry> pool;
  for(const auto& o:order){
   if(static_cast<int>(pool.size())>=cfg_.search.poolSize)break;
   pool.push_back(stats(*o.second));
  }
  if(chosen){
   // The incumbent's variance doubles as the prior for candidates seen for the first time.
   if(hasIncumbent_&&sameCandidate(chosen->candidate,incumbent_))noiseVariance_=stats(*chosen).variance;
   incumbentStats_=stats(*chosen);
   incumbent_=chosen->candidate;
   hasIncumbent_=true;
  }else hasIncumbent_=false;
  pool_=std::move(pool);
  for(const Evaluation& e:evals)++d.rejectCounts[static_cast<int>(e.reject)];
  d.work=work;
  d.completedSweeps=sweeps_;
  d.sweepProgress=grid_.empty()?1:static_cast<double>(cursor_)/static_cast<double>(grid_.size());
  decision_=std::move(d);
  return decision_;
 }

 Evaluation revalidate(const Scene& scene) const {return hasIncumbent_?evaluate(scene,cfg_,incumbent_):Evaluation{};}

 // Every grid candidate, without a budget; for diagnostics and visualisation.
 std::vector<Evaluation> evaluateGrid(const Scene& scene) const {
  std::vector<Evaluation> out;
  out.reserve(grid_.size());
  for(const Candidate& c:grid_)out.push_back(evaluate(scene,cfg_,c));
  return out;
 }

 // Distance in grid steps; used for the continuity penalty.
 double distance(const Candidate& a,const Candidate& b) const {
  const double offsetScale=2*cfg_.search.refineOffset;
  return std::fabs(a.flightTime-b.flightTime)/cfg_.search.timeStep+std::hypot(a.depth-b.depth,a.lateral-b.lateral)/offsetScale+(a.mode!=b.mode?2:0);
 }

private:
 static const Evaluation* cheapest(const std::vector<Evaluation>& evals,const Evaluation* skip){
  const Evaluation* best=nullptr;
  for(const Evaluation& e:evals)if(e.feasible()&&&e!=skip&&(!best||e.cost<best->cost))best=&e;
  return best;
 }
 // Flight times ascending so fast throws are seen first in each pass.
 void buildGrid(){
  const LaunchConfig& L=cfg_.launch;
  const int modeCount=3;
  std::vector<std::pair<double,double>> offsets;
  for(double d:cfg_.search.depthOffsets)for(double l:cfg_.search.lateralOffsets)offsets.push_back({d,l});
  const auto& ds=cfg_.search.depthOffsets;
  const auto& ls=cfg_.search.lateralOffsets;
  if(!ds.empty())depthRange_={*std::min_element(ds.begin(),ds.end()),*std::max_element(ds.begin(),ds.end())};
  if(!ls.empty())lateralRange_={*std::min_element(ls.begin(),ls.end()),*std::max_element(ls.begin(),ls.end())};
  std::stable_sort(offsets.begin(),offsets.end(),[](auto x,auto y){return std::hypot(x.first,x.second)<std::hypot(y.first,y.second);});
  const int steps=static_cast<int>(std::floor((L.maxFlightTime-L.minFlightTime)/cfg_.search.timeStep+1e-9));
  if(cfg_.search.offsetMajor){
   for(auto o:offsets)for(int m=0;m<modeCount;++m)for(int i=0;i<=steps;++i)
    grid_.push_back({L.minFlightTime+i*cfg_.search.timeStep,o.first,o.second,m});
   return;
  }
  for(int i=0;i<=steps;++i){
   const double t=L.minFlightTime+i*cfg_.search.timeStep;
   for(int m=0;m<modeCount;++m)for(auto o:offsets)grid_.push_back({t,o.first,o.second,m});
  }
 }

 struct PoolEntry { Candidate candidate; double mean,variance; };
 PlannerConfig cfg_;
 std::vector<Candidate> grid_;
 std::vector<PoolEntry> pool_;
 PoolEntry incumbentStats_{};
 double noiseVariance_=0;
 std::array<double,2> depthRange_{0,0},lateralRange_{0,0};
 size_t cursor_=0;
 int sweeps_=0,refineTurn_=0;
 std::uint64_t updates_=0;
 bool hasIncumbent_=false,blendingBefore_=false;
 Candidate incumbent_,blendTarget_;
 Decision decision_;
};

}
