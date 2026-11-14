#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include <string>
namespace ff3qb {
// BallGui copies can remain enabled after unequip. Only current visible
// displays contribute manual game power; automatic ballistics need no GUI value.
struct PowerDisplay {unsigned enabled=0,flags=0;std::string text;};
inline int readPower(bool held,const std::vector<PowerDisplay>& displays){
 if(!held)return 0;
 int power=0;bool invalid=false;
 for(const auto& display:displays){
  if(display.enabled!=1||!(display.flags&2))continue;
  auto text=display.text;auto first=text.find_first_not_of(" \r\n\t"),last=text.find_last_not_of(" \r\n\t");text=first==std::string::npos?"":text.substr(first,last-first+1);
  if(text.empty()||text.size()>2||text.find_first_not_of("0123456789")!=std::string::npos){invalid=true;continue;}
  int value=0;for(char c:text)value=value*10+(c-'0');
  if(value<1||value>95){invalid=true;continue;}
  if(power&&power!=value)return -2;power=value;
 }
 return invalid?-1:power;
}
inline bool powerAvailable(bool held,bool automatic,int gamePower){return held&&(automatic||gamePower>0);}
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
inline Solution solve(Vec origin,Vec target,Vec movement,double speed,double gravity,double latency,bool lob=false,double spawn=0){
 if(!finite(origin)||!finite(target)||!finite(movement)||!std::isfinite(speed)||speed<1||speed>150||!std::isfinite(gravity)||gravity<1||gravity>300||!std::isfinite(latency)||latency<0||latency>.3||length(movement)>40||length(target-origin)<3||length(target-origin)>600)return {};
 if(!std::isfinite(spawn)||spawn<0||spawn>5)return {};
 auto displacement=[&](double t){return target+movement*(t+latency)-origin+Vec{0,.5*gravity*t*t,0};};
 auto velocity=[&](double t){return displacement(t)*(speed/(speed*t+spawn));};
 auto error=[&](double t){auto d=displacement(t);return dot(d,d)-(speed*t+spawn)*(speed*t+spawn);};
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
 return {true,origin+v*(spawn/speed),v,target+movement*(t+latency),t,std::atan2(v.y,std::hypot(v.x,v.z))*180/3.141592653589793};
}
inline Vec point(const Solution& s,double gravity,double t){return s.origin+s.velocity*t+Vec{0,-.5*gravity*t*t,0};}
// Solve flight time at a requested elevation, then derive required power.
// No clamping of an unreachable power: that would change the landing point.
inline Solution solveAngle(Vec origin,Vec target,Vec movement,double angle,double gravity,double latency,double spawn=0){
 if(!finite(origin)||!finite(target)||!finite(movement)||!std::isfinite(angle)||angle<5||angle>75||!std::isfinite(gravity)||gravity<1||gravity>300||!std::isfinite(latency)||latency<0||latency>.3||length(movement)>40||length(target-origin)<3||length(target-origin)>600)return {};
 const double tangent=std::tan(angle*3.141592653589793/180.);
 if(!std::isfinite(spawn)||spawn<0||spawn>5)return {};
 auto error=[&](double t){auto d=target+movement*(t+latency)-origin;return std::hypot(d.x,d.z)*tangent-d.y-.5*gravity*t*t;};
 double previous=.06,prior=error(previous);
 for(int i=1;i<=400;i++){
  double next=.06+i*.02,current=error(next);
  if(prior>=0&&current<=0){
   double lo=previous,hi=next;for(int j=0;j<32;j++){double mid=(lo+hi)*.5;if(error(mid)>0)lo=mid;else hi=mid;}
   double t=(lo+hi)*.5;Vec end=target+movement*(t+latency),d=end-origin+Vec{0,.5*gravity*t*t,0};double distance=length(d),power=(distance-spawn)/t;
   if(t<=8&&power>=1&&power<=95){auto direction=d*(1/distance);return {true,origin+direction*spawn,direction*power,end,t,angle};}
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
 if(mode==1)return 12;if(mode==2)return 55;if(mode==3)return 38;if(mode==4)return 18;if(mode==5)return 32;
 double d=length(target-origin);auto route=std::string(routeName(origin,target,movement));
 double angle=14+34*std::clamp((d-35)/135.,0.,1.);
 if(route=="Crossing"||route=="Comeback")angle=std::min(angle,18+12*std::clamp((d-60)/100.,0.,1.));
 return angle;
}
struct RouteHistory {
 struct Sample{double time;Vec position;};std::vector<Sample> samples;
 Vec axis{1,0,0},movement{};std::string route="Reading route",pending;double pendingAt=0;int mode=5;
 void reset(){*this={};}
 void update(double time,Vec origin,Vec position,Vec velocity){
  if(samples.empty()){axis=position-origin;axis.y=0;axis=axis*(1/std::max(1.,length(axis)));movement=velocity;}
  if(!samples.empty()&&time-samples.back().time<.04)return;
  if(!samples.empty()){double dt=time-samples.back().time;if(dt>.5){reset();update(time,origin,position,velocity);return;}auto measured=(position-samples.back().position)*(1/dt);measured.y=0;const bool stopped=length(velocity)<.5&&length(measured)<.5;if(length(measured)<=40)velocity=velocity*.5+measured*.5;movement=stopped?Vec{}:movement*.65+velocity*.35;}
  samples.push_back({time,position});while(samples.size()>2&&time-samples.front().time>1.2)samples.erase(samples.begin());
  if(samples.size()<4)return;
  double speed=std::hypot(movement.x,movement.z),forward=dot(movement,axis),side=std::abs(movement.x*axis.z-movement.z*axis.x),distance=length(position-origin);
  std::string next;int selected=5;
  if(speed<2){next="Stationary";selected=distance>120?2:1;}
  else if(forward<-3){next="Comeback / curl";selected=1;}
  else if(side>std::abs(forward)*1.7){next="Crossing / out";selected=4;}
  else if(side>speed*.35){next="Slant / post";selected=4;}
  else{next=distance>90?"Go / deep":"Vertical stem";selected=distance>100?2:5;}
  if(next!=pending){pending=next;pendingAt=time;}
  if(time-pendingAt>=.2){route=next;mode=selected;}
 }
 double angle(Vec origin,Vec target)const{
  double angle=14+34*std::clamp((length(target-origin)-35)/135.,0.,1.);
  if(route=="Comeback / curl")angle=std::min(angle,22.);
  if(route=="Crossing / out"||route=="Slant / post")angle=std::min(angle,28.);
  return angle;
 }
};
struct AngleTracker {
 bool initialized=false;double value=35,at=0;
 double update(double desired,double time){
  if(!initialized||time<at||time-at>.5){initialized=true;value=desired;}
  else{double delta=desired-value;if(std::abs(delta)>.35)value+=std::clamp(delta,-24*std::max(0.,time-at),24*std::max(0.,time-at));}
  at=time;return value;
 }
};
struct Defender {Vec position,velocity;double speed=22,jumpHeight=7,diveReach=12;};
struct Plan {Solution solution;Vec offset;double power=0,margin=0;bool coverageKnown=false,lob=false;};
// Reach estimates intentionally allow defenders to turn. Sample the catchable
// portion of the whole flight, not just separation at the final catch point.
inline double coverageMargin(const Solution& s,double gravity,const std::vector<Defender>& defenders,double latency){
 double margin=10;const int steps=std::clamp(int(std::ceil(s.time/.04)),24,200);
 for(const auto& d:defenders)for(int i=1;i<=steps;i++){
  double t=s.time*i/steps;auto p=point(s,gravity,t);
  // Conservative estimated envelope: a catcher can jump late in the flight
  // and redirect a dive. These are reach estimates, not guaranteed game limits.
  double rise=std::max(d.jumpHeight,std::max(0.,d.velocity.y*.2));
  if(p.y>d.position.y+4+rise||p.y<d.position.y-8)continue;
  double distance=std::hypot(p.x-d.position.x,p.z-d.position.z);
  double speed=std::clamp(std::max(d.speed,std::hypot(d.velocity.x,d.velocity.z)),16.,45.);
  double dive=std::min(d.diveReach,std::max(0.,t-.1)*45);
  margin=std::min(margin,std::max(0.,distance-4.-dive)/speed-t-latency);
 }return margin;
}
inline Plan planThrow(Vec origin,Vec target,Vec movement,double gravity,double latency,int mode,bool automaticAngle,double manualAngle,bool automaticPower,double gamePower,bool lob,bool fit,const std::vector<Defender>& defenders,double preferredAngle=NAN,Vec previousOffset={},bool powerFirst=false,double spawn=0){
 Plan best;double bestScore=-1e9;
 const bool fixedPower=automaticPower&&automaticAngle&&powerFirst;
 const bool stationary=length(movement)<.5;if(stationary)movement={};
 const double desired=std::isfinite(preferredAngle)?preferredAngle:automaticAngle?presetAngle(mode,origin,target,movement):manualAngle;
 double angle=std::clamp(desired,5.,75.);
 // Resolve reach once for the receiver. Fit candidates share this elevation,
 // so changing the preferred catch offset cannot switch the arc on each frame.
 if(automaticPower&&automaticAngle&&!fixedPower&&!solveAngle(origin,target,movement,angle,gravity,latency,spawn).valid){
  bool found=false;
  for(int step=1;step<=70&&!found;step++)for(int sign:{1,-1}){double candidate=desired+sign*step;if(candidate<5||candidate>75)continue;if(solveAngle(origin,target,movement,candidate,gravity,latency,spawn).valid){angle=candidate;found=true;break;}}
  if(!found)return best;
 }
 const Vec offsets[]={{},{3,0,0},{-3,0,0},{0,0,3},{0,0,-3},{4,0,4},{-4,0,4},{4,0,-4},{-4,0,-4},{9,0,0},{-9,0,0},{0,0,9},{0,0,-9}};
 for(int index=0;index<(fit&&!stationary&&!defenders.empty()?13:1);++index){
  auto offset=offsets[index];Solution s;
  if(fixedPower)s=solve(origin,target+offset,movement,95,gravity,latency,false,spawn);
  else if(automaticPower){
   s=solveAngle(origin,target+offset,movement,angle,gravity,latency,spawn);
  }else s=solve(origin,target+offset,movement,gamePower,gravity,latency,lob||mode==2,spawn);
  if(!s.valid||length(offset)>std::min(10.,4.+s.time*3.))continue;
  const double margin=defenders.empty()?0:coverageMargin(s,gravity,defenders,latency);
  double score=(defenders.empty()?0:std::min(margin,1.5))*10-length(offset)*.12-length(offset-previousOffset)*.15-s.time*.03-std::abs(s.angle-desired)*.012;
  if(score>bestScore){bestScore=score;best={s,offset,length(s.velocity),margin,!defenders.empty()};}
 }return best;
}
// Compare power, arc and reachable catch heights. An estimated opening always
// outranks a contested trajectory; when none exists, retain a contested result.
inline Plan planCoverageThrow(Vec origin,Vec target,Vec movement,double gravity,double latency,const std::vector<Defender>& defenders,bool fit,bool force95,double receiverJump,double worldGravity,Vec previousOffset={},double previousPower=0,bool previousLob=false){
 Plan best;double bestScore=-1e9;
 if(!std::isfinite(receiverJump)||receiverJump<0||receiverJump>40||!std::isfinite(worldGravity)||worldGravity<10||worldGravity>300)return best;
 const bool stationary=length(movement)<.5;if(stationary)movement={};
 const double jump=std::min(8.,receiverJump*.85);
 const Vec offsets[]={{},{3,0,0},{-3,0,0},{0,0,3},{0,0,-3},{4,0,4},{-4,0,4},{4,0,-4},{-4,0,-4}};
 for(double power:{95.,90.,85.,80.,75.,70.,65.,60.,55.,50.}){
  if(force95&&power!=95)continue;
  for(double height:{0.,jump*.65,jump}){
   // A 95-power pass explicitly asks for a jump catch, including stationary WRs.
   if(power==95&&height<3)continue;
   if(power!=95&&height>0&&(!fit||defenders.empty()))continue;
   for(int i=0;i<(fit&&!stationary&&!defenders.empty()?9:1);++i)for(bool lob:{false,true}){
    Vec offset=offsets[i]+Vec{0,height,0};auto s=solve(origin,target+offset,movement,power,gravity,latency,lob,5);
    if(!s.valid)continue;
    if(height>0){double riseTime=(std::sqrt(2*worldGravity*receiverJump)-std::sqrt(2*worldGravity*(receiverJump-height)))/worldGravity;if(s.time<.15+riseTime)continue;}
    const double margin=defenders.empty()?10:coverageMargin(s,gravity,defenders,latency);
    double score=(margin>.15?1000:0)+std::min(margin,.6)*20-s.time*.8-length(offsets[i])*.1-height*.06-length(offset-previousOffset)*.12;
    if(power==previousPower&&lob==previousLob&&length(offset-previousOffset)<1)score+=.5;
    if(score>bestScore){bestScore=score;best={s,offset,power,margin,!defenders.empty(),lob};}
   }
  }
 }
 return best;
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
 if(!std::isfinite(x)||!std::isfinite(y)||std::abs(x)>1.02||std::abs(y)>1.02)return {};
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
