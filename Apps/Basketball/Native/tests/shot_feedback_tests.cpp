#include "shot_feedback.hpp"
#include "timing_reviewer.hpp"
#include <cassert>
#include <iostream>
using namespace shotFeedback;
int main(){
 assert(parseGrade("Perfect\nRelease")==Grade::Perfect);assert(parseGrade("Good Release")==Grade::Good);
 assert(parseGrade("Slightly Early Release")==Grade::Early);assert(parseGrade("Very Late Release")==Grade::Late);
 assert(parseGrade("0% Contested")==Grade::Unknown);assert(parseGrade("Not Perfect Release")==Grade::Unknown);
 assert(parseContest("0% Contested")==0);assert(parseContest("12.5% Contested")==12.5);
 for(auto s:{"Perfect Release",".% Contested","101% Contested","0%","0% Contested extra","-1% Contested"})assert(!std::isfinite(parseContest(s)));
 Tracker tracker;Frame f{1,2,true,1,.99,0,Grade::Perfect};Shot shot{1,1,1,.01,.1};
 assert(!tracker.observe(f,{},0,0).id);assert(!tracker.observe(f,shot,1,.05).id);
 f.alpha=.8;assert(!tracker.observe(f,shot,1,.11).id);
 auto result=tracker.observe(f,shot,1,.14);assert(result.id==1&&result.grade==Grade::Perfect&&result.meter==.99&&result.contest==0);
 assert(!tracker.observe(f,shot,1,.17).id);
 f.valid=false;tracker.observe(f,shot,1,.18);f.valid=true;f.alpha=1;tracker.observe(f,shot,1,.19);
 f.alpha=.8;tracker.observe(f,shot,1,.21);assert(!tracker.observe(f,shot,1,.24).id);
 // Same label on the next shot must count again, but only after a hidden reset.
 shot={2,2,1,.25,.3};f.alpha=1;tracker.observe(f,shot,2,.27);f.alpha=.8;tracker.observe(f,shot,2,.31);
 assert(tracker.observe(f,shot,2,.34).id==2);
 Tracker stale;f.alpha=.1;stale.observe(f,shot,2,.27);stale.observe(f,shot,2,.31);assert(!stale.observe(f,shot,2,.34).id);
 Tracker other;f.alpha=1;other.observe(f,shot,2,.27);f.alpha=.8;other.observe(f,shot,3,.31);assert(!other.observe(f,shot,3,.34).id); // next input invalidates correlation
 Tracker swapped;f.alpha=1;swapped.observe(f,shot,2,.27);f.owner=3;f.alpha=.8;swapped.observe(f,shot,2,.31);assert(!swapped.observe(f,shot,2,.34).id);f.owner=1;
 Tracker unstable;f.alpha=1;unstable.observe(f,shot,2,.27);f.alpha=.8;unstable.observe(f,shot,2,.31);f.meter=.8;assert(!unstable.observe(f,shot,2,.34).id);assert(unstable.observe(f,shot,2,.37).id==2);
 Tracker expired;f.alpha=1;expired.observe(f,shot,2,3.4);f.alpha=.8;expired.observe(f,shot,2,3.42);assert(!expired.observe(f,shot,2,3.45).id);
 timingReview::Reviewer reviewer;double t=10;for(int i=0;i<5;i++){t+=.5;reviewer.ping(t,30);}
 for(unsigned id=1;id<=3;id++){
  t+=.5;reviewer.ping(t,30);auto tuning=reviewer.prepare(t,2.5,37,1);
  reviewer.observe({id,t,.78,2.5,30,true,false,tuning});
  assert(!reviewer.gameResult({id,static_cast<Grade>(99),t+.05,0,.9}));
  assert(reviewer.view(t).gameFeedbackCount==id-1);
  assert(reviewer.gameResult({id,Grade::Late,t+.1,0,.9}));
  assert(!reviewer.gameResult({id,Grade::Late,t+.2,0,.9}));
 }
 auto view=reviewer.view(t+.1);assert(view.gameFeedbackCount==3&&view.gamePerfect==0&&view.feedbackCount==0&&view.windowShiftMs==-4);
 t+=.5;reviewer.ping(t,30);auto tuning=reviewer.prepare(t,2.5,37,1);reviewer.observe({4,t,.78,2.5,30,true,false,tuning});
 assert(reviewer.gameResult({4,Grade::Good,t+.1,0,.94}));assert(reviewer.view(t+.1).windowShiftMs==-4);
 t+=.5;reviewer.ping(t,30);tuning=reviewer.prepare(t,2.5,37,1);reviewer.observe({5,t,.78,2.5,30,true,false,tuning});
 assert(reviewer.gameResult({5,Grade::Perfect,t+.1,0,1}));view=reviewer.view(t+.1);
 assert(view.gamePerfect==1&&view.gameFeedbackCount==5&&view.perfect==0&&view.resultMeter==100&&view.resultContest==0);
 reviewer.reset();assert(!reviewer.gameResult({5,Grade::Perfect,t+.2,0,1}));
 auto traceResult=[](const std::vector<double>& values,bool gap=false){
  Tracker tr;Frame frame{1,2,true,1,0,0,Grade::Good};Shot s{1,1,1,10,10.05};
  tr.observe(frame,s,1,10.01);double at=10.05;
  for(double v:values){frame.meter=v;tr.observe(frame,s,1,at);at+=.015;}
  if(gap)at+=.07;
  tr.observe(frame,s,1,at);at+=.015;frame.alpha=.8;tr.observe(frame,s,1,at);at+=.03;
  return tr.observe(frame,s,1,at);
 };
 auto rising=traceResult({.5,.6,.7,.8,.9,.94,.94});assert(rising.id==1&&rising.phase==Phase::Rising);
 auto falling=traceResult({.6,.7,.8,.9,.99,.97,.94,.94});assert(falling.id==1&&falling.phase==Phase::Falling);
 // Actual230ms Good trace: the rendered peak was96.61%, not100%.
 assert(traceResult({.934849,.966116,.952828,.945265,.942479,.942479}).phase==Phase::Falling);
 assert(traceResult({.94,.94,.94}).phase==Phase::Unknown);
 assert(traceResult({.9,.901,.9,.901,.902,.901}).phase==Phase::Unknown);
 assert(traceResult({.5,.6,.7,.8,.9,.94},true).phase==Phase::Unknown);
 assert(traceResult({.8,.85,.90,.94,.939,.94}).phase==Phase::Rising);
 for(bool cap:{false,true})for(auto phase:{Phase::Rising,Phase::Falling}){
  timingReview::Reviewer r;double at=1;for(int i=0;i<5;i++){r.ping(at,30);at+=.5;}
  for(unsigned id=1;id<=3;id++){
   r.ping(at,30);auto tune=r.prepare(at,2.5,37,1);r.observe({id,at,.78,2.5,30,true,cap,tune});
   assert(r.gameResult({id,Grade::Good,at+.1,0,.94,phase}));assert(!r.gameResult({id,Grade::Good,at+.2,0,.94,phase}));at+=.5;
  }
  auto v=r.view(at-.4);const int correction=phase==Phase::Rising?4:-4;
  assert(v.windowShiftMs==correction&&v.gameFeedbackCount==3&&v.gamePerfect==0&&v.gameGrade=="Good");
 }
 // An uncertain Good breaks the streak; equal final percentages on opposite
 // phases must never be treated as identical evidence.
 timingReview::Reviewer mixed;double at=1;for(int i=0;i<5;i++){mixed.ping(at,30);at+=.5;}
 const Phase phases[]={Phase::Rising,Phase::Rising,Phase::Unknown,Phase::Rising,Phase::Falling,Phase::Falling};
 for(unsigned i=0;i<6;i++){mixed.ping(at,30);auto tune=mixed.prepare(at,2.5,37,1);mixed.observe({i+1,at,.78,2.5,30,true,false,tune});mixed.gameResult({i+1,Grade::Good,at+.1,0,.94,phases[i]});at+=.5;}
 assert(mixed.view(at-.4).windowShiftMs==0);
 // Missing or ineligible feedback between two pairs must not complete a streak.
 for(bool manual:{false,true}){
  timingReview::Reviewer interrupted;at=1;
  for(int i=0;i<5;i++){interrupted.ping(at,30);at+=.5;}
  for(unsigned id=1;id<=5;id++){
   interrupted.ping(at,30);auto tune=interrupted.prepare(at,2.5,37,1);
   interrupted.observe({id,at,.78,2.5,30,!(manual&&id==3),false,tune});
   if(id!=3||manual)interrupted.gameResult({id,Grade::Good,at+.1,0,.94,Phase::Rising});
   at+=.5;
  }
  assert(interrupted.view(at-.4).windowShiftMs==0);
 }
 timingReview::Reviewer limited;at=1;for(int i=0;i<5;i++){limited.ping(at,30);at+=.5;}
 for(unsigned i=1;i<=3;i++){limited.ping(at,30);auto tune=limited.prepare(at,2.5,37,1);limited.observe({i,at,.78,2.5,30,true,true,tune,77,81,0});limited.gameResult({i,Grade::Good,at+.1,0,.94,Phase::Rising});at+=.5;}
 assert(limited.view(at-.4).guardMs==0&&limited.view(at-.4).windowShiftMs==4); // No spare polling margin: move window, not safety budget.
 std::cout<<"Shot feedback parsing, correlation, deduplication, reset and automatic learning checks passed\n";
}
