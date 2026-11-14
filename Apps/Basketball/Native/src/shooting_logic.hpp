#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
#include <deque>
#include <utility>
namespace shooting {
// The standalone/manual decision logic retains its existing uncapped default.
inline constexpr int BasketballAutoMaximum=81;
inline constexpr int BasketballAutoMinimum=77;
inline constexpr double PollSeconds=.002;
inline double reviewMargin(double required,int correctionMs){return std::clamp(std::max(.016,required)+std::clamp(correctionMs,-12,20)*.001,required,.1);}
// Keep ordinary small-jitter smoothing, but never average a new latency regime
// with the previous one for another 1-2 seconds after a substantial ping step.
class PingFilter {
 std::deque<double> values;double last=-1;int filtered=0;
public:
 void reset(){values.clear();last=-1;filtered=0;}
 bool add(double t,double value){
  if(!std::isfinite(t)||!std::isfinite(value)||value<1||value>=1000||(!values.empty()&&t<=last))return false;
  if(!values.empty()&&(t-last>2||std::abs(value-filtered)>20))values.clear();
  values.push_back(value);while(values.size()>5)values.pop_front();last=t;
  std::vector<double> sorted(values.begin(),values.end());std::sort(sorted.begin(),sorted.end());
  filtered=int(std::lround((sorted[(sorted.size()-1)/2]+sorted[sorted.size()/2])*.5));return true;
 }
 int value()const{return filtered;}
 bool fresh(double t)const{return !values.empty()&&std::isfinite(t)&&t>=last&&t-last<=2;}
};
struct AutoWindow {double minimum=BasketballAutoMinimum,maximum=BasketballAutoMaximum;bool valid=false,outOfRange=false;};
// User calibration is 77-81% at 20-30ms. Preserve that entire baseline band.
// Use the engine's existing empirical full-ping model for the delta only;
// this is not an assertion that network RTT equals one-way input latency.
inline AutoWindow pingWindow(int ping,double rate,bool fresh){
 if(!fresh||ping<1||ping>1000||!std::isfinite(rate)||rate<.1||rate>20)return {};
 const int delta=ping<20?ping-20:ping>30?ping-30:0;
 const double shift=rate*delta*.1;
 AutoWindow w{BasketballAutoMinimum-shift,BasketballAutoMaximum-shift,true,false};
 if(w.minimum<0||w.maximum>100){w.valid=false;w.outOfRange=true;}
 return w;
}
// Positive calibration delays release. Preserve window width and leave room
// below the turnaround; never make an invalid ping model valid by clipping it.
inline AutoWindow calibratedWindow(AutoWindow base,double rate,int shiftMs){
 if(!base.valid||!std::isfinite(rate)||rate<.1||rate>20||!std::isfinite(base.minimum)||!std::isfinite(base.maximum)||base.minimum<0||base.maximum>100||base.minimum>base.maximum)return {};
 if(!shiftMs)return base;
 const double width=base.maximum-base.minimum;
 if(width>99)return {};
 const double shift=std::clamp(rate*std::clamp(shiftMs,-40,40)*.1,-base.minimum,99-base.maximum);
 base.minimum+=shift;base.maximum+=shift;return base;
}
inline double calibratedLead(double base,int shiftMs){return std::max(0.,base-std::clamp(shiftMs,-40,40)*.001);}
struct Options {bool automatic=false,assist=false,holdEarly=true,protectFall=true,closeout=false;int minimum=80,maximum=100,maxPing=1000,freshness=120,trim=40,overholdLead=40;double autoMaximum=100,autoMinimum=0;bool autoWindowValid=true,calibrated=false;};
struct Sample {double fill=0,rate=0,age=0,lead=0;bool pending=false,falling=false,threat=false;int ping=30;double pollAhead=.016;};
inline bool deferEarlyRelease(bool enabled,bool hold,bool active,bool falling,double fill,double age,int minimum,int freshness){return enabled&&hold&&active&&!falling&&fill>=0&&fill<minimum*.01&&age>=0&&age<=freshness*.001;}
enum class Release {None,Auto,Early,Maximum,Falling,Closeout,AutoMaximum};
inline Release decide(const Options&o,const Sample&s){
 if(!std::isfinite(s.age)||s.age<0||!std::isfinite(s.fill)||!std::isfinite(s.lead)||s.lead<0||s.age>o.freshness*.001||s.fill<0||s.fill>1.05||!std::isfinite(s.rate)||std::abs(s.rate)<.1||std::abs(s.rate)>20)return Release::None;
 // An observed floor wins over every predictive/assisted decision while Auto
 // Time is on. Network lead, learned margin and extrapolation cannot bypass it.
 // Physical key-up and emergency cleanup are handled outside this decision.
 if(o.automatic&&(!o.autoWindowValid||!std::isfinite(o.autoMinimum)||!std::isfinite(o.autoMaximum)||o.autoMinimum<0||o.autoMaximum>100||o.autoMinimum>o.autoMaximum||s.fill<o.autoMinimum*.01))return Release::None;
 // Manual-release limits use observed fill; automatic timing and overhold use latency prediction.
 const double predicted=s.fill+s.rate*(s.age+s.lead),lo=o.minimum*.01,hi=o.assist?o.maximum*.01:1.0;
 // Auto Time's outgoing deadline is independent of the optional manual window.
 // Forecast the next poll, not network delivery: this ceiling is a send limit.
 // If an update skips the deadline, release now instead of holding indefinitely.
 if(o.automatic&&o.autoMaximum>0&&o.autoMaximum<100&&s.ping<=o.maxPing&&!s.falling&&s.rate>0&&
    std::isfinite(s.pollAhead)&&s.pollAhead>=0&&s.pollAhead<=.1&&
    s.fill+s.rate*(s.age+s.pollAhead)+1e-9>=o.autoMaximum*.01)return Release::AutoMaximum;
 if(o.assist&&s.pending&&s.fill+1e-6>=lo)return Release::Early;
 // Manual assistance must not silently force a calibrated automatic shot
 // back to its old maximum. Physical pending releases above remain honored.
 if(o.assist&&!(o.automatic&&o.calibrated)){
  if(!s.falling&&o.maximum<100&&s.fill+1e-6>=hi)return Release::Maximum;
  if(o.protectFall&&o.minimum==100&&s.fill>=.999)return Release::Maximum;
  const double protectLead=s.age+std::clamp(s.lead+.012+o.overholdLead*.001,.016,.5);
  // Sending only after the observed turnaround can be too late: the game keeps
  // advancing the meter while the key-up is delivered/applied. Forecast the
  // peak and the subsequent floor crossing from the current rising trend.
  // Overhold must be allowed to send before the observed minimum: otherwise
  // that gate defeats latency compensation regardless of the protection lead.
  // Use a bounded arrival forecast for this gate, independently of the deadline.
  // Pending manual releases still require observed fill above the minimum.
  const double arrivalHorizon=std::clamp(s.age,0.,1./60.)+std::clamp(s.lead,0.,.060);
  const double risingArrival=s.fill+std::max(0.,s.rate)*arrivalHorizon;
  if(o.protectFall&&!s.pending&&!s.falling&&o.maximum==100&&o.minimum<100&&s.rate>.1&&risingArrival>=std::max(.80,lo)){
   double toPeak=std::max(0.,(1-s.fill)/s.rate),toFloor=(1-lo)/(s.rate*1.15);
   if(toPeak+toFloor<=protectLead)return Release::Falling;
  }
  if(o.protectFall&&s.falling&&s.rate<-.1&&s.fill+s.rate*protectLead<=lo)return Release::Falling;

 }
 if(o.automatic&&(!o.assist||o.calibrated||s.fill+1e-6>=lo)&&s.ping<=o.maxPing&&!s.falling&&predicted>=1)return Release::Auto;
 if(o.closeout&&s.threat&&!s.falling&&s.fill>=std::max(.90,o.assist?lo:0.)&&s.fill<=hi)return Release::Closeout;
 return Release::None;
}
struct Descent {
 double peak=0,previous=0,previousAt=0,rate=0;int drops=0;bool initialized=false,falling=false;
 void reset(){*this={};}
 void add(double t,double fill){
  if(!initialized){initialized=true;peak=previous=fill;previousAt=t;return;}
  if(std::abs(fill-previous)<.00001)return;
  double dt=t-previousAt,delta=fill-previous;
  if(fill>peak){peak=fill;drops=0;}
  if(delta<0&&dt>=.002&&dt<=.20){double v=delta/dt;if(v> -20&&v<-.1){rate=drops?std::min(rate*.35+v*.65,v):v;++drops;}}
  else if(delta>.001&&!falling)drops=0;
  if((drops>=2&&peak-fill>=.0039)||(drops>=1&&peak-fill>=.008))falling=true;
  previous=fill;previousAt=t;
 }
};
// A new input can begin while the previous shot's meter is fading to zero.
// Do not carry that descent into the new shot's first rising samples.
struct MeterTrend {
 std::deque<std::pair<double,double>> samples;
 Descent descent;
 struct Estimate{double rate=0,span=0;size_t points=0;bool valid=false;} estimate;
 bool active=false;
 bool captureReady(int ping=25)const{
  if(!active||!estimate.valid)return false;
  double span=std::clamp(.6/estimate.rate,.05,.1);
  const auto w=pingWindow(ping,estimate.rate,true);
  // At high ping the send deadline can arrive before a full100ms fit. Keep
  // at least50ms of actual rising data, but do not wait past the modeled floor.
  if(w.valid)span=std::min(span,std::clamp((w.minimum*.01-samples.front().second)/estimate.rate-.008,.05,.1));
  return estimate.span>=span&&samples.back().second>=.2;
 }
 void reset(){samples.clear();descent.reset();estimate={};active=false;}
 void fit(){
  estimate={};estimate.points=samples.size();
  if(samples.size()<4)return;
  estimate.span=samples.back().first-samples.front().first;
  std::vector<double> slopes;slopes.reserve(samples.size()*samples.size()/2);
  for(size_t i=0;i<samples.size();++i)for(size_t j=i+1;j<samples.size();++j){
   double dt=samples[j].first-samples[i].first;if(dt>=.05)slopes.push_back((samples[j].second-samples[i].second)/dt);
  }
  if(slopes.size()<3)return;
  auto middle=slopes.begin()+slopes.size()/2;std::nth_element(slopes.begin(),middle,slopes.end());double rate=*middle;
  if(slopes.size()%2==0)rate=(rate+*std::max_element(slopes.begin(),middle))*.5;
  estimate.rate=rate;estimate.valid=std::isfinite(rate)&&rate>=.1&&rate<=20;
 }
 bool add(double t,double fill){
  if(!std::isfinite(t)||!std::isfinite(fill)||fill<0||fill>1.05){reset();return false;}
  if(!samples.empty()&&(t<samples.back().first||t-samples.back().first>.2))reset();
  if(!samples.empty()&&std::abs(fill-samples.back().second)<=.00001)return false;
  // The zero before animation starts is not a rising-meter sample. Including
  // that idle/network wait produces a false low first slope, frozen for the
  // whole shot by consumers. Begin the fit at the first observed positive fill.
  if(!active&&!samples.empty()&&samples.back().second<=.00001&&fill>.00001){samples.clear();descent.reset();}
  if(!active&&!samples.empty()&&fill<samples.back().second){samples.clear();descent.reset();}
  if(!samples.empty()&&t-samples.back().first<.002)samples.pop_back();
  samples.emplace_back(t,fill);
  // Retain time coverage, not nine frames. At 144+ FPS nine samples cannot
  // supply the 50ms-separated pairs the rate estimator requires.
  while(samples.size()>4&&(t-samples.front().first>.14||samples.size()>80))samples.pop_front();
  active=active||(samples.size()>=3&&samples.back().second-samples.front().second>.01);
  if(active)descent.add(t,fill);else descent.reset();
  fit();
  return true;
 }
};
struct Vec {float x=0,y=0,z=0;Vec operator+(Vec b)const{return{x+b.x,y+b.y,z+b.z};}Vec operator-(Vec b)const{return{x-b.x,y-b.y,z-b.z};}Vec operator*(float s)const{return{x*s,y*s,z*s};}};
inline float length(Vec v){return std::sqrt(v.x*v.x+v.z*v.z);}
inline Vec cap(Vec v,float max=30){v.y=0;float n=length(v);return n>max?v*(max/n):v;}
inline double pressure(double d){double c=std::clamp((12-d)/10.,0.,1.),w=std::clamp((5-d)/3.,0.,1.);return std::clamp(std::pow(c,1.6)*(1+4.3*3*w)*std::pow(.07,std::max(0.,d-6.5)),0.,1.);}
struct Defender {Vec position,velocity;float speed=16;};
inline double estimate(Vec origin,Vec velocity,std::vector<Defender> list,double horizon){
 velocity=cap(velocity);double t=0;
 for(auto&d:list){d.velocity=cap(d.velocity);d.speed=std::clamp(std::max(d.speed,length(d.velocity)),16.f,30.f);}
 while(t<horizon){float dt=(float)std::min(.04,horizon-t);for(auto&d:list){if(t>=.18){Vec delta=origin+velocity*(float)t-d.position;float n=length(delta);Vec want=n>.01f?delta*(std::min(d.speed,n/dt)/n):Vec{};d.velocity=cap(d.velocity+cap(want-d.velocity,60*dt),d.speed);}d.position=d.position+d.velocity*dt;}t+=dt;}
 std::vector<double> scores;for(auto&d:list)scores.push_back(pressure(length(d.position-(origin+velocity*(float)horizon))));std::sort(scores.rbegin(),scores.rend());double total=0;for(size_t i=0;i<scores.size();i++)total=i?total+(1-total)*scores[i]*.25:scores[i];return std::clamp(total*87,0.,100.);
}
}
