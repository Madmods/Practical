#pragma once
#include "ff3_qb_logic.hpp"
namespace ff3qb {
struct BallObservation {std::uint64_t id=0;Vec position,velocity;double gravity=28;bool anchored=false;};
struct BallFlight {std::uint64_t id=0;Vec position,velocity;double gravity=28,at=0,born=0,quietAt=0,levelAt=0;bool ended=false;std::vector<Vec> trail;};
struct FlightTracker {
 std::vector<BallFlight> flights;
 void clear(){flights.clear();}
 void update(const std::vector<BallObservation>& observations,double time){
  if(!std::isfinite(time))return;
  flights.erase(std::remove_if(flights.begin(),flights.end(),[&](const BallFlight& f){return time<f.at||time-f.at>.5||std::none_of(observations.begin(),observations.end(),[&](const BallObservation& o){return o.id==f.id;});}),flights.end());
  for(const auto& o:observations){
   if(!o.id||!finite(o.position)||!finite(o.velocity)||length(o.velocity)>300||!std::isfinite(o.gravity)||o.gravity<0||o.gravity>300)continue;
   auto it=std::find_if(flights.begin(),flights.end(),[&](const BallFlight& f){return f.id==o.id;});
   if(it==flights.end()){if(flights.size()>=64||o.anchored||length(o.velocity)<4)continue;flights.push_back({o.id,o.position,o.velocity,o.gravity,time,time});it=flights.end()-1;}
   auto& f=*it;double dt=time-f.at;
   if(dt<0||dt>.5){f.ended=true;f.trail.clear();}
   bool quiet=length(o.velocity)<2&&length(o.position-f.position)<.1;
   if(!quiet)f.quietAt=0;else if(!f.quietAt)f.quietAt=time;
   // A rolling landing may retain horizontal speed. Sustained level motion
   // longer than a gravity-limited apex also ends the airborne trail.
   bool level=std::abs(o.velocity.y)<.75&&std::abs(o.position.y-f.position.y)<.08;
   if(!level)f.levelAt=0;else if(!f.levelAt)f.levelAt=time;
   bool supported=f.levelAt&&time-f.levelAt>(o.gravity>0?std::max(.12,1.5/o.gravity+.05):.25);
   const bool contact=dt>0&&f.velocity.y<-3&&o.velocity.y-f.velocity.y>std::max(6.,o.gravity*dt);
   if(o.anchored||contact||supported||(f.quietAt&&time-f.quietAt>.18)||time-f.born>10)f.ended=true;
   f.position=o.position;f.velocity=o.velocity;f.gravity=o.gravity;f.at=time;
   if(f.ended){f.trail.clear();continue;}
   if(f.trail.empty()||length(f.trail.back()-o.position)>.35)f.trail.push_back(o.position);
   if(f.trail.size()>96)f.trail.erase(f.trail.begin());
  }
 }
 std::vector<BallFlight> visible()const{std::vector<BallFlight> out;for(const auto& f:flights)if(!f.ended)out.push_back(f);return out;}
};
}
