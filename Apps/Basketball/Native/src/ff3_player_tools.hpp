#pragma once
#include "ff3_assist_logic.hpp"
#include <set>
namespace ff3Assist {
struct HeadPlatformGate {
 struct State{bool active=false,touched=false,blocked=false;double touchedAt=0;};std::map<std::uint64_t,State> states;std::set<std::uint64_t> expanded;
 bool allow(std::uint64_t id,Vec feet,Vec head,Vec original,double width,bool smart,double time=0,bool resetAfterContact=false,double delay=.5){
  if(!id||!ff3qb::finite(feet)||!ff3qb::finite(head)||!ff3qb::finite(original)||!std::isfinite(width)||width<3||width>20||!std::isfinite(time)||!std::isfinite(delay)||delay<.1||delay>3)return false;
  auto& state=states[id];const double gap=distance(feet,head),above=feet.y-head.y,clear=original.y*.5+.15;
  if(!smart){state={true,false,false,0};expanded.insert(id);return true;}
  if(gap>width*.5+2||above<-.6){state={};expanded.erase(id);return false;}
  if(state.blocked){if(above<=clear-.1)state.blocked=false;expanded.erase(id);return false;}
  if(!state.active&&above>=clear)state.active=true;
  if(state.active&&!state.touched&&above<=.55){state.touched=true;state.touchedAt=time;}
  if(state.active&&state.touched&&resetAfterContact&&time-state.touchedAt>=delay){state={false,false,true,0};expanded.erase(id);return false;}
  if(state.active)expanded.insert(id);else expanded.erase(id);return state.active;
 }
 void clear(){expanded.clear();states.clear();}
};
struct ContactBoostGate {
 std::uint64_t target=0;double last=-10;bool latched=false;
 bool request(std::uint64_t id,bool touching,double time){if(!touching){if(time-last>.15)latched=false;return false;}if(!id||!std::isfinite(time)||latched&&target==id||time-last<.55)return false;target=id;latched=true;last=time;return true;}
 void clear(){target=0;last=-10;latched=false;}
};
inline bool freshDivePress(double pressed,double time){return pressed>0&&std::isfinite(pressed)&&std::isfinite(time)&&time>=pressed&&time-pressed<=.18;}
inline Vec normalDiveVelocity(Vec current,Vec camera,double speed){auto heading=ff3qb::length(flat(current))>=2?unit(flat(current)):unit(flat(camera));auto value=heading*std::clamp(speed,24.,65.);value.y=current.y;return value;}
inline Vec flatHeadSize(double width){return {width,.35,width};}
inline Vec quickStep(Vec position,Vec forward,double distance,const ff3qb::FieldBounds* bounds=nullptr){
 if(!ff3qb::finite(position)||!ff3qb::finite(forward)||!std::isfinite(distance)||distance<1||distance>12||ff3qb::length(flat(forward))<.5)return {NAN,NAN,NAN};
 const auto target=position+unit(flat(forward))*distance;if(bounds&&(!bounds->valid()||!bounds->contains(target,1)))return {NAN,NAN,NAN};return target;
}
template<class Writer> inline bool relocateParts(const std::vector<std::pair<std::uint64_t,Vec>>& parts,Vec delta,Writer write){
 if(parts.empty()||parts.size()>24||!ff3qb::finite(delta)||ff3qb::length(delta)>12.001||std::abs(delta.y)>.01)return false;
 for(const auto& p:parts)if(!p.first||!ff3qb::finite(p.second))return false;
 size_t applied=0;for(;applied<parts.size();++applied)if(!write(parts[applied].first,parts[applied].second+delta))break;
 if(applied==parts.size())return true;while(applied){--applied;write(parts[applied].first,parts[applied].second);}return false;
}
struct ServerAutomation {
 std::uint32_t pid=0;std::uint64_t model=0;double joined=0,last=0,lowSince=0,next=0;
 void reset(){pid=0;model=0;joined=last=lowSince=next=0;}
 int choose(std::uint32_t p,std::uint64_t m,double time,bool practice,int population,int minimum,bool hop,bool change,int ownTeam,int otherTeam){
  if(p!=pid||m!=model){pid=p;model=m;joined=time;lowSince=0;next=time+2;return 0;}
  if(practice||!p||!m||!std::isfinite(time)||time<joined||population<1||minimum<1||minimum>100)return 0;
  if(time<next)return 0;next=time+2;if(time-joined<20||last>0&&time-last<120)return 0;
  if(hop&&population<minimum){if(!lowSince){lowSince=time;return 0;}if(time-lowSince>=6){last=time;lowSince=0;return 2;}}else lowSince=0;
  if(change&&ownTeam>=2&&otherTeam>=1&&ownTeam-otherTeam>=2){last=time;return 1;}return 0;
 }
};
}
