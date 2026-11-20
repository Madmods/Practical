#pragma once
#include "webui/vanta_ui.hpp"
#include "ff3_qb_runtime.hpp"
namespace ff3Hud {
inline bool throwSession(const ff3Native::Frame& f,double time){
 const auto& shot=f.throwView;
 return shot.click&&shot.heldBall&&f.localPlayer&&f.pid&&f.model&&f.own&&shot.pid==f.pid&&shot.model==f.model&&shot.own==f.own&&shot.player==f.localPlayer&&std::isfinite(time)&&std::isfinite(shot.releasedAt)&&time>=shot.releasedAt&&time-shot.releasedAt<=15;
}
inline bool ownThrowSession(const ff3Native::Frame& f,double time){return f.throwView.ballVerified&&throwSession(f,time);}
struct Landing {bool owned=false;double eta=NAN,elapsed=0;};
inline Landing ownLanding(const ff3Native::Frame& f,double time){
 Landing out;const auto& shot=f.throwView;
 if(f.held||shot.phase!=ff3qb::ThrowView::Flight||!shot.ball||!ownThrowSession(f,time))return out;
 const ff3qb::BallFlight* ball=nullptr;for(const auto& candidate:f.balls)if(candidate.id==shot.ball){if(ball)return out;ball=&candidate;}
 if(!ball||!ball->airborne||ball->ended||ball->landed||!std::isfinite(ball->at)||time<ball->at||time-ball->at>.30||!std::isfinite(ball->born)||!std::isfinite(shot.ballBorn)||std::abs(ball->born-shot.ballBorn)>.0001||ball->born<shot.releasedAt-.1)return out;
 const bool attributed=ball->thrower&&(ball->thrower==f.localPlayer||ball->thrower==f.own);
 if(ball->thrower&&!attributed)return out;
 if(!attributed&&ball->id!=shot.heldBall)return out;
 out.owned=true;out.elapsed=time-shot.releasedAt;
 if(!ball->model.valid)return out;
 auto observed=*ball;observed.at=ball->model.observedTime(ball->position);const auto forecast=ff3qb::forecast(observed,f.groundY);
 if(forecast.valid)out.eta=std::max(0.,forecast.remaining-(time-ball->at));
 return out;
}
inline json::Value message(const ff3Native::Frame& f,double time=now()){
 using namespace basketballUi;auto out=object();out.o[L"type"]=string(L"ff3Hud");auto state=object();
 const auto& shot=f.throwView;
 if(!f.held&&shot.phase==ff3qb::ThrowView::Pending&&throwSession(f,time)&&time-shot.releasedAt<=1.2){
  state.o[L"phase"]=string(L"release");state.o[L"throwId"]=string(std::to_wstring(f.pid)+L":"+std::to_wstring(shot.click));state.o[L"mode"]=string(L"Released");state.o[L"power"]=number(shot.power/95);state.o[L"angle"]=number(shot.angle);state.o[L"air"]=number(shot.air);state.o[L"distance"]=number(shot.distance);state.o[L"check"]=string(L"CHECKING");
  auto target=object();target.o[L"role"]=string(widen(shot.name.empty()?"Receiver":shot.name));state.o[L"target"]=target;
 }else if(!f.held&&shot.phase==ff3qb::ThrowView::Flight){
  const auto landing=ownLanding(f,time);
  if(landing.owned){
   state.o[L"phase"]=string(L"flight");state.o[L"owned"]=basketballUi::boolean(true);state.o[L"ballId"]=string(std::to_wstring(shot.ball));state.o[L"throwId"]=string(std::to_wstring(f.pid)+L":"+std::to_wstring(shot.click));state.o[L"elapsed"]=number(landing.elapsed);if(std::isfinite(landing.eta))state.o[L"eta"]=number(landing.eta);
   auto target=object();target.o[L"role"]=string(widen(shot.name.empty()?"Receiver":shot.name));state.o[L"target"]=target;
   state.o[L"mode"]=string(widen(shot.mode));state.o[L"power"]=number(shot.power/95);state.o[L"angle"]=number(shot.angle);state.o[L"air"]=number(std::isfinite(landing.eta)?landing.elapsed+landing.eta:shot.air);state.o[L"distance"]=number(shot.distance);
   if(std::isfinite(shot.toSpot))state.o[L"to_spot"]=number(shot.toSpot);if(std::isfinite(shot.nearD))state.o[L"near_d"]=number(shot.nearD);
   state.o[L"flightSub"]=string(widen(shot.detail));state.o[L"unknownCoverage"]=basketballUi::boolean(!shot.coverageKnown);state.o[L"intable"]=basketballUi::boolean(shot.coverageKnown&&shot.intable);
   state.o[L"check"]=string(!shot.coverageKnown?L"CHECKING":shot.intable?L"INTERCEPT":std::isfinite(shot.margin)&&shot.margin<=ff3qb::catchSafetyMargin?L"TIGHT":L"CLEAR");
  }
  else if(ownThrowSession(f,time)&&shot.ball){
   // Keep a populated handoff while an observed landing becomes a result.
   // Never retain or extrapolate a countdown after the live ball disappears.
   const ff3qb::BallFlight* observed=nullptr;bool duplicate=false;for(const auto& b:f.balls)if(b.id==shot.ball){duplicate=observed!=nullptr;observed=&b;}
   if(observed&&!duplicate&&std::isfinite(observed->at)&&time>=observed->at&&time-observed->at<=.3&&std::isfinite(observed->born)&&std::abs(observed->born-shot.ballBorn)<.0001&&(!observed->thrower||observed->thrower==f.localPlayer||observed->thrower==f.own)&&(observed->landed||observed->ended)){
    state.o[L"phase"]=string(L"settling");state.o[L"throwId"]=string(std::to_wstring(f.pid)+L":"+std::to_wstring(shot.click));state.o[L"result"]=string(L"Ball landed");state.o[L"resultBy"]=string(L"Checking result");state.o[L"air"]=number(time-shot.releasedAt);state.o[L"distance"]=number(shot.distance);
    auto target=object();target.o[L"role"]=string(widen(shot.name.empty()?"Receiver":shot.name));state.o[L"target"]=target;
   }
   else if(!observed&&time>=f.at&&time-f.at<=.2){
    state.o[L"phase"]=string(L"flight");state.o[L"owned"]=basketballUi::boolean(true);state.o[L"ballId"]=string(std::to_wstring(shot.ball));state.o[L"throwId"]=string(std::to_wstring(f.pid)+L":"+std::to_wstring(shot.click));state.o[L"flightSub"]=string(L"Reading your ball");state.o[L"check"]=string(L"CHECKING");
    auto target=object();target.o[L"role"]=string(widen(shot.name.empty()?"Receiver":shot.name));state.o[L"target"]=target;
   }
  }
 }else if(!f.held&&shot.phase==ff3qb::ThrowView::Done&&ownThrowSession(f,time)){
  state.o[L"phase"]=string(L"done");
  if(std::isfinite(shot.eta))state.o[L"eta"]=number(shot.eta);if(std::isfinite(shot.toSpot))state.o[L"to_spot"]=number(shot.toSpot);if(std::isfinite(shot.nearD))state.o[L"near_d"]=number(shot.nearD);
  state.o[L"air"]=number(shot.air);state.o[L"distance"]=number(shot.distance);state.o[L"flightSub"]=string(widen(shot.detail));state.o[L"result"]=string(widen(shot.result));state.o[L"resultBy"]=string(widen(shot.resultBy));
  state.o[L"unknownCoverage"]=basketballUi::boolean(!shot.coverageKnown);state.o[L"intable"]=basketballUi::boolean(shot.intable);
  if(std::isfinite(shot.riskAt))state.o[L"riskAt"]=number(shot.riskAt);
  auto target=object();target.o[L"role"]=string(widen(shot.name.empty()?"Receiver":shot.name));state.o[L"target"]=target;
 }else if(f.ready&&f.held){
  auto trajectory=ff3Native::predictedThrow(f,now());if(!trajectory.valid){out.o[L"throw"]=state;return out;}
  state.o[L"phase"]=string(L"windup");state.o[L"mode"]=string(ff3Native::force95?L"Beam":widen(f.passLabel.empty()?ff3qb::presetName(f.mode):f.passLabel));
  state.o[L"power"]=number(f.power/95);state.o[L"angle"]=number(trajectory.angle);state.o[L"air"]=number(trajectory.time);state.o[L"distance"]=number(f.distance);
  auto target=object();target.o[L"role"]=string(widen(f.name));state.o[L"target"]=target;
  double nearest=1000;for(const auto& d:f.hudDefenders)nearest=std::min(nearest,ff3qb::length(d.position-trajectory.catchPoint));
  state.o[L"open"]=number(nearest);state.o[L"unknownCoverage"]=basketballUi::boolean(!f.hudCoverageKnown||f.hudReachEstimated);
  const auto risk=ff3qb::riskForecast(trajectory,f.gravity,f.hudDefenders,std::clamp(f.ping*.0005+.05,0.,.25),f.groundY);
  state.o[L"intable"]=basketballUi::boolean(f.hudCoverageKnown&&risk.intable);
  state.o[L"possibleRisk"]=basketballUi::boolean(f.hudCoverageKnown&&risk.possible);
  state.o[L"clearance"]=number(risk.margin);
  state.o[L"check"]=string(ff3qb::throwCheckLabel(ff3qb::throwCheck(f.hudCoverageKnown,f.hudReachEstimated,risk)));
  if(f.hudCoverageKnown&&(risk.intable||risk.possible))state.o[L"riskAt"]=number(risk.intable?risk.projected.time:risk.limit.time);
 }
 out.o[L"throw"]=state;return out;
}
class Overlay {
 vanta::UiHost view_;UINT_PTR timer_=0;inline static Overlay* current_=nullptr;
 static void CALLBACK tick(HWND,UINT,UINT_PTR,DWORD){if(current_)current_->update();}
 void update(){
  auto f=ff3Native::snapshot();bool live=ff3Native::enabled&&ff3Native::showHud&&ff3Native::admitted()&&ff3Native::focused(f.pid)&&now()>=f.at&&now()-f.at<.2;
  if(!live){view_.PlaceOverlay(nullptr);return;}
  RECT area{f.origin.x,f.origin.y,f.origin.x+f.client.right,f.origin.y+f.client.bottom};
  view_.PlaceOverlay(&area,1000,110,.035);view_.Post(json::dump(message(f)));
 }
public:
 bool start(HINSTANCE instance,const vanta::AppInfo& parent){
  vanta::AppInfo info=parent;info.title=L"Vanta QB HUD";info.page=L"throw-hud.html";info.resource=107;info.overlay=true;info.dataName=L"VantaQbHud";info.product=L"ff3Hud";info.testMode=false;
  info.glass=parent.glass;
  if(!view_.Create(instance,info,{}))return false;current_=this;timer_=SetTimer(nullptr,0,60,tick);return timer_!=0;
 }
 void stop(){if(timer_)KillTimer(nullptr,timer_);timer_=0;current_=nullptr;view_.Shutdown();}
 ~Overlay(){stop();}
};
inline int test(HINSTANCE instance,const std::filesystem::path& folder){
 ff3Native::Frame boundary;boundary.studio=true;boundary.ready=boundary.held=boundary.hudCoverageKnown=true;boundary.gravity=0;boundary.solution={true,{0,8,0},{10,0,0},{10,8,0},1,0};boundary.hudDefenders={{{5,8,1.5},{},0,0,0,1,1,1,196.2}};
 if(message(boundary)[L"throw"][L"intable"].boolean())return 1; // A safety buffer is not an overlapping catch volume.
 boundary.hudDefenders[0].position.z=.5;if(!message(boundary)[L"throw"][L"intable"].boolean())return 1;
 if(message(boundary)[L"throw"][L"riskAt"].num()<=0)return 1; // Warn before release about a future crossing, not an already-passed DB.
 boundary.throwView.phase=ff3qb::ThrowView::Done;boundary.throwView.name="Previous pass";boundary.name="Current receiver";
 auto current=message(boundary)[L"throw"];if(current[L"phase"].str()!=L"windup"||current[L"target"][L"role"].str()!=L"Current receiver")return 1;
 boundary.ready=false;if(!message(boundary)[L"throw"][L"phase"].str().empty())return 1; // An equipped ball cannot show another flight/result card.
 ff3Native::Frame unrelated;unrelated.balls.push_back({});if(!message(unrelated)[L"throw"][L"phase"].str().empty())return 1;
 ff3Native::Frame owned;owned.pid=7;owned.model=1;owned.own=2;owned.localPlayer=3;owned.at=10.2;owned.groundY=0;owned.menuOpen=false;
 ff3qb::BallFlight ball;ball.id=88;ball.thrower=3;ball.airborne=true;ball.born=10.1;ball.at=10.2;ball.position={0,60,0};ball.gravity=28;ball.fieldY=0;ball.model={true,10.2,28,0,ball.position,{50,10,0},10.2};owned.balls={ball};
 ff3qb::ThrowIntent launch;launch.pid=7;launch.click=10;launch.model=1;launch.own=2;launch.player=3;launch.heldBall=5;launch.at=10;
 ff3qb::ThrowTracker tracker;tracker.begin(launch,{},{});tracker.update(10.2,1,2,{ball},{},0,false);owned.throwView=tracker.view;owned.throwView.eta=.05;
 unsigned ownershipChecks=0;auto checkOwn=[&](bool ok){++ownershipChecks;if(!ok)std::ofstream(folder/L"qb-hud-native-failure.txt")<<ownershipChecks;return ok;};
 owned.throwView.name="Practice WR";owned.throwView.mode="Beam";owned.throwView.power=95;owned.throwView.angle=24;owned.throwView.toSpot=7;owned.throwView.nearD=9;
 auto base=message(owned,10.2)[L"throw"];
 if(!checkOwn(base[L"target"][L"role"].str()==L"Practice WR"&&base[L"mode"].str()==L"Beam"&&base[L"power"].num()==1&&base[L"angle"].num()==24&&base[L"to_spot"].num()==7&&base[L"near_d"].num()==9))return 1;
 if(!checkOwn(base[L"phase"].str()==L"flight"&&base[L"owned"].boolean()&&base[L"ballId"].str()==L"88"&&base[L"eta"].num()>2))return 1;
 if(!checkOwn(base[L"eta"].num()>owned.throwView.eta+1))return 1; // Ground landing, not the earlier receiver opportunity.
 auto changed=owned;auto foreign=ball;foreign.id=99;foreign.thrower=900;foreign.position={0,1,0};foreign.model.origin=foreign.position;
 changed.balls={foreign,ball};if(!checkOwn(std::abs(message(changed,10.2)[L"throw"][L"eta"].num()-base[L"eta"].num())<1e-8))return 1;
 changed.balls={foreign};if(!checkOwn(message(changed,10.2)[L"throw"][L"ballId"].str()==L"88"&&message(changed,10.2)[L"throw"][L"eta"].type==json::Value::Null))return 1;
 foreign.thrower=3;changed.balls={foreign};if(!checkOwn(message(changed,10.2)[L"throw"][L"ballId"].str()==L"88"&&message(changed,10.2)[L"throw"][L"eta"].type==json::Value::Null))return 1; // Another own ball never supplies our countdown.
 for(int i=0;i<13;++i){changed=owned;switch(i){case 0:changed.balls[0].thrower=900;break;case 1:changed.balls[0].thrower=0;break;case 2:changed.balls[0].born+=1;break;case 3:changed.balls[0].airborne=false;break;case 4:changed.balls[0].ended=true;break;case 5:changed.balls[0].landed=true;break;case 6:changed.held=true;break;case 7:changed.model=9;break;case 8:changed.own=9;break;case 9:changed.localPlayer=9;break;case 10:changed.pid=9;break;case 11:changed.throwView.click=0;break;case 12:changed.balls.push_back(ball);break;}
  if(!checkOwn((i==4||i==5)?message(changed,10.2)[L"throw"][L"phase"].str()==L"settling":message(changed,10.2)[L"throw"][L"phase"].str().empty()))return 1;
 }
 changed=owned;changed.balls[0].model.valid=false;if(!checkOwn(message(changed,10.2)[L"throw"][L"phase"].str()==L"flight"&&message(changed,10.2)[L"throw"][L"eta"].type==json::Value::Null))return 1;
 changed=owned;changed.groundY=NAN;changed.balls[0].fieldY=NAN;if(!checkOwn(message(changed,10.2)[L"throw"][L"eta"].type==json::Value::Null))return 1;
 if(!checkOwn(message(owned,10.6)[L"throw"][L"phase"].str().empty()&&message(owned,10.1)[L"throw"][L"phase"].str().empty()))return 1;
 changed=owned;changed.throwView.heldBall=88;changed.balls[0].thrower=0;if(!checkOwn(message(changed,10.2)[L"throw"][L"phase"].str()==L"flight"))return 1;
 tracker.begin(launch,{ball},{});tracker.update(10.2,1,2,{ball},{},0,false);changed=owned;changed.throwView=tracker.view;if(!checkOwn(message(changed,10.2)[L"throw"][L"phase"].str()==L"release"&&message(changed,10.2)[L"throw"][L"eta"].type==json::Value::Null))return 1;
 if(!checkOwn(!owned.releaseVisual&&message(owned,10.2)[L"throw"][L"phase"].str()==L"flight"))return 1; // Independent of the optional aim-beam snapshot.
 changed=owned;changed.throwView.phase=ff3qb::ThrowView::Done;changed.throwView.result="Intercepted";if(!checkOwn(message(changed,10.2)[L"throw"][L"result"].str()==L"Intercepted"))return 1;
 changed.throwView.ballVerified=false;if(!checkOwn(message(changed,10.2)[L"throw"][L"phase"].str().empty()))return 1;
 changed=owned;changed.throwView.phase=ff3qb::ThrowView::Pending;changed.throwView.ballVerified=false;changed.throwView.ball=0;
 if(!checkOwn(message(changed,10.2)[L"throw"][L"phase"].str()==L"release"&&message(changed,10.2)[L"throw"][L"eta"].type==json::Value::Null))return 1;
 if(!checkOwn(message(changed,11.3)[L"throw"][L"phase"].str().empty()))return 1;
 changed.model=9;if(!checkOwn(message(changed,10.2)[L"throw"][L"phase"].str().empty()))return 1;
 vanta::UiHost view;vanta::AppInfo info;info.title=L"Vanta HUD test";info.page=L"throw-hud.html";info.resource=107;info.overlay=true;info.testMode=true;info.dataName=L"VantaQbHudTest";info.glass=vanta::Glass::None;int result=1,captures=0;
 vanta::UiCallbacks cb;cb.custom=[&](const std::wstring& type,const json::Value& value){
  if(type==L"hudFixture"){view.PlaceOverlay(nullptr);ff3Native::Frame fixture;fixture.studio=fixture.solution.valid=true;fixture.ready=fixture.held=true;fixture.name="Native message";fixture.power=95;fixture.solution.angle=28;view.Post(json::dump(message(fixture)));}
  if(type==L"hudLandingFixture")view.Post(json::dump(message(owned,10.2)));
  if(type==L"hudReveal"){RECT area{-3000,-1080,-1920,0};view.PlaceOverlay(&area,1000,110,0);}
  if(type==L"hudCapture"){auto phase=value[L"phase"].str();if(phase==L"aim"||phase==L"flight"||phase==L"result"||phase==L"transition-start"||phase==L"transition-mid"||phase==L"transition-end")view.Capture((folder/(L"qb-html-"+phase+L".png")).wstring(),[&](bool ok){if(ok)++captures;});}
  if(type==L"hudTestResult"){auto report=value;report.o[L"nativeOwnershipChecks"]=basketballUi::number(ownershipChecks);std::ofstream file(folder/L"qb-html-test.json");file<<winrt::to_string(json::dump(report));result=value[L"ok"].boolean()&&captures==6?0:1;PostQuitMessage(0);}
  if(type==L"testTimeout")PostQuitMessage(0);
 };
 if(!view.Create(instance,info,cb))return 1;
 DWORD affinity=0;if(!GetWindowDisplayAffinity(view.Window(),&affinity)||affinity!=capturePrivacy::Exclude){view.Shutdown();return 1;}
 RECT area{-3000,-1080,-1920,0};view.PlaceOverlay(&area,1000,110,0);
 const auto styles=GetWindowLongPtrW(view.Window(),GWL_EXSTYLE);if(!(styles&WS_EX_NOACTIVATE)||!(styles&WS_EX_TOOLWINDOW)){view.Shutdown();return 1;}
 view.Run();view.Shutdown();return result;
}
}
