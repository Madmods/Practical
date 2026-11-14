#include "game_modules.hpp"
#include <cassert>
#include <iostream>
using namespace gameModules;
Snapshot fixture(Id id,double t){
 Snapshot s;s.pid=1;s.model=100;s.sampledAt=t;
 const auto& m=registry[id==Id::Basketball?0:1];s.placeId=m.rootPlace;s.universeId=m.universe;
 s.process=s.version=s.opened=s.loaded=s.player=s.world=s.renderer=true;
 if(id==Id::Basketball){s.character=200;s.characterReady=s.meter=true;}return s;
}
int main(){
 int checks=0;auto check=[&](bool b){assert(b);++checks;};
 Router r;auto bb=fixture(Id::Basketball,10),ff=fixture(Id::FootballFusion3,11);
 check(identify(bb)->id==Id::Basketball);check(identify(ff)->id==Id::FootballFusion3);
 check(gameModules::compatible(ff)&&!basketballConnection::compatible(ff));check(registry[1].gameplay);
 auto mismatch=bb;mismatch.universeId=registry[1].universe;check(!identify(mismatch));
 auto subplace=ff;subplace.placeId=123456;check(identify(subplace)->id==Id::FootballFusion3);
 for(auto s:{bb,ff}){
  s.placeId=0;check(!identify(s));s.placeId=1;s.universeId=0;check(!identify(s));
  s.universeId=42;check(!identify(s));
 }
 r.publish(bb);check(r.open(10,true,true)==Id::None);
 bb.sampledAt=10.8;r.publish(bb);check(r.open(10.8,false,true)==Id::None);check(r.open(10.8,true,false)==Id::None);
 check(r.open(10.8,true,true)==Id::Basketball);
 r.publish(ff);check(r.active(11)==Id::None);check(r.open(11,true,true)==Id::None);
 ff.sampledAt=11.8;r.publish(ff);check(r.active(11.8)==Id::None);check(r.open(11.8,true,true)==Id::FootballFusion3);
 check(r.active(11.8,false)==Id::None);check(r.active(11.8,true)==Id::None);
 check(r.open(11.8,true,true)==Id::FootballFusion3);r.close();check(r.active(11.8)==Id::None);
 for(auto member:{&Snapshot::process,&Snapshot::version,&Snapshot::opened,&Snapshot::loaded,&Snapshot::player,&Snapshot::world,&Snapshot::renderer}){
  auto s=ff;s.*member=false;check(!gameModules::compatible(s));r.publish(s);check(r.open(11.8,true,true)==Id::None);
 }
 Router stale;r.publish(ff);ff.sampledAt=12.6;r.publish(ff);check(r.open(12.6,true,true)==Id::FootballFusion3);
 check(r.active(14)==Id::None);check(r.open(12,true,true)==Id::None);
 ff.sampledAt=14;r.publish(ff);check(r.open(14,true,true)==Id::None);
 ff.sampledAt=14.8;r.publish(ff);check(r.open(14.8,true,true)==Id::FootballFusion3);
 ff.model++;ff.sampledAt=15;r.publish(ff);check(r.active(15)==Id::None);
 bb.sampledAt=16;r.publish(bb);bb.sampledAt=16.8;r.publish(bb);check(r.open(16.8,true,true)==Id::Basketball);
 bb.character++;bb.sampledAt=17;r.publish(bb);check(r.active(17)==Id::None);
 bb.sampledAt=17.8;r.publish(bb);check(r.active(17.8)==Id::None);check(r.open(17.8,true,true)==Id::Basketball);
 auto noMeter=bb;noMeter.meter=false;check(!gameModules::compatible(noMeter));
 std::cout<<checks<<" game identity, module isolation, stability, session transition and admission checks passed\n";
}
