#include "rolling_logic.hpp"
#include <cassert>
#include <cstdio>
int main(){
 using namespace rolling;
 Plan raw;raw.valid=true;raw.point={10,0,0};raw.time=1;PlanDisplay display;
 display.update(1,raw,10,{0,0,0});raw.point.x=10.04f;auto quiet=display.update(1,raw,10.01,{1,0,0});assert(quiet.point.x==10&&quiet.start.x==1);
 assert(display.update(1,{},10.06,{2,0,0}).valid);assert(!display.update(1,{},10.2,{}).valid);
 raw.point.x=20;auto switched=display.update(2,raw,10.21,{});assert(switched.point.x==20);assert(!display.update(0,{},10.22,{}).valid);
 InterceptMotion fit;PlanDisplay visual;Ball truth{{10,5,0},{11,-7,2},0,45,0,.75f,.55f};Runner runner{{0,0,8},20,4.5f};Plan previous;double maxJump=0;int missing=0,valid=0;double lastSolve=-10;Plan solved;
 // Game updates at 60 Hz while the renderer polls at 240 Hz; include tiny
 // position quantization noise and several floor rebounds.
 for(int i=0;i<960;i++){
  double t=i/240.;float gameTime=(i/4)/60.f;auto position=at(truth,gameTime);position.x+=((i/4)%3-1)*.002f;
  fit.update(10+t,position,45,0,.75f);Plan next;
  if(fit.valid){if(t-lastSolve>=1./60){solved=make(fit.ball,runner,{});lastSolve=t;}next=solved;}
  auto shown=visual.update((fit.valid||fit.waiting)?1:0,next,10+t,runner.p);
  if(i>40){if(shown.valid){valid++;if(previous.valid)maxJump=std::max(maxJump,(double)flatLength(shown.point-previous.point));}else missing++;}
  previous=shown;
 }
 printf("Bounce replay: visible=%d missing=%d max rendered step=%.3f studs\n",valid,missing,maxJump);
 assert(valid>850);assert(missing<15);assert(maxJump<.35);
 puts("Prediction deadband, rebound continuity, stale clearing and target-switch checks passed");
}
