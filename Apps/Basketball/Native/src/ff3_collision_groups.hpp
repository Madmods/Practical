#pragma once
#include "ff3_collision_logic.hpp"
namespace ff3Play {
// Verified against the current player build and an external-only Studio physics test.
inline constexpr addr physicsGroupOffset=0x1bd;
static_assert(physicsGroupOffset+1==Offsets::Primitive::Flags,"Physics group layout requires revalidation");
inline bool collisionName(HANDLE h,addr at,std::string& value){
 auto size=rd<addr>(h,at+16,~addr{}),capacity=rd<addr>(h,at+24,~addr{});
 if(size>100||capacity<size||capacity>4096)return false;
 if(!size){value.clear();return true;}if(capacity>=16)at=rd<addr>(h,at);
 char bytes[101]{};SIZE_T read=0;if(!ReadProcessMemory(h,(void*)at,bytes,size,&read)||read!=size)return false;value.assign(bytes,size);return true;
}
struct NativeCollisionPlan {
 struct CharacterPart {addr id=0,parent=0,primitive=0;unsigned group=0;bool catchPart=false;};
 Scene scene;addr workspace=0,world=0,records=0;size_t worldOffset=0;CollisionRules rules;
 std::uint32_t bodies=0,catches=0;std::vector<CharacterPart> parts;bool loaded=false,complete=false;std::string failure;
 bool registry(HANDLE h,addr ws){
  for(size_t offset:{size_t(0x3f8),size_t(0x420)}){
   auto nativeWorld=rd<addr>(h,ws+offset),data=rd<addr>(h,nativeWorld+0x3f8),control=rd<addr>(h,nativeWorld+0x400);
   if(!data||data!=control+24)continue;CollisionRules next;bool ok=true;unsigned named=0;
   for(unsigned id=0;id<32;++id){if(!collisionName(h,data+id*32,next.names[id])){ok=false;break;}if(!next.names[id].empty())++named;}
   SIZE_T read=0;if(!ok||!named||rd<unsigned>(h,data+0x498)!=named||!ReadProcessMemory(h,(void*)(data+0x400),next.masks.data(),sizeof next.masks,&read)||read!=sizeof next.masks||!next.valid())continue;
   workspace=ws;world=nativeWorld;records=data;worldOffset=offset;rules=std::move(next);return true;
  }return false;
 }
 bool sample(HANDLE h,const Scene& s,addr ws){
  loaded=complete=false;failure.clear();parts.clear();bodies=catches=0;scene=s;
  if(!h||!observing(s,now())||!ws||!registry(h,ws))return false;
  auto hands=ff3Native::catchParts(h,s.own);if(!hands.first||!hands.second||hands.first==hands.second)return false;
  for(auto id:{hands.first,hands.second}){
   auto primitive=rd<addr>(h,id+Offsets::BasePart::Primitive);auto flags=rd<unsigned char>(h,primitive+Offsets::Primitive::Flags,255),group=rd<unsigned char>(h,primitive+physicsGroupOffset,255);
   if(rd<addr>(h,id+Offsets::Instance::Parent)!=s.own||!primitive||flags==255||(flags&(Offsets::PrimitiveFlags::CanCollide|Offsets::PrimitiveFlags::Anchored))||group>=32)return false;
   catches|=1u<<group;parts.push_back({id,s.own,primitive,group,true});
  }
  std::vector<addr> todo=children(h,s.own);unsigned visited=0;
  while(!todo.empty()&&visited++<1024&&now()-s.at<.10){auto id=todo.back();todo.pop_back();if(id==hands.first||id==hands.second||instanceClassIs(h,id,"Tool"))continue;
   const bool physical=instanceClassIs(h,id,"Part")||instanceClassIs(h,id,"MeshPart")||instanceClassIs(h,id,"UnionOperation");
   if(physical){
    auto primitive=rd<addr>(h,id+Offsets::BasePart::Primitive);auto flags=rd<unsigned char>(h,primitive+Offsets::Primitive::Flags,255),group=rd<unsigned char>(h,primitive+physicsGroupOffset,255);
    if(!primitive||flags==255||group>=32){failure="Unreadable body: "+nm(h,id)+" flags="+std::to_string(flags)+" group="+std::to_string(group);return false;}if(flags&Offsets::PrimitiveFlags::CanCollide){bodies|=1u<<group;parts.push_back({id,rd<addr>(h,id+Offsets::Instance::Parent),primitive,group,false});}
   }if(physical||instanceClassIs(h,id,"Model")||instanceClassIs(h,id,"Folder")||instanceClassIs(h,id,"Accessory")||instanceClassIs(h,id,"Hat"))for(auto c:children(h,id))todo.push_back(c);
  }
  complete=todo.empty()&&now()-s.at<.10;loaded=complete&&bodies&&catches&&!(bodies&catches);if(!loaded)failure=complete&&(bodies&catches)?"Body and catch parts share a collision group":"Character scan visited="+std::to_string(visited)+" remaining="+std::to_string(todo.size());return loaded;
 }
 int choose(const BallPart& ball)const{return loaded?rules.choose(ball.physicsGroup,bodies,catches):-1;}
 bool loose(HANDLE h,addr ball)const{
  auto node=rd<addr>(h,ball+Offsets::Instance::Parent);
  for(int depth=0;depth<5&&node;++depth){if(node==workspace)return true;if(instanceClassIs(h,node,"Tool")||child(h,node,"Humanoid"))return false;node=rd<addr>(h,node+Offsets::Instance::Parent);}return false;
 }
 bool protects(HANDLE h,const BallPart& ball,unsigned group,bool applied=true)const{
  if(!loaded||group>=32||!observing(scene,now())||!ff3Native::focused(scene.pid)||rd<addr>(h,scene.player+Offsets::Player::ModelInstance)!=scene.own||!ball.part.ball||choose(ball)!=int(group)||!loose(h,ball.part.id))return false;
  if(rd<addr>(h,workspace+worldOffset)!=world||rd<addr>(h,world+0x3f8)!=records||rd<std::uint32_t>(h,records+0x400+group*4)!=rules.masks[group])return false;
  std::string name;if(!collisionName(h,records+group*32,name)||name!=rules.names[group])return false;
  for(const auto& p:parts){auto flags=rd<unsigned char>(h,p.primitive+Offsets::Primitive::Flags,255);
   if(rd<addr>(h,p.id+Offsets::Instance::Parent)!=p.parent||rd<addr>(h,p.id+Offsets::BasePart::Primitive)!=p.primitive||rd<unsigned char>(h,p.primitive+physicsGroupOffset,255)!=p.group||flags==255)return false;
   if(p.catchPart){if(flags&(Offsets::PrimitiveFlags::CanCollide|Offsets::PrimitiveFlags::Anchored))return false;}else if(!(flags&Offsets::PrimitiveFlags::CanCollide))return false;
   if(rd<std::uint32_t>(h,records+0x400+p.group*4)!=rules.masks[p.group])return false;
  }
  auto flags=rd<unsigned char>(h,ball.part.primitive+Offsets::Primitive::Flags,255);
  return flags!=255&&rd<addr>(h,ball.part.id+Offsets::BasePart::Primitive)==ball.part.primitive&&(!applied||rd<unsigned char>(h,ball.part.primitive+physicsGroupOffset,255)==group);
 }
};
}
