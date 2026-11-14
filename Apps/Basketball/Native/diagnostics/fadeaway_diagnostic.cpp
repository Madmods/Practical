#define NOMINMAX
#include "basketball_ui_bridge.hpp"
#include <iostream>
// Bounded read-only trace. Production reader/planner, simulated held activation;
// no engine threads, input hooks, Inputs adapter or real key requests.
int main(){
 HANDLE h=nullptr;DWORD pid=0;addr base=0;if(!connectRoblox(h,pid,base))return 1;
 auto root=rd<addr>(h,rd<addr>(h,base+Offsets::FakeDataModel::Pointer)+Offsets::FakeDataModel::RealDataModel);
 auto local=rd<addr>(h,child(h,root,"Players")+Offsets::Player::LocalPlayer);
 auto own=path(h,root,{"Workspace","Characters",nm(h,local)});
 gamePid=pid;sceneRoot=root;sceneOwn=own;scenePlayer=local;
 wchar_t exe[MAX_PATH]{};GetModuleFileNameW(nullptr,exe,MAX_PATH);auto folder=std::filesystem::path(exe).parent_path();
 basketballUi::load(folder/L".."/L".."/L"bat-timing-settings.json");
 defense::Config config;config.snapshot();config.values[defense::Guard]=1;
 defense::Reader reader;defense::Planner planner;defense::Intent intent;intent.focused=intent.activation=true;
 std::ofstream out(folder/L"fadeaway-trace.csv");out<<"time,handler,x,z,vx,vz,own_x,own_z,back_speed,separation,meter,shooting,gather,fade,moving,reason\n";
 int held=0,fade=0,shots=0,anchors=0;float maxAnchorError=0;double start=now();
 for(int n=0;n<600;n++){
  auto f=reader.read(now());auto d=planner.step(config,f.scene,intent,f.scene.at);auto a=defense::actor(f.scene,d.target);
  if(a){++held;if(a->shotMeterActive||a->shooting)++shots;if(planner.fade.until>=f.scene.at)++fade;
   auto axis=defense::unit(defense::flat(a->goal-a->p));
   if(d.stance&&!d.obstacle&&d.mode!=defense::Denying&&d.response!="Giving the backdown space"){
    auto offset=defense::flat(d.destination-a->p);maxAnchorError=std::max(maxAnchorError,defense::length(offset-axis*defense::dot(offset,axis)));++anchors;
   }
   out<<f.scene.at-start<<",1,"<<a->p.x<<","<<a->p.z<<","<<a->v.x<<","<<a->v.z<<","<<f.scene.own.p.x<<","<<f.scene.own.p.z<<","<<-defense::dot(defense::flat(a->v),axis)<<","<<defense::distance(a->p,f.scene.own.p)<<","<<a->shotMeterActive<<","<<a->shooting<<","<<(planner.gather.since>=0)<<","<<(planner.fade.until>=f.scene.at)<<","<<d.moving<<",\""<<d.reason<<"\"\n";
  }else out<<f.scene.at-start<<",0,0,0,0,0,0,0,0,0,0,0,0,0,"<<d.moving<<",\""<<d.reason<<"\"\n";
  Sleep(20);
 }
 std::cout<<"samples=600 handler_samples="<<held<<" shot_cue_samples="<<shots<<" fade_samples="<<fade<<" basket_anchor_samples="<<anchors<<" max_basket_line_error="<<maxAnchorError<<" input_requests=0\n";CloseHandle(h);
}
