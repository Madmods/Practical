#define NOMINMAX
#include "../src/game_module_ui.hpp"
#include <iostream>
#include <iomanip>
void tree(HANDLE h,addr root,int depth,int& budget,std::string prefix){
 if(depth<0||budget<=0)return;
 for(auto o:children(h,root)){
  if(--budget<0)return;
  const auto name=nm(h,o),kind=defense::classOf(h,o);auto label=prefix+name;
  std::cout<<label<<" ["<<kind<<"]";
  if(kind=="NumberValue")std::cout<<" value="<<rd<double>(h,o+Offsets::Misc::Value,NAN);
  if(kind=="IntValue")std::cout<<" value="<<rd<long long>(h,o+Offsets::Misc::Value);
  if(kind=="ObjectValue")std::cout<<" value="<<nm(h,rd<addr>(h,o+Offsets::Misc::Value));
  if(defense::partClass(kind)){auto prim=rd<addr>(h,o+Offsets::BasePart::Primitive);auto p=rd<shooting::Vec>(h,prim+Offsets::Primitive::Position,{NAN,NAN,NAN});auto v=rd<shooting::Vec>(h,prim+Offsets::Primitive::AssemblyLinearVelocity);std::cout<<" pos="<<p.x<<','<<p.y<<','<<p.z<<" vel="<<v.x<<','<<v.y<<','<<v.z;}
  if(kind=="TextLabel"||kind=="TextButton")std::cout<<" text="<<defense::textAt(h,o+0xa30);
  std::cout<<'\n';
  // Do not descend into chats or inspect executable scripts.
  if(name.find("Chat")==std::string::npos&&kind!="LocalScript"&&kind!="ModuleScript"&&kind!="Script")tree(h,o,depth-1,budget,label+"/");
 }
}
int main(int argc,char** argv){
 HANDLE h=nullptr;DWORD pid=0;addr base=0;if(!connectRoblox(h,pid,base))return 2;
 auto s=inspectBasketball(h,base,pid);auto module=gameModules::identify(s);
 if(!module||module->id!=gameModules::Id::FootballFusion3){CloseHandle(h);return 3;}
 auto ws=child(h,s.model,"Workspace"),players=child(h,s.model,"Players"),player=rd<addr>(h,players+Offsets::Player::LocalPlayer),own=rd<addr>(h,player+Offsets::Player::ModelInstance);
 if(argc>1&&std::string(argv[1])=="--throw-trace"){
  auto handle=path(h,own,{"Football","Handle"});if(!handle){CloseHandle(h);return 4;}
  auto camera=rd<addr>(h,ws+Offsets::Workspace::CurrentCamera);auto cp=rd<shooting::Vec>(h,camera+Offsets::Camera::Position);
  std::cout<<"CAMERA "<<cp.x<<','<<cp.y<<','<<cp.z<<'\n';
  const int seconds=argc>2?std::clamp(std::atoi(argv[2]),1,90):20;
  std::set<addr> previousBalls;for(auto o:children(h,ws))if(nm(h,o)=="Football")previousBalls.insert(o);
  bool released=false;std::cout<<"CLOCK "<<std::fixed<<std::setprecision(6)<<now()<<'\n';
  double start=now();while(now()-start<seconds){
   if(!released&&!child(h,own,"Football")){
    addr found=0;bool ambiguous=false;
    for(auto o:children(h,ws))if(!previousBalls.count(o)&&nm(h,o)=="Football"){
     auto by=rd<addr>(h,child(h,o,"ThrownBy")+Offsets::Misc::Value);
     if(by==player||by==own){if(found)ambiguous=true;found=o;}
    }
    if(found&&!ambiguous){handle=found;released=true;std::cout<<"ASSOCIATED "<<handle<<'\n';}
   }
   auto prim=rd<addr>(h,handle+Offsets::BasePart::Primitive);auto p=rd<shooting::Vec>(h,prim+Offsets::Primitive::Position,{NAN,NAN,NAN}),v=rd<shooting::Vec>(h,prim+Offsets::Primitive::AssemblyLinearVelocity);
   auto hp=rd<addr>(h,child(h,own,"Head")+Offsets::BasePart::Primitive);auto head=rd<shooting::Vec>(h,hp+Offsets::Primitive::Position);POINT cursor{};GetCursorPos(&cursor);
   const auto parent=rd<addr>(h,handle+Offsets::Instance::Parent),owner=rd<addr>(h,parent+Offsets::Instance::Parent);
   std::cout<<now()-start<<','<<(child(h,own,"Football")!=0)<<','<<p.x<<','<<p.y<<','<<p.z<<','<<v.x<<','<<v.y<<','<<v.z<<','<<head.x<<','<<head.y<<','<<head.z<<','<<cursor.x<<','<<cursor.y<<','<<defense::classOf(h,parent)<<','<<nm(h,owner)<<'\n';Sleep(20);
  }CloseHandle(h);return 0;
 }
 std::cout<<"place="<<s.placeId<<" universe="<<s.universeId<<" worldGravity="<<rd<float>(h,rd<addr>(h,ws+Offsets::Workspace::World)+Offsets::World::Gravity,NAN)<<" readonlyGravity="<<rd<float>(h,ws+Offsets::Workspace::ReadOnlyGravity,NAN)<<'\n';
 int budget=80;tree(h,ws,0,budget,"Workspace/");
 budget=120;tree(h,own,0,budget,"Character/");
 for(auto o:children(h,own))if(nm(h,o)=="Football"||defense::classOf(h,o)=="Tool"){budget=90;tree(h,o,4,budget,"Equipped/"+nm(h,o)+"/");}
 auto gui=child(h,player,"PlayerGui");budget=80;tree(h,gui,0,budget,"UI/");
 for(auto o:children(h,gui))if(nm(h,o)!="MainGui"&&nm(h,o).find("Chat")==std::string::npos){budget=90;tree(h,o,3,budget,"UI/"+nm(h,o)+"/");}
 budget=90;tree(h,child(h,ws,"npcwr"),2,budget,"NPC/");
 for(auto o:children(h,ws))if(nm(h,o)=="Football"){budget=40;tree(h,o,2,budget,"Ball/");break;}
 for(auto p:children(h,players)){auto rig=rd<addr>(h,p+Offsets::Player::ModelInstance);std::cout<<"Player="<<nm(h,p)<<" rig="<<nm(h,rig)<<" team="<<nm(h,rd<addr>(h,p+Offsets::Player::Team))<<'\n';}
 struct Observation{double t;shooting::Vec p,v;};std::map<addr,Observation> previous;
 for(int i=0;i<150;i++){
  auto objects=children(h,ws);double t=now();
  for(auto ball:objects)if(nm(h,ball)=="Football"){
   auto prim=rd<addr>(h,ball+Offsets::BasePart::Primitive);auto p=rd<shooting::Vec>(h,prim+Offsets::Primitive::Position,{NAN,NAN,NAN}),v=rd<shooting::Vec>(h,prim+Offsets::Primitive::AssemblyLinearVelocity);
   const auto last=previous.find(ball);if(last!=previous.end()){
    const auto& a=last->second;double dt=t-a.t;
    if(dt>.1&&dt<.3&&p.y>10&&a.p.y>10&&shooting::length(v)>20&&shooting::length(v-a.v)<3)std::cout<<"FLIGHT dt="<<dt<<" gravity="<<(a.v.y-v.y)/dt<<" speed="<<sqrt(v.x*v.x+v.y*v.y+v.z*v.z)<<" v="<<v.x<<','<<v.y<<','<<v.z<<'\n';
    if(dt>.12)previous[ball]={t,p,v};
   }else previous[ball]={t,p,v};
  }Sleep(20);
 }
 CloseHandle(h);
}
