#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>
#include "shot_feedback.hpp"

// Local bounded controller. Meter observations are not game timing grades.
namespace timingReview {
enum class Grade { Early, Perfect, Late };
struct Tuning { int adjustmentMs=0,guardMs=0; unsigned generation=0; bool eligible=false; int windowShiftMs=0; };
struct Shot {
 unsigned id=0; double at=0,fill=0,rate=0; int ping=0;
 bool automatic=false,ceiling=false; Tuning tuning;
 double minimum=77,maximum=81;
 double laterRoomMs=12;
};
struct View {
 unsigned shots=0,feedbackCount=0,perfect=0,overshoots=0,pendingId=0;
 unsigned gameFeedbackCount=0,gamePerfect=0,gameShotId=0;
 double resultMeter=0,resultContest=0,resultSent=0;
 std::string gameGrade,resultPhase;
 int adjustmentMs=0,guardMs=0,ping=0,jitter=0; bool stable=false;
 int windowShiftMs=0;
 std::string reason="Collecting ping samples",last="No reviewed shots";
};
class Reviewer {
 struct Ping { double at; int value; };
 std::deque<Ping> pings;
 std::deque<Shot> recent;
 View state;
 Shot pending;
 unsigned generation=1,lastId=0,lastGameId=0;
 std::uintptr_t owner=0;
 double referenceRate=0; int baseline=0,referencePing=0;
 int streak=0,lastDirection=0,overshootStreak=0;
 int calibrationDirection=0,calibrationStepMs=4;
 bool streakFromGame=false;
 bool context=false,consumed=true,streakCeiling=false;
 void clearLearning(const char* why){
  state.adjustmentMs=state.guardMs=0;streak=lastDirection=overshootStreak=0;
  state.windowShiftMs=0;calibrationDirection=0;calibrationStepMs=4;streakFromGame=false;
  consumed=true;++generation;state.reason=why;
 }
 bool networkReady(double t)const{
  return std::isfinite(t)&&pings.size()>=5&&t>=pings.back().at&&t-pings.back().at<=2&&state.jitter<=15;
 }
public:
 bool usable(const Tuning& tuning,double t)const{return tuning.eligible&&tuning.generation==generation&&networkReady(t);}
 bool baselineIs(int value)const{return context&&baseline==value;}
 void reset(){*this=Reviewer{};}
 void ping(double t,int value){
  if(!std::isfinite(t)||value<1||value>1000)return;
  if(!pings.empty()&&t<=pings.back().at)return;
  if(!pings.empty()&&(t-pings.back().at>2||std::abs(value-state.ping)>20)){
   pings.clear();clearLearning("Connection changed; collecting a fresh baseline");
  }
  pings.push_back({t,value});while(pings.size()>12)pings.pop_front();
  std::vector<int> values;for(const auto& p:pings)values.push_back(p.value);
  std::sort(values.begin(),values.end());state.ping=values[values.size()/2];
  state.jitter=values.back()-values.front();
  if(state.jitter>15){streak=lastDirection=overshootStreak=0;consumed=true;}
 }
 Tuning prepare(double t,double rate,int base,std::uintptr_t session){
  if(!std::isfinite(rate)||rate<.1||rate>20||!session)return {};
  if(!context||owner!=session||baseline!=base||std::abs(state.ping-referencePing)>25||std::abs(rate-referenceRate)>std::max(.3,referenceRate*.25)){
   clearLearning(context?"Shot conditions changed; learning reset":"Watching releases and waiting for game feedback");
   owner=session;baseline=base;referenceRate=rate;referencePing=state.ping;context=true;
  }
  const bool ok=networkReady(t);
  return {ok?state.adjustmentMs:0,ok?state.guardMs:0,generation,ok,ok?state.windowShiftMs:0};
 }
 void observe(const Shot& shot){
  if(!shot.id||shot.id<=lastId||!std::isfinite(shot.at)||!std::isfinite(shot.fill)||!std::isfinite(shot.rate)||
     shot.fill<0||shot.fill>1.05||shot.rate<.1||shot.rate>20||!std::isfinite(shot.minimum)||!std::isfinite(shot.maximum)||shot.minimum<0||shot.maximum>100||shot.minimum>shot.maximum)return;
  // A shot with no accepted feedback interrupts consecutive evidence.
  if(!consumed){streak=lastDirection=0;}
  lastId=shot.id;++state.shots;consumed=true;
  recent.push_back(shot);while(recent.size()>60)recent.pop_front();
  const bool over=shot.fill>shot.maximum*.01+.005;if(over)++state.overshoots;
  state.last="Shot "+std::to_string(shot.id)+" sent at "+std::to_string(int(std::lround(shot.fill*100)))+"%";
  if(!shot.automatic||!shot.tuning.eligible||shot.tuning.generation!=generation||!networkReady(shot.at)||std::abs(shot.rate-referenceRate)>std::max(.3,referenceRate*.25)){
   state.reason="Review only: manual release or unstable/missing ping";overshootStreak=streak=lastDirection=0;return;
  }
  pending=shot;consumed=false;
  if(over){
   if(++overshootStreak>=3){
    const int before=state.guardMs;state.guardMs=std::min(20,state.guardMs+2);overshootStreak=0;
    state.reason=before==state.guardMs?"Headroom limit reached; review stalled meter updates":"Three releases exceeded their shot window; added 2 ms of headroom";
   }else state.reason="Release exceeded its shot window; waiting for repeated evidence";
  }else{overshootStreak=0;state.reason="Release reviewed; game result unknown";}
 }
 bool feedback(unsigned id,Grade grade,double t,bool fromGame=false){
  if((grade!=Grade::Early&&grade!=Grade::Perfect&&grade!=Grade::Late)||consumed||!id||pending.id!=id||
     pending.tuning.generation!=generation||!networkReady(t)||t<pending.at||t-pending.at>20)return false;
  consumed=true;if(!fromGame)++state.feedbackCount;
  if(grade==Grade::Perfect){if(!fromGame)++state.perfect;streak=lastDirection=0;state.reason=fromGame?"Game confirmed Perfect; retained current timing":"Your Perfect feedback retained the current timing";return true;}
  const int direction=grade==Grade::Late?1:-1;
  if(lastDirection!=direction||streakFromGame!=fromGame||(!fromGame&&streakCeiling!=pending.ceiling)){lastDirection=direction;streakCeiling=pending.ceiling;streakFromGame=fromGame;streak=0;}
  if(++streak<3){state.reason=std::string(fromGame?"Game feedback":"Your feedback")+" recorded ("+std::to_string(streak)+"/3 in a row)";return true;}
  streak=0;
  if(fromGame){
   // Move BOTH the send window and the predictive deadline. Changing only
   // trim cannot correct an early release forced by the window ceiling.
   // The same correction applies across automatic release causes.
   if(calibrationDirection&&calibrationDirection!=direction)calibrationStepMs=std::max(1,calibrationStepMs/2);
   calibrationDirection=direction;
   const int before=state.windowShiftMs;
   state.windowShiftMs=std::clamp(before-direction*calibrationStepMs,-40,40);
   state.reason=before==state.windowShiftMs?"Automatic calibration limit reached; timing retained":
    "Three consistent game results: automatic timing moved "+std::to_string(std::abs(state.windowShiftMs-before))+" ms "+(direction<0?"later":"earlier");
   return true;
  }
  if(pending.ceiling&&direction<0){
   const int before=state.guardMs;
   const int lower=-int(std::floor(std::clamp(std::isfinite(pending.laterRoomMs)?pending.laterRoomMs:0.,0.,12.)));
   state.guardMs=std::max(lower,state.guardMs-2);
   state.reason=before==state.guardMs?"Later correction limited by measured polling delay and the shot ceiling":state.guardMs>before?"Polling delay increased; reduced the later correction":"Three early-side results: move ceiling release 2 ms later within polling limits";return true;
  }
  if(pending.ceiling){const int before=state.guardMs;state.guardMs=std::min(20,state.guardMs+2);state.reason=before==state.guardMs?"Headroom limit reached; timing retained":"Three Late results: added 2 ms of ceiling headroom";}
  else {const int before=state.adjustmentMs;state.adjustmentMs=std::clamp(state.adjustmentMs+direction*2,-12,12);state.reason=before==state.adjustmentMs?"Adjustment limit reached; timing retained":direction>0?"Three Late results: send 2 ms earlier":"Three Early results: send 2 ms later";}
  return true;
 }
 bool gameResult(const shotFeedback::Result& result){
  if(result.grade!=shotFeedback::Grade::Early&&result.grade!=shotFeedback::Grade::Perfect&&result.grade!=shotFeedback::Grade::Late&&result.grade!=shotFeedback::Grade::Good)return false;
  if(!result.id||result.id<=lastGameId||recent.empty()||recent.back().id!=result.id||
     !std::isfinite(result.at)||result.at<recent.back().at||result.at-recent.back().at>3||
     !std::isfinite(result.contest)||result.contest<0||result.contest>100||!std::isfinite(result.meter)||result.meter<0||result.meter>1.05||result.grade==shotFeedback::Grade::Unknown)return false;
  lastGameId=result.id;++state.gameFeedbackCount;state.gameShotId=result.id;
  state.resultMeter=result.meter*100;state.resultContest=result.contest;state.resultSent=recent.back().fill*100;
  state.resultPhase=shotFeedback::phaseName(result.phase);
  switch(result.grade){
   case shotFeedback::Grade::Perfect:state.gameGrade="Perfect";++state.gamePerfect;break;
   case shotFeedback::Grade::Early:state.gameGrade="Early";break;
   case shotFeedback::Grade::Late:state.gameGrade="Late";break;
   case shotFeedback::Grade::Good:state.gameGrade="Good";break;
   default:return false;
  }
  // Contest can change the outcome independently of release timing. Record
  // it, but require uncontested feedback before updating calibration.
  if(result.contest>0){consumed=true;streak=lastDirection=0;state.reason="Contested result recorded; calibration unchanged";return true;}
  if(result.grade==shotFeedback::Grade::Good){
   if(result.phase!=shotFeedback::Phase::Rising&&result.phase!=shotFeedback::Phase::Falling){consumed=true;streak=lastDirection=0;state.reason="Good recorded; meter direction was unclear, so no correction";return true;}
   const auto direction=result.phase==shotFeedback::Phase::Rising?Grade::Early:Grade::Late;
   if(feedback(result.id,direction,result.at,true))state.reason=std::string("Good on ")+shotFeedback::phaseName(result.phase)+" meter: "+state.reason;
   else {consumed=true;streak=lastDirection=0;state.reason="Good direction recorded; unstable or changed conditions prevent learning";}
   return true;
  }
  const auto grade=result.grade==shotFeedback::Grade::Perfect?Grade::Perfect:result.grade==shotFeedback::Grade::Early?Grade::Early:Grade::Late;
  if(!feedback(result.id,grade,result.at,true)){consumed=true;streak=lastDirection=0;state.reason="Game result recorded; unstable or changed conditions prevent learning";}
  return true;
 }
 View view(double t)const{
  auto out=state;out.stable=networkReady(t);
  out.pendingId=!consumed&&out.stable&&pending.tuning.generation==generation&&t>=pending.at&&t-pending.at<=20?pending.id:0;
  if(!out.stable)out.reason=pings.empty()||t-pings.back().at>2?"Waiting for fresh ping; learning paused":"Collecting stable ping; learning paused";
  return out;
 }
 size_t historySize()const{return recent.size();}
};
}
