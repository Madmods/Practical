#pragma once
#include "network_launch.hpp"
namespace ff3Packets {
class EarlyListener {
 using Launcher=std::function<HANDLE(const std::filesystem::path&,const std::wstring&)>;
 std::filesystem::path helper_,folder_;Launcher launcher_;HANDLE child_=nullptr;ULONGLONG retryAt_=0,startedAt_=0;unsigned attempts_=0;
public:
 EarlyListener(std::filesystem::path helper={},Launcher launcher={}):helper_(std::move(helper)),launcher_(std::move(launcher)){
  if(helper_.empty()){wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);helper_=std::filesystem::path(exe).parent_path()/L"network_decoder"/L"runtime"/L"vanta-network-host.exe";}
  if(!launcher_)launcher_=vantaNetwork::startHidden;
 }
 bool ensure(){
  if(child_){if(WaitForSingleObject(child_,0)!=WAIT_OBJECT_0){if(GetTickCount64()-startedAt_>5000&&!ready())std::ofstream(folder_/L"stop").put('1');return true;}CloseHandle(child_);child_=nullptr;}
  if(attempts_>=3||GetTickCount64()<retryAt_)return false;
  ++attempts_;retryAt_=GetTickCount64()+5000;
  try{if(!std::filesystem::exists(helper_))return false;folder_=std::filesystem::temp_directory_path()/L"VantaNetworkInspector"/(L"join-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));std::filesystem::create_directories(folder_);
   const auto args=L"--join-listener --session \""+folder_.wstring()+L"\" --pid 0 --parent "+std::to_wstring(GetCurrentProcessId());child_=launcher_(helper_,args);startedAt_=GetTickCount64();return child_!=nullptr;
  }catch(...){return false;}
 }
 bool ready()const{
  if(!child_||WaitForSingleObject(child_,0)!=WAIT_TIMEOUT)return false;
  try{auto file=folder_/L"state.json";if(!std::filesystem::exists(file)||std::filesystem::file_size(file)>4096)return false;std::ifstream in(file);std::string bytes((std::istreambuf_iterator<char>(in)),{});auto state=json::parse(std::wstring(winrt::to_hstring(bytes)));const double tick=double(GetTickCount64()),heartbeat=state[L"heartbeat"].num();return state[L"listenerReady"].boolean()&&heartbeat>0&&tick>=heartbeat&&tick-heartbeat<3000;}catch(...){return false;}
 }
 const std::filesystem::path& folder()const{return folder_;}
 bool waitReady(unsigned milliseconds){const auto until=GetTickCount64()+milliseconds;do{if(ready())return true;if(!child_||WaitForSingleObject(child_,0)!=WAIT_TIMEOUT)break;Sleep(10);}while(GetTickCount64()<until);return false;}
 void stop(){if(!folder_.empty()){std::ofstream(folder_/L"stop").put('1');}if(child_){CloseHandle(child_);child_=nullptr;}}
 ~EarlyListener(){stop();}
};
}
