#pragma once
#include "ff3_assist_logic.hpp"
namespace ff3Assist {
// A conservative sphere inside one observed catch part, expressed from the head.
// This deliberately does not treat a maximum arm sweep or Mags size as a catch.
struct ContactReach {Vec offset;double radius=1.25;bool measured=false,anticipated=false;};
inline bool validContactReach(const ContactReach& r){return r.measured&&ff3qb::finite(r.offset)&&ff3qb::length(r.offset)<10&&std::isfinite(r.radius)&&r.radius>=.3&&r.radius<=3;}
inline ContactReach raisedCatchContact(ContactReach actual,Vec arm,Vec head){
 if(!validContactReach(actual)||!ff3qb::finite(arm)||!ff3qb::finite(head)||arm.y<.5||arm.y>4||head.y<.3||head.y>3)return actual;
 const double raised=std::clamp(arm.y-head.y*.5+actual.radius*.75,.8,3.2);
 if(actual.offset.y>=raised-.4)return actual;
 return {{0,raised,0},actual.radius,true,true};
}
}
