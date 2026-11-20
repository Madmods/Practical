#define NOMINMAX
#include "../src/ff3_qb_overlay.hpp"
#include "../src/ff3_packet_bridge.hpp"
#include <iostream>
int main(){
 winrt::init_apartment();unsigned checks=0;
 auto check=[&](bool ok){++checks;if(!ok){std::cerr<<"Failed check "<<checks<<'\n';std::exit(1);}};
 using namespace ff3Native;
 {Frame f;f.head={0,8,0};f.targetPosition={70,8,0};f.targetVelocity={20,0,0};f.at=10;f.gravity=28;f.power=95;f.mode=0;f.minimumAngle=25;f.maximumAngle=50;f.selectedLob=false;f.solution=ff3qb::solve(f.head,f.targetPosition,f.targetVelocity,95,28,.05,false,5);
  const bool oldAngle=automaticAngle,oldPower=autoPower,oldBeam=force95;automaticAngle=autoPower=true;force95=true;
  auto prediction=predictedThrow(f,10.08);check(prediction.valid&&prediction.angle>=25-1e-6&&prediction.angle<=50+1e-6);
  check(ff3qb::length(ff3qb::point(prediction,28,prediction.time)-(f.targetPosition+f.targetVelocity*(.08+prediction.time+.05)))<1e-4);
  automaticAngle=oldAngle;autoPower=oldPower;force95=oldBeam;
 }

 { // Real service lifecycle with an in-process event instead of an elevated helper.
  auto helper=std::filesystem::temp_directory_path()/(L"vanta-network-test-"+std::to_wstring(GetCurrentProcessId()));std::ofstream(helper).put('1');
  std::vector<std::filesystem::path> sessions;unsigned launches=0;HANDLE event=nullptr;
  auto launcher=[&](const std::filesystem::path&,const std::wstring& parameters)->HANDLE{++launches;auto a=parameters.find(L"--session \"")+11;auto b=parameters.find(L'\"',a);sessions.push_back(parameters.substr(a,b-a));check(std::filesystem::exists(sessions.back()/L"setup-only"));return event=CreateEventW(nullptr,TRUE,FALSE,nullptr);};
  auto state=[&](const wchar_t* capture,bool ready,bool rejoin=false,ULONGLONG age=0){auto root=basketballUi::object();root.o[L"capture"]=basketballUi::string(capture);root.o[L"ready"]=basketballUi::boolean(ready);root.o[L"requiresRejoin"]=basketballUi::boolean(rejoin);root.o[L"phase"]=basketballUi::string(rejoin?L"rejoin":L"ready");root.o[L"status"]=basketballUi::string(rejoin?L"Rejoin without holding a football":L"Prepared");root.o[L"heartbeat"]=basketballUi::number(double(GetTickCount64()-age));std::ofstream(sessions.back()/L"state.json")<<winrt::to_string(json::dump(root));};
  {ff3Packets::Service service(helper,launcher);check(service.prepare(77));check(service.prepare(77)&&launches==1);state(L"Preparing",true);service.poll();check(service.networking()[L"ready"].boolean()&&!service.inputReady());
   check(service.start(77)&&launches==1&&!std::filesystem::exists(sessions.back()/L"setup-only"));check(!service.inputReady());
   state(L"Running",true);Sleep(210);service.poll();check(service.inputReady());service.disableEdits();check(!service.inputReady()&&!service.networking()[L"editing"].boolean());
   {std::ifstream in(sessions.back()/L"plan.json");std::string bytes((std::istreambuf_iterator<char>(in)),{});check(!json::parse(std::wstring(winrt::to_hstring(bytes)))[L"active"].boolean());}
   check(service.prepare(77)&&launches==1&&!std::filesystem::exists(sessions.back()/L"setup-only"));check(service.start(77)&&service.inputReady());
   state(L"Preparing",false,true);Sleep(210);service.poll();check(service.networking()[L"requiresRejoin"].boolean()&&!service.inputReady());
   state(L"Running",true,false,4000);Sleep(210);service.poll();check(!service.networking()[L"ready"].boolean()&&!service.inputReady()&&service.retryable());
   state(L"Stopped",false,true,4000);Sleep(210);service.poll();check(service.networking()[L"requiresRejoin"].boolean()&&service.retryable());
   service.stop();check(std::filesystem::exists(sessions.back()/L"stop"));SetEvent(event);
  }
  launches=0;
  {ff3Packets::Service service(helper,[&](const std::filesystem::path&,const std::wstring& parameters)->HANDLE{++launches;auto a=parameters.find(L"--session \"")+11;sessions.push_back(parameters.substr(a,parameters.find(L'\"',a)-a));return nullptr;});check(!service.prepare(77));for(int i=0;i<5;++i)check(!service.prepare(77));check(launches==1&&service.retryable());check(!service.prepare(77,true)&&launches==2);check(!service.prepare(88)&&launches==3);}
  for(const auto& path:sessions)std::filesystem::remove_all(path);std::filesystem::remove(helper);
 }
 {Frame practicePlan;practicePlan.practiceRange=true;auto p=basketballUi::object();ff3Packets::applyEnvironmentPolicy(p,practicePlan);
  check(p[L"environment"].str()==L"practice"&&!p[L"boundaryRequired"].boolean()&&!p[L"field"][L"known"].boolean());
  Frame matchPlan;auto m=basketballUi::object();ff3Packets::applyEnvironmentPolicy(m,matchPlan);check(m[L"environment"].str()==L"match"&&m[L"boundaryRequired"].boolean()&&!m[L"field"][L"known"].boolean());
  check(json::parse(json::dump(p))[L"boundaryRequired"].type==json::Value::Bool&&!json::parse(json::dump(p))[L"boundaryRequired"].boolean());
 }
 {Frame launch;launch.pid=7;launch.model=1;launch.own=2;launch.at=10;launch.gravity=28;launch.groundY=0;launch.target=8;launch.ready=launch.solution.valid=true;launch.solution.origin={0,8,0};launch.solution.velocity={40,18,0};launch.solution.time=1;launch.solution.catchPoint={40,12,0};
  Frame live=launch;live.localPlayer=3;live.releaseVisual=std::make_shared<Frame>(launch);live.throwView.releasedAt=10;live.throwView.phase=ff3qb::ThrowView::Flight;live.throwView.ballVerified=true;live.throwView.ball=88;
  ff3Visual::ReleaseBeam beam;Frame shown;check(beam.select(live,shown,10.1));
  live.releaseVisual.reset();live.throwView.phase=ff3qb::ThrowView::Hidden;live.revision=50;check(beam.select(live,shown,10.2)&&shown.afterRelease&&shown.solution.origin.x==0);
  ff3qb::BallFlight ball;ball.id=88;ball.thrower=3;ball.landed=true;live.balls={ball};check(!beam.select(live,shown,10.3));
  live.releaseVisual=std::make_shared<Frame>(launch);beam.clear();live.throwView.phase=ff3qb::ThrowView::Flight;live.balls[0].landed=false;check(beam.select(live,shown,10.2));
  live.balls[0].landed=true;check(!beam.select(live,shown,10.3));live.balls.clear();check(!beam.select(live,shown,10.4));
  live.releaseVisual=std::make_shared<Frame>(launch);live.model=9;check(!beam.select(live,shown,10.2));
 }
 {Frame practiceFrame;practiceFrame.practiceRange=isPracticeRange(81310542478972ULL);
  check(practiceFrame.practiceRange&&!isPracticeRange(0)&&!isPracticeRange(8204899140ULL));
  check(fieldAvailable(practiceFrame)&&throwBounds(practiceFrame)==nullptr);
  ff3qb::Defender receiver;receiver.position={60,8,0};receiver.radius=2;receiver.above=2;receiver.below=4;
  ff3qb::Solution path{true,{0,8,0},{60,0,0},{60,8,0},1,0};
  check(catchAllowed(practiceFrame,path,receiver,.05));
  auto unreachable=path;unreachable.catchPoint.x+=40;check(!catchAllowed(practiceFrame,unreachable,receiver,.05));
  auto bad=path;bad.valid=false;check(!catchAllowed(practiceFrame,bad,receiver,.05));
  check(!catchAllowed(practiceFrame,path,receiver,NAN));
  Frame match;check(!fieldAvailable(match)&&throwBounds(match)==&match.field&&!catchAllowed(match,path,receiver,.05));
  auto manual=ff3qb::planThrow({0,8,0},{60,8,0},{},28,.05,0,false,15,true,60,false,false,{},15,{},false,5,0,50,throwBounds(practiceFrame),&receiver);
  check(manual.solution.valid&&catchAllowed(practiceFrame,manual.solution,receiver,.05));
  auto automatic=ff3qb::planCoverageThrow({0,8,0},{60,8,0},{},28,.05,{},false,false,0,196.2,{},0,false,NAN,true,.1,&receiver,0,nullptr,0,0,50,throwBounds(practiceFrame));
  check(automatic.solution.valid&&catchAllowed(practiceFrame,automatic.solution,receiver,.05));
 }
 check(robloxClientPriority(false,2,false)>robloxClientPriority(false,0,true));
 check(robloxClientPriority(false,2,true)>robloxClientPriority(false,2,false));
 check(robloxClientPriority(true,0,false)>robloxClientPriority(false,2,true));
 check(robloxClientPriority(false,1,false)>robloxClientPriority(false,0,false));
 check(install()&&inputPumpReady&&inputPumpId!=GetCurrentThreadId());
 auto pumpId=inputPumpId.load();check(install()&&inputPumpId==pumpId);
 uninstall();check(!inputPumpReady&&!inputPumpId&&!inputPump.joinable()&&!mouse&&!keyboard);
 const auto basketballBefore=json::dump(basketballUi::snapshot());
 check(!enabled&&!force95);check(!set(L"autotime",basketballUi::boolean(true)));
 check(!set(L"ff3_qb.arc",basketballUi::number(2)));check(!set(L"ff3_qb.arc",basketballUi::number(NAN)));
 check(!set(L"ff3_qb.power",basketballUi::number(1)));check(!set(L"ff3_qb.unknown",basketballUi::boolean(true)));
 check(set(L"ff3_qb.arc",basketballUi::number(1)));check(arc==1);
 check(set(L"ff3_qb.power",basketballUi::boolean(false)));check(set(L"ff3_qb",basketballUi::boolean(true)));
 check(set(L"ff3_qb.preset",basketballUi::number(4)));check(passMode==4);
 check(!set(L"ff3_qb.preset",basketballUi::number(6)));check(!set(L"ff3_qb.angle",basketballUi::number(4)));check(!set(L"ff3_qb.angle",basketballUi::number(86)));check(!set(L"ff3_qb.radius",basketballUi::number(INFINITY)));
 check(set(L"ff3_qb.angle",basketballUi::number(85))&&manualAngle==85);
 check(!set(L"ff3_qb.dragangle",basketballUi::boolean(true)));
 check(set(L"ff3_qb.angle",basketballUi::number(42)));check(set(L"ff3_qb.autoangle",basketballUi::boolean(false)));check(set(L"ff3_qb.fit",basketballUi::boolean(true)));
 check(set(L"ff3_qb.lead",basketballUi::number(-5))&&leadDistance==-5);
 check(set(L"ff3_qb.height",basketballUi::number(4))&&catchHeightAdjustment==4);
 check(!set(L"ff3_qb.height",basketballUi::number(9)));check(!set(L"ff3_qb.height",basketballUi::number(-4)));
 check(!set(L"ff3_qb.lead",basketballUi::number(13)));check(!set(L"ff3_qb.lead",basketballUi::number(-13)));check(!set(L"ff3_qb.lead",basketballUi::number(.5)));
 const auto savedRevision=revision.load();stop();check(!enabled&&revision>savedRevision);
 auto file=std::filesystem::temp_directory_path()/(L"vanta-ff3-test-"+std::to_wstring(GetCurrentProcessId())+L".json");
 check(save(file));set(L"ff3_qb.arc",basketballUi::number(0));set(L"ff3_qb",basketballUi::boolean(true));load(file);
 check(arc==1&&autoPower&&!enabled);check(passMode==4&&manualAngle==42&&!automaticAngle&&smartFit);
 check(catchHeightAdjustment==4&&modules()[L"ff3_qb"][L"rows"][L"height"].num()==4);check(set(L"ff3_qb.height",basketballUi::number(0)));
 check(leadDistance==-5&&modules()[L"ff3_qb"][L"rows"][L"lead"].num()==-5);check(set(L"ff3_qb.lead",basketballUi::number(0)));
 check(set(L"ff3_qb.power",basketballUi::boolean(false)));check(save(file));autoPower=true;load(file);check(!autoPower);
 check(!set(L"ff3_qb.force95",basketballUi::number(1)));check(set(L"ff3_qb.force95",basketballUi::boolean(true)));check(force95&&passMode==0&&automaticAngle&&autoPower);
 check(selectedLead()==3&&selectedHeight()==0);
 check(set(L"ff3_qb.height",basketballUi::number(7))&&beamCatchHeight==7&&catchHeightAdjustment==0);
 check(save(file));beamCatchHeight=8;load(file);check(force95&&selectedHeight()==7&&selectedLead()==3);
 check(set(L"ff3_qb.height",basketballUi::number(8)));
 check(save(file));force95=false;load(file);check(force95);check(set(L"ff3_qb.preset",basketballUi::number(2))&&!force95);std::filesystem::remove(file);
 check(json::dump(basketballUi::snapshot())==basketballBefore);
 Frame f;f.pid=5;f.model=100;f.own=200;f.tool=300;f.target=400;f.revision=revision;f.ready=true;f.power=95;f.gravity=28;f.aim={500,300,true};f.client={0,0,1920,1080};
 {
  physicalClickAt=GetTickCount64()-2000;physicalLeftHeld=true;heldTarget=f.target;requested=f;requestedAt=now();requested.at=requestedAt-.01;
  auto fresh=requested;fresh.at=now();fresh.head.x=9;fresh.menuOpen=false;publish(fresh);auto held=packetFrame(fresh);
  check(held.held&&held.retained&&held.frame.head.x==9&&held.clicked==physicalClickAt.load());
  physicalLeftHeld=false;physicalReleasedAt=GetTickCount64();held=packetFrame(fresh);check(!held.held&&held.retained&&held.released==physicalReleasedAt.load());
  physicalReleasedAt=GetTickCount64()-351;check(!packetFrame(fresh).retained);
  {std::lock_guard<std::mutex> lock(mutex);cancelPress();requested={};requestedAt=0;current={};}
 }
 {std::lock_guard<std::mutex> lock(mutex);traceInput("test-held-press",true,true,f);}
 {auto input=inputReport();check(input[L"action"].str()==L"test-held-press"&&input[L"blocked"].boolean()&&input[L"transport"].boolean());check(input[L"target"].num()==400&&input[L"sequence"].num()>0);flushInputTraces();std::lock_guard<std::mutex> lock(mutex);recentInput={};}
 {
  const bool priorBeam=force95,priorPower=autoPower,priorAuto=automaticAngle;
  force95=autoPower=automaticAngle=true;
  Frame beam;beam.at=10;beam.mode=0;beam.head={0,5,0};beam.targetPosition={71,5,0};beam.targetVelocity={20,0,0};beam.power=59;beam.gravity=28;beam.ping=0;
  beam.solution=ff3qb::solve(beam.head,beam.targetPosition,beam.targetVelocity,59,28,.05,false,5);
  check(beam.solution.valid);
  check(!ff3qb::solve(beam.head,beam.targetPosition+beam.targetVelocity*.164,beam.targetVelocity,59,28,.05,false,5).valid);
  auto shown=predictedThrow(beam,10.164);
  check(shown.valid&&std::abs(shown.angle-beam.solution.angle)<1e-6&&ff3qb::length(shown.velocity)>59&&ff3qb::length(shown.velocity)<=95);
  check(ff3qb::length(ff3qb::point(shown,28,shown.time)-(beam.targetPosition+beam.targetVelocity*(.164+shown.time+.05)))<1e-5);
  force95=priorBeam;autoPower=priorPower;automaticAngle=priorAuto;
 }
 {auto visual=f;visual.at=10;visual.head={0,8,0};visual.targetPosition={70,16,0};visual.targetVelocity={0,0,20};visual.ping=20;visual.mode=0;bool priorBeam=force95;force95=true;
  auto drawn=ff3Visual::projectedThrow(visual,10.08);auto sent=ff3qb::solve(visual.head,visual.targetPosition+visual.targetVelocity*.08,visual.targetVelocity,95,28,.06,false,5);
  check(drawn.valid&&sent.valid&&ff3qb::length(drawn.catchPoint-sent.catchPoint)<1e-6&&ff3qb::length(drawn.velocity-sent.velocity)<1e-6);force95=priorBeam;
 }
 {const bool priorBeam=force95,priorPower=autoPower,priorAngle=automaticAngle;force95=autoPower=automaticAngle=true;
  Frame edge;edge.at=10;edge.head={0,8,0};edge.targetPosition={77,8,0};edge.targetVelocity={2,0,0};edge.power=95;edge.gravity=28;
  edge.field={true,{0,3,0},{1,0,0},{0,0,1},80,180};edge.receiverReach.position=edge.targetPosition;edge.receiverReach.velocity=edge.targetVelocity;edge.receiverReach.jumpHeight=0;
  edge.solution=ff3qb::solve(edge.head,edge.targetPosition,edge.targetVelocity,95,28,.05,false,5);
  check(predictedThrow(edge,10).valid);check(!predictedThrow(edge,10.45).valid);
  force95=priorBeam;autoPower=priorPower;automaticAngle=priorAngle;
 }
 {auto visual=f;visual.at=10;visual.head={0,8,0};visual.targetPosition={65,6.5,0};visual.targetVelocity={0,0,20};visual.ping=20;visual.mode=0;visual.fixedPowerRoute=true;
  bool priorBeam=force95;force95=false;auto drawn=ff3Visual::projectedThrow(visual,10.08);auto sent=ff3qb::solve(visual.head,visual.targetPosition+visual.targetVelocity*.08,visual.targetVelocity,95,28,.06,false,5);
  check(drawn.valid&&drawn.angle<5&&sent.valid&&ff3qb::length(drawn.velocity-sent.velocity)<1e-6);force95=priorBeam;
 }
 {Frame release=f;release.ready=true;release.at=10;release.solution=ff3qb::solve({0,8,0},{80,8,0},{},95,28,.05,false,5);release.target=400;
  Frame live=f;live.releaseVisual=std::make_shared<Frame>(release);live.throwView.phase=ff3qb::ThrowView::Flight;live.head={100,20,80};Frame shown;
  check(ff3Visual::releasedFrame(live,shown,10.5)&&shown.afterRelease&&ff3qb::length(shown.solution.origin-release.solution.origin)<1e-9&&!shown.releaseVisual);
  live.held=true;check(!ff3Visual::releasedFrame(live,shown,10.5));live.held=false;live.model++;check(!ff3Visual::releasedFrame(live,shown,10.5));live.model--;
  live.throwView.phase=ff3qb::ThrowView::Hidden;check(ff3Visual::releasedFrame(live,shown,10.5));
 }
 {
  // A maximum-power Auto preview must not disappear because a tiny origin
  // change makes its OLD fixed angle require 95.01 power.
  const bool oldAutomatic=automaticAngle,oldPower=autoPower,oldBeam=force95;
  automaticAngle=autoPower=true;force95=false;
  Frame v;v.at=10;v.mode=0;v.head={0,8,0};v.targetPosition={100,8,0};v.power=95;v.gravity=28;v.ping=20;
  v.solution=ff3qb::solve(v.head,v.targetPosition,{},95,28,.06,false,5);
  v.head.x=-.1;
  check(!ff3qb::solveAngle(v.head,v.targetPosition,{},v.solution.angle,28,.06,5).valid);
  check(anglePowerPlan(v));auto shown=predictedThrow(v,10.01);
  check(shown.valid&&ff3qb::length(shown.catchPoint-v.targetPosition)<1e-6);
  v.mode=2;check(anglePowerPlan(v));
  v.head={0,8,0};v.solution=ff3qb::solveAngle(v.head,v.targetPosition,{},55,28,.06,5);v.power=ff3qb::length(v.solution.velocity);v.selectedLob=true;
  shown=predictedThrow(v,10);check(shown.valid&&std::abs(shown.angle-50)<1e-6&&ff3qb::length(shown.catchPoint-v.targetPosition)<1e-6);
  automaticAngle=false;check(anglePowerPlan(v)); // R/F keeps its explicit elevation.
  shown=predictedThrow(v,10);check(shown.valid&&std::abs(shown.angle-55)<1e-6);
  automaticAngle=true;v.mode=0;v.head={0,8,0};v.targetPosition={50,8,0};
  v.solution=ff3qb::solveAngle(v.head,v.targetPosition,{},40,28,.06,5);v.power=ff3qb::length(v.solution.velocity);v.selectedLob=false;
  v.head.x=-8;check(!ff3qb::solve(v.head,v.targetPosition,{},v.power,28,.06,false,5).valid);
  shown=predictedThrow(v,10);check(shown.valid&&std::abs(shown.angle-40)<1e-6&&ff3qb::length(shown.velocity)>v.power&&ff3qb::length(shown.catchPoint-v.targetPosition)<1e-6);
  automaticAngle=oldAutomatic;autoPower=oldPower;force95=oldBeam;
 }
 {
  Frame previous=f;previous.held=previous.ready=previous.solution.valid=true;previous.at=10;
  Frame gap=previous;gap.at=10.04;gap.target=0;gap.ready=false;gap.reason="Aim near a teammate or practice receiver";
  auto grace=bridgeTargetGap(previous,gap,10.04);check(grace.target==previous.target&&grace.targetGrace&&grace.at==10);
  check(canBridgeTargetGap(grace,gap,10.09));check(!canBridgeTargetGap(grace,gap,10.101));
  auto next=gap;next.target=999;next.ready=true;
  check(bridgeTargetGap(grace,next,10.05).target==999);
  gap.held=false;check(!canBridgeTargetGap(previous,gap,10.05));gap.held=true;
  gap.textFocus=true;check(!canBridgeTargetGap(previous,gap,10.05));gap.textFocus=false;
  ++gap.revision;check(!canBridgeTargetGap(previous,gap,10.05));--gap.revision;
  gap.reason="Focus Football Fusion 3";check(!canBridgeTargetGap(previous,gap,10.05));
  ff3qb::SelectionLatch mouse;mouse.update(1,false,0);check(mouse.update(2,true,.001,false,0)==2);
 }
 {
  Frame cue;cue.hudCoverageKnown=true;cue.risk.margin=2;
  check(std::wstring(ff3Visual::clearanceLabel(cue))==L"CLEAR");
  cue.risk.possible=true;check(std::wstring(ff3Visual::clearanceLabel(cue))==L"TIGHT");
  auto tight=ff3Visual::accent(cue);cue.risk.intable=true;check(std::wstring(ff3Visual::clearanceLabel(cue))==L"INTERCEPT");
  auto intercepted=ff3Visual::accent(cue);check(tight.GetValue()!=intercepted.GetValue());
  cue.hudReachEstimated=true;check(std::wstring(ff3Visual::clearanceLabel(cue))==L"INTERCEPT");
  cue.risk={};check(std::wstring(ff3Visual::clearanceLabel(cue))==L"CHECKING");
  cue.hudReachEstimated=false;cue.risk.margin=.5;check(std::wstring(ff3Visual::clearanceLabel(cue))==L"TIGHT"&&ff3Visual::accent(cue).GetValue()==tight.GetValue());
 }
 {ff3Visual::PreviewPlan preview;auto valid=f;valid.held=valid.solution.valid=true;
  check(preview.select(valid,10).ready);auto gap=valid;gap.ready=gap.solution.valid=false;
  check(preview.select(gap,10.05).ready);check(!gap.ready); // Only the renderer receives the retained plan.
  check(!preview.select(gap,10.2).ready);
  preview.select(valid,11);gap.held=false;check(!preview.select(gap,11.01).ready);
  preview.select(valid,12);gap.held=true;++gap.revision;check(!preview.select(gap,12.01).ready);
  preview.select(valid,13);gap=valid;gap.ready=false;++gap.target;check(!preview.select(gap,13.01).ready);
 }
 f.gameMouse={500,300,0};f.matrix[0]=f.matrix[5]=f.matrix[10]=f.matrix[15]=1;
 check(!eligible(f,now())); // No admitted session or game focus: always closed.
 {
  Frame ready=f;ready.at=10;Frame released=ready;released.at=10.08;released.ready=false;released.tool=0;released.held=false;
  check(clickPlanValid(ready,released,10.02,10.08,1000,1060,ready.revision));
  check(!clickPlanValid(ready,released,10.12,10.15,1000,1030,ready.revision));
  check(!clickPlanValid(ready,released,10.02,10.38,1000,1360,ready.revision));
  check(!clickPlanValid(ready,released,10.02,10.08,1000,1060,ready.revision+1));
  ++released.own;check(!clickPlanValid(ready,released,10.02,10.08,1000,1060,ready.revision));
  --released.own;ready.textFocus=true;check(!clickPlanValid(ready,released,10.02,10.08,1000,1060,ready.revision));
  ready.textFocus=false;ready.at=now()-.02;requested=ready;requestedAt=now()-.01;physicalClickAt=GetTickCount64()-10;
  auto dispatch=packetFrame(released);check(dispatch.retained&&dispatch.frame.tool==ready.tool&&dispatch.clicked==physicalClickAt.load());
  physicalClickAt=0;dispatch=packetFrame(released);check(!dispatch.retained&&!dispatch.frame.tool&&dispatch.clicked==0);requested={};requestedAt=0;
 }
 {float live[16]{},sample[16]{};live[0]=1;
  check(ff3Visual::ViewReader::matrix(GetCurrentProcess(),reinterpret_cast<addr>(live),sample)&&sample[0]==1);
  live[0]=2;check(ff3Visual::ViewReader::matrix(GetCurrentProcess(),reinterpret_cast<addr>(live),sample)&&sample[0]==2);
  live[0]=NAN;check(!ff3Visual::ViewReader::matrix(GetCurrentProcess(),reinterpret_cast<addr>(live),sample));}
 check(dispatchConsistent(f,f,f,{500,300}));
 for(int i=0;i<14;i++){
  Frame changed=f;
  switch(i){case 0:++changed.pid;break;case 1:++changed.model;break;case 2:++changed.own;break;case 3:++changed.tool;break;case 4:++changed.target;break;case 5:++changed.revision;break;case 6:changed.ready=false;break;case 7:changed.power=90;break;case 8:changed.gravity=30;break;case 9:changed.origin.x=10;break;case 10:changed.client.right=1000;break;case 11:changed.aim.x+=30;break;case 12:changed.aim.valid=false;break;case 13:changed.window=(HWND)1;break;}
  check(!dispatchConsistent(f,f,changed,{500,300}));
 }
 check(!dispatchConsistent(f,f,f,{510,300}));
 Frame unacknowledged=f;unacknowledged.gameMouse={400,300,0};check(!dispatchConsistent(f,f,unacknowledged,{500,300}));
 Frame rotated=f;rotated.matrix[0]=.99f;rotated.matrix[2]=.1f;check(!dispatchConsistent(f,f,rotated,{500,300}));
 Frame missing=f;missing.gameMouse.x=NAN;check(!dispatchConsistent(f,f,missing,{500,300}));
 Frame previous=f;++previous.revision;check(!dispatchConsistent(previous,f,f,{500,300}));
 {ff3qb::SelectionRetention retain;
  check(retain.keep(42,19,20,1));check(retain.keep(42,29,20,1.05));
  check(retain.keep(42,70,20,1.1));check(retain.keep(42,70,20,1.4));check(!retain.keep(42,70,20,1.8));
  check(retain.keep(42,900,20,1.5,true));check(!retain.keep(43,900,20,1.6));
  check(!retain.keep(43,NAN,20,1.7));check(retain.keep(43,5,20,2));check(!retain.keep(43,100,20,1.9));
  ff3qb::SelectionLatch latch;check(latch.update(42,false,1)==42);
  for(int i=0;i<20;++i)check(latch.update(i%2?43:42,true,1+i*.02)==42);
  check(latch.update(42,true,1.9)==42);check(latch.update(43,true,2)==42);check(latch.update(43,true,2.2)==43);
 }
 {
  // Two overlapping receivers must not tie merely because both capsules hit.
  auto center=ff3qb::selectionScore(100,100,100,130,100,70,10);
  auto neighbor=ff3qb::selectionScore(100,100,106,130,106,70,10);
  check(center<neighbor&&neighbor<0);
  check(ff3qb::selectionScore(130,100,100,130,100,70,10)==20);
  check(ff3qb::selectionScore(100,125,100,130,100,70,10)<0);
  ff3qb::SelectionLatch deliberate;check(deliberate.update(42,false,1)==42);
  check(deliberate.update(43,true,2,false,.06)==42);
  check(deliberate.update(43,true,2.07,false,.06)==43);
  check(deliberate.update(43,true,2.08,true)==43);
  Frame edge;edge.ready=true;edge.at=10;edge.gravity=28;edge.power=95;
  edge.solution=ff3qb::solve({0,8,0},{60,8,0},{},95,28,0,false,5);
  check(edge.solution.valid);edge.head={NAN,0,0};
  check(!ff3Visual::projectedThrow(edge,10.05).valid);
  check(!ff3Visual::projectedThrow(edge,10.3).valid);
  edge.ready=false;check(!ff3Visual::projectedThrow(edge,10.05).valid);
 }
 Frame keyFrame;keyFrame.target=456;keyFrame.solution.valid=true;keyFrame.solution.angle=32;lockedTarget=0;
 check(hotkey('Q',keyFrame,false)&&lockedTarget==456);check(hotkey('Q',keyFrame,true)&&lockedTarget==456);check(hotkey('Q',keyFrame,false)&&lockedTarget==0);
 passMode=5;check(hotkey('Z',keyFrame,false)&&passMode==0&&automaticAngle&&autoPower);check(hotkey('Z',keyFrame,true)&&passMode==0);
 force95=true;check(hotkey('R',keyFrame,false)&&manualAngle==33&&!automaticAngle&&autoPower&&!force95);check(hotkey('F',keyFrame,false)&&manualAngle==32);
 check(set(L"ff3_qb.radius",basketballUi::number(600)));check(!set(L"ff3_qb.radius",basketballUi::number(701)));
 check(set(L"ff3_qb.trails",basketballUi::boolean(false))&&!ballTrails);check(set(L"ff3_qb.trails",basketballUi::boolean(true))&&ballTrails);
 check(set(L"ff3_qb.dive",basketballUi::number(16)));check(!set(L"ff3_qb.dive",basketballUi::number(25)));
 int stops=0;packetStop=[&](){++stops;};enabled=true;stop(false);check(!enabled&&stops==0);stop();check(stops==1);packetStop={};
 clicks.down(true,now());stop();check(!clicks.pending&&clicks.up());
 record(f,"Test input only",true);check(report()[L"throws"].num()==1);check(!mouse);
 std::cout<<checks<<" FF3 settings, persistence, session isolation and dispatch cancellation checks passed (no game inputs)\n";
 {
  using namespace Gdiplus;ULONG_PTR token=0;GdiplusStartupInput startup;check(GdiplusStartup(&token,&startup,nullptr)==Ok);
  Frame sample;sample.groundY=0;sample.client={0,0,1280,720};sample.ready=sample.held=sample.wheelVisible=sample.coverageKnown=true;sample.name="Practice receiver";sample.route="Go / deep";sample.coverageCount=3;sample.coverageDetail="3 catchers / jump + dive estimate";sample.locked=true;sample.mode=2;sample.guarded=true;sample.margin=.54;sample.power=78;sample.distance=146;sample.solution.angle=48;sample.solution.time=2.36;
  PumpSurface field;sample.target=123;sample.gravity=28;sample.fitOffset={4,4,0};sample.solution=ff3qb::solveAngle({0,8,0},{104,12,0},{},35,28,.05,5);sample.matrix[0]=.012f;sample.matrix[5]=.025f;sample.matrix[6]=.012f;sample.matrix[3]=-.65f;sample.matrix[7]=-.5f;sample.matrix[15]=1;sample.receiver=ff3qb::project({100,8,0},sample.matrix,1280,720);
  sample.defenders={{{75,8,8},{},20,7,12}};sample.coverageTrace={0,.3,.54,{55,13,0}};sample.hudDefenders=sample.defenders;sample.hudCoverageKnown=sample.receiverReachKnown=true;sample.receiverReach.position={100,8,0};sample.risk=ff3qb::riskForecast(sample.solution,28,sample.hudDefenders,.05);ff3Visual::renderField(field,sample,false);check(field.bits&&!field.visible);
  {Bitmap image(field.width,field.height,field.width*4,PixelFormat32bppPARGB,(BYTE*)field.bits);CLSID png={0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e}};check(image.Save(L"build/ff3-field-preview.png",&png,nullptr)==Ok);}
  {auto unknown=sample;unknown.hudCoverageKnown=false;unknown.hudReachEstimated=true;ff3Visual::renderField(field,unknown,false);
   auto* pixels=static_cast<unsigned char*>(field.bits);std::vector<unsigned char> estimated(pixels,pixels+size_t(field.width)*field.height*4);unknown.hudDefenders.clear();ff3Visual::renderField(field,unknown,false);
   check(!std::equal(estimated.begin(),estimated.end(),pixels)); // Missing shapes cannot hide available DB range geometry.
  }
  sample.afterRelease=true;sample.throwView.phase=ff3qb::ThrowView::Flight;sample.throwView.ball=77;ff3Visual::renderField(field,sample,false);
  ff3qb::BallFlight owned;owned.id=77;owned.at=.8;owned.gravity=28;owned.model={true,0,28,0,sample.solution.origin,sample.solution.velocity+ff3qb::Vec{0,2,0}};owned.position=owned.model.pointAt(.8);sample.balls={owned};ff3Visual::renderBalls(field,sample,false,false);
  {Bitmap image(field.width,field.height,field.width*4,PixelFormat32bppPARGB,(BYTE*)field.bits);CLSID png={0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e}};check(image.Save(L"build/ff3-release-preview.png",&png,nullptr)==Ok);}sample.afterRelease=false;
  sample.ready=false;sample.fitBlocked=true;sample.reason="Coverage incomplete: no assisted throw; normal click unchanged";
  sample.selectedPosition={100,8,0};
  ff3Visual::renderField(field,sample,false);check(field.bits&&!field.visible);
  {auto* pixels=static_cast<unsigned char*>(field.bits);std::vector<unsigned char> selected(pixels,pixels+size_t(field.width)*field.height*4);
   check(std::any_of(selected.begin(),selected.end(),[](auto p){return p!=0;}));
   sample.locked=false;ff3Visual::renderField(field,sample,false);check(!std::equal(selected.begin(),selected.end(),pixels));sample.locked=true;
  }
  {
   bool wasReady=sample.ready;sample.ready=true;ff3Visual::renderField(field,sample,false);
   auto* data=static_cast<unsigned char*>(field.bits);size_t bytes=size_t(field.width)*field.height*4;
   std::vector<unsigned char> before(data,data+bytes);check(std::any_of(before.begin(),before.end(),[](auto pixel){return pixel!=0;}));sample.balls.clear();ff3Visual::renderBalls(field,sample,false,false);
   check(std::equal(before.begin(),before.end(),data)); // Compositing balls cannot erase QB geometry.
   ff3Visual::FramePacer pacer;double began=now();for(int i=0;i<12;i++)pacer.wait(now());
   double elapsed=now()-began;check(elapsed>.06&&elapsed<.5);std::cout<<"Overlay pacing sample: "<<12/elapsed<<" Hz (timer only)\n";
   sample.ready=wasReady;ff3Visual::renderField(field,sample,false);
  }
  {Bitmap image(field.width,field.height,field.width*4,PixelFormat32bppPARGB,(BYTE*)field.bits);CLSID png={0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e}};check(image.Save(L"build/ff3-field-blocked-preview.png",&png,nullptr)==Ok);}
  field.close();PumpSurface balls;ff3qb::BallFlight flight;flight.position={60,22,0};flight.velocity={40,-8,0};flight.gravity=28;for(int i=30;i>=0;i--){double dt=i*.05;flight.trail.push_back(flight.position-flight.velocity*dt+ff3qb::Vec{0,-.5*28*dt*dt,0});}flight.model={true,0,28,0,flight.position,flight.velocity};sample.balls={flight};ff3Visual::renderBalls(balls,sample,false);check(balls.bits&&!balls.visible);
  {Bitmap image(balls.width,balls.height,balls.width*4,PixelFormat32bppPARGB,(BYTE*)balls.bits);CLSID png={0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e}};check(image.Save(L"build/ff3-balls-preview.png",&png,nullptr)==Ok);}
  balls.close();GdiplusShutdown(token);
 }
 winrt::uninit_apartment();
 std::cout<<checks<<" total native and compositor checks passed\n";
}
