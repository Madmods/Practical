#pragma once
#include "basketball_connection.hpp"

// Experience identity selects a module; titles and coincidental world objects
// never do. The Godot Football companion is a separate product, not FF3.
namespace gameModules {
enum class Id { None, Basketball, FootballFusion3 };
struct Module {Id id;const wchar_t* key;const wchar_t* name;std::uint64_t rootPlace,universe;bool gameplay;};
inline constexpr Module registry[]={
 {Id::Basketball,L"basketball",L"Practical Basketball",basketballConnection::PlaceId,basketballConnection::UniverseId,true},
 // Verified via https://apis.roblox.com/universes/v1/places/82866880824588/universe on 2026-11-13.
 {Id::FootballFusion3,L"ff3",L"Football Fusion 3",82866880824588ULL,9908641400ULL,true}
};
using Snapshot=basketballConnection::Snapshot;
inline const Module* identify(const Snapshot& s){
 if(!s.placeId||!s.universeId)return nullptr;
 // A known root with a conflicting universe is never accepted as another module.
 for(const auto& m:registry)if(s.placeId==m.rootPlace&&s.universeId!=m.universe)return nullptr;
 for(const auto& m:registry)if(s.universeId==m.universe)return &m;
 return nullptr;
}
inline bool compatible(const Snapshot& s){
 const auto* m=identify(s);if(!m)return false;
 if(m->id==Id::Basketball)return basketballConnection::compatible(s);
 // Workspace admission is separate from QB readiness. The QB reader verifies
 // possession, target, camera, power, physics, and focus before any input.
 return s.pid&&s.process&&s.version&&s.opened&&s.loaded&&s.model&&s.player&&s.world&&s.renderer;
}
inline bool ready(const Snapshot& s,double time){return basketballConnection::fresh(s,time)&&gameModules::compatible(s)&&s.stableSince>=0&&s.sampledAt-s.stableSince>=basketballConnection::StableSeconds;}
class Router {
 std::mutex mutex_;Snapshot snapshot_,admitted_;Id active_=Id::None;
 bool admitted(double t){return active_!=Id::None&&gameModules::ready(snapshot_,t)&&basketballConnection::sameSession(snapshot_,admitted_);}
public:
 void publish(Snapshot next){
  std::lock_guard<std::mutex> lock(mutex_);
  next.stableSince=gameModules::compatible(next)?(gameModules::compatible(snapshot_)&&basketballConnection::sameSession(snapshot_,next)&&basketballConnection::fresh(snapshot_,next.sampledAt)?snapshot_.stableSince:next.sampledAt):-10;
  snapshot_=next;if(!admitted(next.sampledAt))active_=Id::None;
 }
 Snapshot get(){std::lock_guard<std::mutex> lock(mutex_);return snapshot_;}
 Id open(double time,bool authenticated,bool releaseCurrent){
  std::lock_guard<std::mutex> lock(mutex_);active_=Id::None;
  if(authenticated&&releaseCurrent&&gameModules::ready(snapshot_,time)){admitted_=snapshot_;active_=identify(snapshot_)->id;}
  return active_;
 }
 Id active(double time,bool releaseCurrent=true){std::lock_guard<std::mutex> lock(mutex_);if(!releaseCurrent||!admitted(time))active_=Id::None;return active_;}
 void close(){std::lock_guard<std::mutex> lock(mutex_);active_=Id::None;}
};
inline Router router;
inline void publish(const Snapshot& s){basketballConnection::state.publish(s);router.publish(s);}
}
