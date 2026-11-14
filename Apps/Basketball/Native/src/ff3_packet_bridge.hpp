#pragma once
#include "ff3_qb_runtime.hpp"
#include <shellapi.h>

namespace ff3Packets {
class Service {
 std::mutex mutex_;HANDLE host_=nullptr;std::filesystem::path helper_,session_;ULONGLONG lastWrite_=0,lastRead_=0;unsigned applied_=0;bool ready_=false;std::wstring message_=L"Enable QB aim to begin";
 static void marker(const std::filesystem::path& path){try{std::ofstream(path).put('1');}catch(...){}}
 static json::Value vector(ff3qb::Vec value){auto out=basketballUi::array();for(auto v:{value.x,value.y,value.z})out.a.push_back(basketballUi::number(v));return out;}
 static void write(const std::filesystem::path& path,const json::Value& value){auto temporary=path;temporary+=L".tmp";{std::ofstream out(temporary,std::ios::binary);out<<winrt::to_string(json::dump(value));out.flush();if(!out)return;}MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);}
public:
 Service(){wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);helper_=std::filesystem::path(exe).parent_path()/L"network_decoder"/L"runtime"/L"vanta-network-host.exe";}
 ~Service(){stop();if(host_)CloseHandle(host_);}
 bool start(DWORD pid){
  std::lock_guard<std::mutex> lock(mutex_);
  if(host_){if(WaitForSingleObject(host_,0)!=WAIT_OBJECT_0){message_=L"Finishing the previous throw; retry shortly";return false;}CloseHandle(host_);host_=nullptr;}
  if(!pid||!std::filesystem::exists(helper_)){message_=L"QB throw helper is unavailable";return false;}
  try{
   session_=std::filesystem::temp_directory_path()/L"VantaNetworkInspector"/(L"qb-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));std::filesystem::create_directories(session_);
   const auto parameters=L"--qb-mode --session \""+session_.wstring()+L"\" --pid "+std::to_wstring(pid)+L" --parent "+std::to_wstring(GetCurrentProcessId());
   SHELLEXECUTEINFOW launch{sizeof launch};launch.fMask=SEE_MASK_NOCLOSEPROCESS|SEE_MASK_FLAG_NO_UI;launch.lpVerb=L"runas";launch.lpFile=helper_.c_str();launch.lpParameters=parameters.c_str();launch.nShow=SW_HIDE;
   if(!ShellExecuteExW(&launch)||!launch.hProcess){message_=L"QB setup needs Windows administrator approval";return false;}
   host_=launch.hProcess;ready_=false;applied_=0;lastRead_=lastWrite_=0;message_=L"Preparing QB throw control";return true;
  }catch(...){message_=L"Could not start QB throw control";return false;}
 }
 void stop(){std::lock_guard<std::mutex> lock(mutex_);ready_=false;if(host_)marker(session_/L"stop");}
 std::pair<bool,std::wstring> status(){std::lock_guard<std::mutex> lock(mutex_);return{ready_,message_};}
 void update(const ff3Native::Frame& f){
  bool recorded=false;
  {std::lock_guard<std::mutex> lock(mutex_);if(!host_)return;const auto tick=GetTickCount64();
   if(tick-lastWrite_>=25){
    auto plan=basketballUi::object();const bool active=ff3Native::enabled&&f.ready&&f.revision==ff3Native::revision&&now()>=f.at&&now()-f.at<.1&&ff3Native::admitted()&&ff3Native::focused(f.pid)&&!f.textFocus;
    plan.o[L"mode"]=basketballUi::string(L"ff3-qb");plan.o[L"active"]=basketballUi::boolean(active);plan.o[L"pid"]=basketballUi::number(f.pid);plan.o[L"issued"]=basketballUi::number(double(tick));plan.o[L"expires"]=basketballUi::number(double(tick+150));plan.o[L"clickAt"]=basketballUi::number(double(ff3Native::physicalClickAt.load()));plan.o[L"head"]=vector(f.head);plan.o[L"targetPosition"]=vector(f.targetPosition);plan.o[L"targetVelocity"]=vector(f.targetVelocity);plan.o[L"gravity"]=basketballUi::number(f.gravity);plan.o[L"latency"]=basketballUi::number(std::clamp(f.ping*.0005+.05,0.,.25));plan.o[L"lob"]=basketballUi::boolean(ff3Native::arc==1||f.mode==2);plan.o[L"angle"]=basketballUi::number(f.solution.angle);plan.o[L"anglePower"]=basketballUi::boolean(ff3Native::autoPower);plan.o[L"autoPower"]=basketballUi::boolean(ff3Native::autoPower);
    write(session_/L"plan.json",plan);lastWrite_=tick;
   }
   if(tick-lastRead_>=200){lastRead_=tick;try{
     auto path=session_/L"state.json";if(std::filesystem::exists(path)&&std::filesystem::file_size(path)<=65536){std::ifstream in(path,std::ios::binary);std::string bytes((std::istreambuf_iterator<char>(in)),{});auto root=json::parse(std::wstring(winrt::to_hstring(bytes)));ready_=root[L"ready"].boolean();message_=root[L"error"].str();if(message_.empty())message_=root[L"status"].str(L"Preparing QB throw control");auto count=root[L"stats"][L"modifiedThrows"].num();if(count>applied_&&count<100000){applied_=unsigned(count);recorded=true;}}
    }catch(...){}
    if(WaitForSingleObject(host_,0)==WAIT_OBJECT_0){CloseHandle(host_);host_=nullptr;ready_=false;if(message_.empty())message_=L"QB throw control stopped; toggle QB aim to retry";}
   }
  }
  if(recorded)ff3Native::record(f,"QB aim and power applied",true);
 }
};
}
