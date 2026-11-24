#pragma once
#include "ff3_assist_logic.hpp"
namespace ff3Assist {
inline Vec capped(Vec value,double limit){const double size=ff3qb::length(value);return size>limit&&size>0?value*(limit/size):value;}
inline double ease(double u){u=std::clamp(u,0.,1.);return u*u*u*(10+u*(-15+6*u));}
inline double easeRate(double u){u=std::clamp(u,0.,1.);return 30*u*u*(1-u)*(1-u);}
// Control root position, not Head: dive/body rotation moves the head relative
// to the actual assembly and must not create a false positional error.
inline Vec motionPosition(const ff3Play::Scene& scene){return ff3qb::finite(scene.rootPosition)?scene.rootPosition:scene.head;}
struct VelocitySample {bool valid=false,complete=false;Vec reference,velocity;bool timedOut=false;};
// Solve distance = speed*latency + speed^2/(2*deceleration). Speed is a
// ceiling; close targets always use less, even with the slider at maximum.
inline double arrivalSpeed(double gap,double limit,double radius=.3){
 if(!std::isfinite(gap)||!std::isfinite(limit)||!std::isfinite(radius)||limit<0||radius<0)return 0;
 const double available=std::max(0.,gap-radius),deceleration=220.,latency=.065;
 return std::min({limit,available*20.,std::sqrt(std::pow(deceleration*latency,2)+2*deceleration*available)-deceleration*latency});
}
inline bool pullRequest(int mode,bool down,bool prior,bool busy,double time,double next){return !busy&&time>=next&&(mode==2||mode==1&&down||mode==0&&down&&!prior);}
inline bool pullReleased(int mode,bool down){return mode==1&&!down;}

// Feedback controller with a bounded target and an explicit braking phase.
// Time bounds the action; it never declares arrival just because time elapsed.
struct PullTween {
 bool active=false,stopping=false;Vec origin,initial,goal;double at=0,duration=0,limit=0,ramp=.04,settledAt=-1,arrival=.3,lastSample=0;
 bool begin(Vec position,Vec velocity,Vec destination,double time,double minimumTime,double maxSpeed){
  active=false;if(!ff3qb::finite(position)||!ff3qb::finite(velocity)||!ff3qb::finite(destination)||!std::isfinite(time)||!std::isfinite(minimumTime)||!std::isfinite(maxSpeed)||minimumTime<.05||minimumTime>.5||maxSpeed<10||maxSpeed>120||ff3qb::length(velocity)>180)return false;
  const double gap=ff3qb::length(destination-position);if(gap<.1||gap>24)return false;
  origin=position;initial=velocity;goal=destination;at=lastSample=time;limit=maxSpeed;stopping=false;settledAt=-1;
  ramp=std::clamp(minimumTime*.12,.008,.035);duration=std::max(minimumTime,2.2*gap/maxSpeed);if(duration>4.5)return false;active=true;return true;
 }
 bool retarget(Vec destination,double range=24){if(!active||!ff3qb::finite(destination)||!std::isfinite(range)||range<0||ff3qb::length(destination-origin)>std::min(24.,range))return false;goal=destination;return true;}
 void brake(){stopping=true;settledAt=-1;}
 VelocitySample sample(Vec position,Vec velocity,double time,double gravity=0){
  if(!active||!ff3qb::finite(position)||!ff3qb::finite(velocity)||!std::isfinite(time)||time<at||!std::isfinite(gravity)||gravity<0||gravity>300)return {};
  const double elapsed=time-at,gap=ff3qb::length(goal-position);const bool atContact=gap<=arrival+.05;const double interval=std::clamp(time-lastSample,.004,.05);lastSample=time;
  if(elapsed>duration+.7)return {true,true,goal,{},true};
  if(stopping||atContact){if(ff3qb::length(flat(velocity))<=4&&std::abs(velocity.y)<=std::max(4.,300*interval)){if(settledAt<0)settledAt=time;}else settledAt=-1;const bool complete=settledAt>=0&&time-settledAt>=.04;return {true,complete,goal,complete?Vec{}:Vec{0,.5*gravity*interval,0}};}
  settledAt=-1;const double speed=arrivalSpeed(gap,limit,arrival),blend=ease(elapsed/ramp);
  const auto desired=unit(goal-position)*speed;
  return {true,false,goal,capped(capped(initial*(1-blend)+desired*blend,speed)+Vec{0,.5*gravity*interval,0},limit)};
 }
 void cancel(){active=stopping=false;settledAt=-1;}
};

// A short additive forward impulse, not a positional controller. Steering is
// confined to a forward cone; distance never slows, reverses or stops the dive.
struct DiveVelocity {
 bool active=false,stopping=false;Vec origin,initial,goal,forward{0,0,-1};double at=0,speed=0,steering=.65,duration=.12;
 bool begin(Vec position,Vec velocity,Vec destination,double time,double boost,Vec fallback={0,0,-1},double assist=.65){
  active=false;if(!ff3qb::finite(position)||!ff3qb::finite(velocity)||!ff3qb::finite(destination)||!ff3qb::finite(fallback)||!std::isfinite(time)||!std::isfinite(boost)||boost<0||boost>60||!std::isfinite(assist)||assist<0||assist>1||ff3qb::length(flat(velocity))>120)return false;
  initial=flat(velocity);forward=unit(ff3qb::length(initial)>=4?initial:flat(fallback));if(ff3qb::length(forward)<.5)return false;
  origin=position;goal=destination;at=time;speed=boost;steering=assist;stopping=false;active=true;return true;
 }
 bool retarget(Vec destination){if(!active||!ff3qb::finite(destination))return false;goal=destination;return true;}
 void brake(){stopping=true;}
 VelocitySample sample(Vec position,Vec currentVelocity,double time){
  if(!active||!ff3qb::finite(position)||!ff3qb::finite(currentVelocity)||!std::isfinite(time)||time<at)return {};
  const double elapsed=time-at;if(stopping||elapsed>=duration)return {true,true,goal,currentVelocity};
  auto toward=unit(flat(goal-position));const double dot=std::clamp(ff3qb::dot(forward,toward),-1.,1.);
  double angle=dot>.05?std::atan2(forward.x*toward.z-forward.z*toward.x,dot):0.;
  angle=std::clamp(angle,-steering*.6108652382,steering*.6108652382);
  const double cosine=std::cos(angle),sine=std::sin(angle);const Vec push{forward.x*cosine-forward.z*sine,0,forward.x*sine+forward.z*cosine};
  auto value=capped(initial+push*(speed*ease(elapsed/.016)),120.);value.y=currentVelocity.y;return {true,false,goal,value};
 }
 void cancel(){active=stopping=false;}
};
inline bool freePhysics(int state,unsigned platform){return state==16||state==14&&platform==1;}
inline bool diveOwnsMovement(double activated,double time,bool pulse,bool naturalPhysics){return pulse||activated>0&&(time<activated+.55||time<activated+2&&naturalPhysics);}
inline bool diveInputSettled(double requested,double time){return std::isfinite(requested)&&std::isfinite(time)&&time>=requested+.025;}
inline bool diveAirborne(const ff3Play::Scene& scene){
 const double standing=std::isfinite(scene.standingRootHeight)?scene.standingRootHeight:3.5;
 return !scene.grounded&&std::isfinite(scene.groundY)&&ff3qb::finite(motionPosition(scene))&&motionPosition(scene).y-scene.groundY>std::max(1.5,standing*.55);
}
inline bool diveLaunchReady(double requested,double time,int state,unsigned platform,Vec velocity,bool airborne=true){
 // A fresh E press while airborne can precede the replicated ragdoll flag.
 // Do not wait for that flag, and never begin a stale action after touchdown.
 return requested>0&&std::isfinite(requested)&&std::isfinite(time)&&time>=requested&&time-requested<=.18&&airborne&&(freePhysics(state,platform)||state==3||state==5)&&ff3qb::finite(velocity)&&ff3qb::length(flat(velocity))>=4;
}
inline bool controlledPhysics(int state,unsigned platform,unsigned evaluation){return freePhysics(state,platform)||evaluation==0;}
// Pull the assembly toward the ball center. Preserve only the clearance needed
// for the character to remain above the field; head pose adds no aim offset.
inline Vec rootContact(const ff3Play::Scene& scene,Vec ball){
 const double standing=std::isfinite(scene.standingHeadHeight)?scene.standingHeadHeight:5.;
 if(std::isfinite(scene.groundY))ball.y=std::max(ball.y,scene.groundY+(std::isfinite(scene.standingRootHeight)?scene.standingRootHeight:standing-1.5));return ball;
}
enum class PreparationResult {Idle,Waiting,Ready,Timeout,Invalid};
struct PullPreparation {
 bool waiting=false;Vec goal;double at=0,minimum=0,speed=0,resultAt=0;unsigned requests=0,timeouts=0;PreparationResult last=PreparationResult::Idle;
 bool request(Vec target,double time,double minTime,double maxSpeed){if(!ff3qb::finite(target)||!std::isfinite(time)||minTime<.05||minTime>.5||maxSpeed<10||maxSpeed>120)return false;goal=target;at=time;minimum=minTime;speed=maxSpeed;waiting=true;last=PreparationResult::Waiting;++requests;return true;}
 PreparationResult step(PullTween& tween,const ff3Play::Scene& scene,double time,int state,unsigned platform,unsigned evaluation=255){
  if(!waiting)return PreparationResult::Idle;if(time<at||time-at>.25){waiting=false;++timeouts;resultAt=time;return last=PreparationResult::Timeout;}
  if(!controlledPhysics(state,platform,evaluation))return PreparationResult::Waiting;waiting=false;
  resultAt=time;return last=tween.begin(motionPosition(scene),scene.velocity,goal,time,minimum,speed)?PreparationResult::Ready:PreparationResult::Invalid;
 }
 void cancel(){waiting=false;}
};

// Independent position samples verify whether commanded velocity reached the
// simulation. Stored velocity and ordinary walking alone cannot confirm it.
struct MotionWitness {
 Vec priorPosition,baseline,command,measured;double priorAt=0,matchedTime=0;unsigned matched=0,mismatched=0;bool armed=false,horizontal=false;
 void begin(const ff3Play::Scene& scene,bool flatOnly){priorPosition=motionPosition(scene);baseline=scene.velocity;priorAt=scene.at;matchedTime=0;matched=mismatched=0;armed=false;horizontal=flatOnly;measured={};}
 void issued(Vec value){command=value;armed=true;}
 bool step(const ff3Play::Scene& scene){const double dt=scene.at-priorAt;if(dt<=0)return false;const auto current=motionPosition(scene);const auto change=current-priorPosition;priorPosition=current;priorAt=scene.at;
  if(!armed||dt<.004||dt>.15){matched=0;matchedTime=0;return false;}measured=change*(1/dt);
  const auto expected=horizontal?flat(command):command,actual=horizontal?flat(measured):measured,normal=horizontal?flat(baseline):baseline;
  const double speed=ff3qb::length(expected),demand=ff3qb::length(expected-normal);
  if(speed>=6&&demand>=6&&ff3qb::length(actual-expected)<=std::max(5.,speed*.25)){++matched;matchedTime+=dt;}else{++mismatched;matched=0;matchedTime=0;}
  return matched>=2&&matchedTime>=.04;
 }
};
}
