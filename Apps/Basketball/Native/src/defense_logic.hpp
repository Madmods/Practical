#pragma once
#include "shooting_logic.hpp"
#include "defense_settings.hpp"
#include "defense_matchup.hpp"
#include <map>
#include <string>
namespace defense {
using shooting::Vec;using Identity=uintptr_t;
// Missing or unreadable team membership is not evidence of an opponent.
inline bool opposingTeams(Identity ownTeam,Identity otherTeam,bool isSelf=false){return !isSelf&&ownTeam!=0&&otherTeam!=0&&ownTeam!=otherTeam;}
// Per-character assigned goals describe the court matchup when global Teams
// are unused. Only explicit ObjectValue assignments qualify, never nearest-rim estimates.
inline bool opposingMatchup(Identity ownTeam,Identity otherTeam,Identity ownGoal,Identity otherGoal,bool isSelf=false){
 if(isSelf)return false;
 if(ownGoal&&otherGoal)return ownGoal!=otherGoal;
 return opposingTeams(ownTeam,otherTeam);
}
using shooting::length;
inline bool practiceOpponent(const std::string& name,bool mappedPlayer,bool hasHumanoid,bool explicitGoal,bool enabled){
 if(!enabled||mappedPlayer||!hasHumanoid||!explicitGoal||name.size()<3||name.compare(0,2,"AI")!=0)return false;
 return std::all_of(name.begin()+2,name.end(),[](char c){return c>='0'&&c<='9';});
}
inline Vec flat(Vec a){a.y=0;return a;}inline float dot(Vec a,Vec b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline Vec unit(Vec a){float n=length(a);return n>.001f?a*(1/n):Vec{};}inline float distance(Vec a,Vec b){return length(flat(a-b));}
inline bool finite(Vec a){return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z)&&length(a)<1e6f;}
// Separate engage/release thresholds prevent digital movement-key chatter.
inline bool movementAxis(float amount,bool wasDown){return amount>(wasDown?.22f:.38f);}
inline bool arrivalBrake(Vec error,Vec relativeVelocity){
 error=flat(error);relativeVelocity=flat(relativeVelocity);
 if(!finite(error)||!finite(relativeVelocity)||length(error)>1.5f)return false;
 float speed2=dot(relativeVelocity,relativeVelocity);if(speed2<1)return false;
 float at=dot(error,relativeVelocity)/speed2;
 return at>0&&at<=.07f&&length(error-relativeVelocity*at)<.55f;
}
struct Config{std::array<int,Count> values{};Config(){for(int i=0;i<Count;i++)values[i]=options[i].initial;}void snapshot(){for(int i=0;i<Count;i++)values[i]=options[i].value.load();}bool on(Id i)const{return values[i]!=0;}float v(Id i)const{return values[i]*options[i].scale;}double seconds(Id i)const{return values[i]*.001;}};
struct Actor{Identity id=0;std::string name;Vec p{},v{},facing{0,0,-1},goal{};float speed=16;bool enemy=false,goalValid=false;bool shooting=false,shotMeterActive=false;float shotMeterFill=0;double shotMeterAge=0;bool shotMeterAdvancing=false,shotReleased=false;bool shotMeterRising=false;};
struct Ball{Identity id=0,owner=0,previousOwner=0;Vec p{},v{};float gravity=45;bool velocityValid=false,shot=false,lob=false,boardHit=false;double releasedAt=-10,boardAt=-10;unsigned flight=0;float heldRiseSpeed=0;bool meterObserved=false;};
struct Scene{bool valid=false,grounded=false,playingKnown=false,playing=false;double at=0;Actor own;float floor=0;Vec cameraForward{},cameraRight{};std::vector<Actor> actors;std::vector<Ball> balls;};
struct Intent{bool focused=false,activation=false,catchHeld=false,manual=false,chat=false;Identity selected=0;};
enum Mode{Off,Waiting,Guarding,Denying,Intercepting,Blocking,LobTracking,Rebounding,Selecting};
struct Decision{Mode mode=Off;Vec destination{},direction{},face{},marker{};Identity target=0,obstacle=0,ball=0;bool moving=false,stance=false,sprint=false,jump=false,steal=false,showMarker=false;float coverage=0;std::string reason="Defense off";bool retreat=false;Vec basketAxis{};std::string observation="No active matchup",response="Waiting",explanation="Waiting for a defensive scene";Vec predictionAnchor{},predictionVelocity{};bool predictionValid=false;std::string blockReason="No opponent windup";};
inline Vec predictedOpponent(const Decision& d,float seconds){
 if(!d.predictionValid||!finite(d.predictionAnchor)||!finite(d.predictionVelocity)||!std::isfinite(seconds))return d.predictionAnchor;
 return d.predictionAnchor+shooting::cap(flat(d.predictionVelocity)*std::clamp(seconds,0.f,.35f),6.f);
}
inline const Actor* actor(const Scene&s,Identity id){if(id==s.own.id)return &s.own;for(auto&a:s.actors)if(a.id==id)return &a;return nullptr;}
inline const Ball* heldBall(const Scene&s,Identity id){for(auto&b:s.balls)if(b.owner==id)return &b;return nullptr;}
struct Meeting{bool valid=false;Vec p{};float time=0;};
inline Meeting meet(const Scene&s,const Ball&b,float reach,float maxHeight,float horizon,float lead=0,float minHeight=1.8f){
 if(!b.velocityValid||!finite(b.p)||!finite(b.v))return {};for(float t=.04f;t<=horizon;t+=.025f){float dt=t+lead;Vec p=b.p+b.v*dt+Vec{0,-.5f*b.gravity*dt*dt,0};float h=p.y-s.own.p.y;if(h<minHeight||h>maxHeight)continue;if(distance(s.own.p,p)<=s.own.speed*t+reach-.25f)return {true,p,t};}return {};
}
inline bool facing(const Actor&a,Vec p,float threshold=.15f){Vec d=flat(p-a.p);return length(d)<.1f||dot(unit(a.facing),unit(d))>=threshold;}
// Geometric post-up estimate, not a replicated game action or push hitbox.
// Require a sustained back-to-basket stance; a brief pivot must not change spacing.
struct PostUpRead {
 Identity owner=0,defender=0;double since=-1,last=-1,seen=-1;bool active=false;
 void reset(){*this={};}
 bool update(bool eligible,const Actor* a,const Actor& own,double t){
  if(!eligible||!a||!a->enemy||!a->goalValid||!std::isfinite(t)||!finite(a->p)||!finite(a->v)||!finite(a->facing)||!finite(a->goal)||!finite(own.p)||!finite(own.v)||length(flat(a->facing))<.5f||fabs(a->v.y)>2.5f){reset();return false;}
  if(owner!=a->id||defender!=own.id||last<0||t<=last||t-last>.12){reset();owner=a->id;defender=own.id;}
  last=t;
  const Vec axis=unit(flat(a->goal-a->p)),toDefender=unit(flat(own.p-a->p)),look=unit(flat(a->facing));
  const float speed=length(flat(a->v)),toward=dot(flat(a->v),axis);
  const bool plausible=distance(a->p,a->goal)<=22&&distance(a->p,own.p)<=18&&
   dot(look,axis)<-.55f&&dot(look,toDefender)<-.45f&&dot(toDefender,axis)>.2f&&speed<=7&&toward>=-.6f;
  if(plausible){if(since<0)since=t;seen=t;if(t-since>=.18)active=true;}
  else {since=-1;if(!active||t-seen>.12||speed>9||toward< -2||dot(look,axis)>.1f)active=false;}
  return active;
 }
};
inline constexpr float PostBuffer=3.5f;
// Do not cut through the handler while correcting laterally around the buffer.
inline Vec postRoute(Vec away,Vec desired){
 const float separation=length(flat(away));if(separation<.01f)return desired;
 const Vec radial=unit(flat(away));const float inward=dot(desired,radial);
 if(inward<0&&length(flat(away)+desired*.8f)<PostBuffer)
  return unit(desired-radial*inward+radial*.15f);
 return desired;
}
struct MeaningfulMotion {
 Identity id=0;Vec position{},velocity{};bool valid=false,accepted=false;
 void observe(const Actor& a,bool urgent){
  accepted=false;Vec axis=unit(flat(a.goal-a.p)),delta=flat(a.p-position);
  float depth=dot(delta,axis),lateral=length(delta-axis*depth);
  bool drive=depth>.12f&&dot(flat(a.v),axis)>3;
  if(!valid||id!=a.id||urgent||drive||lateral>.85f||fabs(depth)>.65f){
   id=a.id;position=a.p;velocity=a.v;valid=true;accepted=true;
  }
  if(length(flat(a.v))<.8f)velocity={};
 }
};
// A meter must actually advance. Static UI, reset animations, missing samples,
// and a replacement gradient never count as a shot. No jump is inferred here.
struct ShotMeter {
 Identity source=0;float previous=0,runFill=0;double at=-1,lastRise=-10,activeUntil=-10,activeSince=-1;int rises=0;
 void reset(){*this={};}
 bool update(Identity current,float fill,double t){
  if(!current||!std::isfinite(fill)||fill<0||fill>1.05f||!std::isfinite(t)){reset();return false;}
  auto seed=[&](){reset();source=current;previous=runFill=fill;at=t;};
  if(source!=current||at<0||t<=at||t-at>.12){seed();return false;}
  float delta=fill-previous;
  if(delta<-.005f){seed();return false;}
  if(delta>.004f){
   if(rises==0||t-lastRise>.12){rises=0;runFill=previous;}
   rises=std::min(rises+1,3);lastRise=t;
   if(rises>=2&&fill-runFill>=.015f){if(t>activeUntil)activeSince=t;activeUntil=t+.18;}
  }else if(delta<-.001f)rises=0; // Duplicate samples can occur between UI updates.
  previous=fill;at=t;return t<=activeUntil;
 }
};
struct ShotGather {
 Identity owner=0;double since=-1;
 bool update(const Actor* a,const Ball* ball,double t){
  if(!a||!ball){owner=0;since=-1;return false;}
  float height=ball->p.y-a->p.y;
  bool plausible=height>=1.9f&&height<=4.5f&&distance(ball->p,a->p)<=1.6f&&length(flat(a->v))<6;
  if(!plausible){owner=0;since=-1;return false;}
  if(owner!=a->id||since<0){owner=a->id;since=t;}
  return t-since>=.08;
 }
};
// Read the held-ball side, not its frame-by-frame bounce. A hand change must
// persist before we move the anchor, and the bias never exceeds0.65 studs.
struct BallSideRead {
 Identity owner=0,ball=0;double last=-1,since=-1;int candidate=0,side=0;float offset=0;
 void reset(){*this={};}
 float update(const Actor* a,const Ball* b,double t){
  if(!a||!b||!a->goalValid||!finite(b->p)||!finite(a->p)||!finite(a->goal)||!std::isfinite(t)){reset();return 0;}
  if(owner!=a->id||ball!=b->id||last<0||t<=last||t-last>.12){reset();owner=a->id;ball=b->id;last=t;}
  const double dt=std::clamp(t-last,0.,.1);last=t;
  Vec axis=unit(flat(a->goal-a->p)),lateral{-axis.z,0,axis.x};
  float x=dot(flat(b->p-a->p),lateral),height=b->p.y-a->p.y;
  bool attached=distance(b->p,a->p)<=3.5f&&height>=-4&&height<=4;
  int next=attached&&std::abs(x)>=.4f?(x>0?1:-1):0;
  if(next!=candidate){candidate=next;since=t;}
  if(since<0)since=t;
  if(t-since>=(candidate?.12:.18))side=candidate;
  float target=side*.65f;offset+=(float)std::clamp(double(target-offset),-2.5*dt,2.5*dt);
  return offset;
 }
};
// Report the first failed windup gate so a skipped attempt can be diagnosed.
// The deadline is intentionally early: ownership alone can lag behind release.
inline const char* blockWindupGate(const Config& c,const Scene& s,const Actor& a,const Ball& b){
 float threshold=c.values[Response]==0?.40f:c.values[Response]==2?.20f:.30f;
 float deadline=c.values[Response]==0?.80f:c.values[Response]==2?.70f:.75f;
 float lead=c.values[Response]==0?.09f:c.values[Response]==2?.18f:.14f;
 if(!b.owner||a.shotReleased)return "Release observed";
 if(!a.enemy)return "Not an opponent";
 if(!a.shotMeterActive||!a.shotMeterAdvancing)return "Waiting for advancing charge";
 if(!std::isfinite(a.shotMeterFill)||a.shotMeterFill>=deadline)return "Windup window passed";
 if(a.shotMeterFill<threshold)return "Charge below early threshold";
 if(!std::isfinite(a.shotMeterAge)||a.shotMeterAge<c.seconds(BlockDelay))return "Block reaction delay";
 if(!finite(b.p)||!std::isfinite(b.heldRiseSpeed)||b.heldRiseSpeed<=1)return "Waiting for upward ball lift";
 float height=b.p.y-s.own.p.y;
 if(height<1.8f||height+b.heldRiseSpeed*lead<2.5f)return "Ball too low for jump";
 if(distance(b.p,a.p)>3.5f||b.p.y-a.p.y>3.5f)return "Ball outside held gather";
 if(height>7.5f||distance(b.p,s.own.p)>c.v(BlockRange))return "Outside block reach";
 if(!facing(s.own,b.p))return "Ball behind defender";
 if(!s.grounded)return "Defender not grounded";
 return nullptr;
}
// A body retreat is evidence of space creation, not proof of a shot. One abrupt
// cut gets a short pressure response; a shot cue confirms a tighter closeout.
struct FadeawayRead {
 struct Sample{double at;Vec p,v,look;};std::deque<Sample> history;
 Identity owner=0,defender=0;double until=-1,quietSince=-1;bool cutArmed=true,confirmed=false,spin=false;
 void reset(){*this={};}
 bool update(bool eligible,const Actor* a,const Actor& own,bool shotCue,double t){
  if(!eligible||!a||!a->enemy||!a->goalValid||!std::isfinite(t)||!finite(a->p)||!finite(a->v)||!finite(a->goal)||!finite(own.p)||length(flat(a->v))>45){reset();return false;}
  if(owner!=a->id||defender!=own.id||(!history.empty()&&(t<=history.back().at||t-history.back().at>.12))){reset();owner=a->id;defender=own.id;}
  const Vec axis=unit(flat(a->goal-a->p)),away=unit(flat(a->p-own.p));
  const float back=-dot(flat(a->v),axis),separating=dot(flat(a->v),away);
  confirmed=shotCue;
  // A brief speed dip during a pivot is not the end of a retreat. A real
  // reversal cancels immediately; a sustained stop rearms the next cut.
  if(back<1||separating<-.5f){
   if(quietSince<0)quietSince=t;
   if(back< -2||separating< -2||t-quietSince>=.08||shotCue){until=-1;cutArmed=true;spin=false;}
  }else quietSince=-1;
  if(!history.empty()){
   const auto previous=history.back();const double dt=t-previous.at;
   const Vec delta=flat(a->p-previous.p);
   if(dt<.005||length(delta)>45*dt+.2f){reset();owner=a->id;defender=own.id;history.push_back({t,a->p,a->v,a->facing});return false;}
   const Vec observed=delta*(float)(1/dt);
   while(history.size()>1&&t-history.front().at>.18)history.pop_front();
   const auto baseline=history.front();
   const bool retreat=back>=3.5f&&separating>=2&&-dot(observed,axis)>=3&&dot(observed,away)>=1.5f;
   float priorBack=back;for(const auto& sample:history)priorBack=std::min(priorBack,-dot(flat(sample.v),axis));
   const bool sudden=retreat&&t-baseline.at>=.019&&back-priorBack>=4.5f;
   if(retreat&&((cutArmed&&sudden)||shotCue)){
    // Allow time to cover the stand-off gap, but never renew a motion-only
    // burst indefinitely. Shot evidence can sustain its separate closeout.
    until=t+(shotCue?.12:std::clamp((distance(own.p,a->p)-3.25f)/std::max(4.f,own.speed-std::max(0.f,separating)),.30f,.60f));if(sudden)cutArmed=false;
    spin=finite(a->facing)&&finite(baseline.look)&&length(flat(a->facing))>.5f&&length(flat(baseline.look))>.5f&&dot(unit(flat(a->facing)),unit(flat(baseline.look)))<.25f;
   }
  }
  history.push_back({t,a->p,a->v,a->facing});while(history.size()>16)history.pop_front();
  return t<=until;
 }
};
struct Planner {
 std::map<Identity,unsigned> windupJumped;
 Identity target=0,opportunity=0;double acquired=-10,lastSteal=-10,lastJump=-10,opportunityAt=-10,lastAt=0;Vec smooth{};int side=1;Identity screen=0;double sideUntil=0,raisedSince=-10;Identity raisedOwner=0;std::map<Identity,unsigned> jumped;
 bool settled=false,aimValid=false;Vec stableAim{};float adaptiveGap=-1;MeaningfulMotion motion;ShotGather gather;PostUpRead post;FadeawayRead fade;BallSideRead ballSide;
 void reset(){target=opportunity=0;acquired=opportunityAt=-10;smooth={};screen=0;settled=false;aimValid=false;adaptiveGap=-1;motion={};gather={};post.reset();fade.reset();ballSide.reset();}
 Decision step(const Config&c,const Scene&s,const Intent&i,double t){
  Decision d;bool urgentGuard=false,idleActive=false,brakingForArrival=false,postRetreat=false;double dt=lastAt>0?std::clamp(t-lastAt,.001,.1):.016;lastAt=t;
  if(!i.focused||i.chat||!s.valid||t-s.at>.12||!finite(s.own.p)||s.playingKnown&&!s.playing){reset();d.mode=Waiting;d.reason=i.chat?"Paused while typing":!i.focused?"Focus Roblox to engage":s.playingKnown&&!s.playing?"Waiting for a match":"Waiting for a fresh defensive scene";return d;}
  if(heldBall(s,s.own.id)){reset();d.reason="You have possession";return d;}
  const Actor* handler=nullptr;float nearest=c.v(Acquire);
  for(auto&b:s.balls)if(b.owner){auto a=actor(s,b.owner);if(a&&a->enemy&&distance(s.own.p,a->p)<nearest){nearest=distance(s.own.p,a->p);handler=a;}}
  const Actor* mark=actor(s,i.selected);if(mark&&!mark->enemy)mark=nullptr;
  bool releasedShot=false;
  if(!handler&&c.on(Guard)&&i.activation)for(auto&b:s.balls)if(!b.owner&&b.shot&&t>=b.releasedAt&&t-b.releasedAt<=.35){auto a=actor(s,b.previousOwner);if(a&&a->enemy&&a->goalValid&&distance(a->p,s.own.p)<nearest){nearest=distance(a->p,s.own.p);handler=a;releasedShot=true;}}
  bool gathered=gather.update(handler,handler?heldBall(s,handler->id):nullptr,t);
  bool raised=handler&&(handler->shooting||handler->shotMeterActive||gathered);
  bool earlyMeter=handler&&handler->shotMeterRising&&!handler->shotReleased;
  bool denial=i.activation&&c.on(Offball)&&mark&&!heldBall(s,mark->id);
  const Actor* wanted=denial?mark:c.on(Lock)&&mark?mark:handler;
  bool guard=i.activation&&((denial&&c.on(Offball))||(!denial&&c.on(Guard)));
  const bool fading=fade.update(guard&&!denial&&handler&&wanted==handler&&!releasedShot,handler,s.own,raised,t);
  bool closeout=guard&&!denial&&handler&&wanted==handler&&(raised||earlyMeter||releasedShot||fading);
  float ballBias=ballSide.update(guard&&!denial&&wanted==handler?handler:nullptr,handler?heldBall(s,handler->id):nullptr,t);
  bool guardOwnsMovement=c.on(Guard)&&!i.catchHeld; // Auto guard owns automatic positioning, even across possession gaps.
  if(!guard){settled=false;smooth={};motion={};gather={};}
  if(wanted&&wanted->id!=target){target=wanted->id;acquired=t;smooth={};settled=false;aimValid=false;adaptiveGap=-1;}else if(!wanted){target=0;smooth={};settled=false;aimValid=false;adaptiveGap=-1;}
  d.target=wanted?wanted->id:0;
  const bool postGuard=post.update(guard&&!denial&&!closeout&&wanted==handler&&c.on(AntiPost),handler,s.own,t);
  auto jump=[&](const Ball&b,Mode mode){float height=b.p.y-s.own.p.y;if(!s.grounded||height<(mode==Blocking&&b.owner?1.8f:3.f)||height>(mode==Blocking?8.f:c.v(ReboundHeight)+1)||t-lastJump<c.seconds(JumpCooldown)||(i.manual&&mode!=Blocking))return false;
   bool once=mode!=Blocking||c.on(BlockOnce);if(once&&jumped[b.id]==b.flight+1)return false;d.jump=true;lastJump=t;jumped[b.id]=b.flight+1;d.mode=mode;d.ball=b.id;return true;};
  auto steal=[&](const Ball&b){if(!c.on(Steal)&&!i.catchHeld)return false;if(t-lastSteal<c.seconds(StealCooldown))return false;d.steal=true;lastSteal=t;d.ball=b.id;return true;};
  // Lobs reserve their flight before block/intercept/rebound decisions. No jump at launch.
  const Ball* lob=nullptr;Meeting lobMeet;
  if(c.on(Oop)&&!guardOwnsMovement)for(auto&b:s.balls)if(!b.owner&&b.lob&&b.velocityValid&&distance(b.p,s.own.p)<=c.v(OopRange)&&t-b.releasedAt<3){auto owner=actor(s,b.previousOwner);if(!owner||!owner->enemy)continue;auto m=meet(s,b,3.0f,8,1.1f);if(m.valid&&(!lob||m.time<lobMeet.time)){lob=&b;lobMeet=m;}}
  if(lob){d.mode=LobTracking;d.reason="Lob reserved: waiting for backboard deflection";d.ball=lob->id;d.marker=lobMeet.p;d.showMarker=c.on(OopGuide);d.destination=lobMeet.p;d.destination.y=s.own.p.y;d.moving=c.on(OopMove);d.face=lob->p;
   if(lob->boardHit&&t-lob->boardAt<.2&&distance(s.own.p,lob->p)<=5.5f&&facing(s.own,lob->p)){if(jump(*lob,LobTracking))d.reason="Backboard deflection: jump requested";}
   float h=lob->p.y-s.own.p.y;if(h>-.5f&&h<4.5f&&distance(lob->p,s.own.p)<=3.5f&&(c.on(Steal)||i.catchHeld))steal(*lob);
  }else{
   // Interception is opt-in. Manual R can request a catch while auto-steal remains off.
   const Ball* pass=nullptr;Meeting passMeet;
   if(!guardOwnsMovement&&c.on(Intercept)&&(i.catchHeld||c.on(InterceptAuto)&&c.on(Steal)))for(auto&b:s.balls)if(!b.owner&&!b.lob&&b.velocityValid&&(c.on(InterceptShots)||!b.shot)&&distance(b.p,s.own.p)<=18){auto a=actor(s,b.previousOwner);if(!a||!a->enemy)continue;auto m=meet(s,b,c.v(InterceptReach),c.on(InterceptJump)?8.f:4.f,.65f,0,-.5f);if(m.valid&&(!pass||m.time<passMeet.time)){pass=&b;passMeet=m;}}
   if(pass){d.mode=Intercepting;d.reason="Moving to reachable pass";d.ball=pass->id;d.destination=passMeet.p;d.destination.y=s.own.p.y;d.moving=c.on(InterceptMove);d.marker=passMeet.p;d.showMarker=true;d.face=pass->p;float h=pass->p.y-s.own.p.y;
    if(distance(pass->p,s.own.p)<=c.v(InterceptReach)&&h>-.5f&&h<4.5f&&facing(s.own,pass->p))steal(*pass);
    if(c.on(InterceptJump)&&h>=3&&h<=8&&distance(pass->p,s.own.p)<=3.5f)jump(*pass,Intercepting);
   }else if(c.on(Rebound)&&!guardOwnsMovement){
    const Ball* ball=nullptr;Meeting point;
    for(auto&b:s.balls)if(!b.owner&&!b.lob&&b.velocityValid&&b.v.y<-.5&&distance(b.p,s.own.p)<=c.v(ReboundRange)){auto releasedBy=actor(s,b.previousOwner);if(!releasedBy||!releasedBy->enemy)continue;auto m=meet(s,b,c.v(ReboundReach),c.v(ReboundHeight),1.1f,(float)c.seconds(ReboundLead));if(m.valid&&(!ball||m.time<point.time)){ball=&b;point=m;}}
    if(ball){d.mode=Rebounding;d.reason="Tracking descending-ball meeting point";d.ball=ball->id;d.marker=point.p;d.showMarker=true;d.destination=point.p;d.destination.y=s.own.p.y;d.moving=c.on(ReboundChase);d.face=ball->p;
     if(distance(s.own.p,ball->p)<=c.v(ReboundReach)&&ball->p.y-s.own.p.y<=c.v(ReboundHeight))jump(*ball,Rebounding);
    }else if(c.on(ReboundChase)&&c.on(ReboundEarly))for(auto&b:s.balls)if(!b.owner&&b.shot&&b.v.y>0){auto owner=actor(s,b.previousOwner);if(owner&&owner->enemy&&owner->goalValid&&distance(owner->goal,s.own.p)<30){d.mode=Rebounding;d.reason="Early rim positioning";d.destination=owner->goal+unit(flat(s.own.p-owner->goal))*c.v(ReboundSpot);d.destination.y=s.own.p.y;d.moving=true;break;}}
   }
   // Some clients keep ownership until the ball is already above block reach.
   // Start during the upward gather, allowing time for our jump animation.
   // Require current charge advancement and a ball still near the shooter's body;
   // delayed ownership must not turn an already-flying ball into a windup jump.
   if(c.on(Block)&&d.mode!=Intercepting)for(auto& b:s.balls){auto shooter=actor(s,b.owner);
    if(!shooter||!shooter->enemy)continue;
    if(auto why=blockWindupGate(c,s,*shooter,b)){d.blockReason=why;continue;}
    if(windupJumped[b.id]==b.flight+1){d.blockReason="Already attempted this possession";continue;}
    if(t-lastJump<c.seconds(JumpCooldown)){d.blockReason="Jump cooldown";continue;}
    if(jump(b,Blocking)){windupJumped[b.id]=b.flight+1;d.blockReason="Early windup jump requested";d.reason="Rising shot gather: early block requested";d.face=b.p;break;}
   }
   // No release fallback: if the windup window is missed, do not jump late.
  }
  bool reserved=d.mode==LobTracking||d.mode==Intercepting||d.mode==Rebounding;
  if(!reserved&&guard&&wanted){d.face=handler?handler->p:wanted->p;
   if(denial&&!handler){d.mode=Waiting;d.reason="Matchup selected; waiting for ball handler";}
   else if(!denial&&!wanted->goalValid){d.mode=Waiting;d.reason="Ball handler found; goal position unavailable";}
   else if(!closeout&&t-acquired<(denial?c.seconds(OffDelay):c.seconds(Reaction))){d.mode=Waiting;d.reason="Acquiring matchup";}
   else{Vec forward=unit(flat((denial?handler->p:wanted->goal)-wanted->p));float gap=denial?c.v(OffGap):c.v(Gap);float speed=length(flat(wanted->v));bool rim=!denial&&distance(wanted->p,wanted->goal)<c.v(RimRange);
    // Reference: src/engine/06_guard_input_movement.luau.
    // The destination belongs to the handler-to-basket line, not our own position.
    motion.observe(*wanted,closeout||denial||postGuard);
    Vec velocity=shooting::cap(flat(motion.velocity),30);
    if(length(velocity)<.8f)velocity={}; // avoid leading tiny idle velocity noise
    d.predictionAnchor=motion.position;d.predictionVelocity=velocity;d.predictionValid=true;
    Vec predicted=motion.position+velocity*(denial?.07f:.08f);
    Vec facingGoal=denial?forward:unit(flat(wanted->goal-predicted));
    forward=facingGoal;
    float drive=std::max(0.f,dot(velocity,forward));
    float retreatSpeed=-dot(velocity,forward);
    float depth=dot(flat(s.own.p-wanted->p),forward);
    bool trackingRetreat=!denial&&c.on(Adaptive)&&retreatSpeed>2.5f;
    // Preserve the requested stand-off while the handler retreats/dribbles.
    // Cap only at the basket so the target point cannot continue past the rim.
    bool rimContain=rim&&!raised;
    if(!denial)gap=std::min(gap,std::max(2.f,distance(predicted,wanted->goal)-1.5f));
    if(postGuard){gap=4.25f+std::clamp(drive*.12f,0.f,1.f);predicted=wanted->p+velocity*.08f;forward=unit(flat(wanted->goal-wanted->p));}
    if(closeout){gap=fading&&!raised?3.25f:2.f;predicted=wanted->p+(fading?shooting::cap(flat(wanted->v)*.10f,1.25f):Vec{});forward=unit(flat(wanted->goal-predicted));}
    Vec aim=predicted+forward*gap;
    // On-ball coverage is anchored to the current handler-to-hoop line.
    // Keep useful depth prediction, but do not stand beside a predicted body
    // position or cut across the handler's facing direction.
    if(!denial){
     forward=unit(flat(wanted->goal-wanted->p));
     float depthLimit=std::max(0.f,distance(wanted->p,wanted->goal)-.75f);
     // Cut off a drive by gaining basket-side depth, not stepping beside it.
     if(c.on(Cutoff)&&!closeout&&!postGuard)aim=aim+forward*std::clamp(drive*.045f,0.f,.65f);
     aim=wanted->p+forward*std::clamp(dot(flat(aim-wanted->p),forward),std::min(1.f,depthLimit),depthLimit);
    }
    urgentGuard=rimContain||trackingRetreat||closeout||postGuard;
    d.basketAxis=forward;d.retreat=!denial&&drive>2&&dot(flat(aim-s.own.p),forward)>.1f;
    if(denial&&c.on(Humanize)&&c.on(Idle)&&!urgentGuard&&speed<.6&&distance(aim,s.own.p)<2&&!rim){Vec lateral{-forward.z,0,forward.x};double phase=fmod(t+double(wanted->id%100)*.13,5.7);if(phase<1.15){aim=aim+lateral*(float)(.55*sin(phase*3.14159/1.15));idleActive=true;}}
    // A spatial deadband absorbs tiny observation changes without a timed delay.
    float aimTolerance=speed>2?.08f:.18f;
    if(!aimValid||distance(aim,stableAim)>aimTolerance){stableAim=aim;aimValid=true;}
    aim=stableAim;
    // Recheck the held anchor without defeating the small-motion deadband.
    if(!denial){float limit=std::max(0.f,distance(wanted->p,wanted->goal)-.75f),depth=dot(flat(aim-wanted->p),forward);Vec onLine=wanted->p+forward*std::clamp(depth,std::min(1.f,limit),limit);if(distance(aim,onLine)>aimTolerance||depth<0||depth>limit){aim=onLine;stableAim=aim;}}
    // Add after basket-line stabilization; do not feed the bias back into the
    // central anchor. Near the rim, scale down to preserve basket-side coverage.
    if(!denial){float depth=std::max(0.f,dot(flat(aim-wanted->p),forward));ballBias=std::clamp(ballBias,-depth*.25f,depth*.25f);aim=aim+Vec{-forward.z,0,forward.x}*ballBias;}
    d.destination=aim;d.destination.y=s.own.p.y;d.moving=true;d.stance=true;if(!d.jump)d.mode=denial?Denying:Guarding;if(!d.jump)d.reason=denial?"Denying the selected passing lane":rim?"Containing near the rim":"Maintaining basket-side coverage";
    if(d.retreat&&!d.jump)d.reason="Retreating basket-side while matching the drive";
    if(closeout){if(!d.jump)d.reason=fading?(raised?(fade.spin?"Closing spin fadeaway":"Closing fadeaway retreat"):"Pressuring sudden backward cut"):handler->shotMeterActive?"Closing active shot meter":earlyMeter?"Closing rising shot meter":"Closing shot gather / shot release";d.retreat=false;}
    if(postGuard&&!d.jump)d.reason="Anti-post: holding a close non-contact gap";
    d.coverage=std::clamp(dot(unit(flat(s.own.p-wanted->p)),forward),0.f,1.f)*std::clamp(1-distance(aim,s.own.p)/8,0.f,1.f);
    // Rim assistance must not bypass the same pre-release deadline or repeat
    // a windup attempt merely because the shooter is still moving upward.
    if(!d.jump&&!denial&&rim&&c.on(RimBlock)){auto b=heldBall(s,wanted->id);if(b&&wanted->v.y>2.0&&distance(wanted->p,wanted->goal)<7&&!blockWindupGate(c,s,*wanted,*b)&&windupJumped[b->id]!=b->flight+1&&jump(*b,Blocking)){windupJumped[b->id]=b->flight+1;d.blockReason="Early rim windup jump requested";d.reason="High ball at rim: dunk block requested";}}
   }
  }
  if(c.on(Steal)&&handler&&!reserved&&!d.jump){auto b=heldBall(s,handler->id);bool valid=b&&distance(b->p,s.own.p)<=c.v(StealReach)&&fabs(b->p.y-s.own.p.y)<3.5f&&facing(s.own,b->p,.35f)&&fabs(handler->v.y)<3;
   if(valid){if(opportunity!=handler->id){opportunity=handler->id;opportunityAt=t;}if(t-opportunityAt>=c.seconds(StealDelay)&&steal(*b))opportunityAt=t;}else{opportunity=0;opportunityAt=-10;}}
  if(c.on(Recovery)&&t-lastSteal<c.seconds(RecoveryWindow)&&!d.showMarker){float best=c.v(RecoveryRange);for(auto&b:s.balls)if(!b.owner&&distance(b.p,s.own.p)<best){best=distance(b.p,s.own.p);d.marker=b.p;d.showMarker=true;}}
  if(d.moving&&c.on(Screens)){Vec route=flat(d.destination-s.own.p);float routeLength=length(route);Vec dir=unit(route);for(auto&a:s.actors){if(a.id==d.target)continue;Vec rel=flat(a.p-s.own.p);float along=dot(rel,dir);if(along<.2||along>std::min(routeLength,9.f))continue;Vec lateral=rel-dir*along;if(length(lateral)>3.2)continue;
    if(screen!=a.id||t>sideUntil){screen=a.id;side=dot(lateral,Vec{-dir.z,0,dir.x})>0?-1:1;sideUntil=t+.7;}d.destination=a.p+Vec{-dir.z,0,dir.x}*(side*3.6f)+dir*1.2f;d.destination.y=s.own.p.y;d.obstacle=a.id;d.reason="Routing around player obstruction";break;}}
  // Check crowding even when already settled. Body approach, not ball/dribble
  // motion, expands the buffer. This also overrides a screen route into contact.
  if(postGuard&&d.stance){
   Vec away=flat(s.own.p-wanted->p);float separation=length(away);
   Vec radial=separation>.01f?unit(away):d.basketAxis;
   const float closing=std::max(0.f,dot(flat(wanted->v-s.own.v),radial));
   if(separation<PostBuffer||separation-closing*.12f<PostBuffer){
    postRetreat=true;settled=false;aimValid=false;d.moving=true;d.retreat=true;d.obstacle=0;
    d.destination=wanted->p+radial*(PostBuffer+.85f+std::clamp(closing*.12f,0.f,1.f));d.destination.y=s.own.p.y;
    d.reason="Anti-post: giving space to the backdown";
   }
  }
  if(d.moving){float gap=distance(d.destination,s.own.p);
   if(!reserved){bool movingTarget=wanted&&length(flat(wanted->v))>2;float enter=closeout?.25f:std::abs(ballBias)>.1f?.30f:movingTarget?.40f:.65f,leave=closeout?.45f:std::abs(ballBias)>.1f?.50f:movingTarget?.75f:1.10f;if(gap<=enter)settled=true;else if(gap>=leave)settled=false;}else settled=false;
   bool arriving=!postRetreat&&!reserved&&wanted&&arrivalBrake(d.destination-s.own.p,s.own.v-wanted->v);
   brakingForArrival=arriving;
   // Do not restart a settled correction from residual velocity alone.
   if(arriving&&length(flat(wanted->v))<2&&gap<(closeout?.25f:1.10f))settled=true;
   if(gap<(closeout?.25f:.35f)||settled||arriving)d.moving=false;else{
    Vec difference=flat(d.destination-s.own.p),desired=unit(difference);
    if(d.stance&&length(d.basketAxis)>.5f&&!d.obstacle){Vec lateral=difference-d.basketAxis*dot(difference,d.basketAxis);float correction=.6f*std::clamp((gap-1.5f)/2.f,0.f,1.f);desired=unit(difference+lateral*correction);}
    if(postGuard&&d.stance)desired=postRoute(flat(s.own.p-wanted->p),desired);
    bool contact=postRetreat;
    if(d.stance&&!closeout&&wanted&&c.on(AntiPush)){
     Vec away=flat(s.own.p-wanted->p);float separation=length(away);
     if(separation>.1f&&separation<2.f){contact=true;desired=unit(away);}
    }
    if(c.on(Humanize)&&!reserved&&!contact&&length(smooth)>.01f){
     float level=std::clamp(c.v(Softness)/100.f,0.f,1.f);
     // Never continue away from a new correction after the handler cuts.
     if(dot(smooth,desired)<=0)smooth=desired;
     else smooth=unit(smooth+(desired-smooth)*(float)(1-exp(-dt*(urgentGuard?28.f:24.f-8.f*level))));
    }else smooth=desired;
    if(postGuard&&d.stance)smooth=postRoute(flat(s.own.p-wanted->p),smooth);
    d.direction=smooth;
   }}
  if(i.manual&&reserved){d.moving=false;d.reason="Manual movement overrides pursuit";}
  // Close coverage uses ordinary steps; sprint is reserved for catching up.
  d.sprint=d.moving&&c.on(Sprint)&&(reserved||distance(d.destination,s.own.p)>3.5f)&&(!postGuard||!d.stance||distance(s.own.p,wanted->p)>8);if(d.jump)d.stance=false;
  if(d.mode==Off){if(c.on(Offball)&&!mark)d.reason="Select an opponent for off-ball guard";else if((c.on(Guard)||c.on(Offball))&&!i.activation)d.reason="Hold your activation key to guard";else if(guard&&!wanted)d.reason="Waiting for a nearby ball handler";else if(c.on(Steal)||c.on(Block)||c.on(Oop)||c.on(Rebound)||c.on(Intercept))d.reason="Watching possession and ball flight";}
  d.explanation=d.reason;
  if(wanted){
   Vec toward=unit(flat(wanted->goal-wanted->p));float forwardSpeed=dot(flat(wanted->v),toward);
   float sideSpeed=length(flat(wanted->v)-toward*forwardSpeed);
   bool continuing=length(flat(wanted->v))>2&&length(flat(motion.velocity))>2&&dot(unit(flat(wanted->v)),unit(flat(motion.velocity)))>.8f;
   d.observation=fading&&d.stance?(raised?(fade.spin?"Spin and retreat with shot cue":"Retreat with shot cue"):"Sudden backward cut"):postGuard&&d.stance?"Post-up stance (estimated)":releasedShot?"Shot released":wanted->shotMeterActive?"Shot meter advancing":wanted->shooting?"Shooting cue":gathered?"Possible shot gather":
    length(flat(wanted->v))<.8f?"Stationary / stopped":!motion.accepted&&!continuing?"Small motion filtered":
    forwardSpeed>3?"Driving toward basket":forwardSpeed< -2.5f?"Retreating from basket":sideSpeed>2?"Moving across the lane":"Repositioning";
  }
  if(d.stance){
   if(postGuard){d.response=postRetreat?"Giving the backdown space":d.moving?"Matching the post-up":"Holding the post buffer";d.explanation="Sustained back-to-basket stance; close coverage with a body-contact buffer.";}
   else if(!d.moving){d.response=brakingForArrival?"Settling into position":"Holding the lane";d.explanation=brakingForArrival?"Releasing movement to avoid overshooting.":"Already within the positioning tolerance.";}
   else if(closeout){d.response=fading?"Closing retreat space":"Closing the shooter";d.explanation=fading?(raised?"Backward body movement plus a shot cue; close to the predicted retreat point.":"Sharp backward change; brief pressure without assuming a shot or requesting a jump."):gathered&&!wanted->shooting&&!wanted->shotMeterActive&&!releasedShot?"High, close ball held briefly; gather estimate.":"Shot cue detected; reducing the gap to 2 studs.";}
   else if(d.obstacle){d.response="Avoiding an obstruction";d.explanation="A player intersects the planned route.";}
   else if(d.retreat){d.response="Retreating to contain";d.explanation="Recovering space between handler and basket.";}
   else{d.response="Matching the basket line";d.explanation="Correcting position toward the stand-off point.";}
  }else if(d.mode==Rebounding)d.response="Pursuing rebound";
  else if(d.mode==LobTracking)d.response="Tracking lob";
  else if(d.mode==Intercepting)d.response="Intercepting pass";
  if(d.jump){d.response="Jump requested";d.explanation="Release/height/reach checks allowed a jump.";}
  if(!d.moving)smooth={};if(jumped.size()>80)jumped.clear();if(windupJumped.size()>80)windupJumped.clear();return d;
 }
};
}








