#pragma once
#include "ff3_power_steps.hpp"
#include <algorithm>
#include <array>
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
  if(display.enabled!=1||display.flags!=1)continue;
  auto text=display.text;auto first=text.find_first_not_of(" \r\n\t"),last=text.find_last_not_of(" \r\n\t");text=first==std::string::npos?"":text.substr(first,last-first+1);
  if(text.empty()||text.size()>2||text.find_first_not_of("0123456789")!=std::string::npos){invalid=true;continue;}
  int value=0;for(char c:text)value=value*10+(c-'0');
  if(value<1||value>95){invalid=true;continue;}
  if(power&&power!=value)return -2;power=value;
 }
 return invalid?-1:power;
}
inline bool powerAvailable(bool held,bool automatic,int gamePower){return held&&(automatic||gamePower>0);}
// Roblox may draw its own cursor with the Windows cursor hidden. Visibility
// does not identify a locked pointer; confinement and camera mode do.
inline bool cursorModeAllowed(bool visible,bool rightHeld,bool firstPerson,bool confined=false){return !rightHeld&&!firstPerson&&!confined;}
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
struct FieldBounds {
 bool known=false;Vec center{},right{1,0,0},forward{0,0,1};double halfWidth=0,halfLength=0;
 bool valid()const{return known&&finite(center)&&finite(right)&&finite(forward)&&std::abs(right.y)<.01&&std::abs(forward.y)<.01&&std::abs(length(right)-1)<.01&&std::abs(length(forward)-1)<.01&&std::abs(dot(right,forward))<.01&&std::isfinite(halfWidth)&&std::isfinite(halfLength)&&halfWidth>=30&&halfWidth<=150&&halfLength>=80&&halfLength<=350;}
 bool contains(Vec p,double inset=0)const{auto d=p-center;return valid()&&finite(p)&&std::abs(dot(d,right))<=halfWidth-inset&&std::abs(dot(d,forward))<=halfLength-inset;}
 Vec closest(Vec p,double inset=1)const{auto d=p-center;auto q=center+right*std::clamp(dot(d,right),-halfWidth+inset,halfWidth-inset)+forward*std::clamp(dot(d,forward),-halfLength+inset,halfLength-inset);q.y=p.y;return q;}
};
struct FieldLine {Vec position,size,right,forward;};
// Only the four perimeter stripes define the field. The backing Ground and
// outside grass include dead-ball territory and must never define legal bounds.
inline FieldBounds fieldFromLines(Vec center,Vec right,Vec forward,const std::vector<FieldLine>& lines){
 FieldBounds result{true,center,right,forward,80,180};if(!result.valid()||lines.size()!=4)return {};
 std::vector<double> xs,zs;std::vector<double> xSpans,zSpans;
 for(const auto& line:lines){
  if(!finite(line.position)||!finite(line.size)||!finite(line.right)||!finite(line.forward)||line.size.y<=0||line.size.y>5||std::abs(line.position.y-center.y)>4||std::abs(length(line.right)-1)>.01||std::abs(length(line.forward)-1)>.01||std::abs(dot(line.right,line.forward))>.01||std::abs(line.right.y)>.01||std::abs(line.forward.y)>.01)return {};
  double x=dot(line.position-center,right),z=dot(line.position-center,forward);
  double ex=(std::abs(dot(line.right,right))*line.size.x+std::abs(dot(line.forward,right))*line.size.z)*.5;
  double ez=(std::abs(dot(line.right,forward))*line.size.x+std::abs(dot(line.forward,forward))*line.size.z)*.5;
  if(ez>30&&ex>.01&&ex<=3&&std::abs(x)>30){xs.push_back(x+(x<0?ex:-ex));zSpans.push_back(ez-std::abs(z));}
  else if(ex>30&&ez>.01&&ez<=3&&std::abs(z)>80){zs.push_back(z+(z<0?ez:-ez));xSpans.push_back(ex-std::abs(x));}
  else return {};
 }
 if(xs.size()!=2||zs.size()!=2)return {};std::sort(xs.begin(),xs.end());std::sort(zs.begin(),zs.end());
 if(xs[0]>=0||xs[1]<=0||zs[0]>=0||zs[1]<=0)return {};
 result.center=center+right*((xs[0]+xs[1])*.5)+forward*((zs[0]+zs[1])*.5);result.halfWidth=(xs[1]-xs[0])*.5;result.halfLength=(zs[1]-zs[0])*.5;
 if(!result.valid()||*std::min_element(xSpans.begin(),xSpans.end())<std::max(-xs[0],xs[1])-.5||*std::min_element(zSpans.begin(),zSpans.end())<std::max(-zs[0],zs[1])-.5)return {};
 return result;
}
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
struct RouteIntent {
 struct Turn {double time;Vec position;int sign;};
 std::vector<Turn> turns;
 Vec lastDirection{};
 void observeTurns(double time,Vec position,Vec direction){
  const double cross=lastDirection.x*direction.z-lastDirection.z*direction.x;
  const double alignment=dot(lastDirection,direction);
  lastDirection=direction;
  turns.erase(std::remove_if(turns.begin(),turns.end(),[&](const Turn& turn){return time-turn.time>.8;}),turns.end());
  if(std::abs(cross)<.08||alignment<.35)return;
  const int sign=cross>0?1:-1;
  if(!turns.empty()&&turns.back().sign==sign)return;
  turns.push_back({time,position,sign});
  if(turns.size()<3)return;
  const auto& first=turns[turns.size()-3];
  const auto& middle=turns[turns.size()-2];
  if(time-middle.time>.35||middle.time-first.time>.35)return;
  Vec cycle=position-first.position;cycle.y=0;
  const double distance=length(cycle);
  if(distance<.1)return;
  Vec axis=cycle*(1/distance);
  if(dot(axis,direction)<.6)return;
  // Matching turn phases span a complete weave, independent of the first
  // sampled heading. Displacement preserves the actual downfield speed.
  heading=axis;pending={};wobbleUntil=time+.35;
 }
 Vec heading{1,0,0},pending{},center{},correction{};bool ready=false,wobble=false;
 double at=0,pendingAt=0,sideAt=-10,wobbleUntil=-10;int sideSign=0;std::vector<double> reversals;
 Vec update(double time,Vec position,Vec velocity,Vec net){
  velocity.y=net.y=0;const double speed=length(velocity);
  if(!ready||time<at||time-at>.5){
   *this={};ready=true;at=time;center=position;if(speed>=1.5)heading=lastDirection=velocity*(1/speed);return speed<1.5?Vec{}:velocity;
  }
  const double dt=std::clamp(time-at,.001,.25);at=time;
  if(speed<1.5){turns.clear();lastDirection={};pending={};reversals.clear();sideSign=0;wobble=false;wobbleUntil=-10;center=position;correction={};return {};}
  observeTurns(time,position,velocity*(1/speed));
  Vec side{-heading.z,0,heading.x},direction=velocity*(1/speed);
  double forward=dot(velocity,heading),lateral=dot(velocity,side),alignment=dot(direction,heading);
  int sign=std::abs(lateral)>std::max(2.,speed*.12)?(lateral<0?-1:1):0;
  if(sign&&forward>speed*.65){
   if(sideSign&&sign!=sideSign&&time-sideAt<.45)reversals.push_back(time);
   if(sign!=sideSign){sideSign=sign;sideAt=time;}
  }
  reversals.erase(std::remove_if(reversals.begin(),reversals.end(),[&](double t){return time-t>.65;}),reversals.end());
  if(reversals.size()>=2&&forward>speed*.65)wobbleUntil=time+.35;
  wobble=time<wobbleUntil&&forward>speed*.6;
  bool changed=false;
  if(alignment<.9945){
   if(length(pending)<.5||dot(pending,direction)<.94||dot(pending,side)*lateral<0){pending=direction;pendingAt=time;}
   else{pending=pending*.7+direction*.3;pending=pending*(1/std::max(.001,length(pending)));}
   const double hold=alignment<.35?.08:(wobble?.28:.2);
   bool sustained=!wobble||(dot(net,side)*lateral>=0&&std::abs(dot(net,side))>=std::abs(lateral)*.55);
   if(time-pendingAt>=hold&&sustained){heading=pending;pending={};reversals.clear();sideSign=0;wobble=false;wobbleUntil=-10;center=position;correction={};changed=true;}
  }else pending={};
  if(!changed&&!wobble&&alignment>=.9945&&length(net)>2){
   Vec netDirection=net*(1/length(net));if(dot(netDirection,heading)>.985){heading=heading*(1-dt*.4)+netDirection*(dt*.4);heading=heading*(1/std::max(.001,length(heading)));}
  }
  side={-heading.z,0,heading.x};forward=dot(velocity,heading);
  Vec output=velocity;
  if(!changed&&dot(direction,heading)>.6){
   double across=(wobble?dot(net,side):dot(velocity,side))*.15;
   output=heading*std::max(0.,forward)+side*std::clamp(across,-2.,2.);
  }
  // Center only the small lateral anchor oscillation; never smooth progress
  // downfield, and keep the catch geometry anchored to the actual receiver.
  auto projected=center+heading*dot(position-center,heading);
  center=projected+side*(dot(position-projected,side)*(1-std::exp(-dt/.18)));
  Vec desired=wobble?side*std::clamp(dot(center-position,side),-1.5,1.5):Vec{};
  correction=changed?Vec{}:correction+(desired-correction)*(1-std::exp(-dt/.12));
  return output;
 }
};
struct RouteHistory {
 struct Sample{double time;Vec position;};std::vector<Sample> samples;
 Vec axis{1,0,0},movement{};RouteIntent intent;std::string route="Reading route",pending;double pendingAt=0;int mode=5;
 void reset(){*this={};}
 void update(double time,Vec origin,Vec position,Vec velocity){
  if(samples.empty()){axis=position-origin;axis.y=0;axis=axis*(1/std::max(1.,length(axis)));movement=length(velocity)<1.5?Vec{}:velocity;}
  if(!samples.empty()&&time-samples.back().time<.04)return;
  Vec net=velocity;
  for(const auto& s:samples)if(time-s.time<=.35&&time-s.time>=.15){net=(position-s.position)*(1/(time-s.time));net.y=0;break;}
  bool stopped=length(velocity)<1.5;
  if(!samples.empty()){double dt=time-samples.back().time;if(dt>.5){reset();update(time,origin,position,velocity);return;}auto measured=(position-samples.back().position)*(1/dt);measured.y=0;stopped=length(velocity)<1.5&&length(measured)<1.5;if(length(measured)<=45)velocity=velocity*.5+measured*.5;}
  auto intended=intent.update(time,position,stopped?Vec{}:velocity,net);
  double blend=length(intended-movement)>8?.7:.4;movement=stopped?Vec{}:movement*(1-blend)+intended*blend;
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
 // Observed dive/knockback velocity is transient, not a new WalkSpeed.
 (void)velocity;
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
struct Plan {Solution solution;Vec offset;double power=0,margin=0;bool coverageKnown=false,lob=false;double penetration=0,deliveryMargin=NAN,launchMargin=NAN,receiverMargin=NAN,robustMargin=NAN;};
// One spatial uncertainty buffer; do not add a second speed-scaled time buffer.
inline constexpr double catchSafetyMargin=1.;
inline constexpr double beamMaxAngle=85.;
inline double jumpRise(double jump,double gravity,double horizon,double verticalSpeed=0){
 double launch=std::sqrt(2*gravity*std::max(0.,jump)),t=std::min(std::max(0.,horizon),launch/gravity);
 double rise=launch*t-.5*gravity*t*t;
 t=std::min(std::max(0.,horizon),std::max(0.,verticalSpeed)/gravity);
 return std::max(rise,std::max(0.,verticalSpeed)*t-.5*gravity*t*t);
}
inline double horizontalTravel(const Defender& d,double horizon){
 horizon=std::max(0.,horizon);const double speed=std::clamp(d.speed,0.,45.);
 const double burst=std::max(0.,std::min(45.,std::hypot(d.velocity.x,d.velocity.z))-speed)*std::min(horizon,.35);
 const double futureDive=std::max(0.,horizon-.1)*std::max(0.,45-speed);
 return speed*horizon+std::min(d.diveReach,std::max(burst,futureDive));
}
inline double reachGap(const Defender& d,Vec p,double horizon){
 horizon=std::max(0.,horizon);const double travel=horizontalTravel(d,horizon);
 double verticalRadius=(d.above+d.below)*.5,horizontalRadius=std::min(d.radius,verticalRadius);
 double shoulderRadius=std::max(0.,d.radius-verticalRadius),shoulderY=d.position.y+(d.above-d.below)*.5;
 double horizontal=std::max(0.,std::hypot(p.x-d.position.x,p.z-d.position.z)-shoulderRadius-travel);
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
 return std::max(0.,d.radius-vr)+horizontalTravel(d,horizon)+hr*std::sqrt(std::max(0.,1-vertical*vertical/(vr*vr)));
}
// Receiver placement follows the observed route. Do not assume a stationary
// receiver will sprint sideways simply because a defender could do so.
inline double receiverGap(const Defender& receiver,Vec point,double horizon,const FieldBounds* field=nullptr){
 if(field&&(!field->valid()||!field->contains(receiver.position,.5)))return INFINITY;
 auto legal=[&](const Defender& pose){return !field||field->contains(pose.position,1.);};
 horizon=std::max(0.,horizon);
 auto comfortable=receiver;comfortable.jumpHeight*=.85;
 comfortable.radius=std::max(.1,comfortable.radius-.4);comfortable.above=std::max(0.,comfortable.above-.25);
 const double jumpTime=std::max(0.,horizon-.18);
 auto pose=comfortable;Vec movement{receiver.velocity.x,0,receiver.velocity.z};
 // Continuing the observed route is always an option. Redirect must add a
 // reachable alternative, not replace running with a stop after 0.3 seconds.
 auto following=comfortable;following.position=following.position+movement*horizon;
 following.velocity={0,receiver.velocity.y,0};following.speed=0;following.diveReach=0;
 if(length(movement)>.5)following.radius+=std::min(3.,std::max(0.,horizon-.15)*4.);
 const double routeGap=legal(following)?reachGap(following,point,jumpTime):INFINITY;
 if(receiver.redirect){
  // Reserve reaction/turning time. Moving adjustments keep route progress;
  // stationary adjustments retain a small, conservative reach allowance.
  const double delay=.18+(length(movement)>.5?.12:0.);
  pose.position=pose.position+movement*std::min(horizon,delay);
  pose.velocity={0,receiver.velocity.y,0};pose.speed=0;pose.diveReach=0;
  if(length(movement)>.5){
   const double available=std::max(0.,horizon-delay),speed=length(movement);
   const Vec forward=movement*(1/speed),side{-forward.z,0,forward.x},delta=(field?field->closest(point,1.):point)-pose.position;
   const double progress=dot(delta,forward);
   // Continue making progress along the route while widening/slowing within
   // a 35-degree cone. Do not model a full reversal or an instant sidestep.
   if(available<=0||progress<speed*available*.55)return routeGap;
   const double lateral=dot(delta,side),distance=std::hypot(progress,lateral);
   const Vec direction=std::abs(lateral)<=progress*.7002075382097097?Vec{delta.x,0,delta.z}*(1/std::max(1e-9,distance)):
    forward*.8191520442889918+side*(lateral<0?-.573576436351046:.573576436351046);
   const double travel=std::clamp(dot(delta,direction),0.,std::clamp(std::min(receiver.speed,speed),0.,45.)*available);
   pose.position=pose.position+direction*travel;
   return std::min(routeGap,legal(pose)?reachGap(pose,point,jumpTime):INFINITY);
  }
  const double adjustment=std::min(8.,std::max(0.,horizon-delay)*std::clamp(receiver.speed,0.,45.)*.5);
  if(field){auto delta=field->closest(point,1.)-pose.position;delta.y=0;pose.position=pose.position+delta*(std::min(1.,adjustment/std::max(1e-9,length(delta))));}
  else pose.radius+=adjustment;
  return std::min(routeGap,legal(pose)?reachGap(pose,point,std::max(0.,horizon-.18)):INFINITY);
 }
 return routeGap;
}
inline bool validBoundaryMargin(double margin){return std::isfinite(margin)&&margin>=0&&margin<=20;}
inline bool boundaryAllows(Vec point,const FieldBounds& field,double margin=2){
 if(!field.valid()||!finite(point)||!validBoundaryMargin(margin))return false;
 auto outside=point-field.closest(point,0);outside.y=0;return length(outside)<=margin+1e-6;
}
inline bool inBoundsCatch(const Solution& s,const Defender& receiver,double latency,const FieldBounds& field,double boundaryMargin=2){
 if(!s.valid||!field.valid()||!finite(s.catchPoint)||!std::isfinite(s.time)||s.time<=0||!std::isfinite(latency)||latency<0||latency>.3||!field.contains(s.origin,-3))return false;
 // Permit hands/ball over the line, but never substitute an out-of-bounds
 // receiver pose or a stadium-sized catch radius. Corners share one budget.
 if(!boundaryAllows(s.catchPoint,field,boundaryMargin))return false;
 return receiverGap(receiver,s.catchPoint,s.time+latency,&field)<=-.25;
}
inline void addBoundaryOffsets(std::vector<Vec>& offsets,Vec target,Vec movement,const FieldBounds* field,double boundaryMargin=2){
 if(!field||!field->valid()||!validBoundaryMargin(boundaryMargin))return;
 for(double t:{.25,.5,1.,1.5,2.,3.,4.}){auto future=target+movement*t;auto offset=field->closest(future,2.)-future;offset.y=0;if(length(offset)>.01&&length(offset)<=24)offsets.push_back(offset);}
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
  // This catch volume cannot produce a smaller signed gap than its radius.
  // Once that bound is met, further samples cannot change the result.
  const double minimumGap=-std::min(d.radius,(d.above+d.below)*.5);
  if(margin<=minimumGap)continue;
  // Horizontal distance to the entire segment gives a conservative lower
  // bound on the 3D gap. Distant actors cannot improve the current minimum.
  double speed=std::clamp(std::max(d.speed,std::hypot(d.velocity.x,d.velocity.z)),0.,45.);
  double bound=length(delta-horizontal*u)-d.radius-speed*(s.time+latency)-d.diveReach;
  if(bound>=margin)continue;
  double closestTime=0,closestGap=INFINITY;
  for(int i=1;i<steps;i++){
  double t=s.time*i/steps;auto p=point(s,gravity,t);double gap=reachGap(defenders[n],p,t+latency);
  if(gap<closestGap){closestGap=gap;closestTime=t;}
  if(gap<margin){margin=gap;if(trace)*trace={int(n),t,gap,p};}
  if(margin<=stopBelow)return margin;
  if(margin<=minimumGap)break;
 }
 // Refine the closest sampled encounter so a narrow minimum between time
 // samples cannot consume the extra clearance buffer unnoticed.
 if(margin>minimumGap&&closestGap<margin+1.5){
  const double step=s.time/steps;
  double low=std::max(0.,closestTime-step),high=std::min(s.time,closestTime+step);
  for(int iteration=0;iteration<12;++iteration){
   double left=(2*low+high)/3,right=(low+2*high)/3;
   double a=reachGap(d,point(s,gravity,left),left+latency);
   double b=reachGap(d,point(s,gravity,right),right+latency);
   if(a<b)high=right;else low=left;
  }
  sample(n,(low+high)*.5);
 }
 }return margin;
}
// Signed clearance bottoms out inside a catch volume. Rank contested paths by
// their horizontal penetration separately without changing the risk label.
inline double horizontalPenetration(const Defender& d,Vec p,double horizon){
 const double radius=reachSectionRadius(d,p.y,horizon);
 return radius>=0?std::max(0.,radius-std::hypot(p.x-d.position.x,p.z-d.position.z)):0.;
}
inline double coveragePenetration(const Solution& s,double gravity,const std::vector<Defender>& defenders,double latency,double ground=NAN){
 if(!s.valid)return INFINITY;
 auto flight=std::isfinite(ground)?groundFlight(s,gravity,ground):s;if(!flight.valid)return INFINITY;
 double worst=0;const int steps=std::clamp(int(std::ceil(flight.time/.05)),32,120);
 for(const auto& d:defenders)worst=std::max(worst,horizontalPenetration(d,s.catchPoint,s.time+latency));
 for(int i=0;i<=steps;++i){double t=flight.time*i/steps;auto p=point(flight,gravity,t);
  for(const auto& d:defenders)worst=std::max(worst,horizontalPenetration(d,p,t+latency));
 }return worst;
}
// Distinguish observed-route interception from a worst-case reach envelope.
// The envelope still drives Smart fit; a possible turn/dive is not a certainty.
struct RiskForecast {double margin=1000,projectedMargin=1000;CoverageTrace limit,projected;bool intable=false,possible=false;};
enum class ThrowCheck {Checking,Clear,Tight,Intercept};
inline ThrowCheck throwCheck(bool known,bool estimated,const RiskForecast& risk){
 if(!known)return ThrowCheck::Checking;
 if(risk.intable)return ThrowCheck::Intercept;
 if(risk.possible||risk.margin<=catchSafetyMargin)return ThrowCheck::Tight;
 return estimated?ThrowCheck::Checking:ThrowCheck::Clear;
}
inline const wchar_t* throwCheckLabel(ThrowCheck status){switch(status){case ThrowCheck::Clear:return L"CLEAR";case ThrowCheck::Tight:return L"TIGHT";case ThrowCheck::Intercept:return L"INTERCEPT";default:return L"CHECKING";}}
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
 if(redirect){
  for(int i=0;i<8;++i){double a=i*6.28318530718/8;for(double r:{10.,16.,18.,20.,22.})offsets.push_back({r*std::cos(a),0,r*std::sin(a)});}
  if(length(forward)>.5&&std::hypot(movement.x,movement.z)>.5){
   forward=forward*(1/length(forward));Vec side{-forward.z,0,forward.x};
   for(double width:{10.,14.,18.})for(double sign:{-1.,1.})for(double along:{-4.,0.})offsets.push_back(side*(sign*width)+forward*along);
  }
 }
 return offsets;
}
// Additional adjustment around observed route lead, not a hypothetical new
// sprint. This bound is independently enforced again by the throw helper.
inline double placementLimit(Vec movement,double flight,bool redirect=false){if(redirect)return std::min(24.,6.+std::max(0.,flight-.18)*14.);return std::hypot(movement.x,movement.z)<.5?6.:3.+std::min(3.,std::max(0.,flight-.15)*4.);}
inline Vec leadAdjustment(Vec origin,Vec target,Vec movement,double studs){
 Vec direction{movement.x,0,movement.z};if(length(direction)<.5)direction={target.x-origin.x,0,target.z-origin.z};
 return length(direction)>.001?direction*(std::clamp(studs,-12.,12.)/length(direction)):Vec{};
}
// Configured catch height remains the floor. Lead is a preference: Smart Fit
// can shorten or widen it when a different receiver-reachable spot is safer.
inline bool preservesCatchHeight(Vec offset){return finite(offset)&&offset.y>=-1e-6;}
inline bool placementAllowed(Vec offset,Vec movement,double flight,bool redirect=false){return finite(offset)&&std::isfinite(flight)&&flight>=0&&std::hypot(offset.x,offset.z)<=placementLimit(movement,flight,redirect)+1e-6;}
inline bool receiverCanReach(Vec offset,Vec movement,double flight,double jump){
 // Head target already includes route lead. Extra placement must fit a modest
 // catch radius plus time to adjust; high jump catches get less lateral reach.
 const double horizontal=std::hypot(offset.x,offset.z),speed=std::hypot(movement.x,movement.z);
 const double reach=4.+(speed>.5?std::min(4.,std::max(0.,flight-.2)*6.):0.);
 const double jumpCost=jump>0?std::clamp(offset.y/jump,0.,1.)*2.:0.;
 return horizontal<=std::max(3.,reach-jumpCost)+.001;
}
// An open receiver gaining distance needs a catchable arc rather than a
// maximum-power skim. Unknown coverage and routes across/toward the QB do
// not establish this situation. The actual trajectory is still revalidated.
inline double openAwayMinimum(Vec origin,Vec target,Vec movement,const std::vector<Defender>& defenders,bool coverageKnown){
 if(!coverageKnown||!finite(origin)||!finite(target)||!finite(movement))return -30.;
 auto route=movement;route.y=0;auto away=target-origin;away.y=0;
 const double speed=length(route),distance=length(away);
 if(speed<8||distance<12)return -30.;
 const double radial=dot(route,away)/distance;
 if(radial<6||radial/speed<.65)return -30.;
 for(const auto& d:defenders){const double gap=reachGap(d,target,.15);if(!std::isfinite(gap)||gap<=6)return -30.;}
 return 25.;
}
inline Plan planThrow(Vec origin,Vec target,Vec movement,double gravity,double latency,int mode,bool automaticAngle,double manualAngle,bool automaticPower,double gamePower,bool lob,bool fit,const std::vector<Defender>& defenders,double preferredAngle=NAN,Vec previousOffset={},bool powerFirst=false,double spawn=0,double ground=NAN,double maxAngle=75,const FieldBounds* field=nullptr,const Defender* receiver=nullptr,double minAngle=-30,double boundaryMargin=2){
 Plan best;double bestScore=-1e9;
 const bool fixedPower=automaticPower&&automaticAngle&&powerFirst;
 const bool stationary=length(movement)<.5;if(stationary)movement={};
 const double desired=std::isfinite(preferredAngle)?preferredAngle:automaticAngle?presetAngle(mode,origin,target,movement):manualAngle;
 const double floor=std::max(5.,minAngle);if(floor>maxAngle)return best;
 double angle=std::clamp(desired,floor,maxAngle);
 // Resolve reach once for the receiver. Fit candidates share this elevation,
 // so changing the preferred catch offset cannot switch the arc on each frame.
 if(automaticPower&&automaticAngle&&!fixedPower&&!solveAngle(origin,target,movement,angle,gravity,latency,spawn).valid){
  bool found=false;
  for(int step=1;step<=70&&!found;step++)for(int sign:{1,-1}){double candidate=desired+sign*step;if(candidate<floor||candidate>maxAngle)continue;if(solveAngle(origin,target,movement,candidate,gravity,latency,spawn).valid){angle=candidate;found=true;break;}}
  if(!found)return best;
 }
 auto offsets=fitOffsets(origin,target,movement,defenders);if(!fit||defenders.empty())offsets.resize(1);addBoundaryOffsets(offsets,target,movement,field,boundaryMargin);
 for(size_t index=0;index<offsets.size();++index){
  auto offset=offsets[index];Solution s;if(!placementAllowed(offset,movement,8)||!preservesCatchHeight(offset))continue;
  if(fixedPower)s=solve(origin,target+offset,movement,95,gravity,latency,false,spawn);
  else if(automaticPower){
   s=solveAngle(origin,target+offset,movement,angle,gravity,latency,spawn);
  }else s=solve(origin,target+offset,movement,gamePower,gravity,latency,lob||mode==2,spawn);
  if(!s.valid||s.angle<minAngle-1e-6||s.angle>maxAngle||!placementAllowed(offset,movement,s.time)||(!field&&!receiverCanReach(offset,movement,s.time,0)))continue;
  if(field&&(!receiver||!inBoundsCatch(s,*receiver,latency,*field,boundaryMargin)))continue;
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
inline bool estimatedClear(const Plan& p,bool fit){return p.solution.valid&&(!fit||!p.coverageKnown||(p.margin>catchSafetyMargin&&(!std::isfinite(p.deliveryMargin)||p.deliveryMargin>catchSafetyMargin)&&(!std::isfinite(p.launchMargin)||p.launchMargin>catchSafetyMargin)));}
inline double deliveryClearance(const Plan& p){double margin=std::isfinite(p.deliveryMargin)?p.deliveryMargin:p.margin;return std::isfinite(p.launchMargin)?std::min(margin,p.launchMargin):margin;}
inline double launchClearance(const Plan& p){return std::isfinite(p.launchMargin)?p.launchMargin:deliveryClearance(p);}
// One transitive ordering for every evaluated candidate. Pairwise deadbands
// can form preference cycles; stability is applied after choosing this goal.
using PlanRank=std::array<double,17>;
inline PlanRank rankPlan(const Plan& p,bool fit,bool beam,Vec previousOffset={},double preferredAngle=35,bool fastest=false){
 if(!p.solution.valid)return {};
 const bool assessed=fit&&p.coverageKnown;
 const bool delivery=!assessed||deliveryClearance(p)>catchSafetyMargin;
 const bool launch=!assessed||launchClearance(p)>catchSafetyMargin;
 const bool whole=!assessed||estimatedClear(p,true);
 auto bucket=[](double value,double step){return std::floor(std::clamp(std::isfinite(value)?value:-1000.,-1000.,1000.)/step);};
 const double effort=std::hypot(p.offset.x,p.offset.z)*.03+std::max(0.,p.offset.y)*.04;
 const double style=beam?(95-p.power)*.055+p.solution.time*.65+std::max(0.,p.solution.angle-30)*.012:std::abs(p.solution.angle-std::clamp(preferredAngle,5.,75.))*.18+p.solution.time*.08;
 const double cost=style+length(p.offset-previousOffset)*.02+effort;
 // Auto Smart Fit minimizes arrival time once the path and receiver have
 // clearance. Extra unused clearance must not buy an unnecessary sky lob.
 // Unsafe candidates retain the existing clearance-first ordering.
 if(fastest&&whole&&delivery){
  const bool robust=!fit||!std::isfinite(p.robustMargin)||p.robustMargin>=0;
  return {1.,1.,1.,0.,1.,0.,0.,double(robust),0.,-p.solution.time,0.,-effort,p.power,-length(p.offset),-std::max(0.,p.offset.y),-p.offset.x,-p.offset.z};
 }
 return {1.,double(delivery),delivery?1.:double(launch),delivery?0.:bucket(deliveryClearance(p),.25),double(whole),whole?0.:bucket(std::min(p.margin,deliveryClearance(p)),.25),whole?0.:-bucket(p.penetration,.5),fit&&std::isfinite(p.robustMargin)?bucket(std::min(1.5,p.robustMargin),.5):0.,fit&&std::isfinite(p.receiverMargin)?bucket(std::min(2.,p.receiverMargin),.5):0.,beam?bucket(p.power,.5):0.,beam&&whole?-bucket(p.solution.angle,.5):0.,-cost,p.power,-length(p.offset),-std::max(0.,p.offset.y),-p.offset.x,-p.offset.z};
}
inline bool rankedBetter(const Plan& a,const Plan& b,bool fit,bool beam,Vec previousOffset={},double preferredAngle=35,bool fastest=false){return rankPlan(a,fit,beam,previousOffset,preferredAngle,fastest)>rankPlan(b,fit,beam,previousOffset,preferredAngle,fastest);}
inline void retainRanked(std::vector<Plan>& pool,const Plan& p,bool fit,bool beam,Vec previousOffset={},double preferredAngle=35,bool fastest=false){
 if(!p.solution.valid)return;
 if(pool.size()==8&&!rankedBetter(p,pool.back(),fit,beam,previousOffset,preferredAngle,fastest))return;
 auto same=std::find_if(pool.begin(),pool.end(),[&](const Plan& q){return q.lob==p.lob&&std::abs(q.power-p.power)<1&&length(q.offset-p.offset)<.75;});
 if(same!=pool.end()){if(!rankedBetter(p,*same,fit,beam,previousOffset,preferredAngle,fastest))return;*same=p;}else pool.push_back(p);
 std::sort(pool.begin(),pool.end(),[&](const Plan& a,const Plan& b){return rankedBetter(a,b,fit,beam,previousOffset,preferredAngle,fastest);});
 if(pool.size()>8)pool.resize(8);
}
inline const std::vector<int>& powerSearchOrder(){
 static const auto order=[](){std::vector<int> powers{95,80,60,40,20,1,90,70,50,30,10,85,75,65,55,45,35,25,15,5};
  for(int p=95;p>=1;--p)if(std::find(powers.begin(),powers.end(),p)==powers.end())powers.push_back(p);return powers;}();
 return order;
}
inline int deliveryPriority(const Plan& a,const Plan& b,bool fit){
 if(!fit)return 0;
 const double am=deliveryClearance(a),bm=deliveryClearance(b);
 const bool ac=am>catchSafetyMargin,bc=bm>catchSafetyMargin;
 if(ac!=bc)return ac?1:-1;
 if(!ac){
  const bool al=launchClearance(a)>catchSafetyMargin,bl=launchClearance(b)>catchSafetyMargin;
  if(al!=bl)return al?1:-1;
  if(std::abs(am-bm)>.25)return am>bm?1:-1;
 }return 0;
}
inline bool continuitySafe(const Plan& held,const Plan& desired,bool fit){
 if(!held.solution.valid)return false;if(!fit)return true;
 if(std::isfinite(desired.robustMargin)&&std::isfinite(held.robustMargin)&&held.robustMargin<std::min(1.5,desired.robustMargin)-.5)return false;
 if(deliveryPriority(held,desired,true)<0)return false;
 if(estimatedClear(desired,true)&&!estimatedClear(held,true))return false;
 return held.margin>=desired.margin-.25&&(estimatedClear(desired,true)||held.penetration<=desired.penetration+1.);
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
 std::vector<Plan> leaders;size_t scanIndex=0;unsigned candidatesConsidered=0,candidatesRanked=0,tiersVisited=0;
 Plan accept(Plan desired,const Plan& held,double time,bool fit=false){
  if(!desired.solution.valid){goal={};pending={};return desired;}
  // Do not chase adjacent search tiers when both paths have equivalent risk.
  // A newly clear path or a meaningful power improvement still wins at once.
  if(held.solution.valid&&held.lob==desired.lob&&std::abs(held.solution.angle-desired.solution.angle)<=1.35&&std::abs(held.power-desired.power)<=.5&&length(held.offset-desired.offset)<1&&estimatedClear(held,fit)==estimatedClear(desired,fit)&&desired.margin-held.margin<.25&&continuitySafe(held,desired,fit)){pending={};goal=held;return held;}
  const bool change=goal.solution.valid&&(goal.lob!=desired.lob||std::abs(goal.power-desired.power)>.75||std::abs(goal.solution.angle-desired.solution.angle)>.35||length(goal.offset-desired.offset)>.75);
  if(change&&held.solution.valid){
   bool same=pending.solution.valid&&pending.lob==desired.lob&&std::abs(pending.power-desired.power)<=2&&length(pending.offset-desired.offset)<3;
   if(!same||time<since){pending=desired;since=time;}
   if(time-since<.22&&continuitySafe(held,desired,fit))return held;
  }
  pending={};goal=desired;return desired;
 }
};
inline Plan planCoverageThrow(Vec origin,Vec target,Vec movement,double gravity,double latency,const std::vector<Defender>& defenders,bool fit,bool beam,double receiverJump,double worldGravity,Vec previousOffset={},double previousPower=0,bool previousLob=false,double previousAngle=NAN,bool search=true,double elapsed=.1,const Defender* receiver=nullptr,double heightAdjustment=0,BeamMemory* memory=nullptr,double time=0,double ground=NAN,double maxAngle=beamMaxAngle,const FieldBounds* field=nullptr,double minAngle=-30,double boundaryMargin=2,double fixedPower=0,double preferredAngle=35,double timingTolerance=.025,int powerStep=1,double powerAnchor=0,bool fastest=false){
 if(!std::isfinite(receiverJump)||receiverJump<0||receiverJump>40||!std::isfinite(worldGravity)||worldGravity<10||worldGravity>300||!std::isfinite(fixedPower)||fixedPower<0||fixedPower>95||!std::isfinite(preferredAngle)||!std::isfinite(timingTolerance)||timingTolerance<.025||timingTolerance>.15)return {};
 if(!powerGridValid(powerStep,powerAnchor))return {};
 if(length(movement)<.5)movement={};
 const bool redirect=fit&&receiver&&receiver->redirect;
 const double jump=std::max(0.,(receiver?std::max(0.,receiver->jumpHeight+receiver->above-.75):std::min(8.,receiverJump*.85))-heightAdjustment);
 std::vector<Plan> ranked;unsigned considered=0,evaluated=0;
 auto better=[&](const Plan& a,const Plan& b){
  return rankedBetter(a,b,fit,beam,previousOffset,preferredAngle,fastest);
 };
 auto candidate=[&](const Solution& solution,Vec offset,bool lob,const Plan* incumbent=nullptr,double selectedPower=0){
  ++considered;
  Plan p;
  if(powerStep>1&&!powerReachable(selectedPower>0?selectedPower:length(solution.velocity),powerStep,powerAnchor))return p;
  if(fixedPower>0&&std::abs((selectedPower>0?selectedPower:length(solution.velocity))-fixedPower)>.001)return p;
  if(!solution.valid||!placementAllowed(offset,movement,solution.time,redirect)||!preservesCatchHeight(offset)||solution.angle<std::max(-30.,minAngle)-1e-6||solution.angle>maxAngle||offset.y<0||offset.y>jump+.001||(defenders.empty()&&offset.y>0))return p;
  if(field&&(!receiver||!inBoundsCatch(solution,*receiver,latency,*field,boundaryMargin)))return p;
  if(defenders.empty()&&!receiver&&length(offset)>.001)return p;
  if(receiver){if(receiverGap(*receiver,solution.catchPoint,solution.time+latency)>-.25)return p;}
  else{if(!receiverCanReach(offset,movement,solution.time,jump))return p;
   if(offset.y>0){const double ascent=(std::sqrt(2*worldGravity*receiverJump)-std::sqrt(2*worldGravity*(receiverJump-offset.y)))/worldGravity;if(solution.time<.15+ascent)return p;}}
  p={solution,offset,selectedPower>0?selectedPower:std::min(95.,length(solution.velocity)),10,!defenders.empty(),lob};
  if(receiver){
   p.receiverMargin=-receiverGap(*receiver,solution.catchPoint,solution.time+latency);
   p.robustMargin=p.receiverMargin;
   Vec along=movement;along.y=0;const double speed=length(along);
   along=speed>.5?along*(1/speed):Vec{1,0,0};
   const Vec across{-along.z,0,along.x};
   // Small changes in route speed/direction and release timing are evaluated
   // as alternative futures. These bounds are assumptions, not calibration.
   for(const auto& axis:{along,across})for(double sign:{-1.,1.}){
    auto perturbed=*receiver;perturbed.velocity=perturbed.velocity+axis*(sign*.75);
    for(double timing:{-timingTolerance,timingTolerance}){
     const double horizon=std::max(0.,solution.time+latency+timing);
     p.robustMargin=std::min(p.robustMargin,-receiverGap(perturbed,solution.catchPoint,horizon)-.35);
    }
   }
  }
  auto launch=solution;launch.time=std::min(solution.time,std::min(.45,std::max(.12,solution.time*.35)));launch.catchPoint=point(solution,gravity,launch.time);
  if(incumbent&&incumbent->solution.valid&&!defenders.empty()){
   // Endpoint/launch clearance is an upper bound on whole-path clearance.
   // If even this optimistic score loses, the full scan cannot win either.
   p.margin=p.deliveryMargin=p.launchMargin=1000;
   for(const auto& d:defenders){
    double start=reachGap(d,solution.origin,latency);
    p.deliveryMargin=std::min(p.deliveryMargin,std::min(start,reachGap(d,solution.catchPoint,solution.time+latency)));
    p.launchMargin=std::min(p.launchMargin,std::min(start,reachGap(d,launch.catchPoint,launch.time+latency)));
    if(fit)p.penetration=std::max(p.penetration,std::max(horizontalPenetration(d,solution.origin,latency),horizontalPenetration(d,solution.catchPoint,solution.time+latency)));
   }
   p.deliveryMargin=std::min(p.deliveryMargin,p.launchMargin);p.margin=p.deliveryMargin;
   if(std::isfinite(ground)){auto full=groundFlight(solution,gravity,ground);if(!full.valid)return Plan{};for(const auto& d:defenders){p.margin=std::min(p.margin,reachGap(d,full.catchPoint,full.time+latency));if(fit)p.penetration=std::max(p.penetration,horizontalPenetration(d,full.catchPoint,full.time+latency));}}
   if(!better(p,*incumbent))return Plan{};
  }
  p.deliveryMargin=defenders.empty()?10:coverageMargin(solution,gravity,defenders,latency);
  p.launchMargin=defenders.empty()?10:coverageMargin(launch,gravity,defenders,latency);
  p.deliveryMargin=std::min(p.deliveryMargin,p.launchMargin);
  p.margin=p.deliveryMargin;
  if(!defenders.empty()&&std::isfinite(ground)){auto full=groundFlight(solution,gravity,ground);if(!full.valid)return Plan{};p.margin=std::min(p.margin,coverageMargin(full,gravity,defenders,latency));}
  if(fit&&!defenders.empty()){
   const double fastest=45.;
   const double coverageReserve=p.margin-catchSafetyMargin-.35-fastest*timingTolerance;
   p.robustMargin=std::isfinite(p.robustMargin)?std::min(p.robustMargin,coverageReserve):coverageReserve;
  }
  if(fit&&!defenders.empty()&&p.margin<=catchSafetyMargin)p.penetration=coveragePenetration(solution,gravity,defenders,latency,ground);
  ++evaluated;retainRanked(ranked,p,fit,beam,previousOffset,preferredAngle,fastest);
  return p;
 };
 auto replan=[&](double power,Vec offset,bool lob,const Plan* incumbent=nullptr){if(!placementAllowed(offset,movement,8,redirect))return Plan{};return candidate(solve(origin,target+offset,movement,power,gravity,latency,lob,5),offset,lob,incumbent,power);};
 // Apply one time-based continuity constraint to every exit, including retained
 // roots and transitions from contested to clear. Coverage is recomputed for
 // the intermediate arc; it is never labelled clear merely because the goal is.
 auto stable=[&](Plan desired){
  if(fastest){
   // Keep small search noise quiet, but never ease through a slower loft.
   // The retained route is freshly solved and rescored at this observation.
   if(previousPower>=1&&previousPower<=95){
    auto held=replan(previousPower,previousOffset,previousLob);
    if(held.solution.valid&&desired.solution.valid&&held.lob==desired.lob&&length(held.offset-desired.offset)<.75&&held.solution.time<=desired.solution.time+.025&&estimatedClear(held,fit)==estimatedClear(desired,fit)&&continuitySafe(held,desired,fit))return held;
   }
   return desired;
  }
  if(memory&&desired.solution.valid){
   double angle=memory->elevation.update(desired.solution.angle,time);
   if(std::abs(angle-desired.solution.angle)>.001){
    auto solved=solveAngle(origin,target+desired.offset,movement,angle,gravity,latency,5);
    auto high=solved.valid?solve(origin,target+desired.offset,movement,length(solved.velocity),gravity,latency,true,5):Solution{};
    auto heldAngle=candidate(solved,desired.offset,high.valid&&std::abs(high.time-solved.time)<1e-5);
    if(heldAngle.solution.valid&&(!beam||std::abs(heldAngle.power-desired.power)<=.5)&&estimatedClear(heldAngle,fit)==estimatedClear(desired,fit)&&continuitySafe(heldAngle,desired,fit))desired=heldAngle;
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
     if(quiet.solution.valid&&std::abs(quiet.power-desired.power)<=.5&&estimatedClear(quiet,fit)==estimatedClear(desired,fit)&&continuitySafe(quiet,desired,fit))return quiet;
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
  if(beam&&memory)desired=memory->accept(desired,held,time,fit);
  auto chosen=stable(desired);
  if(memory){memory->leaders=ranked;memory->candidatesConsidered=considered;memory->candidatesRanked=evaluated;}
  return chosen;
 };
 Plan best=held;
 auto seedAngle=[&](double angle){
  auto solved=solveAngle(origin,target,movement,angle,gravity,latency,5);
  if(!solved.valid)return;
  if(powerStep>1){for(double power:powerNeighbors(length(solved.velocity),powerStep,powerAnchor))if(power>0)for(bool high:{false,true}){auto p=replan(power,{},high,&best);if(better(p,best))best=p;}}
  else{auto high=solved.valid?solve(origin,target,movement,length(solved.velocity),gravity,latency,true,5):Solution{};auto p=candidate(solved,{},high.valid&&std::abs(high.time-solved.time)<1e-5,&best);if(better(p,best))best=p;}
 };
 if(!beam&&!fastest&&fixedPower==0)for(double angle:{preferredAngle,preferredAngle-5,preferredAngle+5}){
  if(angle<std::max(5.,minAngle)||angle>maxAngle)continue;
  seedAngle(angle);
 }
 // Seed real fixed-angle interceptions immediately, including quiet frames.
 // Filtering shallow arcs alone could leave a live three-tier sweep waiting
 // for a lower-power band. These solves choose power and lead together.
 if(minAngle>=5)for(double angle:{minAngle,minAngle+5,minAngle+10}){
  if(angle>maxAngle)continue;
  seedAngle(angle);
 }
 if(memory){
  // Re-solve and re-score retained alternatives on every frame. Old positions,
  // velocities, risk scores and launch origins are never reused as evidence.
  const auto previous=memory->leaders;
  if(memory->goal.solution.valid){auto goal=replan(memory->goal.power,memory->goal.offset,memory->goal.lob,&best);if(better(goal,best))best=goal;}
  for(const auto& old:previous){auto p=replan(old.power,old.offset,old.lob,&best);if(better(p,best))best=p;}
  // A newly opened 95-power window should not wait for a sweep to complete.
  if(fixedPower==0)for(const auto& offset:{previousOffset,Vec{}})for(bool high:{false,true}){auto p=replan(95,offset,high,&best);if(better(p,best))best=p;}
 }
 if(fixedPower==0&&!search&&best.solution.valid)return finish(best);
 auto offsets=fitOffsets(origin,target,movement,defenders,redirect);if(!fit||(!receiver&&defenders.empty()))offsets.resize(1);addBoundaryOffsets(offsets,target,movement,field,boundaryMargin);
 const auto& powerOrder=powerSearchOrder();size_t scanIndex=0;
 if(memory){
  bool reuse=memory->scanActive&&time>=memory->scanAt&&time-memory->scanAt<4&&length(origin-memory->scanOrigin)<6&&length(target-(memory->scanTarget+memory->scanMovement*(time-memory->scanAt)))<3&&length(movement-memory->scanMovement)<3;
  if(reuse){scanIndex=memory->scanIndex;auto prior=replan(memory->scanBest.power,memory->scanBest.offset,memory->scanBest.lob,&best);if(better(prior,best))best=prior;}
  else{memory->scanBest={};memory->scanIndex=0;memory->tiersVisited=0;memory->scanOrigin=origin;memory->scanTarget=target;memory->scanMovement=movement;memory->scanAt=time;}
  memory->scanActive=false;
 }
 // Loss of the retained solution must revisit full power immediately. A
 // resumed low-power sweep can otherwise miss a newly narrow distant window.
 if(!best.solution.valid)scanIndex=0;
 // A contested coarse tier must also explore slower/higher trajectories
 // immediately. Arrival-time solves cover fractional powers between the tiers
 // and cannot be starved by a moving QB or a restarted fine-power scan.
 auto rescue=[&](){if(fit&&!defenders.empty()&&!estimatedClear(best,true)){
  const double fast=std::clamp((length(target-origin)-5)/95.,.12,2.);
  for(double scale:{1.05,1.35,1.75,2.25,3.,4.}){
   const double arrival=fast*scale;if(arrival>8)continue;
   for(double height:{0.,jump*.7,jump})for(auto offset:offsets){
    offset.y=height;if(!placementAllowed(offset,movement,arrival,redirect))continue;
    auto solved=solveTimed(origin,target+offset,movement,arrival,gravity,latency);
    if(!solved.valid)continue;
    auto escape=candidate(solved,offset,false,&best);
    if(better(escape,best)){
     // Descending at the catch does not identify the high ballistic root.
     // Classify the retained solution exactly so dispatch solves the same arc.
     auto high=solve(origin,target+offset,movement,escape.power,gravity,latency,true,5);
     escape.lob=high.valid&&std::abs(high.time-solved.time)<1e-5;
     best=escape;
    }
   }
  }
 }};
 auto atPower=[&](double power){Plan tier=best;
  for(double height:{0.,jump*.45,jump*.65,jump*.8,jump*.9,jump}){
   if(height>0&&(!fit||defenders.empty()))continue;
   for(size_t i=0;i<offsets.size();++i){
    const Vec offset=offsets[i]+Vec{0,height,0};if(!placementAllowed(offset,movement,8,redirect))continue;
    Solution high;auto low=solve(origin,target+offset,movement,power,gravity,latency,false,5,&high);
    for(bool lob:{false,true}){auto p=candidate(lob?high:low,offset,lob,&tier,power);if(better(p,tier))tier=p;}
   }
  }
  return tier;
 };
 // Visit distant power bands early, then fill every unvisited integer tier.
 // Three complete tiers per live cycle keep the work bounded; no narrow
 // high-power window is permanently skipped by the coarse initial probes.
 int tiers=0;
 if(fixedPower>0){auto selected=atPower(fixedPower);if(better(selected,best))best=selected;if(memory){memory->tiersVisited=1;memory->scanActive=false;}return finish(best);}
 for(;scanIndex<powerOrder.size();){
  const double power=powerOrder[scanIndex++];
  if(!powerReachable(power,powerStep,powerAnchor))continue;
  if(memory)++memory->tiersVisited;
  auto tier=atPower(power);if(better(tier,best))best=tier;
  if(scanIndex==1&&(!fastest||!memory))rescue();
  // A clear maximum-power low arc needs no slower/lower-power sweep.
  if((beam||fastest)&&power==95&&best.power>=94.999&&estimatedClear(best,fit)&&(!fastest||!std::isfinite(best.robustMargin)||best.robustMargin>=0)&&best.solution.angle<=35){scanIndex=powerOrder.size();break;}
  if(memory&&++tiers>=(fastest?1:3)){
   break;
  }
 }
 // Refine two ranked neighborhoods, including fractional powers/heights.
 // This is a local improvement, not a claim of a continuous global optimum.
 std::vector<Plan> seeds;if(best.solution.valid)seeds.push_back(best);if(ranked.size()>1)seeds.push_back(ranked[1]);
 for(auto seed:seeds)for(double step:{1.,.25}){
  for(auto delta:{Vec{step,0,0},Vec{-step,0,0},Vec{0,step,0},Vec{0,-step,0},Vec{0,0,step},Vec{0,0,-step}}){auto p=replan(seed.power,seed.offset+delta,seed.lob,&seed);if(better(p,best))best=p;if(better(p,seed))seed=p;}
  for(double sign:{-1.,1.}){double power=seed.power+sign*step;if(power<1||power>95)continue;auto p=replan(power,seed.offset,seed.lob,&seed);if(better(p,best))best=p;if(better(p,seed))seed=p;}
 }
 if(memory){memory->scanIndex=scanIndex;memory->nextPower=scanIndex<powerOrder.size()?powerOrder[scanIndex]:0;memory->scanBest=best;memory->scanActive=scanIndex<powerOrder.size();}
 if(!ranked.empty()&&better(ranked.front(),best))best=ranked.front();
 if(!best.solution.valid)return finish(held);
 // The freshly revalidated held plan already participated in this ranking.
 // Do not override a better goal with a second, different scoring rule.
 // finish() retains the existing safety-aware temporal smoothing.
 return finish(best);
}
// Optional loft candidate. Auto route uses the full coverage search instead
// of forcing this candidate over a less exposed low arc.
inline Plan planLoftThrow(Vec origin,Vec target,Vec movement,double gravity,double latency,const std::vector<Defender>& defenders,bool fit,const Defender* receiver,Vec previousOffset={},double ground=NAN,double previousAngle=NAN){
 Plan best;
 std::vector<double> angles;
 if(std::isfinite(previousAngle)&&previousAngle>=30&&previousAngle<=50)angles.push_back(previousAngle);
 for(double angle:{40.,45.,50.,35.,30.})if(std::none_of(angles.begin(),angles.end(),[&](double a){return std::abs(a-angle)<.01;}))angles.push_back(angle);
 for(double angle:angles){
  auto p=planThrow(origin,target,movement,gravity,latency,2,true,angle,true,95,true,fit,defenders,angle,previousOffset,false,5,ground,50);
  if(!p.solution.valid||(receiver&&receiverGap(*receiver,p.solution.catchPoint,p.solution.time+latency)>-.25))continue;
  auto high=solve(origin,target+p.offset,movement,p.power,gravity,latency,true,5);
  p.lob=high.valid&&std::abs(high.time-p.solution.time)<1e-5;
  if(estimatedClear(p,fit))return p;
  if(!best.solution.valid||p.margin>best.margin+.25)best=p;
 }
 return best;
}
enum class RoutePass {Fit,Head,Bullet,DiveBullet};
inline const char* routePassName(RoutePass pass){switch(pass){case RoutePass::Head:return "Head pass";case RoutePass::Bullet:return "Bullet";case RoutePass::DiveBullet:return "Dive lead";default:return "Smart fit";}}
inline RoutePass chooseRoutePass(Vec origin,Vec target,Vec movement,const std::vector<Defender>& defenders){
 double nearest=1000;for(const auto& d:defenders)nearest=std::min(nearest,length(d.position-target));
 Vec axis=target-origin;axis.y=0;double speed=std::hypot(movement.x,movement.z);
 double forward=dot(axis,movement)/std::max(1.,length(axis));
 if(speed>2&&std::abs(forward)<speed*.8)return nearest<24?RoutePass::DiveBullet:RoutePass::Bullet;
 return nearest>32?RoutePass::Bullet:RoutePass::Fit;
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
  if(!s.valid||(!headPass&&(s.angle>30||s.angle< -30))||!placementAllowed(offset,movement,s.time)||(receiver&&receiverGap(*receiver,s.catchPoint,s.time+latency)>-.25))return {};
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
struct SelectionRetention {
 std::uint64_t target=0;double nearAt=-1;
 bool keep(std::uint64_t identity,double distance,double acquisition,double time,bool locked=false){
  if(target!=identity){target=identity;nearAt=-1;}
  if(!identity||!std::isfinite(distance)||!std::isfinite(time))return false;
  if(locked)return true;
  const double release=std::max(acquisition+18.,acquisition*1.75);
  if(distance<=release){nearAt=time;return true;}
  return nearAt>=0&&time>=nearAt&&time-nearAt<=.65;
 }
};
struct SelectionLatch {
 std::uint64_t current=0,pending=0;double since=0;
 std::uint64_t update(std::uint64_t proposed,bool currentAvailable,double time,bool explicitLock=false,double dwell=.16){
  if(explicitLock||!current||!currentAvailable){current=proposed;pending=0;return current;}
  if(proposed==current||!proposed){pending=0;return current;}
  if(pending!=proposed){pending=proposed;since=time;}
  if(time-since>=dwell){current=proposed;pending=0;}
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
// Inside a body, prefer its centerline. The old clamped distance made every
// overlapping body score zero, so actor enumeration order chose the receiver.
inline double selectionScore(double x,double y,double footX,double footY,double headX,double headY,double radius){
 const double axis=selectionDistance(x,y,footX,footY,headX,headY,0,false);
 radius=std::max(1.,radius);
 return axis<radius?axis/radius-1:axis-radius;
}
struct Screen {double x=0,y=0;bool valid=false;};
inline Screen project(Vec p,const float* m,int width,int height){
 if(!finite(p)||width<100||height<100)return {};
 const double w=m[12]*p.x+m[13]*p.y+m[14]*p.z+m[15];if(!std::isfinite(w)||w<.01)return {};
 const double x=(m[0]*p.x+m[1]*p.y+m[2]*p.z+m[3])/w,y=(m[4]*p.x+m[5]*p.y+m[6]*p.z+m[7])/w;
 if(!std::isfinite(x)||!std::isfinite(y)||std::abs(x)>1.02||std::abs(y)>1.02)return {};
 return {(x+1)*width*.5,(1-y)*height*.5,true};
}
// A physical hold keeps one throw intent live; after up, retain only a short
// delivery window. Plan/sample freshness is checked independently.
inline bool pressAuthorized(std::uint64_t clicked,std::uint64_t released,bool held,std::uint64_t tick){
 if(!clicked||tick<clicked||tick-clicked>300000)return false;
 if(held)return !released&&tick-clicked<=300000;
 if(released)return released>=clicked&&tick>=released&&tick-released<=350;
 return tick-clicked<=350;
}
struct ClickGate {
 bool owned=false,pending=false;double at=0;
 bool down(bool eligible,double time){if(owned)return true;if(!eligible||!std::isfinite(time))return false;owned=pending=true;at=time;return true;}
 bool up(){bool result=owned;owned=false;return result;}
 bool take(double time,bool eligible){if(!pending)return false;pending=false;return eligible&&time>=at&&time-at<=.15;}
 void cancel(){pending=false;} // Swallow only the matching physical up after interception.
};
}
