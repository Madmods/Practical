#pragma once
#include <condition_variable>
namespace ff3Play {
struct FastSnapBinding {
 bool active=false,r6Palm=false;DWORD pid=0;addr model=0,own=0,player=0,workspace=0,humanoid=0,head=0,headPrimitive=0,hand=0,handPrimitive=0,gui=0,inputState=0,graphics=0;
 CatchPart ball;Vec handSize;double sceneAt=0,verifiedAt=0,range=0;std::vector<std::pair<addr,addr>> ancestors;
};
inline bool fastSnapFresh(const FastSnapBinding& b,double time){return b.active&&std::isfinite(time)&&std::isfinite(b.sceneAt)&&time>=b.sceneAt&&time-b.sceneAt<=.18;}
inline bool sameFastSnap(const FastSnapBinding& a,const FastSnapBinding& b){return a.pid==b.pid&&a.model==b.model&&a.own==b.own&&a.player==b.player&&a.ball.id==b.ball.id&&a.ball.primitive==b.ball.primitive&&a.ball.rig==b.ball.rig&&a.ball.mesh==b.ball.mesh&&a.range==b.range;}
struct FastSnapResult {bool attempted=false,written=false,readbackNear=false,renderedNear=false,motionWritten=false;addr motionPointer=0;Vec origin{NAN,NAN,NAN},destination{NAN,NAN,NAN},renderedPosition{NAN,NAN,NAN};double distance=INFINITY;};
struct FastSnapStats {
 unsigned writes=0,writeFailures=0,motionWrites=0;addr lastBall=0,motionPointer=0;double lastWriteAt=0,maximumIntervalMs=0,meanIntervalMs=0,lastDistance=INFINITY;unsigned intervals=0;
 bool readbackNear=false,renderedNear=false;Vec lastOrigin{NAN,NAN,NAN},lastDestination{NAN,NAN,NAN},renderedPosition{NAN,NAN,NAN};
};
struct FastSnapAccess {
 HANDLE h=nullptr;DWORD pid=0;~FastSnapAccess(){if(h)CloseHandle(h);}
 bool bind(DWORD next){if(pid!=next||h&&WaitForSingleObject(h,0)!=WAIT_TIMEOUT){if(h)CloseHandle(h);h=nullptr;pid=next;}if(!h)h=OpenProcess(PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_VM_OPERATION|SYNCHRONIZE,FALSE,next);return h!=nullptr;}
 static FastSnapBinding prepare(HANDLE h,const Scene& s,const SnapCommand& command,double range){
  FastSnapBinding b;if(!command.move||!observing(s,now())||s.possessed||!h||!std::isfinite(snapRadius(range)))return b;
  b.pid=s.pid;b.model=s.model;b.own=s.own;b.player=s.player;b.range=range;b.sceneAt=s.at;b.ball=command.ball.part;b.workspace=child(h,s.model,"Workspace");
  const auto& p=b.ball;if(!p.ball||!p.id||!p.primitive||!p.mesh||!instanceClassIs(h,p.id,"Part")||!path(h,p.id,{"Configuration","Gravity"})||!ff3Native::looseWorldBall(h,p.id,b.workspace)||rd<addr>(h,p.id+Offsets::BasePart::Primitive)!=p.primitive||rd<addr>(h,p.id+Offsets::Instance::Parent)!=p.rig||rd<addr>(h,p.mesh+Offsets::Instance::Parent)!=p.id)return {};
  const auto hand=HandSnapAccess::contact(h,s);if(!hand.valid(s))return {};b.hand=hand.id;b.handPrimitive=hand.primitive;b.handSize=hand.size;b.r6Palm=nm(h,b.hand)=="Right Arm";
  b.head=child(h,s.own,"Head");b.headPrimitive=rd<addr>(h,b.head+Offsets::BasePart::Primitive);b.humanoid=child(h,s.own,"Humanoid");
  b.gui=child(h,s.model,"GuiService");b.inputState=rd<addr>(h,child(h,s.model,"UserInputService")+Offsets::UserInputService::WindowInputState);
  if(!b.workspace||!b.head||!b.headPrimitive||!b.humanoid||!b.gui||!b.inputState)return {};
  addr ancestor=p.rig;for(int depth=0;ancestor&&ancestor!=b.workspace&&depth<18;++depth){const auto parent=rd<addr>(h,ancestor+Offsets::Instance::Parent);b.ancestors.push_back({ancestor,parent});ancestor=parent;}if(ancestor!=b.workspace)return {};
  if(ff3qb::finite(snapRenderedPosition(h,p)))b.graphics=rd<addr>(h,p.id+0x180);
  b.verifiedAt=now();b.active=true;return b;
 }
 bool identities(const FastSnapBinding& b,double time){
  if(!fastSnapFresh(b,time)||!running||!magsOn||ff3Native::inputBusy||magsTeleportSize()!=b.range||!ff3Native::admitted()||!ff3Native::focused(b.pid)||!h||WaitForSingleObject(h,0)!=WAIT_TIMEOUT)return false;
  const auto health=rd<float>(h,b.humanoid+Offsets::Humanoid::Health,NAN);if(rd<addr>(h,b.player+Offsets::Player::ModelInstance)!=b.own||rd<addr>(h,b.humanoid+Offsets::Instance::Parent)!=b.own||!std::isfinite(health)||health<=0||rd<unsigned char>(h,b.gui+Offsets::GuiService::MenuIsOpen,255)!=0||rd<addr>(h,b.inputState+Offsets::WindowInputState::CurrentTextBox,~addr(0))!=0)return false;
  for(const auto& link:b.ancestors)if(rd<addr>(h,link.first+Offsets::Instance::Parent)!=link.second)return false;
  const auto& p=b.ball;return rd<addr>(h,p.id+Offsets::Instance::Parent)==p.rig&&rd<addr>(h,p.id+Offsets::BasePart::Primitive)==p.primitive&&rd<addr>(h,p.primitive+Offsets::Primitive::Owner)==p.id&&rd<addr>(h,p.mesh+Offsets::Instance::Parent)==p.id&&rd<addr>(h,b.head+Offsets::Instance::Parent)==b.own&&rd<addr>(h,b.head+Offsets::BasePart::Primitive)==b.headPrimitive&&rd<addr>(h,b.hand+Offsets::Instance::Parent)==b.own&&rd<addr>(h,b.hand+Offsets::BasePart::Primitive)==b.handPrimitive;
 }
 SnapContact contact(const FastSnapBinding& b,Scene& scene){
  scene.own=b.own;scene.head=ff3Native::vector(rd<shooting::Vec>(h,b.headPrimitive+Offsets::Primitive::Position,{NAN,NAN,NAN}));
  return {b.hand,b.handPrimitive,b.own,readSnapPalmPosition(h,b.handPrimitive,b.handSize,b.r6Palm),ff3Native::vector(rd<shooting::Vec>(h,b.handPrimitive+Offsets::Primitive::AssemblyLinearVelocity,{NAN,NAN,NAN})),b.handSize,false};
 }
 FastSnapResult step(const FastSnapBinding& b){
  FastSnapResult out;if(!bind(b.pid)||!identities(b,now()))return out;Scene scene;auto hand=contact(b,scene);if(!hand.valid(scene))return out;
  out.origin=snapTransferOrigin(h,b.ball.primitive);out.destination=snapHandTarget(hand);out.distance=snapDistance(out.origin,hand.position);if(!withinSnapDistance(out.origin,hand.position,b.range))return out;
  auto guard=[&](){if(!identities(b,now()))return false;Scene current;const auto live=contact(b,current);const auto size=ff3Native::vector(rd<shooting::Vec>(h,b.ball.primitive+Offsets::Primitive::Size,{NAN,NAN,NAN}));return live.valid(current)&&ff3qb::finite(size)&&std::min({size.x,size.y,size.z})>.1&&std::max({size.x,size.y,size.z})<=maxMagSize&&withinSnapDistance(snapWorldPosition(h,b.ball.primitive),live.position,b.range)&&snapDistance(out.destination,live.position)<.75;};
  out.attempted=true;const auto result=writeSnapFields(h,b.ball.primitive,out.destination,hand.velocity,guard,b.range);out.written=result.written;out.readbackNear=result.readbackNear;out.motionWritten=result.motionWritten;out.motionPointer=result.motionPointer;
  if(b.graphics&&rd<addr>(h,b.ball.id+0x180)==b.graphics)out.renderedPosition=ff3Native::vector(rd<shooting::Vec>(h,b.graphics+0xac,{NAN,NAN,NAN}));
  out.renderedNear=ff3qb::finite(out.renderedPosition)&&snapDistance(out.renderedPosition,hand.position)<1;return out;
 }
};
class FastHandSnap {
public:
 using Action=std::function<FastSnapResult(const FastSnapBinding&)>;
private:
 FastSnapAccess access;Action action;mutable std::mutex mutex;std::condition_variable changed;FastSnapBinding binding;FastSnapStats stats;bool alive=true;std::thread thread;
 void loop(){PreciseWait wait;for(;;){double began=now();{std::unique_lock<std::mutex> lock(mutex);changed.wait(lock,[&](){return !alive||binding.active;});if(!alive)return;if(!fastSnapFresh(binding,now())){binding.active=false;continue;}
   began=now();const auto result=action(binding);if(result.attempted){stats.lastOrigin=result.origin;stats.lastDestination=result.destination;stats.lastDistance=result.distance;stats.readbackNear=result.readbackNear;stats.renderedNear=result.renderedNear;stats.renderedPosition=result.renderedPosition;stats.motionPointer=result.motionPointer;if(result.motionWritten)++stats.motionWrites;
    if(result.written){const double at=now();if(stats.lastBall==binding.ball.id&&stats.lastWriteAt>0&&at-stats.lastWriteAt<.18){const double interval=(at-stats.lastWriteAt)*1000;stats.maximumIntervalMs=std::max(stats.maximumIntervalMs,interval);stats.meanIntervalMs+=(interval-stats.meanIntervalMs)/++stats.intervals;}stats.lastBall=binding.ball.id;stats.lastWriteAt=at;++stats.writes;}else ++stats.writeFailures;}}
   const auto remaining=.002-(now()-began);wait.milliseconds(DWORD(std::max(1.,std::round(remaining*1000))));
  }}
public:
 explicit FastHandSnap(Action custom={}):action(custom?std::move(custom):Action([this](const FastSnapBinding& b){return access.step(b);})),thread([this](){loop();}){}
 ~FastHandSnap(){{std::lock_guard<std::mutex> lock(mutex);alive=false;binding.active=false;}changed.notify_all();if(thread.joinable())thread.join();}
 FastHandSnap(const FastHandSnap&)=delete;FastHandSnap& operator=(const FastHandSnap&)=delete;
 void submitBinding(const FastSnapBinding& next){std::lock_guard<std::mutex> lock(mutex);if(!binding.active||!sameFastSnap(binding,next)){stats.lastWriteAt=0;stats.maximumIntervalMs=stats.meanIntervalMs=0;stats.intervals=0;}binding=next;changed.notify_one();}
 bool submit(HANDLE readHandle,const Scene& scene,const SnapCommand& command,double range){
  if(!command.move||!observing(scene,now())||scene.possessed){clear();return false;}
  {std::lock_guard<std::mutex> lock(mutex);const auto& p=command.ball.part;if(binding.active&&binding.pid==scene.pid&&binding.model==scene.model&&binding.own==scene.own&&binding.player==scene.player&&binding.ball.id==p.id&&binding.ball.primitive==p.primitive&&binding.ball.mesh==p.mesh&&binding.ball.rig==p.rig&&binding.range==range&&now()-binding.verifiedAt<.1){binding.sceneAt=scene.at;return true;}}
  auto next=FastSnapAccess::prepare(readHandle,scene,command,range);const bool ok=next.active;submitBinding(next);return ok;
 }
 void clear(){std::lock_guard<std::mutex> lock(mutex);binding.active=false;stats.lastWriteAt=0;}
 FastSnapStats snapshot()const{std::lock_guard<std::mutex> lock(mutex);return stats;}
};
}
