#define NOMINMAX
#include "../src/ff3_qb_runtime.hpp"
#include <iostream>
// Read-only planning probe: no hooks, no worker, no synthetic inputs.
int main(int argc,char** argv){HANDLE h=nullptr;DWORD pid=0;addr base=0;if(!connectRoblox(h,pid,base))return 2;
 for(int i=0;i<5;i++){gameModules::publish(inspectBasketball(h,base,pid));Sleep(250);}
 basketballConnection::entered=true;basketballConnection::releaseCurrent=true;
 if(gameModules::router.open(now(),true,true)!=gameModules::Id::FootballFusion3){CloseHandle(h);return 3;}
 ff3Native::Reader reader;ff3Native::enabled=true;
 auto session=gameModules::router.get();auto players=child(h,session.model,"Players"),player=rd<addr>(h,players+Offsets::Player::LocalPlayer);
 auto own=rd<addr>(h,player+Offsets::Player::ModelInstance),tool=child(h,own,"Football"),ball=child(h,tool,"Handle");
 std::cout<<"OWNERSHIP pid="<<pid<<" own="<<own<<" tool="<<tool<<" toolClass="<<defense::classOf(h,tool)<<" toolParent="<<rd<addr>(h,tool+Offsets::Instance::Parent)<<" ball="<<ball<<" ballParent="<<rd<addr>(h,ball+Offsets::Instance::Parent)<<'\n';
 auto mouseService=child(h,session.model,"MouseService");CURSORINFO cursorInfo{sizeof cursorInfo};GetCursorInfo(&cursorInfo);RECT clip{};GetClipCursor(&clip);
 if(argc>1&&std::string(argv[1])=="--power-controls"){
  for(int i=0;i<100;++i){gameModules::publish(inspectBasketball(h,base,pid));auto f=reader.read(0,0,false,true);std::cout<<"CONTROL tick="<<GetTickCount64()<<" held="<<f.held<<" wheel="<<f.wheelVisible<<" power="<<f.gamePower<<" gui="<<f.powerGui<<" focused="<<ff3Native::focused(pid)<<" menu="<<f.menuOpen<<" typing="<<f.textFocus<<" reason="<<f.reason<<'\n';Sleep(50);}CloseHandle(h);return 0;
 }
 if(argc>1&&std::string(argv[1])=="--track-mouse"){
  auto window=ff3Native::clientWindow(pid);POINT origin{};ClientToScreen(window,&origin);
  for(int i=0;i<600;++i){POINT cursor{};GetCursorPos(&cursor);const auto first=rd<shooting::Vec>(h,rd<addr>(h,mouseService+Offsets::MouseService::InputObject)+Offsets::MouseService::MousePosition,{NAN,NAN,NAN}),second=rd<shooting::Vec>(h,rd<addr>(h,mouseService+Offsets::MouseService::InputObject2)+Offsets::MouseService::MousePosition,{NAN,NAN,NAN});
   std::cout<<i<<','<<ff3Native::focused(pid)<<','<<cursor.x-origin.x<<','<<cursor.y-origin.y<<','<<first.x<<','<<first.y<<','<<second.x<<','<<second.y<<'\n';Sleep(50);
  }CloseHandle(h);return 0;
 }
 std::cout<<"CURSOR flags="<<cursorInfo.flags<<" pos="<<cursorInfo.ptScreenPos.x<<','<<cursorInfo.ptScreenPos.y<<" clip="<<clip.left<<','<<clip.top<<','<<clip.right<<','<<clip.bottom<<" cameraMode="<<rd<int>(h,player+Offsets::Player::CameraMode,-1)<<" mouseService="<<mouseService<<'\n';
 for(auto offset:{Offsets::MouseService::InputObject,Offsets::MouseService::InputObject2}){auto input=rd<addr>(h,mouseService+offset);auto p=rd<shooting::Vec>(h,input+Offsets::MouseService::MousePosition,{NAN,NAN,NAN});std::cout<<"GAME_MOUSE offset="<<offset<<" object="<<input<<" pos="<<p.x<<','<<p.y<<','<<p.z<<'\n';}
 for(auto gui:children(h,child(h,player,"PlayerGui")))if(nm(h,gui)=="BallGui"){
  auto frame=child(h,gui,"Frame0"),display=child(h,frame,"Disp");
  std::cout<<"UI_OWNERSHIP gui="<<gui<<" order="<<rd<int>(h,gui+Offsets::ScreenGui::DisplayOrder)<<" frame="<<frame<<" frameParent="<<rd<addr>(h,frame+Offsets::Instance::Parent)<<" display="<<display<<" displayParent="<<rd<addr>(h,display+Offsets::Instance::Parent)<<'\n';
  std::cout<<"POWER_UI enabled="<<int(rd<unsigned char>(h,gui+Offsets::GuiObject::ScreenGui_Enabled,255))<<" visible="<<int(rd<unsigned char>(h,frame+Offsets::GuiObject::Visible,255))<<" display="<<defense::textAt(h,display+Offsets::GuiObject::Text)<<'\n';
 }
 const bool targets=argc>1&&std::string(argv[1])=="--targets";
 for(int i=0;i<10;i++){
  gameModules::publish(inspectBasketball(h,base,pid));
  auto start=now();auto f=reader.read();
  if(targets)for(const auto& a:reader.actors)if(a.npc){auto candidate=reader.read(a.rig);if(candidate.ready){f=candidate;break;}}
  std::cout<<"reason="<<f.reason<<" held="<<f.held<<" wheel="<<f.wheelVisible<<" gamePower="<<f.gamePower<<" power="<<f.power<<" gravity="<<f.gravity<<" ping="<<f.ping<<" target="<<f.name<<" angle="<<f.solution.angle<<" flight="<<f.solution.time<<" aim="<<f.aim.x<<','<<f.aim.y<<" receiver="<<f.receiver.x<<','<<f.receiver.y<<" readMs="<<(now()-start)*1000<<'\n';Sleep(100);
 }
 CloseHandle(h);
}
