#include "defense_logic.hpp"
#include <iostream>
#include <sstream>
#include <cstdlib>
using namespace defense;
static int checks=0;
static void check(bool ok,const char*message){++checks;if(!ok){std::cerr<<"FAIL: "<<message<<"\n";std::exit(1);}}
static Scene scene(){Scene s;s.valid=s.grounded=true;s.at=10;s.floor=0;s.own={1,"self",{8,3,0},{},{0,0,-1},{0,10,20},16,false,true};s.cameraForward={0,0,-1};s.cameraRight={1,0,0};s.actors.push_back({2,"handler",{0,3,-5},{},{0,0,1},{0,10,20},16,true,true});s.actors.push_back({3,"receiver",{10,3,-10},{},{0,0,1},{0,10,20},16,true,true});Ball b;b.id=100;b.owner=2;b.p={0,4,-4};s.balls.push_back(b);return s;}
static Intent intent(){Intent i;i.focused=i.activation=true;return i;}
static void tick(Planner&p,const Config&c,Scene&s,const Intent&i,double t){s.at=t;p.step(c,s,i,t);}
int main(){
 for(int mode=0;mode<3;mode++){
  options[Gap].value=775;options[Block].value=0;applyResponsePreset(mode);Config preset;preset.snapshot();
  check(preset.values[Response]==mode,"mode persists");check(preset.values[Gap]==775&&!preset.on(Block),"presets preserve stand-off and feature toggles");
  check(preset.values[Reaction]==(mode==0?80:mode==1?20:0),"preset acquisition delay");
  check(preset.on(Humanize)==(mode!=2)&&!preset.on(Idle),"preset steering and no idle wiggle");
 }
 loadSettings([](const char*,int d){return d;});
 // Actual clients can retain possession beyond the reachable release window.
 // Validate the earlier multi-cue opportunity, without relying on release flags.
 auto chargeScene=scene();chargeScene.own.p={0,3,0};chargeScene.balls[0].p={0,6.2f,-3};chargeScene.balls[0].heldRiseSpeed=5;chargeScene.balls[0].flight=3;
 chargeScene.actors[0].shotMeterActive=chargeScene.actors[0].shotMeterAdvancing=true;chargeScene.actors[0].shotMeterFill=.45f;chargeScene.actors[0].shotMeterAge=.3;
 Config bc;bc.values[Block]=1;auto bi=intent();bi.manual=true;Planner bp;
 auto bd=bp.step(bc,chargeScene,bi,10);check(bd.jump&&bd.mode==Blocking,"late charge plus rising ball blocks while manually moving");
 chargeScene.at=11.5;check(!bp.step(bc,chargeScene,bi,11.5).jump,"same charge cannot repeat after cooldown");
 auto released=chargeScene;auto&rb=released.balls[0];rb.owner=0;rb.previousOwner=2;rb.flight=4;rb.shot=rb.velocityValid=true;rb.v={0,12,2};rb.releasedAt=11.4;
 check(!bp.step(bc,released,bi,11.5).jump,"early block reserves following release");
 chargeScene.at=12;chargeScene.balls[0].flight=5;check(bp.step(bc,chargeScene,bi,12).jump,"new possession can block again");
 for(int mode=0;mode<3;mode++){
  bc.values[Response]=mode;float threshold=mode==0?.40f:mode==1?.30f:.20f;
  chargeScene.actors[0].shotMeterFill=threshold-.01f;bp={};check(!bp.step(bc,chargeScene,bi,12).jump,"mode waits for its charge threshold");
  chargeScene.actors[0].shotMeterFill=threshold+.001f;bp={};check(bp.step(bc,chargeScene,bi,12).jump,"mode qualifies at late-charge threshold");
 }
 bc.values[Response]=1;chargeScene.actors[0].shotMeterFill=.45f;
 auto rejects=[&](Scene candidate,const char*why){Planner clean;check(!clean.step(bc,candidate,bi,candidate.at).jump,why);};
 auto bad=chargeScene;bad.balls[0].heldRiseSpeed=0;rejects(bad,"static raised ball is not an early block");
 bad=chargeScene;bad.balls[0].heldRiseSpeed=-3;rejects(bad,"falling ball is not an early block");
 bad=chargeScene;bad.balls[0].heldRiseSpeed=NAN;rejects(bad,"invalid ball rise rejected");
 bad=chargeScene;bad.balls[0].p.y=4.5f;rejects(bad,"low charging ball cannot trigger dive");
 bad=chargeScene;bad.balls[0].p.y=12;rejects(bad,"unreachable high charge rejected");
 bad=chargeScene;bad.balls[0].p.z=-10;rejects(bad,"distant charge rejected");
 bad=chargeScene;bad.own.facing={0,0,1};rejects(bad,"charge behind defender rejected");
 bad=chargeScene;bad.grounded=false;rejects(bad,"airborne defender cannot request jump");
 bad=chargeScene;bad.actors[0].enemy=false;rejects(bad,"friendly charge rejected");
 bad=chargeScene;bad.actors[0].shotMeterActive=false;rejects(bad,"expired meter cannot trigger early block");
 bad=chargeScene;bad.actors[0].shotMeterFill=NAN;rejects(bad,"invalid meter fill rejected");
 bad=chargeScene;bad.actors[0].shotMeterAge=.01;rejects(bad,"charge respects block reaction delay");
 bad=chargeScene;bc.values[Block]=0;rejects(bad,"auto block off never requests early block");bc.values[Block]=1;
 bad=chargeScene;bi.focused=false;rejects(bad,"focus loss cancels early blocks");bi.focused=true;bi.chat=true;rejects(bad,"chat cancels early blocks");bi.chat=false;
 Planner releaseFallback;released.at=13;released.balls[0].releasedAt=12.9;released.balls[0].flight=0;
 check(!releaseFallback.step(bc,released,bi,13).jump,"release-only fallback is disabled even without meter");
 // Sequence regression: balanced must jump while the charge is still progressing,
 // before the old 85% threshold or the ownership release, then never make up late.
 auto sequence=chargeScene;sequence.at=20;sequence.balls[0].flight=20;sequence.balls[0].p={0,5.7f,-3};sequence.balls[0].heldRiseSpeed=5;sequence.balls[0].meterObserved=true;
 sequence.actors[0].shotMeterFill=.25f;sequence.actors[0].shotMeterAge=.12;Planner timing;
 check(!timing.step(bc,sequence,bi,20).jump,"balanced waits through early low charge");
 sequence.at=20.02;sequence.actors[0].shotMeterFill=.60f;
 check(timing.step(bc,sequence,bi,20.02).jump,"balanced starts jump at rising gather before 85 percent and release");
 sequence.at=20.2;sequence.balls[0].owner=0;sequence.balls[0].previousOwner=2;sequence.balls[0].releasedAt=20.1;sequence.balls[0].flight=21;sequence.balls[0].p.y=7;sequence.balls[0].shot=sequence.balls[0].velocityValid=true;sequence.balls[0].v={0,15,0};
 Planner missedWindup;check(!missedWindup.step(bc,sequence,bi,20.2).jump,"missed tracked windup never jumps after release");
 sequence.balls[0].meterObserved=false;sequence.at=20.25;check(!missedWindup.step(bc,sequence,bi,20.25).jump,"untracked late release is also outside fallback window");
 bad=chargeScene;bad.actors[0].shotMeterAdvancing=false;rejects(bad,"held ownership with stopped meter cannot cause late jump");
 bad=chargeScene;bad.actors[0].shotMeterFill=1;rejects(bad,"completed meter cannot trigger windup jump");
 bad=chargeScene;bad.balls[0].p.z=0;rejects(bad,"ball detached from shooter cannot trigger windup jump");
 bad=chargeScene;bad.balls[0].p.y=5.3f;bad.balls[0].heldRiseSpeed=1.1f;rejects(bad,"low slowly rising ball cannot exploit jump lead");
 bad=chargeScene;bad.actors[0].shotReleased=true;rejects(bad,"explicit release defeats lagging ownership and advancing meter");
 for(int mode=0;mode<3;mode++){
  bc.values[Response]=mode;bad=chargeScene;bad.actors[0].shotMeterFill=mode==0?.80f:mode==1?.75f:.70f;
  rejects(bad,"late windup deadline forbids block even while ownership still held");
 }
 bc.values[Response]=1;bad=chargeScene;bad.balls[0].p.y=5;bad.balls[0].heldRiseSpeed=15;
 Planner lift;check(lift.step(bc,bad,bi,bad.at).jump,"fast upward gather above 1.8 starts before old high-ball gate");
 // Short-horizon visual forecasts use accepted player movement, never ball motion.
 Decision forecast;forecast.predictionValid=true;forecast.predictionAnchor={2,3,4};forecast.predictionVelocity={10,50,0};
 auto near=predictedOpponent(forecast,.175f),far=predictedOpponent(forecast,.35f);
 check(std::abs(near.x-3.75f)<.001f&&std::abs(far.x-5.5f)<.001f&&far.y==3,"forecast times and flat court movement");
 check(distance(predictedOpponent(forecast,4),far)<.001f,"forecast horizon capped");
 forecast.predictionVelocity={1000,0,0};check(distance(predictedOpponent(forecast,.35f),forecast.predictionAnchor)<=6.001f,"forecast displacement bounded");
 forecast.predictionValid=false;check(distance(predictedOpponent(forecast,.35f),forecast.predictionAnchor)==0,"no forecast when planner has no movement estimate");
 Config c;c.values[Guard]=1;c.values[Reaction]=0;c.values[Humanize]=0;auto s=scene();auto i=intent();Planner p;auto d=p.step(c,s,i,10);check(d.moving&&d.stance,"on-ball guard engages");check(d.destination.z==0,"basket-side guard target");check(!d.steal&&!d.jump,"guard alone never steals or blocks");
 check(d.predictionValid&&distance(d.predictionAnchor,s.actors[0].p)<.001f,"visual forecast anchor supplied by planner");
 i.activation=false;d=p.step(c,s,i,10.02);check(!d.moving&&!d.stance,"release activation immediately stops guarding");i.activation=true;i.focused=false;d=p.step(c,s,i,10.03);check(!d.moving&&!d.jump&&!d.steal,"focus loss releases actions");i.focused=true;i.chat=true;d=p.step(c,s,i,10.04);check(d.mode==Waiting&&!d.stance,"typing pauses defense");i.chat=false;d=p.step(c,s,i,10.3);check(!d.moving,"stale scene pauses defense");s.at=10.3;s.playingKnown=true;s.playing=false;check(p.step(c,s,i,10.3).mode==Waiting,"known lobby does not automate");s.playing=true;s.balls[0].owner=1;check(!p.step(c,s,i,10.3).moving,"own possession stops defense");
 s=scene();c.values[Offball]=1;c.values[OffDelay]=0;i=intent();i.selected=3;d=p.step(c,s,i,10);check(d.mode==Denying&&d.target==3,"selected receiver drives off-ball guard");check(d.destination.x<s.actors[1].p.x,"deny between receiver and handler");s.balls[0].owner=3;d=p.step(c,s,i,10);check(d.mode==Guarding&&d.target==3,"selected receiver gaining ball hands off to on-ball");
 c=Config{};s=scene();i=intent();s.own.p={0,3,0};s.balls[0].p={0,4,-2};Planner steal;c.values[Steal]=1;c.values[StealDelay]=120;check(!steal.step(c,s,i,10).steal,"steal waits for continuous opportunity");s.at=10.13;check(steal.step(c,s,i,10.13).steal,"steal fires in range after delay");s.at=10.3;check(!steal.step(c,s,i,10.3).steal,"steal cooldown shared");s.balls[0].p={0,4,2};s.at=12;check(!steal.step(c,s,i,12).steal,"steal rejects ball behind player");
 c=Config{};s=scene();s.own.p={0,3,0};auto&b=s.balls[0];b.owner=0;b.previousOwner=2;b.p={0,7,-3};b.v={0,10,3};b.velocityValid=true;b.shot=true;b.releasedAt=9.9;b.flight=1;c.values[Block]=1;Planner block;d=block.step(c,s,i,10);check(!d.jump,"released upward flight never triggers auto block");s.at=12;b.releasedAt=11.9;check(!block.step(c,s,i,12).jump,"same flight only jumps once");++b.flight;s.at=13.3;b.releasedAt=13.2;b.p.y=3.4f;check(!block.step(c,s,i,13.3).jump,"low ball never causes dive");b.p.y=7;b.owner=2;check(!block.step(c,s,i,13.3).jump,"windup possession alone never triggers auto block");
 c=Config{};c.values[Oop]=1;s=scene();s.own.p={0,3,0};auto&lob=s.balls[0];lob.owner=0;lob.previousOwner=2;lob.lob=true;lob.velocityValid=true;lob.p={0,8,-3};lob.v={0,-2,2};lob.releasedAt=9.7;lob.flight=3;Planner oop;d=oop.step(c,s,i,10);check(d.mode==LobTracking&&!d.jump,"lob is reserved without launch jump");lob.boardHit=true;lob.boardAt=10;s.at=10.02;d=oop.step(c,s,i,10.02);check(d.jump,"confirmed board deflection in reach jumps");check(!d.steal,"lob never catches automatically with steal off");
 c=Config{};c.values[Intercept]=1;s=scene();s.own.p={0,3,0};auto&pass=s.balls[0];pass.owner=0;pass.previousOwner=2;pass.p={0,4,-3};pass.v={0,0,20};pass.velocityValid=true;pass.releasedAt=9.9;Planner intercept;i.catchHeld=false;check(intercept.step(c,s,i,10).mode!=Intercepting,"intercept requires explicit activation");i.catchHeld=true;d=intercept.step(c,s,i,10);check(d.mode==Intercepting,"chest-height passes are reachable");check(d.steal&&!d.jump,"manual catch can request normal catch without low-ball dive");i.manual=true;d=intercept.step(c,s,i,10.02);check(!d.moving,"manual movement overrides pursuit");i.manual=false;i.catchHeld=false;
 c=Config{};c.values[Rebound]=1;s=scene();s.own.p={0,3,0};auto&reb=s.balls[0];reb.owner=0;reb.previousOwner=2;reb.p={0,8,-3};reb.v={0,-5,0};reb.velocityValid=true;Planner rebound;d=rebound.step(c,s,i,10);check(d.mode==Rebounding&&d.jump,"descending high reachable ball triggers rebound");reb.p.y=.8;reb.v={2,0,0};s.at=11.4;check(!rebound.step(c,s,i,11.4).jump,"floor ball cannot trigger rebound jump");
 c=Config{};c.values[Guard]=1;c.values[Reaction]=0;c.values[Screens]=1;s=scene();s.actors.push_back({4,"screen",{4,3,0},{},{0,0,1},{},16,true,false});Planner screen;d=screen.step(c,s,i,10);check(d.obstacle==4,"screen on route is detected");check(fabs(d.destination.z)>3,"screen creates lateral detour");
 s.own.p.x=NAN;check(!screen.step(c,s,i,10).moving,"invalid positions do not move");
 std::ostringstream output;writeSettings(output);for(auto&o:options)check(output.str().find(std::string("def_")+o.key)!=std::string::npos,"every setting serialized");loadSettings([](const char*,int){return 999999;});for(auto&o:options)check(o.value<=o.hi,"loaded settings clamped");loadSettings([](const char*,int d){return d;});
 std::cout<<checks<<" defense checks passed\n";
}
