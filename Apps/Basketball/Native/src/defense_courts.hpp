#pragma once

// Included by defense_runtime.hpp after its version-checked instance helpers.
// Park publishes active membership as BoolValues named for players, under each
// court's Game/Attributes/Players/Team1 or Team2. The Goal object alone cannot
// distinguish half-court opponents, who attack the very same basket.
struct CourtRoster {
 struct Court{addr id,game,attributes,players,teams[2],ballValue;};
 struct Slot{addr player,value,parent,court;unsigned team;std::string name;};
 addr workspace=0,map=0,container=0;bool available=false;
 std::vector<Court> courts;std::vector<Slot> slots;
 void scan(HANDLE h,addr ws,const std::map<std::string,addr>& players){
  *this={};workspace=ws;map=child(h,ws,"Map");container=child(h,map,"Courts");available=container!=0;
  if(!available)return;
  const auto list=children(h,container);if(list.size()>32)return;
  for(auto id:list){
   auto game=child(h,id,"Game"),attrs=child(h,game,"Attributes"),roster=child(h,attrs,"Players");
   auto one=child(h,roster,"Team1"),two=child(h,roster,"Team2"),ball=child(h,attrs,"Basketball");
   if(!one||!two||classOf(h,one)!="Folder"||classOf(h,two)!="Folder")continue;
   courts.push_back({id,game,attrs,roster,{one,two},classOf(h,ball)=="ObjectValue"?ball:0});
   for(unsigned team=1;team<=2;team++){
    auto group=team==1?one:two;const auto entries=children(h,group);if(entries.size()>32)continue;
    for(auto entry:entries){auto name=nm(h,entry);auto found=players.find(name);if(found!=players.end()&&classOf(h,entry)=="BoolValue")slots.push_back({found->second,entry,group,id,team,name});}
   }
  }
 }
 bool parent(HANDLE h,addr object,addr expected)const{return object&&expected&&rd<addr>(h,object+Offsets::Instance::Parent)==expected;}
 bool practiceLayout(HANDLE h)const{
  if(!available||!parent(h,map,workspace)||!parent(h,container,map))return false;
  const auto list=children(h,container);if(list.size()!=1)return false;
  const addr court=list.front();if(nm(h,court)!="Court1"||classOf(h,court)!="Model"||child(h,court,"Game"))return false;
  auto one=child(h,court,"HalfCourt1"),two=child(h,court,"HalfCourt2"),items=child(h,court,"Items");
  return parent(h,court,container)&&parent(h,one,court)&&parent(h,two,court)&&parent(h,items,court)&&classOf(h,one)=="Model"&&classOf(h,two)=="Model"&&classOf(h,items)=="Folder";
 }
 bool current(HANDLE h,const Court& c)const{
  return parent(h,map,workspace)&&parent(h,container,map)&&parent(h,c.id,container)&&parent(h,c.game,c.id)&&parent(h,c.attributes,c.game)&&parent(h,c.players,c.attributes)&&parent(h,c.teams[0],c.players)&&parent(h,c.teams[1],c.players);
 }
 CourtMembers read(HANDLE h)const{
  CourtMembers result;std::set<addr> valid;
  for(const auto& c:courts)if(current(h,c))valid.insert(c.id);
  for(const auto& s:slots)if(valid.count(s.court)&&parent(h,s.value,s.parent)&&nm(h,s.value)==s.name&&rd<unsigned char>(h,s.value+Offsets::Misc::Value,255)==1)result.add(s.player,{s.court,s.team});
  return result;
 }
 addr ball(HANDLE h,CourtMember member)const{
  if(!member.valid())return 0;
  for(const auto& c:courts)if(c.id==member.court&&current(h,c)&&parent(h,c.ballValue,c.attributes))return rd<addr>(h,c.ballValue+Offsets::Misc::Value);
  return 0;
 }
};
