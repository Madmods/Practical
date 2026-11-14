#include "shooting_logic.hpp"
#include "timing_reviewer.hpp"
#include <cassert>
#include <iostream>
int main(){
 shooting::PingFilter ping;double t=1;
 for(int i=0;i<5;i++){assert(ping.add(t,25));t+=.5;}
 for(int next:{90,175,30,95,25}){assert(ping.add(t,next));assert(ping.value()==next);t+=.5;}
 assert(!ping.add(t,NAN));assert(!ping.add(t,1000));assert(!ping.add(t-1,30));assert(ping.value()==25);
 assert(!ping.fresh(t+2));assert(ping.add(t+3,38));assert(ping.value()==38);
 ping.reset();assert(!ping.fresh(t));assert(ping.value()==0);
 for(int i=0;i<5;i++){ping.add(t,24+i);t+=.5;}assert(ping.value()==26);
 // Actual 175ms late-shot onset: idle zero caused a false 0.5 slope.
 shooting::MeterTrend trace;
 for(int i=0;i<192;i++)trace.add(i*.002,0);
 const double values[]={.0677675,.106562,.165301,.195086,.240897,.273102,.305179,.360446,.391915,.445868,.477956,.526888};
 bool captured=false;
 for(int i=0;i<12;i++){
  trace.add(.384+i*.016,values[i]);
  if(!captured&&trace.captureReady()){
   assert(trace.estimate.rate>2.1&&trace.estimate.rate<3.1);
   auto w=shooting::pingWindow(175,trace.estimate.rate,true);assert(w.maximum<51&&values[i]<.5);captured=true;
  }
 }
 assert(captured);
 for(double required:{.006,.01,.016,.025,.1})for(int learned:{-12,-2,0,2,20}){
  double margin=shooting::reviewMargin(required,learned);assert(margin>=required&&margin<=.1);
 }
 assert(shooting::reviewMargin(.006,-2)<shooting::reviewMargin(.006,0));
 assert(shooting::reviewMargin(.025,-12)==.025);
 // Reproduce the high-ping fit deadline: finish measuring before reaching
 // the233ms send window, rather than requiring a fixed100ms span.
 shooting::MeterTrend highPing;bool earlyFit=false;
 for(int i=0;i<40;i++){double value=.04+i*.01;highPing.add(1+i*.004,value);if(highPing.captureReady(233)){assert(value<.31);earlyFit=true;break;}}
 assert(earlyFit);
 // Fast animations cannot wait a fixed100ms beyond their entire rising phase.
 for(double rate:{8.,12.})for(int fps:{144,360}){
  shooting::MeterTrend fast;bool ready=false;
  for(int f=0;f<fps;f++){
   const double phase=rate*f/fps;if(phase>=.95)break;fast.add(1.+double(f)/fps,phase);
   if(fast.captureReady()){ready=true;break;}
  }
  assert(ready);
 }
 int onsets=0;
 for(double delay:{.02,.08,.14,.19,.25,.4,.6})for(double rate:{1.5,2.5,4.})for(int fps:{30,60,144,360}){
  shooting::MeterTrend trend;bool ready=false;int frame=-1;
  for(int i=0;i<600;i++){
   double stamp=i*.002,phase=stamp-delay;int f=phase<0?-1:int(phase*fps);
   if(f!=frame||phase<0){frame=f;trend.add(stamp,phase<0?0:std::min(1.,rate*f/fps));}
   if(trend.captureReady()){assert(std::abs(trend.estimate.rate-rate)<rate*.08);ready=true;break;}
  }
  assert(ready);++onsets;
 }
 timingReview::Reviewer review;t=10;unsigned id=0;
 for(int p:{25,95,175,25}){
  for(int n=0;n<5;n++){review.ping(t,p);t+=.5;}
  auto tune=review.prepare(t-.1,2.5,37,1);assert(tune.eligible);
  review.observe({++id,t-.1,.78,2.5,p,true,false,tune});
  review.ping(t,p==25?95:25);assert(!review.usable(tune,t));assert(!review.feedback(id,timingReview::Grade::Late,t));t+=.5;
 }
 std::cout<<onsets<<" delayed-animation onset cases, recorded late-shot replay, immediate ping-step and reviewer transition checks passed\n";
}
