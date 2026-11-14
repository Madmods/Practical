#pragma once
#include "shooting_logic.hpp"
#include "flight_logic.hpp"
#include <deque>
#include <cstdint>
namespace rolling {
using shooting::Vec;
inline float flatLength(Vec a){return sqrtf(a.x*a.x+a.z*a.z);}
struct Ball{Vec p{},v{};float deceleration=0,gravity=0,floor=0,radius=.75f,restitution=.55f;};
inline Vec at(const Ball&b,float t){
 float speed=flatLength(b.v),a=std::clamp(b.deceleration,0.f,12.f),horizontalTime=t;
 if(a>.01f)horizontalTime=std::min(horizontalTime,speed/a);float travel=std::max(0.f,speed*horizontalTime-.5f*a*horizontalTime*horizontalTime);
 Vec p=b.p;if(speed>.01f)p=p+Vec{b.v.x/speed,0,b.v.z/speed}*travel;
 if(b.gravity>1){float g=b.gravity,y=std::max(b.floor+b.radius,b.p.y),vy=b.v.y,remain=t;
  for(int bounce=0;bounce<12;bounce++){float height=std::max(0.f,y-b.floor-b.radius);float contact=(vy+sqrtf(vy*vy+2*g*height))/g;
   if(contact>remain){y+=vy*remain-.5f*g*remain*remain;remain=0;break;}
   remain-=std::max(0.f,contact);vy=std::max(0.f,g*contact-vy)*std::clamp(b.restitution,.1f,.85f);y=b.floor+b.radius;
   if(vy<1.5f||remain<=0)break;
  }p.y=std::max(b.floor+b.radius,y);
 }return p;
}
// Short motion samples bridge the fitting reset at a floor bounce.
struct BounceMotion {
 Vec p{},v{};double at=-1,bounceAt=-10,downAt=-10;float downSpeed=0,restitution=.55f;bool valid=false;
 bool update(double t,Vec position,float gravity,float floor,float radius){
  bool bounced=false;if(at<0||t-at>.25){p=position;at=t;valid=false;return false;}
  float dt=(float)(t-at);if(dt<.012f||shooting::length(position-p)<.008f){if(dt>.1f)valid=false;return false;}
  auto raw=(position-p)*(1/dt);if(shooting::length(raw)>180){p=position;at=t;valid=false;return false;}
  if(raw.y<-2){downSpeed=-raw.y;downAt=t;}
  if(raw.y>2&&t-downAt<.25&&position.y-floor-radius<1.5f&&t-bounceAt>.18){
   restitution=std::clamp((raw.y+.5f*gravity*dt)/std::max(3.f,downSpeed),.2f,.8f);bounceAt=t;bounced=true;downAt=-10;
  }
  v=raw+Vec{0,.5f*gravity*dt,0};p=position;at=t;valid=true;return bounced;
 }
};
// One ballistic fit per phase; never switch to noisy two-position velocities.
struct InterceptMotion {
 flight::Track air,flat;BounceMotion bounce;Ball ball;bool valid=false,waiting=false,grounded=false;
 double goodAt=-10,movedAt=-10,groundSince=-1,speedAt=-10;float lastSpeed=0,drag=0;Vec last{};bool started=false;
 void update(double t,Vec p,float gravity,float floor,float radius){
  if(!started||shooting::length(p-last)>.008f){movedAt=t;last=p;started=true;}
  bool impact=bounce.update(t,p,gravity,floor,radius);if(impact){air.reset();flat.reset();grounded=false;groundSince=-1;}
  auto flying=air.add(t,p,gravity,.4f),rolling=flat.add(t,p,0,.4f);float height=p.y-floor-radius;
  bool low=height<.3f&&height>=-.15f&&rolling.valid&&fabs(rolling.v.y)<1.5f;
  if(low){if(groundSince<0)groundSince=t;if(t-groundSince>.065)grounded=true;}else{groundSince=-1;if(height>.45f||(flying.at>0&&flying.v.y>2.5f))grounded=false;}
  valid=false;
  if(grounded&&rolling.valid&&t-rolling.at<.09){float speed=flatLength(rolling.v);if(t-speedAt>.06){if(t-speedAt<.25)drag=drag*.85f+std::clamp((lastSpeed-speed)/(float)(t-speedAt),0.f,8.f)*.15f;speedAt=t;lastSpeed=speed;}ball={p,{rolling.v.x,0,rolling.v.z},drag};valid=true;}
  else if(flying.at>0&&t-flying.at<.09&&height>=-.15f&&(height>.35f||shooting::length(flying.v)>.4f)){
   // Rebound factor stays consistent; observed post-impact flight refits directly.
   ball={p,flying.v,0,gravity,floor,radius,.55f};valid=true;
  }
  if(t-movedAt>.10&&height<.35f)valid=false;
  if(valid)goodAt=t;waiting=!valid&&t-goodAt<.085&&t-movedAt<.10;
 }
};
// This game can report Walkspeed=0 because it moves characters itself.
// Learn horizontal speed from distinct position updates, not duplicate frames.
struct MotionSpeed {
 Vec last{};double at=-1;std::deque<float> samples;
 float observe(Vec p,double t,float nominal){
  if(at<0){at=t;last=p;}else{double dt=t-at;float distance=flatLength(p-last);
   if(dt>.25){at=t;last=p;}else if(distance>.02f&&dt>.012){float speed=distance/(float)dt;if(speed>=2&&speed<=60){samples.push_back(speed);if(samples.size()>12)samples.pop_front();}last=p;at=t;}
  }
  if(std::isfinite(nominal)&&nominal>=1&&nominal<=80)return nominal;
  if(samples.size()>=3){std::vector<float> sorted(samples.begin(),samples.end());std::sort(sorted.begin(),sorted.end());return sorted[sorted.size()*3/4];}
  return 16; // provisional until movement has been observed
 }
};
struct Runner{Vec p{};float speed=16,catchHeight=4.5f;};
struct Intercept{bool valid=false;float time=0;Vec point{};};
inline Intercept earliest(const Ball&b,const Runner&r){
 if(!std::isfinite(flatLength(b.p))||!std::isfinite(flatLength(b.v))||!std::isfinite(flatLength(r.p))||!std::isfinite(r.speed)||r.speed<1)return {};
 auto reachable=[&](float t){auto q=at(b,t);return (b.gravity<=1||q.y<=b.floor+r.catchHeight)&&flatLength(q-r.p)<=r.speed*t+1.f;};
 if(reachable(0))return {true,0,b.p};
 for(int i=1;i<=240;i++){float hi=i*.025f;if(!reachable(hi))continue;float lo=hi-.025f;for(int k=0;k<12;k++){float mid=(hi+lo)*.5f;if(reachable(mid))hi=mid;else lo=mid;}return {true,hi,at(b,hi)};}
 return {};
}
struct Plan{bool valid=false;Ball ball;Vec start{},point{};float time=0,otherTime=-1;std::vector<Vec> ballPath;};
inline Plan make(const Ball&b,const Runner&own,const std::vector<Runner>&others){
 Plan p;auto hit=earliest(b,own);if(!hit.valid)return p;p.valid=true;p.ball=b;p.start=own.p;p.point=hit.point;p.time=hit.time;
 for(auto&r:others){auto q=earliest(b,r);if(q.valid&&(p.otherTime<0||q.time<p.otherTime))p.otherTime=q.time;}
 for(int i=0;i<=32;i++)p.ballPath.push_back(at(b,hit.time*i/32.f));return p;
}
// Stabilize only the world-space estimate, never camera projection or player pose.
struct PlanDisplay {
 Plan value;uint64_t target=0;double at=-1,validAt=-10;
 Plan update(uint64_t id,const Plan&raw,double t,Vec start){
  float dt=at<0?0:(float)std::clamp(t-at,0.,.05);at=t;
  if(!id||id!=target){value=raw;target=id;validAt=raw.valid?t:-10;}
  else if(raw.valid){
   validAt=t;if(!value.valid)value=raw;else{auto prior=value.point;float error=flatLength(raw.point-prior);float alpha=1-expf(-dt/.035f);value=raw;if(error<.07f){value.point.x=prior.x;value.point.z=prior.z;}else {value.point.x=prior.x+(raw.point.x-prior.x)*alpha;value.point.z=prior.z+(raw.point.z-prior.z)*alpha;}}
  }else if(t-validAt>.085)value={};
  if(value.valid)value.start=start;return value;
 }
};

}
