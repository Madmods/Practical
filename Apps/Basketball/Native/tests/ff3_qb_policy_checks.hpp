#pragma once
template<class Check> void qbPolicyChecks(Check check){
 using namespace ff3qb;
 for(double ping:{20.,80.,160.,300.}){
  auto practice=throwTiming(ping,false),match=throwTiming(ping,true);
  check(practice.outbound==match.outbound&&practice.replicationLead==0&&practice.tolerance==.025);
  check(std::abs(match.replicationLead-ping*.0005)<1e-10&&match.tolerance>=.05&&match.tolerance<=.15);
  const Vec velocity{20,0,0},observed{100,8,0},origin{0,8,0};
  auto received=solve(origin,observed+velocity*match.replicationLead,velocity,95,28,match.outbound,false,5);
  check(received.valid&&length(received.catchPoint-(observed+velocity*(match.replicationLead+match.outbound+received.time)))<1e-5);
 }
 check(!std::isfinite(throwTiming(NAN,true).outbound)&&!std::isfinite(throwTiming(401,true).outbound));
 check(normalArcMinimum(48,-30)==40&&normalArcMinimum(14,-30)==6&&normalArcMinimum(10,25)==25);
 for(int current=5;current<=95;current+=5)for(int desired=5;desired<=95;desired+=5){
  int value=current,taps=0;bool valid=true;
  while(value!=desired&&taps<19){const auto next=powerKeyStep(value,desired);valid=valid&&next.valid&&next.expected==value+(desired>value?5:-5)&&next.key==unsigned(desired>value?'R':'F');value=next.expected;++taps;}
  check(valid&&value==desired&&taps==std::abs(desired-current)/5);
 }
 check(!powerKeyStep(95,71).valid&&!powerKeyStep(0,70).valid&&powerKeyStep(70,70).key==0);
 check(powerKeyStep(73,43).valid&&powerKeyStep(73,43).expected==68&&powerKeyStep(73,93).expected==78);
 check(powerNeighbors(72,5,95)==std::array<double,2>{70,75}&&powerNeighbors(95,5,73)==std::array<double,2>{93,0});
 for(unsigned key:{unsigned('R'),unsigned('F')}){
  bool held=true;std::vector<bool> edges;auto send=[&](unsigned actual,bool down){check(actual==key);edges.push_back(down);return true;};auto physical=[&](){return held;};
  {PowerKeyPress press(send,physical);check(!press.begin(key)&&edges.empty());}
  held=false;{PowerKeyPress press(send,physical);check(press.begin(key)&&edges==std::vector<bool>{true});check(press.release()&&edges==std::vector<bool>({true,false}));check(press.release()&&edges.size()==2);}
  edges.clear();{PowerKeyPress press(send,physical);check(press.begin(key));}check(edges==std::vector<bool>({true,false}));
  edges.clear();{PowerKeyPress press(send,physical);check(press.begin(key));held=true;}check(edges==std::vector<bool>{true});held=false;
  int ups=0;{PowerKeyPress press([&](unsigned,bool down){return down||++ups>1;},physical);check(press.begin(key));check(press.release()&&ups==2);}
 }
 check(powerStepResult(90,85,90,.005)==PowerStepResult::Waiting&&powerStepResult(90,85,85,.035)==PowerStepResult::Acknowledged);
 check(powerStepResult(90,85,80,.035)==PowerStepResult::Unexpected&&powerStepResult(90,85,90,.3)==PowerStepResult::Timeout);
 {CameraPreparation state;check(state.canPrepare(0)&&!state.ready());state.pauseUntil(150);check(!state.canPrepare(149)&&state.canPrepare(150));
  const auto before=state.clicked();check(state.acknowledge(before)&&state.ready());check(state.shot(200)&&!state.ready()&&state.clicked()==200);
  check(!state.acknowledge(before)&&!state.ready());check(!state.canPrepare(699)&&state.canPrepare(700));state.pauseUntil(300);check(!state.canPrepare(699));
  check(state.acknowledge(200)&&state.ready());state.clear();check(!state.ready());check(!state.shot(800)&&!state.canPrepare(1299));
 }
 for(bool beam:{false,true})for(double anchor:{73.,95.})for(double distance:{70.,185.}){
  const Vec origin{0,8,0};Defender receiver{{distance,8,0},{20,0,0},20,7,12,3,2,4};BeamMemory memory;
  const auto p=planCoverageThrow(origin,receiver.position,receiver.velocity,28,.08,{},true,beam,7,196.2,{},0,false,NAN,true,.1,&receiver,0,&memory,0,0,50,nullptr,25,2,0,40,.08,5,anchor);
  check(p.solution.valid&&powerReachable(p.power,5,anchor));
  check(receiverGap(receiver,p.solution.catchPoint,p.solution.time+.08)<=-.25);
  for(const auto& candidate:memory.leaders)check(powerReachable(candidate.power,5,anchor));
 }
 for(double distance:{70.,185.}){
  const Vec origin{0,8,0};Defender receiver{{distance,8,0},{20,0,0},20,7,12,3,2,4};
  const double desired=presetAngle(0,origin,receiver.position,receiver.velocity),floor=normalArcMinimum(desired,25);
  BeamMemory memory;auto normal=planCoverageThrow(origin,receiver.position,receiver.velocity,28,.12,{},true,false,7,196.2,{},0,false,NAN,true,.1,&receiver,0,&memory,0,0,50,nullptr,floor,2,0,desired,.1);
  check(normal.solution.valid&&normal.solution.angle>=floor-1e-6&&normal.power<=95);
  check(receiverGap(receiver,normal.solution.catchPoint,normal.solution.time+.12)<=-.25);
 }
 // Visual guidance and the Camera wheel confirmation use the same search,
 // with power constrained to the value the game will actually throw.
 for(double power:{73.,95.}){
  const Vec origin{0,8,0};Defender receiver{{130,8,0},{},20,7,12,3,2,4};BeamMemory memory;
  auto p=planCoverageThrow(origin,receiver.position,{},28,.08,{},true,false,7,196.2,{},0,false,NAN,true,.1,&receiver,0,&memory,0,0,50,nullptr,-30,2,power,40,.09);
  check(p.solution.valid&&std::abs(p.power-power)<1e-8);
  check(std::abs(length(p.solution.velocity)-power)<1e-7&&length(point(p.solution,28,p.solution.time)-p.solution.catchPoint)<1e-6);
  for(const auto& candidate:memory.leaders)check(std::abs(candidate.power-power)<1e-8);
 }
 {using namespace ff3Native;Frame f;f.at=10;f.ready=f.held=true;f.target=5;f.revision=8;f.solution.valid=true;
  check(packetPressIssue(f,true,10.01,8).empty());check(packetPressIssue(f,false,10.01,8)=="Packet Aim is preparing");
  check(!packetPressIssue(f,true,10.2,8).empty()&&!packetPressIssue(f,true,9.9,8).empty()&&!packetPressIssue(f,true,10.01,9).empty());
  f.ready=false;f.reason="No receiver selected";check(packetPressIssue(f,true,10.01,8)==f.reason);
  f.origin={200,60};f.wheelVisible=true;f.powerButtons={{1,73,1,1,{100,100},{40,20},0}};
  check(powerWheelHit(f,{320,170})&&!powerWheelHit(f,{400,200}));f.wheelVisible=false;check(!powerWheelHit(f,{320,170}));
  ClickGate gate;check(gate.down(true,10));gate.cancel();check(gate.up()&&!gate.up());
 }
 {using namespace ff3Native;Frame request;request.pid=1;request.model=2;request.own=3;request.tool=4;request.powerGui=5;request.window=(HWND)6;request.held=request.ready=true;request.target=9;request.revision=1;
  auto sample=request;sample.ready=false;sample.target=0;sample.gravity=0;sample.power=0;check(samePowerContext(request,sample));
  sample.tool=99;check(!samePowerContext(request,sample));sample=request;sample.held=false;check(!samePowerContext(request,sample));sample=request;sample.powerGui=0;check(!samePowerContext(request,sample));
 }
 {using namespace ff3Native;const bool busy=inputBusy,cancelled=inputCancelled,packet=packetMode,visual=visualOnly;const auto history=throwHistory;const auto action=lastAction;const auto sequence=historySequence;const auto at=actionAt;
  packetMode=visualOnly=false;inputBusy=inputCancelled=true;const auto count=throwHistory.a.size();record(Frame{},"Throw cancelled: interrupted preparation");check(throwHistory.a.size()==count&&lastAction==action);
  record(Frame{},"Manual throw before Camera Aim was ready");check(throwHistory.a.size()==std::min(size_t(8),count+1)&&throwHistory.a.back()[L"outcome"].str()==L"normal");
  throwHistory=history;lastAction=action;historySequence=sequence;actionAt=at;inputBusy=busy;inputCancelled=cancelled;packetMode=packet;visualOnly=visual;
 }
}
