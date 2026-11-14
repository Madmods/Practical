#pragma once
#include <cstdint>
#include <cmath>
#include <mutex>
#include <atomic>

namespace basketballConnection {
inline constexpr std::uint64_t PlaceId=80681221431821ULL;
// Roblox's public place-to-universe API maps the supplied root place and its
// court place 137269396533485 to this experience (verified 2026-11-13).
inline constexpr std::uint64_t UniverseId=7529591378ULL;
inline constexpr double FreshSeconds=1.25, StableSeconds=.75;
// The legacy renderer keeps its old behavior; the Basketball WebView entry point
// explicitly enables this gate before starting any worker.
inline std::atomic<bool> required{false};
inline std::atomic<bool> entered{false};
inline std::atomic<bool> releaseCurrent{false};
struct Snapshot {
 double sampledAt=-10,stableSince=-10;
 std::uint64_t placeId=0,universeId=0,model=0,character=0;
 unsigned pid=0;
 bool process=false,version=false,opened=false,loaded=false,player=false;
 bool characterReady=false,world=false,meter=false,renderer=false;
};
inline bool targetGame(const Snapshot& s){return s.placeId!=0&&s.universeId==UniverseId;}
inline bool sameSession(const Snapshot& a,const Snapshot& b){return a.pid==b.pid&&a.model==b.model&&a.character==b.character&&a.placeId==b.placeId&&a.universeId==b.universeId;}
inline bool compatible(const Snapshot& s){return s.process&&s.version&&s.opened&&s.loaded&&s.model&&targetGame(s)&&s.player&&s.characterReady&&s.character&&s.world&&s.meter&&s.renderer;}
inline bool fresh(const Snapshot& s,double now){return std::isfinite(now)&&std::isfinite(s.sampledAt)&&now>=s.sampledAt&&now-s.sampledAt<=FreshSeconds;}
inline bool ready(const Snapshot& s,double now){return fresh(s,now)&&compatible(s)&&s.stableSince>=0&&s.sampledAt-s.stableSince>=StableSeconds;}
class Store {
 std::mutex mutex_;Snapshot value_;
public:
 void publish(Snapshot next){
  std::lock_guard<std::mutex> lock(mutex_);
  if(compatible(next))next.stableSince=compatible(value_)&&sameSession(value_,next)&&fresh(value_,next.sampledAt)?value_.stableSince:next.sampledAt;
  else next.stableSince=-10;
  value_=next;
 }
 Snapshot get(){std::lock_guard<std::mutex> lock(mutex_);return value_;}
 bool readyAt(double time){return ready(get(),time);}
};
inline Store state;
}
