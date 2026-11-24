#pragma once
#include "ff3_qb_logic.hpp"
namespace ff3qb {
struct ThrowTiming {double outbound=0,replicationLead=0,tolerance=.025;};
// Performance Stats supplies our round-trip ping, not another player's ping.
// A remote avatar is observed after inbound transit. Advance that observation
// toward the server-present estimate before the usual outbound launch solve.
// Interpolation/remote-client delay remains uncertain; use a bounded tolerance
// envelope rather than reporting this as a measurement of that player's ping.
inline ThrowTiming throwTiming(double pingMilliseconds,bool remoteHuman){
 if(!std::isfinite(pingMilliseconds)||pingMilliseconds<1||pingMilliseconds>400)return {NAN,NAN,NAN};
 const double oneWay=pingMilliseconds*.0005;
 return {std::clamp(oneWay+.05,0.,.25),remoteHuman?oneWay:0.,remoteHuman?std::clamp(.05+oneWay,.05,.15):.025};
}
inline double normalArcMinimum(double preferred,double existing){return std::isfinite(preferred)&&std::isfinite(existing)?std::max(existing,std::clamp(preferred-8,5.,45.)):NAN;}
}
