#pragma once
#include "shooting_logic.hpp"
#include <cstdint>
namespace pump {
enum Status {Off,NeedBall,Ready,Watching,Jumped,Moving,Covered,Opening,Shooting,Expired,NeedTiming,Unavailable};
struct Config{bool enabled=false;double range=12,delay=.12,maxContest=10;};
struct Observation{double time=0,tapAt=0;uint64_t ball=0,tapBall=0,target=0;bool focused=false,timing=false,stationary=false,fresh=false;double distance=999,height=0,verticalSpeed=0,otherContest=100;unsigned tap=0;uint64_t tapTarget=0;double tapHeight=0;bool tapGrounded=false;};
struct Decision{Status state=Off;bool shoot=false;uint64_t target=0;};
struct Logic{
 unsigned consumed=0;uint64_t locked=0,ball=0;double fakeAt=0,baseline=0,jumpAt=-1,clearAt=-1;bool waiting=false,committed=false;Status state=Off;
 Decision step(const Config&c,const Observation&o){
  auto out=[&](Status s,bool fire=false){state=s;return Decision{s,fire,locked};};
  if(!c.enabled||!o.focused){waiting=false;locked=0;consumed=o.tap;return out(Off);}
  if(!o.fresh){waiting=false;locked=0;consumed=o.tap;return out(Unavailable);}
  if(!o.ball){waiting=false;locked=0;consumed=o.tap;return out(NeedBall);}
  if(!o.timing){waiting=false;consumed=o.tap;return out(NeedTiming);}
  if(o.tap!=consumed){consumed=o.tap;waiting=false;locked=0;committed=false;clearAt=jumpAt=-1;
   if(o.tapAt>0&&o.time-o.tapAt<.2&&o.ball==o.tapBall&&o.target&&o.target==o.tapTarget&&o.distance<=c.range&&o.tapGrounded){waiting=true;locked=o.target;ball=o.ball;fakeAt=o.tapAt;baseline=o.tapHeight;}
  }
  if(!waiting){if(o.time-fakeAt>1.5)committed=false;return out(committed?Shooting:Ready);}
  if(o.ball!=ball||o.target!=locked||o.time-fakeAt>1.25||o.distance>c.range+2){waiting=false;locked=0;return out(Expired);}
  const double rise=o.height-baseline;
  if(jumpAt<0){if(o.verticalSpeed>3&&rise>.55)jumpAt=o.time;else return out(Watching);}
  if(rise<.15&&o.time-jumpAt>.12){waiting=false;return out(Expired);}
  if(o.time-fakeAt<.18||o.time-jumpAt<c.delay||rise<.85||o.verticalSpeed>-.5)return out(Jumped);
  if(!o.stationary){clearAt=-1;return out(Moving);}
  if(o.otherContest>c.maxContest){clearAt=-1;return out(Covered);}
  if(clearAt<0)clearAt=o.time;
  if(o.time-clearAt<.04)return out(Opening);
  waiting=false;committed=true;return out(Shooting,true);
 }
};
}
