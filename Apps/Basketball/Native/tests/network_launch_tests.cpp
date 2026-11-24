#define NOMINMAX
#include "../src/network_launch.hpp"
#include <fstream>
#include <iostream>

bool elevated(){
 HANDLE token=nullptr;if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&token))return false;
 TOKEN_ELEVATION value{};DWORD size=0;const bool ok=GetTokenInformation(token,TokenElevation,&value,sizeof value,&size);CloseHandle(token);
 return ok&&value.TokenIsElevated;
}
int wmain(int argc,wchar_t** argv){
 if(argc==3&&std::wstring(argv[1])==L"--child-probe"){
  std::ofstream out{std::filesystem::path(argv[2])};out<<elevated()<<' '<<(GetConsoleWindow()==nullptr);return out?0:2;
 }
 unsigned checks=0;auto check=[&](bool ok){++checks;if(!ok){std::cerr<<"Failed launch check "<<checks<<'\n';std::exit(1);}};
 wchar_t path[32768]{};check(GetModuleFileNameW(nullptr,path,32768)>0);
 const auto folder=std::filesystem::temp_directory_path()/(L"Vanta launch test "+std::to_wstring(GetCurrentProcessId()));std::filesystem::create_directories(folder);
 const auto marker=folder/L"token result.txt";
 HANDLE child=vantaNetwork::startHidden(path,L"--child-probe \""+marker.wstring()+L"\"");check(child!=nullptr);
 check(WaitForSingleObject(child,10000)==WAIT_OBJECT_0);DWORD code=1;check(GetExitCodeProcess(child,&code)&&code==0);CloseHandle(child);
 int childElevated=-1,hidden=-1;std::ifstream in(marker);in>>childElevated>>hidden;check(bool(in));in.close();
 check(childElevated==int(elevated()));check(hidden==1);
 check(vantaNetwork::startHidden(folder/L"missing helper.exe",L"")==nullptr);
 std::filesystem::remove(marker);std::filesystem::remove(folder);
 std::cout<<checks<<" hidden helper launch checks passed; child inherited parent elevation, no UAC requested\n";
}
