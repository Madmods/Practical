#define NOMINMAX
#include "../src/ff3_play_runtime.hpp"
#include <iostream>
int main(){
 winrt::init_apartment();using namespace ff3Play;unsigned checks=0;
 auto check=[&](bool ok,const char* why){++checks;if(!ok){std::cerr<<"FAIL "<<checks<<": "<<why<<'\n';std::exit(1);}};
 Scene s;s.valid=s.focused=s.grounded=s.activation=true;s.typing=s.menu=false;s.pid=1;s.model=100;s.own=200;s.player=300;s.at=10;s.head={0,8,0};s.shape={2,2,2,true};s.worldGravity=196.2;s.jumpVelocity=50;s.speed=22;s.cameraForward={0,0,-1};s.cameraRight={1,0,0};
 Config c;c.catchOn=c.jumpOn=true;ff3qb::BallFlight ball;ball.id=10;ball.airborne=true;ball.at=10;ball.position={0,8,-3};ball.velocity={0,0,30};ball.gravity=28;s.balls={ball};Planner planner;
 check(groundBallCandidate({0,.6,0},{},0),"stationary ground ball remains eligible without an airborne flight");
 check(groundBallCandidate({0,.6,0},{18,0,0},0),"rolling ground ball retains ground reach");
 check(!groundBallCandidate({0,8,0},{},0)&&!groundBallCandidate({0,.6,0},{0,-15,0},0),"airborne and descending balls do not use resting-ball reach");
 check(!groundBallCandidate({0,.6,0},{},NAN)&&!groundBallCandidate({0,-5,0},{},0),"unknown ground and underground balls cannot activate catch resizing");
 check(!nonSolidCatchFlags(255)&&!nonSolidCatchFlags(Offsets::PrimitiveFlags::CanCollide)&&!nonSolidCatchFlags(Offsets::PrimitiveFlags::Anchored),"ground reach rejects unreadable, solid and anchored catch parts");
 check(nonSolidCatchFlags(0)&&nonSolidCatchFlags(Offsets::PrimitiveFlags::CanTouch|Offsets::PrimitiveFlags::CanQuery),"ground reach accepts non-solid catch parts without changing touch or query flags");
 {BallEdits edits;std::vector<BallPart> ground{{{42,43,44,{.85,1.2,.85},true,45},{1,1,1},{0,.6,0},true}};publishBallGuides(s,edits,ground,true);auto snapshot=volumeSnapshot();check(snapshot.parts.size()==1&&snapshot.parts[0].guide&&sameSize(snapshot.parts[0].size,{.85,1.2,.85}),"ground guide publishes normal ball geometry independently of ball resizing");publishBallGuides(s,edits,ground,false);check(volumeSnapshot().parts.empty(),"disabling Mags clears ground guides too");}
 {
  BallPart air{{51,52,53,{.85,1.2,.85},true,54},{1,1,1},{0,8,0},true};
  BallPart floor{{61,62,63,{.85,1.2,.85},true,64},{1,1,1},{0,.6,0},true};
  BallEdits edits;edits.edits.push_back({air,{10,10,10},air.meshScale,false,true,false,true});
  publishBallGuides(s,edits,{air,floor},true);auto snapshot=volumeSnapshot();
  check(snapshot.parts.size()==2&&!snapshot.parts[0].guide&&snapshot.parts[1].guide,"active and resting balls each publish one sphere without duplicates");
  edits.edits.clear();publishBallGuides(s,edits,{air,floor},true);snapshot=volumeSnapshot();
  check(snapshot.parts.size()==2&&snapshot.parts[0].guide&&snapshot.parts[1].guide,"sphere persists when a ball leaves the enlargement set near landing");
  check(sameSize(snapshot.parts[0].size,air.part.size)&&sameSize(snapshot.parts[1].size,floor.part.size)&&floor.canCollide,"persistent guides retain measured normal ball geometry and collision");
  publishBallGuides(s,edits,{},true);check(volumeSnapshot().parts.empty(),"picked-up or removed loose balls clear their guides");
  publishBallGuides(s,edits,{floor},false);check(volumeSnapshot().parts.empty(),"pausing or disabling Mags clears persistent spheres");
 }
 {
  auto outgoing=ball;outgoing.thrower=s.player;check(!magBallCandidate(s,outgoing),"own Player throw keeps normal ball collision size");
  outgoing.thrower=s.own;check(!magBallCandidate(s,outgoing),"own character throw keeps normal ball collision size");
  outgoing.thrower=0;check(!magBallCandidate(s,outgoing),"unattributed clone waits for ownership before resizing");
  outgoing.thrower=999;check(magBallCandidate(s,outgoing),"identified incoming ball can still use Mags");
  auto quarterback=s;quarterback.possessed=true;check(!magBallCandidate(quarterback,outgoing),"equipped QB does not resize nearby balls");
 }
 auto d=planner.step(c,s,10);check(d.catchNow&&!d.jumpNow&&d.ball==10,"low pass catches without jump");check(!planner.step(c,s,10.01).catchNow,"catch cooldown prevents spam");
 {auto early=s;early.balls[0].position.z=-12;Planner p;auto prepare=p.step(c,early,10);check(prepare.catchNow&&prepare.catchHold&&prepare.eta>.25,"raises hands before arrival without a physical click");}
 for(int reason=0;reason<6;reason++){auto blocked=s;Planner p;switch(reason){case 0:blocked.focused=false;break;case 1:blocked.menu=true;break;case 2:blocked.typing=true;break;case 3:blocked.possessed=true;break;case 4:blocked.at=9;break;case 5:blocked.valid=false;break;}auto stopped=p.step(c,blocked,10);check(!stopped.catchNow&&!stopped.jumpNow&&!stopped.moving,"unsafe scene never acts");}
 {auto equipped=s;equipped.possessed=true;check(observing(equipped,10)&&!allowed(equipped,10),"passive Mags stays available with possession while catch input stays blocked");}
 {auto own=s;own.balls[0].thrower=s.player;Planner p;check(!p.step(c,own,10).catchNow,"does not catch its own outgoing throw");own.balls[0].thrower=0;own.balls[0].ended=true;check(!p.step(c,own,10).catchNow,"landed ball is ignored");}
 {auto high=s;high.balls[0].position={0,13,-8};Planner p;auto jump=p.step(c,high,10);check(jump.jumpNow&&jump.ball==10,"high reachable pass schedules jump");check(!p.step(c,high,10.01).jumpNow,"one jump per flight");high.grounded=false;p.reset();check(!p.step(c,high,10).jumpNow,"does not double jump");high.grounded=true;high.balls[0].position.y=100;p.reset();auto no=p.step(c,high,10);check(!no.jumpNow&&!no.catchNow,"unreachable high ball does not spam inputs");}
 {auto missing=s;missing.shape.measured=false;Planner p;auto no=p.step(c,missing,10);check(!no.catchNow&&!no.jumpNow,"missing catch geometry does not invent reach");}
 {auto high=s;high.naturalShape=high.shape;high.shape={18,16,17,true};high.balls[0].position={0,13,-8};Planner p;check(p.step(c,high,10).jumpNow,"enlarged Mags geometry cannot suppress a required jump");}
 check(receivingShape({2,2,1},{2,1,1},{1,2,1},{50,50,50},{50,50,50}).measured,"50-stud geometry remains valid for receiving");
 {auto stale=s;stale.balls[0].at=9;Planner p;check(!p.step(c,stale,10).catchNow,"stale flight is ignored");}
 {auto chase=s;chase.balls.clear();chase.carriers={{500,600,{0,8,-40},{10,0,0}}};Config rush; rush.rushOn=true;Planner p;auto run=p.step(rush,chase,10);check(run.moving&&run.target==500&&run.direction.x>0&&run.direction.z<0,"rush leads the opposing carrier");chase.activation=false;check(!p.step(rush,chase,10).moving,"release activation stops rush");chase.activation=true;chase.manual=true;check(!p.step(rush,chase,10).moving,"manual movement wins");chase.manual=false;chase.carriers.clear();check(!p.step(rush,chase,10).moving,"release of possession stops pursuit");chase.carriers={{500,600,{0,8,-400},{}}};check(!p.step(rush,chase,10).moving,"acquire range is enforced");chase.carriers={{500,600,{0,8,-2},{}}};check(!p.step(rush,chase,10).moving,"stops at the carrier instead of running through them");}
 {
  std::vector<std::pair<int,bool>> events;Inputs input([&](int key,bool down){events.push_back({key,down});return true;});
  auto saw=[&](int key,bool down){return std::find(events.begin(),events.end(),std::make_pair(key,down))!=events.end();};
  Decision catchIt;catchIt.catchNow=true;input.apply(c,s,catchIt,10);check(saw(VK_LBUTTON,true),"catch reaches mock input");events.clear();input.apply(c,s,{},10.03);check(events.empty(),"catch pulse lasts through input sampling");s.at=10.13;input.apply(c,s,{},10.13);check(saw(VK_LBUTTON,false),"catch pulse releases");s.at=10.08;
  Config rush;rush.rushOn=true;physicalEdge(VK_XBUTTON2,true);Decision run;run.moving=true;run.direction={0,0,-1};events.clear();input.apply(rush,s,run,10.08);check(saw('W',true),"rush uses camera-relative movement");
  events.clear();physicalEdge('A',true);input.apply(rush,s,run,10.09);check(saw('W',false)&&!saw('A',false),"physical WASD cancels owned movement without releasing manual input");physicalEdge('A',false);
  events.clear();physicalEdge(VK_XBUTTON2,false);input.apply(rush,s,run,10.09);check(events.empty(),"rush key release cannot inject movement");
  physicalEdge(VK_LBUTTON,true);events.clear();input.apply(c,s,catchIt,10.09);check(!saw(VK_LBUTTON,true),"manual catch is not duplicated");input.release();check(!saw(VK_LBUTTON,false),"manual catch is not released");physicalEdge(VK_LBUTTON,false);
  events.clear();auto possession=s;possession.possessed=true;input.apply(c,possession,catchIt,10.09);check(events.empty(),"possession prevents accidental throw input");
  input.apply(c,s,catchIt,10.09);events.clear();auto blur=s;blur.focused=false;input.apply(c,blur,{},10.10);check(saw(VK_LBUTTON,false),"focus loss releases an active catch pulse immediately");
  input.apply(c,s,catchIt,10.10);events.clear();auto disabled=c;disabled.catchOn=false;input.apply(disabled,s,{},10.11);check(saw(VK_LBUTTON,false),"turning catch off cancels a pending pulse");
  events.clear();auto bad=c;bad.catchKey=999;bad.jumpKey=-1;catchIt.jumpNow=true;input.apply(bad,s,catchIt,10.11);check(events.empty(),"invalid input codes cannot index the pulse array");
 }
 {int attempts=0;Inputs failed([&](int,bool down){return down||++attempts>1;});Decision d;d.catchNow=true;failed.apply(c,s,d,10.08);failed.release();check(failed.owned[VK_LBUTTON],"failed key release retains ownership for retry");failed.release();check(!failed.owned[VK_LBUTTON],"subsequent release retries the owned input");}
 {std::vector<std::pair<int,bool>> events;Inputs input([&](int key,bool down){events.push_back({key,down});return true;});Decision hold;hold.catchHold=hold.catchNow=true;auto frame=s;frame.at=11;
  input.apply(c,frame,hold,11);check(input.owned[VK_LBUTTON],"automatic hands start with no physical press");
  for(int i=1;i<=30;i++){frame.at=11+i*.02;hold.catchNow=false;input.apply(c,frame,hold,frame.at);}check(events.size()==1&&input.owned[VK_LBUTTON],"catch remains held through arrival instead of 65-ms tapping");
  frame.possessed=true;input.apply(c,frame,{},frame.at);check(events.size()==2&&!events.back().second&&!input.owned[VK_LBUTTON],"catch releases on possession without another throw press");
 }
 {
  std::vector<CatchPart> live={{1000,2000,3000,{2,3,4}},{1001,2000,3001,{2,3,4}}};ReachEdits edits;bool fail=false;
  ReachEdits::Read read=[&](const CatchPart& seed,CatchPart& out){for(const auto& p:live)if(p.id==seed.id){out=p;return true;}return false;};
  ReachEdits::Write write=[&](const CatchPart& part,Vec size){for(auto& p:live)if(p.id==part.id){if(fail&&p.id==1001)return false;p.size=size;return true;}return false;};
  check(edits.apply(live,4.5,read,write)&&sameSize(live[0].size,{4.5,4.5,4.5}),"Mags sets both parts to an absolute stud size");
  check(edits.apply(live,4.5,read,write)&&sameSize(live[0].size,{4.5,4.5,4.5}),"repeated frames do not compound scale");
  check(!edits.restore([](const auto&,auto&){return false;},write)&&edits.edits.size()==2,"transient read failure preserves restoration records");
  check(edits.apply(live,50,read,write)&&sameSize(live[0].size,{50,50,50}),"50-stud maximum keeps restorable original dimensions");
  check(edits.apply(live,Vec{20,4,12},read,write)&&sameSize(live[0].size,{20,4,12})&&sameSize(live[1].size,{20,4,12}),"custom dimensions apply to both hands");
  check(edits.apply(live,Vec{20,4,12},read,write)&&sameSize(edits.edits[0].original.size,{2,3,4}),"custom dimensions retain the true original across repeated frames");
  check(edits.restore(read,write)&&sameSize(live[0].size,{2,3,4})&&sameSize(live[1].size,{2,3,4}),"disable restores both original parts");
  fail=true;check(!edits.apply(live,2,read,write)&&sameSize(live[0].size,{2,3,4}),"partial failure rolls back earlier part");fail=false;
  check(edits.apply(live,2,read,write),"can retry after failed write");live[0].size={5,5,5};check(edits.restore(read,write)&&sameSize(live[0].size,{5,5,5}),"restoration never overwrites a new game size");
  check(edits.apply(live,1.5,read,write),"can acquire current parts");live[0].primitive++;auto changed=live[0].size;check(edits.restore(read,write)&&sameSize(live[0].size,changed),"reused primitive identity is never written");
  check(!edits.apply(live,50.1,read,write),"reach scale bounds enforced");
 }
 check(set(L"ff3_mags.studs",basketballUi::number(4.5)),"Mags setting accepted");check(!set(L"ff3_mags.studs",basketballUi::number(50.1)),"invalid scale rejected");
 check(set(L"ff3_rush.input",basketballUi::keys(VK_XBUTTON1)),"rush side-button binding accepted");check(!set(L"ff3_rush.input",basketballUi::keys('W')),"movement key conflicts rejected");
 auto file=std::filesystem::temp_directory_path()/(L"ff3-play-"+std::to_wstring(GetCurrentProcessId())+L".json");catchOn=jumpOn=magsOn=rushOn=true;check(save(file),"settings save");reachTenths=50;load(file);check(reachTenths==45&&!any(),"preferences persist while startup automation stays off");std::filesystem::remove(file);
 {auto legacy=std::filesystem::temp_directory_path()/(L"ff3-play-legacy-"+std::to_wstring(GetCurrentProcessId())+L".json");{std::ofstream out(legacy);out<<"{\"ff3_mags\":{\"on\":true,\"rows\":{\"reach\":300}}}";}reachTenths=50;load(legacy);check(reachTenths==45&&!magsOn,"old percentage setting migrates to studs with automation off");std::filesystem::remove(legacy);}
 check(set(L"ff3_mags.studs",basketballUi::number(50))&&reachTenths==500,"maximum is exactly 50 studs");check(!set(L"ff3_mags.studs",basketballUi::number(.5)),"below-minimum stud value rejected");
 check(magsVisual&&set(L"ff3_mags.visual",basketballUi::boolean(false))&&!magsVisual,"volume outline starts on and can be disabled");check(set(L"ff3_mags.visual",basketballUi::boolean(true))&&magsVisual,"volume outline can be restored");
 check(sameSize(catchSize(0,20,{}),{20,20,20}),"Cube preserves the working uniform method");
 check(sameSize(catchSize(1,20,{}),{20,10,20}),"Wide uses independent horizontal and vertical dimensions");
 check(sameSize(catchSize(2,20,{}),{10,20,10}),"Tall uses a narrow vertical volume");
 check(sameSize(catchSize(3,20,{18,9,12}),{18,9,12}),"Custom uses explicit axis dimensions");
 check(validCatchSize(catchSize(1,1,{}))&&!validCatchSize({20,0,20})&&!validCatchSize({50,50,51}),"preset minimum and custom bounds are enforced");
 {std::vector<std::pair<int,bool>> events;Inputs input([&](int key,bool down){events.push_back({key,down});return true;});Config refresh=c;refresh.refresh=true;refresh.refreshPeriod=.7;Decision hold;hold.catchHold=true;hold.eta=.4;auto frame=s;frame.at=13;
  input.apply(refresh,frame,hold,13);frame.at=13.72;input.apply(refresh,frame,hold,13.72);check(events.size()==2&&!events.back().second&&!input.owned[VK_LBUTTON],"long-window refresh releases only its owned catch input");
  frame.at=13.74;input.apply(refresh,frame,hold,13.74);check(events.size()==2,"refresh includes a real release interval");
  frame.at=13.78;input.apply(refresh,frame,hold,13.78);check(events.size()==3&&events.back().second&&input.refreshCount==1,"refresh raises hands again after the release interval");
  hold.eta=.05;frame.at=14.6;input.apply(refresh,frame,hold,14.6);check(events.size()==3&&input.owned[VK_LBUTTON],"imminent arrival preserves the catch hold instead of refreshing");
  physicalEdge(VK_LBUTTON,true);frame.at=14.7;input.apply(refresh,frame,hold,14.7);input.release();check(events.size()==3,"refresh cannot release a manually held catch");physicalEdge(VK_LBUTTON,false);
 }
 {stop();magsOn=true;check(!config().catchOn,"Mags does not enable hidden catch input");
  check(!set(L"ff3_mags.hands",basketballUi::boolean(true))&&!set(L"ff3_mags.width",basketballUi::number(18)),"removed axis and hands settings cannot silently activate");
  check(set(L"ff3_mags.studs",basketballUi::number(20))&&sameSize(magsDimensions(),{20,20,20}),"one slider controls every dimension");
  check(set(L"ff3_mags.delay",basketballUi::number(120))&&magDelayMs==120,"activation delay accepted");check(!set(L"ff3_mags.delay",basketballUi::number(501)),"delay cap enforced");
  auto cfg=std::filesystem::temp_directory_path()/(L"ff3-ball-mags-"+std::to_wstring(GetCurrentProcessId())+L".json");check(save(cfg),"unified preferences save");reachTenths=500;magDelayMs=0;load(cfg);
  check(reachTenths==200&&magDelayMs==120&&!any(),"size and delay persist while automation stays off");auto rows=modules()[L"ff3_mags"][L"rows"];check(rows.o.size()==3&&rows[L"shape"].type==json::Value::Null,"unified schema contains only size, delay and visual");
  {std::ofstream out(cfg);out<<"{\"ff3_mags\":{\"rows\":{\"shape\":3,\"width\":50,\"height\":9,\"depth\":12,\"hands\":true,\"studs\":20}}}";}load(cfg);check(sameSize(magsDimensions(),{20,20,20})&&!config().catchOn,"old presets migrate without hidden automatic controls");std::filesystem::remove(cfg);
 }
 {BallDelay gate;check(gate.ready({1},10,100).empty(),"new ball waits for activation delay");check(gate.ready({1},10.05,100).empty(),"partial delay cannot activate");check(gate.ready({1},10.1,100)==std::vector<std::uint64_t>{1},"continuous presence activates at configured delay");gate.ready({},10.2,100);check(gate.ready({1},10.3,100).empty(),"leaving range resets delay");check(gate.ready({2,2,0},10.31,0)==std::vector<std::uint64_t>{2},"zero delay is immediate without duplicate ball edits");}
 {std::vector<CatchPart> live={{1000,2000,3000,{1.4,1.65,1.4}},{1001,2000,3001,{1.4,1.65,1.4}},{1002,2100,3002,{.85,1.2,.85},true,4000}};ReachEdits edits;
  ReachEdits::Read read=[&](const CatchPart& seed,CatchPart& out){for(const auto& p:live)if(p.id==seed.id){out=p;if(seed.ball&&p.rig)out.rig=seed.rig;return true;}return false;};
  ReachEdits::Write write=[&](const CatchPart& p,Vec size){for(auto& part:live)if(p.id==part.id){part.size=size;return true;}return false;};
  check(edits.apply(live,20,read,write)&&sameSize(live[2].size,{20,20,20}),"legacy recovery fixture retains old combined dimensions");check(edits.edits[2].original.ball&&edits.edits[2].original.mesh==4000,"ball lease retains its separate mesh identity");
  live[2].rig=9000;std::vector<CatchPart> hands{live[0],live[1]};check(edits.apply(hands,20,read,write)&&sameSize(live[2].size,{.85,1.2,.85}),"caught ball restores after moving to a tool or character");
  check(edits.restore(read,write)&&sameSize(live[0].size,{1.4,1.65,1.4}),"legacy recovery restores original hand dimensions");
 }

 {auto oldRecovery=recoveryPath;recoveryPath=std::filesystem::temp_directory_path()/(L"ff3-multi-ball-journal-"+std::to_wstring(GetCurrentProcessId())+L".json");Mags journal;journal.pid=GetCurrentProcessId();journal.h=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|PROCESS_VM_READ|SYNCHRONIZE,FALSE,journal.pid);
  for(size_t i=0;i<maxBallEdits;++i){BallPart part{{65536+i*32,90000,100000+i*32,{.85,1.2,.85},true,200000+i*32},{.7,.9,.7}};journal.edits.edits.push_back({part,{50,50,50},part.meshScale,false,true,false,true,5,true});journal.edits.edits.back().original.physicsGroup=1;}
  check(journal.h&&journal.journal(),"all replicated balls can be journaled before mutation");auto parsed=basketballUi::readConfig(recoveryPath);check(parsed[L"parts"].a.size()==maxBallEdits&&std::filesystem::file_size(recoveryPath)<65536,"64-ball journal fits the production parser budget");check(parsed[L"version"].num()==4&&parsed[L"parts"].a[63][L"collisionOwned"].boolean(),"version-4 recovery retains the final collision record");check(parsed[L"parts"].a[63][L"groupOriginal"].num()==1&&parsed[L"parts"].a[63][L"groupApplied"].num()==5&&parsed[L"parts"].a[63][L"groupOwned"].boolean(),"version-4 journal preserves original/applied group and restoration ownership");journal.edits.edits.clear();check(journal.restore()&&!std::filesystem::exists(recoveryPath),"empty recovery cleanup needs no process writes");recoveryPath=oldRecovery;
 }
 winrt::uninit_apartment();std::cout<<checks<<" football assist checks passed (mock inputs and memory only)\n";
}
