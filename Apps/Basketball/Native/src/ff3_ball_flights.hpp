#pragma once
#include "ff3_qb_logic.hpp"
namespace ff3qb {
struct BallObservation {std::uint64_t id=0;Vec position,velocity;double gravity=28;bool anchored=false;};
struct BallFlight {std::uint64_t id=0;Vec position,velocity;double gravity=28,at=0,born=0,quietAt=0,levelAt=0;bool ended=false;std::vector<Vec> trail;Vec measuredPosition{};double measuredAt=0,lastMotion=0;bool measured=false,airborne=false;};
struct FlightForecast {bool valid=false;double remaining=0;Vec landing;std::vector<Vec> path;};
inline FlightForecast forecast(const BallFlight& f,double groundY){
 FlightForecast out;if(!finite(f.position)||!finite(f.velocity)||!std::isfinite(groundY)||!std::isfinite(f.gravity)||f.gravity<1||f.gravity>300)return out;
 const double floor=groundY+.6,height=f.position.y-floor;
 if(height<-.1)return out;
 const double t=(f.velocity.y+std::sqrt(f.velocity.y*f.velocity.y+2*f.gravity*std::max(0.,height)))/f.gravity;
 if(!std::isfinite(t)||t<=.01||t>10)return out;
 out.valid=true;out.remaining=t;
 for(int i=0;i<=80;i++){double dt=t*i/80.;out.path.push_back(f.position+f.velocity*dt+Vec{0,-.5*f.gravity*dt*dt,0});}
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
   if(it==flights.end()){if(flights.size()>=64)continue;BallFlight f;f.id=o.id;f.position=f.measuredPosition=o.position;f.gravity=o.gravity;f.at=f.measuredAt=f.lastMotion=time;f.trail.push_back(o.position);flights.push_back(f);continue;}
   auto& f=*it;f.position=o.position;f.gravity=o.gravity;f.at=time;if(f.ended)continue;
   if(f.airborne&&f.measured&&f.velocity.y<0&&std::isfinite(groundY)&&o.position.y<=groundY+.7){f.ended=true;f.trail.clear();continue;}
   double dt=time-f.measuredAt;auto delta=o.position-f.measuredPosition;
   if(dt>=.035&&length(delta)>.005){
    if(dt>.3&&!f.airborne){f.measuredPosition=o.position;f.measuredAt=f.lastMotion=time;f.trail={o.position};continue;}
    if(dt>.3||length(delta)>std::max(15.,dt*250)){f.ended=true;f.trail.clear();continue;}
    // Position samples yield current velocity. The raw assembly velocity can
    // remain the launch value for script-driven footballs, including anchored ones.
    auto v=delta*(1/dt)+Vec{0,-.5*o.gravity*dt,0};
    const bool contact=f.measured&&f.velocity.y<-3&&delta.y>.05&&v.y>3;
    bool level=std::abs(delta.y)<.08&&std::abs(v.y)<2;
    if(!level)f.levelAt=0;else if(!f.levelAt)f.levelAt=time;
    bool supported=f.levelAt&&time-f.levelAt>(o.gravity>0?std::max(.15,2/o.gravity+.05):.3);
    if(f.airborne&&(contact||supported)){f.ended=true;f.trail.clear();continue;}
    f.velocity=f.measured?(f.velocity+Vec{0,-o.gravity*dt,0})*.6+v*.4:v;f.measured=true;f.measuredPosition=o.position;f.measuredAt=time;f.lastMotion=time;
    if(!f.airborne&&length(v)>4){f.airborne=true;f.born=time;}
    if(f.airborne){f.trail.push_back(o.position);if(f.trail.size()>300)f.trail.erase(f.trail.begin()+1);}
   }
   if(f.airborne&&(time-f.lastMotion>.3||time-f.born>10)){f.ended=true;f.trail.clear();}
  }
 }
 std::vector<BallFlight> visible()const{std::vector<BallFlight> out;for(const auto& f:flights)if(f.airborne&&!f.ended)out.push_back(f);return out;}
};
}
