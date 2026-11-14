#include "pump_logic.hpp"
#include <cassert>
#include <iostream>
using namespace pump;
Observation initial(){Observation o;o.time=10;o.tapAt=10;o.ball=o.tapBall=10;o.target=o.tapTarget=20;o.focused=o.timing=o.stationary=o.fresh=o.tapGrounded=true;o.distance=6;o.height=o.tapHeight=4;o.verticalSpeed=0;o.otherContest=0;o.tap=1;return o;}
int main(){Config cfg{true,12,.12,10};Logic l;auto o=initial();assert(l.step(cfg,o).state==Watching);
 o.time=10.12;o.height=5;o.verticalSpeed=10;assert(l.step(cfg,o).state==Jumped);
 o.time=10.30;o.height=6;o.verticalSpeed=1;assert(!l.step(cfg,o).shoot); // no jump-at-the-peak shot
 o.time=10.35;o.verticalSpeed=-4;assert(l.step(cfg,o).state==Opening);
 o.time=10.40;assert(l.step(cfg,o).shoot);o.time=10.44;assert(!l.step(cfg,o).shoot); // exactly once
 for(int reason=0;reason<7;reason++){Logic t;auto x=initial();t.step(cfg,x);x.time=10.1;x.height=5;x.verticalSpeed=8;t.step(cfg,x);x.time=10.4;x.height=6;x.verticalSpeed=-4;
  if(reason==0)x.ball=0;if(reason==1)x.ball=11;if(reason==2)x.focused=false;if(reason==3)x.fresh=false;if(reason==4)x.stationary=false;if(reason==5)x.otherContest=60;if(reason==6)x.timing=false;
  assert(!t.step(cfg,x).shoot);x.time+=.1;assert(!t.step(cfg,x).shoot);
 }
 Logic noJump;auto n=initial();noJump.step(cfg,n);n.time=10.4;assert(!noJump.step(cfg,n).shoot);n.time=11.5;assert(noJump.step(cfg,n).state==Expired);
 Logic airborne;auto a=initial();a.tapGrounded=false;assert(airborne.step(cfg,a).state==Ready);a.time=10.5;a.height=6;a.verticalSpeed=-4;assert(!airborne.step(cfg,a).shoot);
 Logic off;auto q=initial();Config disabled=cfg;disabled.enabled=false;off.step(disabled,q);assert(off.step(cfg,q).state==Ready); // no replay when enabling
 std::cout<<"Pump fake state-machine checks passed\n";
}
