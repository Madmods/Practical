#define NOMINMAX
#include "engine.hpp"
#include <iostream>
// Read-only Park verification. Plans are inspected but never sent to Inputs.
// Anonymous roster summaries only; no player names, IDs or input workers.
int main(){
 HANDLE h=nullptr;DWORD pid=0;addr base=0;if(!connectRoblox(h,pid,base)){std::cout<<"Supported process unavailable\n";return 1;}
 auto root=rd<addr>(h,rd<addr>(h,base+Offsets::FakeDataModel::Pointer)+Offsets::FakeDataModel::RealDataModel);
 auto players=child(h,root,"Players"),local=rd<addr>(h,players+Offsets::Player::LocalPlayer),ws=child(h,root,"Workspace"),characters=child(h,ws,"Characters");
 std::map<std::string,addr> lookup;for(auto p:children(h,players))lookup[nm(h,p)]=p;
 defense::CourtRoster roster;roster.scan(h,ws,lookup);auto members=roster.read(h);
 std::cout<<"park_roster_available="<<roster.available<<" local_active="<<members.get(local).valid()<<" active_members="<<members.players.size()<<"\n";
 gamePid=pid;sceneRoot=root;int checked=0,shared=0,regressed=0;
 for(const auto& court:roster.courts){
  addr representatives[2]{};int sizes[2]{};
  for(const auto& entry:members.players)if(entry.second.valid()&&entry.second.court==court.id){sizes[entry.second.team-1]++;representatives[entry.second.team-1]=entry.first;}
  std::cout<<"court="<<nm(h,court.id)<<" team1="<<sizes[0]<<" team2="<<sizes[1];
  if(!representatives[0]||!representatives[1]){std::cout<<" inactive\n";continue;}
  auto rig=[&](addr player){return child(h,characters,nm(h,player));};
  auto goal=[&](addr player){return rd<addr>(h,path(h,rig(player),{"Attributes","Goal"})+Offsets::Misc::Value);};
  const auto a=goal(representatives[0]),b=goal(representatives[1]);const bool sameHoop=a&&a==b;
  auto ball=roster.ball(h,members.get(representatives[0])),holder=rd<addr>(h,path(h,ball,{"Attributes","Character"})+Offsets::Misc::Value);
  auto holderPlayer=lookup.find(nm(h,holder));unsigned defending=0;
  if(holderPlayer!=lookup.end()&&members.get(holderPlayer->second).valid())defending=members.get(holderPlayer->second).team==1?1:0;
  auto player=representatives[defending];scenePlayer=player;sceneOwn=rig(player);
  defense::Reader reader;const auto start=now();auto frame=reader.read(start);const auto elapsed=now()-start;int enemies=0,allies=0;bool handler=false;
  for(auto& actor:frame.scene.actors){if(actor.enemy){++enemies;handler=handler||defense::heldBall(frame.scene,actor.id);}else ++allies;}
  defense::Config config;config.values[defense::Guard]=1;config.values[defense::Reaction]=0;
  defense::Intent intent;intent.focused=intent.activation=true;defense::Planner planner;auto decision=planner.step(config,frame.scene,intent,now());
  bool policy=defense::courtOpponent(members.get(representatives[0]),members.get(representatives[1]));
  std::cout<<" same_hoop="<<sameHoop<<" legacy_opponent="<<defense::opposingMatchup(0,0,a,b)<<" roster_opponent="<<policy<<" reader_enemies="<<enemies<<" reader_allies="<<allies<<" reader_balls="<<frame.scene.balls.size()<<" playing="<<frame.scene.playing<<" valid="<<frame.scene.valid<<" read_ms="<<elapsed*1000<<" handler="<<handler<<" planned_target="<<bool(decision.target)<<" reason="<<decision.reason<<"\n";
  ++checked;if(sameHoop)++shared;if(!policy||!frame.scene.playing)++regressed;
 }
 scenePlayer=local;sceneOwn=child(h,characters,nm(h,local));defense::Reader localReader;auto own=localReader.read(now());
 std::cout<<"local_reader_playing="<<own.scene.playing<<" local_reader_actors="<<own.scene.actors.size()<<" local_reader_balls="<<own.scene.balls.size()<<"\n";
 std::cout<<"active_courts_checked="<<checked<<" shared_hoop_courts="<<shared<<" policy_failures="<<regressed<<" input_requests=0\n";
 CloseHandle(h);return regressed?2:0;
}
