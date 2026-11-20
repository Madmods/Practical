#pragma once
#include "ff3_qb_logic.hpp"
#include "ff3_throw_state.hpp"
#include "basketball_ui_bridge.hpp"
#include <iomanip>
#include <functional>
#include <memory>
#include <thread>
namespace ff3Native {
using ff3qb::Vec;
inline std::atomic<bool> enabled{false},autoPower{true},practice{true},visuals{true};
inline std::atomic<int> arc{0};
inline std::atomic<int> passMode{0},manualAngle{35},selectionRadius{20};
inline std::atomic<bool> automaticAngle{true},smartFit{true},showHud{false},showCandidates{true};
inline std::atomic<unsigned> revision{0};
inline std::atomic<addr> lockedTarget{0};
inline std::atomic<int> diveReach{12},leadDistance{5},catchHeightAdjustment{0},beamLeadDistance{3},beamCatchHeight{0};
inline std::atomic<bool> force95{false},ballTrails{true};
inline std::atomic<addr> heldTarget{0};inline std::atomic<bool> physicalLeftHeld{false};inline std::atomic<ULONGLONG> physicalReleasedAt{0};
inline int selectedLead(){return force95?beamLeadDistance.load():leadDistance.load();}
inline int selectedHeight(){return force95?beamCatchHeight.load():catchHeightAdjustment.load();}
inline std::filesystem::path logPath;
inline HHOOK mouse=nullptr;
inline HHOOK keyboard=nullptr;
inline std::thread inputPump;inline std::mutex inputPumpMutex;inline std::atomic<DWORD> inputPumpId{0};inline std::atomic<bool> inputPumpReady{false};
inline constexpr bool packetMode=true;
inline std::atomic<ULONGLONG> physicalClickAt{0};
inline std::atomic<double> renderHz{0},renderMs{0};
struct Frame {
 bool studio=false,practiceRange=false;
 bool menuOpen=true;
 std::shared_ptr<Frame> releaseVisual;bool afterRelease=false;
 unsigned revision=0;
 double at=0,gravity=0,power=0,gamePower=0,ping=0,margin=0,distance=0,planningMs=0;DWORD pid=0;addr model=0,own=0,tool=0,target=0;
 addr localPlayer=0,team=0,heldBall=0,headPrimitive=0;ff3qb::ThrowView throwView;std::vector<ff3qb::Defender> hudDefenders;std::vector<bool> hudShapeMeasured;bool hudReachEstimated=false;bool hudCoverageKnown=false,fixedPowerRoute=false;double appliedLead=0;
 HWND window=nullptr;RECT client{};POINT origin{},cursor{};float matrix[16]{};
 std::vector<ff3qb::BallFlight> balls;
 ff3qb::Solution solution;ff3qb::Screen aim,receiver;
 addr cameraMatrix=0;Vec gameMouse{NAN,NAN,NAN};
 Vec head{},targetPosition{},targetVelocity{},leadOffset{},selectedPosition{};addr selectedPrimitive=0;double groundY=NAN;
 ff3qb::FieldBounds field;
 ff3qb::CoverageTrace coverageTrace;ff3qb::RiskForecast risk;
 ff3qb::Defender receiverReach;bool receiverReachKnown=false,wobbleFiltered=false;
 Vec rawTargetVelocity{},routeCorrection{};double deliveryMargin=0,launchMargin=0,minimumAngle=-30,maximumAngle=85;
 unsigned candidatesConsidered=0,candidatesRanked=0;size_t powersChecked=0;std::vector<ff3qb::Plan> rankedThrows;
 std::vector<ff3qb::Defender> defenders;bool guarded=false,fitBlocked=false;Vec fitOffset{};bool coverageKnown=false,wheelVisible=false,selectedLob=false;
 int mode=0,coverageCount=0;bool locked=false;std::string route,coverageDetail,passLabel;
 struct Candidate{ff3qb::Screen screen;std::string name;bool selected=false;};std::vector<Candidate> candidates;
 std::string name,reason="Enable QB aim to begin";bool held=false,ready=false,textFocus=false,targetGrace=false;
};
// One solve policy for preview and dispatch. Automatic plans already selected
// a power/root; pinning their angle again can demand >95 after tiny movement.
inline bool anglePowerPlan(const Frame& f){return autoPower&&!force95&&!f.fixedPowerRoute;}
inline bool isPracticeRange(std::uint64_t place){return place==81310542478972ULL;}
inline bool fieldAvailable(const Frame& f){return f.practiceRange||f.field.valid();}
inline const ff3qb::FieldBounds* throwBounds(const Frame& f){return f.practiceRange?nullptr:&f.field;}
inline bool catchAllowed(const Frame& f,const ff3qb::Solution& s,const ff3qb::Defender& receiver,double latency){
 if(!f.practiceRange)return ff3qb::inBoundsCatch(s,receiver,latency,f.field);
 return s.valid&&ff3qb::finite(s.catchPoint)&&std::isfinite(s.time)&&s.time>0&&std::isfinite(latency)&&latency>=0&&latency<=.3&&ff3qb::receiverGap(receiver,s.catchPoint,s.time+latency)<=-.25;
}
inline ff3qb::Solution predictedThrow(const Frame& f,double time){
 if(f.studio)return f.solution;
 const double age=std::clamp(time-f.at,0.,.45),latency=std::clamp(f.ping*.0005+.05,0.,.25);
 auto target=f.targetPosition+f.targetVelocity*age;
 const bool powerFirst=automaticAngle&&autoPower&&f.mode==0;
 const bool anglePower=anglePowerPlan(f);
 auto solved=anglePower?ff3qb::solveAngle(f.head,target,f.targetVelocity,f.solution.angle,f.gravity,latency,5):ff3qb::solve(f.head,target,f.targetVelocity,f.power,f.gravity,latency,automaticAngle&&autoPower?f.selectedLob:(arc==1||f.mode==2),5);
 // A retained Beam tier can fall just outside range as the receiver/QB moves.
 // Keep its elevation and recalculate power instead of losing the throw.
 if(!anglePower&&!solved.valid&&autoPower&&f.solution.angle>=5&&f.solution.angle<=85)solved=ff3qb::solveAngle(f.head,target,f.targetVelocity,f.solution.angle,f.gravity,latency,5);
 if(anglePower&&!solved.valid&&automaticAngle){
  auto low=ff3qb::solve(f.head,target,f.targetVelocity,95,f.gravity,latency,false,5),high=ff3qb::solve(f.head,target,f.targetVelocity,95,f.gravity,latency,true,5);
  for(const auto& candidate:{low,high})if(candidate.valid&&candidate.angle<=50&&(!solved.valid||std::abs(candidate.angle-f.solution.angle)<std::abs(solved.angle-f.solution.angle)))solved=candidate;
 }
 if(automaticAngle&&!force95&&solved.valid&&solved.angle>50)solved=ff3qb::solveAngle(f.head,target,f.targetVelocity,50,f.gravity,latency,5);
 if(solved.valid&&solved.angle>f.maximumAngle+1e-6)solved=ff3qb::solveAngle(f.head,target,f.targetVelocity,f.maximumAngle,f.gravity,latency,5);
 if(solved.valid&&solved.angle<f.minimumAngle-1e-6)solved=ff3qb::solveAngle(f.head,target,f.targetVelocity,f.minimumAngle,f.gravity,latency,5);
 auto receiver=f.receiverReach;receiver.position=receiver.position+Vec{receiver.velocity.x,0,receiver.velocity.z}*age;
 if((f.practiceRange||f.field.known)&&!catchAllowed(f,solved,receiver,latency))return {};
 return solved;
}
inline std::mutex mutex;inline Frame current,requested;inline double requestedAt=0;inline ff3qb::ClickGate clicks;
struct InputTrace {ULONGLONG at=0;unsigned sequence=0;std::string action,reason;bool blocked=false,transport=false,held=false,ready=false,menu=false,typing=false,enabled=false,drag=false;double ageMs=0;DWORD pid=0;addr target=0;};
inline InputTrace recentInput;inline std::vector<InputTrace> pendingInputTraces;
inline std::function<void(const Frame&)> frameSink;
inline std::function<void(unsigned,bool)> inputObserver;
inline std::function<bool()> extraVisuals;
inline std::function<void()> packetStop;
inline std::function<std::pair<bool,std::wstring>()> packetStatus;
inline std::function<bool()> packetInputReady;
inline std::function<bool()> packetRetryable;
inline std::function<Frame()> studioRead;
inline std::function<bool(Frame&)> studioView;
inline unsigned throwCount=0;inline std::string lastAction;inline double actionAt=0;
inline void record(const Frame& f,const char* action,bool sent=false){
 {std::lock_guard<std::mutex> lock(mutex);lastAction=action;actionAt=now();if(sent)++throwCount;}
 if(!logPath.empty()){std::ofstream out(logPath,std::ios::app);if(out)out<<std::setprecision(12)<<now()<<','<<action<<','<<f.pid<<','<<f.target<<','<<f.power<<','<<f.gravity<<','<<f.ping<<','<<f.solution.angle<<','<<f.solution.time<<','<<f.solution.origin.x<<','<<f.solution.origin.y<<','<<f.solution.origin.z<<','<<f.solution.velocity.x<<','<<f.solution.velocity.y<<','<<f.solution.velocity.z<<','<<f.aim.x<<','<<f.aim.y<<','<<f.gameMouse.x<<','<<f.gameMouse.y<<'\n';}
}
inline Frame snapshot(){std::lock_guard<std::mutex> lock(mutex);return current;}
inline bool canBridgeTargetGap(const Frame& previous,const Frame& live,double time){
 const bool lost=live.reason=="Aim near a teammate or practice receiver"||live.reason=="Locked receiver unavailable or off screen; Q unlocks";
 return lost&&!live.target&&!live.textFocus&&live.held&&previous.ready&&previous.solution.valid&&previous.target&&previous.pid&&previous.model&&previous.own&&previous.pid==live.pid&&previous.model==live.model&&previous.own==live.own&&previous.tool&&previous.tool==live.tool&&previous.revision==live.revision&&time>=previous.at&&time-previous.at<.1;
}
inline Frame bridgeTargetGap(const Frame& previous,const Frame& live,double time){
 if(!canBridgeTargetGap(previous,live,time))return live;
 auto held=previous;held.targetGrace=true;
 held.balls=live.balls;held.throwView=live.throwView;held.cursor=live.cursor;
 std::copy(std::begin(live.matrix),std::end(live.matrix),held.matrix);held.cameraMatrix=live.cameraMatrix;held.client=live.client;held.origin=live.origin;
 // Preserve the original sample timestamp. Repeated misses cannot renew grace.
 return held;
}
// Called with the input mutex held.
inline void cancelPress(){physicalLeftHeld=false;physicalReleasedAt=0;physicalClickAt=0;heldTarget=0;}
inline bool samePressSession(const Frame& a,const Frame& b){return a.pid&&a.pid==b.pid&&a.model&&a.model==b.model&&a.own&&a.own==b.own;}
inline void publish(Frame f){
 {std::lock_guard<std::mutex> lock(mutex);f=bridgeTargetGap(current,f,now());
  if(physicalLeftHeld&&f.pid&&f.model&&f.own&&!samePressSession(requested,f))cancelPress();
  if(physicalLeftHeld&&f.ready&&!f.menuOpen&&!f.textFocus&&samePressSession(requested,f)){requested=f;requestedAt=now();if(!heldTarget)heldTarget=f.target;}
  current=f;
 }if(frameSink)frameSink(f);
}
inline void expirePhysicalHold(ULONGLONG tick){
 std::lock_guard<std::mutex> lock(mutex);const auto clicked=physicalClickAt.load();
 if(physicalLeftHeld&&clicked&&tick>=clicked+50&&!(GetAsyncKeyState(VK_LBUTTON)&0x8000)){physicalLeftHeld=false;physicalReleasedAt=tick;}
 const auto released=physicalReleasedAt.load();if(!physicalLeftHeld&&released&&tick>=released&&tick-released>350)heldTarget=0;
}
inline bool admitted(){return basketballConnection::entered&&basketballConnection::releaseCurrent&&gameModules::router.active(now(),true)==gameModules::Id::FootballFusion3;}
inline bool focused(DWORD pid){DWORD currentPid=0;GetWindowThreadProcessId(GetForegroundWindow(),&currentPid);return pid&&pid==currentPid;}
// Called with the input mutex held. Hook callbacks only enqueue metadata;
// the worker writes it later, keeping disk I/O out of the mouse hook.
inline void traceInput(const char* action,bool blocked,bool transport,const Frame& f){
 InputTrace t;t.at=GetTickCount64();t.sequence=recentInput.sequence+1;t.action=action;t.reason=f.reason;t.blocked=blocked;t.transport=transport;t.held=f.held;t.ready=f.ready;t.menu=f.menuOpen;t.typing=f.textFocus;t.enabled=enabled;t.drag=false;t.ageMs=std::max(0.,now()-f.at)*1000;t.pid=f.pid;t.target=f.target;recentInput=t;
 if(pendingInputTraces.size()<64)pendingInputTraces.push_back(std::move(t));
}
inline json::Value inputTraceJson(const InputTrace& t){
 using namespace basketballUi;auto out=object();out.o[L"at"]=number(double(t.at));out.o[L"sequence"]=number(t.sequence);out.o[L"action"]=string(widen(t.action));out.o[L"reason"]=string(widen(t.reason));out.o[L"blocked"]=basketballUi::boolean(t.blocked);out.o[L"transport"]=basketballUi::boolean(t.transport);out.o[L"held"]=basketballUi::boolean(t.held);out.o[L"ready"]=basketballUi::boolean(t.ready);out.o[L"menu"]=basketballUi::boolean(t.menu);out.o[L"typing"]=basketballUi::boolean(t.typing);out.o[L"enabled"]=basketballUi::boolean(t.enabled);out.o[L"dragEnabled"]=basketballUi::boolean(t.drag);out.o[L"sampleAgeMs"]=number(t.ageMs);out.o[L"pid"]=number(t.pid);out.o[L"target"]=number(double(t.target));return out;
}
inline json::Value inputReport(){
 InputTrace t;bool held;ULONGLONG released;addr target;{std::lock_guard<std::mutex> lock(mutex);t=recentInput;held=physicalLeftHeld;released=physicalReleasedAt;target=heldTarget;}
 auto out=inputTraceJson(t);out.o[L"physicalHeld"]=basketballUi::boolean(held);out.o[L"releasedAt"]=basketballUi::number(double(released));out.o[L"pinnedTarget"]=basketballUi::number(double(target));out.o[L"hookThread"]=basketballUi::number(inputPumpId.load());return out;
}
inline void flushInputTraces(){
 std::vector<InputTrace> events;{std::lock_guard<std::mutex> lock(mutex);events.swap(pendingInputTraces);}if(events.empty()||logPath.empty())return;
 std::ofstream out(logPath.parent_path()/L"ff3-qb-input.jsonl",std::ios::app);if(out)for(const auto& event:events)out<<winrt::to_string(json::dump(inputTraceJson(event)))<<'\n';
}
// Retain the plan owned by a physical click through the game's unequip/release.
// It must have been fresh when clicked, and cannot survive a session or setting change.
inline bool clickPlanValid(const Frame& request,const Frame& live,double captured,double time,ULONGLONG clicked,ULONGLONG tick,unsigned version,ULONGLONG released=0,bool held=false){
 return ff3qb::pressAuthorized(clicked,released,held,tick)&&request.ready&&!request.textFocus&&request.revision==version&&request.pid&&request.pid==live.pid&&request.model==live.model&&request.own&&request.own==live.own&&captured>=request.at&&captured-request.at<.1&&time>=captured&&time-captured<=.35;
}
struct DispatchFrame {Frame frame;ULONGLONG clicked=0;bool retained=false,held=false;ULONGLONG released=0;};
inline DispatchFrame packetFrame(const Frame& live){std::lock_guard<std::mutex> lock(mutex);const auto clicked=physicalClickAt.load(),released=physicalReleasedAt.load();const bool held=physicalLeftHeld.load();const bool retained=clickPlanValid(requested,live,requestedAt,now(),clicked,GetTickCount64(),revision.load(),released,held);return {retained?requested:live,clicked,retained,held,released};}
inline bool freeCursor(){CURSORINFO info{sizeof info};return GetCursorInfo(&info)&&ff3qb::cursorModeAllowed((info.flags&CURSOR_SHOWING)!=0,(GetAsyncKeyState(VK_RBUTTON)&0x8000)!=0,false);}
inline Vec vector(shooting::Vec v){return {v.x,v.y,v.z};}
inline Vec position(HANDLE h,addr part){auto primitive=rd<addr>(h,part+Offsets::BasePart::Primitive);return vector(rd<shooting::Vec>(h,primitive+Offsets::Primitive::Position,{NAN,NAN,NAN}));}
inline Vec velocity(HANDLE h,addr part){return vector(rd<shooting::Vec>(h,rd<addr>(h,part+Offsets::BasePart::Primitive)+Offsets::Primitive::AssemblyLinearVelocity,{NAN,NAN,NAN}));}
inline Vec partSize(HANDLE h,addr part){return vector(rd<shooting::Vec>(h,rd<addr>(h,part+Offsets::BasePart::Primitive)+Offsets::Primitive::Size,{NAN,NAN,NAN}));}
inline std::pair<addr,addr> catchParts(HANDLE h,addr rig){
 for(auto names:{std::pair<const char*,const char*>{"CatchL","CatchR"},{"CatchLeft","CatchRight"}}){
  auto left=child(h,rig,names.first),right=child(h,rig,names.second);
  if(left&&right&&defense::partClass(defense::classOf(h,left))&&defense::partClass(defense::classOf(h,right)))return {left,right};
 }
 return {};
}
inline ff3qb::CatchGeometry readCatchGeometry(HANDLE h,addr rig,double ballRadius){
 auto parts=catchParts(h,rig);auto left=parts.first,right=parts.second;
 if(!left||!right)return {};
 auto a=ff3qb::catchGeometry(partSize(h,child(h,rig,"Torso")),partSize(h,child(h,rig,"Head")),partSize(h,child(h,rig,"Left Arm")),partSize(h,left),ballRadius);
 auto b=ff3qb::catchGeometry(partSize(h,child(h,rig,"Torso")),partSize(h,child(h,rig,"Head")),partSize(h,child(h,rig,"Right Arm")),partSize(h,right),ballRadius);
 if(!a.measured||!b.measured)return {};return {std::max(a.radius,b.radius),std::max(a.above,b.above),std::max(a.below,b.below),true};
}
inline bool descendant(HANDLE h,addr part,addr parent){return part&&parent&&rd<addr>(h,part+Offsets::Instance::Parent)==parent;}
struct FieldReader {
 addr workspace=0,ground=0,lines=0;std::vector<addr> stripes;double scanned=-10;
 ff3qb::FieldBounds read(HANDLE h,addr ws,double time){
  if(workspace!=ws||time<scanned||time-scanned>.5){
   workspace=ws;scanned=time;stripes.clear();auto field=path(h,ws,{"Models","Field"});
   ground=child(h,field,"Ground");lines=child(h,field,"Lines");
   for(auto part:children(h,lines))if(nm(h,part)=="Big"&&defense::partClass(defense::classOf(h,part)))stripes.push_back(part);
  }
  if(stripes.size()!=4||!ground||!lines||!descendant(h,ground,path(h,ws,{"Models","Field"}))||!descendant(h,lines,path(h,ws,{"Models","Field"})))return {};
  defense::Matrix3 missing;for(auto& v:missing.m)v=NAN;
  auto rotation=[&](addr part){return rd<defense::Matrix3>(h,rd<addr>(h,part+Offsets::BasePart::Primitive)+Offsets::Primitive::Rotation,missing);};
  auto r=rotation(ground);Vec right{r.m[0],r.m[3],r.m[6]},forward{r.m[2],r.m[5],r.m[8]};std::vector<ff3qb::FieldLine> measured;
  for(auto part:stripes){
   if(!descendant(h,part,lines)||nm(h,part)!="Big")return {};auto primitive=rd<addr>(h,part+Offsets::BasePart::Primitive);
   if(!(rd<unsigned char>(h,primitive+Offsets::Primitive::Flags)&Offsets::PrimitiveFlags::Anchored))return {};
   auto m=rotation(part);measured.push_back({position(h,part),partSize(h,part),{m.m[0],m.m[3],m.m[6]},{m.m[2],m.m[5],m.m[8]}});
  }
  return ff3qb::fieldFromLines(position(h,ground),right,forward,measured);
 }
};
struct Reader {
 FieldReader fieldReader;
 double groundY=NAN;addr groundWorkspace=0;
 ff3qb::FlightTracker flightTracker;std::vector<addr> flightParts;addr flightWorkspace=0;double flightScanAt=-10;
 void collectBalls(HANDLE h,addr ws,Frame& f,bool required=false){
  if(!required&&!ballTrails&&!(enabled&&(showHud||visuals))){flightTracker.clear();flightParts.clear();return;}
  if(flightWorkspace!=ws){flightTracker.clear();flightParts.clear();flightScanAt=-10;flightWorkspace=ws;}
  if(f.at-flightScanAt>.15){
   flightParts.clear();flightScanAt=f.at;std::vector<std::pair<addr,int>> queue{{ws,0}};size_t budget=0;
   for(size_t index=0;index<queue.size()&&budget<2048&&flightParts.size()<64;index++)for(auto o:children(h,queue[index].first)){
    if(++budget>2048)break;auto cls=defense::classOf(h,o);
    if(nm(h,o)=="Football"&&(cls=="Part"||cls=="MeshPart"||cls=="UnionOperation"))flightParts.push_back(o);
    else if(queue[index].second<2&&(cls=="Folder"||cls=="Model")&&!child(h,o,"Humanoid"))queue.push_back({o,queue[index].second+1});
   }
  }
  // Receiving has its own reader. Its expensive discovery pass must not stamp
  // fresh observations with the time from before discovery began.
  if(required)f.at=now();
  std::vector<ff3qb::BallObservation> observations;
  for(auto part:flightParts){auto parent=rd<addr>(h,part+Offsets::Instance::Parent);auto cls=defense::classOf(h,parent);if(!parent||cls=="Tool"||child(h,parent,"Humanoid"))continue;
   addr ancestor=parent;for(int depth=0;depth<4&&ancestor&&ancestor!=ws;depth++)ancestor=rd<addr>(h,ancestor+Offsets::Instance::Parent);if(ancestor!=ws)continue;
   auto primitive=rd<addr>(h,part+Offsets::BasePart::Primitive);auto p=position(h,part),v=velocity(h,part);if(!primitive||!ff3qb::finite(p))continue;if(!ff3qb::finite(v))v={};
   double gravity=-rd<double>(h,path(h,part,{"Configuration","Gravity"})+Offsets::Misc::Value,NAN);if(!std::isfinite(gravity)||gravity<1||gravity>300)gravity=0;
   auto by=child(h,part,"ThrownBy");auto thrower=by&&defense::classOf(h,by)=="ObjectValue"?rd<addr>(h,by+Offsets::Misc::Value):0;
   observations.push_back({part,p,v,gravity,(rd<unsigned char>(h,primitive+Offsets::Primitive::Flags)&Offsets::PrimitiveFlags::Anchored)!=0,thrower});
  }
  flightTracker.update(observations,f.at,f.groundY);f.balls=flightTracker.visible();
 }

 HANDLE handle=nullptr;DWORD pid=0;addr model=0,moduleBase=0,retained=0;double scanAt=-10;
 addr routeTarget=0,routeTool=0;ff3qb::SelectionLatch selection;ff3qb::SelectionRetention selectionRetention;ff3qb::RouteHistory routeHistory;ff3qb::AngleTracker angleHistory;ff3qb::BeamMemory beamMemory;ff3qb::RoutePassLatch routePass;Vec previousFit{};double previousPower=0;bool previousLob=false;double previousAngle=NAN;unsigned plannerRevision=0;double nextPlanSearch=0,lastSearch=0,lastPlanAt=0;
 struct Actor{addr rig=0,player=0,team=0;bool npc=false;ff3qb::CatchGeometry shape;};std::vector<Actor> actors;std::map<addr,double> potentialSpeeds;
 addr knownTool=0,knownBall=0,knownOwner=0;

 std::shared_ptr<Frame> releaseVisual;ff3qb::ThrowTracker throwTracker;ULONGLONG seenThrowClick=0;double shapeBallRadius=.6;
 void refreshActors(HANDLE h,addr root,addr ws,addr players,bool practicePlace,double time){
  if(model==root&&time-scanAt<=.25)return;
  const auto priorActors=actors;const bool sameModel=model==root;
  actors.clear();if(model!=root){potentialSpeeds.clear();retained=0;lockedTarget=0;routeTarget=0;}model=root;scanAt=time;
  auto local=rd<addr>(h,players+Offsets::Player::LocalPlayer);
  for(auto p:children(h,players)){auto rig=rd<addr>(h,p+Offsets::Player::ModelInstance);if(p!=local&&rig)actors.push_back({rig,p,rd<addr>(h,p+Offsets::Player::Team),false});}
  if(practicePlace)for(auto group:children(h,child(h,ws,"npcwr")))for(auto rig:children(h,group))if(defense::classOf(h,rig)=="Model"&&child(h,rig,"WR"))actors.push_back({rig,0,0,true});
  const addr keep=lockedTarget.load()?lockedTarget.load():retained;
  if(sameModel&&keep&&std::none_of(actors.begin(),actors.end(),[&](const Actor& a){return a.rig==keep;})){
   auto old=std::find_if(priorActors.begin(),priorActors.end(),[&](const Actor& a){return a.rig==keep;});
   if(old!=priorActors.end()&&descendant(h,keep,ws)&&rd<float>(h,child(h,keep,"Humanoid")+Offsets::Humanoid::Health,0)>0
      &&(old->npc||rd<addr>(h,old->player+Offsets::Player::ModelInstance)==keep))actors.push_back(*old);
  }
  for(auto& actor:actors)actor.shape=readCatchGeometry(h,actor.rig,shapeBallRadius);
  for(auto it=potentialSpeeds.begin();it!=potentialSpeeds.end();){if(std::none_of(actors.begin(),actors.end(),[&](const Actor& a){return a.rig==it->first;}))it=potentialSpeeds.erase(it);else ++it;}
 }
 static addr possessedBall(HANDLE h,addr container){
  if(!container)return 0;int budget=0;
  for(auto item:children(h,container)){if(++budget>64)break;auto cls=defense::classOf(h,item);auto part=cls=="Tool"?child(h,item,"Handle"):nm(h,item)=="Football"&&defense::partClass(cls)?item:0;if(part&&path(h,part,{"Configuration","Gravity"}))return part;}return 0;
 }
 std::vector<ff3qb::ThrowActor> throwActors(HANDLE h,addr ws,const Frame& f){
  std::vector<ff3qb::ThrowActor> out;double world=rd<float>(h,ws+Offsets::Workspace::ReadOnlyGravity,NAN);
  for(const auto& a:actors){
   if(a.npc){auto active=child(h,a.rig,"Enabled");if(active&&!rd<bool>(h,active+Offsets::Misc::Value))continue;}
   auto hum=child(h,a.rig,"Humanoid"),root=child(h,a.rig,"HumanoidRootPart");double hp=rd<float>(h,hum+Offsets::Humanoid::Health,NAN);if(std::isfinite(hp)&&hp<=0)continue;
   ff3qb::ThrowActor item;item.id=a.rig;item.name=nm(h,a.rig);item.reach.position=position(h,child(h,a.rig,"Head"));item.reach.velocity=velocity(h,root);
   item.valid=std::isfinite(hp)&&ff3qb::finite(item.reach.position)&&ff3qb::finite(item.reach.velocity)&&ff3qb::length(item.reach.velocity)<100&&std::isfinite(world)&&world>=10&&world<=300;
   item.sideKnown=a.npc||(f.team&&a.team);item.opponent=a.npc?!rd<bool>(h,child(h,a.rig,"WR")+Offsets::Misc::Value):f.team!=a.team;
   if(a.rig==throwTracker.intent.target){item.sideKnown=true;item.opponent=false;}
   auto& reach=item.reach;reach.worldGravity=std::isfinite(world)?world:196.2;double jump=rd<float>(h,hum+Offsets::Humanoid::JumpHeight,NAN);
   if(rd<bool>(h,hum+Offsets::Humanoid::UseJumpPower)){double jp=rd<float>(h,hum+Offsets::Humanoid::JumpPower,NAN);jump=jp*jp/(2*reach.worldGravity);}
   reach.jumpHeight=std::isfinite(jump)&&jump>=0&&jump<=40?jump:7;double walk=rd<float>(h,hum+Offsets::Humanoid::Walkspeed,NAN);reach.speed=std::isfinite(walk)&&walk>=0&&walk<=45?walk:22;reach.diveReach=diveReach;reach.radius=a.shape.radius;reach.above=a.shape.above;reach.below=a.shape.below;
   item.ball=possessedBall(h,a.rig);if(!item.ball)item.ball=possessedBall(h,child(h,a.npc?a.rig:a.player,"Backpack"));
   if(item.ball){auto by=child(h,item.ball,"ThrownBy");if(by&&defense::classOf(h,by)=="ObjectValue")item.thrower=rd<addr>(h,by+Offsets::Misc::Value);}
   out.push_back(item);
  }return out;
 }
 void updateThrow(HANDLE h,addr ws,Frame& f){
  if(!enabled||(!showHud&&!visuals)){throwTracker.clear();releaseVisual.reset();f.releaseVisual.reset();return;}
  Frame clicked;ULONGLONG click=0;double captured=0;
  {std::lock_guard<std::mutex> lock(mutex);click=physicalClickAt;clicked=requested;captured=requestedAt;}
  const double observedAt=now(); // A click can arrive after this reader frame began.
  const bool newClick=click&&click!=seenThrowClick;
  if(throwTracker.view.phase==ff3qb::ThrowView::Hidden&&!newClick){releaseVisual.reset();f.releaseVisual.reset();return;}
  auto people=throwActors(h,ws,f);
  if(newClick){seenThrowClick=click;if(clicked.held&&clicked.own==f.own&&clicked.model==f.model&&captured>=clicked.at&&captured-clicked.at<.1&&observedAt>=captured&&observedAt-captured<.5){
   ff3qb::ThrowIntent intent{click,f.model,f.own,f.localPlayer,clicked.target,clicked.heldBall,captured,clicked.power,clicked.solution.angle,clicked.solution.time,clicked.distance,clicked.ready?clicked.solution.catchPoint.y:NAN,clicked.name,force95?"Beam":ff3qb::presetName(clicked.mode)};
   intent.pid=f.pid;
   auto forecast=predictedThrow(clicked,captured);if(!forecast.valid)forecast=clicked.solution;const auto risk=ff3qb::riskForecast(forecast,clicked.gravity,clicked.hudDefenders,std::clamp(clicked.ping*.0005+.05,0.,.25),clicked.groundY);intent.predictionKnown=clicked.ready&&clicked.hudCoverageKnown;intent.predictedIntable=intent.predictionKnown&&risk.intable;if(intent.predictedIntable)intent.predictedRiskAt=risk.projected.time;
   throwTracker.begin(intent,clicked.balls,people);
   releaseVisual.reset();if(clicked.ready&&forecast.valid){releaseVisual=std::make_shared<Frame>(clicked);releaseVisual->releaseVisual.reset();releaseVisual->solution=forecast;releaseVisual->risk=risk;releaseVisual->at=captured;releaseVisual->afterRelease=true;}
  }}
  const auto before=throwTracker.view.phase;throwTracker.update(observedAt,f.model,f.own,flightTracker.flights,people,f.groundY,actors.size()<=128);f.throwView=throwTracker.view;f.releaseVisual=releaseVisual;
  if(before!=ff3qb::ThrowView::Done&&f.throwView.phase==ff3qb::ThrowView::Done){auto action="Throw result: "+f.throwView.result;record(f,action.c_str());}
 }
 ~Reader(){if(handle)CloseHandle(handle);}
 Frame read(addr forced=0){
  Frame f;f.releaseVisual=releaseVisual;f.at=now();f.revision=revision;f.mode=passMode;f.throwView=throwTracker.view;auto session=gameModules::router.get();f.pid=session.pid;f.model=session.model;f.practiceRange=isPracticeRange(session.placeId);
  if(!admitted()){f.reason="Connect to Football Fusion 3";return f;}
  if(pid!=f.pid){if(handle)CloseHandle(handle);handle=OpenProcess(PROCESS_VM_READ|SYNCHRONIZE,FALSE,f.pid);pid=f.pid;model=moduleBase=0;lockedTarget=0;routeTarget=0;flightWorkspace=0;groundY=NAN;flightTracker.clear();}
  if(!handle||WaitForSingleObject(handle,0)!=WAIT_TIMEOUT){f.reason="Waiting for the game process";return f;}
  auto h=handle;auto root=f.model;auto ws=child(h,root,"Workspace"),players=child(h,root,"Players");
  auto player=rd<addr>(h,players+Offsets::Player::LocalPlayer);f.localPlayer=player;f.team=rd<addr>(h,player+Offsets::Player::Team);f.own=rd<addr>(h,player+Offsets::Player::ModelInstance);
  auto head=child(h,f.own,"Head"),hum=child(h,f.own,"Humanoid");
  const float health=rd<float>(h,hum+Offsets::Humanoid::Health,NAN);
  f.window=GetForegroundWindow();if(!focused(f.pid)){f.reason="Focus Football Fusion 3";return f;}
  if(!GetClientRect(f.window,&f.client)||!ClientToScreen(f.window,&f.origin)||!GetCursorPos(&f.cursor)){f.reason="Waiting for viewport";return f;}
  const int width=f.client.right,height=f.client.bottom;
  auto camera=rd<addr>(h,ws+Offsets::Workspace::CurrentCamera);Vec cameraPosition=vector(rd<shooting::Vec>(h,camera+Offsets::Camera::Position,{NAN,NAN,NAN}));
  // Project using camera view/projection from the same version-gated process.
  // Resolve once per process, independently of Basketball's cleared scene.
  if(!moduleBase){HANDLE modules=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);MODULEENTRY32W entry{sizeof entry};if(modules!=INVALID_HANDLE_VALUE){if(Module32FirstW(modules,&entry))moduleBase=(addr)entry.modBaseAddr;CloseHandle(modules);}}
  const auto visual=rd<addr>(h,moduleBase+Offsets::VisualEngine::Pointer);
  struct Matrix{float m[16];};Matrix invalid;for(auto& v:invalid.m)v=NAN;const auto matrix=rd<Matrix>(h,visual+Offsets::VisualEngine::ViewMatrix,invalid);std::copy(std::begin(matrix.m),std::end(matrix.m),f.matrix);
  if(!ff3qb::finite(cameraPosition)||!visual||!std::all_of(std::begin(f.matrix),std::end(f.matrix),[](float v){return std::isfinite(v);})){f.reason="Camera data unavailable";return f;}
  f.cameraMatrix=visual+Offsets::VisualEngine::ViewMatrix;
  if(groundWorkspace!=ws){groundWorkspace=ws;groundY=NAN;}
  // Use the local grounded rig's foot plane; retain it while jumping.
  if(f.own&&rd<int>(h,hum+Offsets::Humanoid::FloorMaterial,1792)!=1792){
   auto foot=child(h,f.own,"LeftFoot");if(!foot)foot=child(h,f.own,"Left Leg");
   auto p=position(h,foot),size=partSize(h,foot),v=velocity(h,child(h,f.own,"HumanoidRootPart"));
   if(foot&&ff3qb::finite(p)&&ff3qb::finite(size)&&size.y>0&&size.y<8&&ff3qb::finite(v)&&std::abs(v.y)<2)groundY=p.y-size.y*.5;
  }
  f.groundY=groundY;if(enabled){if(!f.practiceRange)f.field=fieldReader.read(h,ws,f.at);refreshActors(h,root,ws,players,f.practiceRange,f.at);}collectBalls(h,ws,f);updateThrow(h,ws,f);
  if(!f.own||!std::isfinite(health)||health<=0){f.reason="Waiting for your live character";return f;}
  if(!enabled){f.reason="QB aim off";return f;}
  auto ui=child(h,root,"UserInputService"),input=rd<addr>(h,ui+Offsets::UserInputService::WindowInputState);
  f.textFocus=!input||rd<addr>(h,input+Offsets::WindowInputState::CurrentTextBox)!=0;
  {auto guiService=child(h,root,"GuiService");f.menuOpen=rd<unsigned char>(h,guiService+Offsets::GuiService::MenuIsOpen,255)!=0;}
  if(f.menuOpen){f.reason="Close the game menu";return f;}
  if(f.textFocus){f.reason="Typing or input data unavailable";return f;}
  f.tool=child(h,f.own,"Football");auto ball=child(h,f.tool,"Handle");
  // Match renamed equipped footballs by their physics-bearing handle.
  if(!ball)for(auto item:children(h,f.own))if(defense::classOf(h,item)=="Tool"){
   auto handlePart=child(h,item,"Handle"),value=path(h,handlePart,{"Configuration","Gravity"});
   if(handlePart&&value&&defense::classOf(h,value)=="NumberValue"){f.tool=item;ball=handlePart;break;}
  }
  // Reuse identity only when its current ownership is independently verified.
  // A released ball or unequipped tool fails these ancestry checks immediately.
  if(!ball&&knownOwner==f.own&&knownTool&&knownBall&&defense::classOf(h,knownTool)=="Tool"&&descendant(h,knownTool,f.own)&&descendant(h,knownBall,knownTool)){
   f.tool=knownTool;ball=knownBall;
  }
  f.held=f.tool&&defense::classOf(h,f.tool)=="Tool"&&descendant(h,f.tool,f.own)&&descendant(h,ball,f.tool);
  if(f.held){knownTool=f.tool;knownBall=ball;knownOwner=f.own;}

  f.heldBall=f.held?ball:0;
  auto ballSize=partSize(h,ball);double ballRadius=std::max({ballSize.x,ballSize.y,ballSize.z})*.5;
  const bool ballDimensions=ff3qb::finite(ballSize)&&ballRadius>0&&ballRadius<=3;
  if(f.held&&ballDimensions)shapeBallRadius=ballRadius;
  std::vector<ff3qb::PowerDisplay> powerDisplays;
  for(auto gui:children(h,child(h,player,"PlayerGui")))if(nm(h,gui)=="BallGui"&&defense::classOf(h,gui)=="ScreenGui"){
   auto frame=child(h,gui,"Frame0"),display=child(h,frame,"Disp");
   if(!descendant(h,frame,gui)||!descendant(h,display,frame)||defense::classOf(h,display)!="TextLabel")continue;
   powerDisplays.push_back({rd<unsigned char>(h,gui+Offsets::GuiObject::ScreenGui_Enabled),rd<unsigned char>(h,frame+Offsets::GuiObject::Visible),defense::textAt(h,display+Offsets::GuiObject::Text)});
  }
  const int visiblePower=ff3qb::readPower(f.held,powerDisplays);
  f.gamePower=std::max(0,visiblePower);f.wheelVisible=f.gamePower>0;f.power=f.gamePower;
  const std::string powerReason=visiblePower==-2?"Visible power displays disagree":visiblePower==-1?"Football power text is invalid":"Waiting for game power; enable calculated power to throw without the wheel";

  if(!packetMode&&(!freeCursor()||rd<int>(h,player+Offsets::Player::CameraMode,-1)!=0||ff3qb::length(cameraPosition-position(h,head))<2.0)){f.reason="Use third-person with shift-lock off; release right mouse";return f;}
  // Roblox can hide the Windows cursor while drawing its own cursor. Hidden
  // does not mean center-locked. GetCursorPos also follows a genuinely locked
  // cursor, without alternating between two unrelated selection positions.
  if(!lockedTarget&&!heldTarget&&(f.cursor.x<f.origin.x||f.cursor.y<f.origin.y||f.cursor.x>=f.origin.x+width||f.cursor.y>=f.origin.y+height)){f.reason="Move the cursor into the game";return f;}
  auto mouseService=child(h,root,"MouseService"),mouseInput=rd<addr>(h,mouseService+Offsets::MouseService::InputObject);
  f.gameMouse=vector(rd<shooting::Vec>(h,mouseInput+Offsets::MouseService::MousePosition,{NAN,NAN,NAN}));
  if(!packetMode&&(!ff3qb::finite(f.gameMouse)||f.gameMouse.x<0||f.gameMouse.x>width||f.gameMouse.y<0||f.gameMouse.y>height)){f.reason="Game cursor data unavailable";return f;}

  const auto ownTeam=rd<addr>(h,player+Offsets::Player::Team);const bool practicePlace=f.practiceRange;
  Vec origin=position(h,head);f.head=origin;f.headPrimitive=rd<addr>(h,head+Offsets::BasePart::Primitive);double best=std::min(double(selectionRadius.load()),std::min(width,height)*.35);
  std::vector<ff3qb::Defender> defenders;
  const addr pinned=heldTarget.load()?heldTarget.load():lockedTarget.load();const addr wanted=forced?forced:pinned;f.locked=pinned!=0;
  Vec targetPosition{},targetVelocity{},retainedPosition{},retainedVelocity{};Frame retainedFrame;bool retainedAvailable=false;
  for(const auto& a:actors){
   if(wanted&&a.rig!=wanted)continue;
   if(a.npc){if(!practice||!practicePlace||!rd<bool>(h,child(h,a.rig,"WR")+Offsets::Misc::Value))continue;auto active=child(h,a.rig,"Enabled");if(active&&!rd<bool>(h,active+Offsets::Misc::Value))continue;}
   else if(ownTeam){if(rd<addr>(h,a.player+Offsets::Player::Team)!=ownTeam)continue;}
   else if(!practice||!practicePlace||rd<addr>(h,a.player+Offsets::Player::Team)!=0)continue;
   auto hrp=child(h,a.rig,"HumanoidRootPart"),targetHum=child(h,a.rig,"Humanoid");auto hp=rd<float>(h,targetHum+Offsets::Humanoid::Health,NAN);
   if(!std::isfinite(hp)||hp<=0||!hrp)continue;
   Vec p=position(h,child(h,a.rig,"Head")),v=velocity(h,hrp);if(!ff3qb::finite(p))p=position(h,hrp)+Vec{0,1.5,0};if(!ff3qb::finite(p))continue;if(ff3qb::finite(v))v.y=0;
   auto screen=ff3qb::project(p,f.matrix,width,height);if(!screen.valid&&!wanted)continue;
   auto top=ff3qb::project(p+Vec{0,1.5,0},f.matrix,width,height),bottom=ff3qb::project(p-Vec{0,4.5,0},f.matrix,width,height);
   double distance=top.valid&&bottom.valid?ff3qb::selectionScore(f.cursor.x-f.origin.x,f.cursor.y-f.origin.y,bottom.x,bottom.y,top.x,top.y,std::clamp(std::abs(top.y-bottom.y)*.18,2.,14.)):std::hypot(screen.x-(f.cursor.x-f.origin.x),screen.y-(f.cursor.y-f.origin.y));
   f.candidates.push_back({screen,nm(h,a.rig),false});
   if(ff3qb::length(p-origin)<6||ff3qb::length(p-origin)>1000)continue;
   if(a.rig==retained&&selectionRetention.keep(a.rig,distance,std::min(double(selectionRadius.load()),std::min(width,height)*.35),f.at,wanted!=0)){retainedAvailable=true;retainedFrame.target=a.rig;retainedFrame.name=nm(h,a.rig);retainedFrame.receiver=screen;retainedPosition=p;retainedVelocity=v;}
   if(!wanted&&(distance>best||(distance==best&&f.target&&(f.target==retained||(a.rig!=retained&&a.rig>f.target)))))continue;
   best=distance;f.target=a.rig;f.name=nm(h,a.rig);f.receiver=screen;targetPosition=p;targetVelocity=v;
  }
  selection.current=retained;auto selected=selection.update(f.target,retainedAvailable,f.at,wanted!=0,0);
  if(selected&&selected!=f.target&&retainedAvailable){f.target=retainedFrame.target;f.name=retainedFrame.name;f.receiver=retainedFrame.receiver;targetPosition=retainedPosition;targetVelocity=retainedVelocity;}
  if(!f.target){retained=0;selection={};selectionRetention={};f.reason=pinned?"Locked receiver unavailable or off screen; Q unlocks":"Aim near a teammate or practice receiver";return f;}
  if(retained!=f.target){selectionRetention={};selectionRetention.keep(f.target,0,selectionRadius.load(),f.at,wanted!=0);}
  retained=f.target;f.selectedPosition=targetPosition;f.selectedPrimitive=rd<addr>(h,child(h,f.target,"Head")+Offsets::BasePart::Primitive);for(auto& c:f.candidates)c.selected=c.screen.x==f.receiver.x&&c.screen.y==f.receiver.y;
  // Selection is independent of throw eligibility. A missing ball/velocity
  // sample must not erase a Q lock or its receiver marker.
  if(!f.held){f.reason="Equip a football";return f;}
  if(!ballDimensions){f.reason="Ball dimensions unavailable";return f;}
  if(!fieldAvailable(f)){f.reason="Field boundaries unavailable: no assisted throw";return f;}
  if(!ff3qb::finite(targetVelocity)||std::hypot(targetVelocity.x,targetVelocity.z)>45){f.reason="Waiting for receiver movement data";return f;}
  const bool receiverChanged=routeTarget!=f.target||routeTool!=f.tool;
  if(receiverChanged)routeHistory.reset();
  if(receiverChanged||plannerRevision!=f.revision){plannerRevision=f.revision;routeTarget=f.target;routeTool=f.tool;angleHistory={};beamMemory={};routePass={};previousFit={};previousPower=0;previousLob=false;previousAngle=NAN;nextPlanSearch=0;}
  const Vec measuredReceiverPosition=targetPosition;f.rawTargetVelocity=targetVelocity;
  routeHistory.update(f.at,origin,position(h,child(h,f.target,"HumanoidRootPart")),targetVelocity);targetVelocity=routeHistory.movement;if(ff3qb::length(targetVelocity)<.5)targetVelocity={};
  f.wobbleFiltered=routeHistory.intent.wobble;f.routeCorrection=routeHistory.intent.correction;targetPosition=targetPosition+f.routeCorrection;
  f.route=routeHistory.route;
  bool coverageComplete=std::isfinite(f.groundY);f.hudCoverageKnown=true;int measuredShapes=0;const double worldGravity=rd<float>(h,ws+Offsets::Workspace::ReadOnlyGravity,NAN);
  if(!std::isfinite(worldGravity)||worldGravity<10||worldGravity>300)coverageComplete=false;
  // All other live players and enabled practice bots can contest a catch.
  // Do not require a team assignment in the practice place.
  for(const auto& a:actors)if(a.rig!=f.target&&a.rig!=f.own){
   if(a.npc){auto active=child(h,a.rig,"Enabled");if(active&&defense::classOf(h,active)=="BoolValue"&&!rd<bool>(h,active+Offsets::Misc::Value))continue;}
   auto hrp=child(h,a.rig,"HumanoidRootPart"),humanoid=child(h,a.rig,"Humanoid");float health=rd<float>(h,humanoid+Offsets::Humanoid::Health,NAN);
   if(std::isfinite(health)&&health<=0)continue;
   auto p=position(h,child(h,a.rig,"Head")),v=velocity(h,hrp);if(!ff3qb::finite(p))p=position(h,hrp)+Vec{0,1.5,0};
   if(!hrp||!std::isfinite(health)||!ff3qb::finite(p)||!ff3qb::finite(v)){coverageComplete=false;f.hudCoverageKnown=false;continue;}
   if(ff3qb::length(p-origin)>1200)continue;
   double jump=rd<float>(h,humanoid+Offsets::Humanoid::JumpHeight,NAN);
   if(rd<bool>(h,humanoid+Offsets::Humanoid::UseJumpPower)){double power=rd<float>(h,humanoid+Offsets::Humanoid::JumpPower,NAN),world=rd<float>(h,ws+Offsets::Workspace::ReadOnlyGravity,NAN);if(std::isfinite(power)&&power>=0&&power<150&&std::isfinite(world)&&world>10&&world<300)jump=power*power/(2*world);}
   if(!std::isfinite(jump)||jump<0||jump>40)jump=7;
   double walk=rd<float>(h,humanoid+Offsets::Humanoid::Walkspeed,NAN);if(!std::isfinite(walk)||walk<0||walk>45)walk=22;
   if(std::hypot(v.x,v.z)>45){coverageComplete=false;continue;}
   const double potential=ff3qb::movementPotential(walk,v,potentialSpeeds.count(a.rig)?potentialSpeeds[a.rig]:22.);potentialSpeeds[a.rig]=potential;
   defenders.push_back({p,v,potential,jump,double(diveReach.load()),a.shape.radius,a.shape.above,a.shape.below,std::isfinite(worldGravity)?worldGravity:196.2});
   const bool sideKnown=a.npc||(f.team&&a.team);
   if(!sideKnown||(a.npc?!rd<bool>(h,child(h,a.rig,"WR")+Offsets::Misc::Value):a.team!=f.team)){f.hudDefenders.push_back(defenders.back());f.hudShapeMeasured.push_back(a.shape.measured);if(!a.shape.measured)f.hudReachEstimated=true;}
   if(a.shape.measured)++measuredShapes;
  }
  f.coverageKnown=coverageComplete;f.hudCoverageKnown=f.hudCoverageKnown&&coverageComplete;f.coverageCount=int(defenders.size());
  f.coverageDetail=!coverageComplete?"Some catcher data unavailable":defenders.empty()?"No other active catchers":std::to_string(defenders.size())+" catchers / jump + dive estimate";
  if(!defenders.empty())f.coverageDetail+=" / "+std::to_string(measuredShapes)+" measured shapes";
  if(!defenders.empty()){double nearest=1e9;for(const auto& d:defenders)nearest=std::min(nearest,ff3qb::length(d.position-targetPosition));f.coverageDetail+= " / nearest "+std::to_string(int(std::lround(nearest)))+" st";}
  f.targetPosition=targetPosition;f.targetVelocity=targetVelocity;f.defenders=defenders;f.guarded=smartFit;
  if(smartFit&&(!f.coverageKnown||defenders.size()>128)){f.fitBlocked=true;f.reason="Coverage incomplete: no assisted throw; normal click unchanged";return f;}
  if(!ff3qb::powerAvailable(f.held,autoPower,visiblePower)){f.reason=powerReason;return f;}
  const auto gravity=path(h,ball,{"Configuration","Gravity"});
  f.gravity=-rd<double>(h,gravity+Offsets::Misc::Value,NAN);
  if(!gravity||defense::classOf(h,gravity)!="NumberValue"||!std::isfinite(f.gravity)||f.gravity<1||f.gravity>300){f.reason="Ball physics unavailable";return f;}
  auto ping=path(h,root,{"Stats","PerformanceStats","Ping"});double pingValue=rd<double>(h,ping+0x98,NAN);
  if(!std::isfinite(pingValue)||pingValue<1||pingValue>400){f.reason="Waiting for usable ping";return f;}f.ping=pingValue;

  // The server launches five studs along the sent direction. Solve that
  // offset exactly, using the same model as the packet editor.
  const bool powerFirst=automaticAngle&&autoPower&&f.mode==0;
  const double desired=automaticAngle?ff3qb::presetAngle(f.mode,origin,targetPosition,targetVelocity):double(manualAngle.load());
  const double stableAngle=automaticAngle?angleHistory.update(desired,f.at):desired;
  f.appliedLead=selectedLead();
  f.leadOffset=ff3qb::leadAdjustment(origin,targetPosition,targetVelocity,selectedLead());f.leadOffset.y=selectedHeight();const Vec aimTarget=targetPosition+f.leadOffset;
  const double planningStart=now();
   auto receiverHum=child(h,f.target,"Humanoid");double world=rd<float>(h,ws+Offsets::Workspace::ReadOnlyGravity,NAN),jump=rd<float>(h,receiverHum+Offsets::Humanoid::JumpHeight,NAN);
   if(rd<bool>(h,receiverHum+Offsets::Humanoid::UseJumpPower)){double jp=rd<float>(h,receiverHum+Offsets::Humanoid::JumpPower,NAN);jump=std::isfinite(jp)&&jp>=0&&jp<150&&world>10?jp*jp/(2*world):NAN;}
   // Unknown receiver jump capability cannot justify an elevated catch point.
   if(!std::isfinite(jump)||jump<0||jump>40)jump=0;
   if(!std::isfinite(world)||world<10||world>300){world=196.2;jump=0;}
   ff3qb::Defender receiver{measuredReceiverPosition,targetVelocity,20,jump,double(diveReach.load())};bool measuredReceiver=false,receiverIsPlayer=false;
   double walk=rd<float>(h,receiverHum+Offsets::Humanoid::Walkspeed,NAN);if(std::isfinite(walk)&&walk>=0&&walk<=45)receiver.speed=std::max(walk,std::hypot(targetVelocity.x,targetVelocity.z));
   for(const auto& actor:actors)if(actor.rig==f.target){receiverIsPlayer=!actor.npc;if(actor.shape.measured){receiver.radius=actor.shape.radius;receiver.above=actor.shape.above;receiver.below=actor.shape.below;receiver.worldGravity=world;measuredReceiver=true;}break;}
   // Let this receiver adjust after reaction time; every candidate and the
   // throw helper enforce that same receiver's measured catch envelope.
   // Practice bots follow their programmed route; they do not steer toward
   // an advertised catch spot. Only a human receiver can choose that redirect.
   receiver.redirect=smartFit&&measuredReceiver&&receiver.speed>0&&receiverIsPlayer;
   if(!measuredReceiver){receiver.radius=3;receiver.above=2;receiver.below=4;receiver.worldGravity=world;}
   f.receiverReach=receiver;
   f.receiverReachKnown=measuredReceiver;
  f.minimumAngle=automaticAngle&&autoPower?ff3qb::openAwayMinimum(origin,aimTarget,targetVelocity,defenders,f.coverageKnown):-30.;
  f.maximumAngle=!automaticAngle?85:f.minimumAngle>=5?50:force95?85:50;
  auto planned=powerFirst?ff3qb::Plan{}:ff3qb::planThrow(origin,aimTarget,targetVelocity,f.gravity,std::clamp(f.ping*.0005+.05,0.,.25),f.mode,automaticAngle,manualAngle,autoPower,f.gamePower,arc==1,smartFit,defenders,stableAngle,previousFit,powerFirst,5,f.groundY,f.maximumAngle,throwBounds(f),&receiver,f.minimumAngle);
  if(powerFirst){
   const bool searching=f.at>=nextPlanSearch;const double step=lastPlanAt?f.at-lastPlanAt:.1;
   const double latency=std::clamp(f.ping*.0005+.05,0.,.25);
   // Auto searches power, both arc roots and route-relative placements in
   // either mode. A contested loft must not overwrite a less exposed bullet.
   planned=ff3qb::planCoverageThrow(origin,aimTarget,targetVelocity,f.gravity,latency,defenders,smartFit,force95,jump,world,previousFit,previousPower,previousLob,previousAngle,searching,step,&receiver,f.leadOffset.y,&beamMemory,f.at,f.groundY,f.maximumAngle,throwBounds(f),f.minimumAngle);
   f.candidatesConsidered=beamMemory.candidatesConsidered;f.candidatesRanked=beamMemory.candidatesRanked;f.powersChecked=beamMemory.tiersVisited;f.rankedThrows=beamMemory.leaders;
   f.passLabel=force95?"Beam":planned.solution.angle>=35?"Lob":planned.solution.angle<=15?"Bullet":"Smart fit";lastPlanAt=f.at;
   if(f.at>=nextPlanSearch){lastSearch=f.at;nextPlanSearch=f.at+.10;}
  }
  if(!powerFirst&&automaticAngle&&autoPower&&planned.solution.valid){
   const double angle=std::min(force95?75.:50.,beamMemory.elevation.update(planned.solution.angle,f.at));
   if(std::abs(angle-planned.solution.angle)>.001){
    auto quiet=ff3qb::solveAngle(origin,aimTarget+planned.offset,targetVelocity,angle,f.gravity,std::clamp(f.ping*.0005+.05,0.,.25),5);
    double margin=quiet.valid?ff3qb::coverageMargin(quiet,f.gravity,defenders,std::clamp(f.ping*.0005+.05,0.,.25),nullptr,-INFINITY,f.groundY):-INFINITY;
    bool reachable=catchAllowed(f,quiet,receiver,std::clamp(f.ping*.0005+.05,0.,.25));
    if(reachable&&quiet.angle>=f.minimumAngle-1e-6&&(!smartFit||margin>=planned.margin-.25)){planned.solution=quiet;planned.power=ff3qb::length(quiet.velocity);planned.margin=margin;}
    else beamMemory.elevation.accept(planned.solution.angle,f.at);
   }
  }
  f.planningMs=(now()-planningStart)*1000;
  auto solution=planned.solution;
  f.deliveryMargin=ff3qb::deliveryClearance(planned);f.launchMargin=ff3qb::launchClearance(planned);
  // Very low bullets use their selected power/root at dispatch. The manual
  // fixed-angle solver intentionally starts at five degrees.
  f.fixedPowerRoute=powerFirst&&solution.valid&&solution.angle<5;
  if(!solution.valid||((smartFit||selectedHeight()!=0)&&measuredReceiver&&ff3qb::receiverGap(receiver,solution.catchPoint,solution.time+std::clamp(f.ping*.0005+.05,0.,.25))>-.25)){f.fitBlocked=smartFit;f.reason=f.practiceRange?"No reachable practice catch at these settings":"No reachable catch in bounds at these settings";return f;}
  if(!catchAllowed(f,solution,receiver,std::clamp(f.ping*.0005+.05,0.,.25))){f.reason=f.practiceRange?"No receiver-reachable practice catch":"No receiver-reachable catch in bounds";return f;}
  if(automaticAngle&&autoPower){auto high=ff3qb::solve(origin,aimTarget+planned.offset,targetVelocity,planned.power,f.gravity,std::clamp(f.ping*.0005+.05,0.,.25),true,5);planned.lob=high.valid&&std::abs(high.time-solution.time)<1e-5;}
  f.power=planned.power;f.fitOffset=planned.offset;f.targetPosition=aimTarget+planned.offset;f.margin=defenders.empty()?10:planned.margin;previousFit=planned.offset;previousPower=planned.power;previousLob=planned.lob;previousAngle=planned.solution.angle;f.selectedLob=planned.lob;f.distance=ff3qb::length(targetPosition-origin);
  f.solution=solution;f.risk=ff3qb::riskForecast(solution,f.gravity,f.hudDefenders,std::clamp(f.ping*.0005+.05,0.,.25),f.groundY);
  if(!defenders.empty())f.margin=std::min(planned.margin,ff3qb::coverageMargin(solution,f.gravity,defenders,std::clamp(f.ping*.0005+.05,0.,.25),&f.coverageTrace,-INFINITY,f.groundY));
  // A distant point minimizes camera/head parallax. Normal mouse hit testing
  // still belongs to the game, so geometry can affect the actual throw.
  f.aim=ff3qb::project(origin+solution.velocity*(1000./f.power),f.matrix,width,height);
  if(!packetMode&&!f.aim.valid){f.reason="Turn the camera toward the throw arc";return f;}
  if(!descendant(h,f.tool,f.own)||rd<addr>(h,player+Offsets::Player::ModelInstance)!=f.own||rd<std::uint64_t>(h,root+Offsets::DataModel::GameId)!=gameModules::registry[1].universe){f.reason="Game state changed";return f;}
  f.ready=true;f.reason=f.coverageKnown&&f.coverageCount>0&&f.margin<=ff3qb::catchSafetyMargin?"Ready: contested fit to "+f.name:f.fitOffset.y>=3?"Elevated catch: "+f.name:"Ready: click to throw to "+f.name;return f;
 }
};
inline bool eligible(const Frame& f,double time){return enabled&&f.revision==revision&&f.ready&&time>=f.at&&time-f.at<.10&&admitted()&&focused(f.pid)&&(packetMode||freeCursor());}
inline bool inGameClient(const Frame& f,POINT p){return p.x>=f.origin.x&&p.y>=f.origin.y&&p.x<f.origin.x+f.client.right&&p.y<f.origin.y+f.client.bottom;}
inline bool sameIdentity(const Frame& a,const Frame& b){return a.pid&&a.pid==b.pid&&a.model&&a.model==b.model&&a.own&&a.own==b.own&&a.tool&&a.tool==b.tool&&a.target&&a.target==b.target&&a.revision==b.revision;}
inline bool dispatchConsistent(const Frame& request,const Frame& aimed,const Frame& verify,POINT cursor){
 if(!sameIdentity(request,aimed)||!sameIdentity(aimed,verify)||!verify.ready||!aimed.ready||verify.power!=aimed.power||verify.gravity!=aimed.gravity||verify.window!=aimed.window||verify.origin.x!=aimed.origin.x||verify.origin.y!=aimed.origin.y||verify.client.right!=aimed.client.right||verify.client.bottom!=aimed.client.bottom)return false;
 const POINT expected{aimed.origin.x+LONG(std::lround(aimed.aim.x)),aimed.origin.y+LONG(std::lround(aimed.aim.y))};
 return verify.aim.valid&&aimed.aim.valid&&ff3qb::cameraStable(aimed.matrix,verify.matrix)&&std::hypot(verify.aim.x-aimed.aim.x,verify.aim.y-aimed.aim.y)<8&&std::hypot(double(cursor.x-expected.x),double(cursor.y-expected.y))<4&&std::hypot(verify.gameMouse.x-aimed.aim.x,verify.gameMouse.y-aimed.aim.y)<4;
}
inline LRESULT CALLBACK mouseProc(int n,WPARAM w,LPARAM l){
 if(n==HC_ACTION){auto* event=(MSLLHOOKSTRUCT*)l;const bool injected=(event->flags&LLMHF_INJECTED)!=0;
  if(inputObserver&&!injected){unsigned key=w==WM_LBUTTONDOWN||w==WM_LBUTTONUP?VK_LBUTTON:w==WM_RBUTTONDOWN||w==WM_RBUTTONUP?VK_RBUTTON:w==WM_MBUTTONDOWN||w==WM_MBUTTONUP?VK_MBUTTON:w==WM_XBUTTONDOWN||w==WM_XBUTTONUP?(HIWORD(event->mouseData)==XBUTTON1?VK_XBUTTON1:VK_XBUTTON2):0;if(key)inputObserver(key,w==WM_LBUTTONDOWN||w==WM_RBUTTONDOWN||w==WM_MBUTTONDOWN||w==WM_XBUTTONDOWN);}
  if(packetMode&&!injected&&(w==WM_LBUTTONDOWN||w==WM_LBUTTONUP)){
   Frame click;bool captured=false;const bool transport=packetInputReady&&packetInputReady();
   {std::lock_guard<std::mutex> lock(mutex);const double time=now();
    if(w==WM_LBUTTONDOWN&&enabled&&current.pid&&focused(current.pid)&&admitted()&&!current.menuOpen&&!current.textFocus){
     if(!physicalLeftHeld)physicalClickAt=GetTickCount64();physicalLeftHeld=true;physicalReleasedAt=0;
     heldTarget=lockedTarget.load()?lockedTarget.load():current.target;requested=current;requestedAt=time;click=requested;captured=true;traceInput("physical-down-passed",false,transport,current);
    }else if(w==WM_LBUTTONUP&&physicalLeftHeld){
     physicalLeftHeld=false;physicalReleasedAt=GetTickCount64();if(current.ready&&samePressSession(requested,current)){requested=current;requestedAt=time;}
     click=current;captured=true;traceInput("physical-up-passed",false,transport,current);
    }
   }if(captured&&frameSink)frameSink(click);
  }
  if(!packetMode&&!injected){std::lock_guard<std::mutex> lock(mutex);if(w==WM_LBUTTONUP&&clicks.up())return 1;if(w==WM_LBUTTONDOWN){const double distance=std::hypot(double(event->pt.x-current.cursor.x),double(event->pt.y-current.cursor.y));if(clicks.down(distance<40&&eligible(current,now()),now())){requested=current;return 1;}}}
 }return CallNextHookEx(mouse,n,w,l);
}
inline bool angleKeyOwned[4]={false,false,false,false};
inline bool hotkey(unsigned key,const Frame& f,bool repeat){
 if(key=='Q'){if(repeat)return true;if(lockedTarget)lockedTarget=0;else if(f.target)lockedTarget=f.target;else return false;++revision;return true;}
 if(key=='Z'){if(!repeat){passMode=(passMode.load()+1)%6;force95=false;automaticAngle=true;autoPower=true;++revision;}return true;}
 if(key=='R'||key=='F'){if(automaticAngle&&f.solution.valid)manualAngle=int(std::lround(f.solution.angle));automaticAngle=false;force95=false;autoPower=true;manualAngle=std::clamp(manualAngle.load()+(key=='R'?1:-1),5,85);++revision;return true;}
 return false;
}
inline LRESULT CALLBACK keyboardProc(int n,WPARAM w,LPARAM l){
 if(n==HC_ACTION&&inputObserver){auto* event=(KBDLLHOOKSTRUCT*)l;if(!(event->flags&LLKHF_INJECTED)&&(w==WM_KEYDOWN||w==WM_SYSKEYDOWN||w==WM_KEYUP||w==WM_SYSKEYUP))inputObserver(event->vkCode,w==WM_KEYDOWN||w==WM_SYSKEYDOWN);}
 if(n==HC_ACTION){auto* e=(KBDLLHOOKSTRUCT*)l;int index=e->vkCode=='R'?0:e->vkCode=='F'?1:e->vkCode=='Q'?2:e->vkCode=='Z'?3:-1;
  if(!(e->flags&LLKHF_INJECTED)&&(w==WM_KEYDOWN||w==WM_SYSKEYDOWN)&&(index>=0||e->vkCode==VK_ESCAPE)){std::lock_guard<std::mutex> lock(mutex);if(e->vkCode==VK_ESCAPE)cancelPress();}
  if(index>=0&&!(e->flags&LLKHF_INJECTED)){
   bool down=w==WM_KEYDOWN||w==WM_SYSKEYDOWN,up=w==WM_KEYUP||w==WM_SYSKEYUP;
   if(up&&angleKeyOwned[index]){angleKeyOwned[index]=false;return 1;}
   auto f=snapshot();
   if(down&&enabled&&(f.held||lockedTarget)&&!f.textFocus&&now()-f.at<.15&&focused(f.pid)&&admitted()&&!(GetAsyncKeyState(VK_CONTROL)&0x8000)&&!(GetAsyncKeyState(VK_MENU)&0x8000)){
    if(hotkey(unsigned(e->vkCode),f,angleKeyOwned[index])){angleKeyOwned[index]=true;return 1;}
   }
   if(down&&angleKeyOwned[index])return 1;
  }
 }return CallNextHookEx(keyboard,n,w,l);
}
inline bool install(){
 std::lock_guard<std::mutex> lifecycle(inputPumpMutex);if(inputPump.joinable())return inputPumpReady;
 HANDLE ready=CreateEventW(nullptr,TRUE,FALSE,nullptr);if(!ready)return false;
 try{inputPump=std::thread([ready]{
  MSG message{};PeekMessageW(&message,nullptr,WM_USER,WM_USER,PM_NOREMOVE);inputPumpId=GetCurrentThreadId();
  mouse=SetWindowsHookExW(WH_MOUSE_LL,mouseProc,GetModuleHandleW(nullptr),0);keyboard=SetWindowsHookExW(WH_KEYBOARD_LL,keyboardProc,GetModuleHandleW(nullptr),0);inputPumpReady=mouse&&keyboard;SetEvent(ready);
  if(inputPumpReady)while(GetMessageW(&message,nullptr,0,0)>0){TranslateMessage(&message);DispatchMessageW(&message);}
  if(mouse)UnhookWindowsHookEx(mouse);if(keyboard)UnhookWindowsHookEx(keyboard);mouse=keyboard=nullptr;inputPumpReady=false;inputPumpId=0;
 });}catch(...){CloseHandle(ready);return false;}
 const bool started=WaitForSingleObject(ready,5000)==WAIT_OBJECT_0&&inputPumpReady;
 if(!started){if(inputPumpId)PostThreadMessageW(inputPumpId,WM_QUIT,0,0);inputPump.join();}
 CloseHandle(ready);return started;
}
inline void uninstall(){
 {std::lock_guard<std::mutex> lifecycle(inputPumpMutex);if(inputPump.joinable()){if(inputPumpId)PostThreadMessageW(inputPumpId,WM_QUIT,0,0);inputPump.join();}}
 for(auto& owned:angleKeyOwned)owned=false;std::lock_guard<std::mutex> lock(mutex);clicks={};cancelPress();requested={};
}
inline void stop(bool stopTransport=true){enabled=false;lockedTarget=0;++revision;physicalClickAt=0;{std::lock_guard<std::mutex> lock(mutex);clicks.cancel();cancelPress();current={};current.reason="QB aim off";}if(stopTransport&&packetStop)packetStop();}
inline bool moveTo(POINT point){const int x=GetSystemMetrics(SM_XVIRTUALSCREEN),y=GetSystemMetrics(SM_YVIRTUALSCREEN),w=GetSystemMetrics(SM_CXVIRTUALSCREEN),h=GetSystemMetrics(SM_CYVIRTUALSCREEN);if(w<2||h<2)return false;INPUT in{};in.type=INPUT_MOUSE;in.mi.dwFlags=MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_VIRTUALDESK;in.mi.dx=LONG(std::lround((point.x-x)*65535./(w-1)));in.mi.dy=LONG(std::lround((point.y-y)*65535./(h-1)));return SendInput(1,&in,sizeof in)==1;}
inline void tapPower(){INPUT in[2]{};in[0].type=in[1].type=INPUT_KEYBOARD;in[0].ki.wVk=in[1].ki.wVk='R';in[1].ki.dwFlags=KEYEVENTF_KEYUP;if(SendInput(2,in,sizeof(INPUT))==1)SendInput(1,&in[1],sizeof(INPUT));}
inline void worker(){
 Reader reader;double powerAt=-10;
 while(running){
  flushInputTraces();expirePhysicalHold(GetTickCount64());
  if(studioRead){publish(studioRead());Sleep(8);continue;}
  if((!enabled&&!ballTrails&&!(extraVisuals&&extraVisuals()))||!admitted()){reader.flightTracker.clear();reader.throwTracker.clear();Frame idle;idle.reason=enabled?"Connect to Football Fusion 3":"QB aim off";publish(idle);{std::lock_guard<std::mutex> lock(mutex);clicks.cancel();cancelPress();}Sleep(30);continue;}
  auto f=reader.read();
  if(!enabled){publish(f);Sleep(16);continue;}
  if(packetMode){
   // Dispatch retains its own click snapshot. Re-reading an already released
   // tool here used to erase assistance before the outgoing call arrived.
   publish(f);Sleep(8);continue;
  }
  publish(f);
  Frame request;bool send=false;
  {std::lock_guard<std::mutex> lock(mutex);if(clicks.pending){request=requested;send=clicks.take(now(),eligible(requested,now()));}}
  if(send){
   record(request,"Click intercepted");
   bool completed=false;const auto requestRevision=request.revision;
   auto fresh=reader.read(request.target);
   if(sameIdentity(request,fresh)&&eligible(fresh,now())){
    POINT point{fresh.origin.x+LONG(std::lround(fresh.aim.x)),fresh.origin.y+LONG(std::lround(fresh.aim.y))};
    if(moveTo(point)){
     // Wait for the game's own input object to acknowledge the new point,
     // then allow another render interval. Windows cursor position alone is
     // insufficient; Mouse.Hit may still describe the previous frame.
     record(fresh,"Cursor moved");const double movedAt=now();double acknowledged=-1;Frame verify;bool settled=false;
     while(running&&now()-movedAt<.15){
      Sleep(8);verify=reader.read(request.target);POINT cursor{};GetCursorPos(&cursor);
      if(!eligible(verify,now())||revision!=requestRevision||!sameIdentity(request,verify)||!ff3qb::cameraStable(fresh.matrix,verify.matrix))break;
      if(dispatchConsistent(request,fresh,verify,cursor)){if(acknowledged<0)acknowledged=now();if(now()-acknowledged>=.035){settled=true;break;}}
      else acknowledged=-1;
     }
     if(settled&&eligible(verify,now())&&revision==requestRevision){
      INPUT input[2]{};input[0].type=input[1].type=INPUT_MOUSE;input[0].mi.dwFlags=MOUSEEVENTF_LEFTDOWN;input[1].mi.dwFlags=MOUSEEVENTF_LEFTUP;
      const UINT sent=SendInput(2,input,sizeof(INPUT));if(sent==1){INPUT up{};up.type=INPUT_MOUSE;up.mi.dwFlags=MOUSEEVENTF_LEFTUP;SendInput(1,&up,sizeof up);}
      completed=true;record(verify,sent==2?"Throw input sent":"Throw input failed",sent==2);
      f.reason=sent==2?"Throw sent to "+fresh.name:"Throw input failed";f.ready=false;publish(f);Sleep(90);
     }
    }
   }
   if(!completed)record(fresh,"Throw cancelled: camera or game aim did not settle");
  }else if(autoPower&&f.target&&f.held&&!f.textFocus&&f.power>0&&f.power<95&&f.revision==revision&&enabled&&admitted()&&focused(f.pid)&&!(GetAsyncKeyState('R')&0x8000)&&!(GetAsyncKeyState('F')&0x8000)&&now()-powerAt>.10){tapPower();powerAt=now();}
  Sleep(8);
 }
}
inline json::Value modules(){using namespace basketballUi;auto out=object(),m=object(),rows=object();m.o[L"on"]=basketballUi::boolean(enabled);rows.o[L"arc"]=number(arc);rows.o[L"preset"]=number(passMode);rows.o[L"angle"]=number(manualAngle);rows.o[L"radius"]=number(selectionRadius);rows.o[L"dive"]=number(diveReach);rows.o[L"lead"]=number(selectedLead());rows.o[L"height"]=number(selectedHeight());rows.o[L"beamLead"]=number(beamLeadDistance);rows.o[L"beamHeight"]=number(beamCatchHeight);rows.o[L"autoangle"]=basketballUi::boolean(automaticAngle);rows.o[L"fit"]=basketballUi::boolean(smartFit);rows.o[L"hud"]=basketballUi::boolean(showHud);rows.o[L"candidates"]=basketballUi::boolean(showCandidates);rows.o[L"power"]=basketballUi::boolean(autoPower);rows.o[L"force95"]=basketballUi::boolean(force95);rows.o[L"trails"]=basketballUi::boolean(ballTrails);rows.o[L"practice"]=basketballUi::boolean(practice);rows.o[L"visuals"]=basketballUi::boolean(visuals);m.o[L"rows"]=rows;out.o[L"ff3_qb"]=m;return out;}
inline bool set(const std::wstring& key,const json::Value& v){
 if(key==L"ff3_qb.arc"){if(v.type!=json::Value::Number||(v.n!=0&&v.n!=1))return false;arc=int(v.n);++revision;return true;}
 if(key==L"ff3_qb.preset"||key==L"ff3_qb.angle"||key==L"ff3_qb.radius"||key==L"ff3_qb.dive"||key==L"ff3_qb.lead"||key==L"ff3_qb.height"||key==L"ff3_qb.beamLead"||key==L"ff3_qb.beamHeight"){
  if(v.type!=json::Value::Number||!std::isfinite(v.n)||std::floor(v.n)!=v.n)return false;
  if(key==L"ff3_qb.preset"){if(v.n<0||v.n>5)return false;passMode=int(v.n);force95=false;automaticAngle=true;autoPower=true;}
  else if(key==L"ff3_qb.angle"){if(v.n<5||v.n>85)return false;manualAngle=int(v.n);}
  else if(key==L"ff3_qb.height"||key==L"ff3_qb.beamHeight"){if(v.n< -3||v.n>8)return false;if(force95||key==L"ff3_qb.beamHeight")beamCatchHeight=int(v.n);else catchHeightAdjustment=int(v.n);}
  else if(key==L"ff3_qb.lead"||key==L"ff3_qb.beamLead"){if(v.n< -12||v.n>12)return false;if(force95||key==L"ff3_qb.beamLead")beamLeadDistance=int(v.n);else leadDistance=int(v.n);}
  else if(key==L"ff3_qb.dive"){if(v.n<0||v.n>24)return false;diveReach=int(v.n);}
  else{if(v.n<10||v.n>700)return false;selectionRadius=int(v.n);}++revision;return true;
 }
 if(v.type!=json::Value::Bool)return false;
 if(key==L"ff3_qb"){enabled=v.b;++revision;if(!v.b)stop();else{std::lock_guard<std::mutex> lock(mutex);current={};current.reason="Waiting for game data";}return true;}
 if(key==L"ff3_qb.force95"){force95=v.b;if(v.b){passMode=0;automaticAngle=true;autoPower=true;}}else if(key==L"ff3_qb.autoangle"){automaticAngle=v.b;if(!v.b)force95=false;}else if(key==L"ff3_qb.fit")smartFit=v.b;else if(key==L"ff3_qb.hud")showHud=v.b;else if(key==L"ff3_qb.candidates")showCandidates=v.b;else if(key==L"ff3_qb.power"){autoPower=v.b;if(!v.b)force95=false;}else if(key==L"ff3_qb.practice")practice=v.b;else if(key==L"ff3_qb.visuals")visuals=v.b;else if(key==L"ff3_qb.trails")ballTrails=v.b;else return false;++revision;return true;
}
inline json::Value report(){using namespace basketballUi;auto f=snapshot();auto out=object();out.o[L"type"]=string(L"ff3Live");out.o[L"modules"]=modules();out.o[L"reason"]=string(widen(f.reason));if(enabled&&packetMode&&packetStatus){const auto state=packetStatus();out.o[L"setup"]=string(state.second);out.o[L"transportReady"]=basketballUi::boolean(state.first);out.o[L"setupFailed"]=basketballUi::boolean(packetRetryable&&packetRetryable());}out.o[L"target"]=string(widen(f.name));out.o[L"power"]=number(f.power);out.o[L"gamePower"]=number(f.gamePower);out.o[L"automaticPower"]=basketballUi::boolean(autoPower);out.o[L"held"]=basketballUi::boolean(f.held);out.o[L"wheel"]=basketballUi::boolean(f.wheelVisible);out.o[L"ready"]=basketballUi::boolean(f.ready);out.o[L"fitBlocked"]=basketballUi::boolean(f.fitBlocked);out.o[L"route"]=string(widen(f.route));out.o[L"preset"]=string(force95?L"Beam":widen(f.passLabel.empty()?ff3qb::presetName(f.mode):f.passLabel));out.o[L"distance"]=number(f.distance);out.o[L"locked"]=basketballUi::boolean(f.locked);out.o[L"coverageCount"]=number(f.coverageCount);out.o[L"coverageDetail"]=string(widen(f.coverageDetail));out.o[L"coverageKnown"]=basketballUi::boolean(f.coverageKnown);out.o[L"margin"]=number(f.margin);out.o[L"fitOffset"]=number(std::hypot(f.fitOffset.x,f.fitOffset.z));out.o[L"catchHeight"]=number(f.fitOffset.y+f.leadOffset.y);out.o[L"lead"]=number(selectedLead());out.o[L"force95"]=basketballUi::boolean(force95);out.o[L"balls"]=number(double(f.balls.size()));out.o[L"angle"]=number(f.solution.angle);out.o[L"flight"]=number(f.solution.time);auto profile=array();if(f.ready){for(int i=0;i<=24;i++){auto p=ff3qb::point(f.solution,f.gravity,f.solution.time*i/24.);auto sample=array();sample.a.push_back(number(std::hypot(p.x-f.solution.origin.x,p.z-f.solution.origin.z)));sample.a.push_back(number(p.y-f.solution.origin.y));profile.a.push_back(sample);}}out.o[L"check"]=string(ff3qb::throwCheckLabel(ff3qb::throwCheck(f.hudCoverageKnown,f.hudReachEstimated,f.risk)));out.o[L"fieldKnown"]=basketballUi::boolean(f.field.valid());out.o[L"trajectory"]=profile;{std::lock_guard<std::mutex> lock(mutex);out.o[L"lastAction"]=string(now()-actionAt<3?widen(lastAction):L"");out.o[L"throws"]=number(throwCount);}return out;}
inline void load(const std::filesystem::path& file){auto cfg=basketballUi::readConfig(file);force95=false;for(auto key:{L"arc",L"preset",L"power",L"practice",L"visuals",L"angle",L"radius",L"dive",L"autoangle",L"fit",L"hud",L"candidates",L"trails",L"lead",L"height",L"beamLead",L"beamHeight",L"force95"})if(cfg[key].type!=json::Value::Null)set(L"ff3_qb."+std::wstring(key),cfg[key]);enabled=false;}
inline bool save(const std::filesystem::path& file){auto data=modules()[L"ff3_qb"][L"rows"];data.o[L"lead"]=basketballUi::number(leadDistance);data.o[L"height"]=basketballUi::number(catchHeightAdjustment);auto temp=file;temp+=L".tmp";{std::ofstream out(temp,std::ios::binary);out<<winrt::to_string(json::dump(data));out.flush();if(!out)return false;}return MoveFileExW(temp.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;}
}
