#pragma once
#include "ff3_reach_edits.hpp"
namespace ff3Play {
struct BallPart {CatchPart part;Vec meshScale,position;bool canCollide=true;unsigned physicsGroup=0;};
inline bool magsFloorContact(const Scene& scene,const BallPart& ball){
 auto applied=scene.magReach.find(ball.part.id);if(applied==scene.magReach.end()||!scene.shape.measured||!ff3qb::finite(ball.position)||!ff3qb::finite(ball.part.size)||!std::isfinite(scene.groundY))return false;
 if(ball.position.y<scene.groundY-1||ball.position.y>scene.groundY+std::max(2.5,ball.part.size.y*.5+.75))return false;
 return catchGap(scene,ball.position,0,false,false,applied->second)<=.2;
}
inline unsigned char collisionFlag(unsigned char flags,unsigned char mask,bool on){return on?static_cast<unsigned char>(flags|mask):static_cast<unsigned char>(flags&~mask);}
inline bool sameBall(const BallPart& a,const BallPart& b){return a.part.id==b.part.id&&a.part.primitive==b.part.primitive&&a.part.mesh==b.part.mesh;}
inline bool validMeshScale(Vec v){return ff3qb::finite(v)&&std::min({v.x,v.y,v.z})>0&&std::max({v.x,v.y,v.z})<=100000;}
inline Vec ballMeshScale(const BallPart& original,Vec desired){return {original.meshScale.x*desired.x/original.part.size.x,original.meshScale.y*desired.y/original.part.size.y,original.meshScale.z*desired.z/original.part.size.z};}
inline constexpr size_t maxBallEdits=64;
struct BallEdits {
 enum class Field {Size,MeshScale};
 struct Edit {BallPart original;Vec applied,meshApplied;bool visible=true,sizeOwned=false,meshOwned=false,collisionOwned=false;unsigned groupApplied=0;bool groupOwned=false;};
 std::vector<Edit> edits;
 using Read=std::function<bool(const BallPart&,BallPart&)>;
 using Write=std::function<bool(const BallPart&,Field,Vec)>;
 using CollisionWrite=std::function<bool(const BallPart&,bool)>;
 using GroupWrite=std::function<bool(const BallPart&,unsigned)>;
 using GroupChoice=std::function<int(const BallPart&)>;
 bool restoreOne(Edit& e,const Read& read,const Write& write,const CollisionWrite& collisionWrite={},const GroupWrite& groupWrite={}){
  BallPart live;if(!read(e.original,live))return false;
  if(live.part.id!=e.original.part.id||live.part.primitive!=e.original.part.primitive){e.sizeOwned=e.meshOwned=e.collisionOwned=e.groupOwned=false;return true;}
  bool ok=true,normalSize=sameSize(live.part.size,e.original.part.size);
  if(e.collisionOwned&&live.canCollide&&!normalSize){if(!collisionWrite||!collisionWrite(e.original,false))return false;live.canCollide=false;}
  if(e.sizeOwned){if(!sameSize(live.part.size,e.applied))e.sizeOwned=false;else if(write(e.original,Field::Size,e.original.part.size)){e.sizeOwned=false;normalSize=true;}else ok=false;}
  if(e.meshOwned){if(live.part.mesh!=e.original.part.mesh||!sameSize(live.meshScale,e.meshApplied))e.meshOwned=false;else if(write(e.original,Field::MeshScale,e.original.meshScale))e.meshOwned=false;else ok=false;}
  // Solid collision can return only after geometry restoration has succeeded.
  if(e.collisionOwned){if(live.canCollide)e.collisionOwned=false;else if(ok&&(normalSize||!e.original.canCollide)){if(!e.original.canCollide||(collisionWrite&&collisionWrite(e.original,true)))e.collisionOwned=false;else ok=false;}else ok=false;}
  // Restore the original group only after normal geometry is confirmed.
  if(e.groupOwned){if(live.physicsGroup!=e.groupApplied)e.groupOwned=false;else if(ok&&normalSize&&groupWrite&&groupWrite(e.original,e.original.physicsGroup))e.groupOwned=false;else ok=false;}
  return ok;
 }
 bool restore(const Read& read,const Write& write,const CollisionWrite& collisionWrite={},const GroupWrite& groupWrite={}){
  bool ok=true;for(auto it=edits.begin();it!=edits.end();){if(restoreOne(*it,read,write,collisionWrite,groupWrite))it=edits.erase(it);else{ok=false;++it;}}return ok;
 }
 bool apply(const std::vector<BallPart>& parts,Vec desired,bool visible,const Read& read,const Write& write,const CollisionWrite& collisionWrite={},const GroupWrite& groupWrite={},const GroupChoice& chooseGroup={}){
  if((groupWrite&&!chooseGroup)||(groupWrite&&collisionWrite))return false;
  if(parts.size()>maxBallEdits||!validCatchSize(desired))return false;
  for(size_t i=0;i<parts.size();++i){const auto& p=parts[i].part;if(!p.ball||!p.id||!p.primitive||!p.mesh)return false;for(size_t j=0;j<i;++j)if(parts[j].part.id==p.id)return false;}
  bool ok=true;
  for(auto it=edits.begin();it!=edits.end();){bool retained=std::any_of(parts.begin(),parts.end(),[&](const auto& p){return sameBall(p,it->original);})&&sameSize(it->applied,desired)&&it->visible==visible;if(retained){++it;continue;}if(restoreOne(*it,read,write,collisionWrite,groupWrite))it=edits.erase(it);else{ok=false;++it;}}
  auto reject=[&](std::uint64_t id){ok=false;auto it=std::find_if(edits.begin(),edits.end(),[&](const auto& e){return e.original.part.id==id;});if(it!=edits.end()&&restoreOne(*it,read,write,collisionWrite,groupWrite))edits.erase(it);};
  for(const auto& seed:parts){
   auto it=std::find_if(edits.begin(),edits.end(),[&](const auto& e){return e.original.part.id==seed.part.id;});if(it!=edits.end()&&(!sameBall(it->original,seed)||!sameSize(it->applied,desired)||it->visible!=visible)){ok=false;continue;}BallPart live;
   if(!read(seed,live)||!sameBall(seed,live)||!ff3qb::finite(live.part.size)||!validMeshScale(live.meshScale)){reject(seed.part.id);continue;}
   if(it==edits.end()){
    if(edits.size()>=maxBallEdits||std::min({live.part.size.x,live.part.size.y,live.part.size.z})<=0||std::max({live.part.size.x,live.part.size.y,live.part.size.z})>16){ok=false;continue;}
    auto scale=visible?ballMeshScale(live,desired):live.meshScale;if(!validMeshScale(scale)){ok=false;continue;}
    // Stage every intended field before the first write, for one durable snapshot.
    edits.push_back({live,desired,scale,visible,!sameSize(live.part.size,desired),visible&&!sameSize(live.meshScale,scale),bool(collisionWrite)&&live.canCollide});it=edits.end()-1;
    if(groupWrite){int chosen=chooseGroup(live);if(chosen<0||chosen>31||live.physicsGroup>31){reject(seed.part.id);continue;}it->groupApplied=unsigned(chosen);it->groupOwned=live.physicsGroup!=it->groupApplied;}
   }
   if(groupWrite){
    int chosen=chooseGroup(it->original);
    if(chosen<0||chosen>31||it->original.physicsGroup>31){reject(seed.part.id);continue;}
    it->groupApplied=unsigned(chosen);
    if(live.physicsGroup!=it->groupApplied){it->groupOwned=true;if(!groupWrite(it->original,it->groupApplied)){reject(seed.part.id);continue;}}
    BallPart guarded;if(!read(it->original,guarded)||!sameBall(it->original,guarded)||guarded.physicsGroup!=it->groupApplied){reject(seed.part.id);continue;}live=guarded;
   }
   if(collisionWrite&&live.canCollide){it->collisionOwned=true;if(!collisionWrite(it->original,false)){reject(seed.part.id);continue;}}
   if(collisionWrite){BallPart guarded;if(!read(it->original,guarded)||!sameBall(it->original,guarded)||guarded.canCollide){reject(seed.part.id);continue;}live=guarded;}
   if(!sameSize(live.part.size,desired)){it->sizeOwned=true;if(!write(it->original,Field::Size,desired)){reject(seed.part.id);continue;}}
   // Roll back immediately if replication resets collision during resizing.
   if(collisionWrite){BallPart guarded;if(!read(it->original,guarded)||!sameBall(it->original,guarded)||guarded.canCollide){reject(seed.part.id);continue;}}
   if(groupWrite){BallPart guarded;if(!read(it->original,guarded)||!sameBall(it->original,guarded)||guarded.physicsGroup!=it->groupApplied||chooseGroup(it->original)!=int(it->groupApplied)){reject(seed.part.id);continue;}}
   if(visible&&!sameSize(live.meshScale,it->meshApplied)){it->meshOwned=true;if(!write(it->original,Field::MeshScale,it->meshApplied)){reject(seed.part.id);continue;}}
  }return ok;
 }
 bool applyFilteredGeometry(const std::vector<BallPart>& parts,Vec desired,const Read& read,const Write& write,const GroupWrite& groupWrite,const GroupChoice& chooseGroup){if(!groupWrite||!chooseGroup)return false;return apply(parts,desired,false,read,write,{},groupWrite,chooseGroup);}
 bool applyGeometry(const std::vector<BallPart>& parts,Vec desired,const Read& read,const Write& write,const CollisionWrite& collisionWrite){if(!collisionWrite)return false;return apply(parts,desired,false,read,write,collisionWrite);}
};
inline std::vector<BallPart> eligibleFilteredBalls(const std::vector<BallPart>& parts,const BallEdits::Read& read,const BallEdits::GroupChoice& choose,unsigned& skipped){
 std::vector<BallPart> eligible;skipped=0;
 for(const auto& seed:parts){BallPart live;if(!read(seed,live)||!sameBall(seed,live)||choose(live)<0){++skipped;continue;}eligible.push_back(live);}
 return eligible;
}
// Shared runtime gate: restoring takes priority over any new ball modification.
template<class Editor>
bool updateBallMags(Editor& editor,const Scene& scene,const std::vector<BallPart>& balls,bool enabled,double time,bool restoreAllOnFailure=true){
 if(!enabled||!backgroundObserving(scene,time))return editor.restore();
 if(editor.apply(scene,balls))return true;
 if(restoreAllOnFailure)editor.restore();return false;
}
}
