#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include <string>
namespace ff3qb {
// The version-gated FF3 Frame0 visibility byte was observed as 2 when visible,
// 0 when hidden; it is a flag field, not a C++ bool equal to 1.
inline int visiblePower(unsigned enabled,unsigned flags,std::string text){
 if(enabled!=1||!(flags&2))return 0;
 auto first=text.find_first_not_of(" \r\n\t"),last=text.find_last_not_of(" \r\n\t");text=first==std::string::npos?"":text.substr(first,last-first+1);
 if(text.empty()||text.size()>2||text.find_first_not_of("0123456789")!=std::string::npos)return -1;
 int value=0;for(char c:text)value=value*10+(c-'0');return value>=1&&value<=95?value:-1;
}
inline bool cursorModeAllowed(bool visible,bool rightHeld,bool firstPerson){return visible&&!rightHeld&&!firstPerson;}
inline bool cameraStable(const float* a,const float* b){
 for(int row:{0,4}){
  double na=0,nb=0,dot=0;
  for(int i=0;i<3;i++){na+=double(a[row+i])*a[row+i];nb+=double(b[row+i])*b[row+i];dot+=double(a[row+i])*b[row+i];}
  if(!std::isfinite(na)||!std::isfinite(nb)||!std::isfinite(dot)||na<1e-8||nb<1e-8||dot/std::sqrt(na*nb)<.999961923)return false; // 0.5 degrees
 }return true;
}
struct Vec {double x=0,y=0,z=0;Vec operator+(Vec b)const{return{x+b.x,y+b.y,z+b.z};}Vec operator-(Vec b)const{return{x-b.x,y-b.y,z-b.z};}Vec operator*(double s)const{return{x*s,y*s,z*s};}};
inline double dot(Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline double length(Vec a){return std::sqrt(dot(a,a));}
inline bool finite(Vec a){return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z);}
struct Solution {bool valid=false;Vec origin,velocity,catchPoint;double time=0,angle=0;};
// Constant-velocity interception with measured gravity and the visible power.
// Both ballistic roots are solved numerically; unreachable throws fail closed.
inline Solution solve(Vec origin,Vec target,Vec movement,double speed,double gravity,double latency,bool lob=false){
 if(!finite(origin)||!finite(target)||!finite(movement)||!std::isfinite(speed)||speed<1||speed>150||!std::isfinite(gravity)||gravity<1||gravity>300||!std::isfinite(latency)||latency<0||latency>.3||length(movement)>40||length(target-origin)<3||length(target-origin)>600)return {};
 auto velocity=[&](double t){return (target+movement*(t+latency)-origin+Vec{0,.5*gravity*t*t,0})*(1/t);};
 auto error=[&](double t){auto v=velocity(t);return dot(v,v)-speed*speed;};
 std::vector<double> roots;double previous=.03,prior=error(previous);
 for(int i=1;i<=800;i++){
  double next=.03+i*.01,current=error(next);
  if((prior>0&&current<=0)||(prior<0&&current>=0)){
   double low=previous,high=next;const bool positive=prior>0;
   for(int j=0;j<32;j++){double mid=(low+high)*.5;if((error(mid)>0)==positive)low=mid;else high=mid;}
   roots.push_back((low+high)*.5);
  }previous=next;prior=current;
 }
 if(roots.empty())return {};
 double t=lob?roots.back():roots.front();auto v=velocity(t);
 if(t<.06||t>8||std::abs(length(v)-speed)>.01)return {};
 return {true,origin,v,target+movement*(t+latency),t,std::atan2(v.y,std::hypot(v.x,v.z))*180/3.141592653589793};
}
inline Vec point(const Solution& s,double gravity,double t){return s.origin+s.velocity*t+Vec{0,-.5*gravity*t*t,0};}
// Solve flight time at a requested elevation, then derive required power.
// No clamping of an unreachable power: that would change the landing point.
inline Solution solveAngle(Vec origin,Vec target,Vec movement,double angle,double gravity,double latency){
 if(!finite(origin)||!finite(target)||!finite(movement)||!std::isfinite(angle)||angle<5||angle>75||!std::isfinite(gravity)||gravity<1||gravity>300||!std::isfinite(latency)||latency<0||latency>.3||length(movement)>40||length(target-origin)<3||length(target-origin)>600)return {};
 const double tangent=std::tan(angle*3.141592653589793/180.);
 auto error=[&](double t){auto d=target+movement*(t+latency)-origin;return std::hypot(d.x,d.z)*tangent-d.y-.5*gravity*t*t;};
 double previous=.06,prior=error(previous);
 for(int i=1;i<=400;i++){
  double next=.06+i*.02,current=error(next);
  if(prior>=0&&current<=0){
   double lo=previous,hi=next;for(int j=0;j<32;j++){double mid=(lo+hi)*.5;if(error(mid)>0)lo=mid;else hi=mid;}
   double t=(lo+hi)*.5;Vec end=target+movement*(t+latency),v=(end-origin+Vec{0,.5*gravity*t*t,0})*(1/t);double power=length(v);
   if(t<=8&&power>=1&&power<=95)return {true,origin,v,end,t,angle};
  }previous=next;prior=current;
 }return {};
}
inline const char* presetName(int mode){static const char* names[]={"Auto route","Bullet","Lob","Mag","Slant","Touch"};return names[std::clamp(mode,0,5)];}
inline const char* routeName(Vec origin,Vec target,Vec movement){
 auto delta=target-origin;double distance=std::hypot(delta.x,delta.z),speed=std::hypot(movement.x,movement.z);
 if(speed<2)return "Stationary";
 double forward=(delta.x*movement.x+delta.z*movement.z)/std::max(1.,distance);
 if(forward<-3)return "Comeback";
 if(std::abs(forward)<speed*.65)return "Crossing";
 return distance>90?"Deep route":"Short route";
}
inline double presetAngle(int mode,Vec origin,Vec target,Vec movement){
 double distance=length(target-origin);
 if(mode==1)return 12;if(mode==2)return 55;if(mode==3)return 38;if(mode==4)return 18;if(mode==5)return 32;
 auto route=std::string(routeName(origin,target,movement));
 if(route=="Crossing"||route=="Comeback")return distance>100?30:18;
 return distance>120?48:distance>65?32:15;
}
struct Defender {Vec position,velocity;};
struct Plan {Solution solution;Vec offset;double power=0,margin=0;bool coverageKnown=false;};
// Reach estimates intentionally allow defenders to turn. Sample the catchable
// portion of the whole flight, not just separation at the final catch point.
inline double coverageMargin(const Solution& s,double gravity,const std::vector<Defender>& defenders,double latency){
 double margin=10;
 for(const auto& d:defenders)for(int i=1;i<=24;i++){
  double t=s.time*i/24.;auto p=point(s,gravity,t);
  if(std::abs(p.y-d.position.y)>7)continue;
  double distance=std::hypot(p.x-d.position.x,p.z-d.position.z);
  double speed=std::clamp(std::hypot(d.velocity.x,d.velocity.z)+3.,20.,32.);
  margin=std::min(margin,std::max(0.,distance-4.)/speed-t-latency);
 }return margin;
}
inline Plan planThrow(Vec origin,Vec target,Vec movement,double gravity,double latency,int mode,bool automaticAngle,double manualAngle,bool automaticPower,double gamePower,bool lob,bool fit,const std::vector<Defender>& defenders){
 Plan best;double bestScore=-1e9;
 const double desired=automaticAngle?presetAngle(mode,origin,target,movement):manualAngle;
 const Vec offsets[]={{},{3,0,0},{-3,0,0},{0,0,3},{0,0,-3},{4,0,4},{-4,0,4},{4,0,-4},{-4,0,-4}};
 for(int index=0;index<(fit&&!defenders.empty()?9:1);++index)for(int elevation=0;elevation<(fit&&automaticAngle&&automaticPower&&!defenders.empty()?3:1);++elevation){
  auto offset=offsets[index];Solution s;
  const double angle=std::clamp(desired+(elevation==1?10:elevation==2?-10:0),5.,75.);
  if(automaticPower){
   s=solveAngle(origin,target+offset,movement,angle,gravity,latency);
   // Automatic angle may trade elevation for reach; manual mode stays exact.
   if(!s.valid&&automaticAngle)for(double a:{25.,35.,45.,55.,65.}){auto candidate=solveAngle(origin,target+offset,movement,a,gravity,latency);if(candidate.valid&&(!s.valid||std::abs(candidate.angle-desired)<std::abs(s.angle-desired)))s=candidate;}
  }else s=solve(origin,target+offset,movement,gamePower,gravity,latency,lob||mode==2);
  if(!s.valid||length(offset)>std::min(6.,2.+s.time*3.))continue;
  const double margin=defenders.empty()?0:coverageMargin(s,gravity,defenders,latency);
  double score=(defenders.empty()?0:std::min(margin,1.5))*10-length(offset)*.12-s.time*.03-std::abs(s.angle-desired)*.012;
  if(score>bestScore){bestScore=score;best={s,offset,length(s.velocity),margin,!defenders.empty()};}
 }return best;
}
// Mouse distance to a projected body capsule, with a bounded retention bonus.
inline double selectionDistance(double x,double y,double footX,double footY,double headX,double headY,double radius,bool retained){
 double dx=headX-footX,dy=headY-footY,length2=dx*dx+dy*dy;
 double t=length2>1?std::clamp(((x-footX)*dx+(y-footY)*dy)/length2,0.,1.):0;
 return std::max(0.,std::hypot(x-footX-t*dx,y-footY-t*dy)-radius)-(retained?12.:0.);
}
struct Screen {double x=0,y=0;bool valid=false;};
inline Screen project(Vec p,const float* m,int width,int height){
 if(!finite(p)||width<100||height<100)return {};
 const double w=m[12]*p.x+m[13]*p.y+m[14]*p.z+m[15];if(!std::isfinite(w)||w<.01)return {};
 const double x=(m[0]*p.x+m[1]*p.y+m[2]*p.z+m[3])/w,y=(m[4]*p.x+m[5]*p.y+m[6]*p.z+m[7])/w;
 if(!std::isfinite(x)||!std::isfinite(y)||std::abs(x)>.98||std::abs(y)>.98)return {};
 return {(x+1)*width*.5,(1-y)*height*.5,true};
}
struct ClickGate {
 bool owned=false,pending=false;double at=0;
 bool down(bool eligible,double time){if(owned)return true;if(!eligible||!std::isfinite(time))return false;owned=pending=true;at=time;return true;}
 bool up(){bool result=owned;owned=false;return result;}
 bool take(double time,bool eligible){if(!pending)return false;pending=false;return eligible&&time>=at&&time-at<=.15;}
 void cancel(){pending=false;} // Swallow only the matching physical up after interception.
};
}
