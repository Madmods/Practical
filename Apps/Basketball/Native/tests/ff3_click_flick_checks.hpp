#pragma once
template<class Check>void clickFlickChecks(Check check){
 using namespace ff3qb;
 std::vector<std::string> calls;std::string fail;bool throwing=false;int permissions=0;
 FlickActions actions;
 auto step=[&](const char* name){calls.push_back(name);return fail!=name;};
 actions.allowed=[&](){return ++permissions!=2||fail!="recheck";};
 actions.power=[&](){return step("power");};actions.capture=[&](){return step("capture");};actions.align=[&](){return step("align");};
 actions.press=[&](){return step("down");};actions.release=[&](){return step("up");};actions.restore=[&](){return step("restore");};
 actions.track=[&](){calls.push_back("track");if(throwing)throw 1;};
 auto result=runFlick(false,actions);check(calls.empty()&&!result.sent&&!result.restored&&permissions==0);
 result=runFlick(true,actions);check(result.sent&&result.released&&result.restored);check(calls==std::vector<std::string>({"power","capture","align","down","track","up","restore"}));
 for(const std::string stage:{"power","capture","align","down","recheck"}){
  calls.clear();permissions=0;fail=stage;result=runFlick(true,actions);check(!result.sent&&!result.released);
  check((std::find(calls.begin(),calls.end(),"up")==calls.end())&&(std::find(calls.begin(),calls.end(),"track")==calls.end()));
  check(result.restored==(stage!="power"&&stage!="capture"));
 }
 calls.clear();permissions=0;fail="up";result=runFlick(true,actions);check(result.sent&&!result.released&&result.restored&&calls.back()=="restore");
 calls.clear();permissions=0;fail="restore";result=runFlick(true,actions);check(result.sent&&result.released&&!result.restored);
 calls.clear();permissions=0;fail.clear();throwing=true;try{runFlick(true,actions);check(false);}catch(int){check(calls[calls.size()-2]=="up"&&calls.back()=="restore");}
 {using namespace ff3Native;Frame f;f.at=10;f.ready=f.held=f.inputThrow=f.solution.valid=true;f.pid=1;f.model=2;f.own=3;f.tool=4;f.target=5;f.revision=7;f.menuOpen=false;f.power=f.gamePower=80;
  check(cameraShotEligible(f,10.01,7));check(!cameraShotEligible(f,9,7)&&!cameraShotEligible(f,10.101,7)&&!cameraShotEligible(f,10.01,8));
  f.textFocus=true;check(!cameraShotEligible(f,10.01,7));f.textFocus=false;f.menuOpen=true;check(!cameraShotEligible(f,10.01,7));f.menuOpen=false;f.solution.valid=false;check(!cameraShotEligible(f,10.01,7));
 }
 {ClickGate click;check(click.down(true,10));check(click.up());check(click.take(10.05,true));check(!click.take(10.06,true));check(!click.up());
  check(click.down(true,11));click.cancel();check(!click.take(11.01,true)&&click.up());
  check(click.down(true,12));check(!click.take(12.151,true)&&click.up());
 }
 {using namespace ff3Native;Frame request;request.pid=1;request.model=2;request.own=3;request.camera=4;request.workspace=5;request.mouseService=6;request.mouseInput=7;request.window=(HWND)8;request.revision=9;request.client={0,0,1280,720};request.gameMouse={500,300,0};
  auto live=request;live.mouseInput=11;check(refreshAimBinding(request,live)&&request.mouseInput==11);
  const auto savedInput=request.mouseInput;live.mouseInput=12;live.held=false;live.tool=0;live.target=0;check(refreshAimBinding(request,live)&&request.mouseInput==12&&savedInput==11);
  for(int field=0;field<11;++field){auto replaced=live;switch(field){case 0:++replaced.pid;break;case 1:++replaced.model;break;case 2:++replaced.own;break;case 3:++replaced.camera;break;case 4:++replaced.workspace;break;case 5:++replaced.mouseService;break;case 6:++replaced.revision;break;case 7:++replaced.origin.x;break;case 8:++replaced.client.right;break;case 9:replaced.mouseInput=0;break;case 10:replaced.gameMouse.x=NAN;break;}
   check(!refreshAimBinding(request,replaced)&&request.mouseInput==12);
  }
 }
 for(double animationTime:{.02,.4,.75,1.2,2.9})check(throwReleaseDecision(animationTime,false,false)==ReleaseDecision::Waiting);
 check(throwReleaseDecision(.8,true,false)==ReleaseDecision::Released);check(throwReleaseDecision(3,false,false)==ReleaseDecision::TimedOut);check(throwReleaseDecision(.4,false,true)==ReleaseDecision::Cancelled);
 {using namespace ff3Native;Frame shot,live;shot.heldBall=7;shot.localPlayer=8;shot.own=9;
  // Tool/cursor/GUI state during the animation is not release evidence.
  live.held=false;live.ready=false;live.mouseInput=0;live.powerGui=0;check(!throwReleaseObserved(shot,live,{}));
  ff3qb::BallObservation ball;ball.id=7;ball.verifiedBall=true;live.looseBalls={ball};check(throwReleaseObserved(shot,live,{7}));
  ball.id=10;ball.thrower=8;live.looseBalls={ball};check(throwReleaseObserved(shot,live,{}));check(!throwReleaseObserved(shot,live,{10}));
  ball.thrower=11;live.looseBalls={ball};check(!throwReleaseObserved(shot,live,{}));
 }
 {std::vector<std::string> sequence;AnimatedAimActions animation;
  animation.allowed=[&](){sequence.push_back("allowed");return true;};animation.prepare=[&](){sequence.push_back("pin");return true;};animation.capture=[&](){sequence.push_back("capture");return true;};animation.follow=[&](){sequence.push_back("animation/release");};animation.restore=[&](){sequence.push_back("restore");return true;};
  check(!followPhysicalThrow(false,animation)&&sequence.empty());check(followPhysicalThrow(true,animation));check(sequence==std::vector<std::string>({"allowed","pin","capture","animation/release","restore"}));
  sequence.clear();animation.follow=[&](){sequence.push_back("animation/release");throw 1;};try{followPhysicalThrow(true,animation);check(false);}catch(int){check(sequence.back()=="restore");}
 }
 {using namespace ff3Native;Frame plan;plan.head={0,8,0};plan.targetPosition={100,8,0};plan.targetVelocity={5,0,0};plan.gravity=28;plan.power=95;plan.solution=solve(plan.head,plan.targetPosition,plan.targetVelocity,95,28,.05,false,5);
  check(plan.solution.valid&&pinPhysicalPower(plan,75));check(plan.power==75&&plan.gamePower==75&&std::abs(length(plan.solution.velocity)-75)<1e-6);check(!pinPhysicalPower(plan,0));
 }
 {using namespace ff3Native;Frame live;live.pid=1;live.model=2;live.own=3;live.tool=4;live.target=5;live.revision=6;live.menuOpen=false;live.ready=live.held=live.inputThrow=true;live.at=10;live.head={0,8,0};live.targetPosition={100,8,0};live.targetVelocity={10,0,0};live.gravity=28;live.power=live.gamePower=80;live.preparationPower=95;live.fitOffset={2,0,1};live.solution=solve(live.head,live.targetPosition,live.targetVelocity,80,28,.05,false,5);
  Frame shot;check(commitCameraShot(live,10.04,6,{100,200},shot));const auto frozen=shot.solution;
  live.power=95;live.targetPosition={200,20,0};live.fitOffset={-8,7,5};live.selectedLob=true;
  auto during=committedThrowView(shot,live);check(during.committed&&during.power==80&&length(during.solution.velocity-frozen.velocity)<1e-10&&length(during.fitOffset-shot.fitOffset)<1e-10);
  auto other=live;++other.model;check(!committedThrowView(shot,other).committed);other=live;other.menuOpen=true;check(committedThrowView(shot,other).menuOpen&&!committedThrowView(shot,other).committed);
  for(double elapsed:{.05,.35,.8,2.9}){auto prediction=predictedThrow(shot,10.04+elapsed);check(length(prediction.velocity-frozen.velocity)<1e-10&&length(prediction.catchPoint-frozen.catchPoint)<1e-10&&shot.power==80&&shot.target==5&&length(shot.fitOffset-Vec{2,0,1})<1e-10);}
  check(!commitCameraShot(live,10.05,6,{100,200},shot)); // Never arm a power the game has not acknowledged.
 }
 {const Vec origin{0,8,0},target{100,8,0},movement{10,0,0};const double delay=.35;
  const auto preview=solve(origin,target+movement*delay,movement,80,28,.05,false,5);check(preview.valid);
  for(double elapsed:{0.,.1,.2,.35}){const auto during=solve(origin,target+movement*elapsed+movement*remainingAnimationDelay(delay,elapsed),movement,80,28,.05,false,5);check(during.valid&&length(during.velocity-preview.velocity)<1e-8&&length(during.catchPoint-preview.catchPoint)<1e-8);}
  check(std::abs(learnAnimationDelay(.35,.4,.05)-.35)<1e-10);check(learnAnimationDelay(.35,3,.05)==.35);check(remainingAnimationDelay(.35,.8)==0);
 }
 {using namespace ff3Native;Plan current,proposed;current.solution.valid=proposed.solution.valid=true;current.power=70;proposed.power=75;current.margin=2;proposed.margin=2.2;
  check(keepWheelPlan(current,proposed,1,2));proposed.power=80;check(!keepWheelPlan(current,proposed,1,2));check(keepWheelPlan(current,proposed,1,1.1));
  proposed.margin=5;check(!keepWheelPlan(current,proposed,1,1.1));proposed.margin=2.2;proposed.solution.valid=false;check(keepWheelPlan(current,proposed,1,2));current.solution.valid=false;check(!keepWheelPlan(current,proposed,1,1.1));
  current.solution.valid=proposed.solution.valid=true;current.solution.time=2;proposed.solution.time=1.8;current.margin=proposed.margin=3;current.power=70;proposed.power=75;check(!keepWheelPlan(current,proposed,1,1.1,true)&&!keepWheelPlan(current,proposed,1,2,true));
 }
 {SelectionLatch latch;latch.current=1;check(latch.update(2,true,1,false,.12)==1);check(latch.update(1,true,1.03,false,.12)==1);check(latch.update(2,true,1.06,false,.12)==1);check(latch.update(1,true,1.09,false,.12)==1);check(latch.update(2,true,1.2,false,.12)==1);check(latch.update(2,true,1.33,false,.12)==2);check(latch.update(3,true,1.34,true,.12)==3);}
}
