#pragma once
#include <array>
// Process discovery has one version source: the offset header used by the reader.
enum class ConnectionState { Waiting, VersionMismatch, AccessDenied, ModuleUnavailable, Loading, Connected };
inline std::atomic<ConnectionState> connectionState{ConnectionState::Waiting};
inline std::atomic<bool> connectionProcessPresent{false},connectionVersionMatches{false};
inline const wchar_t* connectionLabel(){
 switch(connectionState.load()){
 case ConnectionState::VersionMismatch:return L"Roblox update — offsets needed";
 case ConnectionState::AccessDenied:return L"Roblox access denied";
 case ConnectionState::ModuleUnavailable:return L"Retrying Roblox connection";
 case ConnectionState::Loading:return L"Waiting for game data";
 case ConnectionState::Connected:return L"Roblox connected";
 default:return L"Waiting for Roblox";
 }
}
inline bool supportedClientPath(const std::wstring& file){
 auto end=file.find_last_of(L"\\/");if(end==std::wstring::npos)return false;
 auto start=file.find_last_of(L"\\/",end-1);
 auto folder=file.substr(start==std::wstring::npos?0:start+1,end-(start==std::wstring::npos?0:start+1));
 std::wstring expected(Offsets::ClientVersion.begin(),Offsets::ClientVersion.end());
 return !_wcsicmp(folder.c_str(),expected.c_str());
}
// Foreground changes must never retarget a running companion. Prefer the
// newest version-checked client; explicit diagnostic PID pins still win.
inline std::array<std::uint64_t,4> robloxClientPriority(bool pinned,int compatibility,bool /*foreground*/,std::uint64_t created=0){return {std::uint64_t(pinned),created,std::uint64_t(std::max(0,compatibility)),0};}
inline std::uint64_t robloxCreated(HANDLE process){FILETIME born{},exit{},kernel{},user{};if(!GetProcessTimes(process,&born,&exit,&kernel,&user))return 0;return (std::uint64_t(born.dwHighDateTime)<<32)|born.dwLowDateTime;}
inline bool supportedRobloxPid(DWORD pid){
 HANDLE process=pid?OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid):nullptr;if(!process)return false;
 wchar_t file[32768]{};DWORD size=32768;bool ok=QueryFullProcessImageNameW(process,0,file,&size)&&supportedClientPath(file);CloseHandle(process);
 const auto name=std::wstring(file).find_last_of(L"\\/");return ok&&name!=std::wstring::npos&&!_wcsicmp(file+name+1,L"RobloxPlayerBeta.exe");
}
inline bool connectRoblox(HANDLE& handle,DWORD& pid,addr& base,int(*rank)(HANDLE,addr,DWORD)=nullptr){
 bool present=false,versionMatches=false;
 ConnectionState result=ConnectionState::Waiting;
 HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
 if(snapshot==INVALID_HANDLE_VALUE){connectionProcessPresent=false;connectionVersionMatches=false;connectionState=ConnectionState::ModuleUnavailable;return false;}
 PROCESSENTRY32W process{sizeof process};
 std::vector<PROCESSENTRY32W> clients;
 if(Process32FirstW(snapshot,&process))do{
  if(!_wcsicmp(process.szExeFile,L"RobloxPlayerBeta.exe"))clients.push_back(process);
 }while(Process32NextW(snapshot,&process));
 DWORD foregroundPid=0;GetWindowThreadProcessId(GetForegroundWindow(),&foregroundPid);
 wchar_t preferredText[16]{};GetEnvironmentVariableW(L"VANTA_ROBLOX_PID",preferredText,16);
 const DWORD preferredPid=wcstoul(preferredText,nullptr,10);
 HANDLE bestHandle=nullptr;DWORD bestPid=0;addr bestBase=0;std::array<std::uint64_t,4> bestPriority{};
 for(const auto& process:clients){
  if(_wcsicmp(process.szExeFile,L"RobloxPlayerBeta.exe"))continue;
  present=true;
  HANDLE candidate=OpenProcess(PROCESS_VM_READ|PROCESS_QUERY_LIMITED_INFORMATION|SYNCHRONIZE,FALSE,process.th32ProcessID);
  if(!candidate){result=ConnectionState::AccessDenied;continue;}
  wchar_t file[32768]{};DWORD length=32768;
  if(!QueryFullProcessImageNameW(candidate,0,file,&length)){CloseHandle(candidate);result=ConnectionState::ModuleUnavailable;continue;}
  if(!supportedClientPath(file)){CloseHandle(candidate);result=ConnectionState::VersionMismatch;continue;}
  versionMatches=true;
  HANDLE modules=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,process.th32ProcessID);
  MODULEENTRY32W module{sizeof module};bool found=false;
  if(modules!=INVALID_HANDLE_VALUE){
   if(Module32FirstW(modules,&module))do{
    if(!_wcsicmp(module.szModule,L"RobloxPlayerBeta.exe")){found=true;break;}
   }while(Module32NextW(modules,&module));
   CloseHandle(modules);
  }
  if(!found){CloseHandle(candidate);result=ConnectionState::ModuleUnavailable;continue;}
  const addr candidateBase=(addr)module.modBaseAddr;
  auto priority=robloxClientPriority(preferredPid&&process.th32ProcessID==preferredPid,rank?rank(candidate,candidateBase,process.th32ProcessID):0,process.th32ProcessID==foregroundPid,robloxCreated(candidate));
  if(!bestHandle||priority>bestPriority){if(bestHandle)CloseHandle(bestHandle);bestHandle=candidate;bestPid=process.th32ProcessID;bestBase=candidateBase;bestPriority=priority;}
  else CloseHandle(candidate);
 }
 CloseHandle(snapshot);
 if(bestHandle){handle=bestHandle;pid=bestPid;base=bestBase;connectionProcessPresent=true;connectionVersionMatches=true;connectionState=ConnectionState::Loading;return true;}
 connectionProcessPresent=present;connectionVersionMatches=versionMatches;connectionState=result;return false;
}
