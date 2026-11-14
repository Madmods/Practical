#include "defense_logic.hpp"
#include <iostream>
using namespace defense;
int main(){
 int passed=0,failed=0;auto check=[&](bool ok,const char* why){if(ok)++passed;else{++failed;std::cerr<<"FAIL "<<why<<'\n';}};
 Scene base;base.valid=base.grounded=true;base.at=10;base.own.id=1;base.own.p={0,3,8};
 Actor a;a.id=2;a.enemy=a.goalValid=true;a.p={0,3,0};a.goal={0,10,30};a.facing={0,0,1};base.actors.push_back(a);
 Ball b;b.id=3;b.owner=2;b.p={0,3,-1};base.balls.push_back(b);
 Config c;c.values[Guard]=1;c.values[Gap]=800;c.values[Reaction]=500;
 Intent i;i.focused=i.activation=true;
 auto seed=[&](Planner& p,Scene& s){for(int n=0;n<4;n++){s.at=10+n*.02;p.step(c,s,i,s.at);}};
 auto cut=[](Scene& s){s.at+=.02;s.actors[0].p.z-=.16f;s.actors[0].v={0,0,-8};s.balls[0].p=s.actors[0].p+Vec{0,0,-1};};
 Planner p;auto s=base;seed(p,s);cut(s);auto d=p.step(c,s,i,s.at);
 check(d.moving&&d.direction.z<-.9f&&d.reason=="Pressuring sudden backward cut","sharp backward cut pushes immediately despite acquisition delay");
 check(!d.jump&&!d.steal&&d.observation=="Sudden backward cut","movement alone is not labeled a shot and never jumps");
 check(distance(d.destination,s.actors[0].p)<3.25f&&distance(d.destination,s.actors[0].p)>2,"unconfirmed cut uses bounded anticipatory pressure");
 for(int n=0;n<40;n++){cut(s);d=p.step(c,s,i,s.at);}
 check(d.reason!="Pressuring sudden backward cut","steady retreat cannot keep renewing an unconfirmed burst");
 // Live trace exposed a one-frame speed dip cancelling pressure and an180ms
 // burst expiring before an8-stud stand-off could be closed.
 s=base;p={};seed(p,s);cut(s);p.step(c,s,i,s.at);
 s.at+=.03;s.actors[0].v={0,0,-.2f};s.actors[0].p.z-=.006f;
 d=p.step(c,s,i,s.at);check(d.reason=="Pressuring sudden backward cut","single slow pivot sample preserves pressure");
 for(int n=0;n<10;n++){cut(s);d=p.step(c,s,i,s.at);}
 check(d.reason=="Pressuring sudden backward cut","pressure survives beyond180ms while closing stand-off");
 for(int n=0;n<6;n++){s.at+=.02;s.actors[0].v={};d=p.step(c,s,i,s.at);}
 check(p.fade.until<0&&p.fade.cutArmed,"sustained stop cancels and rearms pressure");
 s=base;p={};p.step(c,s,i,s.at);cut(s);d=p.step(c,s,i,s.at);
 check(d.reason=="Pressuring sudden backward cut","first corroborated20ms cut sample does not wait40ms");
 s.at+=.02;s.actors[0].v={0,0,8};s.actors[0].p.z+=.16f;d=p.step(c,s,i,s.at);
 check(p.fade.until<0,"true forward reversal cancels on the same sample");
 s=base;p={};seed(p,s);cut(s);s.actors[0].shooting=true;s.actors[0].facing={0,0,-1};d=p.step(c,s,i,s.at);
 check(d.reason=="Closing spin fadeaway"&&d.observation=="Spin and retreat with shot cue","spin plus backward body motion plus shot cue recognized");
 const Vec expected=s.actors[0].p+Vec{0,0,-.8f}+Vec{0,0,2};check(distance(d.destination,expected)<.01f,"fade closeout leads the retreat instead of chasing the old position");
 for(int n=0;n<20;n++){cut(s);d=p.step(c,s,i,s.at);}
 check(d.reason=="Closing fadeaway retreat"&&distance(d.destination,s.actors[0].p)<2,"shot cue sustains closeout through continued fade");
 check(!d.jump,"fade detection does not bypass auto-block rules");
 s.actors[0].v={};s.at+=.02;d=p.step(c,s,i,s.at);check(d.reason=="Closing shot gather / shot release"&&std::abs(distance(d.destination,s.actors[0].p)-2)<.01f,"stopped shooter loses prediction but retains normal shot closeout");
 // Reject rotation, ball motion, noisy velocity and tiny root movement by themselves.
 for(int kind=0;kind<5;kind++){
  s=base;p={};seed(p,s);s.at+=.02;
  if(kind==0)s.actors[0].facing={0,0,-1};
  if(kind==1)s.balls[0].p.z-=5;
  if(kind==2)s.actors[0].v={0,0,-10};
  if(kind==3){s.actors[0].v={0,0,-8};s.actors[0].p.z-=.01f;}
  if(kind==4){s.actors[0].v={8,0,0};s.actors[0].p.x+=.16f;}
  d=p.step(c,s,i,s.at);check(d.reason!="Pressuring sudden backward cut","unrelated or uncorroborated motion does not trigger pressure");
 }
 s=base;p={};s.actors[0].v={0,0,-6};
 for(int n=0;n<20;n++){s.at=10+n*.02;s.actors[0].p.z=-.12f*n;d=p.step(c,s,i,s.at);check(d.reason!="Pressuring sudden backward cut","already-retreating target needs a new change or a shot cue");}
 s.actors[0].shotMeterActive=true;s.at+=.02;s.actors[0].p.z-=.12f;d=p.step(c,s,i,s.at);check(d.reason=="Closing fadeaway retreat","meter cue upgrades an existing retreat");
 s=base;p={};seed(p,s);s.actors[0].p.z=-20;s.actors[0].v={0,0,-8};s.at+=.02;d=p.step(c,s,i,s.at);check(d.reason!="Pressuring sudden backward cut","teleport cannot masquerade as a fade");
 for(int kind=0;kind<8;kind++){
  s=base;p={};seed(p,s);cut(s);p.step(c,s,i,s.at);Intent in=i;Config cfg=c;s.at+=.02;
  if(kind==0)s.balls.clear();if(kind==1)in.activation=false;if(kind==2)in.focused=false;if(kind==3)in.chat=true;
  if(kind==4)s.actors[0].enemy=false;if(kind==5)cfg.values[Guard]=0;if(kind==6)s.own.id=4;if(kind==7)s.actors[0].goalValid=false;
  d=p.step(cfg,s,in,s.at);check(d.reason!="Pressuring sudden backward cut"&&p.fade.until<0,"invalid context or respawn clears fade pressure");
 }
 s=base;p={};seed(p,s);s.at+=1;cut(s);d=p.step(c,s,i,s.at);check(p.fade.until<0,"missing samples reset reversal history");
 s=base;p={};seed(p,s);s.actors[0].id=4;s.balls[0].owner=4;cut(s);d=p.step(c,s,i,s.at);check(p.fade.until<0,"new handler cannot inherit a reversal");
 // Anti-post must yield immediately when the handler spins out into a fade.
 s=base;s.actors[0].goal.z=15;s.actors[0].facing={0,0,-1};p={};Config post=c;post.values[Reaction]=0;
 for(int n=0;n<15;n++){s.at=10+n*.02;p.step(post,s,i,s.at);}check(p.post.active,"precondition: anti-post active");
 cut(s);s.actors[0].facing={0,0,1};s.actors[0].shotMeterActive=true;d=p.step(post,s,i,s.at);
 check(!p.post.active&&d.reason=="Closing spin fadeaway"&&d.direction.z<0,"spin fade overrides post buffer on the same sample");
 std::cout<<passed<<" fadeaway checks passed; "<<failed<<" failed\n";return failed?1:0;
}
