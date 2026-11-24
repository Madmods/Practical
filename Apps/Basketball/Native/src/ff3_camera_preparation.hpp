#pragma once
#include <atomic>
#include <cstdint>
namespace ff3qb {
struct PointerMovement {
 long x=0,y=0;bool observed=false;
 bool update(long nextX,long nextY){const bool moved=!observed||nextX!=x||nextY!=y;x=nextX;y=nextY;observed=true;return moved;}
};
// Preparation is independent of the real click. The click only records which
// prepared frame belongs to the game's own throw; it never schedules a replay.
class CameraPreparation {
 std::atomic<bool> ready_{false};std::atomic<std::uint64_t> click_{0},resume_{0};
public:
 bool ready()const{return ready_.load();}std::uint64_t clicked()const{return click_.load();}
 void pauseUntil(std::uint64_t tick){ready_=false;auto prior=resume_.load();while(prior<tick&&!resume_.compare_exchange_weak(prior,tick)){};}
 bool canPrepare(std::uint64_t tick)const{return tick>=resume_.load();}
 bool shot(std::uint64_t tick){const bool prepared=ready_.exchange(false);click_=tick;pauseUntil(tick+500);return prepared;}
 void clear(){ready_=false;}
 bool acknowledge(std::uint64_t expectedClick){if(click_.load()!=expectedClick)return false;ready_=true;if(click_.load()!=expectedClick){ready_=false;return false;}return true;}
};
}
