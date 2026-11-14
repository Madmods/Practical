#pragma once
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
inline bool connectRoblox(HANDLE& handle,DWORD& pid,addr& base){
 bool present=false,versionMatches=false;
 ConnectionState result=ConnectionState::Waiting;
 HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
 if(snapshot==INVALID_HANDLE_VALUE){connectionProcessPresent=false;connectionVersionMatches=false;connectionState=ConnectionState::ModuleUnavailable;return false;}
 PROCESSENTRY32W process{sizeof process};
 if(Process32FirstW(snapshot,&process))do{
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
  handle=candidate;pid=process.th32ProcessID;base=(addr)module.modBaseAddr;
  CloseHandle(snapshot);connectionProcessPresent=true;connectionVersionMatches=true;connectionState=ConnectionState::Loading;return true;
 }while(Process32NextW(snapshot,&process));
 CloseHandle(snapshot);connectionProcessPresent=present;connectionVersionMatches=versionMatches;connectionState=result;return false;
}
