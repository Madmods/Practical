#pragma once
#include "ff3_velocity_motion.hpp"
#include <condition_variable>
#include <thread>
namespace ff3Assist {
enum class InterceptionAction {Run,Jump,Dive,JumpDive,Reach};
struct InterceptionConfig {
 double reaction=.09,nativeDiveSpeed=43,boostDistance=0,handRadius=.7,handRaise=1.7,handForward=1.2;
 bool boost=false,smart=false,inDive=false,compare=true,contactMarker=true;
};
struct InterceptionPlan {
 bool valid=false,reachable=false,reliable=false,raceKnown=false,estimated=true;
 DWORD pid=0;std::uint64_t model=0,own=0,ball=0;InterceptionAction action=InterceptionAction::Run;
 Vec contact,body,moveTo,jumpAtPoint,diveAtPoint,heading,landing,desired;
 double plannedAt=0,selectedAt=0,contactAt=0,jumpAt=INFINITY,diveAt=INFINITY,actionAt=0,miss=INFINITY,margin=0,rivalAt=INFINITY,advantage=0,score=INFINITY,window=.08;
};
struct InterceptionInput {ff3Play::Scene scene;std::vector<Actor> players;InterceptionConfig config;double time=0;};
struct ReachMotion {Vec position,velocity;};
inline void accelerateReach(ReachMotion& m,Vec desired,double dt,double acceleration){
 if(dt<=0)return;const auto delta=desired-m.velocity;const double length=ff3qb::length(delta),ramp=std::min(dt,length/acceleration);const auto a=length>1e-7?delta*(acceleration/length):Vec{};
 m.position=m.position+m.velocity*ramp+a*(.5*ramp*ramp)+desired*(dt-ramp);m.velocity=ramp<dt?desired:m.velocity+a*ramp;
}
inline double standingRoot(const ff3Play::Scene& s){return std::isfinite(s.standingRootHeight)?std::clamp(s.standingRootHeight,1.5,5.):3.5;}
inline double standingHead(const ff3Play::Scene& s){return std::isfinite(s.standingHeadHeight)?std::clamp(s.standingHeadHeight,3.,8.):5.;}
inline Vec interceptionRoot(const ff3Play::Scene& s){return ff3qb::finite(s.rootPosition)?s.rootPosition:s.head-Vec{0,standingHead(s)-standingRoot(s),0};}
inline double reachLandingTime(const ff3Play::Scene& s){
 if(s.grounded)return 0;const double height=interceptionRoot(s).y-s.groundY-standingRoot(s);if(height<=0&&s.velocity.y<=0)return 0;
 const double d=s.velocity.y*s.velocity.y+2*s.worldGravity*height;return d>=0?std::max(0.,(s.velocity.y+std::sqrt(d))/s.worldGravity):0;
}
struct InterceptionSchedule {double jump=INFINITY,dive=INFINITY;};
inline std::pair<double,double> rootKinematics(const ff3Play::Scene& s,double t,double jump){
 const double floor=s.groundY+standingRoot(s),g=s.worldGravity;
 if(std::isfinite(jump)&&t>=jump){const double p=t-jump,y=floor+s.jumpVelocity*p-.5*g*p*p;return {std::max(floor,y),y>floor?s.jumpVelocity-g*p:0};}
 if(s.grounded)return {floor,0};const double y=interceptionRoot(s).y+s.velocity.y*t-.5*g*t*t;return {std::max(floor,y),y>floor?s.velocity.y-g*t:0};
}
inline double interceptionHeadY(const ff3Play::Scene& s,double t,const InterceptionSchedule& action,const InterceptionConfig& config){
 const double offset=standingHead(s)-standingRoot(s);
 if(std::isfinite(action.dive)&&t>=action.dive){const auto launch=rootKinematics(s,action.dive,action.jump);const double phase=t-action.dive;const double root=std::max(s.groundY+.9,launch.first+launch.second*phase-.5*s.worldGravity*phase*phase);return root+offset*(1-ease(phase/.12))+.25*ease(phase/.12);}
 if(config.inDive){const double root=std::max(s.groundY+.9,interceptionRoot(s).y+s.velocity.y*t-.5*s.worldGravity*t*t);return root+.3;}
 return rootKinematics(s,t,action.jump).first+offset;
}
inline ReachMotion interceptionMotion(const ff3Play::Scene& s,Vec desired,Vec target,double t,const InterceptionSchedule& action,const InterceptionConfig& c){
 ReachMotion m{flat(interceptionRoot(s)),flat(s.velocity)};const double wait=std::min(t,c.reaction);m.position=m.position+m.velocity*wait;
 if(c.inDive){m.position=flat(interceptionRoot(s))+flat(s.velocity)*t;return m;}
 const double end=std::min(t,action.dive),land=reachLandingTime(s);std::vector<double> edges{wait,end};
 for(double e:{land,action.jump})if(e>wait&&e<end)edges.push_back(e);std::sort(edges.begin(),edges.end());
 for(size_t i=1;i<edges.size();++i){const double mid=(edges[i-1]+edges[i])*.5;const bool airborne=(!s.grounded&&mid<land)||(std::isfinite(action.jump)&&mid>=action.jump);accelerateReach(m,desired,edges[i]-edges[i-1],airborne?30.:90.);}
 if(std::isfinite(action.dive)&&t>action.dive){auto direction=ff3qb::length(desired)>.1?unit(desired):unit(flat(target-m.position));if(ff3qb::length(direction)<.5)direction=unit(s.cameraForward);const double phase=t-action.dive,native=std::clamp(c.nativeDiveSpeed,24.,65.);double extra=0;
  if(c.boost&&c.boostDistance>0){if(c.smart)extra=std::clamp(ff3qb::dot(flat(target-m.position),direction)-native*phase,0.,std::min(c.boostDistance,(120-native)*phase));else{const double duration=std::clamp(.025+c.boostDistance/(120-native),.32,.6);extra=c.boostDistance*std::clamp(phase/duration,0.,1.);}}
  m.position=m.position+direction*(native*phase+extra);m.velocity=direction*(native+(phase>.001?extra/phase:0));
 }return m;
}
inline ReachMotion solveInterceptionMotion(const ff3Play::Scene& s,Vec target,double t,const InterceptionSchedule& schedule,const InterceptionConfig& c,Vec& desired){
 const auto origin=flat(interceptionRoot(s));desired=capped((flat(target)-origin-flat(s.velocity)*c.reaction)*(1/std::max(.04,t-c.reaction)),s.speed);
 ReachMotion result;
 for(int i=0;i<5;++i){result=interceptionMotion(s,desired,target,t,schedule,c);const auto error=flat(target)-result.position;if(ff3qb::length(error)<.04)break;
  const auto dx=(interceptionMotion(s,desired+Vec{.1,0,0},target,t,schedule,c).position-result.position)*10.,dz=(interceptionMotion(s,desired+Vec{0,0,.1},target,t,schedule,c).position-result.position)*10.;const double determinant=dx.x*dz.z-dx.z*dz.x;
  Vec delta=std::abs(determinant)>1e-7?Vec{(error.x*dz.z-error.z*dz.x)/determinant,0,(dx.x*error.z-dx.z*error.x)/determinant}:error*(1/std::max(.1,t));
  Vec best=desired;double gap=ff3qb::length(error);for(double scale:{1.,.5}){const auto next=capped(desired+capped(delta,std::max(1.,s.speed))*scale,s.speed);const double candidate=distance(interceptionMotion(s,next,target,t,schedule,c).position,target);if(candidate<gap){gap=candidate;best=next;}}
  if(distance(best,desired)<.001)break;desired=best;
 }return interceptionMotion(s,desired,target,t,schedule,c);
}
inline std::vector<InterceptionSchedule> interceptionSchedules(const ff3Play::Scene& s,double t,const InterceptionConfig& c){
 std::vector<InterceptionSchedule> out{{}};if(c.inDive)return out;
 const double earliest=reachLandingTime(s)+c.reaction,apex=s.jumpVelocity/s.worldGravity,flight=2*apex;
 if(s.jumpVelocity>=5)for(double phase:{.08,.14,.20,apex,.32,.40,.48,.56})if(phase<flight-.015&&t-phase>=earliest){out.push_back({t-phase,INFINITY});for(double divePhase:{.10,.22,.34,.44})if(phase>=divePhase+.04)out.push_back({t-phase,t-divePhase});}
 if(!s.grounded)for(double phase:{.08,.16,.26,.38})if(t-phase>=c.reaction&&t-phase<reachLandingTime(s))out.push_back({INFINITY,t-phase});return out;
}
inline bool interceptionFlight(const ff3Play::Scene& s,const ff3qb::BallFlight& b,double time){return b.id&&b.airborne&&!b.ended&&ff3qb::finite(b.position)&&ff3qb::finite(b.velocity)&&std::isfinite(b.gravity)&&b.gravity>=1&&b.gravity<=300&&time>=b.at&&time-b.at<.32&&!(b.thrower&&(b.thrower==s.player||b.thrower==s.own));}
inline ff3qb::BallFlight interceptAligned(const ff3qb::BallFlight& source,double time){
 auto b=source;const double age=std::clamp(time-source.at,0.,.32);b.position=ff3Play::ballAt(source,age);Vec v=source.velocity;if(source.model.valid)v=source.model.velocity+Vec{0,-source.gravity*(source.model.observedTime(source.position)-source.model.at),0};b.velocity=v+Vec{0,-source.gravity*age,0};b.model.valid=false;b.at=time;return b;
}
inline double rivalIntercept(const ff3Play::Scene& s,const ff3qb::BallFlight& ball,const std::vector<Actor>& players,double horizon){
 double best=INFINITY;std::vector<const Actor*> rivals;for(const auto& p:players)if(p.id&&p.id!=s.own&&ff3qb::finite(p.position)&&ff3qb::finite(p.velocity)&&p.speed>0&&p.speed<=60&&distance(p.position,ball.position)<130)rivals.push_back(&p);std::sort(rivals.begin(),rivals.end(),[&](const auto* a,const auto* b){return distance(a->position,ball.position)<distance(b->position,ball.position);});if(rivals.size()>10)rivals.resize(10);
 for(const auto* p:rivals)for(double t=.12;t<std::min(horizon,best);t+=.04){const auto point=ff3Play::ballAt(ball,t);if(point.y<s.groundY+.5)break;const double phase=std::min(std::max(0.,t-.09),p->jumpVelocity/s.worldGravity),rise=p->jumpVelocity*phase-.5*s.worldGravity*phase*phase,head=s.groundY+standingHead(s)+rise;
  if(point.y>head+2.7)continue;const double run=p->speed*std::max(0.,t-.09)+std::max(0.,ff3qb::dot(flat(p->velocity),unit(flat(point-p->position))))*std::min(t,.09);double reach=run+2.3;
  if(point.y<=s.groundY+standingRoot(s)+rise+1.6)reach+=std::max(0.,43-p->speed)*std::min(.35,std::max(0.,t-.16));if(distance(p->position,point)<=reach){best=t;break;}}
 return best;
}
inline InterceptionPlan buildInterception(const InterceptionInput& input,const InterceptionPlan* previous=nullptr){
 InterceptionPlan best;const auto& s=input.scene;const double time=input.time;auto config=input.config;
 if(!ff3Play::allowed(s,time)||!std::isfinite(s.groundY)||!ff3qb::finite(interceptionRoot(s))||s.speed<0||s.speed>60||!std::isfinite(s.jumpVelocity)||s.jumpVelocity<0||s.jumpVelocity>150)return best;
 std::vector<ff3qb::BallFlight> balls=s.balls;for(const auto& raw:s.looseBalls){if(!raw.verifiedBall||!raw.id||!ff3qb::finite(raw.position)||!ff3qb::finite(raw.velocity)||raw.position.y<=s.groundY+raw.radius+.2||std::any_of(balls.begin(),balls.end(),[&](const auto& b){return b.id==raw.id;}))continue;ff3qb::BallFlight b;b.id=raw.id;b.thrower=raw.thrower;b.airborne=true;b.at=s.at;b.position=raw.position;b.velocity=raw.velocity;b.gravity=raw.gravity;b.radius=raw.radius;balls.push_back(b);}
 const ff3qb::BallFlight* selected=nullptr;double nearest=250;for(const auto& b:balls)if(interceptionFlight(s,b,time)){const double gap=ff3qb::length(ff3Play::ballAt(b,time-b.at)-s.head);if(gap<nearest){nearest=gap;selected=&b;}}
 if(previous&&previous->valid&&selected)for(const auto& b:balls)if(b.id==previous->ball&&interceptionFlight(s,b,time)&&ff3qb::length(ff3Play::ballAt(b,time-b.at)-s.head)<=nearest+1){selected=&b;break;}
 if(!selected)return best;const auto ball=interceptAligned(*selected,time);if(!std::isfinite(ball.radius)||ball.radius<=0)return best;
 const double radius=std::clamp(config.handRadius,.4,1.2)+std::clamp(ball.radius,.1,1.)-.15,disc=ball.velocity.y*ball.velocity.y+2*ball.gravity*(ball.position.y-s.groundY-ball.radius),groundTime=disc>=0?(ball.velocity.y+std::sqrt(disc))/ball.gravity:0,horizon=std::min(3.5,groundTime);
 if(horizon<.1)return best;const double rival=config.compare?rivalIntercept(s,ball,input.players,horizon):INFINITY;
 auto evaluate=[&](double t,bool committed){if(t<.06||t>horizon)return;const auto point=ff3Play::ballAt(ball,t);for(const auto& schedule:interceptionSchedules(s,t,config)){
   const bool diving=config.inDive||std::isfinite(schedule.dive);const double head=interceptionHeadY(s,t,schedule,config),low=head-(diving?.2:1.),high=head+(diving?.3:config.handRaise),handY=std::clamp(point.y,low,high),vertical=std::abs(point.y-handY),forward=diving?config.handForward*2:config.handForward;
   const auto toward=unit(flat(point-interceptionRoot(s)));const auto target=flat(point)-toward*forward;Vec desired;const auto motion=solveInterceptionMotion(s,target,t,schedule,config,desired);const double horizontal=distance(motion.position,target),gap=std::hypot(horizontal,vertical),miss=gap-radius;const bool reachable=miss<=-.12;
   const auto action=std::isfinite(schedule.dive)?std::isfinite(schedule.jump)?InterceptionAction::JumpDive:InterceptionAction::Dive:std::isfinite(schedule.jump)?InterceptionAction::Jump:config.inDive||!s.grounded?InterceptionAction::Reach:InterceptionAction::Run;
   const double effort=action==InterceptionAction::JumpDive?.13:action==InterceptionAction::Dive?.09:action==InterceptionAction::Jump?.04:0,contested=std::isfinite(rival)&&t>=rival-.06?1.2:0;
   double score=(reachable?-1000:100+std::max(0.,miss)*12)+t*4+effort+contested-std::min(.5,-miss)*.3;
   if(committed&&previous&&previous->action==action)score-=.1;
   if(best.valid&&score>=best.score)continue;best={};best.valid=true;best.pid=s.pid;best.model=s.model;best.own=s.own;best.ball=ball.id;best.action=action;best.reachable=reachable;best.contact=point;best.body={motion.position.x,rootKinematics(s,t,schedule.jump).first,motion.position.z};best.desired=desired;best.heading=ff3qb::length(desired)>.5?unit(desired):toward;
   best.plannedAt=time;best.selectedAt=committed&&previous?previous->selectedAt:time;best.contactAt=time+t;best.jumpAt=std::isfinite(schedule.jump)?time+schedule.jump:INFINITY;best.diveAt=std::isfinite(schedule.dive)?time+schedule.dive:INFINITY;best.actionAt=std::isfinite(best.jumpAt)?best.jumpAt:std::isfinite(best.diveAt)?best.diveAt:best.contactAt;
   best.moveTo=motion.position;best.moveTo.y=s.groundY;best.jumpAtPoint=best.diveAtPoint=best.moveTo;
   if(std::isfinite(schedule.jump)){best.jumpAtPoint=interceptionMotion(s,desired,target,schedule.jump,schedule,config).position;best.jumpAtPoint.y=s.groundY;best.moveTo=best.jumpAtPoint;}
   if(std::isfinite(schedule.dive)){best.diveAtPoint=interceptionMotion(s,desired,target,schedule.dive,schedule,config).position;best.diveAtPoint.y=s.groundY;if(!std::isfinite(schedule.jump))best.moveTo=best.diveAtPoint;}
   best.miss=std::max(0.,miss);best.margin=radius-gap;best.score=score;best.reliable=selected->model.valid?selected->model.error<.4:selected->samples.size()>=4;best.raceKnown=std::isfinite(rival);best.rivalAt=time+rival;best.advantage=rival-t;best.landing=ff3Play::ballAt(ball,groundTime);best.window=std::clamp(.04+std::max(0.,best.margin)/std::max(10.,ff3qb::length(ball.velocity)),.04,.14);
  }};
 // Evaluate a stable absolute contact first. Its epoch is retained while
 // reachable; newly computed contacts must improve materially to replace it.
 InterceptionPlan held;if(previous&&previous->valid&&previous->ball==ball.id&&previous->contactAt>time+.08){config.reaction=std::max(0.,input.config.reaction-(time-previous->selectedAt));evaluate(previous->contactAt-time,true);held=best;best={};}
 config=input.config;for(double t=.12;t<=horizon;t+=.025){evaluate(t,false);if(best.valid&&best.reachable&&t>best.contactAt-time+.1)break;}
 if(held.valid&&held.reachable&&(!best.valid||!best.reachable||best.contactAt>=held.contactAt-.14))return held;return best;
}
class InterceptionService {
 std::mutex mutex_;std::condition_variable ready_;std::thread worker_;bool stopping_=false,pending_=false;unsigned generation_=0;InterceptionInput input_;InterceptionPlan output_;
 void run(){for(;;){InterceptionInput in;InterceptionPlan prior;unsigned generation;{std::unique_lock<std::mutex> lock(mutex_);ready_.wait(lock,[&]{return stopping_||pending_;});if(stopping_)return;in=std::move(input_);pending_=false;prior=output_;generation=generation_;}auto plan=buildInterception(in,prior.valid?&prior:nullptr);{std::lock_guard<std::mutex> lock(mutex_);if(generation==generation_)output_=std::move(plan);}}}
public:
 ~InterceptionService(){ {std::lock_guard<std::mutex> lock(mutex_);stopping_=true;ready_.notify_all();}if(worker_.joinable())worker_.join();}
 void submit(InterceptionInput input){std::lock_guard<std::mutex> lock(mutex_);if(!worker_.joinable())worker_=std::thread([this]{run();});input_=std::move(input);pending_=true;ready_.notify_one();}
 InterceptionPlan snapshot(){std::lock_guard<std::mutex> lock(mutex_);return output_;}
 void clear(){std::lock_guard<std::mutex> lock(mutex_);++generation_;pending_=false;output_={};}
};
}
