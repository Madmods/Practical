#include "timing_reviewer.hpp"
#include "shooting_logic.hpp"
#include <cassert>
#include <iostream>
using namespace timingReview;
struct Fixture {
 Reviewer r;double t=10;unsigned id=0;
 Fixture(){for(int i=0;i<5;i++)tick();}
 void tick(int ping=30){t+=.5;r.ping(t,ping);}
 Tuning shot(double fill=.78,bool ceiling=false,bool automatic=true,double rate=2.5,int base=32,std::uintptr_t session=1){
  tick();auto tuning=r.prepare(t,rate,base,session);r.observe({++id,t,fill,rate,30,automatic,ceiling,tuning});return tuning;
 }
 bool grade(Grade grade){return r.feedback(id,grade,t+.05);}
};
int main(){
 Fixture f;assert(f.r.view(f.t).stable);f.shot();assert(f.r.view(f.t).pendingId==f.id);
 assert(f.r.view(f.t).perfect==0); // a release at 78% is not a confirmed result
 assert(!f.r.feedback(f.id+1,Grade::Late,f.t));
 assert(!f.r.feedback(f.id,static_cast<Grade>(99),f.t));
 assert(f.grade(Grade::Late));assert(!f.grade(Grade::Late));
 assert(f.r.view(f.t).feedbackCount==1&&f.r.view(f.t).adjustmentMs==0);
 for(int i=0;i<2;i++){f.shot();assert(f.grade(Grade::Late));}
 assert(f.r.view(f.t).adjustmentMs==2);
 const auto applied=f.shot();assert(applied.adjustmentMs==2);
 assert(f.grade(Grade::Perfect));assert(f.r.view(f.t).perfect==1);
 assert(applied.adjustmentMs==2); // feedback never mutates an in-flight tuning snapshot
 for(int i=0;i<3;i++){f.shot();assert(f.grade(Grade::Early));}
 assert(f.r.view(f.t).adjustmentMs==0);
 for(int i=0;i<30;i++){f.shot();assert(f.grade(Grade::Early));}
 assert(f.r.view(f.t).adjustmentMs==-12);
 for(int i=0;i<60;i++){f.shot();assert(f.grade(Grade::Late));}
 assert(f.r.view(f.t).adjustmentMs==12&&f.r.historySize()==60);
 f.shot();auto oldId=f.id;auto oldTuning=f.r.prepare(f.t,2.5,32,1);
 f.tick(100);assert(!f.r.view(f.t).stable&&!f.r.usable(oldTuning,f.t));
 assert(f.r.view(f.t).adjustmentMs==0&&!f.r.feedback(oldId,Grade::Late,f.t));
 for(int i=0;i<5;i++)f.tick(100);assert(f.r.view(f.t).stable);
 assert(!f.r.view(f.t+2.01).stable);assert(!f.r.view(f.t-1).stable);

 Fixture cap;
 for(int i=0;i<2;i++)cap.shot(.84,true);
 assert(cap.r.view(cap.t).guardMs==0);
 cap.shot(.84,true);assert(cap.r.view(cap.t).guardMs==2);
 assert(cap.r.view(cap.t).perfect==0&&cap.r.view(cap.t).feedbackCount==0);
 for(int i=0;i<40;i++)cap.shot(.84,true);
 assert(cap.r.view(cap.t).guardMs==20);
 Fixture cappedFeedback;
 for(int i=0;i<3;i++){cappedFeedback.shot(.79,true);assert(cappedFeedback.grade(Grade::Early));}
 assert(cappedFeedback.r.view(cappedFeedback.t).guardMs==-2&&cappedFeedback.r.view(cappedFeedback.t).adjustmentMs==0);
 for(int i=0;i<3;i++){cappedFeedback.shot(.79,true);assert(cappedFeedback.grade(Grade::Late));}
 assert(cappedFeedback.r.view(cappedFeedback.t).guardMs==0&&cappedFeedback.r.view(cappedFeedback.t).adjustmentMs==0);
 for(int i=0;i<3;i++){cappedFeedback.shot(.79,true);assert(cappedFeedback.grade(Grade::Early));}
 assert(cappedFeedback.r.view(cappedFeedback.t).guardMs==-2); // later send is bounded by measured polling uncertainty
 for(int i=0;i<3;i++){cappedFeedback.shot(.78,false);assert(cappedFeedback.grade(i%2?Grade::Late:Grade::Early));}
 assert(cappedFeedback.r.view(cappedFeedback.t).adjustmentMs==0); // contradictory evidence does not train
 Fixture mixedCause;
 for(int i=0;i<2;i++){mixedCause.shot(.78,false);assert(mixedCause.grade(Grade::Late));}
 mixedCause.shot(.79,true);assert(mixedCause.grade(Grade::Late));
 assert(mixedCause.r.view(mixedCause.t).guardMs==0&&mixedCause.r.view(mixedCause.t).adjustmentMs==0);
 Fixture shifted;
 for(int i=0;i<3;i++){
  shifted.tick();auto tuning=shifted.r.prepare(shifted.t,2.5,32,1);
  shifted.r.observe({++shifted.id,shifted.t,.70,2.5,30,true,true,tuning,64.5,68.5});
 }
 assert(shifted.r.view(shifted.t).overshoots==3&&shifted.r.view(shifted.t).guardMs==2);
 Fixture slowerPing;
 for(int i=0;i<3;i++){
  slowerPing.tick();auto tuning=slowerPing.r.prepare(slowerPing.t,2.5,32,1);
  slowerPing.r.observe({++slowerPing.id,slowerPing.t,.82,2.5,30,true,true,tuning,79.5,83.5});
 }
 assert(slowerPing.r.view(slowerPing.t).overshoots==0&&slowerPing.r.view(slowerPing.t).guardMs==0);

 Fixture manual;
 for(int i=0;i<6;i++)manual.shot(.85,true,false);
 assert(manual.r.view(manual.t).guardMs==0&&!manual.grade(Grade::Late));
 Fixture stale;stale.shot();for(int i=0;i<42;i++)stale.tick();
 assert(stale.r.view(stale.t).pendingId==0&&!stale.grade(Grade::Late));
 Fixture jitter;for(int i=0;i<12;i++)jitter.tick(i%2?40:20);
 assert(!jitter.r.view(jitter.t).stable);jitter.shot(.84,true);assert(!jitter.grade(Grade::Late));
 assert(jitter.r.view(jitter.t).guardMs==0);
 Fixture noisyShot;auto noisyTuning=noisyShot.r.prepare(noisyShot.t,2,32,1);
 noisyShot.r.observe({1,noisyShot.t,.85,3.5,30,true,true,noisyTuning});
 assert(noisyShot.r.view(noisyShot.t).pendingId==0); // changing speed within a shot is not training evidence
 Fixture changed;
 for(int i=0;i<3;i++){changed.shot();assert(changed.grade(Grade::Late));}
 auto pending=changed.id;changed.r.prepare(changed.t,2.5,33,1);
 assert(changed.r.view(changed.t).adjustmentMs==0&&!changed.r.feedback(pending,Grade::Late,changed.t));
 changed.shot();pending=changed.id;changed.r.prepare(changed.t,4,32,1);
 assert(!changed.r.feedback(pending,Grade::Late,changed.t));
 changed.shot();pending=changed.id;changed.r.prepare(changed.t,2.5,32,2);
 assert(!changed.r.feedback(pending,Grade::Late,changed.t));
 changed.r.reset();assert(changed.r.historySize()==0&&changed.r.view(changed.t).pendingId==0);
 Reviewer bad;bad.ping(NAN,30);bad.ping(1,0);bad.ping(1,1001);assert(!bad.view(1).stable);
 assert(!bad.prepare(1,NAN,32,1).eligible&&!bad.prepare(1,2.5,32,0).eligible);
 bad.observe({1,1,NAN,2.5,30,true,true,{}});assert(bad.view(1).shots==0);
 Fixture duplicate;duplicate.shot();auto count=duplicate.r.view(duplicate.t).shots;
 duplicate.r.observe({duplicate.id,duplicate.t,.84,2.5,30,true,true,{}});assert(duplicate.r.view(duplicate.t).shots==count);

 // Cross-check learned extremes against the real release decision: the 81%
 // ceiling remains active even with the latest allowable learned adjustment.
 int combinations=0;
 for(int correction:{-12,0,12})for(int guard:{-12,0,2,20})for(int ping:{20,80,200}){
  shooting::Options o;o.automatic=true;o.autoMaximum=shooting::BasketballAutoMaximum;o.autoMinimum=shooting::BasketballAutoMinimum;
  const double lead=.0192117+(ping+32+correction)*.001;
  auto d=shooting::decide(o,{.81,2.5,0,lead,false,false,false,ping,.016+guard*.001});
  assert(d==shooting::Release::AutoMaximum);++combinations;
  for(double fill:{.50,.70,.75,.76,.769999})assert(shooting::decide(o,{fill,2.5,.1,lead,false,false,false,ping,.016+guard*.001})==shooting::Release::None);
 }
 std::cout<<"Timing reviewer lifecycle, evidence, jitter, bounds and "<<combinations<<" ceiling combinations passed\n";
}
