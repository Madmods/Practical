#include "shooting_logic.hpp"
#include "timing_reviewer.hpp"
#include <cassert>
#include <iostream>
#include <limits>
using shotFeedback::Grade;
struct Sent {double time,fill;shooting::Release reason;shooting::AutoWindow window;};
// Deterministic rendered-meter fixture, with a separate 2 ms input polling
// clock. This tests the controller, not the live game's scoring or network.
Sent release(int fps,int ping,double rate,const timingReview::Tuning& tune,int trim=37,bool assist=true){
 auto w=shooting::calibratedWindow(shooting::pingWindow(ping,rate,true),rate,tune.windowShiftMs);
 shooting::Options o;o.automatic=true;o.assist=assist;o.minimum=67;o.maximum=81;
 o.autoMinimum=w.minimum;o.autoMaximum=w.maximum;o.autoWindowValid=w.valid;o.calibrated=tune.eligible;
 double lead=shooting::calibratedLead(.0192117+(ping+trim+tune.adjustmentMs)*.001,tune.windowShiftMs);
 for(int tick=1;tick<1000;tick++){
  const double t=tick*.002,frame=std::floor(t*fps+1e-9)/fps,fill=frame*rate;
  auto result=shooting::decide(o,{fill,rate,t-frame,lead,false,false,false,ping,shooting::reviewMargin(.006,tune.guardMs)});
  if(result!=shooting::Release::None)return {t,fill,result,w};
 }
 assert(false);return {};
}
struct Fixture {
 timingReview::Reviewer r;double t=1;unsigned id=0;int ping=25;
 Fixture(){for(int i=0;i<5;i++){r.ping(t,ping);t+=.5;}}
 timingReview::Tuning tune(){r.ping(t,ping);return r.prepare(t,2.5,37,1);}
 void result(Grade grade,bool automatic=true,double contest=0,bool ceiling=true){
  auto q=tune();r.observe({++id,t,.78,2.5,ping,automatic,ceiling,q});
  assert(r.gameResult({id,grade,t+.1,contest,.94}));t+=.5;
 }
 int shift(){return r.view(t-.4).windowShiftMs;}
};
int main(){
 // Reproduce the original slider problem, then prove actual release moves.
 timingReview::Tuning initial{0,0,1,true,0};
 const auto a=release(240,25,2.5,initial,37),b=release(240,25,2.5,initial,20);
 assert(a.reason==shooting::Release::AutoMaximum&&a.time==b.time);
 auto later=initial;later.windowShiftMs=16;
 auto c=release(240,25,2.5,later);
 assert(c.time>a.time+.010&&c.window.maximum>81&&c.fill>.81);
 assert(c.reason==shooting::Release::AutoMaximum||c.reason==shooting::Release::Auto);
 auto earlier=initial;earlier.windowShiftMs=-16;assert(release(240,25,2.5,earlier).time<a.time-.010);
 // Validity, width, clipping, and startup behavior.
 auto base=shooting::pingWindow(25,2.5,true);
 assert(shooting::calibratedWindow(base,2.5,0).minimum==77);
 assert(!shooting::calibratedWindow({},2.5,10).valid);
 assert(!shooting::calibratedWindow(base,NAN,10).valid);
 for(int shift:{-400,-40,40,400}){auto w=shooting::calibratedWindow(base,20,shift);assert(w.valid&&w.minimum>=0&&w.maximum<=99&&std::abs(w.maximum-w.minimum-4)<1e-9);}
 Fixture f;f.result(Grade::Early);f.result(Grade::Early);assert(f.shift()==0);f.result(Grade::Early);assert(f.shift()==4);
 for(int i=0;i<6;i++)f.result(Grade::Perfect);assert(f.shift()==4);
 for(int i=0;i<3;i++)f.result(Grade::Late);assert(f.shift()==2);
 for(int i=0;i<3;i++)f.result(Grade::Early);assert(f.shift()==3);
 for(int i=0;i<150;i++)f.result(Grade::Early);assert(f.shift()==40);
 for(int i=0;i<300;i++)f.result(Grade::Late);assert(f.shift()==-40);
 for(auto bad:{0,1,2}){
  Fixture x;x.result(Grade::Early);x.result(Grade::Early);
  x.result(bad==2?Grade::Good:Grade::Early,bad!=0,bad==1?1:0);
  x.result(Grade::Early);x.result(Grade::Early);assert(x.shift()==0);
 }
 // A cause transition does not discard otherwise consistent game evidence.
 Fixture causes;causes.result(Grade::Early,true,0,true);causes.result(Grade::Early,true,0,false);causes.result(Grade::Early,true,0,true);assert(causes.shift()==4);
 Fixture reset;for(int i=0;i<3;i++)reset.result(Grade::Early);auto old=reset.tune();
 reset.r.ping(reset.t+.1,90);assert(!reset.r.usable(old,reset.t+.1));assert(reset.r.view(reset.t+.1).windowShiftMs==0);
 Fixture context;for(int i=0;i<3;i++)context.result(Grade::Early);
 assert(context.r.prepare(context.t,2.5,37,2).windowShiftMs==0);
 // Physical pending release still works, while a fixed manual ceiling cannot
 // defeat a learned automatic window. Learner-off manual policy is unchanged.
 shooting::Options o;o.automatic=o.assist=true;o.minimum=67;o.maximum=81;o.autoMinimum=77;o.autoMaximum=90;o.calibrated=true;
 shooting::Sample sample{.82,2.5,0,.01,false,false,false,25,.006};
 assert(shooting::decide(o,sample)==shooting::Release::None);
 sample.pending=true;assert(shooting::decide(o,sample)==shooting::Release::Early);
 sample.pending=false;o.calibrated=false;assert(shooting::decide(o,sample)==shooting::Release::Maximum);
 // Closed-loop calibration over low/high ping, 60/240 FPS, different meter
 // rates, and opposite client timing errors. Grade oracle is synthetic.
 int scenarios=0;
 for(int fps:{60,240})for(int ping:{25,90,150})for(double rate:{1.5,2.5})for(double error:{-.012,.012}){
  timingReview::Reviewer r;double t=1;for(int i=0;i<5;i++){r.ping(t,ping);t+=.5;}
  const double target=release(fps,ping,rate,initial).time+error;
  const double tolerance=fps==60?.009:.003;
  int finalPerfect=0;
  for(unsigned id=1;id<=60;id++){
   r.ping(t,ping);auto q=r.prepare(t,rate,37,1);auto s=release(fps,ping,rate,q);
   const double errorNow=s.time-target;
   auto grade=std::abs(errorNow)<=tolerance?Grade::Perfect:errorNow<0?Grade::Early:Grade::Late;
   r.observe({id,t,s.fill,rate,ping,true,s.reason==shooting::Release::AutoMaximum,q,s.window.minimum,s.window.maximum});
   assert(r.gameResult({id,grade,t+.1,0,.94}));
   if(id>54&&grade==Grade::Perfect)++finalPerfect;t+=.5;
  }
  if(finalPerfect!=6)std::cerr<<"Failed scenario fps="<<fps<<" ping="<<ping<<" rate="<<rate<<" error="<<error<<" shift="<<r.view(t-.4).windowShiftMs<<" perfect="<<finalPerfect<<"\n";
  assert(finalPerfect==6);++scenarios;
 }
 std::cout<<"Automatic calibration regression, bounds, isolation, and "<<scenarios<<" closed-loop simulated scenarios passed\n";
}
