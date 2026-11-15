#pragma once
#include "ff3_ball_flights.hpp"
namespace ff3qb {
struct ThrowActor {
 std::uint64_t id=0,ball=0,thrower=0;Defender reach;bool valid=false,sideKnown=false,opponent=false;std::string name;
};
struct ThrowIntent {
 std::uint64_t click=0,model=0,own=0,player=0,target=0,heldBall=0;
 double at=0,power=0,angle=0,air=0,distance=0,catchHeight=NAN;std::string name,mode;
 bool predictionKnown=false,predictedIntable=false;
 double predictedRiskAt=NAN;
};
struct ThrowView {
 enum Phase{Hidden,Pending,Flight,Done};Phase phase=Hidden;
 std::uint64_t ball=0;
 double eta=NAN,toSpot=NAN,nearD=NAN,margin=NAN,air=0,power=0,angle=0,distance=0;
 bool coverageKnown=false,intable=false;double riskAt=NAN;Vec spot;std::string name,mode,result,resultBy,detail;
};
inline void flightMetrics(ThrowView& view,const BallFlight& ball,double ground,const std::vector<ThrowActor>& actors,std::uint64_t target,bool complete,double catchHeight=NAN){
 view.eta=view.toSpot=view.nearD=view.margin=NAN;view.coverageKnown=false;view.intable=false;
 if(!ball.model.valid){view.eta=NAN;view.detail="Reading your ball flight";return;}
 auto observed=ball;observed.at=ball.model.observedTime(ball.position);
 auto prediction=forecast(observed,ground);if(!prediction.valid)return;
 auto start=ball.model.pointAt(observed.at),v=ball.model.velocity+Vec{0,-ball.gravity*(observed.at-ball.model.at),0};
 const ThrowActor* wr=nullptr;std::vector<Defender> opponents;
 for(const auto& actor:actors){if(actor.id==target&&actor.valid)wr=&actor;if(!actor.valid)complete=false;else if(actor.id!=target&&(actor.opponent||!actor.sideKnown))opponents.push_back(actor.reach);}
 double end=prediction.remaining;
 if(wr){
  // Evaluate the actual remaining arc in 3D, stopping at the first predicted
  // receiver catch opportunity. Nearby/contested WR status is not the risk flag.
  const int steps=std::clamp(int(std::ceil(end/.02)),32,400);
  for(int i=0;i<=steps;i++){double t=end*i/steps;Vec point=start+v*t+Vec{0,-.5*ball.gravity*t*t,0};if(receiverGap(wr->reach,point,t)<=-.25){end=t;break;}}
 }
 view.eta=end;view.spot=start+v*end+Vec{0,-.5*ball.gravity*end*end,0};
 if(wr)view.toSpot=length(wr->reach.position-view.spot);
 double nearest=INFINITY;for(const auto& d:opponents)nearest=std::min(nearest,length(d.position-view.spot));if(std::isfinite(nearest))view.nearD=nearest;
 if(complete){Solution remaining{true,start,v,view.spot,end,0};view.margin=coverageMargin(remaining,ball.gravity,opponents,0);view.coverageKnown=true;view.intable=view.margin<=0;}
 view.detail=wr?"Your pass · predicted catch spot":"Your pass · receiver unavailable";
}
struct ThrowTracker {
 ThrowIntent intent;ThrowView view;std::uint64_t ball=0;double lastSeen=0,doneAt=0,landedAt=0,missingAt=0;Vec lastPosition;
 std::vector<std::uint64_t> priorBalls;std::vector<std::pair<std::uint64_t,std::uint64_t>> priorHeld;
 void clear(){*this={};}
 void begin(const ThrowIntent& next,const std::vector<BallFlight>& existing,const std::vector<ThrowActor>& actors){
  clear();intent=next;view.phase=ThrowView::Pending;view.name=next.name;view.mode=next.mode;view.power=next.power;view.angle=next.angle;view.air=next.air;view.distance=next.distance;view.coverageKnown=next.predictionKnown;view.intable=next.predictedIntable;view.riskAt=next.predictedRiskAt;
  for(const auto& f:existing)priorBalls.push_back(f.id);for(const auto& a:actors)priorHeld.push_back({a.id,a.ball});
 }
 void finish(const std::string& result,double time,const std::string& by={}){view.phase=ThrowView::Done;view.result=result;view.resultBy=by;view.air=std::max(0.,time-intent.at);doneAt=time;}
 void update(double time,std::uint64_t model,std::uint64_t own,const std::vector<BallFlight>& balls,const std::vector<ThrowActor>& actors,double ground,bool complete){
  if(view.phase==ThrowView::Hidden)return;
  if(model!=intent.model||own!=intent.own||time<intent.at){clear();return;}
  if(view.phase==ThrowView::Done){if(time-doneAt>2.6)clear();return;}
  if(landedAt&&time-landedAt>.15){finish("Incomplete",time);return;}
  if(!ball){
   std::uint64_t found=0;bool ambiguous=false;
   for(const auto& f:balls)if(f.airborne&&f.born>=intent.at-.1&&std::find(priorBalls.begin(),priorBalls.end(),f.id)==priorBalls.end()&&(f.id==intent.heldBall||(f.thrower&&(f.thrower==intent.player||f.thrower==intent.own)))){if(found)ambiguous=true;found=f.id;}
   if(found&&!ambiguous){ball=view.ball=found;view.phase=ThrowView::Flight;lastSeen=time;}
   else{if(time-intent.at>1.2)clear();return;}
  }
  const BallFlight* current=nullptr;for(const auto& f:balls)if(f.id==ball){current=&f;break;}
  const ThrowActor* holder=nullptr;unsigned holders=0;
  for(const auto& actor:actors)if(actor.valid&&actor.ball){
   bool newlyHeld=std::none_of(priorHeld.begin(),priorHeld.end(),[&](const auto& prior){return prior.first==actor.id&&prior.second==actor.ball;});
   bool ownedClone=!current&&newlyHeld&&actor.thrower&&(actor.thrower==intent.player||actor.thrower==intent.own)&&time-lastSeen<.6&&length(actor.reach.position-lastPosition)<20;
   if(actor.ball==ball||ownedClone){holder=&actor;++holders;}
  }
  if(holders==1){if(holder->sideKnown)finish(holder->opponent?"Intercepted":"Caught",time,holder->name);else finish("Caught - team unknown",time,holder->name);return;}
  if(current){
   lastSeen=time;lastPosition=current->position;missingAt=0;
   if(current->landed){if(!landedAt)landedAt=time;}else if(!current->ended)flightMetrics(view,*current,ground,actors,intent.target,complete,intent.catchHeight);
   if(intent.predictionKnown){view.coverageKnown=true;view.intable=intent.predictedIntable;view.detail="Your pass · pre-throw risk forecast";}
  }else if(!missingAt)missingAt=time;
  if((missingAt&&time-missingAt>.6)||time-intent.at>12){finish("Result unavailable",time);return;}
 }
};
}
