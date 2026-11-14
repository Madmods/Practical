#define NOMINMAX
#include "engine.hpp"
#include <iostream>
#include <sstream>
// Read-only local character meter/feedback diagnostic. No input hooks.
int main(int argc,char**){
 HANDLE h=nullptr;DWORD pid=0;addr base=0;if(!connectRoblox(h,pid,base))return 1;
 const auto connection=inspectBasketball(h,base,pid);if(!basketballConnection::targetGame(connection)){CloseHandle(h);return 2;}
 std::string previous;const double start=now();
 do{
  auto root=rd<addr>(h,rd<addr>(h,base+Offsets::FakeDataModel::Pointer)+Offsets::FakeDataModel::RealDataModel);
  auto local=rd<addr>(h,child(h,root,"Players")+Offsets::Player::LocalPlayer);
  auto own=path(h,root,{"Workspace","Characters",nm(h,local)}),hrp=child(h,own,"HumanoidRootPart");
  std::ostringstream out;
  const auto feedback=child(h,hrp,"ShotFeedback"),timing=child(h,feedback,"ReleaseTiming"),contest=child(h,feedback,"Contest");
  auto meter=path(h,hrp,{"BatMeter","Meter_Fill","FillGradient"});
  out<<"release="<<defense::textAt(h,timing+0xa30)<<" contest="<<defense::textAt(h,contest+0xa30)
     <<" textAlpha="<<rd<float>(h,timing+0xb0c,NAN)<<" contestAlpha="<<rd<float>(h,contest+0xb0c,NAN)
     <<" fill="<<-rd<float>(h,meter+0x12c,NAN)<<'\n';
  auto current=out.str();if(current!=previous){std::cout<<"at="<<now()-start<<"\n"<<current<<std::flush;previous=current;}
  if(argc==1)break;Sleep(100);
 }while(now()-start<30&&WaitForSingleObject(h,0)==WAIT_TIMEOUT);
 CloseHandle(h);return 0;
}
