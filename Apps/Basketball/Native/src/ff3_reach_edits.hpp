#pragma once
#include "ff3_play_logic.hpp"
#include <functional>
#include <map>
namespace ff3Play {
struct CatchPart {std::uint64_t id=0,rig=0,primitive=0;Vec size;bool ball=false;std::uint64_t mesh=0;bool guide=false;};
inline bool sameSize(Vec a,Vec b){return ff3qb::finite(a)&&ff3qb::finite(b)&&ff3qb::length(a-b)<.0001;}
inline constexpr double maxMagSize=50;
inline bool validCatchSize(Vec size){return ff3qb::finite(size)&&std::min({size.x,size.y,size.z})>=1&&std::max({size.x,size.y,size.z})<=maxMagSize;}
inline Vec catchSize(int shape,double size,Vec custom){
 if(shape==3)return custom;
 if(shape==1)return {size,std::max(1.,size*.5),size};
 if(shape==2)return {std::max(1.,size*.5),size,std::max(1.,size*.5)};
 return {size,size,size};
}
struct BallDelay {
 std::map<std::uint64_t,double> seen;
 std::vector<std::uint64_t> ready(const std::vector<std::uint64_t>& ids,double time,int milliseconds){
  if(!std::isfinite(time)||milliseconds<0||milliseconds>500)return {};
  for(auto it=seen.begin();it!=seen.end();)if(std::find(ids.begin(),ids.end(),it->first)==ids.end())it=seen.erase(it);else ++it;
  std::vector<std::uint64_t> out;for(auto id:ids)if(id){auto [entry,inserted]=seen.emplace(id,time);if(time<entry->second)entry->second=time;if(time-entry->second+1e-8>=milliseconds*.001&&std::find(out.begin(),out.end(),id)==out.end())out.push_back(id);}return out;
 }
};
struct ReachEdits {
 struct Edit {CatchPart original;Vec applied;};std::vector<Edit> edits;
 using Read=std::function<bool(const CatchPart&,CatchPart&)>;using Write=std::function<bool(const CatchPart&,Vec)>;
 bool restore(const Read& read,const Write& write){
  bool ok=true;for(auto it=edits.begin();it!=edits.end();){CatchPart live;
   if(!read(it->original,live)){ok=false;++it;continue;}
   if(live.id!=it->original.id||live.rig!=it->original.rig||live.primitive!=it->original.primitive||!sameSize(live.size,it->applied)){it=edits.erase(it);continue;}
   if(write(live,it->original.size)){it=edits.erase(it);}else{ok=false;++it;}
  }return ok;
 }
 bool apply(const std::vector<CatchPart>& parts,Vec desired,const Read& read,const Write& write){
  if(parts.size()<2||parts.size()>8||!validCatchSize(desired))return false;
  bool changed=edits.size()!=parts.size();
  for(const auto& part:parts){auto e=std::find_if(edits.begin(),edits.end(),[&](const Edit& e){return part.id==e.original.id&&part.rig==e.original.rig&&part.primitive==e.original.primitive;});if(e==edits.end()||!sameSize(e->applied,desired))changed=true;}
  if(changed&&!restore(read,write))return false;
  if(!edits.empty()){
   for(const auto& e:edits){CatchPart live;if(!read(e.original,live)||live.primitive!=e.original.primitive||live.rig!=e.original.rig||!sameSize(live.size,e.applied)){restore(read,write);return false;}}
   return true;
  }
  for(const auto& part:parts){CatchPart live;
   if(!read(part,live)||live.id!=part.id||live.rig!=part.rig||live.primitive!=part.primitive||!live.id||!live.rig||!live.primitive||!ff3qb::finite(live.size)||std::min({live.size.x,live.size.y,live.size.z})<=0||std::max({live.size.x,live.size.y,live.size.z})>16){restore(read,write);return false;}
   Edit e{live,desired};edits.push_back(e);
   if(!write(live,e.applied)){restore(read,write);return false;}
  }return true;
 }
 bool apply(const std::vector<CatchPart>& parts,double studs,const Read& read,const Write& write){return apply(parts,Vec{studs,studs,studs},read,write);}
};
}
