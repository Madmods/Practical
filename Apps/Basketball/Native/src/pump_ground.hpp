#pragma once
#include <cmath>
// A horizontal court estimate. Freeze during jumps, including the zero-speed apex.
// Reacquire a stable new level after a teleport/platform change.
namespace pump {
struct GroundPlane {
 bool valid=false;float y=0,lastCandidate=0;double stableSince=-1,lastAt=-1;
 float update(float candidate,float vertical,double t,float fallback=NAN){
  if(!std::isfinite(candidate))return valid?y:fallback;
  if(!valid){y=std::abs(vertical)<1.5f?candidate:(std::isfinite(fallback)?fallback:candidate);valid=true;}
  bool stable=std::abs(vertical)<1.5f&&(lastAt<0||std::abs(candidate-lastCandidate)<.06f);
  if(!stable)stableSince=-1;else if(stableSince<0)stableSince=t;
  if(candidate<y+.18f&&stable)y=candidate;
  else if(stableSince>=0&&t-stableSince>.35)y=candidate;
  lastCandidate=candidate;lastAt=t;return y;
 }
};
}
