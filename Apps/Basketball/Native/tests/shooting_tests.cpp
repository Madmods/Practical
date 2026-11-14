#include "shooting_logic.hpp"
#include <cassert>
#include <iostream>
using namespace shooting;
int main(){
 int pingCases=0;
 for(double speed:{1.,2.5,4.,19.}){
  double previousLow=INFINITY;
  for(int ping=1;ping<=1000;ping++){
   const auto w=pingWindow(ping,speed,true);
   assert(w.minimum<=previousLow+1e-9);previousLow=w.minimum;
   assert(std::abs(w.maximum-w.minimum-4)<1e-9);
   if(ping>=20&&ping<=30)assert(w.minimum==77&&w.maximum==81&&w.valid);
   Options o;o.automatic=true;o.autoMinimum=w.minimum;o.autoMaximum=w.maximum;o.autoWindowValid=w.valid;
   if(w.valid){
    const double low=w.minimum*.01;
    if(low>0)assert(decide(o,{std::nextafter(low,0.),speed,0,2,false,false,false,ping,.1})==Release::None);
    assert(decide(o,{low,speed,0,2,false,false,false,ping,.1})!=Release::None);
    const double delta=ping<20?(ping-20)*.001:ping>30?(ping-30)*.001:0;
    assert(std::abs(low+speed*delta-.77)<1e-9); // equivalent modeled arrival phase
   }else{
    assert(w.outOfRange);o.assist=true;o.minimum=10;o.maximum=20;
    assert(decide(o,{.8,speed,0,2,true,false,true,ping,.1})==Release::None);
   }
   ++pingCases;
  }
 }
 const auto high=pingWindow(80,2.5,true),lowPing=pingWindow(10,2.5,true);
 assert(high.minimum==64.5&&high.maximum==68.5);
 assert(lowPing.minimum==79.5&&lowPing.maximum==83.5);
 for(auto w:{pingWindow(25,2.5,false),pingWindow(0,2.5,true),pingWindow(1001,2.5,true),pingWindow(25,NAN,true),pingWindow(25,-1,true)})assert(!w.valid);
 std::cout<<pingCases<<" ping-adjusted window cases passed (1-1000ms, four meter speeds)\n";
 Options window;window.automatic=true;window.autoMinimum=BasketballAutoMinimum;window.autoMaximum=BasketballAutoMaximum;
 int floorCases=0;
 for(bool assist:{false,true})for(bool pending:{false,true})for(bool falling:{false,true})
 for(int ping:{20,80,200,1000})for(double speed:{1.,2.5,4.,19.})for(double age:{0.,.02,.12}){
  window.assist=assist;window.minimum=67;window.maximum=70;window.closeout=true;
  const double rate=falling?-speed:speed;
  for(double fill:{0.,.50,.67,.70,.74,.75,.76,.769,std::nextafter(.77,0.)}){
   assert(decide(window,{fill,rate,age,1.2,pending,falling,true,ping,.1})==Release::None);++floorCases;
  }
 }
 window.assist=false;window.closeout=false;
 assert(decide(window,{.77,2.5,0,1.2})!=Release::None);
 assert(decide(window,{.81,2.5,0,0})==Release::AutoMaximum);
 assert(decide(window,{.84,2.5,0,0})==Release::AutoMaximum); // skipped window still releases promptly
 window.automatic=false;window.assist=true;window.minimum=67;window.maximum=70;
 assert(decide(window,{.70,2.5,0,0})==Release::Maximum); // opt-out preserves standalone manual window
 std::cout<<floorCases<<" strict observed 77% floor cases passed\n";
 Options capped;capped.automatic=true;capped.autoMaximum=BasketballAutoMaximum;
 assert(decide(capped,{.76,2.5,0,0})==Release::None);
 assert(decide(capped,{.805,2.5,0,0})==Release::AutoMaximum);
 assert(decide(capped,{.78,2.5,.01,0})==Release::AutoMaximum);
 assert(decide(capped,{.81,2.5,0,0})==Release::AutoMaximum);
 assert(decide(capped,{.84,2.5,0,0})==Release::AutoMaximum); // skipped update: do not keep holding
 capped.assist=true;capped.minimum=95;
 assert(decide(capped,{.81,2.5,0,0,true})==Release::AutoMaximum); // auto ceiling wins over manual floor
 capped.assist=false;
 assert(decide(capped,{.81,2.5,.121,0})==Release::None);
 assert(decide(capped,{.81,-2.5,0,0,false,true})==Release::None);
 assert(decide(capped,{.81,2.5,0,0,false,false,false,1001})==Release::None);
 assert(decide(capped,{.81,NAN,0,0})==Release::None);
 capped.automatic=false;
 assert(decide(capped,{.84,2.5,0,0})==Release::None); // disabled Auto Time never enforces its ceiling
 capped.automatic=true;
 assert(decide(capped,{.70,2.5,0,.15})==Release::Auto); // predictor can still send earlier
 int ceilingCases=0;
 for(int fps:{30,60,90,120,144,165,240,360,500})for(double speed:{1.5,2.5,4.})for(bool jitter:{false,true}){
  MeterTrend trend;int frame=-1;double stamp=0;bool sent=false;
  for(int tick=0;tick<1000;++tick){
   const double t=tick*PollSeconds;int current=int(std::floor(t*fps));
   if(current!=frame){frame=current;stamp=t;double fill=speed*(double(frame)/fps);if(jitter)fill+=.0005*std::sin(frame*1.7);trend.add(stamp,std::clamp(fill,0.,1.));}
   if(trend.active&&trend.estimate.valid){
    auto decision=decide(capped,{trend.samples.back().second,trend.estimate.rate,t-stamp,0});
    if(decision!=Release::None){
     assert(decision==Release::AutoMaximum);
     // Independently check true simulated phase, not merely the stale observed fill.
     assert(speed*t<=.812);assert(speed*t>=.74);sent=true;break;
    }
   }
   if(speed*t>1.05)break;
  }
  assert(sent);++ceilingCases;
 }
 std::cout<<ceilingCases<<" automatic ceiling scenarios passed (30-500 FPS, <=81.2% simulated phase)\n";
 int schedulerCases=0;
 for(int fps:{30,60,144,240})for(double speed:{1.5,2.5,4.})for(double step:{.002,.016,.025,.040}){
  MeterTrend trend;double stamp=0,guard=.016;int frame=-1;bool sent=false;
  for(int tick=0;tick<1000;++tick){
   double t=tick*step;int current=int(std::floor(t*fps));
   if(tick)guard=std::clamp(std::max(guard,2*step+PollSeconds),.016,.1);
   if(current!=frame){frame=current;stamp=t;trend.add(t,std::min(1.,speed*frame/fps));}
   if(trend.active&&trend.estimate.valid&&decide(capped,{trend.samples.back().second,trend.estimate.rate,t-stamp,0,false,false,false,30,guard})!=Release::None){
    if(speed*t>.812)std::cerr<<"Ceiling overshoot: fps="<<fps<<" speed="<<speed<<" poll="<<step<<" phase="<<speed*t<<" rate="<<trend.estimate.rate<<" guard="<<guard<<'\n';
    assert(speed*t<=.812);sent=true;break;
   }
   if(speed*t>1.05)break;
  }
  assert(sent);++schedulerCases;
 }
 std::cout<<schedulerCases<<" scheduler-delay ceiling cases passed (2-40ms sampling)\n";
 // High frame rates used to discard history before any 50ms fit was possible.
 // Measure delivery error across display rates with the real polling/estimator.
 int timingCases=0;
 for(int fps:{30,60,90,120,144,165,240,360,500})for(double speed:{1.5,2.5,4.})for(bool jitter:{false,true}){
  MeterTrend trend;Options automatic;automatic.automatic=true;int frame=-1;double stamp=0;bool sent=false;
  for(int tick=0;tick<1000;++tick){
   const double t=tick*.002;int current=int(std::floor(t*fps));
   if(current!=frame){frame=current;stamp=t;double fill=speed*(double(frame)/fps);if(jitter)fill+=.0005*std::sin(frame*1.7);trend.add(stamp,std::clamp(fill,0.,1.));}
   if(trend.active&&trend.estimate.valid){
    auto decision=decide(automatic,{trend.samples.back().second,trend.estimate.rate,t-stamp,.07,false,false,false,30});
    if(decision==Release::Auto){double applied=speed*(t+.07);assert(std::abs(applied-1)<.025);sent=true;break;}
   }
   if(speed*t>1.05)break;
  }
  assert(sent);assert(trend.samples.size()<=80);++timingCases;
 }
 MeterTrend stalled;for(int i=0;i<30;i++)stalled.add(i*.005,i*.0125);assert(stalled.estimate.valid);
 stalled.add(.5,.5);assert(!stalled.active&&!stalled.estimate.valid); // old speed cannot bridge a stall
 stalled.add(.49,.51);assert(!stalled.estimate.valid); // clock reversal
 stalled.add(NAN,.52);assert(stalled.samples.empty()&&!stalled.estimate.valid);
 Options invalidOptions;invalidOptions.automatic=invalidOptions.assist=true;invalidOptions.maximum=81;
 for(auto bad:{Sample{NAN,2,0,.1},Sample{.9,2,NAN,.1},Sample{.9,2,-.01,.1},Sample{.9,2,0,NAN},Sample{.9,2,0,-.1}})assert(decide(invalidOptions,bad)==Release::None);
 std::cout<<timingCases<<" frame-rate/jitter timing scenarios passed (30-500 FPS)\n";
 // Captured leftover meter fade followed by a new shot. The old fade must
 // never become this shot's falling state or trigger a low-percent release.
 MeterTrend cycle;Options cycleOptions;cycleOptions.assist=true;cycleOptions.minimum=88;
 double cycleTime=0;
 for(double fill:{.104389,.009934,.007507,.005664,.004267,.002866,.002042,.046613,.073519,.124301,.158415,.206217,.24009}){
  cycleTime+=1./60.;cycle.add(cycleTime,fill);
  assert(!cycle.descent.falling);
  assert(decide(cycleOptions,{fill,2.5,0,.10,false,cycle.descent.falling,false,40})==Release::None);
 }
 assert(cycle.active);
 cycle.add(cycleTime+.02,.9);cycle.add(cycleTime+.04,.98);cycle.add(cycleTime+.06,.94);
 assert(cycle.descent.falling); // a real turnaround in the new shot still works
 cycle.reset();assert(!cycle.active&&!cycle.descent.falling&&cycle.samples.empty());
 assert(deferEarlyRelease(true,true,true,false,.60,.01,80,120));
 assert(!deferEarlyRelease(true,true,true,false,.85,.01,80,120));
 assert(!deferEarlyRelease(true,true,true,false,.60,.20,80,120));
 assert(!deferEarlyRelease(true,true,false,false,.60,.01,80,120));
 assert(!deferEarlyRelease(true,true,true,true,.60,.01,80,120));
 assert(!deferEarlyRelease(false,true,true,false,.60,.01,80,120));
 Options o;Sample s{.85,1,.01,.06,false,false,false,30};
 assert(decide(o,s)==Release::None);
 o.automatic=true;assert(decide(o,s)==Release::None);
 s.fill=.94;assert(decide(o,s)==Release::Auto);
 s.ping=200;o.maxPing=100;assert(decide(o,s)==Release::None);
 s.ping=30;s.age=.13;assert(decide(o,s)==Release::None);
 s.age=.01;s.falling=true;assert(decide(o,s)==Release::None);
 o.automatic=false;o.assist=true;o.minimum=80;o.maximum=95;s={.70,1,0,.06,true,false,false,30};assert(decide(o,s)==Release::None);
 s.fill=.75;assert(decide(o,s)==Release::None);s.fill=.80;assert(decide(o,s)==Release::Early);
 s.pending=false;s.fill=.85;assert(decide(o,s)==Release::None); // inside-window release stays manual
 s.fill=.90;assert(decide(o,s)==Release::None);s.fill=.95;assert(decide(o,s)==Release::Maximum);
 o.maximum=100;s={.83,-1,0,.06,false,true,false,30};assert(decide(o,s)==Release::Falling);s.fill=.803;assert(decide(o,s)==Release::Falling);
 o.protectFall=false;assert(decide(o,s)==Release::None);
 o.protectFall=true;o.minimum=100;s={.95,1,0,.06,false,false,false,30};assert(decide(o,s)==Release::None);s.fill=1;assert(decide(o,s)==Release::Maximum);
 o.assist=false;o.closeout=true;o.maximum=80;s={.93,1,0,.06,false,false,true,30};assert(decide(o,s)==Release::Closeout);
 s.fill=.89;assert(decide(o,s)==Release::None);
 s.fill=.93;s.threat=false;assert(decide(o,s)==Release::None);
 o.assist=true;o.minimum=95;o.maximum=100;s.threat=true;assert(decide(o,s)==Release::None);
 // Regression: high ping, a fast meter, and timing trim must never bypass 80%.
 for(int ping: {20,80,200,500})for(double rate:{1.,4.,10.,19.})for(bool automatic:{false,true})for(bool pending:{false,true}){
  Options limits;limits.assist=true;limits.automatic=automatic;limits.minimum=80;limits.maximum=80;
  for(int p=0;p<80;p++)assert(decide(limits,{p/100.,rate,.002,.019+(ping+60)*.001,pending,false,false,ping})==Release::None);
  assert(decide(limits,{.80,rate,.002,.019+(ping+60)*.001,pending,false,false,ping})!=Release::None);
 }
 // Descending samples must not inherit the positive slope from ascent.
 Descent slow;slow.add(0,1);slow.add(.01,.998);assert(!slow.falling);slow.add(.02,.996);assert(slow.falling&&slow.rate<0);
 for(double speed:{1.,2.,4.,8.})for(double interval:{1./240,1./120,1./60}){
  Descent d;d.add(0,.9);d.add(.02,1);Options protect;protect.assist=true;protect.minimum=80;protect.maximum=100;
  bool released=false;
  for(int i=1;i<200;i++){double t=.02+i*interval,fill=1-speed*i*interval;if(fill<0)break;d.add(t,fill);
   if(d.falling&&decide(protect,{fill,d.rate,0,.08,false,true,false,30})==Release::Falling){assert(fill>=.80);released=true;break;}
  }assert(released);
 }
 // Fast ascent anticipates the peak; disabled and stale samples stay inactive.
 Options protect;protect.assist=true;protect.minimum=80;protect.maximum=100;
 assert(decide(protect,{.85,4,0,.08,false,false,false,30})==Release::Falling); // fast ascent needs pre-turn protection
 protect.protectFall=false;assert(decide(protect,{.85,-4,0,.08,false,true,false,30})==Release::None);
 protect.protectFall=true;assert(decide(protect,{.85,-4,.3,.08,false,true,false,30})==Release::None);
 // Overhold lead affects protection independently of the early-release hold.
 Options lead;lead.assist=true;lead.minimum=80;lead.maximum=100;lead.overholdLead=0;
 Sample fall{.92,-2,0,.04,false,true,false,30};assert(decide(lead,fall)==Release::None);
 lead.overholdLead=40;assert(decide(lead,fall)==Release::Falling);
 fall.falling=false;fall.rate=2;assert(decide(lead,fall)==Release::None);
 fall.pending=true;fall.fill=.79;assert(decide(lead,fall)==Release::None);
 fall.fill=.8;assert(decide(lead,fall)==Release::Early);
 Descent accelerating;accelerating.add(0,1);accelerating.add(.02,.99);accelerating.add(.04,.91);assert(accelerating.rate<=-3.99);
 // 88% floor with a 60-Hz meter and delayed input application. Waiting for
 // the first drop misses the floor; anticipating the peak must send sooner.
 for(double rate:{2.4,2.7}){
  Options live;live.assist=true;live.minimum=88;live.maximum=100;live.overholdLead=40;
  bool sent=false;for(int i=0;i<120;i++){double t=i/60.,fill=std::min(1.,rate*t);
   if(decide(live,{fill,rate,0,.10,false,false,false,40})==Release::Falling){double arrival=t+.075,peak=1/rate;double applied=arrival<peak?rate*arrival:1-rate*(arrival-peak);assert(fill<.88);assert(applied>=.88);sent=true;break;}
  }assert(sent);
 }
 // Simulated delivery uncertainty with 60-Hz meter samples and 2-ms polling.
 // This checks arrival inside the window rather than the outgoing key-up fill.
 for(double rate:{2.,2.4,2.7,3.})for(double latency:{.060,.075,.100}){
  Options arrival;arrival.assist=true;arrival.minimum=88;arrival.maximum=100;
  bool sent=false;
  for(int tick=0;tick<500;++tick){double t=tick*.002,stamp=std::floor(t*60)/60.,fill=rate*stamp;
   if(fill>1)break;
   if(decide(arrival,{fill,rate,t-stamp,.10,false,false,false,40})==Release::Falling){
    double phase=rate*(t+latency),result=phase<=1?phase:2-phase;
    assert(result>=.88&&result<=1);sent=true;break;
   }
  }assert(sent);
 }
 Options manual;manual.assist=true;manual.minimum=88;manual.maximum=100;
 // Arrival forecast removes the observed-minimum bottleneck for overhold only.
 assert(decide(manual,{.873619,2.5,.001,.10,false,false,false,40})==Release::Falling);
 assert(decide(manual,{.873619,2.5,.004,.10,false,false,false,40})==Release::Falling);
 assert(decide(manual,{.873619,2.5,.004,.10,true,false,false,40})==Release::None);
 assert(decide(manual,{.65,2.5,.10,.10,false,false,false,40})==Release::None); // extrapolation capped
 manual.minimum=100;
 assert(decide(manual,{.98,2.5,.01,.10,false,false,false,40})==Release::None);
 manual.minimum=88;
 assert(decide(manual,{.83,2.7,0,.12,true,false,false,60})==Release::None);
 assert(decide(manual,{.88,2.7,0,.12,true,false,false,60})==Release::Early);
 manual.protectFall=false;assert(decide(manual,{.92,2.7,0,.12,false,false,false,60})==Release::None);
 assert(pressure(12)==0);assert(pressure(2)==1);assert(pressure(9)<pressure(7));
 std::vector<Defender>d{{{8,0,0},{-20,0,0},20}};
 assert(estimate({}, {}, d,.2)>estimate({}, {}, d,0));
 assert(estimate({}, {}, {},0)==0);
 assert(estimate({}, {}, {{{2,0,0},{},16}},0)==87);
 std::cout<<"Shooting decision and contest geometry checks passed\n";
}
