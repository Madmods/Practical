#pragma once
#include <functional>
#include <cmath>
#include <algorithm>
namespace ff3qb {
// Only a physical-click request may enter an aim sequence. Production follows
// the game's real animation; previews never start a throw or camera correction.
struct FlickActions {
 std::function<bool()> allowed,power,capture,align,press,release,restore;
 std::function<void()> track;
};
struct FlickResult {bool sent=false,released=false,restored=false;};
enum class ReleaseDecision{Waiting,Released,Cancelled,TimedOut};
inline ReleaseDecision throwReleaseDecision(double elapsed,bool observed,bool cancelled){
 if(observed)return ReleaseDecision::Released;if(cancelled)return ReleaseDecision::Cancelled;
 return !std::isfinite(elapsed)||elapsed<0||elapsed>=3?ReleaseDecision::TimedOut:ReleaseDecision::Waiting;
}
inline double remainingAnimationDelay(double planned,double elapsed){return std::isfinite(planned)&&std::isfinite(elapsed)?std::max(0.,planned-std::max(0.,elapsed)):0;}
inline double learnAnimationDelay(double prior,double elapsed,double network){
 if(!std::isfinite(elapsed)||!std::isfinite(network)||elapsed<.08||elapsed>2||network<0||network>.3)return prior;
 return prior*.75+std::clamp(elapsed-network,.05,1.)*.25;
}
struct FlickCleanup {std::function<void()> run;~FlickCleanup(){run();}};
struct AnimatedAimActions {std::function<bool()> allowed,prepare,capture,restore;std::function<void()> follow;};
inline bool followPhysicalThrow(bool clicked,const AnimatedAimActions& action){
 if(!clicked||!action.allowed()||!action.prepare()||!action.capture())return false;
 FlickCleanup restore{[&](){action.restore();}};action.follow();return true;
}
inline FlickResult runFlick(bool clicked,const FlickActions& action){
 FlickResult result;
 if(!clicked||!action.allowed()||!action.power()||!action.capture())return result;
 {
  FlickCleanup restore{[&](){result.restored=action.restore();}};
  if(action.align()&&action.allowed()&&action.press()){
   result.sent=true;
   FlickCleanup release{[&](){result.released=action.release();}};
   action.track();
  }
 }
 return result;
}
}
