#pragma once
#include "ff3_qb_runtime.hpp"
#include <shellapi.h>

namespace ff3Packets {
class Service {
 ff3qb::ResumeIntent resume_;
 std::mutex mutex_;HANDLE host_=nullptr;std::filesystem::path helper_,session_;ULONGLONG lastWrite_=0,lastRead_=0;unsigned applied_=0;bool ready_=false,preserving_=false;DWORD pid_=0;std::wstring message_=L"Enable QB aim to begin";
 static void marker(const std::filesystem::path& path){try{std::ofstream(path).put('1');}catch(...){}}
 static json::Value vector(ff3qb::Vec value){auto out=basketballUi::array();for(auto v:{value.x,value.y,value.z})out.a.push_back(basketballUi::number(v));return out;}
 static json::Value reach(const ff3qb::Defender& d){auto item=basketballUi::object();item.o[L"position"]=vector(d.position);item.o[L"velocity"]=vector(d.velocity);item.o[L"speed"]=basketballUi::number(d.speed);item.o[L"jump"]=basketballUi::number(d.jumpHeight);item.o[L"dive"]=basketballUi::number(d.diveReach);item.o[L"radius"]=basketballUi::number(d.radius);item.o[L"above"]=basketballUi::number(d.above);item.o[L"below"]=basketballUi::number(d.below);item.o[L"worldGravity"]=basketballUi::number(d.worldGravity);return item;}
 static void write(const std::filesystem::path& path,const json::Value& value){auto temporary=path;temporary+=L".tmp";{std::ofstream out(temporary,std::ios::binary);out<<winrt::to_string(json::dump(value));out.flush();if(!out)return;}MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);}
public:
 Service(){wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);helper_=std::filesystem::path(exe).parent_path()/L"network_decoder"/L"runtime"/L"vanta-network-host.exe";}
 ~Service(){stop();if(host_)CloseHandle(host_);}
 bool start(DWORD pid){
  std::lock_guard<std::mutex> lock(mutex_);
  if(host_){if(WaitForSingleObject(host_,0)!=WAIT_OBJECT_0){if(preserving_&&pid_==pid&&!std::filesystem::exists(session_/L"stop")){std::error_code ec;std::filesystem::remove(session_/L"setup-only",ec);if(ec)return false;std::filesystem::remove(session_/L"resume-after-join",ec);preserving_=false;resume_.cancel();return true;}message_=L"Finishing the previous throw; retry shortly";return false;}CloseHandle(host_);host_=nullptr;}
  if(!pid||!std::filesystem::exists(helper_)){message_=L"QB throw helper is unavailable";return false;}
  try{
   session_=std::filesystem::temp_directory_path()/L"VantaNetworkInspector"/(L"qb-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));std::filesystem::create_directories(session_);
   const auto parameters=L"--qb-mode --session \""+session_.wstring()+L"\" --pid "+std::to_wstring(pid)+L" --parent "+std::to_wstring(GetCurrentProcessId());
   SHELLEXECUTEINFOW launch{sizeof launch};launch.fMask=SEE_MASK_NOCLOSEPROCESS|SEE_MASK_FLAG_NO_UI;launch.lpVerb=L"runas";launch.lpFile=helper_.c_str();launch.lpParameters=parameters.c_str();launch.nShow=SW_HIDE;
   if(!ShellExecuteExW(&launch)||!launch.hProcess){message_=L"QB setup needs Windows administrator approval";return false;}
   host_=launch.hProcess;pid_=pid;preserving_=false;resume_.cancel();ready_=false;applied_=0;lastRead_=lastWrite_=0;message_=L"Preparing QB throw control";return true;
  }catch(...){message_=L"Could not start QB throw control";return false;}
 }
 bool resumeWanted(DWORD pid){std::lock_guard<std::mutex> lock(mutex_);return resume_.take(pid);}
 void suspendForRejoin(){std::lock_guard<std::mutex> lock(mutex_);resume_.suspend(ff3Native::enabled,pid_);if(!host_)return;
  // Only a bounded, sniff-only setup may span reconnection. Running packet
  // modification is always stopped and drained before a different session.
  bool preparing=false;try{auto path=session_/L"state.json";if(std::filesystem::exists(path)&&std::filesystem::file_size(path)<65536){std::ifstream in(path);std::string bytes((std::istreambuf_iterator<char>(in)),{});auto state=json::parse(std::wstring(winrt::to_hstring(bytes)));preparing=state[L"capture"].str()==L"Preparing";}}catch(...){}
  ready_=false;preserving_=preparing;if(preparing){marker(session_/L"resume-after-join");marker(session_/L"setup-only");}else marker(session_/L"stop");
 }
 void stop(){std::lock_guard<std::mutex> lock(mutex_);resume_.cancel();preserving_=false;ready_=false;if(host_)marker(session_/L"stop");}
 std::pair<bool,std::wstring> status(){std::lock_guard<std::mutex> lock(mutex_);return{ready_,message_};}
 void update(const ff3Native::Frame& f){
  bool recorded=false;
  {std::lock_guard<std::mutex> lock(mutex_);if(!host_)return;const auto tick=GetTickCount64();
   if(tick-lastWrite_>=25){
    auto plan=basketballUi::object();const bool active=ff3Native::enabled&&f.ready&&f.revision==ff3Native::revision&&now()>=f.at&&now()-f.at<.1&&ff3Native::admitted()&&ff3Native::focused(f.pid)&&!f.textFocus;
    plan.o[L"selectionReason"]=basketballUi::string(basketballUi::widen(f.reason));plan.o[L"targetName"]=basketballUi::string(basketballUi::widen(f.name));plan.o[L"held"]=basketballUi::boolean(f.held);plan.o[L"wheel"]=basketballUi::boolean(f.wheelVisible);plan.o[L"sampleAgeMs"]=basketballUi::number(std::max(0.,now()-f.at)*1000);
    const bool powerFirst=ff3Native::automaticAngle&&ff3Native::autoPower&&f.mode==0;plan.o[L"mode"]=basketballUi::string(L"ff3-qb");plan.o[L"active"]=basketballUi::boolean(active);plan.o[L"pid"]=basketballUi::number(f.pid);plan.o[L"issued"]=basketballUi::number(double(tick));plan.o[L"expires"]=basketballUi::number(double(tick+150));plan.o[L"clickAt"]=basketballUi::number(double(ff3Native::physicalClickAt.load()));plan.o[L"head"]=vector(f.head);plan.o[L"targetPosition"]=vector(f.targetPosition);plan.o[L"targetVelocity"]=vector(f.targetVelocity);plan.o[L"gravity"]=basketballUi::number(f.gravity);plan.o[L"latency"]=basketballUi::number(std::clamp(f.ping*.0005+.05,0.,.25));plan.o[L"lob"]=basketballUi::boolean(powerFirst?f.selectedLob:(ff3Native::arc==1||f.mode==2));plan.o[L"plannedPower"]=basketballUi::number(f.power);plan.o[L"angle"]=basketballUi::number(f.solution.angle);plan.o[L"anglePower"]=basketballUi::boolean(ff3Native::autoPower&&(!powerFirst||!ff3Native::force95));plan.o[L"autoPower"]=basketballUi::boolean(ff3Native::autoPower);
    plan.o[L"beam95"]=basketballUi::boolean(powerFirst&&ff3Native::force95);plan.o[L"smartFit"]=basketballUi::boolean(f.guarded);plan.o[L"coverageKnown"]=basketballUi::boolean(f.coverageKnown);
    plan.o[L"coverageDetail"]=basketballUi::string(basketballUi::widen(f.coverageDetail));plan.o[L"catchHeight"]=basketballUi::number(f.fitOffset.y);plan.o[L"fitOffset"]=vector(f.fitOffset);plan.o[L"flightTime"]=basketballUi::number(f.solution.time);plan.o[L"landing"]=vector(f.solution.catchPoint);
    plan.o[L"coverageModel"]=basketballUi::number(2);plan.o[L"clearanceStuds"]=basketballUi::number(f.margin);plan.o[L"limitingCatcher"]=basketballUi::number(f.coverageTrace.catcher);plan.o[L"limitingTime"]=basketballUi::number(f.coverageTrace.time);plan.o[L"limitingPoint"]=vector(f.coverageTrace.point);
    plan.o[L"routePosition"]=vector(f.targetPosition-f.fitOffset);plan.o[L"planningMs"]=basketballUi::number(f.planningMs);
    auto flights=basketballUi::array();for(const auto& ball:f.balls){auto item=basketballUi::object();item.o[L"position"]=vector(ball.position);item.o[L"velocity"]=vector(ball.velocity);item.o[L"gravity"]=basketballUi::number(ball.gravity);item.o[L"samples"]=basketballUi::number(double(ball.trail.size()));flights.a.push_back(item);}plan.o[L"flights"]=flights;
    auto catchers=basketballUi::array();for(const auto& d:f.defenders)catchers.a.push_back(reach(d));plan.o[L"catchers"]=catchers;
    if(f.receiverReachKnown)plan.o[L"receiver"]=reach(f.receiverReach);
    write(session_/L"plan.json",plan);lastWrite_=tick;
   }
   if(tick-lastRead_>=200){lastRead_=tick;try{
     auto path=session_/L"state.json";if(std::filesystem::exists(path)&&std::filesystem::file_size(path)<=65536){std::ifstream in(path,std::ios::binary);std::string bytes((std::istreambuf_iterator<char>(in)),{});auto root=json::parse(std::wstring(winrt::to_hstring(bytes)));const double heartbeat=root[L"heartbeat"].num();const bool fresh=heartbeat>0&&double(tick)>=heartbeat&&double(tick)-heartbeat<3000;ready_=root[L"ready"].boolean()&&fresh;message_=root[L"error"].str();if(message_.empty())message_=root[L"status"].str(L"Preparing QB throw control");if(root[L"ready"].boolean()&&!fresh)message_=L"QB traffic stalled; reconnecting is required";auto count=root[L"stats"][L"modifiedThrows"].num();if(count>applied_&&count<100000){applied_=unsigned(count);recorded=true;}}
    }catch(...){}
    if(WaitForSingleObject(host_,0)==WAIT_OBJECT_0){CloseHandle(host_);host_=nullptr;ready_=false;if(message_.empty())message_=L"QB throw control stopped; toggle QB aim to retry";}
   }
  }
  if(recorded)ff3Native::record(f,"QB aim and power applied",true);
 }
};
}
