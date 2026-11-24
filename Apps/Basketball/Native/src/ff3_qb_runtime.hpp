#pragma once
#include "ff3_qb_logic.hpp"
#include "ff3_qb_timing.hpp"
#include "ff3_mouse_aim.hpp"
#include "ff3_camera_pan.hpp"
#include "ff3_ball_discovery.hpp"
#include "ff3_input_policy.hpp"
#include "ff3_throw_state.hpp"
#include "basketball_ui_bridge.hpp"
#include "ff3_direct_aim_memory.hpp"
#include "ff3_camera_preparation.hpp"
#include "ff3_cursor_aim.hpp"
#include "ff3_click_flick.hpp"
#include <dwmapi.h>
#pragma comment(lib,"dwmapi.lib")
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
inline std::atomic<bool> showReceivers{true},showRoutes{false},showWindow{false},showRace{false},showThreats{true},landingMarkers{true},futureMarkers{false},ballHighlights{true};
inline std::atomic<unsigned> revision{0};
inline std::atomic<addr> lockedTarget{0};
inline std::atomic<int> diveReach{12},leadDistance{5},catchHeightAdjustment{0},beamLeadDistance{3},beamCatchHeight{0};
inline std::atomic<bool> force95{false},ballTrails{true},looseMarkers{true};
inline std::atomic<int> boundaryHalfStuds{4};
inline std::atomic<bool> oobVisual{true};
inline std::atomic<addr> heldTarget{0};inline std::atomic<bool> physicalLeftHeld{false};inline std::atomic<ULONGLONG> physicalReleasedAt{0};
inline int selectedLead(){return force95?beamLeadDistance.load():leadDistance.load();}
inline int selectedHeight(){return force95?beamCatchHeight.load():catchHeightAdjustment.load();}
inline std::filesystem::path logPath;
inline HHOOK mouse=nullptr;
inline HHOOK keyboard=nullptr;
inline std::thread inputPump;inline std::mutex inputPumpMutex;inline std::atomic<DWORD> inputPumpId{0};inline std::atomic<bool> inputPumpReady{false};
inline std::atomic<bool> packetMode{false},visualOnly{false},cursorOnly{false};
inline int selectedMethod(){return cursorOnly?3:visualOnly?2:packetMode?1:0;}
inline bool wheelMethod(){return !packetMode&&!visualOnly;}
inline bool calculatesPower(){return !visualOnly&&(autoPower||force95);}
inline bool editsPacketPower(){return packetMode&&calculatesPower();}
inline std::atomic<bool> inputBusy{false},inputCancelled{false};
inline std::atomic<bool> flickActive{false};
inline std::atomic<bool> flickDispatched{false};
inline std::atomic<double> animationDelay{.35};
inline std::atomic<bool> cameraPanOwned{false},physicalRightHeld{false};
inline std::atomic<int> aimMovementMs{70};
// These values are managed by the app, rather than exposed as user controls.
inline void managedDefaults(){autoPower=true;automaticAngle=true;practice=true;arc=0;selectionRadius=120;aimMovementMs=70;diveReach=12;}
inline std::atomic<ULONGLONG> physicalClickAt{0};
inline std::atomic<double> renderHz{0},renderMs{0};
struct Frame {
 bool studio=false,practiceRange=false,inputThrow=false,committed=false,arrivalFirst=false;
 double preparationPower=0;
 std::vector<ff3Input::Button> powerButtons;addr powerGui=0;
 bool menuOpen=true;
 std::shared_ptr<Frame> releaseVisual;bool afterRelease=false;
 unsigned revision=0;
 double at=0,gravity=0,power=0,gamePower=0,ping=0,margin=0,distance=0,planningMs=0,boundaryMargin=2,replicationLead=0,previewDelay=0,timingTolerance=.025;DWORD pid=0;addr model=0,own=0,tool=0,target=0;
 addr localPlayer=0,team=0,heldBall=0,headPrimitive=0;ff3qb::ThrowView throwView;std::vector<ff3qb::Defender> hudDefenders;std::vector<bool> hudShapeMeasured;bool hudReachEstimated=false;bool hudCoverageKnown=false,fixedPowerRoute=false;double appliedLead=0;
 HWND window=nullptr;RECT client{};POINT origin{},cursor{};float matrix[16]{};
 std::vector<ff3qb::BallFlight> balls;std::vector<ff3qb::BallObservation> looseBalls;size_t discoveredBalls=0;bool discoveryOverflow=false;
 ff3qb::Solution solution;ff3qb::Screen aim,receiver;
 addr cameraMatrix=0,camera=0,workspace=0,mouseService=0,mouseInput=0;ff3qb::AimRotation cameraRotation;Vec cameraPosition{NAN,NAN,NAN},gameMouse{NAN,NAN,NAN};
 Vec head{},targetPosition{},targetVelocity{},leadOffset{},selectedPosition{};addr selectedPrimitive=0;double groundY=NAN;
 ff3qb::FieldBounds field;double fieldSurfaceY=NAN;
 ff3qb::CoverageTrace coverageTrace;ff3qb::RiskForecast risk;
 ff3qb::Defender receiverReach;bool receiverReachKnown=false,wobbleFiltered=false;
 Vec rawTargetVelocity{},routeCorrection{};double deliveryMargin=0,launchMargin=0,minimumAngle=-30,maximumAngle=85;
 unsigned candidatesConsidered=0,candidatesRanked=0;size_t powersChecked=0;std::vector<ff3qb::Plan> rankedThrows;
 std::vector<ff3qb::Defender> defenders;bool guarded=false,fitBlocked=false;Vec fitOffset{};bool coverageKnown=false,wheelVisible=false,selectedLob=false;
 int mode=0,coverageCount=0;bool locked=false;std::string route,coverageDetail,passLabel;
 struct Candidate{ff3qb::Screen screen;std::string name;bool selected=false;};std::vector<Candidate> candidates;
 struct ReceiverVisual{addr rig=0;Vec position,velocity;std::string name;double space=INFINITY;bool known=false;};std::vector<ReceiverVisual> receiverVisuals;
 std::string name,reason="Enable QB aim to begin";bool held=false,ready=false,textFocus=false,targetGrace=false;
};
// One solve policy for preview and dispatch. Automatic plans already selected
// a power/root; pinning their angle again can demand >95 after tiny movement.
inline bool displayedPowerPlan(const Frame& f){return f.inputThrow||visualOnly;}
inline bool anglePowerPlan(const Frame& f){return !displayedPowerPlan(f)&&autoPower&&!force95&&!f.fixedPowerRoute;}
inline bool isPracticeRange(std::uint64_t place){return place==81310542478972ULL;}
inline bool fieldAvailable(const Frame& f){return f.practiceRange||f.field.valid();}
inline const ff3qb::FieldBounds* throwBounds(const Frame& f){return f.practiceRange?nullptr:&f.field;}
inline bool catchAllowed(const Frame& f,const ff3qb::Solution& s,const ff3qb::Defender& receiver,double latency){
 if(!f.practiceRange)return ff3qb::inBoundsCatch(s,receiver,latency,f.field,f.boundaryMargin);
 return s.valid&&ff3qb::finite(s.catchPoint)&&std::isfinite(s.time)&&s.time>0&&std::isfinite(latency)&&latency>=0&&latency<=.3&&ff3qb::receiverGap(receiver,s.catchPoint,s.time+latency)<=-.25;
}
inline bool pinPhysicalPower(Frame& frame,double power){
 if(!std::isfinite(power)||power<1||power>95)return false;
 const double latency=std::clamp(frame.ping*.0005+.05,0.,.25);
 ff3qb::Solution best;bool highRoot=false;double cost=INFINITY;
 for(bool high:{false,true}){
  auto candidate=ff3qb::solve(frame.head,frame.targetPosition,frame.targetVelocity,power,frame.gravity,latency,high,5);
  if(!candidate.valid||((frame.practiceRange||frame.field.known)&&!catchAllowed(frame,candidate,frame.receiverReach,latency)))continue;
  const double score=frame.solution.valid?std::abs(candidate.angle-frame.solution.angle):(high?100:0);
  if(score<cost){cost=score;best=candidate;highRoot=high;}
 }
 if(!best.valid)return false;frame.power=frame.gamePower=power;frame.solution=best;frame.selectedLob=highRoot;frame.ready=true;if(automaticAngle&&frame.mode==0){frame.minimumAngle=-30;frame.maximumAngle=85;}return true;
}
inline bool keepWheelPlan(const ff3qb::Plan& current,const ff3qb::Plan& proposed,double chosenAt,double time,bool fastest=false){
 if(!current.solution.valid||!std::isfinite(time)||time<chosenAt)return false;
 if(!proposed.solution.valid)return true;
 if(fastest&&(ff3qb::estimatedClear(proposed,true)&&!ff3qb::estimatedClear(current,true)||current.solution.time>proposed.solution.time+.025))return false;
 if(current.margin<proposed.margin-.75)return false;
 return time-chosenAt<.2||(current.lob==proposed.lob&&std::abs(current.power-proposed.power)<=5&&ff3qb::length(current.offset-proposed.offset)<=1);
}
inline ff3qb::Solution predictedThrow(const Frame& f,double time){
 if(f.studio||f.committed)return f.solution;
 const double age=std::clamp(time-f.at,0.,.45),latency=std::clamp(f.ping*.0005+.05,0.,.25);
 auto target=f.targetPosition+f.targetVelocity*age;
 const bool anglePower=anglePowerPlan(f);
 auto solved=anglePower?ff3qb::solveAngle(f.head,target,f.targetVelocity,f.solution.angle,f.gravity,latency,5):ff3qb::solve(f.head,target,f.targetVelocity,f.power,f.gravity,latency,automaticAngle&&(calculatesPower()||visualOnly)?f.selectedLob:(arc==1||f.mode==2),5);
 // A retained Beam tier can fall just outside range as the receiver/QB moves.
 // Keep its elevation and recalculate power instead of losing the throw.
 if(!displayedPowerPlan(f)&&!anglePower&&!solved.valid&&autoPower&&f.solution.angle>=5&&f.solution.angle<=85)solved=ff3qb::solveAngle(f.head,target,f.targetVelocity,f.solution.angle,f.gravity,latency,5);
 if(anglePower&&!solved.valid&&automaticAngle){
  auto low=ff3qb::solve(f.head,target,f.targetVelocity,95,f.gravity,latency,false,5),high=ff3qb::solve(f.head,target,f.targetVelocity,95,f.gravity,latency,true,5);
  for(const auto& candidate:{low,high})if(candidate.valid&&candidate.angle<=50&&(!solved.valid||std::abs(candidate.angle-f.solution.angle)<std::abs(solved.angle-f.solution.angle)))solved=candidate;
 }
 if(!displayedPowerPlan(f)&&automaticAngle&&!force95&&solved.valid&&solved.angle>50)solved=ff3qb::solveAngle(f.head,target,f.targetVelocity,50,f.gravity,latency,5);
 if(!displayedPowerPlan(f)&&solved.valid&&solved.angle>f.maximumAngle+1e-6)solved=ff3qb::solveAngle(f.head,target,f.targetVelocity,f.maximumAngle,f.gravity,latency,5);
 if(!displayedPowerPlan(f)&&solved.valid&&solved.angle<f.minimumAngle-1e-6)solved=ff3qb::solveAngle(f.head,target,f.targetVelocity,f.minimumAngle,f.gravity,latency,5);
 if(displayedPowerPlan(f)&&solved.valid&&(solved.angle<f.minimumAngle-1e-6||solved.angle>f.maximumAngle+1e-6))return {};
 auto receiver=f.receiverReach;receiver.position=receiver.position+Vec{receiver.velocity.x,0,receiver.velocity.z}*age;
 if((f.practiceRange||f.field.known)&&!catchAllowed(f,solved,receiver,latency))return {};
 return solved;
}
inline std::mutex mutex;inline Frame current,requested;inline double requestedAt=0;inline ff3qb::ClickGate clicks,blockedPacketClick;
inline ff3qb::ClickGate flickClicks;inline Frame flickRequest;
inline std::vector<std::pair<Frame,std::string>> pendingPacketCancellations;
inline ff3qb::CameraPreparation cameraPreparation;inline std::atomic<LONG> cameraPixelX{0},cameraPixelY{0};
inline std::vector<std::pair<Frame,bool>> pendingCameraShots;
inline std::function<bool(const Frame&)> cursorClickStart;
inline ff3qb::PointerMovement physicalMouseMovement;
inline bool powerWheelHit(const Frame& f,POINT point){
 if(!f.wheelVisible)return false;
 const ff3Input::Point local{double(point.x-f.origin.x),double(point.y-f.origin.y)};
 return std::any_of(f.powerButtons.begin(),f.powerButtons.end(),[&](const auto& button){return ff3Input::contains(button,local);});
}
inline bool cameraShotEligible(const Frame& f,double time,unsigned version){
 return f.ready&&f.held&&f.target&&f.pid&&f.model&&f.own&&f.tool&&f.inputThrow&&f.solution.valid&&f.power>=1&&f.power<=95&&f.power==f.gamePower&&!f.menuOpen&&!f.textFocus&&f.revision==version&&time>=f.at&&time-f.at<.1;
}
inline bool commitCameraShot(const Frame& live,double time,unsigned version,POINT cursor,Frame& shot){
 if(!cameraShotEligible(live,time,version))return false;
 const auto solution=predictedThrow(live,time);if(!solution.valid)return false;
 shot=live;shot.solution=solution;shot.cursor=cursor;shot.committed=true;shot.at=time;return true;
}
inline std::string packetPressIssue(const Frame& f,bool transport,double time,unsigned version){
 if(!transport)return "Packet Aim is preparing";
 if(f.revision!=version||!std::isfinite(time)||time<f.at||time-f.at>=.1)return "Aim sample is stale";
 if(!f.ready||!f.target||!f.solution.valid)return f.reason.empty()?"No assisted trajectory ready":f.reason;
 return {};
}
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
inline json::Value throwHistory=basketballUi::array();inline unsigned historySequence=0;
inline void record(const Frame& f,const char* action,bool sent=false){
 if(wheelMethod()&&inputBusy&&inputCancelled&&!flickActive&&std::string(action).rfind("Throw cancelled:",0)==0)return;
 {std::lock_guard<std::mutex> lock(mutex);lastAction=action;actionAt=now();if(sent)++throwCount;
  const bool cancelled=lastAction.rfind("Throw cancelled:",0)==0;
  const bool manual=lastAction=="Manual throw before Camera Aim was ready";
  if(cancelled||manual||lastAction=="Throw input failed"||(!packetMode&&lastAction=="Throw input sent")){
   auto item=basketballUi::object();item.o[L"sequence"]=basketballUi::number(++historySequence);item.o[L"at"]=basketballUi::number(double(GetTickCount64()));item.o[L"method"]=basketballUi::number(selectedMethod());
   item.o[L"outcome"]=basketballUi::string(manual?L"normal":cancelled||!sent?L"cancelled":L"sent");item.o[L"reason"]=basketballUi::string(basketballUi::widen(cancelled?lastAction.substr(17):lastAction));item.o[L"power"]=basketballUi::number(f.power);
   throwHistory.a.push_back(item);if(throwHistory.a.size()>8)throwHistory.a.erase(throwHistory.a.begin());
  }
 }
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
inline Frame committedThrowView(const Frame& shot,const Frame& live){
 if(!shot.committed||!samePressSession(shot,live)||live.menuOpen||live.textFocus)return live;
 auto shown=shot;shown.balls=live.balls;shown.looseBalls=live.looseBalls;shown.discoveredBalls=live.discoveredBalls;shown.throwView=live.throwView;shown.releaseVisual=live.releaseVisual;shown.reason="Throw locked; waiting for actual ball release";return shown;
}
inline void publish(Frame f){
 {std::lock_guard<std::mutex> lock(mutex);f=bridgeTargetGap(current,f,now());
  if(!packetMode&&(flickClicks.pending||flickActive))f=committedThrowView(flickRequest,f);
  if(physicalLeftHeld&&f.pid&&f.model&&f.own&&!samePressSession(requested,f))cancelPress();
  if(packetMode&&physicalLeftHeld&&f.ready&&!f.menuOpen&&!f.textFocus&&samePressSession(requested,f)){requested=f;requestedAt=now();if(!heldTarget)heldTarget=f.target;}
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
// Locate the game viewport by its attached PID, never by whichever app
// currently owns keyboard focus. Enumerate only visible top-level windows.
inline HWND clientWindow(DWORD pid){
 struct Search{DWORD pid;HWND window=nullptr;long long area=0;} search{pid};
 if(!pid)return nullptr;
 EnumWindows([](HWND window,LPARAM context)->BOOL{auto& s=*reinterpret_cast<Search*>(context);DWORD owner=0;GetWindowThreadProcessId(window,&owner);RECT rect{};
  if(owner==s.pid&&IsWindowVisible(window)&&!GetWindow(window,GW_OWNER)&&GetClientRect(window,&rect)){const auto area=static_cast<long long>(rect.right)*rect.bottom;if(area>s.area){s.window=window;s.area=area;}}return TRUE;},reinterpret_cast<LPARAM>(&search));
 DWORD owner=0;if(search.window)GetWindowThreadProcessId(search.window,&owner);return owner==pid?search.window:nullptr;
}
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
inline void flushPacketCancellations(){
 std::vector<std::pair<Frame,std::string>> events;{std::lock_guard<std::mutex> lock(mutex);events.swap(pendingPacketCancellations);}
 for(const auto& event:events)record(event.first,("Throw cancelled: "+event.second).c_str());
}
inline void flushCameraShots(){
 std::vector<std::pair<Frame,bool>> shots;{std::lock_guard<std::mutex> lock(mutex);shots.swap(pendingCameraShots);}
 for(const auto& shot:shots)record(shot.first,shot.second?"Throw input sent":"Manual throw before Camera Aim was ready",shot.second);
}
// Retain the plan owned by a physical click through the game's unequip/release.
// It must have been fresh when clicked, and cannot survive a session or setting change.
inline bool clickPlanValid(const Frame& request,const Frame& live,double captured,double time,ULONGLONG clicked,ULONGLONG tick,unsigned version,ULONGLONG released=0,bool held=false){
 return ff3qb::pressAuthorized(clicked,released,held,tick)&&request.ready&&!request.textFocus&&request.revision==version&&request.pid&&request.pid==live.pid&&request.model==live.model&&request.own&&request.own==live.own&&captured>=request.at&&captured-request.at<.1&&time>=captured&&time-captured<=.35;
}
struct DispatchFrame {Frame frame;ULONGLONG clicked=0;bool retained=false,held=false;ULONGLONG released=0;};
inline DispatchFrame packetFrame(const Frame& live){std::lock_guard<std::mutex> lock(mutex);const auto clicked=physicalClickAt.load(),released=physicalReleasedAt.load();const bool held=physicalLeftHeld.load();const bool retained=clickPlanValid(requested,live,requestedAt,now(),clicked,GetTickCount64(),revision.load(),released,held);return {retained?requested:live,clicked,retained,held,released};}
inline bool freeCursor(){CURSORINFO info{sizeof info};RECT clip{};return GetCursorInfo(&info)&&GetClipCursor(&clip)&&ff3qb::cursorModeAllowed((info.flags&CURSOR_SHOWING)!=0,(GetAsyncKeyState(VK_RBUTTON)&0x8000)!=0,false,clip.right-clip.left<=4||clip.bottom-clip.top<=4);}
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
 addr workspace=0,ground=0,lines=0;std::vector<addr> stripes;double scanned=-10,surfaceY=NAN;
 ff3qb::FieldBounds read(HANDLE h,addr ws,double time){
  surfaceY=NAN;if(workspace!=ws||time<scanned||time-scanned>.5){
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
  const auto field=ff3qb::fieldFromLines(position(h,ground),right,forward,measured);
  if(field.valid()){surfaceY=0;for(const auto& stripe:measured)surfaceY+=(stripe.position.y+stripe.size.y*.5)/measured.size();}
  return field;
 }
};
inline bool looseWorldBall(HANDLE h,addr part,addr ws){
 if(!part||!ws)return false;addr node=rd<addr>(h,part+Offsets::Instance::Parent);for(int depth=0;node&&depth<18;++depth){if(node==ws)return true;const auto type=defense::classOf(h,node);if(type=="Backpack"||type=="Player"||type=="Model"&&child(h,node,"Humanoid"))return false;node=rd<addr>(h,node+Offsets::Instance::Parent);}return false;
}
struct Reader {
 FieldReader fieldReader;
 double groundY=NAN;addr groundWorkspace=0;
 ff3qb::FlightTracker flightTracker;std::vector<addr> flightParts;addr flightWorkspace=0;double flightScanAt=-10;ff3qb::BallDiscovery ballCatalog;
 void collectBalls(HANDLE h,addr ws,Frame& f,bool required=false){
  if(!required&&!ballTrails&&!(enabled&&(showHud||visuals))){flightTracker.clear();flightParts.clear();return;}
  if(flightWorkspace!=ws){flightTracker.clear();flightParts.clear();ballCatalog.reset(ws);flightWorkspace=ws;}
  ballCatalog.poll(ws,now(),[&](std::uint64_t node){return children(h,node);},[&](std::uint64_t node){
   const auto cls=defense::classOf(h,node),name=nm(h,node);if(defense::partClass(cls)&&(name=="Football"||name=="Handle"&&path(h,node,{"Configuration","Gravity"})))return ff3qb::DiscoveryInfo{ff3qb::DiscoveryKind::Ball,false};
   if((cls=="Folder"||cls=="Model"||cls=="WorldModel"||cls=="Tool")&&(cls!="Model"||!child(h,node,"Humanoid"))){const auto lower=defense::lower(name);return ff3qb::DiscoveryInfo{ff3qb::DiscoveryKind::Container,lower.find("ball")!=std::string::npos||lower.find("projectile")!=std::string::npos||lower.find("dropped")!=std::string::npos};}return ff3qb::DiscoveryInfo{};
  });
  flightParts.assign(ballCatalog.balls.begin(),ballCatalog.balls.end());std::vector<ff3qb::BallObservation> observations;
  for(auto part:flightParts){if(!looseWorldBall(h,part,ws)){ballCatalog.forget(part);continue;}
   auto primitive=rd<addr>(h,part+Offsets::BasePart::Primitive);auto p=position(h,part),v=velocity(h,part);if(!primitive||!ff3qb::finite(p)){ballCatalog.forget(part);continue;}if(!ff3qb::finite(v))v={};
   const auto gravityValue=path(h,part,{"Configuration","Gravity"});double gravity=-rd<double>(h,gravityValue+Offsets::Misc::Value,NAN);if(!std::isfinite(gravity)||gravity<1||gravity>300)gravity=0;
   auto by=child(h,part,"ThrownBy");auto thrower=by&&defense::classOf(h,by)=="ObjectValue"?rd<addr>(h,by+Offsets::Misc::Value):0;const auto flags=rd<unsigned char>(h,primitive+Offsets::Primitive::Flags,255);if(flags==255)continue;
   observations.push_back({part,p,v,gravity,(flags&Offsets::PrimitiveFlags::Anchored)!=0,thrower,gravityValue!=0,std::clamp(partSize(h,part).y*.5,.2,25.)});ballCatalog.watch(rd<addr>(h,part+Offsets::Instance::Parent));
  }
  f.looseBalls=observations;f.discoveredBalls=observations.size();f.discoveryOverflow=ballCatalog.overflow;
  // The trail tracker is bounded independently of the complete loose-ball list.
  if(observations.size()>64){const auto center=f.head;std::stable_sort(observations.begin(),observations.end(),[&](const auto& a,const auto& b){return ff3qb::length(a.position-center)<ff3qb::length(b.position-center);});observations.resize(64);}
  f.at=now();flightTracker.update(observations,f.at,f.groundY);f.balls=flightTracker.visible();

 }

 HANDLE handle=nullptr;DWORD pid=0;addr model=0,moduleBase=0,retained=0;double scanAt=-10;
 addr routeTarget=0,routeTool=0;ff3qb::SelectionLatch selection;ff3qb::SelectionRetention selectionRetention;ff3qb::RouteHistory routeHistory;ff3qb::AngleTracker angleHistory;ff3qb::BeamMemory beamMemory;ff3qb::RoutePassLatch routePass;Vec previousFit{};double previousPower=0;bool previousLob=false;double previousAngle=NAN;unsigned plannerRevision=0;double nextPlanSearch=0,lastSearch=0,lastPlanAt=0,planChoiceAt=-10;
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
   auto forecast=predictedThrow(clicked,captured);if(!forecast.valid)forecast=clicked.solution;const auto risk=ff3qb::riskForecast(forecast,clicked.gravity,clicked.hudDefenders,std::clamp(clicked.ping*.0005+.05,0.,.25),clicked.arrivalFirst?NAN:clicked.groundY);intent.predictionKnown=clicked.ready&&clicked.hudCoverageKnown;intent.predictedIntable=intent.predictionKnown&&risk.intable;if(intent.predictedIntable)intent.predictedRiskAt=risk.projected.time;
   throwTracker.begin(intent,clicked.balls,people);
   releaseVisual.reset();if(clicked.ready&&forecast.valid){releaseVisual=std::make_shared<Frame>(clicked);releaseVisual->releaseVisual.reset();releaseVisual->solution=forecast;releaseVisual->risk=risk;releaseVisual->at=captured;releaseVisual->afterRelease=true;}
  }}
  const auto before=throwTracker.view.phase;throwTracker.update(observedAt,f.model,f.own,flightTracker.flights,people,f.groundY,actors.size()<=128);f.throwView=throwTracker.view;f.releaseVisual=releaseVisual;
  if(before!=ff3qb::ThrowView::Done&&f.throwView.phase==ff3qb::ThrowView::Done){auto action="Throw result: "+f.throwView.result;record(f,action.c_str());}
 }
 ~Reader(){if(handle)CloseHandle(handle);}
 Frame read(addr forced=0,int inputPower=0,bool cameraPanning=false,bool controlsOnly=false,bool releaseOnly=false){
  Frame f;f.boundaryMargin=boundaryHalfStuds.load()*.5;f.inputThrow=wheelMethod();f.releaseVisual=releaseVisual;f.at=now();f.revision=revision;f.mode=passMode;f.throwView=throwTracker.view;auto session=gameModules::router.get();f.pid=session.pid;f.model=session.model;f.practiceRange=isPracticeRange(session.placeId);
  if(!admitted()){f.reason="Connect to Football Fusion 3";return f;}
  if(pid!=f.pid){if(handle)CloseHandle(handle);handle=OpenProcess(PROCESS_VM_READ|SYNCHRONIZE,FALSE,f.pid);pid=f.pid;model=moduleBase=0;lockedTarget=0;routeTarget=0;flightWorkspace=0;groundY=NAN;flightTracker.clear();}
  if(!handle||WaitForSingleObject(handle,0)!=WAIT_TIMEOUT){f.reason="Waiting for the game process";return f;}
  auto h=handle;auto root=f.model;auto ws=child(h,root,"Workspace"),players=child(h,root,"Players");
  auto player=rd<addr>(h,players+Offsets::Player::LocalPlayer);f.localPlayer=player;f.team=rd<addr>(h,player+Offsets::Player::Team);f.own=rd<addr>(h,player+Offsets::Player::ModelInstance);
  auto head=child(h,f.own,"Head"),hum=child(h,f.own,"Humanoid");
  const float health=rd<float>(h,hum+Offsets::Humanoid::Health,NAN);
  f.window=clientWindow(f.pid);if(!f.window){f.reason="Waiting for the Roblox window";return f;}
  if(!GetClientRect(f.window,&f.client)||!ClientToScreen(f.window,&f.origin)||!GetCursorPos(&f.cursor)){f.reason="Waiting for viewport";return f;}
  const int width=f.client.right,height=f.client.bottom;
 auto camera=rd<addr>(h,ws+Offsets::Workspace::CurrentCamera);Vec cameraPosition=vector(rd<shooting::Vec>(h,camera+Offsets::Camera::Position,{NAN,NAN,NAN}));f.camera=camera;f.workspace=ws;f.cameraPosition=cameraPosition;f.cameraRotation=rd<ff3qb::AimRotation>(h,camera+Offsets::Camera::Rotation);
  // Project using camera view/projection from the same version-gated process.
  // Resolve once per process, independently of Basketball's cleared scene.
  if(!moduleBase){HANDLE modules=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);MODULEENTRY32W entry{sizeof entry};if(modules!=INVALID_HANDLE_VALUE){if(Module32FirstW(modules,&entry))moduleBase=(addr)entry.modBaseAddr;CloseHandle(modules);}}
  const auto visual=rd<addr>(h,moduleBase+Offsets::VisualEngine::Pointer);
  struct Matrix{float m[16];};Matrix invalid;for(auto& v:invalid.m)v=NAN;const auto matrix=rd<Matrix>(h,visual+Offsets::VisualEngine::ViewMatrix,invalid);std::copy(std::begin(matrix.m),std::end(matrix.m),f.matrix);
  if(!ff3qb::finite(cameraPosition)||!visual||!std::all_of(std::begin(f.matrix),std::end(f.matrix),[](float v){return std::isfinite(v);})){f.reason="Camera data unavailable";return f;}
  f.cameraMatrix=visual+Offsets::VisualEngine::ViewMatrix;
  if(groundWorkspace!=ws){groundWorkspace=ws;groundY=NAN;}
  // Use the local grounded rig's foot plane; retain it while jumping.
  if(f.own&&rd<int>(h,hum+Offsets::Humanoid::FloorMaterial,1792)!=1792&&rd<unsigned char>(h,hum+Offsets::Humanoid::PlatformStand,255)==0&&rd<unsigned char>(h,hum+Offsets::Humanoid::EvaluateStateMachine,255)==1&&rd<int>(h,rd<addr>(h,hum+Offsets::Humanoid::HumanoidState)+Offsets::Humanoid::HumanoidStateID,-1)==8){
   auto foot=child(h,f.own,"LeftFoot");if(!foot)foot=child(h,f.own,"Left Leg");
   auto p=position(h,foot),size=partSize(h,foot),v=velocity(h,child(h,f.own,"HumanoidRootPart"));
   if(foot&&ff3qb::finite(p)&&ff3qb::finite(size)&&size.y>0&&size.y<8&&ff3qb::finite(v)&&std::abs(v.y)<2)groundY=p.y-size.y*.5;
  }
  f.menuOpen=rd<unsigned char>(h,child(h,root,"GuiService")+Offsets::GuiService::MenuIsOpen,255)!=0;auto markerInput=rd<addr>(h,child(h,root,"UserInputService")+Offsets::UserInputService::WindowInputState);f.textFocus=!markerInput||rd<addr>(h,markerInput+Offsets::WindowInputState::CurrentTextBox)!=0;
  f.groundY=groundY;f.head=position(h,head);if(enabled&&!controlsOnly&&!releaseOnly){if(!f.practiceRange){f.field=fieldReader.read(h,ws,f.at);f.fieldSurfaceY=fieldReader.surfaceY;}refreshActors(h,root,ws,players,f.practiceRange,f.at);}if(!controlsOnly){collectBalls(h,ws,f);updateThrow(h,ws,f);}
  if(releaseOnly)return f;
  if(!f.own||!std::isfinite(health)||health<=0){f.reason="Waiting for your live character";return f;}
  if(!enabled){f.reason="QB aim off";return f;}
  if(!controlsOnly&&!focused(f.pid)){f.reason="Tracking in background; focus Roblox to throw";return f;}
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
  std::vector<ff3Input::Wheel> wheels;int guiOrder=0;
  for(auto gui:children(h,child(h,player,"PlayerGui"))){
   ++guiOrder;if(nm(h,gui)!="BallGui"||defense::classOf(h,gui)!="ScreenGui")continue;
   auto panel=child(h,gui,"Frame0"),display=child(h,panel,"Disp");
   if(!descendant(h,panel,gui)||!descendant(h,display,panel)||defense::classOf(h,display)!="TextLabel")continue;
   wheels.push_back({gui,rd<int>(h,gui+Offsets::ScreenGui::DisplayOrder),guiOrder,rd<unsigned char>(h,gui+Offsets::GuiObject::ScreenGui_Enabled)==1,rd<unsigned char>(h,panel+Offsets::GuiObject::Visible)==1&&rd<unsigned char>(h,display+Offsets::GuiObject::Visible)==1});
  }
  f.powerGui=ff3Input::topWheel(wheels);
  std::vector<ff3qb::PowerDisplay> powerDisplays;
  if(auto gui=f.powerGui){
   auto frame=child(h,gui,"Frame0"),display=child(h,frame,"Disp");
   if(!descendant(h,frame,gui)||!descendant(h,display,frame)||defense::classOf(h,display)!="TextLabel"){f.reason="Power wheel changed";return f;}
   powerDisplays.push_back({rd<unsigned char>(h,gui+Offsets::GuiObject::ScreenGui_Enabled),rd<unsigned char>(h,frame+Offsets::GuiObject::Visible),defense::textAt(h,display+Offsets::GuiObject::Text)});
   if(wheelMethod()&&rd<unsigned char>(h,gui+Offsets::GuiObject::ScreenGui_Enabled)==1&&rd<unsigned char>(h,frame+Offsets::GuiObject::Visible)==1){
    int order=0;for(auto button:children(h,frame)){
     ++order;const auto kind=defense::classOf(h,button);auto name=nm(h,button);
     int value=ff3Input::powerIndex(name); // all 100 buttons participate in overlap checks
     if(!value||(kind!="ImageButton"&&kind!="TextButton")||rd<unsigned char>(h,button+Offsets::GuiObject::Visible)!=1)continue;
     struct P{float x,y;};auto p=rd<P>(h,button+Offsets::GuiBase2D::AbsolutePosition,{NAN,NAN}),size=rd<P>(h,button+Offsets::GuiBase2D::AbsoluteSize,{NAN,NAN});
     f.powerButtons.push_back({button,value,rd<int>(h,button+Offsets::GuiObject::ZIndex),order,{p.x,p.y},{size.x,size.y},rd<float>(h,button+Offsets::GuiBase2D::AbsoluteRotation,NAN)});
    }
   }
  }
  const int visiblePower=ff3qb::readPower(f.held,powerDisplays);
  f.gamePower=std::max(0,visiblePower);f.wheelVisible=f.gamePower>0;f.power=f.gamePower;
  const std::string powerReason=visiblePower==-2?"Visible power displays disagree":visiblePower==-1?"Football power text is invalid":packetMode?"Waiting for game power; enable calculated power to throw without the wheel":"Equip a football and show its power wheel";
  if(controlsOnly){f.reason=f.wheelVisible?"Power controls sampled":powerReason;return f;}

  const bool cursorAvailable=(cameraPanning&&cameraPanOwned&&!physicalRightHeld)||freeCursor();
  if(wheelMethod()&&(!cursorAvailable||rd<int>(h,player+Offsets::Player::CameraMode,-1)!=0||ff3qb::length(cameraPosition-position(h,head))<2.0)){f.reason="Use third-person with shift-lock off; release right mouse";return f;}
  // Roblox can hide the Windows cursor while drawing its own cursor. Hidden
  // does not mean center-locked. GetCursorPos also follows a genuinely locked
  // cursor, without alternating between two unrelated selection positions.
  if(!lockedTarget&&!heldTarget&&(f.cursor.x<f.origin.x||f.cursor.y<f.origin.y||f.cursor.x>=f.origin.x+width||f.cursor.y>=f.origin.y+height)){f.reason="Move the cursor into the game";return f;}
  auto mouseService=child(h,root,"MouseService"),mouseInput=rd<addr>(h,mouseService+Offsets::MouseService::InputObject);
  f.mouseService=mouseService;f.mouseInput=mouseInput;
  f.gameMouse=vector(rd<shooting::Vec>(h,mouseInput+Offsets::MouseService::MousePosition,{NAN,NAN,NAN}));
  if(wheelMethod()&&(!ff3qb::finite(f.gameMouse)||f.gameMouse.x<0||f.gameMouse.x>width||f.gameMouse.y<0||f.gameMouse.y>height)){f.reason="Game cursor data unavailable";return f;}

  const auto ownTeam=rd<addr>(h,player+Offsets::Player::Team);const bool practicePlace=f.practiceRange;
  Vec origin=position(h,head);f.head=origin;f.headPrimitive=rd<addr>(h,head+Offsets::BasePart::Primitive);double best=std::min(double(selectionRadius.load()),std::min(width,height)*.35);
  std::vector<ff3qb::Defender> defenders;
  const addr pinned=heldTarget.load()?heldTarget.load():lockedTarget.load();const addr wanted=forced?forced:pinned;f.locked=pinned!=0;
  Vec targetPosition{},targetVelocity{},retainedPosition{},retainedVelocity{};Frame retainedFrame;bool retainedAvailable=false;
  for(const auto& a:actors){

   if(a.npc){if(!practice||!practicePlace||!rd<bool>(h,child(h,a.rig,"WR")+Offsets::Misc::Value))continue;auto active=child(h,a.rig,"Enabled");if(active&&!rd<bool>(h,active+Offsets::Misc::Value))continue;}
   else if(ownTeam){if(rd<addr>(h,a.player+Offsets::Player::Team)!=ownTeam)continue;}
   else if(!practice||!practicePlace||rd<addr>(h,a.player+Offsets::Player::Team)!=0)continue;
   auto hrp=child(h,a.rig,"HumanoidRootPart"),targetHum=child(h,a.rig,"Humanoid");auto hp=rd<float>(h,targetHum+Offsets::Humanoid::Health,NAN);
   if(!std::isfinite(hp)||hp<=0||!hrp)continue;
   Vec p=position(h,child(h,a.rig,"Head")),v=velocity(h,hrp);if(!ff3qb::finite(p))p=position(h,hrp)+Vec{0,1.5,0};if(!ff3qb::finite(p))continue;if(ff3qb::finite(v))v.y=0;
   const bool guiding=visualOnly&&f.held&&a.rig==retained;
   auto screen=ff3qb::project(p,f.matrix,width,height);if(!screen.valid&&!wanted&&!guiding)continue;
   auto top=ff3qb::project(p+Vec{0,1.5,0},f.matrix,width,height),bottom=ff3qb::project(p-Vec{0,4.5,0},f.matrix,width,height);
   double distance=top.valid&&bottom.valid?ff3qb::selectionScore(f.cursor.x-f.origin.x,f.cursor.y-f.origin.y,bottom.x,bottom.y,top.x,top.y,std::clamp(std::abs(top.y-bottom.y)*.18,2.,14.)):std::hypot(screen.x-(f.cursor.x-f.origin.x),screen.y-(f.cursor.y-f.origin.y));
   f.receiverVisuals.push_back({a.rig,p,v,nm(h,a.rig)});
   if(wanted&&a.rig!=wanted)continue;
   f.candidates.push_back({screen,nm(h,a.rig),false});
   if(ff3qb::length(p-origin)<6||ff3qb::length(p-origin)>1000)continue;
   if(a.rig==retained&&selectionRetention.keep(a.rig,distance,std::min(double(selectionRadius.load()),std::min(width,height)*.35),f.at,wanted!=0||guiding)){retainedAvailable=true;retainedFrame.target=a.rig;retainedFrame.name=nm(h,a.rig);retainedFrame.receiver=screen;retainedPosition=p;retainedVelocity=v;}
   if(!wanted&&(distance>best||(distance==best&&f.target&&(f.target==retained||(a.rig!=retained&&a.rig>f.target)))))continue;
   best=distance;f.target=a.rig;f.name=nm(h,a.rig);f.receiver=screen;targetPosition=p;targetVelocity=v;
  }
  std::vector<Vec> opponents;for(const auto& a:actors){if(a.npc?(!practicePlace||rd<bool>(h,child(h,a.rig,"WR")+Offsets::Misc::Value)):(!ownTeam||a.team==ownTeam))continue;const auto humanoid=child(h,a.rig,"Humanoid");if(rd<float>(h,humanoid+Offsets::Humanoid::Health,0)<=0)continue;const auto point=position(h,child(h,a.rig,"HumanoidRootPart"));if(ff3qb::finite(point))opponents.push_back(point);}
  for(auto& receiver:f.receiverVisuals)for(const auto& point:opponents){receiver.space=std::min(receiver.space,ff3qb::length(Vec{point.x-receiver.position.x,0,point.z-receiver.position.z}));receiver.known=true;}
  selection.current=retained;auto selected=selection.update(f.target,retainedAvailable,f.at,wanted!=0,wheelMethod()?.12:0);
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
  if(receiverChanged||plannerRevision!=f.revision){plannerRevision=f.revision;routeTarget=f.target;routeTool=f.tool;angleHistory={};beamMemory={};routePass={};previousFit={};previousPower=0;previousLob=false;previousAngle=NAN;nextPlanSearch=0;planChoiceAt=-10;}
  const Vec measuredReceiverPosition=targetPosition;f.rawTargetVelocity=targetVelocity;
  auto ping=path(h,root,{"Stats","PerformanceStats","Ping"});double pingValue=rd<double>(h,ping+0x98,NAN);
  if(!std::isfinite(pingValue)||pingValue<1||pingValue>400){f.reason="Waiting for usable ping";return f;}f.ping=pingValue;
  const bool remotePlayer=!f.practiceRange&&std::any_of(actors.begin(),actors.end(),[&](const auto& actor){return actor.rig==f.target&&!actor.npc;});
  const auto timing=ff3qb::throwTiming(f.ping,remotePlayer);f.replicationLead=timing.replicationLead;f.timingTolerance=timing.tolerance;f.previewDelay=wheelMethod()&&!cursorOnly&&!flickActive?animationDelay.load():0;
  routeHistory.update(f.at,origin,position(h,child(h,f.target,"HumanoidRootPart")),targetVelocity);targetVelocity=routeHistory.movement;if(ff3qb::length(targetVelocity)<.5)targetVelocity={};
  f.wobbleFiltered=routeHistory.intent.wobble;f.routeCorrection=routeHistory.intent.correction;targetPosition=targetPosition+f.routeCorrection+targetVelocity*(f.replicationLead+f.previewDelay);
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
   defenders.push_back({p+v*((a.npc?0:f.replicationLead)+f.previewDelay),v,potential,jump,double(diveReach.load()),a.shape.radius,a.shape.above,a.shape.below,std::isfinite(worldGravity)?worldGravity:196.2});
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
  if(!ff3qb::powerAvailable(f.held,(packetMode||visualOnly)&&calculatesPower(),visiblePower)){f.reason=powerReason;return f;}
  const auto gravity=path(h,ball,{"Configuration","Gravity"});
  f.gravity=-rd<double>(h,gravity+Offsets::Misc::Value,NAN);
  if(!gravity||defense::classOf(h,gravity)!="NumberValue"||!std::isfinite(f.gravity)||f.gravity<1||f.gravity>300){f.reason="Ball physics unavailable";return f;}

  // The server launches five studs along the sent direction. Solve that
  // offset exactly, using the same model as the packet editor.
  const bool powerFirst=automaticAngle&&(calculatesPower()||visualOnly)&&f.mode==0;
  const bool fastestRoute=powerFirst&&calculatesPower()&&!force95;
  f.arrivalFirst=fastestRoute;
  // An interception after the WR's contact time is a missed-catch forecast,
  // not a reason to replace a clear fast delivery with a sky lob.
  const double coverageGround=fastestRoute?NAN:f.groundY;
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
   ff3qb::Defender receiver{measuredReceiverPosition+targetVelocity*(f.replicationLead+f.previewDelay),targetVelocity,20,jump,double(diveReach.load())};bool measuredReceiver=false,receiverIsPlayer=false;
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
  f.minimumAngle=automaticAngle&&calculatesPower()?ff3qb::openAwayMinimum(origin,aimTarget,targetVelocity,defenders,f.coverageKnown):-30.;
  if(powerFirst&&calculatesPower()&&!force95)f.minimumAngle=ff3qb::normalArcMinimum(stableAngle,f.minimumAngle);
  f.maximumAngle=!automaticAngle?85:f.minimumAngle>=5?50:force95?85:50;
  if(fastestRoute){f.minimumAngle=-30;f.maximumAngle=85;}
  auto planned=powerFirst?ff3qb::Plan{}:ff3qb::planThrow(origin,aimTarget,targetVelocity,f.gravity,std::clamp(f.ping*.0005+.05,0.,.25),f.mode,automaticAngle,manualAngle,calculatesPower(),f.gamePower,arc==1,smartFit,defenders,stableAngle,previousFit,powerFirst,5,f.groundY,f.maximumAngle,throwBounds(f),&receiver,f.minimumAngle,f.boundaryMargin);
  if(powerFirst){
   const bool searching=f.at>=nextPlanSearch;const double step=lastPlanAt?f.at-lastPlanAt:.1;
   const double latency=std::clamp(f.ping*.0005+.05,0.,.25);
   // Auto searches power, both arc roots and route-relative placements in
   // either mode. A contested loft must not overwrite a less exposed bullet.
   planned=ff3qb::planCoverageThrow(origin,aimTarget,targetVelocity,f.gravity,latency,defenders,smartFit,force95,jump,world,previousFit,previousPower,previousLob,previousAngle,searching,step,&receiver,f.leadOffset.y,&beamMemory,f.at,coverageGround,f.maximumAngle,throwBounds(f),f.minimumAngle,f.boundaryMargin,visualOnly?f.gamePower:inputPower,stableAngle,f.timingTolerance,wheelMethod()?5:1,wheelMethod()?f.gamePower:0,fastestRoute);
   f.candidatesConsidered=beamMemory.candidatesConsidered;f.candidatesRanked=beamMemory.candidatesRanked;f.powersChecked=beamMemory.tiersVisited;f.rankedThrows=beamMemory.leaders;
   f.passLabel=force95?"Beam":planned.solution.angle>=35?"Lob":planned.solution.angle<=15?"Bullet":"Smart fit";lastPlanAt=f.at;
   if(f.at>=nextPlanSearch){lastSearch=f.at;nextPlanSearch=f.at+.10;}
  }
  if(!powerFirst&&automaticAngle&&calculatesPower()&&planned.solution.valid){
   const double angle=std::min(force95?75.:50.,beamMemory.elevation.update(planned.solution.angle,f.at));
   if(std::abs(angle-planned.solution.angle)>.001){
    auto quiet=ff3qb::solveAngle(origin,aimTarget+planned.offset,targetVelocity,angle,f.gravity,std::clamp(f.ping*.0005+.05,0.,.25),5);
    double margin=quiet.valid?ff3qb::coverageMargin(quiet,f.gravity,defenders,std::clamp(f.ping*.0005+.05,0.,.25),nullptr,-INFINITY,f.groundY):-INFINITY;
    bool reachable=catchAllowed(f,quiet,receiver,std::clamp(f.ping*.0005+.05,0.,.25));
    if(reachable&&quiet.angle>=f.minimumAngle-1e-6&&(!smartFit||margin>=planned.margin-.25)){planned.solution=quiet;planned.power=ff3qb::length(quiet.velocity);planned.margin=margin;}
    else beamMemory.elevation.accept(planned.solution.angle,f.at);
   }
  }
  if(planned.solution.valid){
   // A UI button selects an integer power; solve the real throw at that exact
   // value instead of sending a trajectory calculated at a fractional power.
   ff3qb::Solution best;double chosen=0,cost=INFINITY;bool lob=false;
   const auto neighbors=ff3qb::powerNeighbors(planned.power,wheelMethod()?5:1,wheelMethod()?f.gamePower:0);
   const int lower=inputPower?inputPower:int(neighbors[0]),upper=inputPower?inputPower:int(neighbors[1]);
   for(int power:{lower,upper})if(power>=1&&power<=95)for(bool high:{false,true}){
    auto candidate=ff3qb::solve(origin,aimTarget+planned.offset,targetVelocity,power,f.gravity,std::clamp(f.ping*.0005+.05,0.,.25),high,5);
    if(!candidate.valid||candidate.angle<f.minimumAngle-1e-6||candidate.angle>f.maximumAngle+1e-6||!catchAllowed(f,candidate,receiver,std::clamp(f.ping*.0005+.05,0.,.25)))continue;
    const double score=std::abs(candidate.angle-planned.solution.angle)+std::abs(power-planned.power)*.1;
    if(score<cost){best=candidate;chosen=power;lob=high;cost=score;}
   }
   // Smart Fit can use a different reachable arc when five-point wheel
   // rounding leaves the preferred angle band empty. Keep catch/OOB checks;
   // do not hide a valid physical lob just because the preferred band failed.
   if(!best.valid&&wheelMethod()&&calculatesPower()&&automaticAngle&&f.mode==0&&!force95&&!inputPower){
    for(int power=1;power<=95;++power)if(ff3qb::powerReachable(power,5,f.gamePower))for(bool high:{false,true}){
     auto candidate=ff3qb::solve(origin,aimTarget+planned.offset,targetVelocity,power,f.gravity,std::clamp(f.ping*.0005+.05,0.,.25),high,5);
     if(!candidate.valid||candidate.angle< -30||candidate.angle>85||!catchAllowed(f,candidate,receiver,std::clamp(f.ping*.0005+.05,0.,.25)))continue;
     const double score=std::abs(candidate.angle-planned.solution.angle)+std::abs(power-planned.power)*.1;
     if(score<cost){best=candidate;chosen=power;lob=high;cost=score;}
    }
    if(best.valid){f.minimumAngle=-30;f.maximumAngle=85;}
   }
   planned.solution=best;planned.power=chosen;planned.lob=lob;
   if(best.valid)planned.margin=defenders.empty()?10:ff3qb::coverageMargin(best,f.gravity,defenders,std::clamp(f.ping*.0005+.05,0.,.25),nullptr,-INFINITY,coverageGround);
  }
  f.planningMs=(now()-planningStart)*1000;
  if(wheelMethod()&&calculatesPower()&&automaticAngle&&f.mode==0&&!force95&&!inputPower&&previousPower>0){
   ff3qb::Plan prior;prior.solution=ff3qb::solve(origin,aimTarget+previousFit,targetVelocity,previousPower,f.gravity,std::clamp(f.ping*.0005+.05,0.,.25),previousLob,5);prior.power=previousPower;prior.offset=previousFit;prior.lob=previousLob;
   if(prior.solution.valid&&(prior.solution.angle<f.minimumAngle-1e-6||prior.solution.angle>f.maximumAngle+1e-6||!catchAllowed(f,prior.solution,receiver,std::clamp(f.ping*.0005+.05,0.,.25))))prior.solution={};
   if(prior.solution.valid){prior.margin=defenders.empty()?10:ff3qb::coverageMargin(prior.solution,f.gravity,defenders,std::clamp(f.ping*.0005+.05,0.,.25),nullptr,-INFINITY,coverageGround);prior.coverageKnown=f.coverageKnown;prior.deliveryMargin=prior.launchMargin=prior.robustMargin=prior.margin;if(keepWheelPlan(prior,planned,planChoiceAt,f.at,fastestRoute))planned=prior;}
  }
  if(planned.solution.valid&&(previousPower!=planned.power||previousLob!=planned.lob||ff3qb::length(previousFit-planned.offset)>.01))planChoiceAt=f.at;
  if(planned.solution.valid){
   previousFit=planned.offset;previousPower=planned.power;previousLob=planned.lob;previousAngle=planned.solution.angle;
   f.preparationPower=planned.power;
   // The physical click uses the wheel's present value. Publish a complete
   // plan at that value before it can be clicked, while preparing the goal.
   // Never substitute another root/power after the animation has started.
   if(wheelMethod()&&!cursorOnly&&calculatesPower()&&planned.power!=f.gamePower){
    planned=ff3qb::planCoverageThrow(origin,aimTarget,targetVelocity,f.gravity,std::clamp(f.ping*.0005+.05,0.,.25),defenders,smartFit,force95,jump,world,planned.offset,0,false,NAN,true,.1,&receiver,f.leadOffset.y,nullptr,f.at,coverageGround,f.maximumAngle,throwBounds(f),f.minimumAngle,f.boundaryMargin,f.gamePower,stableAngle,f.timingTolerance,5,f.gamePower,fastestRoute);
    if(!planned.solution.valid){f.power=f.gamePower;f.reason="Preparing power: no reachable arc at the current wheel value";f.planningMs=(now()-planningStart)*1000;return f;}
   }
  }
  auto solution=planned.solution;
  f.deliveryMargin=ff3qb::deliveryClearance(planned);f.launchMargin=ff3qb::launchClearance(planned);
  // Very low bullets use their selected power/root at dispatch. The manual
  // fixed-angle solver intentionally starts at five degrees.
  f.fixedPowerRoute=powerFirst&&solution.valid&&solution.angle<5;
  if(!solution.valid||((smartFit||selectedHeight()!=0)&&measuredReceiver&&ff3qb::receiverGap(receiver,solution.catchPoint,solution.time+std::clamp(f.ping*.0005+.05,0.,.25))>-.25)){f.fitBlocked=smartFit;f.reason=f.practiceRange?"No reachable practice catch at these settings":"No reachable catch within the OOB limit at this power";return f;}
  if(!catchAllowed(f,solution,receiver,std::clamp(f.ping*.0005+.05,0.,.25))){f.reason=f.practiceRange?"No receiver-reachable practice catch":"No receiver-reachable catch within the OOB limit";return f;}
  if(automaticAngle&&calculatesPower()){auto high=ff3qb::solve(origin,aimTarget+planned.offset,targetVelocity,planned.power,f.gravity,std::clamp(f.ping*.0005+.05,0.,.25),true,5);planned.lob=high.valid&&std::abs(high.time-solution.time)<1e-5;}
  f.power=planned.power;f.fitOffset=planned.offset;f.targetPosition=aimTarget+planned.offset;f.margin=defenders.empty()?10:planned.margin;f.selectedLob=planned.lob;f.distance=ff3qb::length(targetPosition-origin);
  f.planningMs=(now()-planningStart)*1000;
  f.solution=solution;f.passLabel=force95?"Beam":solution.angle>=35?"Lob":solution.angle<=15?"Bullet":"Smart fit";f.risk=ff3qb::riskForecast(solution,f.gravity,f.hudDefenders,std::clamp(f.ping*.0005+.05,0.,.25),coverageGround);
  if(!defenders.empty())f.margin=std::min(planned.margin,ff3qb::coverageMargin(solution,f.gravity,defenders,std::clamp(f.ping*.0005+.05,0.,.25),&f.coverageTrace,-INFINITY,coverageGround));
  // Match the camera's 1000-stud sky-ray endpoint to the launch direction.
  // Actual scene hits still belong to Roblox's mouse raycast.
  f.aim=ff3qb::project(ff3qb::mouseAimWorld(origin,f.cameraPosition,solution.velocity),f.matrix,width,height);
  if(!descendant(h,f.tool,f.own)||rd<addr>(h,player+Offsets::Player::ModelInstance)!=f.own||rd<std::uint64_t>(h,root+Offsets::DataModel::GameId)!=gameModules::registry[1].universe){f.reason="Game state changed";return f;}
  f.ready=true;f.reason=f.coverageKnown&&f.coverageCount>0&&f.margin<=ff3qb::catchSafetyMargin?"Ready: contested fit to "+f.name:f.fitOffset.y>=3?"Elevated catch: "+f.name:"Ready: click to throw to "+f.name;return f;
 }
};
inline bool eligible(const Frame& f,double time,bool cameraPanning=false){return enabled&&!visualOnly&&f.revision==revision&&f.ready&&time>=f.at&&time-f.at<.10&&admitted()&&focused(f.pid)&&(packetMode||(cameraPanning&&cameraPanOwned&&!physicalRightHeld)||freeCursor());}
inline bool inGameClient(const Frame& f,POINT p){return p.x>=f.origin.x&&p.y>=f.origin.y&&p.x<f.origin.x+f.client.right&&p.y<f.origin.y+f.client.bottom;}
inline bool sameIdentity(const Frame& a,const Frame& b){return a.pid&&a.pid==b.pid&&a.model&&a.model==b.model&&a.own&&a.own==b.own&&a.tool&&a.tool==b.tool&&a.target&&a.target==b.target&&a.revision==b.revision&&(!a.inputThrow||!b.inputThrow||a.powerGui==b.powerGui);}
// InputObject is an event object, not a session identity. A real click or a
// power key can replace it while the same player/camera/service remain live.
inline bool refreshAimBinding(Frame& request,const Frame& live){
 if(!request.pid||request.pid!=live.pid||!request.model||request.model!=live.model||!request.own||request.own!=live.own||request.revision!=live.revision||
  !request.camera||request.camera!=live.camera||!request.workspace||request.workspace!=live.workspace||!request.mouseService||request.mouseService!=live.mouseService||!live.mouseInput||
  request.window!=live.window||request.origin.x!=live.origin.x||request.origin.y!=live.origin.y||request.client.right!=live.client.right||request.client.bottom!=live.client.bottom||!ff3qb::finite(live.gameMouse))return false;
 request.mouseInput=live.mouseInput;request.gameMouse=live.gameMouse;return true;
}
inline bool throwReleaseObserved(const Frame& shot,const Frame& live,const std::vector<std::uint64_t>& prior){
 for(const auto& ball:live.looseBalls){
  if(!ball.verifiedBall||!ff3qb::finite(ball.position)||!std::isfinite(ball.gravity)||ball.gravity<1||ball.gravity>300)continue;
  // collectBalls excludes every equipped/character-owned part. The held
  // handle appearing here is an actual world release, even if its ID reused
  // an older loose-ball entry. Otherwise require a new explicitly owned ball.
  if(ball.id==shot.heldBall)return true;
  if((ball.thrower==shot.localPlayer||ball.thrower==shot.own)&&ball.thrower&&std::find(prior.begin(),prior.end(),ball.id)==prior.end())return true;
 }
 return false;
}
inline bool dispatchConsistent(const Frame& request,const Frame& aimed,const Frame& verify,POINT cursor){
 if((verify.inputThrow&&(verify.gamePower!=verify.power||aimed.gamePower!=aimed.power))||!sameIdentity(request,aimed)||!sameIdentity(aimed,verify)||!verify.ready||!aimed.ready||verify.power!=aimed.power||verify.gravity!=aimed.gravity||verify.window!=aimed.window||verify.origin.x!=aimed.origin.x||verify.origin.y!=aimed.origin.y||verify.client.right!=aimed.client.right||verify.client.bottom!=aimed.client.bottom)return false;
 const POINT expected{aimed.origin.x+LONG(std::lround(aimed.aim.x)),aimed.origin.y+LONG(std::lround(aimed.aim.y))};
 return verify.aim.valid&&aimed.aim.valid&&ff3qb::cameraStable(aimed.matrix,verify.matrix)&&std::hypot(verify.aim.x-aimed.aim.x,verify.aim.y-aimed.aim.y)<8&&std::hypot(double(cursor.x-expected.x),double(cursor.y-expected.y))<4&&std::hypot(verify.gameMouse.x-aimed.aim.x,verify.gameMouse.y-aimed.aim.y)<4;
}
// Packet Beam returns the cursor to the user's click point after wheel selection.
// Its destination is supplied by the packet editor, not by cursor aiming.
inline bool packetWheelConsistent(const Frame& request,const Frame& aimed,const Frame& verify,POINT cursor,POINT expected){
 if(!sameIdentity(request,aimed)||!sameIdentity(aimed,verify)||!aimed.ready||!verify.ready||aimed.power!=verify.power||aimed.gamePower!=aimed.power||verify.gamePower!=verify.power||verify.window!=aimed.window||verify.origin.x!=aimed.origin.x||verify.origin.y!=aimed.origin.y||verify.client.right!=aimed.client.right||verify.client.bottom!=aimed.client.bottom)return false;
 return inGameClient(verify,expected)&&std::hypot(double(cursor.x-expected.x),double(cursor.y-expected.y))<4&&std::hypot(verify.gameMouse.x-(expected.x-verify.origin.x),verify.gameMouse.y-(expected.y-verify.origin.y))<4;
}
class CursorShot {
 ff3qb::DirectAimMemory memory_;Frame shot_;Vec original_{},last_{};std::mutex guard_;bool active_=false;
 bool identity(bool controls){
  const auto h=memory_.handle();const auto session=gameModules::router.get();
  if(!h||WaitForSingleObject(h,0)!=WAIT_TIMEOUT||session.pid!=shot_.pid||session.model!=shot_.model||!session.version||rd<addr>(h,shot_.localPlayer+Offsets::Player::ModelInstance)!=shot_.own||rd<addr>(h,shot_.workspace+Offsets::Workspace::CurrentCamera)!=shot_.camera||rd<addr>(h,shot_.mouseService+Offsets::MouseService::InputObject)!=shot_.mouseInput)return false;
  return !controls||(enabled&&cursorOnly&&!inputCancelled&&revision==shot_.revision&&admitted()&&focused(shot_.pid)&&freeCursor());
 }
 static Vec point(const Frame& f){return ff3qb::cursorAimPoint(f.head,f.cameraPosition,f.solution.velocity,f.matrix,f.client.right,f.client.bottom,f.gameMouse.z);}
public:
 explicit CursorShot(const Frame& f):memory_(f.pid),shot_(f){}
 bool available(){return identity(true)&&defense::classOf(memory_.handle(),shot_.mouseInput)=="InputObject"&&ff3qb::finite(point(shot_));}
 bool begin(const Frame& f){
  std::lock_guard<std::mutex> lock(guard_);if(active_||!f.ready||!f.held||now()<f.at||now()-f.at>=.1)return false;shot_=f;
  const auto address=shot_.mouseInput+Offsets::MouseService::MousePosition;last_=point(shot_);
  if(!identity(true)||!ff3qb::finite(last_)||!memory_.cursor(address,original_)||!memory_.writeCursor(address,last_,[&](){return identity(true);}))return false;
  active_=true;return true;
 }
 bool pulse(){
  std::lock_guard<std::mutex> lock(guard_);if(!active_||!identity(true))return false;
  auto f=shot_;f.head=position(memory_.handle(),child(memory_.handle(),f.own,"Head"));f.cameraPosition=vector(rd<shooting::Vec>(memory_.handle(),f.camera+Offsets::Camera::Position,{NAN,NAN,NAN}));
  struct Matrix{float m[16];};const auto view=rd<Matrix>(memory_.handle(),f.cameraMatrix);std::copy(std::begin(view.m),std::end(view.m),f.matrix);f.solution=predictedThrow(f,now());
  const auto next=point(f);if(!f.solution.valid||!ff3qb::finite(next)||!memory_.writeCursor(shot_.mouseInput+Offsets::MouseService::MousePosition,next,[&](){return identity(true);}))return false;last_=next;return true;
 }
 void finish(){std::lock_guard<std::mutex> lock(guard_);if(active_){memory_.restoreCursor(shot_.mouseInput+Offsets::MouseService::MousePosition,last_,original_,[&](){return identity(false);});active_=false;}}
 ~CursorShot(){finish();}
};
inline LRESULT CALLBACK mouseProc(int n,WPARAM w,LPARAM l){
 if(n==HC_ACTION){auto* event=(MSLLHOOKSTRUCT*)l;const bool injected=(event->flags&LLMHF_INJECTED)!=0;
  if(!injected){if(w==WM_RBUTTONDOWN)physicalRightHeld=true;else if(w==WM_RBUTTONUP)physicalRightHeld=false;}
  if(inputObserver&&!injected){unsigned key=w==WM_LBUTTONDOWN||w==WM_LBUTTONUP?VK_LBUTTON:w==WM_RBUTTONDOWN||w==WM_RBUTTONUP?VK_RBUTTON:w==WM_MBUTTONDOWN||w==WM_MBUTTONUP?VK_MBUTTON:w==WM_XBUTTONDOWN||w==WM_XBUTTONUP?(HIWORD(event->mouseData)==XBUTTON1?VK_XBUTTON1:VK_XBUTTON2):0;if(key)inputObserver(key,w==WM_LBUTTONDOWN||w==WM_RBUTTONDOWN||w==WM_MBUTTONDOWN||w==WM_XBUTTONDOWN);}
  // Own the matching up even if the user changes modes after a rejected down.
  if(!injected&&(w==WM_LBUTTONUP||w==WM_LBUTTONDOWN)){std::lock_guard<std::mutex> lock(mutex);if(w==WM_LBUTTONUP?blockedPacketClick.up():blockedPacketClick.owned)return 1;}
  if(!injected&&(w==WM_LBUTTONUP||w==WM_LBUTTONDOWN)){
   std::lock_guard<std::mutex> lock(mutex);
   if(flickClicks.owned){
    if(w==WM_LBUTTONUP){flickClicks.up();physicalLeftHeld=false;physicalReleasedAt=GetTickCount64();}
    return 1;
   }
  }
  if(packetMode&&!injected&&(w==WM_LBUTTONDOWN||w==WM_LBUTTONUP)){
   Frame click;bool captured=false;const bool transport=packetInputReady&&packetInputReady();
   {std::lock_guard<std::mutex> lock(mutex);const double time=now();
    if(w==WM_LBUTTONDOWN&&enabled&&current.pid&&focused(current.pid)&&admitted()&&!current.menuOpen&&!current.textFocus&&!powerWheelHit(current,event->pt)){
     if(current.held){const auto issue=packetPressIssue(current,transport,time,revision.load());if(!issue.empty()){
      blockedPacketClick.down(true,time);cancelPress();auto rejected=current;rejected.reason=issue;traceInput("packet-down-blocked",true,transport,rejected);
      if(pendingPacketCancellations.size()<8)pendingPacketCancellations.emplace_back(rejected,issue);return 1;
     }}
     if(!physicalLeftHeld)physicalClickAt=GetTickCount64();physicalLeftHeld=true;physicalReleasedAt=0;
     heldTarget=lockedTarget.load()?lockedTarget.load():current.target;requested=current;requestedAt=time;click=requested;captured=true;traceInput("physical-down-passed",false,transport,current);
    }else if(w==WM_LBUTTONUP&&physicalLeftHeld){
     physicalLeftHeld=false;physicalReleasedAt=GetTickCount64();if(current.ready&&samePressSession(requested,current)){requested=current;requestedAt=time;}
     click=current;captured=true;traceInput("physical-up-passed",false,transport,current);
    }
   }if(captured&&frameSink)frameSink(click);
  }
  if(wheelMethod()&&!injected&&enabled){
   const auto tick=GetTickCount64();
   if(w==WM_MOUSEMOVE){
    const bool moved=physicalMouseMovement.update(event->pt.x,event->pt.y);
    // Repeated same-position events are common with Roblox's drawn cursor.
    // They must not restart the settle timer forever while the user is still.
    if(moved&&(!flickActive||std::hypot(double(event->pt.x-cameraPixelX.load()),double(event->pt.y-cameraPixelY.load()))>2)){if(flickActive&&!flickDispatched)inputCancelled=true;cameraPreparation.pauseUntil(tick+150);}
   }else if(w==WM_RBUTTONDOWN){if(inputBusy)inputCancelled=true;cameraPreparation.pauseUntil(tick+150);}
   if(w==WM_LBUTTONDOWN||w==WM_LBUTTONUP){std::lock_guard<std::mutex> lock(mutex);
    if(w==WM_LBUTTONDOWN&&!cursorOnly&&inGameClient(current,event->pt)&&!powerWheelHit(current,event->pt)){inputCancelled=true;cameraPreparation.pauseUntil(tick+500);}
    if(w==WM_LBUTTONDOWN&&!cursorOnly&&current.held&&focused(current.pid)&&admitted()&&!current.menuOpen&&!current.textFocus&&!powerWheelHit(current,event->pt)){
     const auto time=now();
     if(!flickActive&&!flickClicks.pending&&inGameClient(current,event->pt)&&freeCursor()&&commitCameraShot(current,time,revision.load(),event->pt,flickRequest)){
      flickClicks.pending=true;flickClicks.at=time;flickClicks.owned=false;
      heldTarget=current.target;physicalClickAt=tick;physicalLeftHeld=true;physicalReleasedAt=0;requested=flickRequest;requestedAt=time;inputCancelled=true;flickDispatched=true;
      traceInput("physical-throw-click-passed",false,false,current);
     }else traceInput("camera-manual-unprepared",false,false,current);
    }else if(w==WM_LBUTTONUP&&physicalLeftHeld){physicalLeftHeld=false;physicalReleasedAt=tick;}
   }
   // The game receives the real down/up pair. Aim follows its throw animation;
   // it never waits for a replacement synthetic click to start that animation.
  }
 }return CallNextHookEx(mouse,n,w,l);
}
inline bool angleKeyOwned[4]={false,false,false,false};
inline std::atomic<bool> physicalPowerHeld[2]{};
inline bool hotkey(unsigned key,const Frame& f,bool repeat){
 if(key=='Q'){if(repeat)return true;if(lockedTarget)lockedTarget=0;else if(f.target)lockedTarget=f.target;else return false;++revision;return true;}
 if(key=='Z'){if(!repeat){passMode=(passMode.load()+1)%6;force95=false;automaticAngle=true;autoPower=true;++revision;}return true;}
 return false;
}
inline LRESULT CALLBACK keyboardProc(int n,WPARAM w,LPARAM l){
 if(n==HC_ACTION){auto* event=(KBDLLHOOKSTRUCT*)l;if(!(event->flags&LLKHF_INJECTED)&&(w==WM_KEYDOWN||w==WM_SYSKEYDOWN||w==WM_KEYUP||w==WM_SYSKEYUP)){const bool down=w==WM_KEYDOWN||w==WM_SYSKEYDOWN;if(event->vkCode=='R'||event->vkCode=='F'){physicalPowerHeld[event->vkCode=='R'?0:1]=down;if(down&&inputBusy)inputCancelled=true;}if(inputObserver)inputObserver(event->vkCode,down);}}
 if(n==HC_ACTION){auto* e=(KBDLLHOOKSTRUCT*)l;int index=e->vkCode=='Q'?2:e->vkCode=='Z'?3:-1;
  if(!(e->flags&LLKHF_INJECTED)&&(w==WM_KEYDOWN||w==WM_SYSKEYDOWN)&&(index>=0||e->vkCode==VK_ESCAPE)){std::lock_guard<std::mutex> lock(mutex);if(e->vkCode==VK_ESCAPE){inputCancelled=true;cameraPreparation.pauseUntil(GetTickCount64()+500);cancelPress();}}
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
 physicalMouseMovement={};for(auto& owned:angleKeyOwned)owned=false;std::lock_guard<std::mutex> lock(mutex);clicks={};blockedPacketClick={};flickClicks={};flickRequest={};cancelPress();requested={};
}
inline void stop(bool stopTransport=true){inputCancelled=true;cameraPreparation.clear();enabled=false;lockedTarget=0;++revision;physicalClickAt=0;{std::lock_guard<std::mutex> lock(mutex);clicks.cancel();flickClicks.cancel();cancelPress();current={};current.reason="QB aim off";}if(stopTransport&&packetStop)packetStop();}
inline bool inputPermitted(const Frame& frame,bool cameraPanning=false){
 const bool cursorAvailable=(cameraPanning&&cameraPanOwned&&!physicalRightHeld)||freeCursor();
 return wheelMethod()&&(!packetMode||(packetInputReady&&packetInputReady()))&&frame.inputThrow&&!inputCancelled&&enabled&&running&&frame.revision==revision&&admitted()&&focused(frame.pid)&&cursorAvailable&&!physicalPowerHeld[0]&&!physicalPowerHeld[1]&&!(GetAsyncKeyState(VK_ESCAPE)&0x8000)&&!(GetAsyncKeyState(VK_CONTROL)&0x8000)&&!(GetAsyncKeyState(VK_MENU)&0x8000);
}
inline bool throwTrackingPermitted(const Frame& frame){
 // A throw animation may hide/lock the cursor or remove the power GUI. Those
 // are engine transitions, not cancellation of the already dispatched throw.
 return wheelMethod()&&!cursorOnly&&enabled&&running&&!inputCancelled&&frame.revision==revision&&admitted()&&focused(frame.pid)&&!physicalRightHeld&&
  !(GetAsyncKeyState(VK_ESCAPE)&0x8000)&&!(GetAsyncKeyState(VK_CONTROL)&0x8000)&&!(GetAsyncKeyState(VK_MENU)&0x8000);
}
inline bool powerKeyEdge(unsigned key,bool down){
 const auto scan=MapVirtualKeyW(key,MAPVK_VK_TO_VSC);if(!scan)return false;
 INPUT event{};event.type=INPUT_KEYBOARD;event.ki.wScan=WORD(scan);event.ki.dwFlags=KEYEVENTF_SCANCODE|(down?0:KEYEVENTF_KEYUP);return SendInput(1,&event,sizeof(INPUT))==1;
}
inline bool samePowerContext(const Frame& request,const Frame& sample){
 return sample.held&&request.pid&&request.pid==sample.pid&&request.model==sample.model&&request.own&&request.own==sample.own&&request.tool&&request.tool==sample.tool&&request.powerGui&&request.powerGui==sample.powerGui&&request.window==sample.window&&request.revision==sample.revision;
}
inline bool powerControlReady(const Frame& request,const Frame& sample){
 return samePowerContext(request,sample)&&sample.wheelVisible&&ff3qb::powerGridValid(5,sample.gamePower)&&!sample.menuOpen&&!sample.textFocus&&now()>=sample.at&&now()-sample.at<.1&&inputPermitted(sample);
}
inline bool prepareInputPower(Reader& reader,Frame& fresh,const Frame& request,int power,bool previewStep=false){
 fresh=reader.read(0,0,false,true);
 if(!powerControlReady(request,fresh)){record(fresh,("Throw cancelled: power control unavailable: "+fresh.reason).c_str());return false;}
 if(!calculatesPower()&&fresh.gamePower!=power){record(fresh,"Throw cancelled: game power changed");return false;}
 for(int taps=0;taps<19&&fresh.gamePower!=power;++taps){
  const auto step=ff3qb::powerKeyStep(int(fresh.gamePower),power);if(!step.valid||!step.key){record(fresh,"Throw cancelled: power is unreachable in five-point steps");return false;}
  if(physicalPowerHeld[step.key=='R'?0:1]||(GetAsyncKeyState(step.key)&0x8000)){record(fresh,"Throw cancelled: physical power key is held");return false;}
  ff3qb::PowerKeyPress press(powerKeyEdge,[&](){return physicalPowerHeld[step.key=='R'?0:1].load();});
  {std::lock_guard<std::mutex> lock(mutex);if(flickClicks.pending||flickActive||inputCancelled)return false;
   // An in-flight R/F event can change power before the next full planner
   // sample. Do not advertise the old-power beam as an armed click plan.
   if(previewStep){current.ready=false;current.reason="Updating power; waiting for the game wheel";}}
  if(!inputPermitted(fresh)||!press.begin(step.key)){record(fresh,"Throw cancelled: power key-down was rejected");return false;}
  const auto previous=fresh.gamePower;const auto began=now();bool acknowledged=false;
  const auto action="Power "+std::string(1,char(step.key))+" down: "+std::to_string(int(previous))+" -> "+std::to_string(step.expected);record(fresh,action.c_str());
  while(running&&now()-began<.25){
   Sleep(4);fresh=reader.read(0,0,false,true);
   if(!powerControlReady(request,fresh)){record(fresh,("Throw cancelled: power context changed: "+fresh.reason).c_str());return false;}
   const auto result=ff3qb::powerStepResult(previous,step.expected,fresh.gamePower,now()-began);
   if((result==ff3qb::PowerStepResult::Acknowledged||now()-began>=.04)&&!press.release()){record(fresh,"Throw cancelled: power key-up was rejected");return false;}
   if(result==ff3qb::PowerStepResult::Acknowledged){acknowledged=true;break;}
   if(result==ff3qb::PowerStepResult::Unexpected){record(fresh,("Throw cancelled: unexpected wheel value "+std::to_string(int(fresh.gamePower))+"; expected "+std::to_string(step.expected)).c_str());return false;}
  }
  if(!acknowledged){record(fresh,"Throw cancelled: power wheel did not acknowledge R/F");return false;}
  // Refresh Smart Fit and the beam between pre-click power steps. A full
  // multi-step operation is reserved for an already captured throw click.
  if(previewStep){cameraPreparation.pauseUntil(GetTickCount64()+20);return true;}
  // Keep separate presses distinct to frame-polled controls. This gap never
  // sends another input and is only needed before another five-point step.
  if(fresh.gamePower!=power){Sleep(20);if(physicalPowerHeld[step.key=='R'?0:1]||!inputPermitted(fresh)){record(fresh,"Throw cancelled: power adjustment interrupted");return false;}}
 }
 if(fresh.gamePower==power){
  record(fresh,"Five-point wheel power acknowledged");fresh=reader.read(request.target,power);
  if(!sameIdentity(request,fresh)||!eligible(fresh,now())||!inputPermitted(fresh)){record(fresh,("Throw cancelled: power ready; trajectory unavailable: "+fresh.reason).c_str());return false;}
  return true;
 }
 record(fresh,"Throw cancelled: power step budget exceeded");return false;
}
inline bool directAimIdentity(ff3qb::DirectAimMemory& memory,const Frame& request){
 const auto h=memory.handle();const auto session=gameModules::router.get();
 return h&&(flickDispatched?throwTrackingPermitted(request):inputPermitted(request))&&session.pid==request.pid&&session.model==request.model&&request.camera&&request.mouseService&&request.mouseInput&&request.localPlayer&&
  rd<addr>(h,request.localPlayer+Offsets::Player::ModelInstance)==request.own&&rd<addr>(h,request.workspace+Offsets::Workspace::CurrentCamera)==request.camera&&
  rd<addr>(h,request.mouseService+Offsets::MouseService::InputObject)==request.mouseInput&&defense::classOf(h,request.camera)=="Camera"&&defense::classOf(h,request.mouseInput)=="InputObject";
}
inline bool applyDirectAim(ff3qb::DirectAimMemory& memory,const Frame& request,const Frame& frame,POINT pixel){
 if(frame.camera!=request.camera||frame.mouseInput!=request.mouseInput||frame.origin.x!=request.origin.x||frame.origin.y!=request.origin.y||frame.client.right!=request.client.right||frame.client.bottom!=request.client.bottom)return false;
 const auto pose=ff3qb::directAimPose(frame.cameraRotation,frame.matrix,frame.cameraPosition,frame.head,frame.solution.velocity,{double(pixel.x-frame.origin.x),double(pixel.y-frame.origin.y),true},frame.client.right,frame.client.bottom,frame.gameMouse.z);
 return memory.apply(request.camera+Offsets::Camera::Rotation,request.mouseInput+Offsets::MouseService::MousePosition,pose,[&](){return directAimIdentity(memory,request);});
}
inline bool prepareDirectAim(Reader& reader,ff3qb::DirectAimMemory& memory,Frame& fresh,Frame& request,int power,const std::function<void()>& pump={}){
 const POINT pixel=request.cursor;
 if(!sameIdentity(request,fresh)||!inGameClient(fresh,pixel)||powerWheelHit(fresh,pixel)||!inputPermitted(fresh)){record(fresh,"Throw cancelled: click position or camera unavailable");return false;}
 ff3qb::CursorAimTracker tracking;const auto began=now();
 while(running&&now()-began<.35){
  if(pump)pump();
  fresh=reader.read(request.target,power);
  if(!sameIdentity(request,fresh)||!eligible(fresh,now())||!inputPermitted(fresh)||!refreshAimBinding(request,fresh)||!directAimIdentity(memory,request))break;
  if(!applyDirectAim(memory,request,fresh,pixel)){record(fresh,"Throw cancelled: camera or cursor write unavailable");return false;}
  Sleep(2);auto verify=reader.read(request.target,power);POINT actual{};GetCursorPos(&actual);
  if(!sameIdentity(request,verify)||!eligible(verify,now())||!inputPermitted(verify))break;
  const ff3qb::Screen windows{double(actual.x-verify.origin.x),double(actual.y-verify.origin.y),true},game{verify.gameMouse.x,verify.gameMouse.y,ff3qb::finite(verify.gameMouse)};
  // A stored-field readback is insufficient. The renderer must project the
  // current launch ray under the real cursor for two acknowledged samples.
  if(tracking.update(verify.aim,windows,game,now())&&dispatchConsistent(request,verify,verify,actual)){
   fresh=verify;record(verify,"Direct camera and cursor acknowledged");return true;
  }
 }
 record(fresh,"Throw cancelled: rendered camera did not acknowledge the throw ray");return false;
}
inline void flickThrow(Reader& reader,Frame request,double clickedAt){
 struct Scope{
  Scope(const Frame& f){inputBusy=true;inputCancelled=false;cameraPixelX=f.cursor.x;cameraPixelY=f.cursor.y;}
  ~Scope(){flickDispatched=false;inputBusy=false;flickActive=false;}
 } scope(request);
 ff3qb::DirectAimMemory memory(request.pid);ff3qb::AimViewSnapshot saved;Frame fresh=request;
 const int power=int(std::lround(request.power));int cameraType=-1;addr cameraSubject=0,savedInput=0;double sentAt=clickedAt;std::vector<std::uint64_t> prior;
 std::string failure="Aim unavailable: aim preparation interrupted";
 auto sameCameraObject=[&](){
  const auto h=memory.handle();const auto session=gameModules::router.get();
  return h&&WaitForSingleObject(h,0)==WAIT_TIMEOUT&&session.pid==request.pid&&session.model==request.model&&session.version&&
   rd<addr>(h,request.localPlayer+Offsets::Player::ModelInstance)==request.own&&rd<addr>(h,request.workspace+Offsets::Workspace::CurrentCamera)==request.camera&&
   defense::classOf(h,request.camera)=="Camera";
 };
 auto sameCamera=[&](){return sameCameraObject()&&rd<int>(memory.handle(),request.camera+Offsets::Camera::CameraType,-2)==cameraType&&rd<addr>(memory.handle(),request.camera+Offsets::Camera::CameraSubject)==cameraSubject;};
 auto stationary=[&](){POINT point{};return GetCursorPos(&point)&&std::hypot(double(point.x-request.cursor.x),double(point.y-request.cursor.y))<=2;};
 ff3qb::FlickActions actions;
 actions.allowed=[&](){if(!throwTrackingPermitted(request)){failure="Aim unavailable: user/session control changed";return false;}return true;};
 actions.power=[&](){
  // The physical click already started the animation. Never change power or
  // require the equipped tool/power GUI to survive that click.
  fresh=request;failure="Aim unavailable: committed physical plan is invalid";
  if(!request.committed||!request.solution.valid||request.power!=request.gamePower)return false;
  const auto input=rd<addr>(memory.handle(),request.mouseService+Offsets::MouseService::InputObject);
  if(!input||defense::classOf(memory.handle(),input)!="InputObject"){failure="Aim unavailable: current cursor event is missing";return false;}
  request.mouseInput=fresh.mouseInput=input;
  request.gameMouse=fresh.gameMouse=vector(rd<shooting::Vec>(memory.handle(),input+Offsets::MouseService::MousePosition,{NAN,NAN,NAN}));
  if(!ff3qb::finite(request.gameMouse))return false;
  prior.clear();for(const auto& ball:request.looseBalls)prior.push_back(ball.id);for(const auto& ball:request.balls)prior.push_back(ball.id);
  record(fresh,"Physical throw input passed; tracking animation",true);return true;
 };
 actions.capture=[&](){
  const auto h=memory.handle();const auto oldInput=request.mouseInput;
  failure="Aim unavailable: current aim input unavailable after power preparation";
  if(!refreshAimBinding(request,fresh)||!directAimIdentity(memory,request))return false;
  savedInput=request.mouseInput;
  if(oldInput!=savedInput){const auto detail="Aim input refreshed "+std::to_string(oldInput)+" -> "+std::to_string(savedInput);record(fresh,detail.c_str());}
  cameraType=rd<int>(h,request.camera+Offsets::Camera::CameraType,-1);cameraSubject=rd<addr>(h,request.camera+Offsets::Camera::CameraSubject);
  failure="Aim unavailable: original view could not be saved";
  if(cameraType<0||!sameCamera()||!memory.captureView(request.camera+Offsets::Camera::Rotation,savedInput+Offsets::MouseService::MousePosition,saved))return false;
  return true;
 };
 actions.track=[&](){
  auto retained=fresh;Frame live;double nextObservation=0;
  while(running&&sameCameraObject()){
   const double elapsed=now()-sentAt;
   const bool observed=now()>=nextObservation;
   if(observed){live=reader.read(0,0,false,false,true);nextObservation=now()+.03;}
   const auto decision=ff3qb::throwReleaseDecision(elapsed,throwReleaseObserved(request,live,prior),!throwTrackingPermitted(request)||live.menuOpen||live.textFocus);
   if(decision==ff3qb::ReleaseDecision::Released){animationDelay=ff3qb::learnAnimationDelay(animationDelay.load(),elapsed,std::clamp(request.ping*.0005+.05,0.,.25));const auto detail="Throw release observed after "+std::to_string(int(elapsed*1000))+" ms; restoring view";record(retained,detail.c_str());break;}
   if(decision!=ff3qb::ReleaseDecision::Waiting){record(retained,decision==ff3qb::ReleaseDecision::TimedOut?"Throw release timeout; restoring view":"Throw interrupted by user/session change; restoring view");break;}
   // The animation may remove the tool/power wheel or replace cursor events.
   // These do not end an already sent throw and must not restore its view.
   const auto input=rd<addr>(memory.handle(),request.mouseService+Offsets::MouseService::InputObject);
   if(!input||defense::classOf(memory.handle(),input)!="InputObject"){Sleep(2);continue;}
   request.mouseInput=input;request.gameMouse=vector(rd<shooting::Vec>(memory.handle(),input+Offsets::MouseService::MousePosition,{NAN,NAN,NAN}));
   if(!ff3qb::finite(request.gameMouse)){Sleep(2);continue;}
   auto aimed=fresh;aimed.head=position(memory.handle(),child(memory.handle(),request.own,"Head"));
   // The user committed this fit at the click. Keep that route forecast/root
   // through the short animation instead of changing the advertised throw.
   aimed.cameraPosition=vector(rd<shooting::Vec>(memory.handle(),request.camera+Offsets::Camera::Position,{NAN,NAN,NAN}));
   aimed.cameraRotation=rd<ff3qb::AimRotation>(memory.handle(),request.camera+Offsets::Camera::Rotation);aimed.mouseInput=input;aimed.gameMouse=request.gameMouse;
   struct Matrix{float m[16];};const auto matrix=rd<Matrix>(memory.handle(),request.cameraMatrix);std::copy(std::begin(matrix.m),std::end(matrix.m),aimed.matrix);
   POINT pixel=request.cursor,actual{};if(GetCursorPos(&actual)&&inGameClient(request,actual))pixel=actual;
   if(applyDirectAim(memory,request,aimed,pixel))retained=aimed;
   if(observed){auto shown=fresh;shown.balls=live.balls;shown.looseBalls=live.looseBalls;shown.throwView=live.throwView;shown.releaseVisual=live.releaseVisual;shown.reason="Throw locked; waiting for actual ball release";publish(shown);}Sleep(2);
  }
 };
 actions.restore=[&](){
  // The throw itself may change camera subject/type. Restore after release
  // while the camera object is still ours; explicit right-button input wins.
  const bool restored=memory.restoreView(request.camera+Offsets::Camera::Rotation,savedInput+Offsets::MouseService::MousePosition,saved,
   [&](){return sameCameraObject()&&!physicalRightHeld;},[&](){return sameCameraObject()&&stationary()&&rd<addr>(memory.handle(),request.mouseService+Offsets::MouseService::InputObject)==savedInput;});
  if(restored){
   const double began=now();while(now()-began<.12&&sameCameraObject()){
    struct Matrix{float m[16];};const auto view=rd<Matrix>(memory.handle(),request.cameraMatrix);
    const auto x=ff3qb::aimUnit({view.m[0],view.m[1],view.m[2]}),y=ff3qb::aimUnit({view.m[4],view.m[5],view.m[6]}),z=ff3qb::aimUnit({-view.m[12],-view.m[13],-view.m[14]});
    if(now()-began>=.008&&ff3qb::dot(x,ff3qb::aimColumn(saved.camera.rotation,0))>.9999&&ff3qb::dot(y,ff3qb::aimColumn(saved.camera.rotation,1))>.9999&&ff3qb::dot(z,ff3qb::aimColumn(saved.camera.rotation,2))>.9999)break;Sleep(2);
   }
   DwmFlush();record(request,"Original view restored");
  }
  return restored;
 };
 const ff3qb::AnimatedAimActions animation{actions.allowed,actions.power,actions.capture,actions.restore,actions.track};
 if(!ff3qb::followPhysicalThrow(true,animation))record(fresh,failure.c_str());
 cameraPreparation.pauseUntil(GetTickCount64()+150);
}
inline void preparePreviewPower(Reader& reader,const Frame& request){
 // Power is the only pre-click input. Preserve the full trajectory for the
 // beam/HUD while the displayed wheel catches up with Smart Fit.
 {std::lock_guard<std::mutex> lock(mutex);if(flickClicks.pending||flickActive||inputCancelled||!cameraPreparation.canPrepare(GetTickCount64()))return;inputBusy=true;inputCancelled=false;}
 struct Scope{~Scope(){inputBusy=false;}} scope;
 auto fresh=request;prepareInputPower(reader,fresh,request,int(std::lround(request.preparationPower)),true);
}
inline void trackCursorAim(Reader& reader,Frame request){
 struct Scope{
  Scope(const Frame& f){cameraPreparation.clear();cameraPixelX=f.cursor.x;cameraPixelY=f.cursor.y;inputCancelled=false;inputBusy=true;}
  ~Scope(){std::function<bool(const Frame&)> old;{std::lock_guard<std::mutex> lock(mutex);old.swap(cursorClickStart);}cameraPreparation.clear();inputBusy=false;}
 } scope(request);
 const auto click=cameraPreparation.clicked();const int power=int(std::lround(request.power));
 const auto project=[](const Frame& f){return ff3qb::cursorAimPoint(f.head,f.cameraPosition,f.solution.velocity,f.matrix,f.client.right,f.client.bottom,f.gameMouse.z);};
 if(!ff3qb::finite(project(request))){auto display=request;display.ready=false;display.reason="Cursor test: turn toward the pass direction";publish(display);cameraPreparation.pauseUntil(GetTickCount64()+300);return;}
 auto display=request;display.ready=false;display.reason="Cursor test: preparing power before your click";publish(display);
 auto fresh=request;if(!prepareInputPower(reader,fresh,request,power)){if(!inputCancelled)cameraPreparation.pauseUntil(GetTickCount64()+750);return;}
 auto context=std::make_shared<CursorShot>(fresh);if(!context->available()){record(fresh,"Throw cancelled: cursor test context unavailable");cameraPreparation.pauseUntil(GetTickCount64()+750);return;}
 {std::lock_guard<std::mutex> lock(mutex);cursorClickStart=[context](const Frame& frame){return context->begin(frame);};}
 while(running&&inputPermitted(request)){
  flushInputTraces();flushCameraShots();
  if(cameraPreparation.clicked()!=click){
   const auto began=now();while(running&&now()-began<.25&&inputPermitted(request)&&context->pulse()){
    auto live=reader.read(request.target,power);live.ready=false;live.reason="Cursor test applied; verify the actual throw";publish(live);Sleep(2);
   }context->finish();return;
  }
  auto live=reader.read(request.target,power);
  if(!sameIdentity(request,live)||!eligible(live,now())||live.gamePower!=power)return;
  if(!ff3qb::finite(project(live))){cameraPreparation.clear();live.ready=false;live.reason="Cursor test: pass direction left the supported view";publish(live);return;}
  POINT cursor{};if(!GetCursorPos(&cursor)||std::hypot(double(cursor.x-request.cursor.x),double(cursor.y-request.cursor.y))>2){inputCancelled=true;cameraPreparation.pauseUntil(GetTickCount64()+150);return;}
  cameraPreparation.acknowledge(click);live.reason="Cursor test armed: click normally; camera stays unchanged";publish(live);Sleep(4);
 }
}
inline void worker(){
 Reader reader;
 while(running){
  flushInputTraces();flushPacketCancellations();flushCameraShots();expirePhysicalHold(GetTickCount64());
  Frame clicked;bool queued=false,taken=false;double clickedAt=0;
  {std::lock_guard<std::mutex> lock(mutex);if(flickClicks.pending){queued=true;clicked=flickRequest;clickedAt=flickClicks.at;taken=flickClicks.take(now(),enabled&&!packetMode&&!visualOnly&&!cursorOnly&&admitted());flickActive=taken;}}
  if(queued){if(taken)flickThrow(reader,clicked,clickedAt);else{flickDispatched=false;record(clicked,"Aim unavailable: click expired or aim was disabled");}continue;}
  if(studioRead){publish(studioRead());Sleep(8);continue;}
  if((!enabled&&!ballTrails&&!(extraVisuals&&extraVisuals()))||!admitted()){reader.flightTracker.clear();reader.throwTracker.clear();Frame idle;idle.reason=enabled?"Connect to Football Fusion 3":"QB aim off";publish(idle);{std::lock_guard<std::mutex> lock(mutex);clicks.cancel();cancelPress();}Sleep(30);continue;}
  auto f=reader.read();
  if(!inputBusy&&!flickActive&&cameraPreparation.canPrepare(GetTickCount64()))inputCancelled=false;
  if(enabled&&packetMode&&packetStatus&&f.ready){const auto transport=packetStatus();if(!transport.first){f.ready=false;f.reason=winrt::to_string(transport.second);}}
  if(!enabled){publish(f);Sleep(16);continue;}
  if(visualOnly){publish(f);Sleep(16);continue;}
  if(packetMode){
   // Dispatch retains its own click snapshot. Re-reading an already released
   // tool here used to erase assistance before the outgoing call arrived.
   publish(f);Sleep(8);continue;
  }
  if(cursorOnly){cameraPreparation.clear();f.ready=false;f.reason="Cursor Aim test unavailable: throw correction was not verified";publish(f);Sleep(16);continue;}
  if(f.ready){f.reason=f.gamePower==f.preparationPower?"Click to flick and throw to "+f.name:"Current-power throw ready; preparing faster route to "+f.name;}
  publish(f);
  if(f.preparationPower>=1&&f.preparationPower<=95&&f.preparationPower!=f.gamePower&&f.held&&!f.menuOpen&&!f.textFocus&&f.revision==revision&&now()>=f.at&&now()-f.at<.1&&inputPermitted(f)&&calculatesPower()&&cameraPreparation.canPrepare(GetTickCount64())&&!physicalPowerHeld[0]&&!physicalPowerHeld[1]&&!(GetAsyncKeyState(VK_ESCAPE)&0x8000)&&!(GetAsyncKeyState(VK_CONTROL)&0x8000)&&!(GetAsyncKeyState(VK_MENU)&0x8000))preparePreviewPower(reader,f);
  Sleep(8);
 }
}

inline json::Value inputNetworking(){using namespace basketballUi;auto out=object();out.o[L"type"]=string(L"qbNetworking");out.o[L"phase"]=string(L"input");out.o[L"mode"]=string(L"input");out.o[L"ready"]=basketballUi::boolean(true);out.o[L"editing"]=basketballUi::boolean(false);out.o[L"requiresRejoin"]=basketballUi::boolean(false);out.o[L"retryable"]=basketballUi::boolean(false);out.o[L"detail"]=string(L"Camera Aim keeps the beam and power live. Your real click starts the throw animation; aim follows until actual ball release, then restores the original view. Use third-person with shift-lock off.");return out;}
inline json::Value modules(){using namespace basketballUi;auto out=object(),m=object(),rows=object();m.o[L"on"]=basketballUi::boolean(enabled);rows.o[L"arc"]=number(arc);rows.o[L"preset"]=number(passMode);rows.o[L"angle"]=number(manualAngle);rows.o[L"radius"]=number(selectionRadius);rows.o[L"oob"]=number(boundaryHalfStuds.load()*.5);rows.o[L"oobVisual"]=basketballUi::boolean(oobVisual);rows.o[L"movement"]=number(aimMovementMs);rows.o[L"method"]=number(selectedMethod());rows.o[L"dive"]=number(diveReach);rows.o[L"lead"]=number(selectedLead());rows.o[L"height"]=number(selectedHeight());rows.o[L"beamLead"]=number(beamLeadDistance);rows.o[L"beamHeight"]=number(beamCatchHeight);rows.o[L"autoangle"]=basketballUi::boolean(automaticAngle);rows.o[L"fit"]=basketballUi::boolean(smartFit);rows.o[L"hud"]=basketballUi::boolean(showHud);rows.o[L"candidates"]=basketballUi::boolean(showCandidates);rows.o[L"power"]=basketballUi::boolean(autoPower);rows.o[L"force95"]=basketballUi::boolean(force95);rows.o[L"trails"]=basketballUi::boolean(ballTrails);rows.o[L"looseMarkers"]=basketballUi::boolean(looseMarkers);rows.o[L"practice"]=basketballUi::boolean(practice);rows.o[L"visuals"]=basketballUi::boolean(visuals);rows.o[L"receivers"]=basketballUi::boolean(showReceivers);rows.o[L"routes"]=basketballUi::boolean(showRoutes);rows.o[L"window"]=basketballUi::boolean(showWindow);rows.o[L"race"]=basketballUi::boolean(showRace);rows.o[L"threats"]=basketballUi::boolean(showThreats);rows.o[L"landing"]=basketballUi::boolean(landingMarkers);rows.o[L"future"]=basketballUi::boolean(futureMarkers);rows.o[L"highlight"]=basketballUi::boolean(ballHighlights);m.o[L"rows"]=rows;out.o[L"ff3_qb"]=m;return out;}
inline bool set(const std::wstring& key,const json::Value& v){
 if(key==L"ff3_qb.method"){
  if(v.type!=json::Value::Number||(v.n!=0&&v.n!=1&&v.n!=2))return false;
  if(int(v.n)==selectedMethod())return true;stop();packetMode=v.n==1;visualOnly=v.n==2;cursorOnly=v.n==3;return true;
 }
 if(key==L"ff3_qb.oobVisual"){if(v.type!=json::Value::Bool)return false;oobVisual=v.boolean();return true;}
 if(key==L"ff3_qb.oob"){if(v.type!=json::Value::Number||!ff3qb::validBoundaryMargin(v.n)||std::floor(v.n*2)!=v.n*2)return false;boundaryHalfStuds=int(v.n*2);++revision;return true;}
 if(key==L"ff3_qb.movement"){if(v.type!=json::Value::Number||!std::isfinite(v.n)||v.n<15||v.n>180||std::floor(v.n)!=v.n)return false;aimMovementMs=int(v.n);++revision;return true;}
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
 if(key==L"ff3_qb.force95"){force95=v.b;if(v.b){passMode=0;automaticAngle=true;}}else if(key==L"ff3_qb.autoangle"){automaticAngle=v.b;if(!v.b)force95=false;}else if(key==L"ff3_qb.fit")smartFit=v.b;else if(key==L"ff3_qb.hud")showHud=v.b;else if(key==L"ff3_qb.candidates")showCandidates=v.b;else if(key==L"ff3_qb.power"){autoPower=v.b;}else if(key==L"ff3_qb.practice")practice=v.b;else if(key==L"ff3_qb.visuals")visuals=v.b;else if(key==L"ff3_qb.trails")ballTrails=v.b;else if(key==L"ff3_qb.looseMarkers")looseMarkers=v.b;else if(key==L"ff3_qb.receivers")showReceivers=v.b;else if(key==L"ff3_qb.routes")showRoutes=v.b;else if(key==L"ff3_qb.window")showWindow=v.b;else if(key==L"ff3_qb.race")showRace=v.b;else if(key==L"ff3_qb.threats")showThreats=v.b;else if(key==L"ff3_qb.landing")landingMarkers=v.b;else if(key==L"ff3_qb.future")futureMarkers=v.b;else if(key==L"ff3_qb.highlight")ballHighlights=v.b;else return false;++revision;return true;
}
inline json::Value report(){using namespace basketballUi;auto f=snapshot();auto out=object();out.o[L"type"]=string(L"ff3Live");out.o[L"modules"]=modules();out.o[L"reason"]=string(widen(f.reason));if(enabled&&packetMode&&packetStatus){const auto state=packetStatus();out.o[L"setup"]=string(state.second);out.o[L"transportReady"]=basketballUi::boolean(state.first);out.o[L"setupFailed"]=basketballUi::boolean(packetRetryable&&packetRetryable());}out.o[L"target"]=string(widen(f.name));out.o[L"power"]=number(f.power);out.o[L"gamePower"]=number(f.gamePower);out.o[L"automaticPower"]=basketballUi::boolean(calculatesPower());out.o[L"held"]=basketballUi::boolean(f.held);out.o[L"wheel"]=basketballUi::boolean(f.wheelVisible);out.o[L"ready"]=basketballUi::boolean(f.ready);out.o[L"fitBlocked"]=basketballUi::boolean(f.fitBlocked);out.o[L"route"]=string(widen(f.route));out.o[L"preset"]=string(force95?L"Beam":widen(f.passLabel.empty()?ff3qb::presetName(f.mode):f.passLabel));out.o[L"distance"]=number(f.distance);out.o[L"locked"]=basketballUi::boolean(f.locked);out.o[L"coverageCount"]=number(f.coverageCount);out.o[L"coverageDetail"]=string(widen(f.coverageDetail));out.o[L"coverageKnown"]=basketballUi::boolean(f.coverageKnown);out.o[L"margin"]=number(f.margin);out.o[L"fitOffset"]=number(std::hypot(f.fitOffset.x,f.fitOffset.z));out.o[L"catchHeight"]=number(f.fitOffset.y+f.leadOffset.y);out.o[L"lead"]=number(selectedLead());out.o[L"force95"]=basketballUi::boolean(force95);out.o[L"balls"]=number(double(f.balls.size()));out.o[L"discoveredBalls"]=number(double(f.looseBalls.size()));out.o[L"discoveryOverflow"]=basketballUi::boolean(f.discoveryOverflow);out.o[L"angle"]=number(f.solution.angle);out.o[L"flight"]=number(f.solution.time);auto profile=array();if(f.ready){for(int i=0;i<=24;i++){auto p=ff3qb::point(f.solution,f.gravity,f.solution.time*i/24.);auto sample=array();sample.a.push_back(number(std::hypot(p.x-f.solution.origin.x,p.z-f.solution.origin.z)));sample.a.push_back(number(p.y-f.solution.origin.y));profile.a.push_back(sample);}}out.o[L"check"]=string(ff3qb::throwCheckLabel(ff3qb::throwCheck(f.hudCoverageKnown,f.hudReachEstimated,f.risk)));out.o[L"fieldKnown"]=basketballUi::boolean(f.field.valid());out.o[L"trajectory"]=profile;{std::lock_guard<std::mutex> lock(mutex);out.o[L"lastAction"]=string(now()-actionAt<3?widen(lastAction):L"");out.o[L"throws"]=number(throwCount);auto history=array();for(auto item:throwHistory.a){if(item[L"method"].num()!=double(selectedMethod()))continue;item.o[L"ageMs"]=number(std::max(0.,double(GetTickCount64())-item[L"at"].num()));history.a.push_back(item);}out.o[L"throwHistory"]=history;}return out;}
inline void load(const std::filesystem::path& file){auto cfg=basketballUi::readConfig(file);if(cfg[L"method"].type==json::Value::Number&&cfg[L"method"].n==3)cfg.o[L"method"]=basketballUi::number(2);if(cfg[L"oob"].type==json::Value::Number&&std::isfinite(cfg[L"oob"].n)&&cfg[L"oob"].n>=-10&&cfg[L"oob"].n<0)cfg.o[L"oob"]=basketballUi::number(0);force95=false;packetMode=false;visualOnly=false;cursorOnly=false;boundaryHalfStuds=4;oobVisual=true;looseMarkers=true;for(auto key:{L"method",L"arc",L"preset",L"power",L"practice",L"visuals",L"angle",L"radius",L"oob",L"oobVisual",L"movement",L"dive",L"autoangle",L"fit",L"hud",L"candidates",L"trails",L"looseMarkers",L"lead",L"height",L"beamLead",L"beamHeight",L"force95",L"receivers",L"routes",L"window",L"race",L"threats",L"landing",L"future",L"highlight"})if(cfg[key].type!=json::Value::Null)set(L"ff3_qb."+std::wstring(key),cfg[key]);enabled=false;}
inline bool save(const std::filesystem::path& file){auto data=modules()[L"ff3_qb"][L"rows"];data.o[L"lead"]=basketballUi::number(leadDistance);data.o[L"height"]=basketballUi::number(catchHeightAdjustment);auto temp=file;temp+=L".tmp";{std::ofstream out(temp,std::ios::binary);out<<winrt::to_string(json::dump(data));out.flush();if(!out)return false;}return MoveFileExW(temp.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;}
}
