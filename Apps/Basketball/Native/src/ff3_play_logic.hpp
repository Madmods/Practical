#pragma once
#include "ff3_ball_flights.hpp"
#include <limits>
namespace ff3Play {
using ff3qb::Vec;
inline Vec flat(Vec v){return {v.x,0,v.z};}
inline double distance(Vec a,Vec b){return ff3qb::length(flat(a-b));}
inline Vec unit(Vec v){double n=ff3qb::length(v);return n>1e-6?v*(1/n):Vec{};}
struct Config {bool catchOn=false,jumpOn=false,rushOn=false,hold=true,refresh=false;int catchKey=1,jumpKey=32,rushKey=6;double range=150,stop=3,cooldown=.28,refreshPeriod=.7;};
struct Carrier {std::uint64_t id=0,ball=0;Vec position,velocity;};
struct Scene {
 bool valid=false,focused=false,typing=true,menu=true,possessed=false,grounded=false,manual=false,activation=false;
 double groundY=NAN,at=0,worldGravity=196.2,jumpVelocity=0,speed=0;std::uint32_t pid=0;std::uint64_t model=0,own=0,player=0;
 Vec head,velocity,cameraForward,cameraRight;ff3qb::CatchGeometry shape,naturalShape;
 std::vector<ff3qb::BallFlight> balls;std::vector<Carrier> carriers;
};
struct Decision {bool catchNow=false,jumpNow=false,moving=false,catchHold=false;Vec direction,spot;double eta=0;std::uint64_t ball=0,target=0;std::string reason="Waiting for a ball";};
// Resizing an outgoing or unattributed ball can change QB collisions before
// ownership arrives. Only positively identified incoming throws may be resized.
inline bool magBallCandidate(const Scene& s,const ff3qb::BallFlight& b){return !s.possessed&&s.player&&s.own&&b.id&&b.airborne&&!b.ended&&b.thrower&&b.thrower!=s.player&&b.thrower!=s.own&&ff3qb::finite(b.position)&&ff3qb::length(b.position-s.head)<=80;}
inline bool receivingBallCandidate(const Scene& s,const ff3qb::BallFlight& b,double ground,double time){
 if(!b.id||!b.airborne||b.ended||!std::isfinite(ground)||!std::isfinite(time)||time<b.at||time-b.at>.12||!ff3qb::finite(b.position)||!ff3qb::finite(b.velocity)||!std::isfinite(b.gravity)||b.gravity<1||b.gravity>300)return false;
 // Restore normal geometry and floor collision well before expected impact.
 // Resting, held and low balls never enter the non-solid enlarged state.
 const double horizon=.20+std::max(0.,time-b.at),futureY=b.position.y+b.velocity.y*horizon-.5*b.gravity*horizon*horizon;
 return std::min(b.position.y,futureY)>ground+2.;
}
inline bool groundBallCandidate(Vec position,Vec velocity,double ground){
 return ff3qb::finite(position)&&ff3qb::finite(velocity)&&std::isfinite(ground)&&position.y>=ground-1&&position.y<=ground+2.5&&std::abs(velocity.y)<4&&ff3qb::length(velocity)<60;
}
inline bool observing(const Scene& s,double time){return s.valid&&s.pid&&s.model&&s.own&&s.focused&&!s.typing&&!s.menu&&ff3qb::finite(s.head)&&ff3qb::finite(s.velocity)&&std::isfinite(s.worldGravity)&&s.worldGravity>=10&&s.worldGravity<=300&&std::isfinite(time)&&time>=s.at&&time-s.at<.12;}
inline bool allowed(const Scene& s,double time){return observing(s,time)&&!s.possessed;}
inline Vec ballAt(const ff3qb::BallFlight& b,double seconds){
 // Anchor the forecast at the latest observation, not the earlier fit time.
 Vec velocity=b.velocity;
 if(b.model.valid){double at=b.model.observedTime(b.position);velocity=b.model.velocity+Vec{0,-b.gravity*(at-b.model.at),0};}
 return b.position+velocity*seconds+Vec{0,-.5*b.gravity*seconds*seconds,0};
}
inline double catchGap(const Scene& s,Vec ball,double time,bool jump,bool natural=false){
 Vec head=s.head+s.velocity*time;
 if(s.grounded)head.y=s.head.y;
 else head.y-=.5*s.worldGravity*time*time;
 if(jump){double dt=std::max(0.,time-.045);head.y+=std::max(0.,s.jumpVelocity*dt-.5*s.worldGravity*dt*dt);}
 const double vertical=ball.y-head.y;
 const auto& shape=natural&&s.naturalShape.measured?s.naturalShape:s.shape;
 return std::max({distance(head,ball)-shape.radius,vertical-shape.above,-vertical-shape.below});
}
inline double pursuitTime(Vec delta,Vec velocity,double speed){
 double a=ff3qb::dot(velocity,velocity)-speed*speed,b=2*ff3qb::dot(delta,velocity),c=ff3qb::dot(delta,delta),t=0;
 if(std::abs(a)<1e-6){if(b< -1e-6)t=-c/b;}
 else{double d=b*b-4*a*c;if(d>=0){double r1=(-b-std::sqrt(d))/(2*a),r2=(-b+std::sqrt(d))/(2*a);t=r1>0?r1:r2>0?r2:0;if(r2>0&&t>r2)t=r2;}}
 return std::clamp(std::isfinite(t)?t:0.,0.,.6);
}
struct Planner {
 double lastCatch=-10,lastJump=-10;std::uint64_t jumpedBall=0,rushTarget=0;std::uint32_t pid=0;std::uint64_t model=0,own=0;
 void reset(){*this={};}
 Decision step(const Config& c,const Scene& s,double time){
  Decision out;
  if(pid!=s.pid||model!=s.model||own!=s.own){reset();pid=s.pid;model=s.model;own=s.own;}
  if(!allowed(s,time)){out.reason=s.possessed?"Ball possessed: receiving and rush paused":"Paused: focus, menu, typing or stale state";rushTarget=0;return out;}
  double best=INFINITY;
  if((c.catchOn||c.jumpOn)&&s.shape.measured)for(const auto& ball:s.balls){
   if(!ball.id||!ball.airborne||ball.ended||!ff3qb::finite(ball.position)||!ff3qb::finite(ball.velocity)||!std::isfinite(ball.gravity)||ball.gravity<=0||time<ball.at||time-ball.at>.12||ball.thrower&&(ball.thrower==s.player||ball.thrower==s.own))continue;
   if(distance(s.head,ball.position)>70)continue;
   double direct=INFINITY,jumping=INFINITY,standing=INFINITY;const auto& natural=s.naturalShape.measured?s.naturalShape:s.shape;
   const double horizon=std::min(.65,2*s.jumpVelocity/s.worldGravity+.045);
   for(int i=0;i<=65;i++){double dt=i*.01;auto p=ballAt(ball,dt);
    if(catchGap(s,p,dt,false)<=-.12)direct=std::min(direct,dt);
    if(catchGap(s,p,dt,false,true)<=-.12)standing=std::min(standing,dt);
    if(c.jumpOn&&s.grounded&&s.jumpVelocity>0&&dt>=.08&&dt<=horizon&&p.y>s.head.y+natural.above+.25&&catchGap(s,p,dt,true,true)<=-.12)jumping=std::min(jumping,dt);
   }
   bool jump=jumping<standing&&jumping<INFINITY&&time-lastJump>.7&&jumpedBall!=ball.id;
   double eta=jump?jumping:direct;if(!std::isfinite(eta)||eta>best)continue;
   best=eta;out.ball=ball.id;out.spot=ballAt(ball,eta);out.eta=eta;out.jumpNow=jump;out.catchHold=c.catchOn&&(direct<=.55||jump&&jumping<=.65);out.catchNow=out.catchHold&&time-lastCatch>=c.cooldown;
   out.reason=jump?"Jump catch opportunity":out.catchHold?"Raising hands for incoming pass":"Tracking reachable pass";
  }
  if(out.jumpNow){lastJump=time;jumpedBall=out.ball;}
  if(out.catchNow)lastCatch=time;
  if(c.rushOn){
   if(c.hold&&!s.activation){if(!out.ball)out.reason="Hold rush activation";rushTarget=0;return out;}
   if(s.manual){if(!out.ball)out.reason="Manual movement takes priority";return out;}
   const Carrier* target=nullptr;double closest=c.range;
   for(const auto& actor:s.carriers){double d=distance(actor.position,s.head);if(actor.id&&actor.ball&&ff3qb::finite(actor.position)&&ff3qb::finite(actor.velocity)&&d<=closest){closest=d;target=&actor;}}
   // Retain the carrier while possession remains verified; do not bounce between two balls.
   for(const auto& actor:s.carriers)if(actor.id==rushTarget&&actor.ball&&distance(actor.position,s.head)<=c.range)target=&actor;
   rushTarget=target?target->id:0;
   if(target&&!out.ball){Vec delta=flat(target->position-s.head),velocity=flat(target->velocity);double lead=pursuitTime(delta,velocity,std::max(1.,s.speed));out.spot=target->position+velocity*lead;out.target=target->id;out.direction=unit(flat(out.spot-s.head));out.moving=distance(target->position,s.head)>c.stop;out.reason=out.moving?"Rushing opposing ball carrier":"At ball carrier";}
   else if(!target&&!out.ball)out.reason="No opposing ball carrier in range";
  }
  return out;
 }
};
}
