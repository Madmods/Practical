#define NOMINMAX
#include "basketball_ui_bridge.hpp"
#include <iostream>
// Read-only scene sampling and simulated planning, never Inputs or hooks.
int main(){
 HANDLE h=nullptr;DWORD pid=0;addr base=0;if(!connectRoblox(h,pid,base))return 1;
 auto root=rd<addr>(h,rd<addr>(h,base+Offsets::FakeDataModel::Pointer)+Offsets::FakeDataModel::RealDataModel);
 auto local=rd<addr>(h,child(h,root,"Players")+Offsets::Player::LocalPlayer);
 gamePid=pid;sceneRoot=root;sceneOwn=path(h,root,{"Workspace","Characters",nm(h,local)});scenePlayer=local;
 wchar_t exe[MAX_PATH]{};GetModuleFileNameW(nullptr,exe,MAX_PATH);auto folder=std::filesystem::path(exe).parent_path();
 basketballUi::load(folder/L".."/L".."/L"bat-timing-settings.json");
 defense::Config config;config.snapshot();defense::Reader reader;defense::Planner planner;defense::Intent intent;intent.focused=intent.activation=true;
 std::ofstream out(folder/L"block-trace.csv");out<<"time,owner,flight,meter,advancing,fill,age,released,shooting,ball_height,body_height,rise,body_gap,own_gap,grounded,facing,jump,reason,block_gate\n";
 int samples=0,meters=0,jumps=0;std::map<std::string,int> gates;double start=now();
 for(int n=0;n<1200;n++){
  auto f=reader.read(now());auto d=planner.step(config,f.scene,intent,f.scene.at);
  for(auto& b:f.scene.balls){auto a=defense::actor(f.scene,b.owner?b.owner:b.previousOwner);if(!a||!a->enemy)continue;++samples;if(a->shotMeterActive)++meters;if(d.jump)++jumps;
   if(a->shotMeterActive)++gates[d.blockReason];
   out<<f.scene.at-start<<','<<b.owner<<','<<b.flight<<','<<a->shotMeterActive<<','<<a->shotMeterAdvancing<<','<<a->shotMeterFill<<','<<a->shotMeterAge<<','<<a->shotReleased<<','<<a->shooting<<','<<b.p.y-f.scene.own.p.y<<','<<b.p.y-a->p.y<<','<<b.heldRiseSpeed<<','<<defense::distance(b.p,a->p)<<','<<defense::distance(b.p,f.scene.own.p)<<','<<f.scene.grounded<<','<<defense::facing(f.scene.own,b.p)<<','<<d.jump<<",\""<<d.reason<<"\",\""<<d.blockReason<<"\"\n";
  }
  Sleep(20);
 }
 std::cout<<"enemy_ball_samples="<<samples<<" meter_samples="<<meters<<" planned_jumps="<<jumps<<" input_requests=0\n";for(auto& gate:gates)std::cout<<gate.first<<"="<<gate.second<<'\n';CloseHandle(h);
}
