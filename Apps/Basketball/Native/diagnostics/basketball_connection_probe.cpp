#define NOMINMAX
#include "../src/game_module_ui.hpp"
#include <iostream>
int main(){
 winrt::init_apartment(winrt::apartment_type::single_threaded);
 HANDLE h=nullptr;DWORD pid=0;addr base=0;
 connectRoblox(h,pid,base);
 for(int i=0;i<5;++i){gameModules::publish(inspectBasketball(h,base,pid));if(i<4)Sleep(250);}
 wchar_t exe[MAX_PATH]{};GetModuleFileNameW(nullptr,exe,MAX_PATH);const auto dir=std::filesystem::path(exe).parent_path();
 auto release=vantaRelease::verify(dir/L"vanta-release.json",vantaRelease::sha256(dir/L"Vanta-Basketball.exe"));
 auto s=gameModules::router.get();auto report=gameModuleUi::connection(s,false,release);
 auto observations=basketballUi::object();
 const auto root=rd<addr>(h,rd<addr>(h,base+Offsets::FakeDataModel::Pointer)+Offsets::FakeDataModel::RealDataModel);
 observations.o[L"rawLoadedFlag"]=basketballUi::number(rd<unsigned char>(h,root+Offsets::DataModel::GameLoaded));
 observations.o[L"workspacePresent"]=basketballUi::boolean(child(h,root,"Workspace")!=0);
 observations.o[L"playersPresent"]=basketballUi::boolean(child(h,root,"Players")!=0);
 report.o[L"observations"]=observations;
 std::cout<<winrt::to_string(json::dump(report))<<'\n';
 if(h)CloseHandle(h);return gameModules::ready(s,now())&&release.current?0:2;
}
