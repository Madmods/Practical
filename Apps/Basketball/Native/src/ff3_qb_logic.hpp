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
inline Solution solve(Vec origin,Vec target,Vec movement,double speed,double gravity,double latency,bool lob=false,double spawn=0){
 if(!finite(origin)||!finite(target)||!finite(movement)||!std::isfinite(speed)||speed<1||speed>150||!std::isfinite(gravity)||gravity<1||gravity>300||!std::isfinite(latency)||latency<0||latency>.3||length(movement)>40||length(target-origin)<3||length(target-origin)>600)return {};
 if(!std::isfinite(spawn)||spawn<0||spawn>5)return {};
 auto displacement=[&](double t){return target+movement*(t+latency)-origin+Vec{0,.5*gravity*t*t,0};};
 auto velocity=[&](double t){return displacement(t)*(speed/(speed*t+spawn));};
 auto error=[&](double t){auto d=displacement(t);return dot(d,d)-(speed*t+spawn)*(speed*t+spawn);};
 auto slope=[&](double t){return 2*dot(displacement(t),movement+Vec{0,gravity*t,0})-2*speed*(speed*t+spawn);};
 std::vector<double> roots;double previous=.03,prior=error(previous),priorSlope=slope(previous);
 auto bracket=[&](double low,double high,double a,double b){
  if(!((a>0&&b<=0)||(a<0&&b>=0)))return;
  const bool positive=a>0;
  for(int j=0;j<32;j++){double mid=(low+high)*.5;if((error(mid)>0)==positive)low=mid;else high=mid;}
  double root=(low+high)*.5;if(roots.empty()||root-roots.back()>1e-6)roots.push_back(root);
 };
 for(int i=1;i<=800;i++){
  double next=.03+i*.01,current=error(next),nextSlope=slope(next);
  // Near maximum range both roots may sit inside one 10 ms interval.
  // Split at the local extremum instead of missing the negative minimum.
  if(priorSlope*nextSlope<0){double lo=previous,hi=next;bool positive=priorSlope>0;
   for(int j=0;j<36;j++){double mid=(lo+hi)*.5;if((slope(mid)>0)==positive)lo=mid;else hi=mid;}
   double mid=(lo+hi)*.5,value=error(mid);bracket(previous,mid,prior,value);
   if(std::abs(value)<1e-7&&(roots.empty()||mid-roots.back()>1e-6))roots.push_back(mid);
   bracket(mid,next,value,current);
  }else bracket(previous,next,prior,current);
  previous=next;prior=current;priorSlope=nextSlope;
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
 if(!finite(origin)||!finite(target)||!finite(movement)||!std::isfinite(angle)||angle<5||angle>85||!std::isfinite(gravity)||gravity<1||gravity>300||!std::isfinite(latency)||latency<0||latency>.3||length(movement)>40||length(target-origin)<3||length(target-origin)>600)return {};
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
  if(!samples.empty()){double dt=time-samples.back().time;if(dt>.5){reset();update(time,origin,position,velocity);return;}auto measured=(position-samples.back().position)*(1/dt);measured.y=0;const bool stopped=length(velocity)<.5&&length(measured)<.5;if(length(measured)<=40)velocity=velocity*.5+measured*.5;double blend=length(velocity-movement)>8?.7:.4;movement=stopped?Vec{}:movement*(1-blend)+velocity*blend;}
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
struct Defender {Vec position,velocity;double speed=22,jumpHeight=7,diveReach=12;double radius=6,above=6,below=8,worldGravity=196.2;};
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
struct CoverageTrace {int catcher=-1;double time=0,gap=1000;Vec point;};
// Signed distance in studs from the time-dependent reachable volume. Positive
// means outside. Sample the whole flight, including the actual launch position.
inline double coverageMargin(const Solution& s,double gravity,const std::vector<Defender>& defenders,double latency,CoverageTrace* trace=nullptr,double stopBelow=-INFINITY){
 double margin=1000;const int steps=std::clamp(int(std::ceil(s.time/.02)),32,400);
 for(size_t n=0;n<defenders.size();n++)for(int i=0;i<=steps;i++){
  double t=s.time*i/steps;auto p=point(s,gravity,t);double gap=reachGap(defenders[n],p,t+latency);
  if(gap<margin){margin=gap;if(trace)*trace={int(n),t,gap,p};}
  if(margin<=stopBelow)return margin;
 }return margin;
}
inline std::vector<Vec> fitOffsets(Vec origin,Vec target,Vec movement,const std::vector<Defender>& defenders){
 std::vector<Vec> offsets{{}};for(int i=0;i<8;i++){double a=i*6.283185307179586/8;for(double r:{1.5,3.,6.})offsets.push_back({r*std::cos(a),0,r*std::sin(a)});}
 const double estimate=std::clamp(length(target-origin)/80.,.1,3.);
 Vec receiver=target+movement*estimate,away{};double nearest=1e9;
 for(const auto& d:defenders){auto delta=receiver-(d.position+d.velocity*std::min(.35,estimate));delta.y=0;double distance=length(delta);if(distance<nearest){nearest=distance;away=delta;}}
 Vec forward=movement;forward.y=0;if(length(forward)<.5){forward=target-origin;forward.y=0;}
 if(length(away)<.1)away=forward;
 for(auto direction:{away,forward})if(length(direction)>.1){direction=direction*(1/length(direction));for(double reach:{3.,4.5,6.})offsets.push_back(direction*reach);}
 return offsets;
}
// Additional adjustment around observed route lead, not a hypothetical new
// sprint. This bound is independently enforced again by the throw helper.
inline double placementLimit(Vec movement,double flight){return std::hypot(movement.x,movement.z)<.5?3.:3.+std::min(3.,std::max(0.,flight-.15)*4.);}
inline bool placementAllowed(Vec offset,Vec movement,double flight){return finite(offset)&&std::isfinite(flight)&&flight>=0&&std::hypot(offset.x,offset.z)<=placementLimit(movement,flight)+1e-6;}
inline bool receiverCanReach(Vec offset,Vec movement,double flight,double jump){
 // Head target already includes route lead. Extra placement must fit a modest
 // catch radius plus time to adjust; high jump catches get less lateral reach.
 const double horizontal=std::hypot(offset.x,offset.z),speed=std::hypot(movement.x,movement.z);
 const double reach=4.+(speed>.5?std::min(4.,std::max(0.,flight-.2)*6.):0.);
 const double jumpCost=jump>0?std::clamp(offset.y/jump,0.,1.)*2.:0.;
 return horizontal<=std::max(3.,reach-jumpCost)+.001;
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
 const auto offsets=fitOffsets(origin,target,movement,defenders);
 for(size_t index=0;index<(fit&&!defenders.empty()?offsets.size():1);++index){
  auto offset=offsets[index];Solution s;if(!placementAllowed(offset,movement,8))continue;
  if(fixedPower)s=solve(origin,target+offset,movement,95,gravity,latency,false,spawn);
  else if(automaticPower){
   s=solveAngle(origin,target+offset,movement,angle,gravity,latency,spawn);
  }else s=solve(origin,target+offset,movement,gamePower,gravity,latency,lob||mode==2,spawn);
  if(!s.valid||!placementAllowed(offset,movement,s.time)||!receiverCanReach(offset,movement,s.time,0))continue;
  const double margin=defenders.empty()?0:coverageMargin(s,gravity,defenders,latency);
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
// Solve a selected arrival time directly, including the forward launch offset.
// This samples receiver-reachable placements without thousands of root searches.
inline Solution solveTimed(Vec origin,Vec target,Vec movement,double time,double gravity,double latency){
 if(!finite(origin)||!finite(target)||!finite(movement)||!std::isfinite(time)||time<.06||time>8||!std::isfinite(gravity)||gravity<1||gravity>300||!std::isfinite(latency)||latency<0||latency>.3||length(movement)>40||length(target-origin)<3||length(target-origin)>600)return {};
 auto end=target+movement*(time+latency),d=end-origin+Vec{0,.5*gravity*time*time,0};double distance=length(d),power=(distance-5)/time;
 if(power<1||power>95+1e-8)return {};power=std::min(95.,power);auto direction=d*(1/distance),velocity=direction*power;
 return {true,origin+direction*5,velocity,end,time,std::atan2(velocity.y,std::hypot(velocity.x,velocity.z))*180/3.141592653589793};
}
inline Plan planCoverageThrow(Vec origin,Vec target,Vec movement,double gravity,double latency,const std::vector<Defender>& defenders,bool fit,bool beam,double receiverJump,double worldGravity,Vec previousOffset={},double previousPower=0,bool previousLob=false,double previousAngle=NAN,bool search=true,double elapsed=.1,const Defender* receiver=nullptr){
 if(!std::isfinite(receiverJump)||receiverJump<0||receiverJump>40||!std::isfinite(worldGravity)||worldGravity<10||worldGravity>300)return {};
 if(length(movement)<.5)movement={};
 const double jump=receiver?std::max(0.,receiver->jumpHeight+receiver->above-.35):std::min(8.,receiverJump*.85);
 auto candidate=[&](const Solution& solution,Vec offset,bool lob){
  Plan p;
  if(!solution.valid||!placementAllowed(offset,movement,solution.time)||solution.angle<(beam?-30:5)||solution.angle>beamMaxAngle||offset.y<0||offset.y>jump+.001)return p;
  if(receiver){if(reachGap(*receiver,solution.catchPoint,solution.time+latency)>-.25)return p;}
  else{if(!receiverCanReach(offset,movement,solution.time,jump))return p;
   if(offset.y>0){const double ascent=(std::sqrt(2*worldGravity*receiverJump)-std::sqrt(2*worldGravity*(receiverJump-offset.y)))/worldGravity;if(solution.time<.15+ascent)return p;}}
  return Plan{solution,offset,std::min(95.,length(solution.velocity)),defenders.empty()?10:coverageMargin(solution,gravity,defenders,latency),!defenders.empty(),lob};
 };
 auto replan=[&](double power,Vec offset,bool lob){if(!placementAllowed(offset,movement,8))return Plan{};auto p=candidate(solve(origin,target+offset,movement,power,gravity,latency,lob,5),offset,lob);if(p.solution.valid)p.power=power;return p;};
 Plan held;
 if(previousPower>=1&&previousPower<=95)held=replan(previousPower,previousOffset,previousLob);
 if(!search&&held.solution.valid)return held;
 if(held.solution.valid&&held.power>=94.999&&estimatedClear(held,fit)&&held.margin>catchSafetyMargin+.25)return held;
 auto better=[&](const Plan& a,const Plan& b){
  if(!a.solution.valid)return false;if(!b.solution.valid)return true;
  bool ac=estimatedClear(a,fit),bc=estimatedClear(b,fit);if(ac!=bc)return ac;
  // A contested 95-power pass must not beat a safer lower-power/high arc.
  if(!ac){if(std::abs(a.margin-b.margin)>.25)return a.margin>b.margin;}
  else if(beam&&std::abs(a.power-b.power)>.5)return a.power>b.power;
  auto cost=[&](const Plan& p){return (95-p.power)*.055+p.solution.time*.65+std::max(0.,p.solution.angle-30)*.012+length(p.offset-previousOffset)*.04;};
  return cost(a)<cost(b);
 };
 Plan best;const auto offsets=fitOffsets(origin,target,movement,defenders);
 auto atPower=[&](double power){Plan tier;
  for(double height:{0.,jump*.65,jump}){
   if(height>0&&(!fit||defenders.empty()))continue;
   for(size_t i=0;i<(fit&&!defenders.empty()?offsets.size():1);++i)for(bool lob:{false,true}){
    auto p=replan(power,offsets[i]+Vec{0,height,0},lob);if(better(p,tier))tier=p;
   }
  }
  return tier;
 };
 for(double power:{95.,90.,85.,80.,75.,70.,65.,60.,55.,50.,45.,40.,35.,30.,25.,20.,15.}){
  auto tier=atPower(power);if(better(tier,best))best=tier;
  // A clear maximum-power low arc needs no slower/lower-power sweep.
  if(power==95&&estimatedClear(best,fit)&&best.solution.angle<=35)break;
  // Refine the first clear five-point interval at one-point resolution.
  if(beam&&estimatedClear(best,fit)){
   for(double fine=std::min(95.,power+4);fine>power;fine--){auto p=atPower(fine);if(estimatedClear(p,fit)){best=p;break;}}
   break;
  }
 }
 if(!best.solution.valid)return held;
 if(held.solution.valid){
  const bool hc=estimatedClear(held,fit),bc=estimatedClear(best,fit);
  // Keep a genuinely clear path through minor ranking noise. A newly unsafe
  // retained path cannot hide an available clear replacement.
  if(hc&&bc&&(!beam||held.power>=best.power-.5)&&fitScore(held,fit)>=fitScore(best,fit)-1.0)return held;
  if(!hc&&!bc&&best.margin-held.margin<.5)return held;
  // Rate limits are time based. Every intermediate arc is solved and evaluated
  // again, so interpolated aim never silently changes the intended landing.
  const double dt=std::clamp(elapsed,.005,.15);
  auto delta=best.offset-held.offset;
  Vec offset=held.offset+delta*std::min(1.,12*dt/std::max(.001,length(delta)));
  const double angle=held.solution.angle+std::clamp(best.solution.angle-held.solution.angle,-40*dt,40*dt);
  if(angle>=5&&angle<=85){
   auto solved=solveAngle(origin,target+offset,movement,angle,gravity,latency,5);
   const double power=std::min(95.,length(solved.velocity));
   auto low=solve(origin,target+offset,movement,power,gravity,latency,false,5);
   auto high=solve(origin,target+offset,movement,power,gravity,latency,true,5);
   const bool highRoot=high.valid&&(!low.valid||std::abs(high.time-solved.time)<std::abs(low.time-solved.time));
   auto smooth=candidate(highRoot?high:low,offset,highRoot);if(smooth.solution.valid)smooth.power=power;
   if(smooth.solution.valid&&(!bc||estimatedClear(smooth,fit))&&(!fit||smooth.margin>=held.margin-.25))return smooth;
  }
 }
 return best;
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
// Display-only easing. While the curve differs materially from the current
// ballistic solution the renderer labels it as updating, not a ready path.
struct CurveTracker {
 std::uint64_t target=0;double at=0;std::vector<Vec> points;bool settling=false;
 void reset(){*this={};}
 void update(const Solution& s,double gravity,std::uint64_t identity,double time){
  if(!s.valid||!identity){reset();return;}
  const bool fresh=target!=identity||points.size()!=65||time<at||time-at>.3;
  const double alpha=fresh?1:1-std::exp(-std::max(0.,time-at)/.075);
  if(fresh)points.resize(65);
  double error=0;
  for(size_t i=0;i<points.size();i++){auto desired=point(s,gravity,s.time*i/64.);points[i]=fresh?desired:points[i]+(desired-points[i])*alpha;error=std::max(error,length(points[i]-desired));}
  target=identity;at=time;settling=error>.6;
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
