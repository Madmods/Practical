#pragma once
#include <array>
#include <cstdint>
#include <string>
namespace ff3Play {
struct CollisionRules {
 std::array<std::uint32_t,32> masks{};
 std::array<std::string,32> names{};
 bool valid()const{
  if(names[0]!="Default")return false;
  for(unsigned a=0;a<32;++a)for(unsigned b=0;b<32;++b)if(bool(masks[a]&(1u<<b))!=bool(masks[b]&(1u<<a)))return false;
  return true;
 }
 int choose(unsigned original,std::uint32_t bodies,std::uint32_t catches)const{
  if(original>=32||names[original].empty()||!bodies||!catches||(bodies&catches)||!valid())return -1;
  auto usable=[&](unsigned id){return !names[id].empty()&&masks[id]==~bodies&&(masks[id]&catches)==catches&&(masks[id]&1u);};
  if(usable(original))return int(original);
  for(unsigned id=0;id<32;++id)if(usable(id))return int(id);
  return -1;
 }
};
}
