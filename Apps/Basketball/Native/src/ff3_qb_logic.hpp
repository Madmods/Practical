#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include <string>
namespace ff3qb {
struct ResumeIntent {
 bool pending=false;std::uint32_t pid=0;
 void suspend(bool enabled,std::uint32_t owner){if(enabled){pending=true;pid=owner;}}
 bool take(std::uint32_t owner){bool resume=pending&&pid==owner;pending=false;return resume;}
 void cancel(){pending=false;}
};
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
inline Solution solve(Vec origin,Vec target,Vec movement,double speed,double gravity,double latency,bool lob=false,double spawn=0,Solution* paired=nullptr){
 if(paired)*paired={};
 if(!finite(origin)||!finite(target)||!finite(movement)||!std::isfinite(speed)||speed<1||speed>150||!std::isfinite(gravity)||gravity<1||gravity>300||!std::isfinite(latency)||latency<0||latency>.3||length(movement)>45||length(target-origin)<3||length(target-origin)>600)return {};
 if(!std::isfinite(spawn)||spawn<0||spawn>5)return {};
 // Beyond this time even a vertical launch at full speed cannot reach the
 // target's height. Bound the root scan, especially for low-power Beam tiers.
 const double verticalBudget=speed-movement.y;
 const double verticalDiscriminant=verticalBudget*verticalBudget-2*gravity*(target.y-origin.y+movement.y*latency-spawn);
 if(verticalDiscriminant<0)return {};
 const double latest=std::min(8.03,(verticalBudget+std::sqrt(verticalDiscriminant))/gravity);
 if(latest<.03)return {};
 auto displacement=[&](double t){return target+movement*(t+latency)-origin+Vec{0,.5*gravity*t*t,0};};
 auto velocity=[&](double t){return displacement(t)*(speed/(speed*t+spawn));};
 auto error=[&](double t){auto d=displacement(t);return dot(d,d)-(speed*t+spawn)*(speed*t+spawn);};
 auto slope=[&](double t){return 2*dot(displacement(t),movement+Vec{0,gravity*t,0})-2*speed*(speed*t+spawn);};
 std::vector<double> roots;
 auto bracket=[&](double low,double high,double a,double b){
  if(!((a>0&&b<=0)||(a<0&&b>=0)))return;
  const bool positive=a>0;
  for(int j=0;j<44;j++){double mid=(low+high)*.5;if((error(mid)>0)==positive)low=mid;else high=mid;}
  double root=(low+high)*.5;if(roots.empty()||root-roots.back()>1e-6)roots.push_back(root);
 };
 // The flight equation is quartic. Split its cubic derivative at the roots
 // of its quadratic derivative, then bracket every extremum exactly. This
 // retains tangent/closely spaced roots without an 800-step time sweep.
 std::vector<double> slopeKnots{.03,latest},knots{.03,latest};
 const double a=3*gravity*gravity,b=6*gravity*movement.y;
 const double c=2*(dot(movement,movement)+gravity*(target.y-origin.y+movement.y*latency)-speed*speed);
 const double disc=b*b-4*a*c;
 if(disc>=0)for(double t:{(-b-std::sqrt(disc))/(2*a),(-b+std::sqrt(disc))/(2*a)})if(t>.03&&t<latest)slopeKnots.push_back(t);
 std::sort(slopeKnots.begin(),slopeKnots.end());
 for(size_t i=1;i<slopeKnots.size();++i){double lo=slopeKnots[i-1],hi=slopeKnots[i],left=slope(lo),right=slope(hi);
  if(std::abs(left)<1e-9)knots.push_back(lo);
  if(std::abs(right)<1e-9)knots.push_back(hi);
  if(left*right<0){bool positive=left>0;for(int j=0;j<44;++j){double mid=(lo+hi)*.5;if((slope(mid)>0)==positive)lo=mid;else hi=mid;}knots.push_back((lo+hi)*.5);}
 }
 std::sort(knots.begin(),knots.end());
 for(size_t i=0;i<knots.size();++i){double t=knots[i],value=error(t);
  if(i)bracket(knots[i-1],t,error(knots[i-1]),value);
  if(std::abs(value)<1e-7&&(roots.empty()||t-roots.back()>1e-6))roots.push_back(t);
 }
 if(roots.empty())return {};
 auto make=[&](double t)->Solution{auto v=velocity(t);if(t<.06||t>8||std::abs(length(v)-speed)>.01)return {};return {true,origin+v*(spawn/speed),v,target+movement*(t+latency),t,std::atan2(v.y,std::hypot(v.x,v.z))*180/3.141592653589793};};
 if(paired)*paired=make(lob?roots.front():roots.back());
 return make(lob?roots.back():roots.front());
}
inline Vec point(const Solution& s,double gravity,double t){return s.origin+s.velocity*t+Vec{0,-.5*gravity*t*t,0};}
// Solve flight time at a requested elevation, then derive required power.
// No clamping of an unreachable power: that would change the landing point.
inline Solution solveAngle(Vec origin,Vec target,Vec movement,double angle,double gravity,double latency,double spawn=0){
 if(!finite(origin)||!finite(target)||!finite(movement)||!std::isfinite(angle)||angle<-30||angle>85||!std::isfinite(gravity)||gravity<1||gravity>300||!std::isfinite(latency)||latency<0||latency>.3||length(movement)>45||length(target-origin)<3||length(target-origin)>600)return {};
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
  if(samples.empty()){axis=position-origin;axis.y=0;axis=axis*(1/std::max(1.,length(axis)));movement=length(velocity)<1.5?Vec{}:velocity;}
  if(!samples.empty()&&time-samples.back().time<.04)return;
  if(!samples.empty()){double dt=time-samples.back().time;if(dt>.5){reset();update(time,origin,position,velocity);return;}auto measured=(position-samples.back().position)*(1/dt);measured.y=0;const bool stopped=length(velocity)<1.5&&length(measured)<1.5;if(length(measured)<=45)velocity=velocity*.5+measured*.5;double blend=length(velocity-movement)>8?.7:.4;movement=stopped?Vec{}:movement*(1-blend)+velocity*blend;}
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
struct Defender {Vec position,velocity;double speed=22,jumpHeight=7,diveReach=12;double radius=6,above=6,below=8,worldGravity=196.2;bool redirect=false;};
// A temporary zero WalkSpeed does not prove future immobility.
inline double movementPotential(double walk,Vec velocity,double previous=22){
 double speed=std::max(22.,std::isfinite(previous)?previous:22.);
 if(std::isfinite(walk)&&walk>=0&&walk<=45)speed=std::max(speed,walk);
 if(finite(velocity))speed=std::max(speed,std::hypot(velocity.x,velocity.z));
 return std::clamp(speed,22.,45.);
}
struct CatchGeometry {double radius=6,above=6,below=8;bool measured=false;};
inline CatchGeometry catchGeometry(Vec torso,Vec head,Vec arm,Vec catcher,double ballRadius){
 auto valid=[](Vec v){return finite(v)&&v.x>.1&&v.y>.1&&v.z>.1&&v.x<8&&v.y<8&&v.z<8;};
 if(!valid(torso)||!valid(head)||!valid(arm)||!valid(catcher)||!std::isfinite(ballRadius)||ballRadius<=0||ballRadius>3)return {};
 // A rotation-independent bound around the shoulder, derived from the arm
 // and attached catch part. Use the same dimensions for receiver and DB.
 double reach=std::hypot(arm.y+catcher.y,.5*std::hypot(catcher.x,catcher.z))+ballRadius;
 CatchGeometry g{torso.x*.5+arm.x*.5+reach,std::max(0.,reach-head.y*.5),reach+head.y*.5,true};
 if(g.radius>12||g.above>12||g.below>12)return {};return g;
}
struct Plan {Solution solution;Vec offset;double power=0,margin=0;bool coverageKnown=false,lob=false;};
// One spatial uncertainty buffer; do not add a second speed-scaled time buffer.
inline constexpr double catchSafetyMargin=1.;
inline constexpr double beamMaxAngle=85.;
inline double jumpRise(double jump,double gravity,double horizon,double verticalSpeed=0){
 double launch=std::sqrt(2*gravity*std::max(0.,jump)),t=std::min(std::max(0.,horizon),launch/gravity);
 double rise=launch*t-.5*gravity*t*t;
 t=std::min(std::max(0.,horizon),std::max(0.,verticalSpeed)/gravity);
 return std::max(rise,std::max(0.,verticalSpeed)*t-.5*gravity*t*t);
}
inline double reachGap(const Defender& d,Vec p,double horizon){
 horizon=std::max(0.,horizon);double speed=std::clamp(std::max(d.speed,std::hypot(d.velocity.x,d.velocity.z)),0.,45.);
 // Diving replaces running during its burst. Add only the extra distance,
 // never a full-speed dive on top of a full-speed sprint for the same time.
 double extra=std::min(d.diveReach,std::max(0.,horizon-.1)*std::max(0.,45-speed));
 double verticalRadius=(d.above+d.below)*.5,horizontalRadius=std::min(d.radius,verticalRadius);
 double shoulderRadius=std::max(0.,d.radius-verticalRadius),shoulderY=d.position.y+(d.above-d.below)*.5;
 double horizontal=std::max(0.,std::hypot(p.x-d.position.x,p.z-d.position.z)-shoulderRadius-speed*horizon-extra);
 double top=shoulderY+jumpRise(d.jumpHeight,d.worldGravity,horizon,d.velocity.y);
 double bottom=shoulderY+std::min(0.,d.velocity.y)*std::min(horizon,.25);
 double vertical=std::max(0.,std::max(p.y-top,bottom-p.y));
 // An arm cannot have its full horizontal and full vertical extension at once.
 return (std::hypot(horizontal/horizontalRadius,vertical/verticalRadius)-1)*std::min(horizontalRadius,verticalRadius);
}
// Horizontal cross-section of the same time-dependent catch volume used by
// the planner, at the ball's future height. Negative means above/below reach.
inline double reachSectionRadius(const Defender& d,double y,double horizon){
 horizon=std::max(0.,horizon);double vr=(d.above+d.below)*.5,hr=std::min(d.radius,vr);
 double cy=d.position.y+(d.above-d.below)*.5;
 double top=cy+jumpRise(d.jumpHeight,d.worldGravity,horizon,d.velocity.y),bottom=cy+std::min(0.,d.velocity.y)*std::min(horizon,.25);
 double vertical=std::max(0.,std::max(y-top,bottom-y));if(vertical>vr)return -1;
 double speed=std::clamp(std::max(d.speed,std::hypot(d.velocity.x,d.velocity.z)),0.,45.);
 return std::max(0.,d.radius-vr)+speed*horizon+std::min(d.diveReach,std::max(0.,horizon-.1)*std::max(0.,45-speed))+hr*std::sqrt(std::max(0.,1-vertical*vertical/(vr*vr)));
}
// Receiver placement follows the observed route. Do not assume a stationary
// receiver will sprint sideways simply because a defender could do so.
inline double receiverGap(const Defender& receiver,Vec point,double horizon){
 auto pose=receiver;Vec movement{receiver.velocity.x,0,receiver.velocity.z};
 if(receiver.redirect){
  // Give the WR reaction/turning time and use 70% of measured running speed,
  // not an instantaneous reversal or maximum-effort dive.
  const double delay=.18+(length(movement)>.5?.12:0.);
  pose.position=pose.position+movement*std::min(horizon,delay);
  pose.velocity={0,receiver.velocity.y,0};pose.speed=0;pose.diveReach=0;
  pose.radius+=std::min(18.,std::max(0.,horizon-delay)*std::clamp(receiver.speed,0.,45.)*.7);
  return reachGap(pose,point,std::max(0.,horizon-.18));
 }
 pose.position=pose.position+movement*horizon;pose.velocity={0,receiver.velocity.y,0};pose.speed=0;pose.diveReach=0;
 if(length(movement)>.5)pose.radius+=std::min(3.,std::max(0.,horizon-.15)*4.);
 return reachGap(pose,point,horizon);
}
inline Solution groundFlight(const Solution& catchFlight,double gravity,double ground){
 if(!catchFlight.valid||!std::isfinite(ground)||gravity<=0)return {};
 double height=catchFlight.origin.y-ground-.6,vy=catchFlight.velocity.y,disc=vy*vy+2*gravity*height;
 if(disc<0)return {};double time=(vy+std::sqrt(disc))/gravity;if(time<=0||time>12)return {};
 auto full=catchFlight;full.time=time;full.catchPoint=point(full,gravity,time);return full;
}
struct CoverageTrace {int catcher=-1;double time=0,gap=1000;Vec point;};
// Signed distance in studs from the time-dependent reachable volume. Positive
// means outside. Sample the whole flight, including the actual launch position.
inline double coverageMargin(const Solution& s,double gravity,const std::vector<Defender>& defenders,double latency,CoverageTrace* trace=nullptr,double stopBelow=-INFINITY,double ground=NAN){
 if(std::isfinite(ground)){
  auto full=groundFlight(s,gravity,ground);if(!full.valid)return -INFINITY;
  CoverageTrace before,after;
  double a=coverageMargin(s,gravity,defenders,latency,&before,stopBelow);
  double b=coverageMargin(full,gravity,defenders,latency,&after,stopBelow);
  if(trace)*trace=a<=b?before:after;return std::min(a,b);
 }
 double margin=1000;const int steps=std::clamp(int(std::ceil(s.time/.02)),32,400);
 auto sample=[&](size_t n,double t){auto p=point(s,gravity,t);double gap=reachGap(defenders[n],p,t+latency);if(gap<margin){margin=gap;if(trace)*trace={int(n),t,gap,p};}};
 for(size_t n=0;n<defenders.size();++n){sample(n,0);sample(n,s.time);if(margin<=stopBelow)return margin;}
 const auto end=point(s,gravity,s.time);Vec horizontal{end.x-s.origin.x,0,end.z-s.origin.z};double length2=dot(horizontal,horizontal);
 for(size_t n=0;n<defenders.size();n++){
  const auto& d=defenders[n];Vec delta{d.position.x-s.origin.x,0,d.position.z-s.origin.z};double u=length2>1e-9?std::clamp(dot(delta,horizontal)/length2,0.,1.):0;
  // Horizontal distance to the entire segment gives a conservative lower
  // bound on the 3D gap. Distant actors cannot improve the current minimum.
  double speed=std::clamp(std::max(d.speed,std::hypot(d.velocity.x,d.velocity.z)),0.,45.);
  double bound=length(delta-horizontal*u)-d.radius-speed*(s.time+latency)-d.diveReach;
  if(bound>=margin)continue;
  for(int i=1;i<steps;i++){
  double t=s.time*i/steps;auto p=point(s,gravity,t);double gap=reachGap(defenders[n],p,t+latency);
  if(gap<margin){margin=gap;if(trace)*trace={int(n),t,gap,p};}
  if(margin<=stopBelow)return margin;
 }}return margin;
}
// Distinguish observed-route interception from a worst-case reach envelope.
// The envelope still drives Smart fit; a possible turn/dive is not a certainty.
struct RiskForecast {double margin=1000,projectedMargin=1000;CoverageTrace limit,projected;bool intable=false,possible=false;};
inline RiskForecast riskForecast(const Solution& s,double gravity,const std::vector<Defender>& defenders,double latency,double ground=NAN){
 if(std::isfinite(ground)){
  auto full=groundFlight(s,gravity,ground);if(!full.valid)return {};
  auto before=riskForecast(s,gravity,defenders,latency),after=riskForecast(full,gravity,defenders,latency);
  if(before.margin<after.margin){after.margin=before.margin;after.limit=before.limit;}
  if(before.projectedMargin<after.projectedMargin){after.projectedMargin=before.projectedMargin;after.projected=before.projected;}
  after.intable=after.projectedMargin<=0;after.possible=!after.intable&&after.margin<=0;return after;
 }
 RiskForecast result;if(!s.valid)return result;
 result.margin=coverageMargin(s,gravity,defenders,latency,&result.limit);
 const int steps=std::clamp(int(std::ceil(s.time/.02)),32,400);
 for(size_t n=0;n<defenders.size();++n)for(int i=0;i<=steps;++i){
  double t=s.time*i/steps,horizon=t+latency;auto d=defenders[n];
  d.position=d.position+Vec{d.velocity.x,0,d.velocity.z}*horizon;
  d.velocity.x=d.velocity.z=0;d.speed=0;d.diveReach=0;
  auto p=point(s,gravity,t);double gap=reachGap(d,p,horizon);
  if(gap<result.projectedMargin){result.projectedMargin=gap;result.projected={int(n),t,gap,p};}
 }
 result.intable=result.projectedMargin<=0;result.possible=!result.intable&&result.margin<=0;return result;
}
inline std::vector<Vec> fitOffsets(Vec origin,Vec target,Vec movement,const std::vector<Defender>& defenders,bool redirect=false){
 std::vector<Vec> offsets{{}};for(int i=0;i<8;i++){double a=i*6.283185307179586/8;for(double r:{1.5,3.,6.})offsets.push_back({r*std::cos(a),0,r*std::sin(a)});}
 const double estimate=std::clamp(length(target-origin)/80.,.1,3.);
 Vec receiver=target+movement*estimate,away{};double nearest=1e9;
 for(const auto& d:defenders){auto delta=receiver-(d.position+d.velocity*std::min(.35,estimate));delta.y=0;double distance=length(delta);if(distance<nearest){nearest=distance;away=delta;}}
 Vec forward=movement;forward.y=0;if(length(forward)<.5){forward=target-origin;forward.y=0;}
 if(length(away)<.1)away=forward;
 for(auto direction:{away,forward})if(length(direction)>.1){direction=direction*(1/length(direction));for(double reach:{3.,4.5,6.})offsets.push_back(direction*reach);}
 if(redirect)for(int i=0;i<8;++i){double a=i*6.28318530718/8;for(double r:{10.,16.,22.})offsets.push_back({r*std::cos(a),0,r*std::sin(a)});}
 return offsets;
}
// Additional adjustment around observed route lead, not a hypothetical new
// sprint. This bound is independently enforced again by the throw helper.
inline double placementLimit(Vec movement,double flight,bool redirect=false){if(redirect)return std::min(24.,6.+std::max(0.,flight-.18)*14.);return std::hypot(movement.x,movement.z)<.5?6.:3.+std::min(3.,std::max(0.,flight-.15)*4.);}
inline Vec leadAdjustment(Vec origin,Vec target,Vec movement,double studs){
 Vec direction{movement.x,0,movement.z};if(length(direction)<.5)direction={target.x-origin.x,0,target.z-origin.z};
 return length(direction)>.001?direction*(std::clamp(studs,-12.,12.)/length(direction)):Vec{};
}
inline bool placementAllowed(Vec offset,Vec movement,double flight,bool redirect=false){return finite(offset)&&std::isfinite(flight)&&flight>=0&&std::hypot(offset.x,offset.z)<=placementLimit(movement,flight,redirect)+1e-6;}
inline bool receiverCanReach(Vec offset,Vec movement,double flight,double jump){
 // Head target already includes route lead. Extra placement must fit a modest
 // catch radius plus time to adjust; high jump catches get less lateral reach.
 const double horizontal=std::hypot(offset.x,offset.z),speed=std::hypot(movement.x,movement.z);
 const double reach=4.+(speed>.5?std::min(4.,std::max(0.,flight-.2)*6.):0.);
 const double jumpCost=jump>0?std::clamp(offset.y/jump,0.,1.)*2.:0.;
 return horizontal<=std::max(3.,reach-jumpCost)+.001;
}
inline Plan planThrow(Vec origin,Vec target,Vec movement,double gravity,double latency,int mode,bool automaticAngle,double manualAngle,bool automaticPower,double gamePower,bool lob,bool fit,const std::vector<Defender>& defenders,double preferredAngle=NAN,Vec previousOffset={},bool powerFirst=false,double spawn=0,double ground=NAN){
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
 const auto offsets=fitOffsets(origin,target,movement,defenders);
 for(size_t index=0;index<(fit&&!defenders.empty()?offsets.size():1);++index){
  auto offset=offsets[index];Solution s;if(!placementAllowed(offset,movement,8))continue;
  if(fixedPower)s=solve(origin,target+offset,movement,95,gravity,latency,false,spawn);
  else if(automaticPower){
   s=solveAngle(origin,target+offset,movement,angle,gravity,latency,spawn);
  }else s=solve(origin,target+offset,movement,gamePower,gravity,latency,lob||mode==2,spawn);
  if(!s.valid||!placementAllowed(offset,movement,s.time)||!receiverCanReach(offset,movement,s.time,0))continue;
  const double margin=defenders.empty()?0:coverageMargin(s,gravity,defenders,latency,nullptr,-INFINITY,ground);
  double score=(defenders.empty()?0:std::min(margin,1.5))*10-length(offset)*.12-length(offset-previousOffset)*.15-s.time*.03-std::abs(s.angle-desired)*.012;
  if(score>bestScore){bestScore=score;best={s,offset,length(s.velocity),margin,!defenders.empty()};}
 }return best;
}
// Continuity score for comparable paths. Runtime and helper separately require
// estimated clearance before applying Smart fit assistance.
inline double fitScore(const Plan& p,bool fit){
 return (fit&&p.coverageKnown?std::min(p.margin,.5)*20:0)-p.solution.time*.8-std::hypot(p.offset.x,p.offset.z)*.06-p.offset.y*.03-(95-p.power)*.035-std::pow(std::max(0.,p.solution.angle-30.),2)*.004;
}
// Rank clearance before power. No amount of power makes overlapping catch
// envelopes exclusive. Retain that result as a risk estimate, never as "safe".
inline bool estimatedClear(const Plan& p,bool fit){return p.solution.valid&&(!fit||!p.coverageKnown||p.margin>catchSafetyMargin);}
inline bool continuitySafe(const Plan& held,const Plan& desired,bool fit){
 if(!held.solution.valid)return false;if(!fit)return true;
 if(estimatedClear(desired,true)&&!estimatedClear(held,true))return false;
 return held.margin>=desired.margin-.25;
}
// Solve a selected arrival time directly, including the forward launch offset.
// This samples receiver-reachable placements without thousands of root searches.
inline Solution solveTimed(Vec origin,Vec target,Vec movement,double time,double gravity,double latency){
 if(!finite(origin)||!finite(target)||!finite(movement)||!std::isfinite(time)||time<.06||time>8||!std::isfinite(gravity)||gravity<1||gravity>300||!std::isfinite(latency)||latency<0||latency>.3||length(movement)>45||length(target-origin)<3||length(target-origin)>600)return {};
 auto end=target+movement*(time+latency),d=end-origin+Vec{0,.5*gravity*time*time,0};double distance=length(d),power=(distance-5)/time;
 if(power<1||power>95+1e-8)return {};power=std::min(95.,power);auto direction=d*(1/distance),velocity=direction*power;
 return {true,origin+direction*5,velocity,end,time,std::atan2(velocity.y,std::hypot(velocity.x,velocity.z))*180/3.141592653589793};
}
// Absolute elevation hysteresis, shared by every automatic planner. A small
// reversal must persist; it is not a per-frame rate limit that follows chatter.
struct ElevationIntent {
 double value=NAN,since=0;int direction=0;
 void accept(double angle,double time){value=angle;since=time;direction=0;}
 double update(double desired,double time){
  if(!std::isfinite(value)||time<since){accept(desired,time);return value;}
  double delta=desired-value;if(std::abs(delta)<=1.35){direction=0;return value;}
  int next=delta>0?1:-1;
  if(std::abs(delta)>4){accept(desired,time);return value;}
  if(direction!=next){direction=next;since=time;}
  if(time-since>=.22)accept(desired,time);return value;
 }
};
struct BeamMemory {
 Plan goal,pending;double since=0;bool transitioning=false;ElevationIntent elevation;
 Plan scanBest;int nextPower=95;bool scanActive=false;Vec scanOrigin,scanTarget,scanMovement;double scanAt=0;
 Plan accept(Plan desired,const Plan& held,double time,bool fit=false){
  if(!desired.solution.valid){goal={};pending={};return desired;}
  const bool change=goal.solution.valid&&(goal.lob!=desired.lob||std::abs(goal.power-desired.power)>4||std::abs(goal.solution.angle-desired.solution.angle)>6);
  if(change&&held.solution.valid){
   bool same=pending.solution.valid&&pending.lob==desired.lob&&std::abs(pending.power-desired.power)<=2&&length(pending.offset-desired.offset)<3;
   if(!same||time<since){pending=desired;since=time;}
   if(time-since<.22&&continuitySafe(held,desired,fit))return held;
  }
  pending={};goal=desired;return desired;
 }
};
inline Plan planCoverageThrow(Vec origin,Vec target,Vec movement,double gravity,double latency,const std::vector<Defender>& defenders,bool fit,bool beam,double receiverJump,double worldGravity,Vec previousOffset={},double previousPower=0,bool previousLob=false,double previousAngle=NAN,bool search=true,double elapsed=.1,const Defender* receiver=nullptr,double heightAdjustment=0,BeamMemory* memory=nullptr,double time=0,double ground=NAN){
 if(!std::isfinite(receiverJump)||receiverJump<0||receiverJump>40||!std::isfinite(worldGravity)||worldGravity<10||worldGravity>300)return {};
 if(length(movement)<.5)movement={};
 const bool redirect=fit&&receiver&&receiver->redirect;
 const double jump=std::max(0.,(receiver?std::max(0.,receiver->jumpHeight+receiver->above-.75):std::min(8.,receiverJump*.85))-heightAdjustment);
 auto better=[&](const Plan& a,const Plan& b){
  if(!a.solution.valid)return false;if(!b.solution.valid)return true;
  bool ac=estimatedClear(a,fit),bc=estimatedClear(b,fit);if(ac!=bc)return ac;
  if(!ac&&std::abs(a.margin-b.margin)>.001)return a.margin>b.margin;
  if(beam&&std::abs(a.power-b.power)>.5)return a.power>b.power;
  if(beam&&ac&&std::abs(a.solution.angle-b.solution.angle)>.5)return a.solution.angle<b.solution.angle;
  auto cost=[&](const Plan& p){return (95-p.power)*.055+p.solution.time*.65+std::max(0.,p.solution.angle-30)*.012+length(p.offset-previousOffset)*.04;};
  return cost(a)<cost(b);
 };
 auto candidate=[&](const Solution& solution,Vec offset,bool lob,const Plan* incumbent=nullptr){
  Plan p;
  if(!solution.valid||!placementAllowed(offset,movement,solution.time,redirect)||solution.angle<(beam?-30:5)||solution.angle>beamMaxAngle||offset.y<0||offset.y>jump+.001)return p;
  if(receiver){if(receiverGap(*receiver,solution.catchPoint,solution.time+latency)>-.25)return p;}
  else{if(!receiverCanReach(offset,movement,solution.time,jump))return p;
   if(offset.y>0){const double ascent=(std::sqrt(2*worldGravity*receiverJump)-std::sqrt(2*worldGravity*(receiverJump-offset.y)))/worldGravity;if(solution.time<.15+ascent)return p;}}
  p={solution,offset,std::min(95.,length(solution.velocity)),10,!defenders.empty(),lob};
  if(incumbent&&incumbent->solution.valid&&!defenders.empty()){
   // Endpoint/launch clearance is an upper bound on whole-path clearance.
   // If even this optimistic score loses, the full scan cannot win either.
   p.margin=1000;
   for(const auto& d:defenders)p.margin=std::min(p.margin,std::min(reachGap(d,solution.origin,latency),reachGap(d,point(solution,gravity,solution.time),solution.time+latency)));
   if(std::isfinite(ground)){auto full=groundFlight(solution,gravity,ground);if(!full.valid)return Plan{};for(const auto& d:defenders)p.margin=std::min(p.margin,reachGap(d,full.catchPoint,full.time+latency));}
   if(!better(p,*incumbent))return Plan{};
  }
  p.margin=defenders.empty()?10:coverageMargin(solution,gravity,defenders,latency,nullptr,-INFINITY,ground);return p;
 };
 auto replan=[&](double power,Vec offset,bool lob,const Plan* incumbent=nullptr){if(!placementAllowed(offset,movement,8,redirect))return Plan{};auto p=candidate(solve(origin,target+offset,movement,power,gravity,latency,lob,5),offset,lob,incumbent);if(p.solution.valid)p.power=power;return p;};
 // Apply one time-based continuity constraint to every exit, including retained
 // roots and transitions from contested to clear. Coverage is recomputed for
 // the intermediate arc; it is never labelled clear merely because the goal is.
 auto stable=[&](Plan desired){
  if(memory&&desired.solution.valid){
   double angle=memory->elevation.update(desired.solution.angle,time);
   if(std::abs(angle-desired.solution.angle)>.001){
    auto solved=solveAngle(origin,target+desired.offset,movement,angle,gravity,latency,5);
    auto high=solved.valid?solve(origin,target+desired.offset,movement,length(solved.velocity),gravity,latency,true,5):Solution{};
    auto heldAngle=candidate(solved,desired.offset,high.valid&&std::abs(high.time-solved.time)<1e-5);
    if(heldAngle.solution.valid&&estimatedClear(heldAngle,fit)==estimatedClear(desired,fit)&&heldAngle.margin>=desired.margin-.25)desired=heldAngle;
    else memory->elevation.accept(desired.solution.angle,time);
   }
  }

  // Beam retains a high-power goal. Ease actual reachable throw parameters,
  // so the drawn arc and transmitted trajectory share the same solution.
  if(beam){
   if(!desired.solution.valid||previousPower<1||!std::isfinite(previousAngle))return desired;
   auto delta=desired.offset-previousOffset;const double dt=std::clamp(elapsed,.001,.1),step=12*dt;
   // Keep a physical elevation inside a small band, rather than chasing
   // neighbouring 39/40-degree solutions. Power is solved again at that angle.
   if(memory){double deltaAngle=std::abs(desired.solution.angle-previousAngle);if(deltaAngle>2)memory->transitioning=true;else if(deltaAngle<.01)memory->transitioning=false;}
   if(memory&&!memory->transitioning&&std::abs(desired.solution.angle-previousAngle)<=1.25&&previousAngle>=5){
    auto offset=previousOffset+delta*std::min(1.,step/std::max(.001,length(delta)));
    auto solved=solveAngle(origin,target+offset,movement,previousAngle,gravity,latency,5);
    if(solved.valid){auto high=solve(origin,target+offset,movement,length(solved.velocity),gravity,latency,true,5);auto quiet=candidate(solved,offset,high.valid&&std::abs(high.time-solved.time)<1e-5);
     if(quiet.solution.valid&&estimatedClear(quiet,fit)==estimatedClear(desired,fit)&&quiet.margin>=desired.margin-.25)return quiet;
    }
   }
   if(memory&&std::abs(desired.solution.angle-previousAngle)>45*dt){
    auto offset=previousOffset+delta*std::min(1.,step/std::max(.001,length(delta)));
    auto solved=solveAngle(origin,target+offset,movement,previousAngle+std::clamp(desired.solution.angle-previousAngle,-45*dt,45*dt),gravity,latency,5);
    if(solved.valid){auto high=solve(origin,target+offset,movement,length(solved.velocity),gravity,latency,true,5);auto intermediate=candidate(solved,offset,high.valid&&std::abs(high.time-solved.time)<1e-5);auto prior=replan(previousPower,previousOffset,previousLob);
     if(continuitySafe(intermediate,desired,fit))return intermediate;
    }
   }
   const double power=memory?previousPower+std::clamp(desired.power-previousPower,-100*dt,100*dt):desired.power;
   if(length(delta)>step||std::abs(power-desired.power)>.001){auto offset=previousOffset+delta*std::min(1.,step/std::max(.001,length(delta)));auto eased=replan(power,offset,desired.lob);
    if(continuitySafe(eased,desired,fit))return eased;
   }return desired;
  }
  if(!desired.solution.valid||!std::isfinite(previousAngle)||previousPower<1)return desired;
  const double dt=std::clamp(elapsed,.001,.1),step=24*dt;
  auto delta=desired.offset-previousOffset;
  const Vec offset=previousOffset+delta*std::min(1.,12*dt/std::max(.001,length(delta)));
  const double angle=previousAngle+std::clamp(desired.solution.angle-previousAngle,-step,step);
  if(std::abs(angle-desired.solution.angle)<1e-6&&length(offset-desired.offset)<1e-6)return desired;
  if(angle>=5&&angle<=beamMaxAngle){
   auto solved=solveAngle(origin,target+offset,movement,angle,gravity,latency,5);
   if(solved.valid){
    const double power=length(solved.velocity);
    auto low=solve(origin,target+offset,movement,power,gravity,latency,false,5);
    auto high=solve(origin,target+offset,movement,power,gravity,latency,true,5);
    bool lob=high.valid&&(!low.valid||std::abs(high.time-solved.time)<std::abs(low.time-solved.time));
    auto intermediate=candidate(solved,offset,lob);
    if(continuitySafe(intermediate,desired,fit))return intermediate;
   }
  }
  // A receiver can move beyond the reach of the old elevation. Never invent
  // a smoothed arc with power >95 or a different endpoint in that situation.
  return desired;
 };
 Plan held;
 if(previousPower>=1&&previousPower<=95)held=replan(previousPower,previousOffset,previousLob);
 auto finish=[&](Plan desired){
  if(beam&&memory){
   if(search||!memory->goal.solution.valid)desired=memory->accept(desired,held,time,fit);
   else{auto goal=replan(memory->goal.power,memory->goal.offset,memory->goal.lob);if(goal.solution.valid)desired=goal;else memory->goal={};}
  }return stable(desired);
 };
 if(!search&&held.solution.valid)return finish(held);
 if(!beam&&held.solution.valid&&held.power>=94.999&&estimatedClear(held,fit)&&held.margin>catchSafetyMargin+.25)return stable(held);
 Plan best;const auto offsets=fitOffsets(origin,target,movement,defenders,redirect);
 // Refine integer power tiers over successive live updates. Revalidate the
 // retained candidate against current actors instead of publishing a stale
 // result after a long exhaustive sweep.
 int firstPower=95;
 if(memory){
  bool reuse=memory->scanActive&&time>=memory->scanAt&&time-memory->scanAt<2&&length(origin-memory->scanOrigin)<3&&length(target-(memory->scanTarget+memory->scanMovement*(time-memory->scanAt)))<3&&length(movement-memory->scanMovement)<3;
  if(reuse){firstPower=memory->nextPower;best=replan(memory->scanBest.power,memory->scanBest.offset,memory->scanBest.lob);}
  else{memory->scanBest={};memory->scanOrigin=origin;memory->scanTarget=target;memory->scanMovement=movement;memory->scanAt=time;}
  memory->scanActive=false;
 }
 auto atPower=[&](double power){Plan tier=best;
  for(double height:{0.,jump*.45,jump*.65,jump*.8,jump*.9,jump}){
   if(height>0&&(!fit||defenders.empty()))continue;
   for(size_t i=0;i<(fit&&!defenders.empty()?offsets.size():1);++i){
    const Vec offset=offsets[i]+Vec{0,height,0};if(!placementAllowed(offset,movement,8,redirect))continue;
    Solution high;auto low=solve(origin,target+offset,movement,power,gravity,latency,false,5,&high);
    for(bool lob:{false,true}){auto p=candidate(lob?high:low,offset,lob,&tier);if(p.solution.valid)p.power=power;if(better(p,tier))tier=p;}
   }
  }
  return tier;
 };
 // Beam checks every integer power from the top. A five-point sweep can skip
 // a narrow clear window above its first coarse success.
 int tiers=0;
 for(double power=firstPower;power>=(beam?1:15);power-=beam?1:5){
  auto tier=atPower(power);if(better(tier,best))best=tier;
  // A clear maximum-power low arc needs no slower/lower-power sweep.
  if(power==95&&estimatedClear(best,fit)&&best.solution.angle<=35)break;
  if(beam&&estimatedClear(best,fit))break;
  if(memory&&++tiers>=(beam?6:3)){
   memory->nextPower=int(power)-(beam?1:5);memory->scanBest=best;
   memory->scanActive=memory->nextPower>=(beam?1:15);break;
  }
 }
 if(!best.solution.valid)return finish(held);
 if(held.solution.valid){
  const bool hc=estimatedClear(held,fit),bc=estimatedClear(best,fit);
  if(hc&&bc&&held.power>=best.power-(beam?.5:2.)&&(!beam||held.solution.angle<=best.solution.angle+1.)&&fitScore(held,fit)>=fitScore(best,fit)-1.0)return finish(held);
  if(!hc&&!bc&&(!beam||(held.power>=best.power-.5&&held.solution.angle<=best.solution.angle+2.))&&best.margin-held.margin<.5)return finish(held);
 }
 return finish(best);
}
enum class RoutePass {Fit,Head,Bullet,DiveBullet};
inline const char* routePassName(RoutePass pass){switch(pass){case RoutePass::Head:return "Head pass";case RoutePass::Bullet:return "Bullet";case RoutePass::DiveBullet:return "Dive lead";default:return "Smart fit";}}
inline RoutePass chooseRoutePass(Vec origin,Vec target,Vec movement,const std::vector<Defender>& defenders){
 double nearest=1000;for(const auto& d:defenders)nearest=std::min(nearest,length(d.position-target));
 Vec axis=target-origin;axis.y=0;double speed=std::hypot(movement.x,movement.z);
 double forward=dot(axis,movement)/std::max(1.,length(axis));
 if(speed>2&&std::abs(forward)<speed*.8)return nearest<24?RoutePass::DiveBullet:RoutePass::Bullet;
 return nearest>32?RoutePass::Head:RoutePass::Fit;
}
struct RoutePassLatch {
 RoutePass value=RoutePass::Fit,pending=RoutePass::Fit;double since=0;bool started=false;
 RoutePass update(RoutePass next,double time){if(!started){started=true;value=pending=next;since=time;}
  if(next!=pending){pending=next;since=time;}if(time-since>=.3)value=pending;return value;}
};
// Auto-route candidates target the receiver's head/chest, not the ground.
// A dive lead stays inside the same receiver envelope enforced at dispatch.
inline Plan planRoutePass(RoutePass style,Vec origin,Vec head,Vec movement,double gravity,double latency,const std::vector<Defender>& defenders,bool fit,const Defender* receiver,double previousAngle=NAN,double elapsed=.016,ElevationIntent* intent=nullptr,double time=0,double ground=NAN){
 Plan best;if(style==RoutePass::Fit)return best;
 const bool headPass=style==RoutePass::Head;Vec forward{movement.x,0,movement.z};if(length(forward)>.1)forward=forward*(1/length(forward));
 auto candidate=[&](Solution s,Vec offset)->Plan{
  if(!s.valid||!placementAllowed(offset,movement,s.time)||(receiver&&receiverGap(*receiver,s.catchPoint,s.time+latency)>-.25))return {};
  auto high=solve(origin,head+offset,movement,length(s.velocity),gravity,latency,true,5);
  return {s,offset,length(s.velocity),defenders.empty()?10:coverageMargin(s,gravity,defenders,latency,nullptr,-INFINITY,ground),!defenders.empty(),high.valid&&std::abs(high.time-s.time)<1e-5};
 };
 if(headPass){
  // Descend onto the head with no extra distance beyond observed route lead.
  for(double angle:{55.,60.,65.,70.}){auto s=solveAngle(origin,head,movement,angle,gravity,latency,5);if(!s.valid||s.velocity.y-gravity*s.time>=0)continue;auto p=candidate(s,{});if(estimatedClear(p,fit)){best=p;break;}}
 }else{
  // Power descending, low root only. Lead in front of the route for coverage.
  std::vector<double> leads=style==RoutePass::DiveBullet?std::vector<double>{3.,4.5,6.,1.5,0.}:std::vector<double>{0.};
  for(double power=95;power>=35;power-=1){
   for(double lead:leads){Vec offset=forward*lead+Vec{0,-1.5,0};auto s=solve(origin,head+offset,movement,power,gravity,latency,false,5);if(!s.valid||s.angle>30||s.angle<-30)continue;auto p=candidate(s,offset);if(!p.solution.valid)continue;
    bool clear=estimatedClear(p,fit),oldClear=estimatedClear(best,fit);
    if(!best.solution.valid||(clear&&!oldClear)||(clear==oldClear&&(clear?p.power>best.power+.5:p.margin>best.margin+.25)))best=p;
   }if(estimatedClear(best,fit))break;
  }
 }
 if(intent&&best.solution.valid){
  double angle=intent->update(best.solution.angle,time);
  if(std::abs(angle-best.solution.angle)>.001){auto held=candidate(solveAngle(origin,head+best.offset,movement,angle,gravity,latency,5),best.offset);
   if(held.solution.valid&&estimatedClear(held,fit)==estimatedClear(best,fit)&&held.margin>=best.margin-.25)best=held;else intent->accept(best.solution.angle,time);
  }
 }
 if(!best.solution.valid||!std::isfinite(previousAngle))return best;
 const double delta=best.solution.angle-previousAngle;
 double angle=previousAngle+std::clamp(delta,-24*std::clamp(elapsed,.001,.1),24*std::clamp(elapsed,.001,.1));
 if(angle>=5&&angle<=85){
  auto quiet=candidate(solveAngle(origin,head+best.offset,movement,angle,gravity,latency,5),best.offset);
  if(quiet.solution.valid&&estimatedClear(quiet,fit)==estimatedClear(best,fit)&&quiet.margin>=best.margin-.25)best=quiet;
 }return best;
}
// Cursor selection must persist before replacing a still-visible receiver.
struct SelectionLatch {
 std::uint64_t current=0,pending=0;double since=0;
 std::uint64_t update(std::uint64_t proposed,bool currentAvailable,double time,bool explicitLock=false){
  if(explicitLock||!current||!currentAvailable){current=proposed;pending=0;return current;}
  if(proposed==current||!proposed){pending=0;return current;}
  if(pending!=proposed){pending=proposed;since=time;}
  if(time-since>=.16){current=proposed;pending=0;}
  return current;
 }
};
// Follow launch movement immediately; ease arc and catch-placement changes.
// Display smoothing never controls throw eligibility.
struct CurveTracker {
 std::uint64_t target=0;double at=0;std::vector<Vec> points;bool settling=false;Vec anchor;
 void reset(){*this={};}
 void update(const Solution& s,double gravity,std::uint64_t identity,double time,Vec launchAnchor={NAN,NAN,NAN}){
  if(!s.valid||!identity){if(!identity||identity!=target||time<at||time-at>.3)reset();return;}
  const bool fresh=target!=identity||points.size()!=65||time<at||time-at>.3;
  const double alpha=fresh?1:1-std::exp(-std::max(0.,time-at)/.16);
  if(fresh)points.resize(65);
  const bool physicalAnchor=finite(launchAnchor);if(!physicalAnchor)launchAnchor=point(s,gravity,0);
  const Vec startShift=fresh?Vec{}:physicalAnchor?launchAnchor-anchor:point(s,gravity,0)-points.front();
  double error=0;
  for(size_t i=0;i<points.size();i++){double u=i/64.;auto desired=point(s,gravity,s.time*u);auto moved=points[i]+startShift;points[i]=fresh?desired:moved+(desired-moved)*alpha;error=std::max(error,length(points[i]-desired));}
  target=identity;at=time;anchor=launchAnchor;settling=error>.6;
 }
};
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
