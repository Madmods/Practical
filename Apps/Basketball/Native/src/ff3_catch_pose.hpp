#pragma once
#include "ff3_contact_reach.hpp"
namespace ff3Assist {
struct CatchPose {bool measured=false,handsOut=false;Vec left,right;std::array<ContactReach,2> contacts;};
inline bool catchFormation(Vec left,Vec right,Vec torso){
 if(!ff3qb::finite(left)||!ff3qb::finite(right)||!ff3qb::finite(torso)||torso.x<.5||torso.y<.5||torso.z<.3||std::max({torso.x,torso.y,torso.z})>6)return false;
 auto extended=[&](Vec hand){return std::abs(hand.x)<torso.x*.5+3&&hand.y>-.55&&hand.y<torso.y*.5+3.5&&hand.z<.6&&(hand.z<-.55||hand.y>torso.y*.5+.2);};
 return extended(left)&&extended(right);
}
struct CatchPoseGate {
 bool open=false;double lastOut=0;
 bool update(const CatchPose& pose,double time){
  if(!pose.measured||!std::isfinite(time)){clear();return false;}
  if(pose.handsOut){open=true;lastOut=time;}
  else if(time<lastOut||time-lastOut>.04)open=false;
  return open;
 }
 void clear(){open=false;lastOut=0;}
};
}
