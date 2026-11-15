#pragma once
#include "webui/vanta_ui.hpp"
#include "ff3_qb_runtime.hpp"
namespace ff3Hud {
inline json::Value message(const ff3Native::Frame& f){
 using namespace basketballUi;auto out=object();out.o[L"type"]=string(L"ff3Hud");auto state=object();
 const auto& shot=f.throwView;
 if(!f.held&&shot.phase==ff3qb::ThrowView::Done){
  state.o[L"phase"]=string(L"done");
  if(std::isfinite(shot.eta))state.o[L"eta"]=number(shot.eta);if(std::isfinite(shot.toSpot))state.o[L"to_spot"]=number(shot.toSpot);if(std::isfinite(shot.nearD))state.o[L"near_d"]=number(shot.nearD);
  state.o[L"air"]=number(shot.air);state.o[L"distance"]=number(shot.distance);state.o[L"flightSub"]=string(widen(shot.detail));state.o[L"result"]=string(widen(shot.result));state.o[L"resultBy"]=string(widen(shot.resultBy));
  state.o[L"unknownCoverage"]=basketballUi::boolean(!shot.coverageKnown);state.o[L"intable"]=basketballUi::boolean(shot.intable);
  if(std::isfinite(shot.riskAt))state.o[L"riskAt"]=number(shot.riskAt);
  auto target=object();target.o[L"role"]=string(widen(shot.name.empty()?"Receiver":shot.name));state.o[L"target"]=target;
 }else if(f.ready&&f.held){
  auto trajectory=ff3Native::predictedThrow(f,now());if(!trajectory.valid)trajectory=f.solution;
  state.o[L"phase"]=string(L"windup");state.o[L"mode"]=string(ff3Native::force95?L"Beam":widen(f.passLabel.empty()?ff3qb::presetName(f.mode):f.passLabel));
  state.o[L"power"]=number(f.power/95);state.o[L"angle"]=number(trajectory.angle);state.o[L"air"]=number(trajectory.time);state.o[L"distance"]=number(f.distance);
  auto target=object();target.o[L"role"]=string(widen(f.name));state.o[L"target"]=target;
  double nearest=1000;for(const auto& d:f.hudDefenders)nearest=std::min(nearest,ff3qb::length(d.position-trajectory.catchPoint));
  state.o[L"open"]=number(nearest);state.o[L"unknownCoverage"]=basketballUi::boolean(!f.hudCoverageKnown);
  const auto risk=ff3qb::riskForecast(trajectory,f.gravity,f.hudDefenders,std::clamp(f.ping*.0005+.05,0.,.25));
  state.o[L"intable"]=basketballUi::boolean(f.hudCoverageKnown&&risk.intable);
  state.o[L"possibleRisk"]=basketballUi::boolean(f.hudCoverageKnown&&risk.possible);
  state.o[L"clearance"]=number(risk.margin);
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
  info.glass=vanta::Glass::None;
  if(!view_.Create(instance,info,{}))return false;current_=this;timer_=SetTimer(nullptr,0,60,tick);return timer_!=0;
 }
 void stop(){if(timer_)KillTimer(nullptr,timer_);timer_=0;current_=nullptr;view_.Shutdown();}
 ~Overlay(){stop();}
};
inline int test(HINSTANCE instance,const std::filesystem::path& folder){
 ff3Native::Frame boundary;boundary.ready=boundary.held=boundary.hudCoverageKnown=true;boundary.gravity=0;boundary.solution={true,{0,8,0},{10,0,0},{10,8,0},1,0};boundary.hudDefenders={{{5,8,1.5},{},0,0,0,1,1,1,196.2}};
 if(message(boundary)[L"throw"][L"intable"].boolean())return 1; // A safety buffer is not an overlapping catch volume.
 boundary.hudDefenders[0].position.z=.5;if(!message(boundary)[L"throw"][L"intable"].boolean())return 1;
 if(message(boundary)[L"throw"][L"riskAt"].num()<=0)return 1; // Warn before release about a future crossing, not an already-passed DB.
 boundary.throwView.phase=ff3qb::ThrowView::Done;boundary.throwView.name="Previous pass";boundary.name="Current receiver";
 auto current=message(boundary)[L"throw"];if(current[L"phase"].str()!=L"windup"||current[L"target"][L"role"].str()!=L"Current receiver")return 1;
 boundary.ready=false;if(!message(boundary)[L"throw"][L"phase"].str().empty())return 1; // An equipped ball cannot show another flight/result card.
 ff3Native::Frame unrelated;unrelated.balls.push_back({});if(!message(unrelated)[L"throw"][L"phase"].str().empty())return 1;
 ff3Native::Frame owned;owned.throwView.phase=ff3qb::ThrowView::Flight;owned.throwView.name="Selected WR";owned.throwView.toSpot=4.25;owned.throwView.nearD=8.5;owned.throwView.coverageKnown=owned.throwView.intable=true;
 if(!message(owned)[L"throw"][L"phase"].str().empty())return 1; // In-flight capsule removed, even for our own ball.
 owned.throwView.phase=ff3qb::ThrowView::Done;owned.throwView.result="Intercepted";if(message(owned)[L"throw"][L"result"].str()!=L"Intercepted")return 1;
 vanta::UiHost view;vanta::AppInfo info;info.title=L"Vanta HUD test";info.page=L"throw-hud.html";info.resource=107;info.overlay=true;info.testMode=true;info.dataName=L"VantaQbHudTest";info.glass=vanta::Glass::None;int result=1,captures=0;
 vanta::UiCallbacks cb;cb.custom=[&](const std::wstring& type,const json::Value& value){
  if(type==L"hudFixture"){view.PlaceOverlay(nullptr);ff3Native::Frame fixture;fixture.ready=fixture.held=true;fixture.name="Native message";fixture.power=95;fixture.solution.angle=28;view.Post(json::dump(message(fixture)));}
  if(type==L"hudReveal"){RECT area{-3000,-1080,-1920,0};view.PlaceOverlay(&area,1000,110,0);}
  if(type==L"hudCapture"){auto phase=value[L"phase"].str();if(phase==L"aim"||phase==L"result")view.Capture((folder/(L"qb-html-"+phase+L".png")).wstring(),[&](bool ok){if(ok)++captures;});}
  if(type==L"hudTestResult"){std::ofstream file(folder/L"qb-html-test.json");file<<winrt::to_string(json::dump(value));result=value[L"ok"].boolean()&&captures==2?0:1;PostQuitMessage(0);}
  if(type==L"testTimeout")PostQuitMessage(0);
 };
 if(!view.Create(instance,info,cb))return 1;
 RECT area{-3000,-1080,-1920,0};view.PlaceOverlay(&area,1000,110,0);
 const auto styles=GetWindowLongPtrW(view.Window(),GWL_EXSTYLE);if(!(styles&WS_EX_NOACTIVATE)||!(styles&WS_EX_TOOLWINDOW)){view.Shutdown();return 1;}
 view.Run();view.Shutdown();return result;
}
}
