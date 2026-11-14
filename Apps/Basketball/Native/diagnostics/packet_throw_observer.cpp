#define NOMINMAX
#include "../src/game_module_ui.hpp"
#include <iostream>
#include <set>

long long utcMicros() {
 FILETIME ft{}; GetSystemTimePreciseAsFileTime(&ft);
 ULARGE_INTEGER ticks{}; ticks.LowPart=ft.dwLowDateTime; ticks.HighPart=ft.dwHighDateTime;
 return static_cast<long long>((ticks.QuadPart-116444736000000000ULL)/10);
}
int main(int argc,char** argv) {
 const int seconds=argc>1?std::clamp(std::atoi(argv[1]),5,300):120;
 HANDLE h=nullptr;DWORD pid=0;addr base=0;
 if(!connectRoblox(h,pid,base)) {std::cerr<<"Cannot open supported Roblox client read-only\n";return 2;}
 auto s=inspectBasketball(h,base,pid);auto module=gameModules::identify(s);
 if(!module||module->id!=gameModules::Id::FootballFusion3) {std::cerr<<"Not in verified Football Fusion 3\n";CloseHandle(h);return 3;}
 auto ws=child(h,s.model,"Workspace"),players=child(h,s.model,"Players");
 auto player=rd<addr>(h,players+Offsets::Player::LocalPlayer);
 std::set<addr> seen;for(auto o:children(h,ws))if(nm(h,o)=="Football")seen.insert(o);
 std::cout<<"unix_us,event,held,object,x,y,z,vx,vy,vz\n"<<std::flush;
 bool priorHeld=false,priorClick=false,first=true;
 auto start=now(); double lastScan=0;
 auto emit=[&](const char* event,bool held,addr object){
  auto prim=object?rd<addr>(h,object+Offsets::BasePart::Primitive):0;
  auto p=prim?rd<shooting::Vec>(h,prim+Offsets::Primitive::Position,{NAN,NAN,NAN}):shooting::Vec{NAN,NAN,NAN};
  auto v=prim?rd<shooting::Vec>(h,prim+Offsets::Primitive::AssemblyLinearVelocity):shooting::Vec{NAN,NAN,NAN};
  std::cout<<utcMicros()<<','<<event<<','<<held<<','<<object<<','<<p.x<<','<<p.y<<','<<p.z<<','<<v.x<<','<<v.y<<','<<v.z<<'\n'<<std::flush;
 };
 while(now()-start<seconds&&WaitForSingleObject(h,0)==WAIT_TIMEOUT) {
  auto own=rd<addr>(h,player+Offsets::Player::ModelInstance),tool=child(h,own,"Football");
  bool held=tool!=0; if(held&&!priorHeld)seen.erase(path(h,own,{"Football","Handle"})); DWORD foregroundPid=0;GetWindowThreadProcessId(GetForegroundWindow(),&foregroundPid);
  bool click=foregroundPid==pid&&(GetAsyncKeyState(VK_LBUTTON)&0x8000);
  if(first||held!=priorHeld)emit(first?"initial":held?"equipped":"released",held,path(h,own,{"Football","Handle"}));
  if(click&&!priorClick)emit("left_down",held,path(h,own,{"Football","Handle"}));
  if(!click&&priorClick)emit("left_up",held,0);
  if(now()-lastScan>.02){
   lastScan=now();
   for(auto o:children(h,ws))if(nm(h,o)=="Football"&&!seen.count(o)){
    auto by=rd<addr>(h,child(h,o,"ThrownBy")+Offsets::Misc::Value);
    if(by&&(by==player||(own&&by==own))){seen.insert(o);emit("own_thrown_ball",held,o);}
   }
  }
  first=false;priorHeld=held;priorClick=click;Sleep(5);
 }
 emit("finished",priorHeld,0);CloseHandle(h);return 0;
}


