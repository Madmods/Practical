#define NOMINMAX
#include "../src/ff3_qb_runtime.hpp"
#include <iostream>
// Read-only properties and movement observations. No hooks or input injection.
int main(){
 HANDLE h=nullptr;DWORD pid=0;addr base=0;if(!connectRoblox(h,pid,base))return 2;
 auto session=inspectBasketball(h,base,pid);auto module=gameModules::identify(session);if(!module||module->id!=gameModules::Id::FootballFusion3){CloseHandle(h);return 3;}
 auto ws=child(h,session.model,"Workspace"),players=child(h,session.model,"Players");
 std::vector<addr> rigs;for(auto p:children(h,players)){auto rig=rd<addr>(h,p+Offsets::Player::ModelInstance);if(rig)rigs.push_back(rig);}
 for(auto group:children(h,child(h,ws,"npcwr")))for(auto rig:children(h,group))if(child(h,rig,"WR"))rigs.push_back(rig);
 const double gravity=rd<float>(h,ws+Offsets::Workspace::ReadOnlyGravity,NAN);std::cout<<"gravity="<<gravity<<" actors="<<rigs.size()<<std::endl;
 std::vector<ff3qb::Vec> last(rigs.size());std::vector<double> maxSpeed(rigs.size()),maxUp(rigs.size()),maxDisplacement(rigs.size());
 for(size_t i=0;i<rigs.size();++i){auto hum=child(h,rigs[i],"Humanoid");auto shape=ff3Native::readCatchGeometry(h,rigs[i],.6);auto jp=rd<float>(h,hum+Offsets::Humanoid::JumpPower,NAN);bool use=rd<bool>(h,hum+Offsets::Humanoid::UseJumpPower);
  std::cout<<"actor="<<i<<" walk="<<rd<float>(h,hum+Offsets::Humanoid::Walkspeed,NAN)<<" jumpHeight="<<rd<float>(h,hum+Offsets::Humanoid::JumpHeight,NAN)<<" jumpPower="<<jp<<" usePower="<<use<<" effectiveJump="<<(use?jp*jp/(2*gravity):rd<float>(h,hum+Offsets::Humanoid::JumpHeight,NAN))<<" catchRadius="<<shape.radius<<" above="<<shape.above<<" below="<<shape.below<<" measured="<<shape.measured<<std::endl;
  last[i]=ff3Native::position(h,child(h,rigs[i],"HumanoidRootPart"));
 }
 double prior=now();for(int sample=0;sample<200;++sample){Sleep(100);double time=now(),dt=time-prior;prior=time;for(size_t i=0;i<rigs.size();++i){auto root=child(h,rigs[i],"HumanoidRootPart"),hum=child(h,rigs[i],"Humanoid");auto v=ff3Native::velocity(h,root),p=ff3Native::position(h,root);if(ff3qb::finite(v)){maxSpeed[i]=std::max(maxSpeed[i],std::hypot(v.x,v.z));maxUp[i]=std::max(maxUp[i],v.y);}if(ff3qb::finite(p)&&ff3qb::finite(last[i])&&dt>.05&&dt<.3){auto d=p-last[i];double speed=std::hypot(d.x,d.z)/dt;if(speed<100)maxDisplacement[i]=std::max(maxDisplacement[i],speed);}last[i]=p;
  for(auto o:children(h,root))if(defense::classOf(h,o)=="BodyVelocity"){if(sample%10==0)std::cout<<"BodyVelocity active actor="<<i<<std::endl;}
 }}
 for(size_t i=0;i<rigs.size();++i)std::cout<<"observed actor="<<i<<" maxHorizontal="<<maxSpeed[i]<<" maxUp="<<maxUp[i]<<" positionSpeed="<<maxDisplacement[i]<<std::endl;
 CloseHandle(h);
}
