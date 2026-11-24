#pragma once
#include "ff3_play_logic.hpp"
namespace ff3Assist {
using ff3qb::Vec;using ff3Play::distance;using ff3Play::flat;using ff3Play::unit;
struct Actor {std::uint64_t id=0,player=0;Vec position,velocity;bool opponent=false,carrier=false;double speed=22,jumpVelocity=50,headOffset=1.5;};
struct Opportunity {bool valid=false;int action=0;std::uint64_t ball=0;Vec contact,stand,takeoff;double eta=0,jumpIn=0,radius=0,rise=0,score=-INFINITY;};
struct ReceivePreview {std::array<Opportunity,3> actions;int selected=-1;};
inline bool incoming(const ff3Play::Scene& s,const ff3qb::BallFlight& ball,double time){return ball.id&&ball.airborne&&!ball.ended&&ff3qb::finite(ball.position)&&ff3qb::finite(ball.velocity)&&std::isfinite(ball.gravity)&&ball.gravity>=1&&ball.gravity<=300&&time>=ball.at&&time-ball.at<=.12&&(!ball.lastMotion||(ball.measured&&time-ball.lastMotion<=.2))&&!(ball.thrower&&(ball.thrower==s.player||ball.thrower==s.own));}
inline ReceivePreview preview(const ff3Play::Scene& s,double time,double dive=8,const ff3qb::FieldBounds* bounds=nullptr,const ReceivePreview* previous=nullptr,double elapsed=0){
 ReceivePreview out;if(!ff3Play::allowed(s,time)||!s.naturalShape.measured||!std::isfinite(s.groundY)||!std::isfinite(s.speed)||s.speed<0||s.speed>60||!std::isfinite(dive)||dive<0||dive>16)return out;
 if(!std::isfinite(s.naturalShape.radius)||!std::isfinite(s.naturalShape.above)||!std::isfinite(s.naturalShape.below)||s.naturalShape.radius<=0||s.naturalShape.radius>100||s.naturalShape.above<0||s.naturalShape.above>100||s.naturalShape.below<0||s.naturalShape.below>100||!std::isfinite(s.jumpVelocity)||s.jumpVelocity<0||s.jumpVelocity>150)return out;
 const auto shape=s.naturalShape;const double apex=s.jumpVelocity/s.worldGravity,maxRise=s.jumpVelocity*s.jumpVelocity/(2*s.worldGravity);
 for(const auto& ball:s.balls){if(!incoming(s,ball,time)||distance(ball.position,s.head)>200)continue;
  std::vector<double> samples;for(int step=2;step<=80;++step)samples.push_back(step*.05);
  if(previous&&elapsed>=0&&elapsed<.18)for(const auto& old:previous->actions)if(old.valid&&old.ball==ball.id&&old.eta-elapsed>=.05)samples.push_back(old.eta-elapsed);
  for(double t:samples){auto point=ff3Play::ballAt(ball,t);if(!ff3qb::finite(point)||point.y<s.groundY+.6)continue;
   for(int action=0;action<3;++action){
    if(action==1&&!s.grounded)continue;const bool jump=action>0&&s.grounded;const double phase=jump?std::min(t,apex):0,rise=jump?std::max(0.,s.jumpVelocity*phase-.5*s.worldGravity*phase*phase):0;
    if(jump&&(!s.grounded||s.jumpVelocity<=0||!std::isfinite(maxRise)))continue;
    const double resting=s.groundY+(std::isfinite(s.standingHeadHeight)?s.standingHeadHeight:5),headY=s.grounded?s.head.y:std::max(resting,s.head.y+s.velocity.y*t-.5*s.worldGravity*t*t);
    const double vertical=point.y-(headY+rise);if(vertical>shape.above-.15||vertical< -shape.below+.15)continue;
    const double radius=shape.radius+(action==2?dive:0),gap=distance(s.head,point),walk=std::max(0.,gap-radius+.35);
    if(walk>std::max(0.,s.speed*t-.1))continue;
    const auto direction=unit(flat(point-s.head));auto stand=s.head+direction*walk;stand.y=s.groundY;
    if(bounds&&(!bounds->valid()||!bounds->contains(stand,1)))continue;
    const double travel=distance(s.head,stand);if(travel>s.speed*t)continue;
    const double takeoffAt=jump?std::max(0.,t-phase):action==2?std::max(0.,t-.16):t;
    auto takeoff=s.head+direction*std::min(travel,s.speed*takeoffAt);takeoff.y=s.groundY;
    double score=point.y*.18-travel*.045-t*.25-(action==2?.5:0);
    if(previous&&elapsed>=0&&elapsed<.18){const auto& old=previous->actions[action];if(old.valid&&old.ball==ball.id&&distance(old.contact,point)<12)score+=1.5-std::min(1.5,std::abs(t-(old.eta-elapsed))*4);}
    auto& best=out.actions[action];if(score<=best.score)continue;
    best={true,action,ball.id,point,stand,takeoff,t,takeoffAt,radius,rise,score};
   }
  }
 }
 // Prefer the jump opportunity for a maximum-height preview. Standing remains
 // explicit when the pass is already reachable without leaving the ground.
 if(out.actions[1].valid)out.selected=1;else if(out.actions[0].valid)out.selected=0;else if(out.actions[2].valid)out.selected=2;
 return out;
}
// Smooth only the floor guide. Ball contact and camera projection stay live.
struct GuideTracker {
 ReceivePreview value;double at=0;
 ReceivePreview update(ReceivePreview next,double time){
  const double dt=time-at,alpha=dt>0?1-std::exp(-dt/.045):1;
  for(int i=0;i<3;++i){auto& p=next.actions[i];const auto& old=value.actions[i];if(p.valid&&old.valid&&p.ball==old.ball&&dt>0&&dt<.18&&distance(p.takeoff,old.takeoff)<5){p.takeoff=old.takeoff+(p.takeoff-old.takeoff)*alpha;p.stand=old.stand+(p.stand-old.stand)*alpha;}}
  value=next;at=time;return value;
 }
 void clear(){value={};at=0;}
};
inline bool actionIncoming(const ff3Play::Scene& scene,const ff3qb::BallFlight& ball,double time,bool practice){return incoming(scene,ball,time)&&(practice||ball.thrower!=0);}
inline bool floorLoose(const ff3Play::Scene& scene,const ff3qb::BallObservation& ball){return std::isfinite(scene.groundY)&&ff3qb::finite(ball.position)&&ball.position.y>=scene.groundY-1&&ball.position.y<=scene.groundY+std::max(1.35,std::isfinite(ball.radius)?ball.radius+.35:1.35);}
inline bool movingScriptBall(const ff3Play::Scene& scene,const ff3qb::BallObservation& ball,double time){for(const auto& flight:scene.balls)if(flight.id==ball.id&&incoming(scene,flight,time)&&flight.measured&&ff3qb::length(flight.velocity)>4&&(!flight.lastMotion||time-flight.lastMotion<.2))return true;return false;}
inline bool looseActionCandidate(const ff3Play::Scene& scene,const ff3qb::BallObservation& ball,bool practice){
 if(!ball.id||!ball.verifiedBall||!ff3qb::finite(ball.position)||scene.possessed)return false;
 if(floorLoose(scene,ball))return true;
 if(ball.anchored&&!movingScriptBall(scene,ball,scene.at))return false;
 return !(ball.thrower&&(ball.thrower==scene.own||ball.thrower==scene.player))&&(practice||ball.thrower!=0);
}
inline Vec looseTarget(const ff3Play::Scene& scene,const ff3qb::BallObservation& ball,double lead){
 if(floorLoose(scene,ball))return ball.position;
 for(const auto& flight:scene.balls)if(flight.id==ball.id)return ff3Play::ballAt(flight,lead);
 return ball.position+ball.velocity*lead+Vec{0,-.5*ball.gravity*lead*lead,0};
}
struct Escape {bool valid=false,hold=false;Vec point;std::vector<Vec> path;double clearance=0,score=-INFINITY;};
inline double pressure(Vec point,double time,const std::vector<Actor>& actors){double nearest=80;for(const auto& actor:actors)if(actor.opponent&&ff3qb::finite(actor.position)&&ff3qb::finite(actor.velocity)){const double uncertainty=std::max(0.,time-.65)*std::clamp(ff3qb::length(flat(actor.velocity)),22.,40.)*.5;nearest=std::min(nearest,distance(point,actor.position+flat(actor.velocity)*std::min(time,.65))-3-uncertainty);}return nearest;}
inline Escape escape(const ff3Play::Scene& s,const std::vector<Actor>& actors,double range,const ff3qb::FieldBounds* bounds=nullptr){
 Escape out;if(!ff3Play::observing(s,s.at)||!s.possessed||!std::isfinite(s.groundY)||!std::isfinite(s.speed)||s.speed<=1||s.speed>60||!std::isfinite(range)||range<6||range>40)return out;
 if(bounds&&(!bounds->valid()||!bounds->contains(s.head)))return out;
 bool threat=false;for(const auto& a:actors)if(a.opponent&&distance(a.position,s.head)<45)threat=true;if(!threat)return out;
 const double current=std::min(pressure(s.head,0,actors),pressure(s.head,.6,actors));out={true,current>12,s.head,{s.head},current,current};out.point.y=s.groundY;
 for(int direction=0;direction<32;++direction)for(double scale:{.5,.75,1.}){
  const double angle=direction*6.283185307179586/32,reach=range*scale;Vec end=s.head+Vec{std::cos(angle)*reach,0,std::sin(angle)*reach};end.y=s.groundY;
  if(bounds&&(!bounds->valid()||!bounds->contains(end,2)))continue;
  double nearest=80;bool safe=true;for(int i=1;i<=8;++i){double t=reach*i/8/std::max(1.,s.speed);auto p=s.head+(end-s.head)*(i/8.);if(bounds&&!bounds->contains(p,1)){safe=false;break;}nearest=std::min(nearest,pressure(p,t,actors));}
  if(!safe)continue;double exit=pressure(end,reach/std::max(1.,s.speed),actors),score=nearest*.7+exit*.3-reach*.025;
  if(score<=out.score+.1)continue;out.valid=true;out.hold=false;out.point=end;out.path={s.head,end};out.clearance=nearest;out.score=score;
 }
 return out;
}
inline Vec guardSpot(const ff3Play::Scene& s,const Actor& receiver,const std::vector<Actor>& actors,double cushion,double lead){
 Vec threat=s.head;double nearest=INFINITY;for(const auto& a:actors)if(a.opponent&&a.carrier&&distance(a.position,receiver.position)<nearest){nearest=distance(a.position,receiver.position);threat=a.position;}
 auto predicted=receiver.position+flat(receiver.velocity)*std::clamp(lead,0.,.6);return predicted+unit(flat(threat-predicted))*std::clamp(cushion,1.,15.);
}
inline Vec redirect(Vec velocity,Vec current,Vec target,double speed){auto direction=unit(flat(target-current));if(!ff3qb::finite(velocity)||!ff3qb::finite(target)||!ff3qb::finite(current)||ff3qb::length(direction)<.5||!std::isfinite(speed))return {NAN,NAN,NAN};return direction*std::clamp(speed,0.,65.)+Vec{0,velocity.y,0};}
}
