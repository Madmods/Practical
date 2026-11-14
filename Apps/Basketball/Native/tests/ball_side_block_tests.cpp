#include "defense_logic.hpp"
#include <iostream>
using namespace defense;
int main(){
 int passed=0,failed=0;auto check=[&](bool ok,const char* message){if(ok)++passed;else{++failed;std::cerr<<"FAIL "<<message<<'\n';}};
 Config c;c.values[Guard]=1;c.values[Reaction]=0;c.values[Gap]=800;c.values[Idle]=1;
 Intent i;i.focused=i.activation=true;
 Scene base;base.valid=base.grounded=true;base.at=10;base.own.id=1;base.own.p={0,3,8};base.own.facing={0,0,-1};
 Actor a;a.id=2;a.enemy=a.goalValid=true;a.p={0,3,0};a.goal={0,10,30};base.actors.push_back(a);
 Ball b;b.id=3;b.owner=2;b.p={1.5f,3,0};base.balls.push_back(b);
 auto s=base;Planner p;Decision d;
 for(int n=0;n<40;n++){s.at=10+n*.02;d=p.step(c,s,i,s.at);}
 check(d.destination.x>.60f&&d.destination.x<=.651f,"small settled offset follows the right-hand ball");
 check(std::abs(d.destination.z-8)<.001f,"ball side preserves basket-side depth");
 check(d.moving&&d.direction.x>0,"bias can initiate a small correction from centered stance");
 for(int n=0;n<50;n++){s.at+=.02;s.balls[0].p.x=n%2?1.5f:-1.5f;d=p.step(c,s,i,s.at);}
 check(d.destination.x>.60f,"alternating dribble frames do not alternate guard side");
 for(int n=0;n<40;n++){s.at+=.02;s.balls[0].p.x=-1.5f;d=p.step(c,s,i,s.at);}
 check(d.destination.x<-.60f&&d.destination.x>=-.651f,"sustained hand change moves smoothly to the other side");
 float previous=p.ballSide.offset;s.at+=.02;s.balls[0].p.x=1.5f;p.step(c,s,i,s.at);
 check(std::abs(p.ballSide.offset-previous)<=.051f,"side transition rate is bounded");
 for(int n=0;n<40;n++){s.at+=.02;s.balls[0].p.x=8;d=p.step(c,s,i,s.at);}
 check(std::abs(d.destination.x)<.001f,"detached ball cannot drag guard away from basket line");
 for(int kind=0;kind<5;kind++){
  s=base;p={};for(int n=0;n<40;n++){s.at+=.02;p.step(c,s,i,s.at);}Intent in=i;
  if(kind==0)in.activation=false;if(kind==1)in.focused=false;if(kind==2)s.balls.clear();if(kind==3)s.actors[0].id=4;if(kind==4)s.own.id=9,s.valid=false;
  s.at+=.02;p.step(c,s,in,s.at);check(p.ballSide.offset==0,"loss of guarding context clears old side bias");
 }
 s=base;p={};s.actors[0].shotMeterActive=true;
 for(int n=0;n<40;n++){s.at+=.02;d=p.step(c,s,i,s.at);}
 check(d.destination.x>0&&d.destination.x<=.501f&&d.destination.z>0,"closeout bias stays within25 percent of basket-side depth");
 // First meter rise changes positioning, not jump eligibility.
 s=base;p={};c.values[Reaction]=500;c.values[Block]=1;s.actors[0].shotMeterRising=true;
 d=p.step(c,s,i,s.at);check(d.moving&&d.destination.z<=2.01f&&!d.jump,"first meter rise closes space before confirmed charge without jumping");
 s.actors[0].shotMeterRising=false;s.at+=.10;d=p.step(c,s,i,s.at);check(!d.jump,"isolated meter increase cannot become a block");
 // Reproduce a wide but still attached gather missed by the old2.5-stud gate.
 s=base;s.own.p={0,3,4};s.balls[0].p={2.85f,4.9f,0};s.balls[0].heldRiseSpeed=45;
 auto& shooter=s.actors[0];shooter.shotMeterActive=shooter.shotMeterAdvancing=true;shooter.shotMeterAge=.12;shooter.shotMeterFill=.35f;
 p={};d=p.step(c,s,i,s.at);check(d.jump,"early reachable wide gather blocks before old40-percent threshold");
 s.at+=.02;d=p.step(c,s,i,s.at);check(!d.jump,"windup attempt is not repeated");
 // No missed-window catch-up, even with stale ownership and no release flag.
 for(float fill:{.75f,.80f,.90f,.97f}){s.actors[0].shotMeterFill=fill;p={};d=p.step(c,s,i,s.at);check(!d.jump&&d.blockReason=="Windup window passed","late meter plus retained ownership cannot trigger a block");}
 // Observed human gathers can first reach safe jump height around68 percent.
 // A universal65-percent cutoff would suppress a valid pre-release lift.
 s.actors[0].shotMeterFill=.67965f;s.balls[0].p={0,5.29111f,0};s.balls[0].heldRiseSpeed=49;p={};
 check(p.step(c,s,i,s.at).jump,"reachable human gather remains eligible when it lifts later than AI");
 s.actors[0].shotMeterFill=.35f;s.actors[0].shotReleased=true;p={};check(!p.step(c,s,i,s.at).jump,"explicit release always wins over meter and possession");
 s.actors[0].shotReleased=false;s.balls[0].p.y=4.5f;p={};check(!p.step(c,s,i,s.at).jump,"early charge does not remove low-ball dive protection");
 s.balls[0].p={0,5,0};s.balls[0].heldRiseSpeed=5;s.own.p.z=8;p={};d=p.step(c,s,i,s.at);check(!d.jump&&d.blockReason=="Outside block reach","early timing never expands the configured reach");
 // Rim assistance used to bypass the release and late-meter restrictions.
 s=base;s.own.p={0,3,3};s.actors[0].goal={0,10,5};s.actors[0].v.y=3;s.balls[0].p={0,6.2f,0};s.balls[0].heldRiseSpeed=5;
 s.actors[0].shotMeterActive=s.actors[0].shotMeterAdvancing=true;s.actors[0].shotMeterAge=.2;c.values[Reaction]=0;c.values[RimBlock]=1;
 for(int released=0;released<2;released++){s.actors[0].shotMeterFill=released?.4f:.9f;s.actors[0].shotReleased=released;p={};check(!p.step(c,s,i,s.at).jump,"rim helper cannot jump late or after release");}
 s.actors[0].shotReleased=false;s.actors[0].shotMeterFill=.4f;p={};check(p.step(c,s,i,s.at).jump,"reachable early rim gather still qualifies");
 std::cout<<passed<<" ball-side / early-block checks passed; "<<failed<<" failed\n";return failed?1:0;
}
