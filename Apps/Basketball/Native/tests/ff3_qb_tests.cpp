#include "ff3_qb_logic.hpp"
#include "ff3_throw_state.hpp"
#include <cassert>
#include <iostream>
using namespace ff3qb;
int main(){int count=0;
 {
  const Vec origin{0,8,0};const std::vector<Defender> none;
  assert(openAwayMinimum(origin,{70,8,0},{20,0,0},none,true)==25);
  for(const auto& route:{Vec{},Vec{4,0,0},Vec{-20,0,0},Vec{0,0,20}})assert(openAwayMinimum(origin,{70,8,0},route,none,true)<0);
  assert(openAwayMinimum(origin,{70,8,0},{20,0,0},none,false)<0);
  assert(openAwayMinimum(origin,{5,8,0},{20,0,0},none,true)<0);
  Defender covered;covered.position={70,8,0};assert(openAwayMinimum(origin,covered.position,{20,0,0},{covered},true)<0);
  for(bool beam:{false,true})for(Vec direction:{Vec{1,0,0},Vec{0,0,1},Vec{.6,0,.8}})for(double distance:{35.,70.,110.,160.}){
   Defender wr;wr.position=origin+direction*distance;wr.velocity=direction*20;wr.speed=20;wr.jumpHeight=7;wr.radius=3;wr.above=2;wr.below=4;
   const double floor=openAwayMinimum(origin,wr.position,wr.velocity,none,true);
   auto p=planCoverageThrow(origin,wr.position,wr.velocity,28,.08,none,true,beam,7,196.2,{},0,false,NAN,true,.1,&wr,0,nullptr,0,0,50,nullptr,floor);
   assert(p.solution.valid&&p.solution.angle>=25-1e-6&&p.solution.angle<=50+1e-6&&p.power<=95);
   assert(length(point(p.solution,28,p.solution.time)-(wr.position+p.offset+wr.velocity*(p.solution.time+.08)))<1e-4);
   assert(receiverGap(wr,p.solution.catchPoint,p.solution.time+.08)<=-.25);
   // A retained shallow arc must not leak back through a quiet update or
   // the old elevation hysteresis when the receiver starts running away.
   BeamMemory memory;memory.elevation.accept(7,0);memory.goal={solve(origin,wr.position,wr.velocity,95,28,.08,false,5),{},95,10,false,false};
   auto quiet=planCoverageThrow(origin,wr.position,wr.velocity,28,.08,none,true,beam,7,196.2,{},95,false,7,false,.016,&wr,0,&memory,.016,0,50,nullptr,floor);
   assert(quiet.solution.valid&&quiet.solution.angle>=25-1e-6&&quiet.solution.angle<=50+1e-6);
   auto preset=planThrow(origin,wr.position,wr.velocity,28,.08,1,true,12,true,95,false,false,none,12,{},false,5,0,50,nullptr,&wr,floor);
   assert(preset.solution.valid&&preset.solution.angle>=25-1e-6);
   ++count;
  }
 }

 {
  // Lock during an existing weave, in different world directions and phases.
  for(double rotation:{0.,.7,1.8})for(double dt:{1./60.,.02,.05})for(double phase:{0.,.025,.075,.125})for(double amplitude:{4.,8.,12.}){
   RouteHistory history;
   const Vec forward{std::sin(rotation),0,std::cos(rotation)};
   const Vec side{forward.z,0,-forward.x};
   const double speed=std::sqrt(400-amplitude*amplitude);
   double error=0;int samples=0;
   for(int i=0;i<int(3/dt);++i){
    double t=i*dt,w=t+phase;int segment=int(std::floor((w+1e-9)/.1));double local=w-segment*.1;
    double x=segment%2?amplitude*(.1-local):amplitude*local;
    Vec position=Vec{0,8,100}+forward*(speed*t)+side*x;
    Vec velocity=forward*speed+side*(segment%2?-amplitude:amplitude);
    history.update(t,{},position,velocity);
    if(t>1){error+=length(history.movement-forward*speed);++samples;}
   }
   assert(samples>0&&error/samples<1.);
  }
  Plan tight,comfortable;tight.solution.valid=true;tight.power=95;tight.margin=4;tight.deliveryMargin=4;tight.launchMargin=4;tight.coverageKnown=true;
  tight.receiverMargin=.3;tight.robustMargin=-.5;
  comfortable=tight;comfortable.power=90;comfortable.receiverMargin=2;comfortable.robustMargin=1.5;
  assert(rankedBetter(comfortable,tight,true,true));
  assert(!continuitySafe(tight,comfortable,true));
  Plan exposed=comfortable;exposed.margin=exposed.deliveryMargin=exposed.launchMargin=-1;
  assert(rankedBetter(tight,exposed,true,true));
 }

 {
  assert(pressAuthorized(1000,0,true,5000));
  assert(pressAuthorized(1000,5000,false,5350));
  assert(!pressAuthorized(1000,5000,false,5351));
  assert(!pressAuthorized(1000,5000,true,5100));
  assert(!pressAuthorized(0,0,true,5000));
  assert(!pressAuthorized(6000,0,true,5000));
  assert(!pressAuthorized(1000,0,true,301001));
 }
 {
  auto powers=powerSearchOrder();std::array<int,96> seen{};
  assert(powers.size()==95&&powers[0]==95&&powers[1]<90&&powers[2]<=60);
  for(int p:powers){assert(p>=1&&p<=95);++seen[p];}
  for(int p=1;p<=95;++p)assert(seen[p]==1);
  std::vector<Plan> plans;
  for(int i=0;i<16;++i){Plan p;p.solution.valid=true;p.solution.angle=10+i*.3;p.solution.time=1+i*.02;p.power=95-i*.7;p.coverageKnown=true;p.margin=-4+i*.26;p.deliveryMargin=i%3?2.:-2.;p.launchMargin=i%2?2.:-2.;p.penetration=30-i;plans.push_back(p);}
  for(const auto& a:plans)for(const auto& b:plans)for(const auto& c:plans)if(rankedBetter(a,b,true,true)&&rankedBetter(b,c,true,true))assert(rankedBetter(a,c,true,true));
  auto reversed=plans;std::reverse(reversed.begin(),reversed.end());
  auto sort=[](std::vector<Plan>& pool){std::sort(pool.begin(),pool.end(),[](const Plan& a,const Plan& b){return rankedBetter(a,b,true,true);});};
  sort(plans);sort(reversed);for(size_t i=0;i<plans.size();++i)assert(rankPlan(plans[i],true,true)==rankPlan(reversed[i],true,true));
  std::vector<Plan> pool;for(const auto& p:reversed)retainRanked(pool,p,true,true);
  assert(pool.size()<=8&&rankPlan(pool.front(),true,true)==rankPlan(plans.front(),true,true));
 }
 {
  Defender wr{{80,8,0},{},20,6.37,12,5.88,3.88,4.88,196.2,true};
  BeamMemory memory;Plan old;old.solution=solve({0,8,0},wr.position,{},95,28,.05,false,5);old.power=95;old.margin=10;memory.leaders={old};
  Vec origin{5,8,0};Defender db{{20,8,0},{},22,6.37,12,5.88,3.88,4.88,196.2};
  auto p=planCoverageThrow(origin,wr.position,{},28,.05,{db},true,true,wr.jumpHeight,196.2,{},0,false,NAN,false,.1,&wr,0,&memory,1);
  assert(p.solution.valid&&!memory.leaders.empty()&&memory.candidatesRanked>0);
  for(const auto& fresh:memory.leaders){
   assert(std::abs(length(fresh.solution.origin-origin)-5)<1e-6);
   assert(fresh.margin<=coverageMargin(fresh.solution,28,{db},.05)+1e-9&&fresh.margin<old.margin);
  }
 }
 {
  for(double dt:{.025,.05}){
   RouteHistory h;Vec p{0,8,150};double t=0;
   for(int i=0;i<int(1/dt);++i){t+=dt;p=p+Vec{0,0,20}*dt;h.update(t,{0,8,0},p,{0,0,20});}
   for(int i=0;i<int(2/dt);++i){Vec v{int(i*dt/.1)%2?3.:-3.,0,20};t+=dt;p=p+v*dt;h.update(t,{0,8,0},p,v);}
   assert(std::abs(h.movement.x)<1.2&&h.movement.z>18);
   for(int i=0;i<int(.4/dt);++i){Vec v{4,0,19.6};t+=dt;p=p+v*dt;h.update(t,{0,8,0},p,v);}
   assert(h.movement.x>3&&!h.intent.wobble);
  }
 }
 {
  RouteHistory h;Vec p{0,8,150};double t=0;
  for(int i=0;i<20;++i){t+=.05;p=p+Vec{0,0,20}*.05;h.update(t,{0,8,0},p,{0,0,20});}
  int filtered=0;
  for(int i=0;i<80;++i){Vec v{i%4<2?8.:-8.,0,20};t+=.05;p=p+v*.05;h.update(t,{0,8,0},p,v);
   if(i>20){assert(std::abs(h.movement.x)<1.5&&h.movement.z>18);filtered+=h.intent.wobble;}
   assert(length(h.intent.correction)<=1.500001);
  }
  assert(filtered>30);
  // A held diagonal cut must replace the straight-route assumption promptly.
  for(int i=0;i<8;++i){Vec v{14,0,14};t+=.05;p=p+v*.05;h.update(t,{0,8,0},p,v);}
  assert(h.movement.x>10&&h.intent.heading.x>.5&&!h.intent.wobble);
  // A full reversal is not a small lateral wobble.
  for(int i=0;i<3;++i){Vec v{0,0,-20};t+=.05;p=p+v*.05;h.update(t,{0,8,0},p,v);}
  assert(h.movement.z< -15);
  for(int i=0;i<2;++i){t+=.05;h.update(t,{0,8,0},p,{});}
  assert(length(h.movement)==0&&length(h.intent.correction)==0);
 }
 {
  Plan nearDb,overDb;nearDb.solution.valid=overDb.solution.valid=true;
  nearDb.coverageKnown=overDb.coverageKnown=true;nearDb.margin=-2;overDb.margin=-5;
  nearDb.deliveryMargin=-2;nearDb.launchMargin=-2;overDb.deliveryMargin=2;overDb.launchMargin=2;
  overDb.penetration=60;
  assert(deliveryPriority(overDb,nearDb,true)>0&&!continuitySafe(nearDb,overDb,true));
  assert(!estimatedClear(overDb,true)); // A possible missed-catch interception remains risk.
  auto inconsistent=overDb;inconsistent.margin=inconsistent.deliveryMargin=2;inconsistent.launchMargin=.5;
  assert(!estimatedClear(inconsistent,true)&&deliveryClearance(inconsistent)==.5);
  BeamMemory memory;memory.goal=nearDb;
  assert(memory.accept(overDb,nearDb,1,true).deliveryMargin==2);
  // If every delivery is contested, avoid an immediate launch-corridor block.
  overDb.deliveryMargin=-4;overDb.launchMargin=2;
  assert(deliveryPriority(overDb,nearDb,true)>0);
  Defender db{{20,8,0},{},22,6.37,12,5.88,3.88,4.88,196.2};
  auto low=solve({0,8,0},{100,8,0},{},95,28,.05,false,5);
  assert(low.valid&&reachGap(db,low.catchPoint,low.time+.05)>1);
  CoverageTrace trace;assert(coverageMargin(low,28,{db},.05,&trace)<0&&trace.time<low.time*.5);
  Defender wr{{100,8,0},{},20,6.37,12,5.88,3.88,4.88,196.2,true};BeamMemory search;Plan protectedPass;
  for(int i=0;i<5;++i)protectedPass=planCoverageThrow({0,8,0},wr.position,{},28,.05,{db},true,true,wr.jumpHeight,196.2,{},0,false,NAN,true,.1,&wr,0,&search,i*.1,0);
  assert(protectedPass.solution.valid&&protectedPass.launchMargin>1&&protectedPass.deliveryMargin>1);
  assert(protectedPass.solution.angle>low.angle&&protectedPass.power<95);
 }
 {
  Defender wr{{80,8,0},{0,0,20},20,6.37,12,5.88,3.88,4.88,196.2,true};
  assert(receiverGap(wr,{92,8,40},2)<-.25);
  assert(receiverGap(wr,{68,8,40},2)<-.25);
  assert(receiverGap(wr,{80,8,-20},2)>0);
  assert(receiverGap(wr,{120,8,40},2)>0);
  auto slowing=wr;slowing.velocity={0,0,5};
  assert(receiverGap(slowing,{96,8,10},2)>0);
  auto options=fitOffsets({0,8,0},wr.position,wr.velocity,{},true);
  assert(std::any_of(options.begin(),options.end(),[](Vec p){return p.x==14&&p.z==0;}));
  wr.position={35,8,0};wr.velocity={};wr.redirect=false;
  auto automatic=planCoverageThrow({0,8,0},wr.position,{},28,.05,{},true,false,wr.jumpHeight,196.2,{},0,false,NAN,true,.1,&wr,0,nullptr,0,0,50);
  assert(automatic.solution.valid&&automatic.power>94.99&&automatic.solution.angle<5);
  assert(length(automatic.solution.catchPoint-wr.position)<1e-6);
 }
 {
  // Reproduce the recorded bot-1 miss: the old target was 13.4 studs off
  // its running route. A human could redirect; an unsteered bot cannot.
  const Vec movement{0,0,19.1811090302764},origin{39.37295150756836,7.486886024475098,98.37617492675781};
  Defender bot{{31.4579086303711,7.492891311645508,229.9359186851148},movement,20,6.37105,12,5.88186,3.88186,4.88186,196.2,false};
  const Vec oldCatch{18.0116824185237,16.1955145092612,264.99486380318905};
  assert(receiverGap(bot,oldCatch,1.8081793146045+.075)>0);
  auto human=bot;human.redirect=true;assert(receiverGap(human,oldCatch,1.8081793146045+.075)<0);
  const Vec target=bot.position+leadAdjustment(origin,bot.position,movement,3);
  auto followingDb=bot;followingDb.position.z-=8;followingDb.speed=22;
  for(bool beam:{false,true}){
   BeamMemory memory;Plan planned;Vec offset{};double power=0,angle=NAN;bool high=false;
   for(int i=0;i<3;++i){
    planned=planCoverageThrow(origin,target,movement,28,.075,{followingDb},true,beam,bot.jumpHeight,196.2,offset,power,high,angle,true,.1,&bot,0,&memory,i*.1,0,beam?beamMaxAngle:50);
    assert(planned.solution.valid);
    assert(receiverGap(bot,planned.solution.catchPoint,planned.solution.time+.075)<=-.25);
    auto route=bot.position+movement*(planned.solution.time+.075);
    assert(std::hypot(planned.solution.catchPoint.x-route.x,planned.solution.catchPoint.z-route.z)<9);
    assert(preservesCatchHeight(planned.offset));
    offset=planned.offset;power=planned.power;high=planned.lob;angle=planned.solution.angle;
   }
  }
  assert(preservesCatchHeight({5,2,0}));
  assert(preservesCatchHeight({0,0,-1}));
  assert(!preservesCatchHeight({0,-1,0}));
  assert(preservesCatchHeight({0,1,-2}));
  assert(preservesCatchHeight({0,1,2}));
 }
 {
  Defender db{{0,8,0},{},22,0,12,5.88,3.88,4.88,196.2};
  Solution deep{true,{0,8,0},{10,0,0},{10,7.5,0},1,0},edge=deep;edge.origin.z=edge.catchPoint.z=20;
  const double dm=coverageMargin(deep,1,{db},0),em=coverageMargin(edge,1,{db},0);
  assert(dm<0&&std::abs(dm-em)<1e-9);
  const double dp=coveragePenetration(deep,1,{db},0),ep=coveragePenetration(edge,1,{db},0);
  assert(dp>ep+5&&ep>0);
  Plan held{deep,{},95,dm,true,false,dp},desired{edge,{0,0,20},95,em,true,false,ep};
  assert(!estimatedClear(desired,true));assert(!continuitySafe(held,desired,true));
  BeamMemory memory;memory.goal=held;
  assert(memory.accept(desired,held,1,true).penetration==ep);
  assert(continuitySafe(held,desired,false));
 }
 {
  Defender wr{{0,8,0},{},20,6.37,12,5.88,3.88,4.88,196.2,true};
  assert(receiverGap(wr,{0,14,0},.1)>0);assert(receiverGap(wr,{0,17.5,0},1)>0);
  assert(receiverGap(wr,{0,13,0},.6)<0);assert(receiverGap(wr,{20,8,0},3)>0);
  Plan held;held.solution.valid=true;held.solution.angle=39;held.power=95;held.margin=2;held.coverageKnown=true;
  BeamMemory choice;choice.goal=held;
  for(int i=0;i<100;i++){auto alternate=held;alternate.solution.angle=39+(i%2?1:-1);assert(choice.accept(alternate,held,i*.016,true).solution.angle==39);}
  auto safe=held,unsafe=held;unsafe.margin=-1;choice.goal=unsafe;
  safe.solution.angle=40;assert(choice.accept(safe,unsafe,2,true).margin==2);
 }
 {
  Defender wr{{100,8,0},{0,0,20},20,7,12};wr.redirect=true;
  for(double t:{.5,1.,2.,3.}){
   assert(receiverGap(wr,wr.position+wr.velocity*t,t)<0);
   auto following=wr;following.redirect=false;
   assert(receiverGap(wr,wr.position+wr.velocity*t+Vec{0,3,0},t)<=receiverGap(following,wr.position+wr.velocity*t+Vec{0,3,0},t));
  }
  for(double distance:{25.,75.,150.,250.}){
   Vec target{distance,8,0};Defender receiver{target,{},20,7,12};
   auto lob=planLoftThrow({0,8,0},target,{},28,.05,{},true,&receiver);
   auto beam=planCoverageThrow({0,8,0},target,{},28,.05,{},true,true,7,196.2);
   assert(lob.solution.valid&&lob.solution.angle>=35&&lob.solution.angle<=50&&lob.solution.velocity.y-28*lob.solution.time<0);
   assert(beam.solution.valid&&beam.power==95&&beam.solution.angle<lob.solution.angle);
   assert(length(point(lob.solution,28,lob.solution.time)-target)<1e-6);
  }
  auto capped=planCoverageThrow({0,8,0},{100,8,0},{},28,.05,{},true,false,7,196.2,{},60,true,65,true,.1,nullptr,0,nullptr,0,NAN,50);
  assert(capped.solution.valid&&capped.solution.angle<=50);
  auto preset=planThrow({0,8,0},{80,8,0},{},28,.05,2,true,55,true,95,true,false,{},55,{},false,5,NAN,50);
  assert(preset.solution.valid&&preset.solution.angle<=50);
 }
 {for(double center:{8.,17.,34.,52.,71.}){ElevationIntent hold;hold.update(center,0);for(int i=1;i<=100;++i)assert(std::abs(hold.update(center+(i%2?.55:-.55),i*.016)-center)<1e-8);
   assert(hold.update(center+2,2)==center);assert(hold.update(center-2,2.1)==center);assert(hold.update(center+2,2.2)==center);assert(hold.update(center+2,2.45)==center+2);
  }}
 {auto low=solveAngle({0,8,0},{25,6.5,0},{},-1,28,.05,5);assert(low.valid&&length(point(low,28,low.time)-Vec{25,6.5,0})<1e-6);}

 {auto flight=solve({0,8,0},{80,8,0},{},95,28,.05,false,5);Defender far{{40,8,30},{},22,7,12,2,2,2};
  auto clear=riskForecast(flight,28,{far},.05);assert(!clear.intable&&!clear.possible);
  far.position.z=12;auto possible=riskForecast(flight,28,{far},.05);assert(!possible.intable&&possible.possible);
  far.position.z=0;auto overlap=riskForecast(flight,28,{far},.05);assert(overlap.intable&&overlap.projected.time>0);
  for(double y:{5.,10.,18.}){double radius=reachSectionRadius(far,y,.4);if(radius>=0){assert(std::abs(reachGap(far,{far.position.x+radius,y,far.position.z},.4))<1e-7);}}
 }
 {Vec origin{0,8,0},target{65,8,0};Defender wr{target,{},20,7,12};
  assert(chooseRoutePass(origin,target,{}, {})==RoutePass::Bullet);
  auto stationaryBullet=planRoutePass(RoutePass::Bullet,origin,target,{},28,.05,{},true,&wr,65,.016);
  assert(stationaryBullet.solution.valid&&stationaryBullet.power>94.99&&stationaryBullet.solution.angle<=30&&stationaryBullet.offset.y==-1.5);
  auto head=planRoutePass(RoutePass::Head,origin,target,{},28,.05,{},true,&wr);
  assert(head.solution.valid&&length(head.solution.catchPoint-target)<1e-6&&head.solution.velocity.y-28*head.solution.time<0);
  wr.velocity={0,0,20};assert(chooseRoutePass(origin,target,wr.velocity,{})==RoutePass::Bullet);
  auto bullet=planRoutePass(RoutePass::Bullet,origin,target,wr.velocity,28,.05,{},true,&wr);
  assert(bullet.solution.valid&&bullet.power>94.99&&bullet.solution.angle<=30&&std::abs(bullet.solution.catchPoint.y-(target.y-1.5))<1e-6);
  Defender db=wr;db.position.z=-15;assert(chooseRoutePass(origin,target,wr.velocity,{db})==RoutePass::DiveBullet);
  auto dive=planRoutePass(RoutePass::DiveBullet,origin,target,wr.velocity,28,.05,{db},true,&wr);
  assert(dive.solution.valid&&dive.solution.angle<=30&&dive.offset.z>=0&&receiverGap(wr,dive.solution.catchPoint,dive.solution.time+.05)<=-.25);
  double previous=55;Plan transition;
  for(int i=0;i<180;++i){transition=planRoutePass(RoutePass::Bullet,origin,target,wr.velocity,28,.05,{},true,&wr,previous,.016);assert(transition.solution.valid);previous=transition.solution.angle;}
  assert(transition.power>94.9&&transition.solution.angle<5);
  RoutePassLatch latch;assert(latch.update(RoutePass::Head,0)==RoutePass::Head);for(int i=1;i<20;++i)assert(latch.update(i%2?RoutePass::Bullet:RoutePass::Head,i*.05)==RoutePass::Head);
 }
 {BeamMemory memory;Vec origin{0,8,0},target{80,8,0};auto prior=solveAngle(origin,target,{},40,28,.05,5);double power=length(prior.velocity);bool lob=false;
  memory.goal={prior,{},power,10,false,false};
  // A genuinely better max-power throw may replace the old 40-degree arc,
  // while retaining the endpoint and the established angular slew limit.
  for(int i=0;i<120;++i){
   auto held=planCoverageThrow(origin,target,{},28,.05,{},true,true,7,196.2,{},power,lob,prior.angle,false,.016,nullptr,0,&memory,1+i*.016);
   assert(held.solution.valid&&std::abs(held.solution.angle-prior.angle)<=45*.016+1e-6&&length(point(held.solution,28,held.solution.time)-target)<1e-6);prior=held.solution;power=held.power;lob=held.lob;
  }
  assert(power>94.99);
 }

 {RouteHistory still;for(int i=0;i<30;++i)still.update(i*.05,{0,8,0},{50+(i%2?.01:-.01),8,0},{i%2?1.:-1.,0,0});assert(length(still.movement)==0);}
 {BeamMemory memory;Plan low,high;low.solution.valid=high.solution.valid=true;low.power=95;low.solution.angle=20;high.power=60;high.solution.angle=65;high.lob=true;
  assert(memory.accept(low,{},1).power==95);
  assert(memory.accept(high,low,1.1).power==95);assert(memory.accept(low,low,1.2).power==95);
  assert(memory.accept(high,low,1.3).power==95);assert(memory.accept(high,low,1.4).power==95);assert(memory.accept(high,low,1.55).power==60);
 }
 {BeamMemory memory;Vec origin{0,8,0},target{100,8,0};auto prior=solve(origin,target,{},95,28,.05,false,5);
  memory.goal={solve(origin,target+Vec{0,1,0},{},95,28,.05,false,5),{0,1,0},95,10,false,false};Vec offset{};
  for(int i=0;i<7;++i){auto next=planCoverageThrow(origin,target,{},28,.05,{},true,true,7,196.2,offset,95,false,prior.angle,false,.016,nullptr,0,&memory,1+i*.016);
   assert(next.solution.valid&&next.offset.y>=offset.y&&next.offset.y-offset.y<=.192001);offset=next.offset;prior=next.solution;
  }assert(std::abs(offset.y)<1e-6); // An unnecessary cached height loses to the fresh head placement.
  memory.goal={solve(origin,target,{},95,28,.05,true,5),{},95,10,false,true};double power=95;bool lob=false;prior=solve(origin,target,{},95,28,.05,false,5);
  for(int i=0;i<180;++i){auto next=planCoverageThrow(origin,target,{},28,.05,{},true,true,7,196.2,{},power,lob,prior.angle,false,.016,nullptr,0,&memory,2+i*.016);
   assert(next.solution.valid&&std::abs(next.solution.angle-prior.angle)<=.72001&&length(point(next.solution,28,next.solution.time)-target)<1e-6);
   prior=next.solution;power=next.power;lob=next.lob;
  }assert(power>94.9&&!lob); // An old high-root goal cannot force a slower throw in an open lane.
 }
 {Defender still{{100,8,0},{},20,6.37,12,5.88,3.88,4.88};
  assert(receiverGap(still,{104,8,0},1)<0);
  assert(receiverGap(still,{110,8,0},1)>0&&reachGap(still,{110,8,0},1)<0);
  auto catchArc=solve({0,8,0},{100,16,0},{},95,28,.05,false,5);auto full=groundFlight(catchArc,28,0);
  assert(full.valid&&full.time>catchArc.time&&full.catchPoint.x>catchArc.catchPoint.x);
  assert(std::abs(full.catchPoint.y-.6)<1e-7&&length(full.velocity-catchArc.velocity)<1e-9);
 }
 {Defender wr{{30,8,0},{},20,6.37,12,5.88,3.88,4.88};std::vector<Defender> overlapping{wr};
  auto p=planCoverageThrow({0,8,0},wr.position,{},28,.06,overlapping,true,true,6.37,196.2,{},35,true,80,true,.1,&wr);
  auto direct=solve({0,8,0},wr.position,{},95,28,.06,false,5);
  assert(p.solution.valid&&!estimatedClear(p,true));
  assert(p.margin>=coverageMargin(direct,28,overlapping,.06)-.25);
  auto open=planCoverageThrow({0,8,0},{100,8,0},{},28,.06,{},true,true,6.37,196.2,{},95,true,80,true,.1);
  assert(open.solution.valid&&open.power==95&&!open.lob&&open.solution.angle<20);
 }
 // A DB can resume running; a missed receiver catch does not end coverage.
 assert(movementPotential(0,{})==22&&movementPotential(0,{},30)==30);
 assert(movementPotential(20,{43,0,0})==22);
 assert(movementPotential(20,{},movementPotential(20,{43,0,0}))==22);
 {Defender dive{{0,8,0},{43,0,0},22,7,12,5.88,3.88,4.88};
  assert(std::abs(horizontalTravel(dive,.1)-4.3)<1e-8);
  assert(std::abs(horizontalTravel(dive,2)-56)<1e-8);
  dive.velocity={};assert(std::abs(horizontalTravel(dive,2)-56)<1e-8);
 }

 Defender stopped{{0,8,0},{},0,0,0,2,2,2};
 assert(reachGap(stopped,{10,8,0},.5)>0);stopped.speed=movementPotential(0,{});
 assert(reachGap(stopped,{10,8,0},.5)<0);
 {
  auto s=solve({0,8,0},{70,16,0},{},95,28,.05,false,5);
  Defender db{{125,8,0},{},22,7,12,5.88,3.88,4.88};std::vector<Defender> ds{db};
  CoverageTrace trace;
  assert(coverageMargin(s,28,ds,.05)>1);
  assert(coverageMargin(s,28,ds,.05,&trace,-INFINITY,0)<0&&trace.time>s.time);
  auto risk=riskForecast(s,28,ds,.05,0);assert(risk.intable&&risk.projected.time>s.time);
  Defender wr{{70,8,0},{},20,6.37,12,5.88,3.88,4.88,196.2,true};
  assert(receiverGap(wr,{60,8,0},1)<0&&receiverGap(wr,{60,8,0},.1)>0);
  auto offsets=fitOffsets({0,8,0},wr.position,{},ds,true);
  assert(std::any_of(offsets.begin(),offsets.end(),[](Vec v){return v.x<-9&&std::abs(v.z)<.01;}));
  auto fit=planCoverageThrow({0,8,0},wr.position,{},28,.05,ds,true,true,6.37,196.2,{},0,false,NAN,true,.1,&wr,0,nullptr,0,0);
  assert(fit.solution.valid&&receiverGap(wr,fit.solution.catchPoint,fit.solution.time+.05)<=-.25);
  assert(fit.margin>1&&std::abs(fit.margin-coverageMargin(fit.solution,28,ds,.05,nullptr,-INFINITY,0))<1e-8);
  Plan unsafe=fit;unsafe.margin=-2;unsafe.coverageKnown=true;BeamMemory memory;memory.goal=unsafe;
  auto safe=fit;safe.power=unsafe.power-10;safe.solution.angle=unsafe.solution.angle+10;
  assert(!continuitySafe(unsafe,safe,true));assert(memory.accept(safe,unsafe,1,true).margin==safe.margin);
  // A live search is split into bounded tiers; refinement is revalidated and
  // restarts when the scene changes instead of carrying an old target over.
  auto closeDb=wr;closeDb.redirect=false;closeDb.position.z=5;closeDb.speed=22;BeamMemory scan;
  auto first=planCoverageThrow({0,8,0},wr.position,{},28,.05,{closeDb},true,true,6.37,196.2,{},0,false,NAN,true,.1,&wr,0,&scan,1,0);
  assert(first.solution.valid&&scan.scanActive&&scan.scanIndex==3&&scan.nextPower==powerSearchOrder()[3]);
  auto next=planCoverageThrow({4,8,0},wr.position,{},28,.05,{closeDb},true,true,6.37,196.2,{},0,false,NAN,true,.1,&wr,0,&scan,1.1,0);
  assert(next.solution.valid&&scan.scanActive&&scan.scanIndex==6&&scan.nextPower==powerSearchOrder()[6]);
  wr.position.x+=10;
  auto changed=planCoverageThrow({0,8,0},wr.position,{},28,.05,{closeDb},true,true,6.37,196.2,{},0,false,NAN,true,.1,&wr,0,&scan,1.2,0);
  assert(changed.solution.valid&&scan.scanIndex==3&&scan.nextPower==powerSearchOrder()[3]);
 }

 // Saved contested scene: a measured R6 receiver can adjust to a clear side
 // placement. Search both modes and verify that dispatch reconstructs the arc.
 {Vec origin={-70.8704071044922,7.48999643325806,100.438293457031},target={-65.8327345697048,7.45495080947876,150.687549360086},movement={0,0,0};
  Defender receiver{{-66.0322418212891,7.45495080947876,148.697525024414},movement,20,6.37105,12,5.8818646,3.8818646,4.8818646,196.2,true};
  std::vector<Defender> defenders={{{-86.9555511474609,14.637001991272,231.272689819336},{0,-9.13159942626953,0},43.0147566692609,6.37105004812896,12,5.88186461588846,3.88186461588846,4.88186461588846,196.199996948242},
   {{111.247787475586,7.48837280273438,231.698852539062},{0,0,0},22,6.37105004812896,12,5.88186461588846,3.88186461588846,4.88186461588846,196.199996948242},
   {{-79.8101272583008,7.48929023742676,218.585922241211},{11.2748384475708,0,-16.5210952758789},22,6.37105004812896,12,5.88186461588846,3.88186461588846,4.88186461588846,196.199996948242},
   {{135.899673461914,7.49162292480469,146.835006713867},{17.0625190734863,0,-9.79322719573975},22,6.37105004812896,12,5.88186461588846,3.88186461588846,4.88186461588846,196.199996948242},
   {{48.0192680358887,7.49009895324707,65.1686477661133},{-12.0094966888428,0,5.14325475692749},43.0752261336546,6.37105004812896,12,5.88186461588846,3.88186461588846,4.88186461588846,196.199996948242},
   {{77.5282287597656,7.49098443984985,90.4707565307617},{-5.20614004135132,0,19.1592140197754},42.9854825018803,6.37105004812896,12,5.88186461588846,3.88186461588846,4.88186461588846,196.199996948242},
   {{144.236129760742,7.48837089538574,133.940795898438},{0,0,0},22,6.37105004812896,12,5.88186461588846,3.88186461588846,4.88186461588846,196.199996948242},
   {{43.6706809997559,7.48627042770386,125.112358093262},{0,0,0},22,6.37105004812896,12,5.88186461588846,3.88186461588846,4.88186461588846,196.199996948242},
   {{107.361015319824,7.48837280273438,234.56217956543},{0,0,0},43.0151565673506,6.37105004812896,12,5.88186461588846,3.88186461588846,4.88186461588846,196.199996948242},
   {{11.5425224304199,7.56842613220215,-7.38220930099487},{0,0,0},22,6.37105004812896,12,6,6,8,196.199996948242},
   {{165.658782958984,7.49227905273438,343.208374023438},{0,0,0},22,6.37105004812896,12,5.88186461588846,3.88186461588846,4.88186461588846,196.199996948242},
   {{165.627532958984,7.49227905273438,348.669372558594},{0,0,0},22,6.37105004812896,12,5.88186461588846,3.88186461588846,4.88186461588846,196.199996948242},
   {{37.1294708251953,9.73839378356934,252.371032714844},{0,-0.00174898467957973,0},22,6.37105004812896,12,5.88186461588846,3.88186461588846,4.88186461588846,196.199996948242},
   {{38.066276550293,3.63974213600159,250.148880004883},{0,0,0},22,6.37105004812896,12,5.88186461588846,3.88186461588846,4.88186461588846,196.199996948242}};
  for(bool beam:{false,true}){BeamMemory memory;Plan result;
   for(int tick=0;tick<5;++tick){result=planCoverageThrow(origin,target,movement,28,0.0651110000610352,defenders,true,beam,receiver.jumpHeight,196.2,{},0,false,NAN,true,.1,&receiver,0,&memory,1+tick*.1,2.99171543121338);if(estimatedClear(result,true))break;}
   assert(estimatedClear(result,true));
   assert(receiverGap(receiver,result.solution.catchPoint,result.solution.time+0.0651110000610352)<=-.25);
   auto sent=solve(origin,target+result.offset,movement,result.power,28,0.0651110000610352,result.lob,5);
   assert(sent.valid&&length(sent.velocity-result.solution.velocity)<1e-5&&std::abs(sent.time-result.solution.time)<1e-5);
  }
 }
 auto divingReceiver=solve({0,8,0},{60,8,0},{0,0,43.5},95,28,.05,false,5);
 assert(divingReceiver.valid&&length(point(divingReceiver,28,divingReceiver.time)-divingReceiver.catchPoint)<1e-6);
 assert(!solve({0,8,0},{60,8,0},{0,0,46},95,28,.05,false,5).valid);
 // Pruning distant catchers must preserve the full sampled-path minimum.
 for(double distance:{15.,60.,150.})for(double speed:{45.,70.,95.})for(bool high:{false,true}){
  auto s=solve({0,8,0},{distance,10,0},{3,0,1},speed,28,.06,high,5);if(!s.valid)continue;
  std::vector<Defender> ds={{{-200,8,80},{},20,6.37,12},{{distance*.5,8,4},{5,0,0},20,6.37,12},{{distance+40,12,0},{},20,6.37,12}};
  double exhaustive=1000;int steps=std::clamp(int(std::ceil(s.time/.02)),32,400);
  for(const auto& d:ds)for(int i=0;i<=steps;++i)exhaustive=std::min(exhaustive,reachGap(d,point(s,28,s.time*i/steps),s.time*i/steps+.06));
  double refined=coverageMargin(s,28,ds,.06); assert(refined<=exhaustive+1e-9);
  double dense=1000;int denseSteps=int(std::ceil(s.time/.001));
  for(const auto& d:ds)for(int i=0;i<=denseSteps;++i)dense=std::min(dense,reachGap(d,point(s,28,s.time*i/denseSteps),s.time*i/denseSteps+.06));
  assert(std::abs(refined-dense)<.05);
 }
 for(bool beam:{false,true})for(double height:{-3.,0.,4.,8.}){
  Defender wr{{100,8,0},{},20,6.37,12,5.88,3.88,4.88};
  auto p=planCoverageThrow({0,8,0},{100,8+height,0},{},28,.05,{},true,beam,6.37,196.2,{},0,false,NAN,true,.1,&wr,height);
  assert(p.solution.valid&&std::abs(p.solution.catchPoint.y-(8+height))<1e-6);
  assert(reachGap(wr,p.solution.catchPoint,p.solution.time+.05)<=-.25);
  if(beam)assert(p.power==95);
 }
 ResumeIntent resume;resume.suspend(true,10);resume.suspend(false,10);assert(resume.take(10));assert(!resume.take(10));resume.suspend(true,10);resume.cancel();assert(!resume.take(10));resume.suspend(true,10);assert(!resume.take(11));
 for(double angle:{12.,25.,35.,55.,75.})for(double distance:{15.,50.,90.,150.})for(double drift:{-10.,0.,10.}){
  Vec origin{0,8,0},target{distance,7,0},movement{drift,0,4};auto s=solveAngle(origin,target,movement,angle,28,.06);
  if(s.valid){assert(length(point(s,28,s.time)-s.catchPoint)<1e-6);assert(length(s.velocity)<=95);assert(std::abs(std::atan2(s.velocity.y,std::hypot(s.velocity.x,s.velocity.z))*180/3.141592653589793-angle)<1e-6);++count;}
 }
 assert(!solveAngle({},{500,0,0},{},5,28,.05).valid);
 assert(!solveAngle({},{40,0,0},{},NAN,28,.05).valid);
 assert(presetAngle(0,{},{160,0,0},{10,0,0})>presetAngle(0,{},{30,0,0},{10,0,0}));
 assert(std::string(routeName({},{80,0,0},{0,0,10}))=="Crossing");
 assert(selectionDistance(100,100,100,120,100,80,8,false)==0);
 assert(selectionDistance(150,100,100,120,100,80,8,false)>40);
 assert(selectionDistance(120,100,100,120,100,80,8,true)<selectionDistance(120,100,100,120,100,80,8,false));
 std::vector<Defender> defenders={{{62,7,3},{0,0,0}}};
 auto baseline=planThrow({0,8,0},{60,7,0},{},28,.05,0,true,35,true,95,false,false,defenders);
 auto fit=planThrow({0,8,0},{60,7,0},{},28,.05,0,true,35,true,95,false,true,defenders);
 assert(baseline.solution.valid&&fit.solution.valid&&fit.margin<=catchSafetyMargin); // Coverage does not veto a reachable throw.
 assert(fit.margin>=baseline.margin-1e-6); // Do not worsen a deeply overlapping reach volume.
 auto past=planCoverageThrow({0,8,0},{60,8,0},{10,0,0},28,.05,{{{55,8,0},{},22,9,12}},true,false,7,196.2);
 assert(past.solution.valid&&past.margin>=coverageMargin(solve({0,8,0},{60,8,0},{10,0,0},95,28,.05,false,5),28,{{{55,8,0},{},22,9,12}},.05)-.025); // Fit can clear a trailing DB above or beyond the WR.
 assert(receiverCanReach(past.offset,{10,0,0},past.solution.time,5.95));
 auto still=planCoverageThrow({0,8,0},{60,8,0},{},28,.05,{{{57,8,0},{},22,9,12}},true,false,7,196.2);
 assert(still.solution.valid&&std::hypot(still.offset.x,still.offset.z)<=4.001);
 auto unknown=planThrow({0,8,0},{60,7,0},{},28,.05,0,true,35,true,95,false,true,{});assert(!unknown.coverageKnown&&length(unknown.offset)==0);
 auto manual=planThrow({0,8,0},{60,7,0},{},28,.05,0,false,35,true,95,false,false,{});assert(manual.solution.valid&&manual.solution.angle==35);
 assert(!planThrow({0,8,0},{340,7,0},{},28,.05,0,false,5,true,95,false,false,{}).solution.valid);
 AngleTracker smoother;assert(smoother.update(20,0)==20);assert(std::abs(smoother.update(55,.1)-22.4)<1e-6);assert(smoother.update(22.5,.2)==22.4);
 RouteHistory history;for(int i=0;i<30;i++){double t=i*.05;history.update(t,{0,7,0},{60+10*t,7,0},{10,0,0});}
 assert(history.route=="Vertical stem");
 for(int i=30;i<60;i++){double t=i*.05;history.update(t,{0,7,0},{74.5,7,12*(t-1.45)},{0,0,12});}
 assert(history.route=="Crossing / out"&&history.mode==4);
 auto high=solveAngle({0,8,0},{60,7,0},{},55,28,.05);assert(high.valid);
 std::vector<Defender> standing={{{45,7,14},{},22,0,0}},athletic={{{45,7,14},{},22,9,12}};
 assert(coverageMargin(high,28,athletic,.05)<=coverageMargin(high,28,standing,.05));
 auto stable=planThrow({0,8,0},{60,7,0},{},28,.05,0,true,35,true,95,false,true,std::vector<Defender>{{{45,7,100},{},22,9,12}},32);
 assert(stable.solution.valid&&stable.solution.angle==32); // Fit does not jump +/-10 degrees.
 // A stopped receiver loses historical lead and fit never moves the head target.
 history.update(3.1,{0,7,0},{74.5,7,18},{0,0,0});history.update(3.2,{0,7,0},{74.5,7,18},{0,0,0});assert(length(history.movement)==0);
 for(double distance:{20.,60.,150.,250.})for(Vec movement:{Vec{},Vec{8,0,2}}){
  const Vec head{distance,8,0};
  auto fast=planThrow({0,8,0},head,movement,28,.05,0,true,35,true,60,false,false,athletic,48,{},true,5);
  assert(fast.solution.valid&&std::abs(fast.power-95)<1e-6);
  assert(length(point(fast.solution,28,fast.solution.time)-fast.solution.catchPoint)<1e-5);
  auto timed=solveTimed({0,8,0},head,movement,fast.solution.time,28,.05);
  assert(timed.valid&&length(point(timed,28,timed.time)-timed.catchPoint)<1e-5&&std::abs(length(timed.velocity)-95)<1e-5);
  assert(std::abs(length(fast.solution.origin-Vec{0,8,0})-5)<1e-6);
  if(length(movement)==0){assert(length(fast.offset)==0);assert(length(fast.solution.catchPoint-head)==0);}
  auto angled=solveAngle({0,8,0},head,movement,35,28,.05,5);
  if(angled.valid)assert(length(point(angled,28,angled.time)-angled.catchPoint)<1e-6);
 }
 auto directAuto=planThrow({0,8,0},{150,8,0},{},28,.05,0,true,35,true,60,false,false,athletic,48,{},true,5);
 assert(directAuto.solution.angle<20); // No arbitrary distance-based lob.
 auto jump95=planCoverageThrow({0,8,0},{100,8,0},{},28,.05,{},true,true,7,196.2);
 assert(jump95.solution.valid&&jump95.power==95&&jump95.offset.y==0);
 assert(jump95.offset.x==0&&jump95.offset.z==0);
 assert(length(point(jump95.solution,28,jump95.solution.time)-jump95.solution.catchPoint)<1e-5);
 auto headBeam=planCoverageThrow({0,8,0},{100,8,0},{},28,.05,{},true,true,0,196.2);
 assert(headBeam.solution.valid&&headBeam.power==95&&headBeam.offset.y==0);
 auto shortBeam=planCoverageThrow({0,8,0},{15,8,0},{},28,.05,{},true,true,7,196.2);
 assert(shortBeam.solution.valid&&shortBeam.solution.angle<5&&!shortBeam.lob&&shortBeam.power==95);
 auto closeBeam=planCoverageThrow({0,8,0},{35,8,0},{},28,.05,{},true,true,7,196.2);
 assert(closeBeam.solution.valid&&closeBeam.power>=90&&closeBeam.solution.angle<=beamMaxAngle);
 auto closeCovered=planCoverageThrow({0,8,0},{35,8,0},{},28,.05,{{{35,8,3},{},22,9,12}},true,true,7,196.2,shortBeam.offset,shortBeam.power,shortBeam.lob,shortBeam.solution.angle);
 for(int i=0;i<12;i++)closeCovered=planCoverageThrow({0,8,0},{35,8,0},{},28,.05,{{{35,8,3},{},22,9,12}},true,true,7,196.2,closeCovered.offset,closeCovered.power,closeCovered.lob,closeCovered.solution.angle);
 assert(closeCovered.solution.valid&&closeCovered.margin<=catchSafetyMargin&&closeCovered.power<=95);
 auto fiftyYards=planCoverageThrow({0,8,0},{150,8,0},{22,0,0},28,.05,{{{80,8,5},{},22,9,12}},true,true,7,196.2);
 assert(fiftyYards.solution.valid&&fiftyYards.solution.angle<=beamMaxAngle);
 // Recompute moving geometry without power roundoff or a retained high root.
 Plan beam;
 for(int distance=20;distance<=400;distance+=2){
  beam=planCoverageThrow({0,8,0},{double(distance),8,0},{8,0,2},28,.05,{{{70,8,8},{},22,9,12}},true,true,7,196.2,beam.offset,beam.power,true,70);
  if(!beam.solution.valid)continue;
  assert(beam.power>=15&&beam.power<=95&&beam.solution.angle<=beamMaxAngle);
  auto low=solve({0,8,0},Vec{double(distance),8,0}+beam.offset,{8,0,2},beam.power,28,.05,beam.lob,5);
  if(!low.valid||std::abs(low.time-beam.solution.time)>=1e-6)std::cerr<<"Arc mismatch at "<<distance<<" power "<<beam.power<<" lob "<<beam.lob<<" time "<<beam.solution.time<<" rebuilt "<<low.time<<" angle "<<beam.solution.angle<<'\n';
  assert(low.valid&&std::abs(low.time-beam.solution.time)<1e-6);
  assert(length(point(beam.solution,28,beam.solution.time)-beam.solution.catchPoint)<1e-5);
 }
 assert(!planCoverageThrow({0,8,0},{500,8,0},{},28,.05,{},true,true,7,196.2).solution.valid);
 auto noJump=planCoverageThrow({0,8,0},{100,8,0},{},28,.05,{},true,false,0,196.2);
 assert(noJump.solution.valid&&noJump.power==95&&noJump.offset.y==0);
 std::vector<Defender> underneath={{{45,8,20},{},22,10,12}};
 auto adaptive=planCoverageThrow({0,8,0},{150,8,0},{},28,.05,underneath,true,false,7,196.2);
 auto forced95=planCoverageThrow({0,8,0},{150,8,0},{},28,.05,underneath,true,true,7,196.2);
 assert(adaptive.solution.valid&&forced95.solution.valid);
 assert(forced95.margin>catchSafetyMargin&&forced95.solution.angle<=beamMaxAngle);
 assert(adaptive.power<=95&&adaptive.margin>catchSafetyMargin);
 auto retained=planCoverageThrow({0,8,0},{150.1,8,0},{},28,.05,underneath,true,false,7,196.2,adaptive.offset,adaptive.power,adaptive.lob,adaptive.solution.angle);
 assert(retained.solution.valid&&std::abs(retained.solution.angle-adaptive.solution.angle)<.2);
 std::vector<Defender> onReceiver={{{100,8,0},{},22,9,12}};
 auto contested=planCoverageThrow({0,8,0},{100,8,0},{},28,.05,onReceiver,true,false,7,196.2);
 assert(contested.solution.valid&&contested.margin<=catchSafetyMargin); // Assist remains active without claiming exclusivity.
 assert(contested.power<=95&&contested.offset.y<=7);
 auto contestedHeld=planCoverageThrow({0,8,0},{100.01,8,0},{},28,.05,onReceiver,true,false,7,196.2,contested.offset,contested.power,contested.lob,contested.solution.angle);
 assert(contestedHeld.solution.valid&&std::abs(contestedHeld.solution.angle-contested.solution.angle)<.1);
 // A newly blocked held path must yield to a materially better placement.
 auto redirected=planCoverageThrow({0,8,0},{150,8,0},{},28,.05,underneath,true,false,7,196.2,forced95.offset,forced95.power,forced95.lob,forced95.solution.angle);
 assert(redirected.solution.valid&&redirected.margin>catchSafetyMargin);
 // A moving receiver still gets an assisted, reachable fit under close coverage.
 for(Vec defender:{Vec{100,8,8},Vec{85,8,15},Vec{60,8,5}}){
  std::vector<Defender> coverage{{defender,{},22,9,12}};
  auto plain=planCoverageThrow({0,8,0},{100,8,0},{8,0,0},28,.05,coverage,false,false,7,196.2);
  auto smart=planCoverageThrow({0,8,0},{100,8,0},{8,0,0},28,.05,coverage,true,false,7,196.2);
  assert(plain.solution.valid&&smart.solution.valid);
  assert(smart.margin>=plain.margin-.025);
  assert(length(point(smart.solution,28,smart.solution.time)-smart.solution.catchPoint)<1e-5);
  assert(receiverCanReach(smart.offset,{8,0,0},smart.solution.time,5.95)&&smart.offset.y<=7);
 }
 auto closeLob=planThrow({0,8,0},{25,8,0},{8,0,0},28,.05,2,true,55,true,95,true,true,{},55,{},false,5);
 assert(closeLob.solution.valid&&closeLob.power<50&&closeLob.solution.angle==55);
 auto continuous=adaptive;
 for(int i=1;i<=80;i++){
  double oldAngle=continuous.solution.angle;auto oldOffset=continuous.offset;
  continuous=planCoverageThrow({0,8,0},{150+i*.01,8,.03*std::sin(i)},{},28,.05,underneath,true,false,7,196.2,continuous.offset,continuous.power,continuous.lob,continuous.solution.angle);
  assert(continuous.solution.valid&&std::abs(continuous.solution.angle-oldAngle)<=4.01&&length(continuous.offset-oldOffset)<=1.21);
 }
 auto nearLaunch=jump95.solution.origin;
 assert(coverageMargin(jump95.solution,28,{{nearLaunch,{},22,0,0}},.05)<=catchSafetyMargin);
 // Open regular throws prioritize power; close covered throws can loft at lower power.
 auto open=planCoverageThrow({0,8,0},{100,8,0},{},28,.05,{},true,false,7,196.2);
 assert(open.solution.valid&&open.power>=90&&open.solution.angle<20);
 auto touch=planCoverageThrow({0,8,0},{20,8,0},{},28,.05,{{{19,8,0},{},22,9,12}},true,true,7,196.2);
 assert(touch.solution.valid&&touch.margin<=catchSafetyMargin&&!estimatedClear(touch,true));
 auto closeRegular=planCoverageThrow({0,8,0},{20,8,0},{},28,.05,{{{19,8,0},{},22,9,12}},true,false,7,196.2);
 assert(closeRegular.solution.valid&&closeRegular.margin<=catchSafetyMargin&&!estimatedClear(closeRegular,true));
 auto resetArc=planCoverageThrow({0,8,0},{100,8,0},{},28,.05,{},true,false,7,196.2,{},50,true,60);
 for(int i=0;i<30;i++){double before=resetArc.solution.angle;resetArc=planCoverageThrow({0,8,0},{100,8,0},{},28,.05,{},true,false,7,196.2,resetArc.offset,resetArc.power,resetArc.lob,resetArc.solution.angle);assert(resetArc.solution.valid&&std::abs(resetArc.solution.angle-before)<=4.01);}
 if(!(resetArc.power>=90&&resetArc.solution.angle<20))std::cerr<<"Reset arc: "<<resetArc.power<<" / "<<resetArc.solution.angle<<'\n';
 assert(resetArc.power>=90&&resetArc.solution.angle<20);
 // Smooth comparable paths, but do not retain a worse/unsafe path solely to
 // satisfy the angle limiter when the defender moves into it.
 auto switching=solveAngle({0,8,0},{150,8,0},{},35,28,.05,5);
 Plan smoothSwitch{switching,{},length(switching.velocity),0,true,false};
 for(int i=0;i<24;i++){
  const double before=smoothSwitch.solution.angle;
  std::vector<Defender> movingCoverage{{{45,8,i%2?12.:30.},{},22,9,12}};
  const double oldMargin=coverageMargin(smoothSwitch.solution,28,movingCoverage,.05);
  smoothSwitch=planCoverageThrow({0,8,0},{150,8,0},{},28,.05,{{{45,8,i%2?12.:30.},{},22,9,12}},true,false,7,196.2,smoothSwitch.offset,smoothSwitch.power,smoothSwitch.lob,before,true,.05);
  assert(smoothSwitch.solution.valid);
  assert(std::abs(smoothSwitch.solution.angle-before)<=1.201||smoothSwitch.margin>oldMargin+.25||(oldMargin<=catchSafetyMargin&&estimatedClear(smoothSwitch,true)));
  assert(length(point(smoothSwitch.solution,28,smoothSwitch.solution.time)-smoothSwitch.solution.catchPoint)<1e-6);
 }
 // Beam selects power before visual continuity, even from a previous weak lob.
 auto powerBeam=planCoverageThrow({0,8,0},{100,8,0},{},28,.05,{},true,true,7,196.2,{},50,true,60,true,.016);
 assert(powerBeam.solution.valid&&powerBeam.power==95&&!powerBeam.lob);
 auto elevatedBeam=planCoverageThrow({0,8,0},{150,8,0},{},28,.05,underneath,true,true,7,196.2,{},40,false,35,true,.016);
 assert(elevatedBeam.power==forced95.power&&estimatedClear(elevatedBeam,true));
 {
  ThrowActor wr;wr.id=4;wr.name="WR";wr.valid=wr.sideKnown=true;wr.reach={{100,8,0},{},20,15,12};
  ThrowActor db=wr;db.id=6;db.name="DB";db.opponent=true;db.reach.jumpHeight=0;db.reach.speed=0;db.reach.diveReach=0;
  BallFlight ownBall;ownBall.id=88;ownBall.thrower=3;ownBall.airborne=true;ownBall.born=10.1;ownBall.at=10.2;ownBall.position={0,60,0};ownBall.gravity=28;ownBall.model={true,10.2,28,0,ownBall.position,{50,10,0},10.2};ownBall.fieldY=0;
  ThrowView metrics;flightMetrics(metrics,ownBall,0,{wr,db},4,true);
  assert(metrics.coverageKnown&&!metrics.intable&&std::isfinite(metrics.toSpot)&&std::isfinite(metrics.nearD)); // Overlapping WR/DB positions alone do not mean interceptable.
  auto lowBall=ownBall;lowBall.position={0,8,0};lowBall.model.origin=lowBall.position;lowBall.model.velocity={50,8,0};db.reach.position={50,8,0};
  flightMetrics(metrics,lowBall,0,{wr,db},4,true);assert(metrics.intable&&metrics.margin<=catchSafetyMargin);
  db.opponent=false;flightMetrics(metrics,lowBall,0,{wr,db},4,true);assert(!metrics.intable&&std::isnan(metrics.nearD));
  db.sideKnown=false;flightMetrics(metrics,lowBall,0,{wr,db},4,true);assert(metrics.coverageKnown&&metrics.intable); // Teamless practice players remain potential catchers.
  db.sideKnown=db.opponent=true;
  {auto late=ownBall;late.position={0,40,0};late.model.origin=late.position;late.model.velocity={30,0,0};auto farWr=wr;farWr.reach={{200,8,0},{},0,0,0};auto lateDb=db;lateDb.reach={{45,5,0},{},0,0,0};
   flightMetrics(metrics,late,0,{farWr,lateDb},4,true,35);
   assert(metrics.intable&&metrics.eta>1.5); // A missed intended height cannot hide a DB later in the actual path.
  }
  ThrowIntent intent{10,1,2,3,4,5,10,95,35,2,100,12,"WR","Beam"};
  ThrowTracker shot;shot.begin(intent,{}, {wr,db});
  auto foreign=ownBall;foreign.id=99;foreign.thrower=900;
  shot.update(10.2,1,2,{foreign},{wr,db},0,true);assert(shot.view.phase==ThrowView::Pending&&!shot.ball);
  shot.update(10.25,1,2,{foreign,ownBall},{wr,db},0,true);assert(shot.view.phase==ThrowView::Flight&&shot.ball==88&&shot.view.name=="WR");
  auto caught=wr;caught.ball=88;
  shot.update(10.3,1,2,{}, {caught,db},0,true);assert(shot.view.phase==ThrowView::Done&&shot.view.result=="Caught");
  shot.update(13,1,2,{}, {wr,db},0,true);assert(shot.view.phase==ThrowView::Hidden);
  {auto predicted=intent;predicted.predictionKnown=predicted.predictedIntable=true;ThrowTracker retained;retained.begin(predicted,{}, {wr,db});
   retained.update(10.2,1,2,{ownBall},{wr},0,true);assert(retained.view.coverageKnown&&retained.view.intable); // Passing the DB cannot erase the pre-throw forecast.
  }
  shot.begin(intent,{}, {wr,db});shot.update(10.2,1,2,{ownBall},{wr,db},0,true);db.ball=88;
  shot.update(10.3,1,2,{}, {wr,db},0,true);assert(shot.view.result=="Intercepted");db.ball=0;
  shot.begin(intent,{}, {wr,db});shot.update(10.2,1,2,{ownBall},{wr,db},0,true);
  auto groundBall=ownBall;groundBall.landed=groundBall.ended=true;groundBall.position={100,.6,0};
  shot.update(11,1,2,{groundBall},{wr,db},0,true);shot.update(11.2,1,2,{groundBall},{wr,db},0,true);assert(shot.view.result=="Incomplete");
  shot.begin(intent,{}, {wr,db});shot.update(10.2,1,2,{ownBall},{wr,db},0,true);shot.update(11,1,2,{groundBall},{wr,db},0,true);
  shot.update(11.3,1,2,{}, {caught,db},0,true);assert(shot.view.result=="Incomplete"); // Picking it up after ground contact is not a completed pass.
  shot.begin(intent,{}, {wr,db});shot.update(10.2,1,2,{ownBall},{wr,db},0,true);shot.update(10.3,1,2,{}, {wr,db},0,true);shot.update(11,1,2,{}, {wr,db},0,true);assert(shot.view.result=="Result unavailable");
  shot.begin(intent,{ownBall},{wr,db});shot.update(10.2,1,2,{ownBall},{wr,db},0,true);assert(!shot.ball); // Existing nearby own balls are not a new pass.
  shot.begin(intent,{}, {wr,db});auto another=ownBall;another.id=89;shot.update(10.2,1,2,{ownBall,another},{wr,db},0,true);assert(!shot.ball); // Ambiguous ownership does not pick an arbitrary ball.
  {ThrowTracker exact;exact.begin(intent,{}, {wr,db});exact.update(10.2,1,2,{ownBall},{wr,db},0,true);exact.update(10.25,1,2,{foreign,ownBall,another},{wr,db},0,true);assert(exact.ball==88&&exact.view.ballBorn==ownBall.born);}
  {ThrowTracker exact;auto sameHandle=foreign;sameHandle.id=intent.heldBall;exact.begin(intent,{}, {wr,db});exact.update(10.2,1,2,{sameHandle},{wr,db},0,true);assert(!exact.ball);}
  {ThrowTracker exact;exact.begin(intent,{}, {wr,db});exact.update(10.2,1,2,{ownBall},{wr,db},0,true);auto replacement=wr;replacement.ball=89;replacement.thrower=3;replacement.reach.position=ownBall.position;exact.update(10.3,1,2,{}, {replacement,db},0,true);assert(exact.view.phase==ThrowView::Flight&&exact.ball==88);}
  {ThrowTracker exact;exact.begin(intent,{}, {wr,db});exact.update(10.2,1,2,{ownBall},{wr,db},0,true);auto reused=ownBall;reused.born=10.3;exact.update(10.3,1,2,{reused},{wr,db},0,true);assert(exact.view.phase==ThrowView::Hidden);}
  {ThrowTracker exact;exact.begin(intent,{}, {wr,db});exact.update(10.2,1,2,{ownBall},{wr,db},0,true);auto changedOwner=ownBall;changedOwner.thrower=900;exact.update(10.3,1,2,{changedOwner},{wr,db},0,true);assert(exact.view.phase==ThrowView::Hidden);}
  shot.update(10.3,7,2,{}, {wr,db},0,true);assert(shot.view.phase==ThrowView::Hidden);
 }
 // Script-driven balls can retain their launch velocity and be anchored.
 // Forecast must use changing positions, and a stationary stale-velocity ball stays hidden.
 FlightTracker flights;
 for(int i=0;i<=25;i++){double t=i*.05;flights.update({{1,{40*t,10+20*t-14*t*t,0},{40,20,0},28,true},{2,{0,3,0},{95,60,0},28,false}},1+t);}
 auto observed=flights.visible();assert(observed.size()==1&&observed[0].id==1);
 auto fullForecast=forecast(observed[0],0);assert(fullForecast.valid&&fullForecast.path.size()==81);
 assert(std::abs(fullForecast.landing.y-.6)<1e-7&&fullForecast.remaining>.5);
 const double absoluteLanding=1.25+fullForecast.remaining;
 assert(std::abs(fullForecast.landing.x-40*absoluteLanding)<.02);
 assert(!forecast(observed[0],NAN).valid);
 FlightTracker contact=flights;contact.update({{1,{72,.6,0},{40,-30,0},28,true}},2.3,0);assert(contact.visible().empty());
 auto noGravity=observed[0];noGravity.gravity=0;assert(!forecast(noGravity,0).valid);
 assert(length(leadAdjustment({0,8,0},{100,8,0},{0,0,20},-5)-Vec{0,0,-5})<1e-9);
 assert(length(leadAdjustment({0,8,0},{100,8,0},{},-5)-Vec{-5,0,0})<1e-9);
 assert(length(leadAdjustment({0,8,0},{100,8,0},{},0))==0);
 assert(std::abs(observed[0].velocity.x-40)<.01&&std::abs(observed[0].velocity.y-(20-28*1.25))<.01);
 assert(observed[0].trail.size()>10); // Survives the apex with constant raw velocity.
 auto landed=observed[0].position;
 for(int i=1;i<=8;i++)flights.update({{1,landed,{40,20,0},28,true}},2.25+i*.05);
 assert(flights.visible().empty());
 flights.update({},3);assert(flights.flights.empty()); // Catch/removal clears history.
 for(int i=0;i<8;i++)flights.update({{3,{i*1.25,1,0},{25,-20,0},28,false}},4+i*.05);
 assert(flights.visible().empty()); // Level rolling motion is no longer airborne.
 // Replication jitter must not rotate the full predicted landing each frame.
 FlightTracker jitter;Vec fixedLanding{};bool lockedForecast=false;int forecastCount=0;
 for(int i=0;i<=100;i++){
  double t=i*.02,stamp=std::floor(t/.05)*.05;Vec p{70*stamp,20+45*stamp-14*stamp*stamp,12*stamp};
  jitter.update({{77,p,{70,45,12},28,true}},10+t,0);
  for(const auto& ball:jitter.visible()){
   auto prediction=forecast(ball,std::sin(t*7)*.6);
   if(prediction.valid){if(!lockedForecast){fixedLanding=prediction.landing;lockedForecast=true;}else assert(length(prediction.landing-fixedLanding)<1e-5);++forecastCount;}
  }
 }
 assert(forecastCount>50);
 // Delivery times can accelerate/decelerate independently of actual motion.
 // Geometry must recover the true landing despite those changing delays.
 std::vector<FlightSample> delayed;
 for(int i=0;i<=12;i++){double t=i*.04,arrival=20+t+.035*std::sin(i*.55);delayed.push_back({arrival,{70*t,20+45*t-14*t*t,12*t}});}
 auto spatial=fitFlight(delayed,28);assert(spatial.valid);
 BallFlight delayedBall;delayedBall.model=spatial;delayedBall.gravity=28;delayedBall.at=delayed.back().at;delayedBall.fieldY=0;
 auto actual=forecast(delayedBall,0);double landingTime=(45+std::sqrt(45*45+56*19.4))/28;
 assert(actual.valid&&length(actual.landing-Vec{70*landingTime,.6,12*landingTime})<.01);
 auto originalPath=actual.path;delayedBall.at+=.2;auto advanced=forecast(delayedBall,0);
 for(size_t i=0;i<originalPath.size();i++)assert(length(originalPath[i]-advanced.path[i])<1e-7);
 FlightTracker shortFlight;shortFlight.update({{78,{0,10,0},{},28,true}},0,0);shortFlight.update({{78,{5,12,0},{},28,true}},.05,0);
 assert(!forecast(shortFlight.visible().front(),0).valid); // No two-sample long-range projection.
 auto prior=jitter.visible().front();jitter.update({{77,prior.position+Vec{50,0,0},{},28,true}},12.05,0);assert(jitter.visible().empty()||!forecast(jitter.visible().front(),0).valid);
 // Reproduce the observed 65->70->75 staircase with a receiver running away.
 Plan chase;
 for(int i=0;i<30;i++){double distance=100+i*2.;chase=planCoverageThrow({0,8,0},{distance,8,0},{20,0,0},28,.05,{{{distance-2,8,0},{20,0,0},23,8.37,12}},true,true,6.37,196.2,chase.offset,chase.power,chase.lob,chase.solution.angle);
  assert(chase.solution.valid&&chase.power<=95&&chase.solution.angle<=beamMaxAngle&&chase.margin<=catchSafetyMargin);
 }

 // Previously Beam rejected every arc above 35 degrees and sent a contested
 // 95-power pass. This scene has a clear 65-power high arc over the DB.
 std::vector<Defender> middle{{{40,8,0},{},22,9,12}};
 auto blockedLow=solve({0,8,0},{150,8,0},{},95,28,.05,false,5);
 assert(coverageMargin(blockedLow,28,middle,.05)<=catchSafetyMargin);
 auto over=planCoverageThrow({0,8,0},{150,8,0},{},28,.05,middle,true,true,7,196.2);
 assert(over.solution.valid&&over.power<95&&over.solution.angle>35&&over.margin>catchSafetyMargin&&over.lob);
 assert(length(point(over.solution,28,over.solution.time)-over.solution.catchPoint)<1e-5);
 // Increasing flight time cannot make identical receiver/defender reach
 // exclusive. This must never become an estimated-clear result.
 for(bool beamMode:{false,true}){
  auto overlap=planCoverageThrow({0,8,0},{25,8,0},{},28,.05,{{{25,8,0},{},22,9,12}},true,beamMode,7,196.2);
  assert(overlap.solution.valid&&!estimatedClear(overlap,true));
 }
 SelectionLatch select;assert(select.update(1,false,0)==1);
 // Dimensions from the bounded R6 catch-part check, shared by WR and DB.
 auto geometry=catchGeometry({2,2,1},{2,1,1},{1,2,1},{1.4,1.65,1.4},.6);
 assert(geometry.measured&&geometry.radius>5.8&&geometry.radius<5.9&&geometry.above>3.8&&geometry.above<3.9);
 assert(!catchGeometry({NAN,2,1},{2,1,1},{1,2,1},{1.4,1.65,1.4},.6).measured);
 double actualJump=2500./(2*196.2);
 assert(jumpRise(actualJump,196.2,.01)<.51&&std::abs(jumpRise(actualJump,196.2,.3)-actualJump)<1e-8);
 Defender reach{{0,0,0},{},20,0,12,geometry.radius,geometry.above,geometry.below,196.2};
 assert(std::abs(reachGap(reach,{geometry.radius+20*.2+25*.1,-.5,0},.2))<1e-6); // Dive replaces sprint, not both at full speed.
 assert(reachGap(reach,{geometry.radius,geometry.above,0},0)>0); // Cannot extend an arm fully in two directions.
 Vec closeOrigin{11.364845275879,7.489995002747,-15.778594970703},closeTarget{0,7.488372802734,0};
 Defender wr{closeTarget,{},20,actualJump,12,geometry.radius,geometry.above,geometry.below,196.2};
 Defender db=wr;db.position.z=-5.843841552735;Defender other=db;other.position={1.52346,7.48837,34.29739};
 std::vector<Defender> closeScene{db,other};
 auto window=planCoverageThrow(closeOrigin,closeTarget,{},28,.0624565,closeScene,true,true,actualJump,196.2,{},0,false,NAN,true,.1,&wr);
 assert(window.solution.valid&&placementAllowed(window.offset,{},window.solution.time));
 assert(std::hypot(window.offset.x,window.offset.z)<=3.000001&&reachGap(wr,window.solution.catchPoint,window.solution.time+.0624565)<-.25);
 CoverageTrace threat;coverageMargin(window.solution,28,closeScene,.0624565,&threat);assert(threat.catcher>=0&&threat.time>=0&&threat.time<=window.solution.time);
 auto rebuilt=solve(closeOrigin,closeTarget+window.offset,{},window.power,28,.0624565,window.lob,5);
 assert(rebuilt.valid&&length(rebuilt.catchPoint-window.solution.catchPoint)<1e-5&&std::abs(rebuilt.time-window.solution.time)<1e-6);
 auto heldWindow=planCoverageThrow(closeOrigin,closeTarget,{},28,.0624565,closeScene,true,true,actualJump,196.2,window.offset,window.power,window.lob,window.solution.angle,true,.1,&wr);
 assert(length(heldWindow.offset-window.offset)<1e-6&&heldWindow.power==window.power);
 auto overlapping=planCoverageThrow(closeOrigin,closeTarget,{},28,.0624565,{wr},true,true,actualJump,196.2,{},0,false,NAN,true,.1,&wr);
 assert(!estimatedClear(overlapping,true));
 // The former "clear" fit required an uncommanded 17-stud sprint/dive.
 // It must not survive retention, even though it is theoretically reachable.
 auto corrected=planCoverageThrow(closeOrigin,closeTarget,{},28,.0624565,closeScene,true,true,actualJump,196.2,{11.9801,6.33939,11.9801},95,false,14.553,true,.1,&wr);
 assert(corrected.solution.valid&&placementAllowed(corrected.offset,{},corrected.solution.time));
 assert(!placementAllowed({11.9801,6.33939,11.9801},{},.25));
 for(Vec movement:{Vec{},Vec{0,0,20}})for(double distance:{20.,60.,150.}){
  Defender receiver{{distance,8,0},movement,20,actualJump,12,geometry.radius,geometry.above,geometry.below,196.2};
  auto defender=receiver;defender.position.z-=6;
  auto bounded=planCoverageThrow({0,8,0},receiver.position,movement,28,.06,{defender},true,true,actualJump,196.2,{30,0,40},95,false,15,true,.1,&receiver);
  if(bounded.solution.valid)assert(placementAllowed(bounded.offset,movement,bounded.solution.time)&&std::hypot(bounded.offset.x,bounded.offset.z)<=6.000001);
 }
 // Go-route sketch: two DBs to the right and one underneath-left. Adjust the
 // landing slightly left/short while clearing every DB, not just the nearest.
 Defender routeWr{{0,8,100},{0,0,20},20,actualJump,12,geometry.radius,geometry.above,geometry.below,196.2};
 auto rightDb=routeWr,trailDb=routeWr,underDb=routeWr;
 rightDb.position={40,8,100};trailDb.position={50,8,80};underDb.position={-35,8,45};
 std::vector<Defender> routeCoverage{rightDb,trailDb,underDb};
 auto originalLanding=solve({0,8,0},routeWr.position,routeWr.velocity,95,28,.06,false,5);
 assert(coverageMargin(originalLanding,28,routeCoverage,.06)<=catchSafetyMargin);
 auto sideFit=planCoverageThrow({0,8,0},routeWr.position,routeWr.velocity,28,.06,routeCoverage,true,true,actualJump,196.2,{},0,false,NAN,true,.1,&routeWr);
 assert(estimatedClear(sideFit,true)&&sideFit.power==95&&sideFit.offset.x<0&&sideFit.offset.z<0);
 assert(placementAllowed(sideFit.offset,routeWr.velocity,sideFit.solution.time));
 assert(sideFit.robustMargin>0&&sideFit.receiverMargin>=1);
 assert(receiverGap(routeWr,sideFit.solution.catchPoint,sideFit.solution.time+.06)<-.25);
 assert(select.update(2,true,.05)==1);assert(select.update(1,true,.1)==1);
 assert(select.update(2,true,.15)==1);assert(select.update(2,true,.32)==2);
 assert(select.update(3,true,.33,true)==3);assert(select.update(4,false,.34)==4);
 CurveTracker display;auto lowDisplay=solve({0,8,0},{80,8,0},{},95,28,.05,false,5);
 auto highDisplay=solve({0,8,0},{80,8,0},{},60,28,.05,true,5);
 {CurveTracker anchored;Vec head{0,8,0};anchored.update(lowDisplay,28,1,1,head);auto start=anchored.points.front();
  anchored.update(highDisplay,28,1,1.016,head);
  assert(length(anchored.points.front()-start)<length(highDisplay.origin-start)*.2); // Angle changes cannot teleport the launch point.
 }
 display.update(lowDisplay,28,1,0);auto initial=display.points[32];
 display.update(highDisplay,28,1,.016);assert(display.settling&&length(display.points[32]-initial)<length(point(highDisplay,28,highDisplay.time*.5)-initial)*.3);
 for(int i=2;i<120;i++)display.update(highDisplay,28,1,i*.016);
 assert(!display.settling&&length(display.points[32]-point(highDisplay,28,highDisplay.time*.5))<.01);
 display.update(lowDisplay,28,2,1);assert(!display.settling&&length(display.points[32]-initial)<1e-6);
 // Continuous receiver/QB translation must not leave the display perpetually
 // settling. Launch translation stays attached while new placement eases.
 for(int i=1;i<=120;i++){
  auto moving=lowDisplay;Vec shift{0,0,20*i*.016};moving.origin=moving.origin+shift;moving.catchPoint=moving.catchPoint+shift;
  display.update(moving,28,2,1+i*.016);
  assert(!display.settling&&length(display.points[32]-point(moving,28,moving.time*.5))<1e-6);
 }
 display.update(highDisplay,28,2,2.936);
 assert(length(display.points.front()-highDisplay.origin)<1e-6);
 auto before=display.points[32];display.update({},28,2,2.95);assert(length(display.points[32]-before)<1e-9);
 auto shifted=highDisplay;shifted.velocity.z+=15;shifted.catchPoint=point(shifted,28,shifted.time);
 auto oldEnd=display.points.back();display.update(shifted,28,2,2.966);
 assert(length(display.points.back()-oldEnd)<length(point(shifted,28,shifted.time)-oldEnd)*.2);


 for(bool visible:{false,true})for(bool right:{false,true})for(bool first:{false,true})assert(cursorModeAllowed(visible,right,first)==(visible&&!right&&!first));
 const float identity[]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};float tilted[]={1,0,0,0,0,.8f,.6f,0,0,0,1,0,0,0,0,1};
 assert(cameraStable(identity,identity));assert(!cameraStable(identity,tilted));tilted[4]=NAN;assert(!cameraStable(identity,tilted));
 // Real repro: hidden old wheel 70 and current visible wheel 75.
 assert(readPower(true,{{1,0,"70"},{1,2,"75"}})==75);
 assert(readPower(true,{{1,2,"75"},{1,0,"70"}})==75);
 assert(readPower(true,{{1,0,"70"}})==0);
 assert(readPower(false,{{1,2,"75"}})==0);
 assert(readPower(true,{{0,2,"75"},{1,2," 95 "}})==95);
 assert(readPower(true,{{1,2,"70"},{1,2,"75"}})==-2);
 for(const auto* text:{"96","0","95x","","nan","-5","100"})assert(readPower(true,{{1,2,text}})==-1);
 assert(powerAvailable(true,true,0));assert(powerAvailable(true,true,-2));assert(!powerAvailable(true,false,0));assert(!powerAvailable(true,false,-2));assert(powerAvailable(true,false,75));assert(!powerAvailable(false,true,75));
 assert(readPower(true,{{1,2,"45"}})==45);assert(readPower(false,{{1,2,"45"}})==0);

 for(double speed:{50.,75.,95.})for(double gravity:{28.,40.})for(double distance:{20.,50.,80.})for(double drift:{-12.,0.,12.})for(bool lob:{false,true}){
  Vec origin{0,8,0},target{distance,7,10},v{0,0,drift};auto s=solve(origin,target,v,speed,gravity,.05,lob);
  if(s.valid){assert(length(point(s,gravity,s.time)-s.catchPoint)<1e-5);assert(length(s.catchPoint-(target+v*(s.time+.05)))<1e-5);assert(std::abs(length(s.velocity)-speed)<1e-5);++count;}
 }
 auto direct=solve({0,8,0},{60,7,0},{},95,28,.03),lob=solve({0,8,0},{60,7,0},{},95,28,.03,true);
 for(double speed:{20.,50.,95.})for(double height:{-20.,0.,12.})for(double drift:{-10.,0.,10.}){
  Solution paired;auto low=solve({0,8,0},{60,8+height,0},{drift,0,0},speed,28,.03,false,5,&paired);
  auto separateLow=solve({0,8,0},{60,8+height,0},{drift,0,0},speed,28,.03,false,5);
  auto separateHigh=solve({0,8,0},{60,8+height,0},{drift,0,0},speed,28,.03,true,5);
  assert(low.valid==separateLow.valid&&paired.valid==separateHigh.valid);
  if(low.valid)assert(std::abs(low.time-separateLow.time)<1e-9);
  if(paired.valid)assert(std::abs(paired.time-separateHigh.time)<1e-9);
 }
 assert(direct.valid&&lob.valid&&lob.angle>direct.angle&&lob.time>direct.time);
 assert(!solve({},{500,5,0},{},20,28,0).valid);
 assert(!solve({NAN,0,0},{20,5,0},{},95,28,0).valid);
 assert(!solve({},{20,5,0},{50,0,0},95,28,0).valid);
 assert(!solve({},{20,5,0},{},95,-28,0).valid);
 assert(!solve({},{20,5,0},{},95,28,NAN).valid);
 ClickGate gate;assert(!gate.down(false,1));assert(!gate.up());assert(gate.down(true,2));assert(gate.down(true,2.01));assert(gate.up());assert(gate.take(2.02,true));assert(!gate.take(2.03,true));assert(!gate.up());
 assert(gate.down(true,3));assert(!gate.take(3.2,true));assert(gate.up());
 assert(gate.down(true,4));gate.cancel();assert(!gate.take(4.01,true));assert(gate.up());
 assert(gate.down(true,5));assert(!gate.take(5.01,false));assert(gate.up());
 std::cout<<count<<" ballistic interception cases and invalid-data/click ownership checks passed\n";
}
