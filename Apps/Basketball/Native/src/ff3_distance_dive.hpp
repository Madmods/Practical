#pragma once
#include "ff3_velocity_motion.hpp"
#include "ff3_contact_reach.hpp"
namespace ff3Assist {
enum class DiveTargetKind {Intercept,Landing,Loose,Approach};
struct DistanceDiveTarget {bool valid=false;std::uint64_t ball=0;Vec point;double at=0,eta=0,distance=0;DiveTargetKind kind=DiveTargetKind::Intercept;double requiredExtra=0;bool catchable=false;double forwardClearance=0;};
inline Vec forwardDiveVelocity(Vec value,Vec forward){
 forward=unit(flat(forward));if(ff3qb::length(forward)<.5)return value;
 const auto horizontal=flat(value);const double along=ff3qb::dot(horizontal,forward);if(along>=0)return value;
 const auto side=horizontal-forward*along;const double magnitude=ff3qb::length(side);const auto safe=magnitude>.01?side*(ff3qb::length(horizontal)/magnitude):Vec{};return {safe.x,value.y,safe.z};
}
inline bool retainDiveTarget(const DistanceDiveTarget& target,double seenAt,double time){return target.valid&&std::isfinite(seenAt)&&std::isfinite(time)&&time>=seenAt&&time-seenAt<=.1&&target.at>time+.02;}
inline Vec divePoint(const ff3Play::Scene& s,const ff3qb::BallObservation& ball,double elapsed,double time){
 for(const auto& flight:s.balls)if(flight.id==ball.id&&incoming(s,flight,time))return ff3Play::ballAt(flight,std::max(0.,time-flight.at)+elapsed);
 return looseTarget(s,ball,elapsed);
}
inline double diveLandingTime(const ff3Play::Scene& s,const ff3qb::BallObservation& ball,double time){
 const auto p=divePoint(s,ball,0,time),v=(divePoint(s,ball,.01,time)-p)*100.+Vec{0,.005*ball.gravity,0};
 if(!ff3qb::finite(p)||!ff3qb::finite(v)||!std::isfinite(s.groundY)||ball.gravity<1||ball.gravity>300)return NAN;
 const double height=p.y-(s.groundY+(std::isfinite(ball.radius)?ball.radius:.6));if(height<=.25&&std::abs(v.y)<4)return 0;
 const double d=v.y*v.y+2*ball.gravity*height;if(d<0)return NAN;const double t=(v.y+std::sqrt(d))/ball.gravity;return t>=0&&t<=5?t:NAN;
}
inline DistanceDiveTarget distanceDiveTarget(const ff3Play::Scene& s,const ff3qb::BallObservation& ball,double time,double range,double extraDistance,double naturalSpeed,const ContactReach& contact={},double horizon=.5,bool strict=false,Vec forward={},bool steering=true){
 DistanceDiveTarget out;if(!ff3Play::allowed(s,time)||!ball.id||!ball.verifiedBall||(ball.anchored&&!floorLoose(s,ball)&&!movingScriptBall(s,ball,time))||!ff3qb::finite(ball.position)||!ff3qb::finite(ball.velocity)||!std::isfinite(range)||range<1||range>150||!std::isfinite(extraDistance)||extraDistance<0||extraDistance>30||!std::isfinite(naturalSpeed)||naturalSpeed<0||naturalSpeed>120||!std::isfinite(horizon)||horizon<.04||horizon>.65||!std::isfinite(s.groundY))return out;
 const Vec root=motionPosition(s);const bool measured=validContactReach(contact);const Vec offset=measured?contact.offset:Vec{},rootOffset=s.head-root+offset;
 const double radius=measured?contact.radius:1.25;
 const double floor=s.groundY+std::min(s.head.y-s.groundY,std::isfinite(s.standingHeadHeight)?s.standingHeadHeight:5.);
 const double landed=diveLandingTime(s,ball,time);
 for(double t=.06;t<=horizon;t+=.01){if(std::isfinite(landed)&&landed>0&&t>landed)break;const auto point=divePoint(s,ball,t,time);if(!ff3qb::finite(point))continue;
  const double predicted=s.head.y+s.velocity.y*t-.5*s.worldGravity*t*t,headY=s.grounded?std::max(floor,predicted):predicted,vertical=point.y-(headY+offset.y);
  // An interception needs vertical overlap, not just a nearby landing marker.
  if(std::abs(vertical)>radius-.15)continue;
  Vec destination=point-rootOffset;destination.y=root.y;const double gap=distance(root,destination);
  if(ff3qb::length(forward)>.5&&ff3qb::dot(flat(destination-root),forward)<.25)continue;
  const double lateralReach=std::sqrt(std::max(0.,radius*radius-vertical*vertical))*.6;
  const double turnLoss=naturalSpeed*.025*(1-ff3qb::dot(unit(flat(s.velocity)),unit(flat(destination-root))));
  const auto correction=flat(destination-root)-flat(s.velocity)*t;
  const double needed=strict?(steering?std::max(0.,gap+turnLoss-naturalSpeed*t):ff3qb::length(correction)):std::max(0.,gap+turnLoss-lateralReach-naturalSpeed*t),available=std::min(extraDistance,std::max(0.,120-naturalSpeed)*std::max(0.,t-.02));
  if(strict&&needed>extraDistance+1e-6)continue;
  if(strict&&steering&&gap+radius<naturalSpeed*t)continue;
  if(strict&&!steering&&ff3qb::length(flat(s.velocity)+correction*(1/t))>120)continue;
  if(gap>range||needed>available+.05)continue;
  out={true,ball.id,destination,time+t,t,gap,DiveTargetKind::Intercept};out.requiredExtra=needed;out.catchable=true;return out;
 }
 if(strict)return out;
 // A landing/loose-ball target is a direction aid, not a claimed catch window.
 // Steering need not wait until a strict hand-height interception is possible.
 // A near-term pass position supplies direction while the native dive unfolds.
 const double lead=std::clamp(distance(root,ball.position)/std::max(20.,naturalSpeed+extraDistance/.3),std::min(.06,horizon),std::min(.3,horizon));
 const auto nearPoint=divePoint(s,ball,lead,time);Vec approach=nearPoint-rootOffset;approach.y=root.y;const double approachGap=distance(root,approach);
 if(approachGap>=.25&&approachGap<=range&&(!std::isfinite(landed)||landed>horizon))return {true,ball.id,approach,time+lead,lead,approachGap,DiveTargetKind::Approach};
 if(!std::isfinite(landed))return out;const bool loose=landed==0;
 const auto point=divePoint(s,ball,landed,time);Vec destination=point-rootOffset;destination.y=root.y;const double gap=distance(root,destination);
 if(gap>range||gap<.25){if(approachGap>=.25&&approachGap<=range)return {true,ball.id,approach,time+lead,lead,approachGap,DiveTargetKind::Approach};return out;}
 const double eta=loose?std::clamp(gap/std::max(12.,naturalSpeed+extraDistance/.35),.04,horizon):std::clamp(landed,.04,horizon);
 out={true,ball.id,destination,time+eta,eta,gap,loose?DiveTargetKind::Loose:DiveTargetKind::Landing};return out;
}
// One controller composes independent steering and extra travel. It never writes
// position, invents vertical lift, or acquires Humanoid controller flags.
struct DistanceDive {
 bool active=false,steer=false,boost=false,stopping=false,smart=false;Vec origin,goal,heading,lastPosition,baseHeading,lastBase,extraVelocity;double at=0,until=0,lastAt=0,naturalSpeed=0,budget=0,spent=0,appliedExtra=0,range=0;std::uint64_t ball=0;DiveTargetKind kind=DiveTargetKind::Intercept;
 bool begin(Vec position,Vec velocity,const DistanceDiveTarget& target,double time,bool steering,bool boosting,double extra,double limit,bool automatic=false,double airWindow=0){
  cancel();if(!target.valid||!target.ball||!ff3qb::finite(position)||!ff3qb::finite(velocity)||!ff3qb::finite(target.point)||!std::isfinite(time)||!std::isfinite(target.at)||!std::isfinite(extra)||!std::isfinite(limit)||!std::isfinite(airWindow)||airWindow<0||airWindow>.65||target.at<=time||target.at-time>.65||(!steering&&!boosting)||extra<0||extra>30||limit<1||limit>150||distance(position,target.point)>limit||ff3qb::length(flat(velocity))>120||automatic&&(!target.catchable||!std::isfinite(target.requiredExtra)||target.requiredExtra>extra+1e-6))return false;
  if(steering&&ff3qb::dot(flat(target.point-position),flat(velocity))<-.001)return false;
  origin=lastPosition=position;goal=target.point;heading=unit(flat(goal-position));naturalSpeed=ff3qb::length(flat(velocity));baseHeading=unit(flat(velocity));lastBase=flat(velocity);at=lastAt=time;smart=automatic;until=time+std::clamp(boosting&&!automatic&&airWindow>0?airWindow:target.at-time+.08,.12,.6);budget=boosting?extra:0;range=limit;ball=target.ball;kind=target.kind;steer=steering;boost=boosting;active=true;return true;
 }
 bool retarget(const DistanceDiveTarget& target,Vec position,double time){if(!active||!target.valid||target.ball!=ball||!ff3qb::finite(target.point)||distance(position,target.point)>range||steer&&ff3qb::dot(flat(target.point-position),baseHeading)<-.001){stopping=true;return false;}goal=target.point;kind=target.kind;return true;}
 VelocitySample sample(Vec position,Vec current,double time,const DistanceDiveTarget& target){
  if(!active||!ff3qb::finite(position)||!ff3qb::finite(current)||!std::isfinite(time)||time<lastAt)return {};
  const double dt=std::clamp(time-lastAt,0.,.06),elapsed=time-at;const auto currentFlat=flat(current);const double observed=ff3qb::length(currentFlat);
  // Position snapshots arrive on physics ticks, not on our polling ticks.
  // Charging their per-poll residual counted the same native dive motion as
  // boost repeatedly. Integrate only the extra velocity actually commanded.
  if(dt>0){spent+=appliedExtra*(time-lastAt);lastAt=time;lastPosition=position;}
  const auto toward=unit(flat(goal-position));const double gap=distance(position,goal);
  const bool passed=ff3qb::dot(flat(goal-position),heading)<=.15&&ff3qb::dot(flat(position-origin),heading)>1;
  if(stopping||elapsed>=until-at||gap<.8||passed||!target.valid||target.ball!=ball){
   // Remove only our remaining speed contribution; preserve ordinary dive speed.
   Vec finish=currentFlat;if(appliedExtra>0){const auto direction=unit(extraVelocity);const double excess=std::max(0.,ff3qb::dot(currentFlat-lastBase,direction));finish=finish-direction*std::min(appliedExtra,excess);}finish.y=current.y;return {true,true,goal,steer?forwardDiveVelocity(finish,baseHeading):finish};
  }
  if(elapsed<.08)naturalSpeed=std::min(120.,std::max(naturalSpeed,ff3qb::length(currentFlat-extraVelocity)));
  const double remaining=std::clamp((smart?std::min(until,target.at):until)-time,smart?.008:.06,.6),available=std::max(0.,budget-spent);
  const double along=steer?naturalSpeed:ff3qb::dot(baseHeading*naturalSpeed,toward);
  // Spend the selected extra travel proactively while there is stopping room,
  // rather than silently returning zero whenever native speed could arrive.
  double extra=boost?std::min({available/remaining,available/(smart?std::max(.008,dt):.06),std::max(0.,(gap-.8)/(smart?std::max(.008,remaining):.08)-std::max(0.,along)),std::max(0.,120-naturalSpeed)}):0;extra*=smart?1:ease((elapsed+.008)/.018);
  // Boost-only accelerates along the chosen ball direction. Assist-only keeps
  // observed speed: it cannot quietly add a boost of its own.
  const double base=boost?naturalSpeed:observed;const double speed=std::min(120.,base+extra);
  const double blend=ease((elapsed+.008)/.018);Vec value;
  if(steer){auto direction=unit(unit(currentFlat)*(1-blend)+toward*blend);if(ff3qb::length(direction)<.1)direction=toward;lastBase=direction*base;value=direction*speed;}
  else {lastBase=baseHeading*base;const auto correction=smart?unit(flat(goal-position)-lastBase*remaining):toward;value=capped(lastBase+correction*extra,120.);}
  if(steer)value=forwardDiveVelocity(value,baseHeading);if(!boost)lastBase=value;extraVelocity=value-lastBase;appliedExtra=ff3qb::length(extraVelocity);value.y=current.y;return {true,false,goal,value};
 }
 void brake(){stopping=true;}
 void cancel(){active=steer=boost=stopping=smart=false;at=until=lastAt=naturalSpeed=budget=spent=appliedExtra=range=0;extraVelocity=lastBase=baseHeading={};ball=0;}
};
}
