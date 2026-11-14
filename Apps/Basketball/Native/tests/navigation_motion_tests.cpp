#include "navigation_motion.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
int main(){
 assert(navigationLabelOpacity(0)==0);
 assert(navigationLabelOpacity(.1f)>.09f);
 assert(navigationLabelOpacity(.5f)==.5f);
 assert(navigationLabelOpacity(1)==1);
 assert(navigationContentOffset(0)==10);
 assert(navigationContentOffset(.5f)==5);
 assert(navigationContentOffset(1)==0);
 // Fixed header hit boundary cannot flip groups as the drawer grows.
 for(float p:{.78f,.85f,.95f,1.f}){
  assert(navigationHeaderGroup(p,110,81)==0);
  assert(navigationHeaderGroup(p,120,81)==1);
 }
 assert(navigationHeaderGroup(.5f,120,81)==-1);
 for(float fps:{60.f,120.f,240.f}){
  NavigationMotion m;float dt=1/fps,prior=0;
  for(int i=0;i<(int)std::ceil(.22f*fps);i++){float v=m.update(true,dt);assert(v>=prior&&v<=1);prior=v;}
  assert(m.value>.999f);
  for(int i=0;i<(int)std::ceil(.18f*fps);i++){float v=m.update(false,dt);assert(v<=prior&&v>=0);prior=v;}
  assert(m.value<.001f);
 }
 NavigationMotion m;m.update(true,.05f);assert(m.value>.1f&&m.value<.25f);float prior=m.value;
 float v=m.update(false,.002f);assert(v<prior&&prior-v<.05f);
 prior=v;v=m.update(true,.002f);assert(v>prior&&v-prior<.05f);
 assert(m.update(false,.001f,true)==0);assert(m.update(true,.001f,true)==1);
 std::cout<<"Navigation motion: response, completion, reversal and reduced-motion checks passed\n";
}
