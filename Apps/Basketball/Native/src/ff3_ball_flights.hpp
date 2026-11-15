#pragma once
#include "ff3_qb_logic.hpp"
namespace ff3qb {
struct BallObservation {std::uint64_t id=0;Vec position,velocity;double gravity=28;bool anchored=false;};
struct FlightSample {double at=0;Vec position;};
struct FlightModel {
 bool valid=false;double at=0,gravity=0,error=0;Vec origin,velocity;double startAt=0;
 Vec pointAt(double time)const{double t=time-at;return origin+velocity*t+Vec{0,-.5*gravity*t*t,0};}
 double observedTime(Vec position)const{double speed2=velocity.x*velocity.x+velocity.z*velocity.z;return at+(speed2>1e-6?((position.x-origin.x)*velocity.x+(position.z-origin.z)*velocity.z)/speed2:0);}
};
inline FlightModel fitFlight(const std::vector<FlightSample>& samples,double gravity){
 FlightModel m;if(samples.size()<7||gravity<1||gravity>300)return m;
 const double span=samples.back().at-samples.front().at;if(span<.3||span>1)return m;
 // Fit the spatial parabola y(u), where u is horizontal progress. Replication
 // timestamps can stall or bunch up; they must not determine launch velocity.
 auto first=samples.front().position,last=samples.back().position;
 Vec axis{last.x-first.x,0,last.z-first.z};const double distance=length(axis);if(distance<8)return m;axis=axis*(1/distance);
 double matrix[3][4]{};
 for(const auto& s:samples){double u=dot(s.position-first,axis)/distance,basis[3]{1,u,u*u};for(int i=0;i<3;i++){for(int j=0;j<3;j++)matrix[i][j]+=basis[i]*basis[j];matrix[i][3]+=basis[i]*(s.position.y-first.y);}}
 for(int k=0;k<3;k++){int pivot=k;for(int i=k+1;i<3;i++)if(std::abs(matrix[i][k])>std::abs(matrix[pivot][k]))pivot=i;if(std::abs(matrix[pivot][k])<1e-8)return m;for(int j=k;j<4;j++)std::swap(matrix[k][j],matrix[pivot][j]);double scale=matrix[k][k];for(int j=k;j<4;j++)matrix[k][j]/=scale;for(int i=0;i<3;i++)if(i!=k){double factor=matrix[i][k];for(int j=k;j<4;j++)matrix[i][j]-=factor*matrix[k][j];}}
 const double a=matrix[0][3],b=matrix[1][3],c=matrix[2][3];if(c>=-.02)return m;
 const double speed=distance*std::sqrt(-gravity/(2*c));if(!std::isfinite(speed)||speed<2||speed>180)return m;
 m.at=samples.back().at;m.startAt=m.at-distance/speed;m.gravity=gravity;
 m.origin={last.x,first.y+a+b+c,last.z};m.velocity=axis*speed+Vec{0,(b+2*c)*speed/distance,0};
 double squared=0;for(const auto& s:samples){auto delta=s.position-m.pointAt(m.observedTime(s.position));squared+=dot(delta,delta);}
 m.error=std::sqrt(squared/samples.size());m.valid=finite(m.origin)&&finite(m.velocity)&&length(m.velocity)<180&&m.error<.35;return m;
}
struct BallFlight {std::uint64_t id=0;Vec position,velocity;double gravity=28,at=0,born=0,quietAt=0,levelAt=0;bool ended=false;std::vector<Vec> trail;Vec measuredPosition{};double measuredAt=0,lastMotion=0;bool measured=false,airborne=false;std::vector<FlightSample> samples;FlightModel model;double mismatchAt=0,fieldY=NAN;};
struct FlightForecast {bool valid=false;double remaining=0;Vec landing;std::vector<Vec> path;};
inline FlightForecast forecast(const BallFlight& f,double groundY){
 FlightForecast out;if(!f.model.valid||!std::isfinite(groundY)||!std::isfinite(f.gravity)||std::abs(f.gravity-f.model.gravity)>.01)return out;
 const auto position=f.model.pointAt(f.at),velocity=f.model.velocity+Vec{0,-f.gravity*(f.at-f.model.at),0};
 const double floor=(std::isfinite(f.fieldY)?f.fieldY:groundY)+.6,height=position.y-floor;
 if(height<-.1)return out;
 const double t=(velocity.y+std::sqrt(velocity.y*velocity.y+2*f.gravity*std::max(0.,height)))/f.gravity;
 if(!std::isfinite(t)||t<=.01||t>10)return out;
 out.valid=true;out.remaining=t;
 // Draw one complete world-space arc; advancing the ball does not slide or
 // redraw the curve from its tip. Only the remaining-time value changes.
 const double end=f.at+t,start=std::min(f.model.startAt,f.model.at);
 for(int i=0;i<=80;i++)out.path.push_back(f.model.pointAt(start+(end-start)*i/80.));
 out.landing=out.path.back();return out;
}
struct FlightTracker {
 std::vector<BallFlight> flights;
 void clear(){flights.clear();}
 void update(const std::vector<BallObservation>& observations,double time,double groundY=NAN){
  if(!std::isfinite(time))return;
  flights.erase(std::remove_if(flights.begin(),flights.end(),[&](const BallFlight& f){return time<f.at||time-f.at>.5||std::none_of(observations.begin(),observations.end(),[&](const BallObservation& o){return o.id==f.id;});}),flights.end());
  for(const auto& o:observations){
   if(!o.id||!finite(o.position)||!std::isfinite(o.gravity)||o.gravity<0||o.gravity>300)continue;
   auto it=std::find_if(flights.begin(),flights.end(),[&](const BallFlight& f){return f.id==o.id;});
   if(it==flights.end()){if(flights.size()>=64)continue;BallFlight f;f.id=o.id;f.position=f.measuredPosition=o.position;f.gravity=o.gravity;f.at=f.measuredAt=f.lastMotion=time;f.trail.push_back(o.position);f.samples.push_back({time,o.position});flights.push_back(f);continue;}
   auto& f=*it;f.position=o.position;f.gravity=o.gravity;f.at=time;if(f.ended)continue;
   if(f.airborne&&f.measured&&f.velocity.y<0&&std::isfinite(groundY)&&o.position.y<=groundY+.7){f.ended=true;f.trail.clear();continue;}
   double dt=time-f.measuredAt;auto delta=o.position-f.measuredPosition;
   if(dt>=.035&&length(delta)>.005){
    if(dt>.3&&!f.airborne){f.measuredPosition=o.position;f.measuredAt=f.lastMotion=time;f.trail={o.position};f.samples={{time,o.position}};continue;}
    if(dt>.3||length(delta)>std::max(15.,dt*250)){f.ended=true;f.trail.clear();continue;}
    // Position samples yield current velocity. The raw assembly velocity can
    // remain the launch value for script-driven footballs, including anchored ones.
    auto v=delta*(1/dt)+Vec{0,-.5*o.gravity*dt,0};
    const bool contact=f.measured&&f.velocity.y<-6&&delta.y>.1&&v.y>6;
    bool level=std::abs(delta.y)<.08&&std::abs(v.y)<2;
    if(!level)f.levelAt=0;else if(!f.levelAt)f.levelAt=time;
    bool supported=f.levelAt&&time-f.levelAt>(o.gravity>0?std::max(.15,2/o.gravity+.05):.3);
    if(f.airborne&&(contact||supported)){f.ended=true;f.trail.clear();continue;}
    f.velocity=f.measured?(f.velocity+Vec{0,-o.gravity*dt,0})*.6+v*.4:v;f.measured=true;f.measuredPosition=o.position;f.measuredAt=time;f.lastMotion=time;
    f.samples.push_back({time,o.position});while(f.samples.size()>24||(!f.samples.empty()&&time-f.samples.front().at>.65))f.samples.erase(f.samples.begin());
    if(f.model.valid){
     const double deviation=length(o.position-f.model.pointAt(f.model.observedTime(o.position)));
     const double tolerance=1.0;
     if(std::abs(o.gravity-f.model.gravity)>.01||deviation>tolerance){
      if(!f.mismatchAt)f.mismatchAt=time;
      if(deviation>18||time-f.mismatchAt>.08){f.model={};f.samples={{time,o.position}};f.trail={o.position};f.mismatchAt=0;}
     }else f.mismatchAt=0;
    }else{f.model=fitFlight(f.samples,o.gravity);if(f.model.valid&&std::isfinite(groundY))f.fieldY=groundY;}
    if(f.model.valid){f.velocity=f.model.velocity+Vec{0,-o.gravity*(time-f.model.at),0};if(!std::isfinite(f.fieldY)&&std::isfinite(groundY))f.fieldY=groundY;}
    if(!f.airborne&&length(v)>4){f.airborne=true;f.born=time;}
    if(f.airborne){f.trail.push_back(o.position);if(f.trail.size()>300)f.trail.erase(f.trail.begin()+1);}
   }
   if(f.airborne&&(time-f.lastMotion>.3||time-f.born>10)){f.ended=true;f.trail.clear();}
  }
 }
 std::vector<BallFlight> visible()const{std::vector<BallFlight> out;for(const auto& f:flights)if(f.airborne&&!f.ended)out.push_back(f);return out;}
};
}
