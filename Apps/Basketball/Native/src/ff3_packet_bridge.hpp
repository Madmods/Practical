#pragma once
#include "ff3_qb_runtime.hpp"
#include <shellapi.h>

namespace ff3Packets {
inline void applyEnvironmentPolicy(json::Value& plan,const ff3Native::Frame& f){
 using namespace basketballUi;
 plan.o[L"environment"]=string(f.practiceRange?L"practice":L"match");
 plan.o[L"boundaryRequired"]=basketballUi::boolean(!f.practiceRange);
 auto field=object();field.o[L"known"]=basketballUi::boolean(!f.practiceRange&&f.field.valid());field.o[L"center"]=array();field.o[L"right"]=array();field.o[L"forward"]=array();
 for(auto v:{f.field.center.x,f.field.center.y,f.field.center.z})field.o[L"center"].a.push_back(number(v));
 for(auto v:{f.field.right.x,f.field.right.y,f.field.right.z})field.o[L"right"].a.push_back(number(v));
 for(auto v:{f.field.forward.x,f.field.forward.y,f.field.forward.z})field.o[L"forward"].a.push_back(number(v));
 field.o[L"halfWidth"]=number(f.field.halfWidth);field.o[L"halfLength"]=number(f.field.halfLength);plan.o[L"field"]=field;
}
class Service {
 ff3qb::ResumeIntent resume_;
 std::mutex mutex_;HANDLE host_=nullptr;std::filesystem::path helper_,session_;ULONGLONG lastWrite_=0,lastRead_=0,lastClick_=0;unsigned applied_=0,observedApplied_=0;std::atomic<bool> ready_{false},editing_{false};bool preserving_=false,retryable_=false,prepared_=false,rejoin_=false;DWORD pid_=0,attempted_=0;std::wstring phase_=L"waiting",message_=L"Waiting for Football Fusion 3";
 using Launcher=std::function<HANDLE(const std::filesystem::path&,const std::wstring&)>;Launcher launcher_;
 static void marker(const std::filesystem::path& path){try{std::ofstream(path).put('1');}catch(...){}}
 static json::Value vector(ff3qb::Vec value){auto out=basketballUi::array();for(auto v:{value.x,value.y,value.z})out.a.push_back(basketballUi::number(v));return out;}
 static json::Value reach(const ff3qb::Defender& d){auto item=basketballUi::object();item.o[L"position"]=vector(d.position);item.o[L"velocity"]=vector(d.velocity);item.o[L"speed"]=basketballUi::number(d.speed);item.o[L"jump"]=basketballUi::number(d.jumpHeight);item.o[L"dive"]=basketballUi::number(d.diveReach);item.o[L"radius"]=basketballUi::number(d.radius);item.o[L"above"]=basketballUi::number(d.above);item.o[L"below"]=basketballUi::number(d.below);item.o[L"worldGravity"]=basketballUi::number(d.worldGravity);item.o[L"redirect"]=basketballUi::boolean(d.redirect);return item;}
 static void write(const std::filesystem::path& path,const json::Value& value){auto temporary=path;temporary+=L".tmp";{std::ofstream out(temporary,std::ios::binary);out<<winrt::to_string(json::dump(value));out.flush();if(!out)return;}MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);}
 bool launch(DWORD pid,bool edit,bool retry){
  if(host_){if(WaitForSingleObject(host_,0)!=WAIT_OBJECT_0){if(pid_==pid&&!std::filesystem::exists(session_/L"stop")){if(edit){std::error_code ec;std::filesystem::remove(session_/L"setup-only",ec);if(ec)return false;std::filesystem::remove(session_/L"resume-after-join",ec);editing_=true;preserving_=false;resume_.cancel();}return true;}return false;}CloseHandle(host_);host_=nullptr;}
  if(!pid||(!retry&&attempted_==pid))return false;
  attempted_=pid;editing_=false;prepared_=false;ready_=false;rejoin_=false;retryable_=true;phase_=L"failed";
  if(!std::filesystem::exists(helper_)){message_=L"Networking helper is unavailable. Retry setup after restoring it";return false;}
  try{
   session_=std::filesystem::temp_directory_path()/L"VantaNetworkInspector"/(L"qb-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));std::filesystem::create_directories(session_);
   const auto parameters=L"--qb-mode --session \""+session_.wstring()+L"\" --pid "+std::to_wstring(pid)+L" --parent "+std::to_wstring(GetCurrentProcessId());
   if(!edit)marker(session_/L"setup-only");
   host_=launcher_(helper_,parameters);
   if(!host_){message_=L"Networking setup needs Windows administrator approval. Use Retry setup to try again";return false;}
   pid_=pid;editing_=edit;preserving_=false;resume_.cancel();retryable_=false;applied_=observedApplied_=0;lastRead_=lastWrite_=0;phase_=L"preparing";message_=L"Preparing networking automatically";return true;
  }catch(...){message_=L"Could not prepare networking. Retry setup";return false;}
 }
 void pollLocked(){
  if(!host_)return;const auto tick=GetTickCount64();if(tick-lastRead_<200)return;lastRead_=tick;
  try{auto path=session_/L"state.json";if(std::filesystem::exists(path)&&std::filesystem::file_size(path)<=65536){std::ifstream in(path,std::ios::binary);std::string bytes((std::istreambuf_iterator<char>(in)),{});auto root=json::parse(std::wstring(winrt::to_hstring(bytes)));const double heartbeat=root[L"heartbeat"].num();const bool fresh=heartbeat>0&&double(tick)>=heartbeat&&double(tick)-heartbeat<3000;
   const auto capture=root[L"capture"].str();prepared_=root[L"ready"].boolean()&&fresh;ready_=prepared_&&capture==L"Running";rejoin_=root[L"requiresRejoin"].boolean();phase_=root[L"phase"].str(L"preparing");retryable_=capture==L"Stopped"||capture==L"Failed";message_=root[L"error"].str();if(message_.empty())message_=root[L"status"].str(L"Preparing networking");
   if(root[L"ready"].boolean()&&!fresh){phase_=L"stalled";message_=L"Networking traffic stalled; retry setup";retryable_=true;}if(!fresh&&capture!=L"Stopped"&&capture!=L"Failed")rejoin_=false;
   const auto count=root[L"stats"][L"modifiedThrows"].num();if(count>=0&&count<100000)observedApplied_=unsigned(count);
   if(!editing_&&ready_)message_=L"Networking ready. QBA is off";
  }}catch(...){}
  if(WaitForSingleObject(host_,0)==WAIT_OBJECT_0){CloseHandle(host_);host_=nullptr;ready_=false;prepared_=false;editing_=false;retryable_=true;phase_=L"failed";}
 }
public:
 Service(std::filesystem::path helper={},Launcher launcher={}):helper_(std::move(helper)),launcher_(std::move(launcher)){
  if(helper_.empty()){wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);helper_=std::filesystem::path(exe).parent_path()/L"network_decoder"/L"runtime"/L"vanta-network-host.exe";}
  if(!launcher_)launcher_=[](const std::filesystem::path& helper,const std::wstring& parameters){SHELLEXECUTEINFOW request{sizeof request};request.fMask=SEE_MASK_NOCLOSEPROCESS|SEE_MASK_FLAG_NO_UI;request.lpVerb=L"runas";request.lpFile=helper.c_str();request.lpParameters=parameters.c_str();request.nShow=SW_HIDE;return ShellExecuteExW(&request)?request.hProcess:nullptr;};
 }
 ~Service(){stop();if(host_)CloseHandle(host_);}
 bool prepare(DWORD pid,bool retry=false){std::lock_guard<std::mutex> lock(mutex_);if(host_&&pid_!=pid&&WaitForSingleObject(host_,0)!=WAIT_OBJECT_0){editing_=false;ready_=false;prepared_=false;marker(session_/L"stop");attempted_=0;return false;}return launch(pid,false,retry);}
 bool start(DWORD pid){std::lock_guard<std::mutex> lock(mutex_);return launch(pid,true,true);}
 void poll(){std::lock_guard<std::mutex> lock(mutex_);pollLocked();}
 void disableEdits(){std::lock_guard<std::mutex> lock(mutex_);editing_=false;resume_.cancel();if(host_){auto inactive=basketballUi::object();inactive.o[L"active"]=basketballUi::boolean(false);write(session_/L"plan.json",inactive);}}
 json::Value networking(){std::lock_guard<std::mutex> lock(mutex_);auto out=basketballUi::object();out.o[L"type"]=basketballUi::string(L"qbNetworking");out.o[L"phase"]=basketballUi::string(phase_);out.o[L"detail"]=basketballUi::string(message_);out.o[L"ready"]=basketballUi::boolean(prepared_);out.o[L"editing"]=basketballUi::boolean(editing_);out.o[L"requiresRejoin"]=basketballUi::boolean(rejoin_);out.o[L"retryable"]=basketballUi::boolean(retryable_);return out;}
 bool resumeWanted(DWORD pid){std::lock_guard<std::mutex> lock(mutex_);return resume_.take(pid);}
 void suspendForRejoin(){std::lock_guard<std::mutex> lock(mutex_);resume_.suspend(ff3Native::enabled,pid_);if(!host_)return;
  // Only a bounded, sniff-only setup may span reconnection. Running packet
  // modification is always stopped and drained before a different session.
  bool preparing=false;try{auto path=session_/L"state.json";if(std::filesystem::exists(path)&&std::filesystem::file_size(path)<65536){std::ifstream in(path);std::string bytes((std::istreambuf_iterator<char>(in)),{});auto state=json::parse(std::wstring(winrt::to_hstring(bytes)));preparing=state[L"capture"].str()==L"Preparing";}}catch(...){}
  ready_=false;prepared_=false;editing_=false;attempted_=0;preserving_=preparing;if(preparing){marker(session_/L"resume-after-join");marker(session_/L"setup-only");}else marker(session_/L"stop");
 }
 void stop(){std::lock_guard<std::mutex> lock(mutex_);resume_.cancel();preserving_=false;ready_=false;prepared_=false;editing_=false;if(host_)marker(session_/L"stop");}
 bool retryable(){std::lock_guard<std::mutex> lock(mutex_);return retryable_;}
 std::pair<bool,std::wstring> status(){std::lock_guard<std::mutex> lock(mutex_);return{ready_,message_};}
 bool inputReady()const{return editing_.load()&&ready_.load();}
 void update(const ff3Native::Frame& input){
  bool recorded=false;
  {std::lock_guard<std::mutex> lock(mutex_);if(!host_)return;
   const auto dispatch=ff3Native::packetFrame(input);const auto tick=GetTickCount64(),clicked=dispatch.clicked;const auto& f=dispatch.frame;const bool clickOwned=dispatch.retained;
   if(tick-lastWrite_>=25||clicked!=lastClick_){
    auto plan=basketballUi::object();const bool active=editing_&&ff3Native::enabled&&f.ready&&f.revision==ff3Native::revision&&(clickOwned||(now()>=f.at&&now()-f.at<.1))&&ff3Native::admitted()&&ff3Native::focused(f.pid)&&!f.menuOpen&&!f.textFocus;
    plan.o[L"physicalHeld"]=basketballUi::boolean(dispatch.held);plan.o[L"releasedAt"]=basketballUi::number(double(dispatch.released));
    plan.o[L"cursorX"]=basketballUi::number(f.cursor.x-f.origin.x);plan.o[L"cursorY"]=basketballUi::number(f.cursor.y-f.origin.y);plan.o[L"selectionRadius"]=basketballUi::number(ff3Native::selectionRadius.load());
    plan.o[L"lockedTarget"]=basketballUi::number(double(ff3Native::lockedTarget.load()));plan.o[L"selectedTarget"]=basketballUi::number(double(f.target));
    plan.o[L"selectionReason"]=basketballUi::string(basketballUi::widen(f.reason));plan.o[L"targetName"]=basketballUi::string(basketballUi::widen(f.name));plan.o[L"held"]=basketballUi::boolean(f.held);plan.o[L"wheel"]=basketballUi::boolean(f.wheelVisible);plan.o[L"sampleAgeMs"]=basketballUi::number(std::max(0.,now()-f.at)*1000);
    const bool powerFirst=ff3Native::automaticAngle&&ff3Native::autoPower&&f.mode==0;plan.o[L"mode"]=basketballUi::string(L"ff3-qb");plan.o[L"active"]=basketballUi::boolean(active);plan.o[L"pid"]=basketballUi::number(f.pid);plan.o[L"issued"]=basketballUi::number(double(tick));plan.o[L"expires"]=basketballUi::number(double(tick+150));plan.o[L"clickAt"]=basketballUi::number(double(clicked));plan.o[L"head"]=vector(f.head);plan.o[L"targetPosition"]=vector(f.targetPosition);plan.o[L"targetVelocity"]=vector(f.targetVelocity);plan.o[L"gravity"]=basketballUi::number(f.gravity);plan.o[L"latency"]=basketballUi::number(std::clamp(f.ping*.0005+.05,0.,.25));plan.o[L"lob"]=basketballUi::boolean(ff3Native::automaticAngle&&ff3Native::autoPower?f.selectedLob:(ff3Native::arc==1||f.mode==2));plan.o[L"plannedPower"]=basketballUi::number(f.power);plan.o[L"angle"]=basketballUi::number(f.solution.angle);plan.o[L"anglePower"]=basketballUi::boolean(ff3Native::anglePowerPlan(f));plan.o[L"autoPower"]=basketballUi::boolean(ff3Native::autoPower);
    plan.o[L"targetGrace"]=basketballUi::boolean(f.targetGrace);plan.o[L"angleFallback"]=basketballUi::boolean(ff3Native::automaticAngle);plan.o[L"preserveCatchHeight"]=basketballUi::boolean(true);plan.o[L"heightAdjustment"]=basketballUi::number(f.leadOffset.y);plan.o[L"input"]=ff3Native::inputReport();
    plan.o[L"wobbleFiltered"]=basketballUi::boolean(f.wobbleFiltered);plan.o[L"rawTargetVelocity"]=vector(f.rawTargetVelocity);plan.o[L"routeCorrection"]=vector(f.routeCorrection);
    plan.o[L"deliveryMargin"]=basketballUi::number(f.deliveryMargin);plan.o[L"launchMargin"]=basketballUi::number(f.launchMargin);
    plan.o[L"candidatesConsidered"]=basketballUi::number(f.candidatesConsidered);plan.o[L"candidatesRanked"]=basketballUi::number(f.candidatesRanked);plan.o[L"powersChecked"]=basketballUi::number(double(f.powersChecked));
    auto ranked=basketballUi::array();for(size_t i=0;i<std::min(size_t(3),f.rankedThrows.size());++i){const auto& p=f.rankedThrows[i];auto item=basketballUi::object();item.o[L"rank"]=basketballUi::number(double(i+1));item.o[L"power"]=basketballUi::number(p.power);item.o[L"angle"]=basketballUi::number(p.solution.angle);item.o[L"catchPoint"]=vector(p.solution.catchPoint);item.o[L"offset"]=vector(p.offset);item.o[L"deliveryMargin"]=basketballUi::number(ff3qb::deliveryClearance(p));item.o[L"launchMargin"]=basketballUi::number(ff3qb::launchClearance(p));item.o[L"wholeMargin"]=basketballUi::number(p.margin);ranked.a.push_back(item);}plan.o[L"rankedThrows"]=ranked;
    plan.o[L"minAngle"]=basketballUi::number(f.minimumAngle);
    plan.o[L"maxAngle"]=basketballUi::number(f.maximumAngle);
    plan.o[L"beam95"]=basketballUi::boolean(powerFirst&&ff3Native::force95);plan.o[L"smartFit"]=basketballUi::boolean(f.guarded);plan.o[L"coverageKnown"]=basketballUi::boolean(f.coverageKnown);
    plan.o[L"coverageDetail"]=basketballUi::string(basketballUi::widen(f.coverageDetail));plan.o[L"catchHeight"]=basketballUi::number(f.fitOffset.y);plan.o[L"fitOffset"]=vector(f.fitOffset);plan.o[L"flightTime"]=basketballUi::number(f.solution.time);plan.o[L"landing"]=vector(f.solution.catchPoint);
    plan.o[L"passStyle"]=basketballUi::string(basketballUi::widen(f.passLabel));plan.o[L"riskKnown"]=basketballUi::boolean(f.hudCoverageKnown);plan.o[L"projectedIntable"]=basketballUi::boolean(f.risk.intable);plan.o[L"possibleReachRisk"]=basketballUi::boolean(f.risk.possible);
    plan.o[L"coverageModel"]=basketballUi::number(3);plan.o[L"checkMissedCatch"]=basketballUi::boolean(f.guarded&&std::isfinite(f.groundY));plan.o[L"clearanceStuds"]=basketballUi::number(f.margin);plan.o[L"limitingCatcher"]=basketballUi::number(f.coverageTrace.catcher);plan.o[L"limitingTime"]=basketballUi::number(f.coverageTrace.time);plan.o[L"limitingPoint"]=vector(f.coverageTrace.point);
    plan.o[L"routePosition"]=vector(f.targetPosition-f.fitOffset-f.leadOffset);plan.o[L"leadDistance"]=basketballUi::number(f.appliedLead);plan.o[L"groundY"]=basketballUi::number(std::isfinite(f.groundY)?f.groundY:0);plan.o[L"planningMs"]=basketballUi::number(f.planningMs);plan.o[L"allowContested"]=basketballUi::boolean(true);
    plan.o[L"renderHz"]=basketballUi::number(ff3Native::renderHz.load());plan.o[L"renderMs"]=basketballUi::number(ff3Native::renderMs.load());
    const auto& shot=input.throwView;auto ownThrow=basketballUi::object();ownThrow.o[L"phase"]=basketballUi::number(int(shot.phase));ownThrow.o[L"ball"]=basketballUi::number(double(shot.ball));ownThrow.o[L"result"]=basketballUi::string(basketballUi::widen(shot.result));ownThrow.o[L"coverageKnown"]=basketballUi::boolean(shot.coverageKnown);ownThrow.o[L"intable"]=basketballUi::boolean(shot.intable);if(std::isfinite(shot.toSpot))ownThrow.o[L"wrToSpot"]=basketballUi::number(shot.toSpot);if(std::isfinite(shot.nearD))ownThrow.o[L"nearestD"]=basketballUi::number(shot.nearD);plan.o[L"ownThrow"]=ownThrow;
    auto flights=basketballUi::array();for(const auto& ball:f.balls){auto item=basketballUi::object();item.o[L"position"]=vector(ball.position);item.o[L"velocity"]=vector(ball.velocity);item.o[L"gravity"]=basketballUi::number(ball.gravity);item.o[L"samples"]=basketballUi::number(double(ball.trail.size()));flights.a.push_back(item);}plan.o[L"flights"]=flights;
    auto catchers=basketballUi::array();for(const auto& d:f.defenders)catchers.a.push_back(reach(d));plan.o[L"catchers"]=catchers;
    plan.o[L"receiver"]=reach(f.receiverReach);
    applyEnvironmentPolicy(plan,f);
    plan.o[L"clickPlanRetained"]=basketballUi::boolean(clickOwned);if(clicked&&clicked!=lastClick_&&active&&!ff3Native::logPath.empty())write(ff3Native::logPath.parent_path()/L"ff3-qb-last-release.json",plan);write(session_/L"plan.json",plan);lastWrite_=tick;lastClick_=clicked;
   }
   pollLocked();if(observedApplied_>applied_){applied_=observedApplied_;recorded=true;}
  }
  if(recorded)ff3Native::record(input,"QB aim and power applied",true);
 }
};
}
