#pragma once
#include "ff3_ball_edits.hpp"
namespace ff3Play {
struct SnapContact {
 std::uint64_t id=0,primitive=0,own=0;Vec position,velocity,size;bool touch=false;Vec inward;
 bool valid(const Scene& s)const{return id&&primitive&&own==s.own&&ff3qb::finite(position)&&ff3qb::finite(velocity)&&ff3qb::length(velocity)<=180&&ff3qb::finite(size)&&std::min({size.x,size.y,size.z})>=.2&&std::max({size.x,size.y,size.z})<=6&&ff3qb::length(position-s.head)<10;}
};
inline double snapDistance(Vec ball,Vec hand){return ff3qb::finite(ball)&&ff3qb::finite(hand)?ff3qb::length(ball-hand):INFINITY;}
inline constexpr double maxSnapSize=maxMagSize+2;
inline double combinedSnapSize(double ballSize){return std::isfinite(ballSize)&&ballSize>=1&&ballSize<=maxMagSize?ballSize+2:NAN;}
inline double snapRadius(double fullSize){return std::isfinite(fullSize)&&fullSize>=1&&fullSize<=maxSnapSize?fullSize*.5:NAN;}
inline bool withinSnapDistance(Vec ball,Vec hand,double fullSize){const double radius=snapRadius(fullSize);return std::isfinite(radius)&&snapDistance(ball,hand)<=radius;}
inline Vec snapPalmPosition(Vec center,Vec size,Vec localUp,bool r6){
 if(!ff3qb::finite(center)||!ff3qb::finite(size)||std::min({size.x,size.y,size.z})<.2||std::max({size.x,size.y,size.z})>6||!ff3qb::finite(localUp)||std::abs(ff3qb::length(localUp)-1)>.05)return {NAN,NAN,NAN};
 return center+localUp*(r6?-size.y*.5+.15:0);
}
inline Vec snapHandTarget(const SnapContact& hand){const double distance=ff3qb::length(hand.inward);const double inset=std::min(.25,.25*std::min({hand.size.x,hand.size.y,hand.size.z}));return distance>.01&&ff3qb::finite(hand.inward)?hand.position+hand.inward*(inset/distance):hand.position;}
struct SnapBall {BallPart part;Vec velocity;std::uint64_t thrower=0;bool anchored=false;};
enum class SnapPhase {Idle,Preparing,Transfer,Confirm};
struct SnapCommand {bool move=false;BallPart ball;Vec destination,velocity;};
struct HandSnap {
 SnapPhase phase=SnapPhase::Idle;std::uint32_t pid=0;std::uint64_t model=0,own=0,ball=0,primitive=0,parent=0;
 double phaseAt=0,armedAt=-1,lastMoveAt=0,lastSampleAt=0,resultAt=-10;
 SnapBall tracked;Vec lastDestination;unsigned attempts=0,writes=0,writeFailures=0,contacts=0,possessions=0,unconfirmed=0,cancelled=0,corrections=0;
 std::string result="Hand Snap: waiting for a nearby loose football";
 bool busy()const{return phase!=SnapPhase::Idle;}
 void cancel(const std::string& why="Hand Snap paused"){if(busy())++cancelled;phase=SnapPhase::Idle;ball=primitive=parent=0;armedAt=-1;tracked={};result=why;}
 static bool eligible(const Scene& s,const SnapBall& b,const SnapContact& hand,double range){
  const auto& p=b.part.part;return p.ball&&p.id&&p.primitive&&p.rig&&p.mesh&&b.thrower!=s.player&&b.thrower!=s.own&&ff3qb::finite(b.part.position)&&ff3qb::finite(b.velocity)&&ff3qb::length(b.velocity)<=200&&ff3qb::finite(p.size)&&std::min({p.size.x,p.size.y,p.size.z})>.1&&std::max({p.size.x,p.size.y,p.size.z})<=maxMagSize&&withinSnapDistance(b.part.position,hand.position,range);
 }
 void finish(bool caught,double time,const std::string& why){if(caught)++possessions;else ++unconfirmed;phase=SnapPhase::Idle;ball=primitive=parent=0;armedAt=-1;lastMoveAt=lastSampleAt=0;tracked={};resultAt=time;result=why;}
 SnapCommand step(const Scene& s,const std::vector<SnapBall>& balls,const SnapContact& hand,double range,double time,bool /*catchInputHeld*/,std::uint64_t heldBall,bool enabled=true){
  SnapCommand out;
  if(pid!=s.pid||model!=s.model||own!=s.own){cancel();pid=s.pid;model=s.model;own=s.own;}
  if(!enabled||!observing(s,time)||!std::isfinite(range)||range<1||range>maxSnapSize){cancel(enabled?"Hand Snap paused: focus, menu or stale character":"Hand Snap off");return out;}
  if(busy()&&heldBall){finish(heldBall==ball,time,heldBall==ball?"Hand Snap: game possession observed":"Hand Snap ended: another football was acquired");return out;}
  if(s.possessed||heldBall){if(time-resultAt>=2)result="Hand Snap paused while holding a football";return out;}
  if(!hand.valid(s)){cancel("Hand Snap: verified right-hand catch part unavailable");return out;}
  if(!busy()){
   const SnapBall* selected=nullptr;double best=snapRadius(range)+.0001;
   for(const auto& candidate:balls){if(!eligible(s,candidate,hand,range))continue;const double gap=snapDistance(candidate.part.position,hand.position);if(gap<best){best=gap;selected=&candidate;}}
   if(!selected){if(time-resultAt>2)result="Hand Snap: waiting for an eligible ball in snap distance";return out;}
   tracked=*selected;ball=selected->part.part.id;primitive=selected->part.part.primitive;parent=selected->part.part.rig;phase=SnapPhase::Preparing;phaseAt=time;armedAt=-1;++attempts;result="Hand Snap: selecting loose football";
  }
  auto current=std::find_if(balls.begin(),balls.end(),[&](const auto& b){return b.part.part.id==ball&&b.part.part.primitive==primitive&&b.part.part.rig==parent;});
  if(current==balls.end()){
   if(phase!=SnapPhase::Confirm){phase=SnapPhase::Confirm;phaseAt=time;result="Hand Snap: ball identity unavailable; checking possession";}
   if(time-phaseAt>=.3){finish(false,time,"Hand Snap: selected ball left loose tracking");}return out;
  }
  if(!eligible(s,*current,hand,range)){finish(false,time,"Hand Snap: ball left snap distance or became ineligible");return out;}
  tracked=*current;
  if(phase==SnapPhase::Confirm){phase=SnapPhase::Preparing;phaseAt=time;armedAt=-1;}
  if(phase==SnapPhase::Preparing){
   // Position transfers never request catch input and are independent of
   // the game's brief catch-contact window or any physical catch button.
   phase=SnapPhase::Transfer;phaseAt=time;lastMoveAt=lastSampleAt=0;
  }
  if(phase==SnapPhase::Transfer){
   if(lastMoveAt>0&&time-lastMoveAt>=.004&&time>lastSampleAt){lastSampleAt=time;if(ff3qb::length(current->part.position-lastDestination)<1.)++contacts;else ++corrections;}
   result="Hand Snap: position transfers active; waiting for game possession";
   out.move=true;out.ball=current->part;out.destination=snapHandTarget(hand);out.velocity=hand.velocity;
  }return out;
 }
 void issued(const SnapCommand& command,double time,bool ok){if(!command.move)return;if(ok){++writes;lastMoveAt=time;lastDestination=command.destination;}else{++writeFailures;result="Hand Snap: local write unavailable; retrying while the ball is eligible";}}
};
inline std::uint64_t snapPossessedBall(std::uint64_t equipped,std::uint64_t backpack,const HandSnap& snap){return equipped?equipped:snap.busy()&&backpack==snap.ball?backpack:0;}
}
