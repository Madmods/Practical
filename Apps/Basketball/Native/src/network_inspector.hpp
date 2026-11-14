#pragma once
#include "webui/json_lite.hpp"
#include <functional>
#include <set>
#include <atomic>
#include <thread>
#include <mutex>
#ifndef NETWORK_INSPECTOR_MODEL_ONLY
#include <shellapi.h>
#endif

namespace networkInspector {
inline json::Value object(){json::Value v;v.type=json::Value::Object;return v;}
inline json::Value array(){json::Value v;v.type=json::Value::Array;return v;}
inline json::Value text(const std::wstring& s){json::Value v;v.type=json::Value::String;v.s=s;return v;}
inline json::Value number(double n){json::Value v;v.type=json::Value::Number;v.n=n;return v;}
inline json::Value flag(bool b){json::Value v;v.type=json::Value::Bool;v.b=b;return v;}
inline bool method(const std::wstring& s){return s==L"FireServer"||s==L"InvokeServer"||s==L"OnClientEvent"||s==L"OnClientInvoke";}
inline bool validPath(const std::wstring& s){return !s.empty()&&s.size()<=2048&&std::none_of(s.begin(),s.end(),[](wchar_t c){return c<32;});}
inline bool validValue(const json::Value& v,unsigned depth=0){
 if(depth>16)return false;
 if(v.type==json::Value::Number)return std::isfinite(v.n);
 if(v.type==json::Value::String)return v.s.size()<=8192;
 if(v.type==json::Value::Array){if(v.a.size()>128)return false;for(const auto& x:v.a)if(!validValue(x,depth+1))return false;}
 if(v.type==json::Value::Object){if(v.o.size()>128)return false;for(const auto& x:v.o)if(x.first.size()>256||!validValue(x.second,depth+1))return false;}
 return true;
}
inline bool validateLog(const json::Value& root,json::Value& calls,std::wstring& error){
 const auto& input=root[L"calls"];
 if(root.type!=json::Value::Object||input.type!=json::Value::Array||input.a.size()>500){error=L"Use a JSON object with a calls array of at most 500 records.";return false;}
 auto candidate=array();
 for(const auto& record:input.a){
  if(record.type!=json::Value::Object||!validPath(record[L"path"].str())||!method(record[L"method"].str())||record[L"args"].type!=json::Value::Array||record[L"args"].a.size()>64||!validValue(record[L"args"])){
   error=L"Each record needs a path, supported method, and bounded JSON args array. Nothing was imported.";return false;
  }
  if(record[L"timestamp"].type!=json::Value::String||record[L"timestamp"].s.size()>80){error=L"Each call needs a timestamp string (up to 80 characters).";return false;}
  auto clean=object();for(auto key:{L"path",L"method",L"args",L"timestamp"})clean.o[key]=record[key];
  clean.o[L"source"]=text(L"Imported log");candidate.a.push_back(std::move(clean));
 }
 calls=std::move(candidate);return true;
}
#ifndef NETWORK_INSPECTOR_MODEL_ONLY
inline bool copy(const std::wstring& value){
 if(value.size()>65536||!OpenClipboard(nullptr))return false;
 HGLOBAL memory=GlobalAlloc(GMEM_MOVEABLE,(value.size()+1)*sizeof(wchar_t));
 if(!memory){CloseClipboard();return false;}auto p=GlobalLock(memory);
 if(!p){GlobalFree(memory);CloseClipboard();return false;}
 memcpy(p,value.c_str(),(value.size()+1)*sizeof(wchar_t));GlobalUnlock(memory);
 bool ok=EmptyClipboard()&&SetClipboardData(CF_UNICODETEXT,memory);if(!ok)GlobalFree(memory);CloseClipboard();return ok;
}
class Service {
 std::mutex mutex_;json::Value catalog_=array(),calls_=array();std::wstring status_=L"Catalog not scanned",error_;
 std::wstring decoder_=L"Live call decoder unavailable",capture_=L"Stopped";std::filesystem::path helper_,session_;HANDLE host_=nullptr;bool enabled_=false;
 std::filesystem::file_time_type lastState_{};
 std::atomic<bool> requested_{false},stopping_{false},busy_{false};std::thread worker_;
 DWORD sessionPid_=0;addr sessionModel_=0;uint64_t sessionUniverse_=0;
 std::function<void(const json::Value&)> post_;
 static std::wstring wide(const std::string& s){try{return std::wstring(winrt::to_hstring(s));}catch(...){return L"<unreadable name>";}}
 void pollCapture(){
  std::filesystem::path session;HANDLE host=nullptr;{std::lock_guard<std::mutex> lock(mutex_);session=session_;host=host_;}if(!host)return;
  try{
   const auto file=session/L"state.json";
   if(std::filesystem::exists(file)&&std::filesystem::file_size(file)<=65536){
    const auto stamp=std::filesystem::last_write_time(file);
    if(stamp!=lastState_){
     std::ifstream input(file,std::ios::binary);std::string bytes((std::istreambuf_iterator<char>(input)),{});const auto root=json::parse(wide(bytes));
     json::Value calls;std::wstring error;
     if(validateLog(root,calls,error)){
      for(auto& call:calls.a)call.o[L"source"]=text(L"Captured traffic");
      std::lock_guard<std::mutex> lock(mutex_);calls_=std::move(calls);status_=root[L"status"].str().substr(0,2048);decoder_=root[L"decoder"].str().substr(0,512);capture_=root[L"capture"].str().substr(0,80);error_=root[L"error"].str().substr(0,2048);lastState_=stamp;
     }
    }
   }
  }catch(...){/* Atomic replacement may race a read; retry on the next poll. */}
  if(WaitForSingleObject(host,0)==WAIT_OBJECT_0){
   std::lock_guard<std::mutex> lock(mutex_);DWORD exit=0;GetExitCodeProcess(host_,&exit);CloseHandle(host_);host_=nullptr;capture_=L"Stopped";
   if(exit&&error_.empty())error_=L"Capture helper exited before completion. Retry capture.";
  }
  if(!stopping_)post_(report());
 }
 void scan(){
  const auto session=gameModules::router.get();HANDLE h=nullptr;DWORD pid=0;addr base=0;
  auto rows=array();std::wstring status;bool truncated=false;size_t visited=0;
  if(!gameModules::ready(session,now())||!connectRoblox(h,pid,base)||pid!=session.pid){if(h)CloseHandle(h);status=L"Reconnect to a supported game before scanning.";}
  else{
   struct Node{addr object;std::wstring path;unsigned depth;};std::deque<Node> queue;std::set<addr> seen;
   for(auto name:{"ReplicatedStorage","RobloxReplicatedStorage","Workspace"}){auto root=child(h,session.model,name);if(root)queue.push_back({root,wide(name),0});}
   const auto start=now();
   while(!queue.empty()&&!stopping_){
    if(++visited>12000||rows.a.size()>=1000||now()-start>2){truncated=true;break;}
    auto node=std::move(queue.front());queue.pop_front();if(!seen.insert(node.object).second)continue;
    const auto kind=defense::classOf(h,node.object);
    if(kind=="RemoteEvent"||kind=="UnreliableRemoteEvent"||kind=="RemoteFunction"){
     auto row=object();row.o[L"path"]=text(node.path);row.o[L"class"]=text(wide(kind));row.o[L"id"]=text(std::to_wstring(node.object));rows.a.push_back(std::move(row));
    }
    if(node.depth>=18){truncated=true;continue;}
    if(kind=="Script"||kind=="LocalScript"||kind=="ModuleScript")continue;
    for(auto descendant:children(h,node.object)){
     if(queue.size()>12000){truncated=true;break;}
     auto name=wide(nm(h,descendant));if(name.empty()||name.size()>256)continue;
     if(node.path.size()+name.size()+1>2048){truncated=true;continue;}
     queue.push_back({descendant,node.path+L"."+name,node.depth+1});
    }
   }
   CloseHandle(h);
   const auto current=gameModules::router.get();
   if(current.pid!=session.pid||current.model!=session.model||current.universeId!=session.universeId||!gameModules::ready(current,now())){rows=array();status=L"Game session changed during scan. Refresh the catalog.";}
   else status=truncated?L"Catalog snapshot reached its scan limit; some remotes may be missing.":L"Catalog snapshot complete. Discovered instances are not observed calls.";
  }
  {std::lock_guard<std::mutex> lock(mutex_);catalog_=std::move(rows);status_=std::move(status);error_.clear();}
 }
public:
 explicit Service(std::function<void(const json::Value&)> post,bool enableCapture=true):post_(std::move(post)),enabled_(enableCapture){
  wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);helper_=std::filesystem::path(exe).parent_path()/L"network_decoder"/L"runtime"/L"vanta-network-host.exe";
  if(enabled_&&std::filesystem::exists(helper_))decoder_=L"Ready — limited live decoder";
  if(enabled_)try{
   auto last=helper_.parent_path().parent_path()/L"last-capture.json";
   if(std::filesystem::exists(last)&&std::filesystem::file_size(last)<=65536){
    std::ifstream input(last,std::ios::binary);std::string bytes((std::istreambuf_iterator<char>(input)),{});auto root=json::parse(wide(bytes));json::Value previous;std::wstring error;
    if(validateLog(root,previous,error)){for(auto& call:previous.a)call.o[L"source"]=text(L"Captured traffic");calls_=std::move(previous);status_=L"Previous capture loaded. Capture is stopped. Search Clicked to find the throw call.";}
   }
  }catch(...){}
  worker_=std::thread([this](){CoInitializeEx(nullptr,COINIT_MULTITHREADED);unsigned tick=0;while(!stopping_){if(requested_.exchange(false)){try{scan();}catch(...){std::lock_guard<std::mutex> lock(mutex_);status_=L"Catalog scan failed. Refresh to retry.";catalog_=array();}busy_=false;if(!stopping_)post_(report());}if(++tick%12==0)pollCapture();Sleep(40);}CoUninitialize();});
 }
 ~Service(){stop();}
 void stopCapture(){std::lock_guard<std::mutex> lock(mutex_);if(host_){std::ofstream(session_/L"stop").put('1');std::ofstream(session_/L"packets.pcap.stop").put('1');capture_=L"Stopping";}}
 void stop(){stopCapture();stopping_=true;if(worker_.joinable())worker_.join();std::lock_guard<std::mutex> lock(mutex_);if(host_){CloseHandle(host_);host_=nullptr;}}
 void startCapture(){
  const auto game=gameModules::router.get();
  {std::lock_guard<std::mutex> lock(mutex_);if(host_)return;if(!enabled_||!std::filesystem::exists(helper_)){error_=L"Live decoder helper is unavailable in this build.";return;}if(!gameModules::ready(game,now())){error_=L"Connect to a supported game before starting capture.";return;}}
  try{
   auto session=std::filesystem::temp_directory_path()/L"VantaNetworkInspector"/(std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64()));std::filesystem::create_directories(session);
   auto parameters=L"--session \""+session.wstring()+L"\" --pid "+std::to_wstring(game.pid)+L" --parent "+std::to_wstring(GetCurrentProcessId())+L" --seconds 60";
   SHELLEXECUTEINFOW launch{sizeof launch};launch.fMask=SEE_MASK_NOCLOSEPROCESS|SEE_MASK_FLAG_NO_UI;launch.lpVerb=L"runas";launch.lpFile=helper_.c_str();launch.lpParameters=parameters.c_str();launch.nShow=SW_HIDE;
   if(!ShellExecuteExW(&launch)||!launch.hProcess){std::lock_guard<std::mutex> lock(mutex_);error_=L"Capture did not start. The helper needs Windows administrator approval.";return;}
   {std::lock_guard<std::mutex> lock(mutex_);session_=session;host_=launch.hProcess;lastState_={};capture_=L"Starting";decoder_=L"Waiting for capture helper";status_=L"Capture stops automatically after 60 seconds. Rejoin once to collect the schema.";error_.clear();calls_=array();}
  }catch(...){std::lock_guard<std::mutex> lock(mutex_);error_=L"Could not create a capture session.";}
  post_(report());
 }
 void refresh(){if(!busy_.exchange(true)){requested_=true;post_(report());}}
 json::Value report(){std::lock_guard<std::mutex> lock(mutex_);auto v=object();v.o[L"type"]=text(L"networkInspector");v.o[L"catalog"]=catalog_;v.o[L"calls"]=calls_;v.o[L"busy"]=flag(busy_);v.o[L"status"]=text(status_);v.o[L"error"]=text(error_);v.o[L"capture"]=text(capture_);v.o[L"decoder"]=text(decoder_);v.o[L"active"]=flag(host_!=nullptr);v.o[L"available"]=flag(enabled_&&std::filesystem::exists(helper_));return v;}
 bool import(const json::Value& root){json::Value calls;std::wstring error;bool ok=validateLog(root,calls,error);{std::lock_guard<std::mutex> lock(mutex_);error_=error;if(ok)calls_=std::move(calls);}post_(report());return ok;}
 void clear(){ {std::lock_guard<std::mutex> lock(mutex_);calls_=array();error_.clear();if(host_)std::ofstream(session_/L"clear").put('1');try{std::filesystem::remove(helper_.parent_path().parent_path()/L"last-capture.json");}catch(...){}}post_(report());}
 void observe(DWORD pid,addr model,uint64_t universe){bool changed=false;{std::lock_guard<std::mutex> lock(mutex_);if(pid!=sessionPid_||model!=sessionModel_||universe!=sessionUniverse_){sessionPid_=pid;sessionModel_=model;sessionUniverse_=universe;catalog_=array();status_=L"Game session changed. Refresh the catalog.";changed=true;}}if(changed)post_(report());}
 void reset(){stopCapture(); {std::lock_guard<std::mutex> lock(mutex_);catalog_=array();calls_=array();status_=L"Catalog not scanned";error_.clear();}post_(report());}
 void publish(){post_(report());}
};
inline std::wstring chooseLog(){
 Microsoft::WRL::ComPtr<IFileOpenDialog> dialog;if(FAILED(CoCreateInstance(CLSID_FileOpenDialog,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&dialog))))return {};
 COMDLG_FILTERSPEC filter{L"Decoded call log (*.json)",L"*.json"};dialog->SetFileTypes(1,&filter);dialog->SetTitle(L"Import a decoded call log");
 if(FAILED(dialog->Show(nullptr)))return {};Microsoft::WRL::ComPtr<IShellItem> item;if(FAILED(dialog->GetResult(&item)))return {};PWSTR p=nullptr;if(FAILED(item->GetDisplayName(SIGDN_FILESYSPATH,&p)))return {};std::wstring out=p;CoTaskMemFree(p);return out;
}
#endif
}

