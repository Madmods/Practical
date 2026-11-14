#define NOMINMAX
#include "engine.hpp"
#include <iostream>
// Bounded, read-only observation. No engine workers, hooks, or input requests.
int main(){
 HANDLE h=nullptr;DWORD pid=0;addr base=0;
 if(!connectRoblox(h,pid,base)){std::cout<<"Supported process unavailable\n";return 1;}
 auto root=rd<addr>(h,rd<addr>(h,base+Offsets::FakeDataModel::Pointer)+Offsets::FakeDataModel::RealDataModel);
 auto local=rd<addr>(h,child(h,root,"Players")+Offsets::Player::LocalPlayer);
 auto own=path(h,root,{"Workspace","Characters",nm(h,local)});
 gamePid=pid;sceneRoot=root;sceneOwn=own;scenePlayer=local;
 defense::options[defense::PracticeAI].value=1;
 defense::Reader reader;
 struct Sample{int frames=0,meters=0,actions=0,held=0,active=0,closeouts=0;float low=2,high=-2;std::string name;};
 std::map<addr,Sample> samples;
 defense::Planner planner;defense::Config config;config.values[defense::Guard]=1;config.values[defense::Reaction]=0;config.values[defense::Block]=1;
 int grounded=0,releases=0,blockRequests=0,windupRequests=0,releaseRequests=0,logged=0,heldLogged=0;
 defense::Intent intent;intent.focused=intent.activation=true;
 for(int tick=0;tick<300;tick++){
  auto f=reader.read(now());
  auto decision=planner.step(config,f.scene,intent,f.scene.at);
  if(f.scene.grounded)++grounded;if(decision.jump)++blockRequests;
  if(decision.jump)for(auto& b:f.scene.balls)if(b.id==decision.ball){if(b.owner)++windupRequests;else ++releaseRequests;std::cout<<"jump_request owner_present="<<bool(b.owner)<<" meter_observed="<<b.meterObserved<<" reason="<<decision.reason<<"\n";}
  for(auto& b:f.scene.balls)if(!b.owner&&b.previousOwner&&f.scene.at-b.releasedAt<.25){
   ++releases;if(logged++<18)std::cout<<"release age="<<f.scene.at-b.releasedAt<<" height="<<b.p.y-f.scene.own.p.y<<" distance="<<defense::distance(b.p,f.scene.own.p)<<" vy="<<b.v.y<<" velocity_valid="<<b.velocityValid<<" shot="<<b.shot<<" grounded="<<f.scene.grounded<<" facing="<<defense::facing(f.scene.own,b.p)<<" jump="<<decision.jump<<"\n";
  }
  for(auto&a:f.scene.actors)if(a.enemy){auto& s=samples[a.id];s.name=a.name;++s.frames;if(a.shooting)++s.actions;
   if(defense::heldBall(f.scene,a.id))++s.held;if(a.shotMeterActive)++s.active;
   if(decision.target==a.id&&decision.reason=="Closing active shot meter")++s.closeouts;
   auto meter=path(h,a.id,{"HumanoidRootPart","BatMeter","Meter_Fill","FillGradient"});
   if(meter&&defense::classOf(h,meter)=="UIGradient"){
    float fill=-rd<float>(h,meter+0x12c,NAN);
    if(std::isfinite(fill)&&fill>=0&&fill<=1.05f){++s.meters;s.low=std::min(s.low,fill);s.high=std::max(s.high,fill);
     if(a.shotMeterActive&&heldLogged++<18)if(auto b=defense::heldBall(f.scene,a.id))std::cout<<"windup fill="<<fill<<" height="<<b->p.y-f.scene.own.p.y<<" rise="<<b->heldRiseSpeed<<" advancing="<<a.shotMeterAdvancing<<" distance="<<defense::distance(b->p,f.scene.own.p)<<" body_distance="<<defense::distance(b->p,a.p)<<"\n";
    }
   }
  }
  Sleep(20);
 }
 std::cout<<"Nearby opponents sampled="<<samples.size()<<"\n";
 std::cout<<"grounded_frames="<<grounded<<" recent_release_frames="<<releases<<" planned_jump_requests="<<blockRequests<<" windup_requests="<<windupRequests<<" release_requests="<<releaseRequests<<"\n";
 for(auto& pair:samples){auto& s=pair.second;std::cout<<"actor="<<s.name<<" frames="<<s.frames<<" valid_meter_frames="<<s.meters<<" shooting_action_frames="<<s.actions<<" possession_frames="<<s.held<<" active_meter_frames="<<s.active<<" planned_meter_closeouts="<<s.closeouts;if(s.meters)std::cout<<" meter_min="<<s.low<<" meter_max="<<s.high;std::cout<<"\n";}
 CloseHandle(h);return 0;
}
