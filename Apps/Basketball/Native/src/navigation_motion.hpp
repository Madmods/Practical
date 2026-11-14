#pragma once
#include <algorithm>
inline constexpr float NavigationExpandedWidth=232.f;
inline float navigationLabelOpacity(float amount){return std::clamp(amount,0.f,1.f);}
inline float navigationContentOffset(float amount){return 10*(1-std::clamp(amount,0.f,1.f));}
inline int navigationHeaderGroup(float amount,float x,float y){
 if(amount<.78f||y<64||y>98||x<12||x>NavigationExpandedWidth-12)return -1;
 return x<NavigationExpandedWidth/2?0:1;
}
struct NavigationMotion {
 float value=0,start=0,target=0,elapsed=0,duration=.22f;
 float update(bool open,float dt,bool reduced=false){
  float next=open?1.f:0.f;
  if(next!=target){start=value;target=next;elapsed=0;duration=open?.22f:.18f;}
  if(reduced){value=start=target;elapsed=duration;return value;}
  elapsed=std::min(duration,elapsed+std::max(0.f,dt));
  float progress=elapsed/duration,eased=progress*progress*(3-2*progress);
  value=start+(target-start)*eased;return value;
 }
};
