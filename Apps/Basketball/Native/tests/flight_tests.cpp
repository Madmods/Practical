#include "flight_logic.hpp"
#include <cassert>
#include <cstdio>
int main(){
 flight::Track t;flight::Estimate e;
 for(int i=0;i<=10;i++){float dt=i*.016f;e=t.add(dt,{20*dt,5+18*dt-22.5f*dt*dt,4*dt},45);}
 assert(e.valid);assert(std::abs(e.v.x-20)<.01);assert(std::abs(e.v.y-(18-45*.16))<.01);
 auto arc=flight::predict(e,0,.75f);assert(arc.valid);double total=(18+sqrt(18*18+90*4.25))/45;
 assert(std::abs(arc.time-(total-.16))<.001);assert(std::abs(arc.landing.x-20*total)<.01);assert(arc.points.size()==49);assert(std::abs(arc.landing.y-.03)<.001);
 auto unchanged=t.add(.30,e.p,45);assert(!unchanged.valid);
 t.reset();assert(!t.add(1,{0,5,0},45).valid);assert(!t.add(1.01,{40,20,0},45).valid);
 for(int i=0;i<10;i++)t.add(2+i*.016,{0,1,0},45);assert(!t.estimate.valid);
 assert(!flight::predict({},0,.75f).valid);assert(!flight::predict(e,NAN,.75f).valid);auto bad=e;bad.gravity=0;assert(!flight::predict(bad,0,.75f).valid);
 t.reset();for(int i=0;i<=10;i++){float dt=i*.016f;e=t.add(dt,{20*dt,5+18*dt-22.5f*dt*dt,0},45);}assert(e.valid);assert(!t.add(.18,{1,1,0},45).valid);
 puts("Ball flight: ballistic fit, landing time, stop, teleport, invalid data and collision reset passed");
}
