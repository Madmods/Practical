#include "defense_logic.hpp"
#include <iostream>
using namespace defense;
int main(){
 int passed=0,failed=0;auto check=[&](bool ok,const char* what){if(ok)++passed;else{++failed;std::cerr<<"FAIL "<<what<<'\n';}};
 Config c;c.values[Guard]=1;c.values[Reaction]=0;c.values[Gap]=800;
 Scene base;base.valid=base.grounded=true;base.at=10;base.own.id=1;base.own.p={0,3,8};
 Actor a;a.id=2;a.enemy=a.goalValid=true;a.p={0,3,0};a.goal={0,10,15};a.facing={0,0,-1};base.actors.push_back(a);
 Ball ball;ball.id=3;ball.owner=2;ball.p={0,3,-1};base.balls.push_back(ball);
 Intent intent;intent.focused=intent.activation=true;
 auto warm=[&](Planner& p,Scene& s,const Config& config){Decision d;for(int k=0;k<12;k++){s.at+=.02;d=p.step(config,s,intent,s.at);}return d;};
 Planner p;auto s=base;auto d=p.step(c,s,intent,s.at);
 check(d.observation!="Post-up stance (estimated)"&&distance(d.destination,a.p)>7.9f,"brief pivot does not trigger post spacing");
 d=warm(p,s,c);
 check(d.observation=="Post-up stance (estimated)","sustained back-to-basket possession detected");
 check(distance(d.destination,a.p)>4.2f&&distance(d.destination,a.p)<4.3f,"closes 8 stud stand-off to bounded close coverage");
 check(d.moving&&d.direction.z<0&&!d.jump&&!d.steal&&!d.sprint,"approaches post with ordinary steps only");
 s.own.p=d.destination;s.own.v={};s.at+=.02;d=p.step(c,s,intent,s.at);
 check(!d.moving&&d.response=="Holding the post buffer","holds position without idle shuffling");
 for(int k=0;k<20;k++){s.balls[0].p.x=k%2?1.8f:-1.8f;s.at+=.02;d=p.step(c,s,intent,s.at);check(!d.moving,"dribble movement cannot move the post defender");}
 s.own.p={0,3,3.1f};s.at+=.02;d=p.step(c,s,intent,s.at);
 check(d.moving&&d.direction.z>.9f&&!d.sprint&&d.retreat,"crowding overrides settled state and retreats directly");
 s.own.p={0,3,4};s.own.v={0,0,-3};s.actors[0].v={0,0,4};s.at+=.02;d=p.step(c,s,intent,s.at);
 check(d.direction.z>.9f&&d.response=="Giving the backdown space","predicted relative closing triggers buffer before contact");
 s.own.v={};s.actors[0].shotMeterActive=true;s.at+=.02;d=p.step(c,s,intent,s.at);
 check(distance(d.destination,s.actors[0].p)<2.01f&&d.reason=="Closing active shot meter","shot meter overrides post spacing immediately");
 s.actors[0].shotMeterActive=false;s.at+=.02;d=p.step(c,s,intent,s.at);
 check(!p.post.active,"post must reacquire after a shot cue");
 s=base;p={};d=warm(p,s,c);s.actors[0].facing={0,0,1};s.at+=.02;d=p.step(c,s,intent,s.at);
 check(!p.post.active&&distance(d.destination,a.p)>7.9f,"face-up restores normal stand-off immediately");
 s=base;p={};d=warm(p,s,c);s.actors[0].v={0,0,12};s.at+=.02;d=p.step(c,s,intent,s.at);check(!p.post.active,"fast drive exits post behavior");
 for(int scenario=0;scenario<10;scenario++){
  s=base;p={};d=warm(p,s,c);Config cfg=c;Intent in=intent;s.at+=.02;
  if(scenario==0)s.balls.clear();if(scenario==1)s.actors[0].enemy=false;
  if(scenario==2)in.activation=false;if(scenario==3)in.focused=false;
  if(scenario==4)in.chat=true;if(scenario==5)cfg.values[AntiPost]=0;
  if(scenario==6)s.actors[0].goalValid=false;if(scenario==7)s.at-=1;
  if(scenario==8){s.playingKnown=true;s.playing=false;}if(scenario==9)s.balls[0].owner=1;
  d=p.step(cfg,s,in,scenario==7?s.at+1:s.at);check(!p.post.active,"invalid/disabled context immediately clears post state");
 }
 for(int scenario=0;scenario<8;scenario++){
  s=base;p={};
  if(scenario==0)s.actors[0].facing={0,0,1};if(scenario==1)s.actors[0].goal.z=50;
  if(scenario==2)s.actors[0].v={0,0,-4};if(scenario==3)s.actors[0].v={8,0,0};
  if(scenario==4)s.own.p.z=-8;if(scenario==5)s.actors[0].facing={};
  if(scenario==6)s.actors[0].v.y=4;if(scenario==7)s.actors[0].facing.x=NAN;
  d=warm(p,s,c);check(!p.post.active,"ordinary retreat, face-up, fast motion and invalid facing do not qualify");
 }
 PostUpRead read;check(!read.update(true,&a,base.own,10),"classification starts unconfirmed");
 check(!read.update(true,&a,base.own,10.3),"missing samples restart confirmation");
 read.update(true,&a,base.own,10.4);check(read.update(true,&a,base.own,10.5),"continuous evidence confirms after restart");
 Actor other=a;other.id=4;check(!read.update(true,&other,base.own,10.52),"target switch cannot inherit post confirmation");
 read.update(true,&other,base.own,10.62);read.update(true,&other,base.own,10.72);Actor respawn=base.own;respawn.id=5;
 check(!read.update(true,&other,respawn,10.74),"respawn cannot inherit another defender's post confirmation");
 s=base;p={};warm(p,s,c);Config catchConfig=c;catchConfig.values[Intercept]=1;catchConfig.values[InterceptAuto]=1;catchConfig.values[Steal]=1;
 Ball pass;pass.id=4;pass.previousOwner=2;pass.p={0,5.5f,6};pass.v={0,0,1};pass.velocityValid=true;s.balls.push_back(pass);Intent catchIntent=intent;catchIntent.catchHeld=true;s.at+=.02;
 d=p.step(catchConfig,s,catchIntent,s.at);check(d.mode==Intercepting&&d.sprint,"explicit pass pursuit retains its normal sprint despite nearby post handler");
 Vec safe=postRoute({0,0,3.7f},unit(Vec{1,0,-1}));check(safe.z>=0&&finite(safe),"lateral route does not cut through the buffer");
 // Kinematic regression only: ordinary movement at60Hz against a slow backdown.
 // This is not a server physics/push-distance validation.
 s=base;s.own.p={0,3,4.25f};p={};warm(p,s,c);float smallest=100;int reversals=0,last=0;
 for(int k=0;k<180;k++){
  const float dt=1.f/60;s.at+=dt;s.actors[0].v={0,0,2};s.actors[0].p.z+=2*dt;s.balls[0].p=s.actors[0].p+Vec{0,0,-1};
  d=p.step(c,s,intent,s.at);s.own.v=d.moving?d.direction*12:Vec{};s.own.p=s.own.p+s.own.v*dt;
  smallest=std::min(smallest,distance(s.own.p,s.actors[0].p));int sign=d.moving?(d.direction.z>0?1:-1):0;
  if(sign&&last&&sign!=last)++reversals;if(sign)last=sign;
 }
 check(smallest>=PostBuffer,"slow backdown simulation maintains buffer");check(reversals<=1,"backdown tracking does not oscillate forward/backward");
 std::cout<<passed<<" anti-post checks passed; "<<failed<<" failed\n";return failed?1:0;
}
