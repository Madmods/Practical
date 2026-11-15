#pragma once
#include "webui/vanta_ui.hpp"
#include "ff3_qb_runtime.hpp"
namespace ff3Hud {
inline json::Value message(const ff3Native::Frame& f){
 using namespace basketballUi;auto out=object();out.o[L"type"]=string(L"ff3Hud");auto state=object();
 if(f.ready&&f.held){
  state.o[L"phase"]=string(L"windup");state.o[L"mode"]=string(ff3Native::force95?L"Beam":widen(ff3qb::presetName(f.mode)));
  state.o[L"power"]=number(f.power/95);state.o[L"angle"]=number(f.solution.angle);state.o[L"air"]=number(f.solution.time);state.o[L"distance"]=number(f.distance);
  auto target=object();target.o[L"role"]=string(widen(f.name));state.o[L"target"]=target;
  double nearest=1000;for(const auto& d:f.defenders)nearest=std::min(nearest,ff3qb::length(d.position-(f.targetPosition-f.fitOffset-f.leadOffset)));
  state.o[L"open"]=number(nearest);state.o[L"intable"]=basketballUi::boolean(f.coverageKnown&&f.margin<=ff3qb::catchSafetyMargin);
 }else if(!f.held){
  const ff3qb::BallFlight* ball=nullptr;ff3qb::FlightForecast prediction;
  for(const auto& b:f.balls){auto p=ff3qb::forecast(b,f.groundY);if(p.valid&&(!ball||b.born>ball->born)){ball=&b;prediction=p;}}
  if(ball){state.o[L"phase"]=string(L"flight");state.o[L"eta"]=number(prediction.remaining);state.o[L"air"]=number(prediction.remaining+f.at-ball->born);state.o[L"flightSub"]=string(L"Predicted field landing");auto target=object();target.o[L"role"]=string(L"Ball flight");state.o[L"target"]=target;state.o[L"unknownCoverage"]=basketballUi::boolean(true);}
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
 vanta::UiHost view;vanta::AppInfo info;info.title=L"Vanta HUD test";info.page=L"throw-hud.html";info.resource=107;info.overlay=true;info.testMode=true;info.dataName=L"VantaQbHudTest";info.glass=vanta::Glass::None;int result=1,captures=0;
 vanta::UiCallbacks cb;cb.custom=[&](const std::wstring& type,const json::Value& value){
  if(type==L"hudFixture"){view.PlaceOverlay(nullptr);view.Post(L"{\"type\":\"ff3Hud\",\"throw\":{\"phase\":\"windup\",\"mode\":\"Beam\",\"power\":1,\"angle\":28,\"target\":{\"role\":\"Native message\"}}}");}
  if(type==L"hudReveal"){RECT area{-3000,-1080,-1920,0};view.PlaceOverlay(&area,1000,110,0);}
  if(type==L"hudCapture"){auto phase=value[L"phase"].str();if(phase==L"aim"||phase==L"flight")view.Capture((folder/(L"qb-html-"+phase+L".png")).wstring(),[&](bool ok){if(ok)++captures;});}
  if(type==L"hudTestResult"){std::ofstream file(folder/L"qb-html-test.json");file<<winrt::to_string(json::dump(value));result=value[L"ok"].boolean()&&captures==2?0:1;PostQuitMessage(0);}
  if(type==L"testTimeout")PostQuitMessage(0);
 };
 if(!view.Create(instance,info,cb))return 1;
 RECT area{-3000,-1080,-1920,0};view.PlaceOverlay(&area,1000,110,0);
 const auto styles=GetWindowLongPtrW(view.Window(),GWL_EXSTYLE);if(!(styles&WS_EX_NOACTIVATE)||!(styles&WS_EX_TOOLWINDOW)){view.Shutdown();return 1;}
 view.Run();view.Shutdown();return result;
}
}
