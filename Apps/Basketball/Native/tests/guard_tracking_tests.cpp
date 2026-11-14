#include "defense_logic.hpp"
#include <cassert>
#include <iostream>
using namespace defense;
Scene fixture(){Scene s;s.valid=s.grounded=true;s.at=10;s.own={1,"self",{8,3,0},{},{0,0,-1},{0,10,20},16,false,true};s.actors.push_back({2,"handler",{0,3,-5},{},{0,0,1},{0,10,20},16,true,true});Ball b;b.id=100;b.owner=b.previousOwner=2;b.p={0,4,-4};s.balls.push_back(b);return s;}
int main(){
 // Advancing meter detection rejects static UI, jitter, resets, stale samples,
 // replacement objects, invalid values and a single unconfirmed fill jump.
 ShotMeter meter;
 for(int n=0;n<20;n++)assert(!meter.update(100,.5f,1+n*.02));
 assert(!meter.update(100,.53f,1.4));assert(meter.update(100,.56f,1.42));
 assert(meter.update(100,.56f,1.44));
 assert(meter.update(100,.56f,1.52));assert(!meter.update(100,.56f,1.62));
 assert(!meter.update(100,0,1.64));assert(!meter.update(100,.03f,1.66));assert(meter.update(100,.06f,1.68));
 assert(!meter.update(101,.09f,1.70));assert(!meter.update(101,.12f,1.72));assert(meter.update(101,.15f,1.74));
 assert(!meter.update(101,.4f,2));assert(!meter.update(101,NAN,2.02));
 assert(!meter.update(101,INFINITY,2.04));assert(!meter.update(101,-.01f,2.06));assert(!meter.update(101,1.1f,2.08));
 assert(!meter.update(0,.1f,2.1));assert(!meter.update(101,.1f,NAN));
 meter.reset();for(int n=0;n<100;n++)assert(!meter.update(100,n%2?.402f:.4f,3+n*.02));
 assert(!meter.update(100,.5f,5.02));assert(!meter.update(100,.5f,5.04));
 assert(!meter.update(100,.6f,5.02)); // backward timestamp resets
 meter.reset();assert(!meter.update(100,0,6));assert(!meter.update(100,.03f,6.02));
 assert(!meter.update(100,.03f,6.04));assert(meter.update(100,.06f,6.06)); // slower UI update cadence
 meter.reset();assert(!meter.update(100,0,7));assert(!meter.update(100,.03f,7.02));
 for(int n=1;n<=10;n++)assert(!meter.update(100,.03f,7.02+n*.02));
 assert(!meter.update(100,.06f,7.24)); // two isolated jumps are not advancing fill
 std::cout<<"Shot meter advance, plateau expiry, reset, jitter and invalid-data checks passed\n";
 assert(arrivalBrake({.8f,0,0},{16,0,0}));
 assert(!arrivalBrake({.8f,0,0},{-16,0,0})); // moving away: never pause pursuit
 assert(!arrivalBrake({3,0,0},{16,0,0})); // distant recovery is uninterrupted
 assert(!arrivalBrake({.8f,0,1},{16,0,0})); // would pass alongside destination
 assert(!arrivalBrake({.8f,0,0},{})); // stationary actor must take a step
 assert(!arrivalBrake({.8f,0,0},{NAN,0,0}));
 assert(practiceOpponent("AI1",false,true,true,true));assert(practiceOpponent("AI12",false,true,true,true));
 assert(!practiceOpponent("AI1",true,true,true,true));assert(!practiceOpponent("AI1",false,true,true,false));
 assert(!practiceOpponent("AI1",false,false,true,true));assert(!practiceOpponent("AI1",false,true,false,true));
 assert(!practiceOpponent("AI",false,true,true,true));assert(!practiceOpponent("AI_Player",false,true,true,true));
 assert(opposingTeams(1,2));assert(!opposingTeams(1,1));assert(!opposingTeams(0,2));assert(!opposingTeams(1,0));assert(!opposingTeams(0,0));assert(!opposingTeams(1,2,true));
 assert(opposingMatchup(0,0,100,200));assert(!opposingMatchup(0,0,100,100));
 assert(opposingMatchup(1,1,100,200));assert(!opposingMatchup(1,2,100,100));
 assert(!opposingMatchup(0,0,100,0));assert(!opposingMatchup(0,0,0,200));
 assert(opposingMatchup(1,2,0,0));assert(!opposingMatchup(1,2,100,200,true));
 Config c;c.values[Guard]=1;c.values[Reaction]=0;c.values[Humanize]=0;c.values[Idle]=0;
 Intent i;i.focused=i.activation=true;auto s=fixture();Planner p;
 s.actors[0].v={16,0,0};auto right=p.step(c,s,i,10);assert(std::abs(right.destination.x)<.001f);assert(right.destination.z>s.actors[0].p.z);
 s.actors[0].p.x=-1.5f;s.actors[0].v={-16,0,0};auto left=p.step(c,s,i,10.02);assert(left.destination.x< -1);
 // Sampled tracking and bounded turns converge after a reversal.
 c.values[Humanize]=1;
 s=fixture();s.own.p={0,3,0};s.actors[0].p={5,3,-5};p={};auto d=p.step(c,s,i,10);assert(d.direction.x>0);
 s.actors[0].p.x=-5;d=p.step(c,s,i,10.02);assert(d.direction.x<0);
 s=fixture();s.actors[0].v={10000,0,0};p={};d=p.step(c,s,i,10);assert(d.destination.x<=3.5f);assert(finite(d.destination));
 // Team change is honored on the next planner call, even with a selected lock.
 s=fixture();p={};i.selected=2;c.values[Lock]=1;c.values[Steal]=1;d=p.step(c,s,i,10);assert(d.target==2);
 s.actors[0].enemy=false;d=p.step(c,s,i,10.02);assert(d.target==0&&!d.moving&&!d.steal&&!d.jump);
 for(auto feature:{Block,Oop,Rebound,Intercept})for(bool known:{false,true}){
  Config test;test.values[feature]=1;Scene f=fixture();f.own.p={0,3,0};f.actors[0].enemy=false;
  auto&b=f.balls[0];b.owner=0;b.previousOwner=known?2:999;b.p={0,8,-3};b.v={0,feature==Block?10.f:-2.f,2};b.velocityValid=true;b.releasedAt=9.9;b.shot=feature==Block;b.lob=feature==Oop;b.boardHit=true;b.boardAt=10;
  Intent in;in.focused=in.activation=in.catchHeld=true;Planner plan;auto result=plan.step(test,f,in,10);
  assert(!result.jump&&!result.steal&&!result.moving);
 }
 // Arrival hysteresis prevents alternating taps at the same stand-off point.
 c.values[Adaptive]=0;i.selected=0;s=fixture();s.own.p={.4f,3,0};p={};
 d=p.step(c,s,i,10);assert(!d.moving&&!d.sprint);
 s.own.p.x=.7f;d=p.step(c,s,i,10.02);assert(!d.moving);
 s.own.p.x=1.6f;d=p.step(c,s,i,10.04);assert(d.moving&&!d.sprint);
 s.own.p.x=6;d=p.step(c,s,i,10.06);assert(d.moving&&d.sprint);
 // Large pursuit gaps still ease non-reversing turns with softness enabled.
 s=fixture();s.own.p={-6,3,0};p={};d=p.step(c,s,i,10);assert(d.direction.x>.99f);
 s.own.p={-6,3,-6};d=p.step(c,s,i,10.02);auto direct=unit(flat(d.destination-s.own.p));
 assert(d.direction.z>0&&d.direction.z<direct.z);assert(std::abs(length(d.direction)-1)<.001f);
 // Key thresholds have a retention band, but release on zero or reversal.
 assert(!movementAxis(.30f,false));assert(movementAxis(.30f,true));assert(movementAxis(.40f,false));
 assert(!movementAxis(.21f,true));assert(!movementAxis(0,true));assert(!movementAxis(-1,true));
 i.focused=false;d=p.step(c,s,i,10.04);assert(!d.moving&&!d.sprint);assert(!p.settled);
 // Deterministic ordinary-speed pursuit with two cuts; no live input is sent.
 s=fixture();s.own.p={-8,3,0};p={};i.focused=true;float totalError=0,maxError=0;int samples=0;
 for(int frame=0;frame<400;frame++){
  double t=20+frame*.02;s.at=t;s.actors[0].v={frame<120?8.f:frame<260?-8.f:8.f,0,0};
  s.actors[0].p=s.actors[0].p+s.actors[0].v*.02f;d=p.step(c,s,i,t);
  assert(finite(d.destination)&&finite(d.direction));
  if(d.moving)s.own.p=s.own.p+d.direction*(s.own.speed*.02f);
  if(frame>60){float error=distance(s.own.p,d.destination);totalError+=error;maxError=std::max(maxError,error);++samples;}
 }
 assert(totalError/samples<2.f);assert(maxError<4.f);
 std::cout<<"Synthetic pursuit mean destination error="<<totalError/samples<<", peak="<<maxError<<" studs (ideal movement model)\n";
 // Script positioning must not move to our side when our initial position changes.
 c.values[Adaptive]=0;c.values[Cutoff]=0;c.values[Idle]=0;c.values[RimPress]=0;
 s=fixture();s.actors[0].v={0,0,10};s.actors[0].goal={0,10,100};s.at=30;
 s.own.p={-8,3,0};p={};auto fromLeft=p.step(c,s,i,30);
 s.own.p={8,3,0};p={};auto fromRight=p.step(c,s,i,30);
 assert(std::abs(fromLeft.destination.x)<.001f&&std::abs(fromRight.destination.x)<.001f);
 assert(distance(fromLeft.destination,fromRight.destination)<.001f);
 assert(std::abs(fromRight.destination.z-.8f)<.001f); // -5 + 10*.08 + 5
 assert(fromLeft.direction.x>0&&fromRight.direction.x<0);
 // Mirror symmetry of lateral prediction/centering, with no forced retreat angle.
 s=fixture();s.actors[0].goal={0,10,100};s.actors[0].v={8,0,4};s.own.p={-6,3,0};s.at=31;p={};auto one=p.step(c,s,i,31);
 s.actors[0].v.x=-8;s.own.p.x=6;p={};auto mirror=p.step(c,s,i,31);
 assert(std::abs(one.destination.x+mirror.destination.x)<.001f);
 assert(std::abs(one.direction.x+mirror.direction.x)<.001f);
 assert(std::abs(one.direction.z-mirror.direction.z)<.001f);
 // Direct script human-steering bound: level .6 -> 6.8 rad/s ordinarily.
 s=fixture();s.at=32;s.own.p={-6,3,0};p={};d=p.step(c,s,i,32);Vec old=d.direction;
 s.own.p={-6,3,-6};s.at=32.02;d=p.step(c,s,i,s.at);
 float angle=acosf(std::clamp(dot(old,d.direction),-1.f,1.f));assert(angle>0&&angle<.7854f);
 std::cout<<"Script geometry, centering, symmetry, turn bound and existing guard regressions passed\n";
 // Basket-side coverage must not depend on handler facing, camera, or which
 // side the defender starts on. Lateral lead and idle settings cannot skew it.
 Config basket;c.values[Lock]=0;basket.values[Guard]=1;basket.values[Reaction]=0;
 basket.values[Gap]=800;basket.values[Idle]=1;basket.values[Cutoff]=1;
 int basketCases=0;
 for(int angle=0;angle<8;angle++)for(int mode=0;mode<3;mode++)for(float start:{-12.f,12.f}){
  s=fixture();float theta=angle*3.14159265f/4;Vec axis{cosf(theta),0,sinf(theta)},across{-axis.z,0,axis.x};
  auto& handler=s.actors[0];handler.p={3,3,7};handler.goal=handler.p+axis*30;
  handler.facing=across;handler.v=mode==0?Vec{}:across*12;
  handler.shotMeterActive=mode==2;s.own.p=handler.p+axis*8+across*start;
  s.cameraForward=across;s.cameraRight=axis;s.at=40;p={};i.selected=0;
  for(int frame=0;frame<5;frame++){
   s.at+=.02;handler.p=handler.p+handler.v*.02f;d=p.step(basket,s,i,s.at);
   Vec basketLine=unit(flat(handler.goal-handler.p)),offset=flat(d.destination-handler.p);
   float towardBasket=dot(offset,basketLine);
   assert(towardBasket>0&&towardBasket<distance(handler.p,handler.goal));
   assert(length(offset-basketLine*towardBasket)<.001f);
   assert(dot(unit(flat(d.face-d.destination)),basketLine)<-.999f);
   assert(d.stance&&distance(d.face,handler.p)<.001f);
  }
  ++basketCases;
 }
 // Near the rim the target cannot continue through/past the hoop.
 s=fixture();s.actors[0].goal=s.actors[0].p+Vec{0,0,1};s.at=41;p={};d=p.step(basket,s,i,s.at);
 assert(d.destination.z>=s.actors[0].p.z&&d.destination.z<s.actors[0].goal.z);
 std::cout<<basketCases<<" basket-line scenarios (five frames each) and near-rim bound passed\n";
 s=fixture();s.at=42;s.actors[0].v={0,0,10};p={};basket.values[Cutoff]=0;auto noCut=p.step(basket,s,i,s.at);
 p={};basket.values[Cutoff]=1;auto cutDrive=p.step(basket,s,i,s.at);
 assert(cutDrive.destination.z>noCut.destination.z+.4f&&std::abs(cutDrive.destination.x)<.001f);
 // Initial reaction is paid once for the acquired matchup, not each sample.
 Config delayed;delayed.values[Guard]=1;delayed.values[Reaction]=160;delayed.values[Idle]=0;
 s=fixture();s.at=50;p={};i.focused=i.activation=true;i.selected=0;
 d=p.step(delayed,s,i,50);assert(!d.moving);
 s.at=50.159;d=p.step(delayed,s,i,s.at);assert(!d.moving);
 s.at=50.161;d=p.step(delayed,s,i,s.at);assert(d.moving);
 s.at=50.18;d=p.step(delayed,s,i,s.at);assert(d.moving);
 i.activation=false;s.at=50.19;d=p.step(delayed,s,i,s.at);assert(!d.moving);
 std::cout<<"Optional start delay and immediate release checks passed\n";
 // Default start is immediate even with smoothing enabled, and current position
 // changes reach the destination on the very next frame (no sampling interval).
 Config responsive;responsive.values[Guard]=1;responsive.values[Idle]=0;
 assert(responsive.values[Reaction]==0);s=fixture();s.at=60;p={};i.activation=true;
 d=p.step(responsive,s,i,60);assert(d.moving);auto before=d.destination;
 s.actors[0].p.x=3;s.at=60.02;d=p.step(responsive,s,i,s.at);assert(d.destination.x>before.x+1);
 s.actors[0].p.x=-3;s.at=60.04;d=p.step(responsive,s,i,s.at);assert(d.destination.x<0);
 std::cout<<"Zero-delay defaults and every-frame destination updates passed\n"; responsive.values[Adaptive]=0;responsive.values[Cutoff]=0;
 s=fixture();s.at=70;s.own.p={-.8f,3,0};s.own.v={16,0,0};p={};
 d=p.step(responsive,s,i,70);assert(!d.moving&&!d.sprint&&d.stance);
 s.own.p.x=-1.2f;s.own.v={-16,0,0};s.at=70.02;d=p.step(responsive,s,i,s.at);assert(d.moving);
 assert(Config{}.values[Idle]==0);
 std::cout<<"Arrival braking, recovery and stationary-default regressions passed\n"; // Tiny stationary-target noise must not generate alternating movement taps.
 s=fixture();s.own.p={.4f,3,0};s.at=80;p={};responsive.values[Idle]=0;
 d=p.step(responsive,s,i,80);auto steady=d.destination;assert(!d.moving);
 for(int tick=1;tick<=100;tick++){s.at=80+tick*.02;s.actors[0].p.x=tick%2?.06f:-.06f;s.actors[0].v={tick%2?.3f:-.3f,0,0};d=p.step(responsive,s,i,s.at);assert(!d.moving);assert(distance(d.destination,steady)<.001f);}
 s.actors[0].p.x=2;s.actors[0].v={8,0,0};s.at=82.02;d=p.step(responsive,s,i,s.at);assert(d.moving&&d.destination.x>1);
 // Braking into a stationary assignment holds rather than restarting next tick.
 s=fixture();s.own.p={-.8f,3,0};s.own.v={16,0,0};s.at=83;p={};d=p.step(responsive,s,i,83);assert(!d.moving);
 s.own.v={};s.at=83.02;d=p.step(responsive,s,i,s.at);assert(!d.moving);
 std::cout<<"Stationary noise, immediate real movement and stable arrival hold checks passed\n"; // Residual velocity noise near a settled point cannot restart left/right taps.
 s=fixture();s.own.p={-.4f,3,0};s.at=90;p={};d=p.step(responsive,s,i,90);assert(!d.moving);
 for(int tick=1;tick<=80;tick++){s.at=90+tick*.02;s.own.p.x=tick%2?.8f:-.8f;s.own.v={tick%2?12.f:-12.f,0,0};d=p.step(responsive,s,i,s.at);assert(!d.moving);}
 // Closeout is movement only until an actual release; contact spacing cannot
 // push the defender back to 3.2 studs during the closeout.
 Config shot;shot.values[Guard]=1;shot.values[Idle]=0;shot.values[Block]=1;shot.values[Oop]=1;shot.values[Rebound]=1;
 s=fixture();s.own.p={0,3,0};s.actors[0].goal={0,10,5};s.balls[0].p={0,5,-5};s.at=100;p={};
 d=p.step(shot,s,i,100);assert(!d.jump);s.actors[0].shooting=true;s.at=100.13;d=p.step(shot,s,i,s.at);
 assert(d.moving&&!d.jump&&d.direction.z<0);assert(std::abs(distance(d.destination,s.actors[0].p)-2)<.001f);
 Ball distraction;distraction.id=999;distraction.previousOwner=2;distraction.p={0,8,-3};distraction.v={0,-5,2};distraction.velocityValid=true;distraction.lob=true;distraction.releasedAt=100;s.balls.push_back(distraction);
 s.at=100.15;d=p.step(shot,s,i,s.at);assert(d.mode==Guarding&&d.target==2); // no lob/rebound takeover
 s.balls[0].owner=0;s.balls[0].shot=true;s.balls[0].releasedAt=100.16;s.at=100.18;d=p.step(shot,s,i,s.at);assert(d.target==2&&std::abs(distance(d.destination,s.actors[0].p)-2)<.001f);
 i.activation=false;s.at=100.2;d=p.step(shot,s,i,s.at);assert(d.reason!="Closing shot gather / shot release");
 std::cout<<"Settled velocity noise, shot closeout and movement priority checks passed\n"; // Dribble animation may move the ball wildly without moving the player's root.
 // It must not change the stand-off target or create a closeout.
 Config body;body.values[Guard]=1;body.values[Gap]=800;body.values[Idle]=0;body.values[Oop]=1;body.values[Rebound]=1;
 s=fixture();s.own.p={0,3,3};s.actors[0].goal={0,10,100};s.at=110;p={};i.activation=true;
 d=p.step(body,s,i,110);auto rootTarget=d.destination;assert(!d.moving);
 for(int tick=1;tick<=150;tick++){s.at=110+tick*.02;s.balls[0].p={tick%2?4.f:-4.f,3.f+(tick%30)*.25f,-5};d=p.step(body,s,i,s.at);assert(!d.moving&&!d.jump);assert(distance(rootTarget,d.destination)<.001f);}
 // Straight backpedaling has no lateral destination or steering component.
 s=fixture();s.own.p={0,3,3};s.actors[0].goal={0,10,100};s.actors[0].v={0,0,-8};p={};
 for(int tick=0;tick<150;tick++){s.at=120+tick*.02;s.actors[0].p.z-=.16f;d=p.step(body,s,i,s.at);assert(std::abs(d.destination.x)<.001f&&std::abs(d.direction.x)<.001f);if(d.moving)s.own.p=s.own.p+d.direction*.32f;}
 // No loose-ball movement takeover after the activation key is released.
 s=fixture();s.at=130;s.balls[0].owner=0;s.balls[0].v={0,-5,0};s.balls[0].velocityValid=true;s.balls[0].releasedAt=129.9;i.activation=false;p={};d=p.step(body,s,i,130);assert(!d.moving);
 // Recover directly toward the basket line, not along a contact tangent.
 i.activation=true;s=fixture();s.actors[0].goal={0,10,100};s.own.p={1,3,-8};s.at=131;p={};d=p.step(body,s,i,131);assert(d.direction.x<0&&d.direction.z>0);
 std::cout<<"Ball-animation invariance, straight backpedal and no automatic flight takeover passed\n"; // Small feints do not change the accepted position or prediction velocity.
 MeaningfulMotion intentMotion;Actor mover=fixture().actors[0];mover.goal={0,10,100};intentMotion.observe(mover,false);Vec base=intentMotion.position;
 for(int n=0;n<80;n++){mover.p.x=n%2?.4f:-.4f;mover.v={n%2?12.f:-12.f,0,0};intentMotion.observe(mover,false);assert(distance(intentMotion.position,base)==0);assert(length(intentMotion.velocity)==0);}
 mover.p.x=1;intentMotion.observe(mover,false);assert(intentMotion.position.x==1);
 mover.p.z+=.2f;mover.v={0,0,8};intentMotion.observe(mover,false);assert(intentMotion.position.z==mover.p.z);
 mover.p.x+=.1f;intentMotion.observe(mover,true);assert(intentMotion.position.x==mover.p.x);
 intentMotion={};mover=fixture().actors[0];intentMotion.observe(mover,false);for(int n=0;n<20;n++){mover.p.x+=.05f;intentMotion.observe(mover,false);}assert(intentMotion.position.x>.8f);
 // A brief bounce or ball held to one side does not qualify as a gather.
 ShotGather cue;Ball high;high.owner=mover.id;high.p=mover.p+Vec{0,2.2f,0};mover.v={};
 assert(!cue.update(&mover,&high,200));assert(!cue.update(&mover,&high,200.04));
 high.p.y=mover.p.y;assert(!cue.update(&mover,&high,200.06));high.p=mover.p+Vec{3,2.2f,0};assert(!cue.update(&mover,&high,200.2));
 high.p=mover.p+Vec{0,2.2f,0};assert(!cue.update(&mover,&high,200.3));assert(cue.update(&mover,&high,200.39));
 // Missing action metadata still gets a pre-release closeout, without a jump.
 s=fixture();s.own.p={0,3,3};s.actors[0].goal={0,10,100};s.balls[0].p=s.actors[0].p+Vec{0,2.2f,0};s.at=210;p={};i.activation=true;
 d=p.step(body,s,i,210);assert(!d.moving);s.at=210.09;d=p.step(body,s,i,s.at);assert(d.moving&&!d.jump&&distance(d.destination,s.actors[0].p)<2.01f);
 std::cout<<"Meaningful movement, accumulated slow movement and pre-release shot-gather checks passed\n"; // Explanations come from the same branch as the command, not a separate guess.
 s=fixture();s.own.p={0,3,0};s.at=220;p={};i.activation=true;Config explain;explain.values[Guard]=1;explain.values[Idle]=0;
 d=p.step(explain,s,i,220);assert(d.observation=="Stationary / stopped"&&d.response=="Holding the lane");
 s.actors[0].p.x=.4f;s.actors[0].v={-8,0,0};s.at=220.02;d=p.step(explain,s,i,s.at);assert(d.observation=="Small motion filtered");
 s.actors[0].shooting=true;s.at=220.04;d=p.step(explain,s,i,s.at);assert(d.observation=="Shooting cue"&&d.response=="Closing the shooter");
 MeaningfulMotion stop;mover=fixture().actors[0];mover.v={8,0,0};stop.observe(mover,false);assert(length(stop.velocity)>0);mover.v={};stop.observe(mover,false);assert(length(stop.velocity)==0);
 std::cout<<"Decision explanations and stop-prediction regression checks passed\n";
 // Meter closeout bypasses acquisition delay; high dribble/action metadata is
 // not needed. It remains gated by possession, team, focus, and activation.
 Config meterConfig;meterConfig.values[Guard]=1;meterConfig.values[Gap]=800;meterConfig.values[Reaction]=500;meterConfig.values[Idle]=0;
 s=fixture();s.own.p={0,3,3};s.actors[0].goal={0,10,100};s.actors[0].shotMeterActive=true;s.at=230;p={};i={};i.focused=i.activation=true;
 d=p.step(meterConfig,s,i,230);assert(d.moving&&!d.jump&&d.direction.z<0&&d.reason=="Closing active shot meter");
 assert(std::abs(distance(d.destination,s.actors[0].p)-2)<.001f);
 s.own.p={0,3,-2.4f};s.at=230.02;d=p.step(meterConfig,s,i,s.at);assert(d.moving); // 2.6 studs is still too far
 s.own.p={0,3,-2.8f};s.at=230.04;d=p.step(meterConfig,s,i,s.at);assert(!d.moving); // stop at closeout tolerance
 s.actors[0].shotMeterActive=false;s.at=230.6;d=p.step(meterConfig,s,i,s.at);assert(std::abs(distance(d.destination,s.actors[0].p)-8)<.001f);
 s.actors[0].shotMeterActive=true;s.balls.clear();p={};d=p.step(meterConfig,s,i,s.at);assert(!d.moving);
 s=fixture();s.actors[0].shotMeterActive=true;s.actors[0].enemy=false;p={};d=p.step(meterConfig,s,i,s.at);assert(!d.moving);
 s.actors[0].enemy=true;i.activation=false;p={};d=p.step(meterConfig,s,i,s.at);assert(!d.moving);
 i.activation=true;i.focused=false;p={};d=p.step(meterConfig,s,i,s.at);assert(!d.moving);
 i.focused=true;i.chat=true;p={};d=p.step(meterConfig,s,i,s.at);assert(!d.moving);
 std::cout<<"Meter-only closeout, delay bypass, tighter arrival, cancel and eligibility checks passed\n";
}











