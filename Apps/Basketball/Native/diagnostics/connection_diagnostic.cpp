#define NOMINMAX
#include "engine.hpp"
#include <iostream>
int main(){
 std::wstring version(Offsets::ClientVersion.begin(),Offsets::ClientVersion.end());
 if(!supportedClientPath(L"C:\\Roblox\\Versions\\"+version+L"\\RobloxPlayerBeta.exe")||
    supportedClientPath(L"C:\\Roblox\\Versions\\version-old\\RobloxPlayerBeta.exe")||
    supportedClientPath(L"C:\\Roblox\\Versions\\"+version+L"-extra\\RobloxPlayerBeta.exe"))return 2;
 std::thread worker(engine);
 for(int n=0;n<30&&!linked;n++)Sleep(100);
 Sleep(600);
 HANDLE h=OpenProcess(PROCESS_VM_READ,FALSE,gamePid.load());
 auto own=sceneOwn.load(),root=sceneRoot.load();
 auto meter=path(h,own,{"HumanoidRootPart","BatMeter","Meter_Fill","FillGradient"});
 auto humanoid=child(h,own,"Humanoid"),workspace=child(h,root,"Workspace");
 std::wcout<<L"connection="<<connectionLabel()<<L" pid="<<gamePid.load()<<L" version="<<version<<L"\n";
 std::cout<<"player="<<nm(h,scenePlayer.load())<<" character="<<nm(h,own)
  <<" health="<<rd<float>(h,humanoid+Offsets::Humanoid::Health)
  <<" walkSpeed="<<rd<float>(h,humanoid+Offsets::Humanoid::Walkspeed)
  <<" gravity="<<rd<float>(h,workspace+Offsets::Workspace::ReadOnlyGravity)
  <<" meter="<<nm(h,meter)<<" meterClass="<<defense::classOf(h,meter)
  <<" meterY="<<rd<float>(h,meter+0x12c,NAN)
  <<" livePing="<<livePing.load()<<" pingMs="<<pingMs.load()<<"\n";
 bool ok=linked&&own&&root;
 if(h)CloseHandle(h);running=false;worker.join();return ok?0:1;
}
