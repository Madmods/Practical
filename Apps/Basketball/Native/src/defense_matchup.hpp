#pragma once
#include <cstdint>
#include <map>

namespace defense {
struct CourtMember {
 std::uintptr_t court=0;unsigned team=0;
 bool valid()const{return court&&(team==1||team==2);}
};
inline bool sameCourt(CourtMember a,CourtMember b){return a.valid()&&b.valid()&&a.court==b.court;}
inline bool courtOpponent(CourtMember a,CourtMember b,bool self=false){return !self&&sameCourt(a,b)&&a.team!=b.team;}
// Practice also publishes geometry under Map/Courts, without match rosters. Its presence alone is
// not a match assignment. Keep the exception opt-in and specific to named NPCs
// sharing the local character's explicit goal, never to unmapped humans.
inline bool practiceCourtContext(bool enabled,bool supportedPracticeLayout,bool matchStatePresent,CourtMember member,std::uintptr_t goal){
 return enabled&&supportedPracticeLayout&&!matchStatePresent&&!member.valid()&&goal;
}
inline bool practiceCourtOpponent(bool context,bool qualifiedBot,std::uintptr_t ownGoal,std::uintptr_t otherGoal){
 return context&&qualifiedBot&&ownGoal&&ownGoal==otherGoal;
}
// Unrostered private-gym practice has no opposing team assignments. A mapped
// human sharing the explicit hoop can be a practice partner. Never infer this
// from missing Park data, different hoops, explicit match state or a known ally.
inline bool gymPracticePartner(bool context,bool mappedPlayer,bool matchStatePresent,std::uintptr_t ownGoal,std::uintptr_t otherGoal,std::uintptr_t ownTeam,std::uintptr_t otherTeam){
 return context&&mappedPlayer&&!matchStatePresent&&ownGoal&&ownGoal==otherGoal&&
  ((!ownTeam&&!otherTeam)||(ownTeam&&otherTeam&&ownTeam!=otherTeam));
}
// A player duplicated across rosters is ambiguous until a later clean snapshot.
struct CourtMembers {
 std::map<std::uintptr_t,CourtMember> players;
 void add(std::uintptr_t player,CourtMember member){
  if(!player||!member.valid())return;
  auto [it,added]=players.emplace(player,member);
  if(!added)it->second={};
 }
 CourtMember get(std::uintptr_t player)const{auto it=players.find(player);return it==players.end()?CourtMember{}:it->second;}
};
}
