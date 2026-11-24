#pragma once
#include "ff3_smart_dive.hpp"
#include <deque>
namespace ff3Assist {
inline bool directionalBoostReady(double pressed,double time,int state,unsigned platform,Vec velocity,bool airborne=false){
 return pressed>0&&std::isfinite(pressed)&&std::isfinite(time)&&time>=pressed&&time-pressed<=.18&&(freePhysics(state,platform)||airborne&&(state==3||state==5))&&ff3qb::finite(velocity)&&ff3qb::length(flat(velocity))>=4;
}
inline bool directionalBoostContinues(int state,unsigned platform,bool airborne){return freePhysics(state,platform)||airborne&&(state==3||state==5);}
struct DirectionalBoost {
 bool active=false,stopping=false,smart=false;Vec heading,lastBase,extraVelocity;double nativeSpeed=0,budget=0,spent=0,appliedExtra=0,at=0,until=0,lastAt=0;
 bool tracked=false,coasting=false;Vec lastObserved,smartGoal;double lastObservedAt=0,measuredExtra=0,contactAt=0,clearance=0,lastTargetAt=0;std::uint64_t trackedBall=0;
 std::deque<std::pair<double,double>> recentExtra;
 bool begin(Vec velocity,double time,double extra,double contactIn=0){
  cancel();const double speed=ff3qb::length(flat(velocity));if(!ff3qb::finite(velocity)||!std::isfinite(time)||!std::isfinite(extra)||extra<.05||extra>30||speed<4||speed>=119||!std::isfinite(contactIn)||contactIn<0||contactIn>.65)return false;
  heading=unit(flat(velocity));nativeSpeed=speed;budget=extra;at=lastAt=time;smart=contactIn>0;until=time+(smart?contactIn:std::clamp(.025+extra/(120-speed),.32,.6));lastBase=heading*speed;active=true;return true;
 }
 Vec baseline(Vec current)const{auto value=heading*nativeSpeed;value.y=current.y;return value;}
 Vec direction(Vec current,bool assisted)const{auto value=assisted?unit(flat(current)):heading;if(ff3qb::length(value)<.5)value=heading;value=unit(flat(forwardDiveVelocity(value,heading)));return ff3qb::length(value)>.5?value:heading;}
 bool beginTracked(Vec velocity,Vec position,const DistanceDiveTarget& target,double time,double maximum){
  if(!target.valid||!target.catchable||!ff3qb::finite(position)||!ff3qb::finite(target.point)||!std::isfinite(target.forwardClearance)||target.forwardClearance<0||!std::isfinite(target.requiredExtra)||target.requiredExtra<0||target.requiredExtra>maximum||target.at<=time+.025||target.at>time+.65)return false;
  if(!begin(velocity,time,maximum,target.at-time))return false;
  tracked=true;lastObserved=position;lastObservedAt=lastTargetAt=time;smartGoal=target.point;contactAt=target.at;clearance=target.forwardClearance;trackedBall=target.ball;until=time+.6;return true;
 }
 bool retargetTracked(const DistanceDiveTarget& target,double time){
  if(!tracked||!active||!target.valid||target.ball!=trackedBall||!target.catchable||!ff3qb::finite(target.point)||!std::isfinite(target.forwardClearance)||target.forwardClearance<0||!std::isfinite(target.requiredExtra)||target.requiredExtra<0||target.requiredExtra>budget-spent+.05||target.at<=time+.025||target.at>until)return false;
  smartGoal=target.point;contactAt=target.at;clearance=target.forwardClearance;lastTargetAt=time;coasting=false;return true;
 }
 void missingTarget(double time){coasting=true;if(time-lastTargetAt>.04)brake();}
 VelocitySample sampleTracked(Vec position,Vec current,double time,bool assisted=false){
  if(!tracked||!active||!ff3qb::finite(position)||!ff3qb::finite(current)||!std::isfinite(time)||time<lastAt)return {};
  const double dt=time-lastAt;lastAt=time;
  if(distance(position,lastObserved)>.008){const double elapsed=time-lastObservedAt;measuredExtra+=std::max(0.,distance(position,lastObserved)-nativeSpeed*elapsed);lastObserved=position;lastObservedAt=time;}
  spent=measuredExtra;
  const auto axis=direction(current,assisted);const double gap=ff3qb::dot(flat(smartGoal-position),axis),remaining=contactAt-time;
  while(!recentExtra.empty()&&time-recentExtra.front().first>.08)recentExtra.pop_front();
  auto finish=[&](){auto value=flat(current);double own=appliedExtra;for(const auto& item:recentExtra)own=std::max(own,item.second);if(own>0){const double excess=std::max(0.,ff3qb::dot(value-axis*nativeSpeed,axis));value=value-axis*std::min(excess,own);}value.y=current.y;return VelocitySample{true,true,{},value};};
  if(stopping||time>=until||remaining<=.012||spent>=budget||gap<=0||time-lastObservedAt>.09)return finish();
  // Position feedback pays only for observed extra travel. Keep a reserve for
  // commands that may be in flight between physics samples; never spend the
  // same budget again because an interpolated/root snapshot has not changed.
  const double pending=appliedExtra*std::max(0.,time-lastObservedAt),available=std::max(0.,budget-spent-pending),latency=.02;
  const double observed=std::max(0.,ff3qb::dot(flat(current),axis)),usable=std::max(.012,remaining-latency);
  const double wanted=std::max(0.,(gap-observed*latency)/usable-nativeSpeed);
  const double maxSpeed=std::min(120.,std::sqrt(std::max(0.,180.*180.-current.y*current.y)));
  const double nextFrame=std::max(.025,std::min(.06,dt)+latency),roomSpeed=std::max(0.,(gap+std::max(0.,clearance))/nextFrame-nativeSpeed);
  const double extra=coasting?0:std::min({wanted,available/std::max(.025,remaining),available/nextFrame,std::max(0.,maxSpeed-nativeSpeed),roomSpeed});
  recentExtra.push_back({time,extra});if(recentExtra.size()>32)recentExtra.pop_front();lastBase=axis*nativeSpeed;extraVelocity=axis*extra;appliedExtra=extra;auto value=lastBase+extraVelocity;value.y=current.y;return {true,false,smartGoal,value};
 }
 VelocitySample sample(Vec current,double time,bool assisted=false){
  if(!active||!ff3qb::finite(current)||!std::isfinite(time)||time<lastAt)return {};
  const double dt=time-lastAt;spent+=appliedExtra*dt;lastAt=time;
  if(stopping||time>=until||spent>=budget){auto finish=flat(current);if(appliedExtra>0){const auto direction=unit(extraVelocity);const double excess=std::max(0.,ff3qb::dot(finish-lastBase,direction));finish=finish-direction*std::min(excess,appliedExtra);}finish.y=current.y;return {true,true,{},finish};}
  auto direction=assisted?unit(flat(current)):heading;if(ff3qb::length(direction)<.5)direction=heading;direction=unit(flat(forwardDiveVelocity(direction,heading)));if(ff3qb::length(direction)<.5)direction=heading;
  const double remaining=std::max(.008,until-time),available=std::max(0.,budget-spent),maxSpeed=std::min(120.,std::sqrt(std::max(0.,180.*180.-current.y*current.y)));
  double extra=std::min({available/remaining,available/std::max(.012,dt),std::max(0.,maxSpeed-nativeSpeed)});if(!smart)extra*=ease((time-at+.012)/.025);
  lastBase=direction*nativeSpeed;extraVelocity=direction*extra;appliedExtra=extra;auto value=lastBase+extraVelocity;value.y=current.y;return {true,false,{},value};
 }
 void brake(){stopping=true;}
 void cancel(){active=stopping=smart=tracked=coasting=false;heading=lastBase=extraVelocity=lastObserved=smartGoal={};nativeSpeed=budget=spent=appliedExtra=at=until=lastAt=lastObservedAt=measuredExtra=contactAt=clearance=lastTargetAt=0;trackedBall=0;recentExtra.clear();}
};
inline DistanceDiveTarget directionalSmartTarget(const ff3Play::Scene& scene,const ff3qb::BallObservation& ball,double time,double range,double maximum,Vec heading,double speed,const ContactReach& contact){
 DistanceDiveTarget result;if(!ff3Play::allowed(scene,time)||!ball.id||!ball.verifiedBall||!ff3qb::finite(ball.position)||!ff3qb::finite(ball.velocity)||!std::isfinite(ball.radius)||ball.radius<=0||!std::isfinite(maximum)||maximum<0||maximum>30||!std::isfinite(range)||range<1||range>150)return result;
 heading=unit(flat(heading));if(ff3qb::length(heading)<.5||speed<4||speed>=120)return result;
 const auto root=motionPosition(scene),offset=validContactReach(contact)?contact.offset:Vec{};
 const double radius=(validContactReach(contact)?contact.radius:1.25)+std::clamp(ball.radius,.1,1.)-.15,horizon=std::min(.6,diveAirWindow(scene));
 // Use the currently observed contact pose. Replanning during the dive
 // incorporates its motion without inventing a future raised-hand posture.
 for(double t=.06;t<=horizon;t+=.008){const auto point=divePoint(scene,ball,t,time);const double headY=scene.head.y+scene.velocity.y*t-.5*scene.worldGravity*t*t;
  {const double vertical=point.y-headY-offset.y;if(std::abs(vertical)>radius)continue;
   auto goal=point-(scene.head-root+offset);goal.y=root.y;const auto delta=flat(goal-root);const double along=ff3qb::dot(delta,heading),side=ff3qb::length(delta-heading*along),width=std::sqrt(std::max(0.,radius*radius-vertical*vertical));
   if(along<.25||ff3qb::length(delta)>range||side>width||along+width<speed*t)continue;
   const double forwardWidth=std::sqrt(std::max(0.,width*width-side*side)),destination=std::clamp(speed*t,along-forwardWidth*.45,along+forwardWidth*.45),needed=std::max(0.,destination-speed*t);
   if(needed>maximum+1e-6||needed>(120-speed)*(t-.02))continue;
   goal=root+heading*destination;result={true,ball.id,goal,time+t,t,ff3qb::length(delta),DiveTargetKind::Intercept};result.requiredExtra=needed;result.catchable=true;result.forwardClearance=std::max(0.,along+forwardWidth*.85-destination);return result;
  }
 }return result;
}
template<size_t N> inline DistanceDiveTarget selectSmartBoostTarget(const ff3Play::Scene& scene,bool practice,double time,double range,double maximum,Vec heading,double speed,const std::array<ContactReach,N>& contacts,std::uint64_t locked=0){
 DistanceDiveTarget best;double score=INFINITY;
 for(const auto& ball:scene.looseBalls){if(locked&&ball.id!=locked||!looseActionCandidate(scene,ball,practice)||floorLoose(scene,ball)&&ff3qb::length(ball.velocity)<6)continue;
  for(const auto& contact:contacts){if(!validContactReach(contact))continue;auto target=directionalSmartTarget(scene,ball,time,range,maximum,heading,speed,contact);if(!target.valid)continue;
   const double rank=target.eta+target.distance*.0005;if(rank<score){best=target;score=rank;}
  }
 }return best;
}
}
