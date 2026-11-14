#pragma once
#include <windows.h>
#include <tlhelp32.h>
#include <atomic>
#include <algorithm>
#include <vector>
#include <deque>
#include <string>
#include <cmath>
#include <chrono>
#include <thread>
#include <limits>
#include "../../build-offsets.hpp"
#include "shooting_logic.hpp"
#include "timing_reviewer.hpp"
#include "precise_wait.hpp"
#include "pump_logic.hpp"
#include "defense_settings.hpp"
#include "pump_ground.hpp"
#include <mutex>
#include <map>
#include <fstream>
using addr=uintptr_t;
#include "connection.hpp"
#include "game_modules.hpp"
#include "basketball_connection.hpp"
#include "shot_bindings.hpp"
inline std::atomic<bool> running{true},armed{false},linked{false},meterFound{false},livePing{false};
inline std::atomic<int> trim{40},pingMs{30},fillPct{0},releaseCount{0},lastPct{-1};
inline std::atomic<int> shootKey{'E'},shootKeyAlt{VK_SPACE},activeShotKey{'E'};
inline std::atomic<bool> sessionUnlocked{true};
inline bool isShotKey(int k){return k>0&&(k==shootKey.load()||k==shootKeyAlt.load());}
inline ShotBindingRouter shotRouter;   // hold key, rebindable from the UI
inline double now(){using namespace std::chrono;return duration<double>(steady_clock::now().time_since_epoch()).count();}
template<class T>T rd(HANDLE h,addr a,T fail={}){T v{};SIZE_T n=0;return a>65535&&ReadProcessMemory(h,(void*)a,&v,sizeof v,&n)&&n==sizeof v?v:fail;}
inline std::string nm(HANDLE h,addr o){auto a=rd<addr>(h,o+Offsets::Instance::NameContainer),n=rd<addr>(h,a+16);if(!n||n>120)return {};if(rd<addr>(h,a+24)>=16)a=rd<addr>(h,a);char s[121]{};SIZE_T z;return ReadProcessMemory(h,(void*)a,s,n,&z)&&z==n?std::string(s,n):"";}
inline addr child(HANDLE h,addr o,const std::string& s){auto t=rd<addr>(h,o+Offsets::Instance::ChildrenStart),a=rd<addr>(h,t),b=rd<addr>(h,t+8);if(!a||b<a||b-a>160000)return 0;for(;a<b;a+=16){auto v=rd<addr>(h,a);if(nm(h,v)==s)return v;}return 0;}
inline addr path(HANDLE h,addr o,std::initializer_list<std::string> names){for(auto&s:names){if(!o)return 0;o=child(h,o,s);}return o;}
inline double median(std::vector<double> a){std::sort(a.begin(),a.end());return (a[(a.size()-1)/2]+a[a.size()/2])/2;}
inline std::atomic<bool> assist{false},holdEarly{true},protectFall{true},ceilingOn{false},closeoutOn{false};
inline std::atomic<int> minRelease{80},maxRelease{100},maxPing{1000},freshnessMs{120},contestLimit{30},overholdLeadMs{40},statusCode{0};
inline std::atomic<double> contestNow{-1},contestFuture{-1},contestAt{-10},sampleAt{-10},progress{0},meterSlope{0},effectiveLead{0};
inline std::atomic<DWORD> gamePid{0};
inline std::atomic<bool> physicalDown{false},gameDown{false},pendingUp{false},manualDone{false},blockDown{false},activeMeter{false},fallingMeter{false};
inline std::atomic<unsigned> pressSerial{0};
inline std::atomic<int> lastReleaseReason{0};
struct TimingRelease{unsigned count=0;shooting::Release reason=shooting::Release::None;int percent=0,ping=0,trim=0;double lead=0,rate=0,span=0;size_t samples=0;double minimum=shooting::BasketballAutoMinimum,maximum=shooting::BasketballAutoMaximum;int windowPing=0;addr owner=0;unsigned press=0;double started=0,sent=0;};
inline std::mutex timingReleaseMutex;inline TimingRelease timingRelease;
inline TimingRelease lastTimingRelease(){std::lock_guard<std::mutex> lock(timingReleaseMutex);return timingRelease;}
struct TimingWindow {shooting::AutoWindow bounds;int ping=0;double rate=0;};
inline TimingWindow timingWindow;
inline TimingWindow currentTimingWindow(){std::lock_guard<std::mutex> lock(timingReleaseMutex);return timingWindow;}
inline void publishTimingWindow(const TimingWindow& window){std::lock_guard<std::mutex> lock(timingReleaseMutex);timingWindow=window;}
inline std::atomic<bool> reviewOn{true};
inline std::mutex reviewMutex;inline timingReview::Reviewer reviewer;
inline void resetReviewer(){std::lock_guard<std::mutex> lock(reviewMutex);reviewer.reset();}
inline timingReview::View reviewerView(){std::lock_guard<std::mutex> lock(reviewMutex);return reviewer.view(now());}
inline bool reviewerFeedback(unsigned id,timingReview::Grade grade){
 if(!reviewOn||!armed||!sessionUnlocked||gameDown)return false;
 std::lock_guard<std::mutex> lock(reviewMutex);return reviewer.baselineIs(trim.load())&&reviewer.feedback(id,grade,now());
}
inline std::atomic<bool> ballArcOn{true},ballLandingOn{true},rollingOn{true};
inline std::atomic<bool> pumpOn{false},pumpVisuals{true},pumpOwned{false};
inline std::atomic<int> pumpRange{12},pumpDelay{120},pumpContest{10},pumpStatus{pump::Off};
inline std::atomic<addr> heldBall{0},nearTarget{0},tapBall{0},tapTarget{0};
inline std::atomic<double> sceneAt{-10},nearHeight{0},nearVertical{0},tapHeight{0},tapAt{-10},physicalAt{-10};
inline std::atomic<bool> tapGrounded{false};inline std::atomic<unsigned> tapSerial{0},pumpRequest{0};
inline std::atomic<double> pumpRequestAt{-10},pumpSentAt{-10};inline std::atomic<addr> pumpRequestBall{0},pumpSentTarget{0};
struct PumpVisual {double at=0;bool valid=false;addr ownPrimitive=0,targetPrimitive=0;std::vector<std::pair<float,float>> ownRing,targetRing;std::pair<float,float> ownPoint{},targetPoint{};int state=0;bool targetVisible=false,jumpConfirmed=false,shotSent=false;std::pair<float,float> targetMin{},targetMax{};float gap=0,ownGround=NAN,targetGround=NAN;std::vector<std::pair<float,float>> ownInner,targetInner,groundLink;std::vector<std::vector<std::pair<float,float>>> ownAccents,targetAccents;};
inline std::mutex pumpVisualMutex;inline PumpVisual pumpVisual;
inline HHOOK keyHook=nullptr,mouseHook=nullptr;
inline bool gameFocused(){DWORD p=0;GetWindowThreadProcessId(GetForegroundWindow(),&p);return sessionUnlocked&&p&&p==gamePid.load()&&(!basketballConnection::required||(basketballConnection::entered&&basketballConnection::releaseCurrent&&basketballConnection::state.readyAt(now())&&gameModules::router.active(now())==gameModules::Id::Basketball));}
inline bool nativeShotInput(int sk,bool down){INPUT in{};if(sk==VK_XBUTTON1||sk==VK_XBUTTON2||sk==VK_MBUTTON){in.type=INPUT_MOUSE;in.mi.dwFlags=sk==VK_MBUTTON?(down?MOUSEEVENTF_MIDDLEDOWN:MOUSEEVENTF_MIDDLEUP):(down?MOUSEEVENTF_XDOWN:MOUSEEVENTF_XUP);in.mi.mouseData=sk==VK_XBUTTON1?XBUTTON1:sk==VK_XBUTTON2?XBUTTON2:0;}else{in.type=INPUT_KEYBOARD;UINT sc=MapVirtualKeyW(sk,MAPVK_VK_TO_VSC_EX);in.ki.wScan=sc&255;in.ki.dwFlags=KEYEVENTF_SCANCODE|(down?0:KEYEVENTF_KEYUP)|((sc&0xff00)?KEYEVENTF_EXTENDEDKEY:0);}return SendInput(1,&in,sizeof in)==1;}
using ShotSender=bool(*)(int,bool);inline ShotSender shotSender=nativeShotInput;
inline bool sendShotInput(bool down){return shotSender(activeShotKey.load(),down);}
inline bool sendRelease(){return sendShotInput(false);}
// Only the selected shot input is observed. Injected events always pass through.
inline bool inputEdge(bool down){
 if(down){
  if(!gameFocused())return false;
  if(physicalDown.exchange(true))return blockDown.load()||!gameDown.load();
  if(pumpOwned.exchange(false))return true; // manual input takes ownership of the existing hold
  physicalAt=now();tapBall=heldBall.load();tapTarget=nearTarget.load();tapHeight=nearHeight.load();tapGrounded=std::abs(nearVertical.load())<2.5&&now()-sceneAt.load()<.15;
  if(pendingUp&&gameDown&&activeMeter){pendingUp=false;manualDone=false;return true;}
  manualDone=false;pendingUp=false;activeMeter=false;
  bool blocked=ceilingOn&&now()-contestAt.load()<.25&&contestNow>=contestLimit;
  blockDown=blocked;gameDown=!blocked;++pressSerial;
  if(blocked)statusCode=10;
  return blocked;
 }
 bool was=physicalDown.exchange(false);
 if(blockDown.exchange(false)){gameDown=false;return true;}
 if(!was)return false;
 if(pumpOn&&gameFocused()&&now()-physicalAt.load()<=.17&&now()-sceneAt.load()<.15&&heldBall&&heldBall==tapBall&&(!activeMeter||progress<.18)){
  tapAt=now();++tapSerial;manualDone=true;pendingUp=false;gameDown=false;return false;
 }
 if(gameFocused()&&gameDown&&shooting::deferEarlyRelease(assist,holdEarly,activeMeter,fallingMeter,progress,now()-sampleAt.load(),minRelease,freshnessMs)){pendingUp=true;statusCode=4;return true;}
 manualDone=true;pendingUp=false;gameDown=false;return false;
}
inline bool shotEdge(int key,bool down){
 auto edge=shotRouter.observe(key,down,gameFocused()&&(!gameDown||key==activeShotKey.load()));
 if(edge==ShotBindingRouter::Ignore)return false;
 if(edge==ShotBindingRouter::Start)activeShotKey=key;
 return inputEdge(down);
}
inline LRESULT CALLBACK keyboardProc(int n,WPARAM w,LPARAM l){if(n==HC_ACTION){auto*k=(KBDLLHOOKSTRUCT*)l;if(!(k->flags&LLKHF_INJECTED)){bool down=w==WM_KEYDOWN||w==WM_SYSKEYDOWN;bool edge=down&&!defense::held(k->vkCode);defense::physicalEdge(k->vkCode,down);if(edge&&gameFocused()){if(k->vkCode==VK_RETURN)defense::chat=!defense::chat.load();if(k->vkCode==VK_OEM_2)defense::chat=true;if(k->vkCode==VK_ESCAPE){defense::chat=false;defense::pick=false;}}if(isShotKey(k->vkCode)&&shotEdge(k->vkCode,down))return 1;}}return CallNextHookEx(keyHook,n,w,l);}
inline LRESULT CALLBACK mouseProc(int n,WPARAM w,LPARAM l){if(n==HC_ACTION){auto*m=(MSLLHOOKSTRUCT*)l;if(!(m->flags&LLMHF_INJECTED)){int key=0;bool down=false;if(w==WM_MBUTTONDOWN||w==WM_MBUTTONUP){key=VK_MBUTTON;down=w==WM_MBUTTONDOWN;}if(w==WM_XBUTTONDOWN||w==WM_XBUTTONUP){key=HIWORD(m->mouseData)==XBUTTON1?VK_XBUTTON1:VK_XBUTTON2;down=w==WM_XBUTTONDOWN;}if(w==WM_LBUTTONDOWN||w==WM_LBUTTONUP){key=VK_LBUTTON;down=w==WM_LBUTTONDOWN;}if(w==WM_RBUTTONDOWN||w==WM_RBUTTONUP){key=VK_RBUTTON;down=w==WM_RBUTTONDOWN;}if(key)defense::physicalEdge(key,down);if(key&&isShotKey(key)&&shotEdge(key,down))return 1;}}return CallNextHookEx(mouseHook,n,w,l);}
inline bool installInput(){keyHook=SetWindowsHookExW(WH_KEYBOARD_LL,keyboardProc,GetModuleHandleW(nullptr),0);mouseHook=SetWindowsHookExW(WH_MOUSE_LL,mouseProc,GetModuleHandleW(nullptr),0);return keyHook&&mouseHook;}
inline void uninstallInput(){if(keyHook)UnhookWindowsHookEx(keyHook);if(mouseHook)UnhookWindowsHookEx(mouseHook);if((pendingUp||pumpOwned)&&gameDown)sendRelease();keyHook=mouseHook=nullptr;}
inline std::vector<addr> children(HANDLE h,addr o){std::vector<addr> v;auto c=rd<addr>(h,o+Offsets::Instance::ChildrenStart),a=rd<addr>(h,c),b=rd<addr>(h,c+8);if(!a||b<a||b-a>160000)return v;for(;a<b;a+=16){auto p=rd<addr>(h,a);if(p)v.push_back(p);}return v;}
inline bool positionOf(HANDLE h,addr model,shooting::Vec&p,shooting::Vec&v){auto part=child(h,model,"HumanoidRootPart"),prim=rd<addr>(h,part+Offsets::BasePart::Primitive);if(!prim)return false;p=rd<shooting::Vec>(h,prim+Offsets::Primitive::Position,{NAN,NAN,NAN});v=rd<shooting::Vec>(h,prim+Offsets::Primitive::AssemblyLinearVelocity);if(!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z))v={};return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)&&fabs(p.x)<1e6&&fabs(p.y)<1e6&&fabs(p.z)<1e6;}
// Geometry estimate uses the calibrated curve. Character guard/court attributes are
// not mapped externally, so this is deliberately not presented as server contest.
inline void updateContest(HANDLE h,addr root,addr own,addr player){
 shooting::Vec p,v;if(!positionOf(h,own,p,v)){contestNow=-1;contestFuture=-1;contestAt=-10;return;}
 auto players=child(h,root,"Players");std::map<std::string,addr> teams;for(auto a:children(h,players))teams[nm(h,a)]=rd<addr>(h,a+Offsets::Player::Team);
 auto team=rd<addr>(h,player+Offsets::Player::Team);std::vector<shooting::Defender> list;
 for(auto model:children(h,path(h,root,{"Workspace","Characters"}))){if(model==own)continue;auto it=teams.find(nm(h,model));if(team&&it!=teams.end()&&it->second==team)continue;
  auto hum=child(h,model,"Humanoid");if(!hum||rd<float>(h,hum+Offsets::Humanoid::Health)<=0)continue;
  shooting::Vec pos,vel;if(!positionOf(h,model,pos,vel)||fabs(pos.y-p.y)>9||shooting::length(pos-p)>30)continue;
  list.push_back({pos,vel,16});
 }
 contestNow=shooting::estimate(p,v,list,0);contestFuture=shooting::estimate(p,v,list,.2);contestAt=now();
}
inline std::atomic<addr> sceneRoot{0},sceneOwn{0},scenePlayer{0};
// Read-only compatibility probe, independent of active shooting/tracking. Run on
// the reader worker, never on the window thread. No ball possession is required.
inline basketballConnection::Snapshot inspectBasketball(HANDLE h,addr base,DWORD pid){
 basketballConnection::Snapshot s;s.sampledAt=now();s.pid=pid;
 s.process=connectionProcessPresent;s.version=connectionVersionMatches;
 s.opened=h&&WaitForSingleObject(h,0)==WAIT_TIMEOUT;if(!s.opened)return s;
 const addr root=rd<addr>(h,rd<addr>(h,base+Offsets::FakeDataModel::Pointer)+Offsets::FakeDataModel::RealDataModel);
 s.model=root;s.placeId=rd<std::uint64_t>(h,root+Offsets::DataModel::PlaceId);s.universeId=rd<std::uint64_t>(h,root+Offsets::DataModel::GameId);
 // The legacy GameLoaded field is not a verified bool in this client (observed
 // value 31). Prove readiness from readable services and feature dependencies.
 const addr players=child(h,root,"Players"),ws=child(h,root,"Workspace");
 s.loaded=root&&players&&ws;
 if(!s.loaded||!gameModules::identify(s))return s;
 const addr player=rd<addr>(h,players+Offsets::Player::LocalPlayer);
 const auto who=nm(h,player);s.player=player&&!who.empty();
 if(gameModules::identify(s)->id==gameModules::Id::FootballFusion3){
  // Shared, read-only connection dependencies only. No Basketball-specific
  // object traversal or FF3 feature assumptions.
  s.world=ws!=0;s.renderer=rd<addr>(h,base+Offsets::VisualEngine::Pointer)!=0;
  if(rd<addr>(h,rd<addr>(h,base+Offsets::FakeDataModel::Pointer)+Offsets::FakeDataModel::RealDataModel)!=root||rd<std::uint64_t>(h,root+Offsets::DataModel::PlaceId)!=s.placeId||rd<std::uint64_t>(h,root+Offsets::DataModel::GameId)!=s.universeId||rd<addr>(h,players+Offsets::Player::LocalPlayer)!=player||WaitForSingleObject(h,0)!=WAIT_TIMEOUT){s.loaded=false;s.player=false;}
  s.sampledAt=now();return s;
 }
 const addr characters=child(h,ws,"Characters");
 s.world=ws&&characters&&child(h,ws,"Basketballs");
 const addr own=s.player?child(h,characters,who):0;s.character=own;
 shooting::Vec position,velocity;const addr hum=child(h,own,"Humanoid");
 const float health=rd<float>(h,hum+Offsets::Humanoid::Health,NAN);
 s.characterReady=own&&hum&&std::isfinite(health)&&health>0&&positionOf(h,own,position,velocity);
 const addr meter=path(h,own,{"HumanoidRootPart","BatMeter","Meter_Fill","FillGradient"});
 const float fill=-rd<float>(h,meter+0x12c,NAN);s.meter=meter&&std::isfinite(fill)&&fill>=0&&fill<=1.05f;
 const addr visual=rd<addr>(h,base+Offsets::VisualEngine::Pointer);
 struct Matrix{float m[16];};Matrix bad;for(auto& v:bad.m)v=NAN;
 const auto matrix=rd<Matrix>(h,visual+Offsets::VisualEngine::ViewMatrix,bad);
 bool finite=true,nonzero=false;for(auto v:matrix.m){finite=finite&&std::isfinite(v);nonzero=nonzero||fabs(v)>.00001f;}
 s.renderer=visual&&finite&&nonzero;
 // Reject a snapshot assembled across a teleport or process exit.
 if(rd<addr>(h,rd<addr>(h,base+Offsets::FakeDataModel::Pointer)+Offsets::FakeDataModel::RealDataModel)!=root||rd<std::uint64_t>(h,root+Offsets::DataModel::PlaceId)!=s.placeId||rd<std::uint64_t>(h,root+Offsets::DataModel::GameId)!=s.universeId||child(h,characters,who)!=own||WaitForSingleObject(h,0)!=WAIT_TIMEOUT){s.loaded=false;s.player=false;}
 s.sampledAt=now();return s;
}
// Geometry runs separately so scanning defenders cannot stall meter sampling.
inline void updatePump(HANDLE h,addr root,addr own,addr player,addr visualEngine);
inline std::atomic<addr> sceneVisualEngine{0};
inline void contestWorker(){HANDLE h=nullptr;DWORD attached=0;while(running){DWORD pid=gamePid.load();if(pid!=attached){if(h)CloseHandle(h);h=pid?OpenProcess(PROCESS_VM_READ|SYNCHRONIZE,FALSE,pid):nullptr;attached=pid;}
 if(h&&WaitForSingleObject(h,0)==WAIT_TIMEOUT){if(ceilingOn)updateContest(h,sceneRoot.load(),sceneOwn.load(),scenePlayer.load());if(pumpOn)updatePump(h,sceneRoot.load(),sceneOwn.load(),scenePlayer.load(),sceneVisualEngine.load());else{pumpStatus=pump::Off;heldBall=0;}}Sleep(pumpOn?20:50);}if(h)CloseHandle(h);}
inline void engine(){
 PreciseWait meterWait;
 HANDLE h=nullptr;DWORD pid=0;addr base=0,m=0,ping=0,root=0,own=0,player=0;
 double lookup=-10,net=-10,lastPing=-10,start=0,threatSince=-1,connectionProbe=-10;bool tracking=false,initialized=false,falling=false;unsigned seen=0;int failures=0;unsigned seenPump=0;double ownedSince=0;
 shooting::MeterTrend trend;auto& descent=trend.descent;shooting::PingFilter pingFilter;
 const double minimumPoll=meterWait.highResolution()?.006:.016;
 double lastMeterAt=-1,pollAhead=minimumPoll;
 timingReview::Tuning shotTuning;bool tuningCaptured=false,reviewWasActive=false;int shotBase=0;
 TimingWindow shotWindow;bool windowCaptured=false;
 auto stop=[&](int reason){if(pumpOwned.exchange(false)&&gameDown){sendRelease();gameDown=false;}tracking=false;activeMeter=false;fillPct=0;statusCode=reason;publishTimingWindow({});if(pendingUp&&gameDown){if(sendRelease()){pendingUp=false;gameDown=false;}else statusCode=12;}};
 while(running){double t=now();
  const bool reviewActive=reviewOn&&sessionUnlocked;
  if(reviewActive!=reviewWasActive){resetReviewer();reviewWasActive=reviewActive;}
  if(h&&WaitForSingleObject(h,0)!=WAIT_TIMEOUT){resetReviewer();stop(0);CloseHandle(h);h=nullptr;gamePid=0;linked=false;meterFound=false;livePing=false;m=ping=0;pingFilter.reset();pingMs=30;lastPing=-10;contestAt=-10;sceneRoot=0;sceneOwn=0;scenePlayer=0;sceneVisualEngine=0;heldBall=0;nearTarget=0;sceneAt=-10;connectionState=ConnectionState::Waiting;}
  if(!h){if(!connectRoblox(h,pid,base)){if(basketballConnection::required)gameModules::publish(inspectBasketball(nullptr,0,0));Sleep(350);continue;}gamePid=pid;lookup=-10;net=-10;connectionProbe=-10;}
  if(basketballConnection::required&&t-connectionProbe>=.25){
   const auto previous=basketballConnection::state.get();auto connection=inspectBasketball(h,base,pid);
   gameModules::publish(connection);connectionProbe=now();
   if(!basketballConnection::compatible(connection)||!basketballConnection::sameSession(previous,connection)){resetReviewer();stop(1);m=0;lookup=-10;}
  }
  // Stop all Basketball discovery and input work in other experiences. The
  // shared connection probe above continues, so routing can recover normally.
  if(basketballConnection::required&&!basketballConnection::compatible(basketballConnection::state.get())){
   if(tracking||pendingUp||pumpOwned)stop(1);
   sceneRoot=0;sceneOwn=0;scenePlayer=0;sceneVisualEngine=0;heldBall=0;nearTarget=0;sceneAt=-10;meterFound=false;livePing=false;linked=false;
   Sleep(10);continue;
  }
  if(!tracking&&t-lookup>1){root=rd<addr>(h,rd<addr>(h,base+Offsets::FakeDataModel::Pointer)+Offsets::FakeDataModel::RealDataModel);player=rd<addr>(h,child(h,root,"Players")+Offsets::Player::LocalPlayer);auto who=nm(h,player);linked=!who.empty();connectionState=linked?ConnectionState::Connected:ConnectionState::Loading;own=path(h,root,{"Workspace","Characters",who});m=path(h,own,{"HumanoidRootPart","BatMeter","Meter_Fill","FillGradient"});meterFound=m!=0;ping=path(h,root,{"Stats","PerformanceStats","Ping"});lookup=t;sceneRoot=root;sceneOwn=own;scenePlayer=player;sceneVisualEngine=rd<addr>(h,base+Offsets::VisualEngine::Pointer);}
  if(t-net>.5){net=t;struct Stat{double value;char label[16];};auto s=rd<Stat>(h,ping+0x98);s.label[15]=0;double x;char unit[16]{};if(s.value>0&&s.value<1000&&sscanf_s(s.label,"%lf %15s",&x,unit,16u)==2&&!strcmp(unit,"msec")&&fabs(x-s.value)<.051){pingFilter.add(t,x);pingMs=pingFilter.value();lastPing=t;if(reviewActive){std::lock_guard<std::mutex> lock(reviewMutex);reviewer.ping(t,(int)std::lround(x));}}livePing=t-lastPing<5;if(!livePing){pingMs=30;pingFilter.reset();}}
  if(pumpOwned&&(!pumpOn||!armed||!gameFocused()||!heldBall||heldBall!=pumpRequestBall||t-ownedSince>2.5)){stop(11);pumpStatus=pump::Expired;}
  if(seenPump!=pumpRequest.load()){seenPump=pumpRequest;bool ok=pumpOn&&armed&&gameFocused()&&heldBall&&heldBall==pumpRequestBall&&!physicalDown&&!gameDown&&m&&t-pumpRequestAt.load()<.15&&t-sceneAt.load()<.15;
   if(ok)activeShotKey=shootKey.load();
   if(ok&&sendShotInput(true)){manualDone=false;pendingUp=false;gameDown=true;blockDown=false;activeMeter=false;pumpOwned=true;ownedSince=t;pumpSentTarget=nearTarget.load();pumpSentAt=t;++pressSerial;}else pumpStatus=pump::Expired;
  }
  if(pendingUp&&!tracking)stop(11);
  if(!gameFocused()||(!armed&&!assist&&!ceilingOn)||!m){if(tracking||pendingUp)stop(1);Sleep(5);continue;}
  if(seen!=pressSerial.load()){seen=pressSerial;tracking=gameDown&&!blockDown;initialized=false;falling=false;fallingMeter=false;trend.reset();sampleAt=-10;threatSince=-1;start=t;failures=0;lastMeterAt=-1;pollAhead=minimumPoll;tuningCaptured=false;shotTuning={};windowCaptured=false;shotWindow={};publishTimingWindow({});statusCode=tracking?2:10;}
  if(tracking&&(manualDone||!gameDown)){stop(3);}
  if(tracking){
   double value=-rd<float>(h,m+0x12c,NAN);
   // Timestamp the meter observation after any discovery/compatibility work.
   // Using the loop's earlier timestamp makes the sample look older than it is.
   t=now();
   // Even a high-resolution wait is not a real-time guarantee. Preserve the
   // measured two-poll uncertainty budget; learned later corrections may only
   // remove spare margin above this budget, never the budget itself.
   if(lastMeterAt>=0)pollAhead=std::clamp(std::max(pollAhead,2*(t-lastMeterAt)+shooting::PollSeconds),minimumPoll,.1);
   lastMeterAt=t;
   if(t-start>3){stop(11);}
   else if(!std::isfinite(value)||value<0||value>1.05){if(initialized)stop(11);}
   else if(initialized||value<=.25){
    initialized=true;progress=value;fillPct=(int)round(value*100);
    if(trend.add(t,value))sampleAt=t;
    activeMeter=trend.active;falling=descent.falling;fallingMeter=falling;
    if(pendingUp&&(!assist||!holdEarly||t-sampleAt.load()>freshnessMs*.001)){stop(11);}
    else{
     if(activeMeter&&(trend.estimate.valid||(falling&&descent.rate<-.1))){double rate=falling?descent.rate:trend.estimate.rate;meterSlope=rate;
      if(!windowCaptured&&!falling&&trend.captureReady(pingMs.load())){
       shotWindow={shooting::pingWindow(pingMs.load(),rate,t-lastPing>=0&&t-lastPing<=2),pingMs.load(),rate};
       windowCaptured=true;publishTimingWindow(shotWindow);
      }
      bool tuned=false;
      if(reviewActive&&armed&&!falling&&windowCaptured){std::lock_guard<std::mutex> lock(reviewMutex);
       if(!tuningCaptured){shotBase=trim.load();shotTuning=reviewer.prepare(t,rate,shotBase,own);tuningCaptured=true;}
       tuned=shotBase==trim.load()&&reviewer.usable(shotTuning,t);
      }
      const int effectiveTrim=std::clamp(trim.load()+(tuned?shotTuning.adjustmentMs:0),-30,60);
      const int calibrationMs=tuned?shotTuning.windowShiftMs:0;
      auto effectiveWindow=shotWindow;
      effectiveWindow.bounds=shooting::calibratedWindow(shotWindow.bounds,shotWindow.rate,calibrationMs);
      publishTimingWindow(effectiveWindow);
      double lead=shooting::calibratedLead(.0192117+(pingMs+effectiveTrim)*.001,calibrationMs);effectiveLead=lead;
      shooting::Options o{armed.load(),assist.load(),holdEarly.load(),protectFall.load(),ceilingOn&&closeoutOn,minRelease.load(),maxRelease.load(),maxPing.load(),freshnessMs.load(),trim.load(),overholdLeadMs.load()};
      o.autoMaximum=effectiveWindow.bounds.maximum;o.autoMinimum=effectiveWindow.bounds.minimum;
      o.autoWindowValid=windowCaptured&&effectiveWindow.bounds.valid&&t-lastPing>=0&&t-lastPing<=2;
      o.calibrated=tuned;
      bool threat=t-contestAt.load()<.25&&std::max(contestNow.load(),contestFuture.load())>=contestLimit;
      if(threat){if(threatSince<0)threatSince=t;}else threatSince=-1;threat=threat&&t-threatSince>=.08;
      const double reviewGuard=shooting::reviewMargin(pollAhead,tuned?shotTuning.guardMs:0);
      auto decision=shooting::decide(o,{value,rate,t-sampleAt.load(),lead,pendingUp.load(),falling,threat,pingMs.load(),reviewGuard});
      if(decision!=shooting::Release::None){if(sendRelease()){lastPct=fillPct.load();lastReleaseReason=(int)decision;const auto count=++releaseCount;{std::lock_guard<std::mutex> lock(timingReleaseMutex);timingRelease={static_cast<unsigned>(count),decision,fillPct.load(),pingMs.load(),effectiveTrim,lead,rate,trend.estimate.span,trend.estimate.points,effectiveWindow.bounds.minimum,effectiveWindow.bounds.maximum,shotWindow.ping,own,seen,start,t};}
       if(reviewActive){auto applied=shotTuning;applied.eligible=tuned;std::lock_guard<std::mutex> lock(reviewMutex);reviewer.observe({static_cast<unsigned>(count),t,value,rate,pingMs.load(),decision==shooting::Release::Auto||decision==shooting::Release::AutoMaximum,decision==shooting::Release::AutoMaximum,applied,effectiveWindow.bounds.minimum,effectiveWindow.bounds.maximum,std::max(0.,(.016-pollAhead)*1000)});}
       pumpOwned=false;pendingUp=false;gameDown=false;tracking=false;activeMeter=false;statusCode=5;}else if(++failures>=3){stop(12);}}
      else statusCode=pendingUp?4:armed&&pingMs>maxPing?6:t-sampleAt.load()>freshnessMs*.001?7:falling?9:8;
     }
    }
   }
  }
  if(tracking)meterWait.milliseconds(static_cast<DWORD>(shooting::PollSeconds*1000));else Sleep(5);
 }
 if((pendingUp||pumpOwned)&&gameDown)sendRelease();if(h)CloseHandle(h);
}

#include "pump_scene.hpp"
#include "defense_runtime.hpp"
#include "shot_feedback_runtime.hpp"
