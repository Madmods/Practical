#define NOMINMAX
#include "../src/ff3_qb_runtime.hpp"
#include <iostream>
// Read-only field geometry discovery; no hooks, input, or gameplay workers.
int main(int argc,char**){
 HANDLE h=nullptr;DWORD pid=0;addr base=0;if(!connectRoblox(h,pid,base))return 2;
 auto session=inspectBasketball(h,base,pid);auto module=gameModules::identify(session);
 if(!module||module->id!=gameModules::Id::FootballFusion3){CloseHandle(h);return 3;}
 auto ws=child(h,session.model,"Workspace");
 ff3Native::FieldReader fieldReader;auto field=fieldReader.read(h,ws,now());std::cout<<"FIELD known="<<field.valid()<<" center="<<field.center.x<<','<<field.center.y<<','<<field.center.z<<" half="<<field.halfWidth<<','<<field.halfLength<<'\n';
 if(argc>1){
  auto players=child(h,session.model,"Players"),player=rd<addr>(h,players+Offsets::Player::LocalPlayer),rig=rd<addr>(h,player+Offsets::Player::ModelInstance);
  auto list=[&](addr parent){for(auto part:children(h,parent)){auto cls=defense::classOf(h,part);auto size=ff3Native::partSize(h,part);std::cout<<nm(h,parent)<<'/'<<nm(h,part)<<" ["<<cls<<"] size="<<size.x<<','<<size.y<<','<<size.z<<'\n';}};
  list(rig);for(auto part:children(h,rig))if(defense::partClass(defense::classOf(h,part)))list(part);
  ff3Native::Reader balls;ff3Native::Frame sample;sample.at=now();balls.collectBalls(h,ws,sample,true);
  for(auto part:balls.flightParts){std::cout<<"BALL "<<part<<" parent="<<nm(h,rd<addr>(h,part+Offsets::Instance::Parent))<<'\n';list(part);}
  CloseHandle(h);return 0;
 }
 struct Node{addr id;int depth;std::string path;};std::vector<Node> queue{{ws,0,"Workspace"}};unsigned budget=0;
 for(size_t i=0;i<queue.size()&&budget<12000;i++){
  auto node=queue[i];
  for(auto o:children(h,node.id)){
   if(++budget>12000)break;auto name=nm(h,o),cls=defense::classOf(h,o),path=node.path+"/"+name;
   if(defense::partClass(cls)){
    auto size=ff3Native::partSize(h,o);
    if(size.x>60||size.z>60||name.find("Bound")!=std::string::npos){
     auto p=ff3Native::position(h,o);auto prim=rd<addr>(h,o+Offsets::BasePart::Primitive);auto r=rd<defense::Matrix3>(h,prim+Offsets::Primitive::Rotation);
     std::cout<<path<<" ["<<cls<<"] pos="<<p.x<<','<<p.y<<','<<p.z<<" size="<<size.x<<','<<size.y<<','<<size.z<<" rot=";
     for(auto v:r.m)std::cout<<v<<',';std::cout<<'\n';
    }
   }
   if(node.depth<6&&(cls=="Folder"||cls=="Model")&&!child(h,o,"Humanoid"))queue.push_back({o,node.depth+1,path});
  }
 }
 std::cout<<"scanned="<<budget<<" place="<<session.placeId<<'\n';CloseHandle(h);
}
