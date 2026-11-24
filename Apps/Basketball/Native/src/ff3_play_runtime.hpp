#pragma once
#include "ff3_qb_runtime.hpp"
#include "ff3_reach_edits.hpp"
#include "ff3_ball_edits.hpp"
#include "ff3_hand_snap.hpp"
#include "ff3_collision_groups.hpp"
#include <sstream>
namespace ff3Play {
inline std::atomic<bool> catchOn{false},jumpOn{false},rushOn{false},magsOn{false},holdRush{true};
inline std::atomic<bool> movementFrozen{false};
inline std::atomic<int> catchBinding{0},jumpBinding{VK_SPACE},rushBinding{VK_XBUTTON2},rushRange{150},reachTenths{200};
inline std::atomic<int> refreshMs{700},magDelayMs{0};
inline std::atomic<bool> catchRefresh{false},magsVisual{true},magsAutoCatch{false},magsPickup{false},magsFastPass{true};
inline std::atomic<int> magsCatchLeadMs{250},magsCatchHoldMs{140};
inline std::atomic<bool> navigationOwned{false};inline std::function<void()> extraStop;
inline std::array<std::atomic<unsigned char>,256> keyOwners{};
inline bool claimKey(int key,unsigned char owner){unsigned char expected=0;return keyOwners[key].compare_exchange_strong(expected,owner)||expected==owner;}
inline void releaseKey(int key,unsigned char owner){unsigned char expected=owner;keyOwners[key].compare_exchange_strong(expected,0);}
inline std::array<std::atomic<bool>,256> physical{};inline std::array<std::atomic<std::uint64_t>,256> pressSerial{};inline std::array<std::atomic<double>,256> pressTime{};inline std::mutex statusMutex;inline std::string status="Receiving and defense assists off";
inline std::filesystem::path recoveryPath,stopPath,diagnosticPath;inline bool recoveryPending=false;
struct BallVolumes {DWORD pid=0;addr model=0;double at=0;bool snap=false;std::uint64_t selected=0;std::vector<CatchPart> parts;std::map<std::uint64_t,double> additionalReach;};
inline std::mutex volumesMutex;inline BallVolumes volumes;
inline BallVolumes volumeSnapshot(){std::lock_guard<std::mutex> lock(volumesMutex);return volumes;}
inline void publishVolumes(const Scene& scene,const ReachEdits& edits,bool active){BallVolumes next;next.pid=scene.pid;next.model=scene.model;next.at=now();if(active)for(const auto& edit:edits.edits)next.parts.push_back(edit.original);std::lock_guard<std::mutex> lock(volumesMutex);volumes=std::move(next);}
inline void publishVolumes(const Scene& scene,const BallEdits& edits,bool active){BallVolumes next;next.pid=scene.pid;next.model=scene.model;next.at=now();if(active)for(const auto& edit:edits.edits)next.parts.push_back(edit.original.part);std::lock_guard<std::mutex> lock(volumesMutex);volumes=std::move(next);}
inline void publishBallGuides(const Scene& scene,const BallEdits& edits,const std::vector<BallPart>& loose,bool active){BallVolumes next;next.pid=scene.pid;next.model=scene.model;next.at=now();if(active){for(const auto& e:edits.edits){next.parts.push_back(e.original.part);next.additionalReach[e.original.part.id]=std::max(0.,.5*std::min({e.applied.x,e.applied.y,e.applied.z})-.5*std::max({e.original.part.size.x,e.original.part.size.y,e.original.part.size.z}));}for(const auto& b:loose){if(std::any_of(next.parts.begin(),next.parts.end(),[&](const auto& p){return p.id==b.part.id;}))continue;auto p=b.part;p.guide=true;next.parts.push_back(p);}}std::lock_guard<std::mutex> lock(volumesMutex);volumes=std::move(next);}
inline void publishSnapGuides(const Scene& scene,const std::vector<BallPart>& parts,bool active,std::uint64_t selected){BallVolumes next;next.pid=scene.pid;next.model=scene.model;next.at=now();next.snap=true;next.selected=selected;if(active)for(const auto& b:parts){auto observation=std::find_if(scene.looseBalls.begin(),scene.looseBalls.end(),[&](const auto& value){return value.id==b.part.id;});if(observation!=scene.looseBalls.end()&&observation->thrower&&(observation->thrower==scene.player||observation->thrower==scene.own))continue;auto part=b.part;part.guide=true;next.parts.push_back(part);}std::lock_guard<std::mutex> lock(volumesMutex);volumes=std::move(next);}
inline bool any(){return catchOn||jumpOn||rushOn||magsOn;}
inline void physicalEdge(unsigned key,bool down){if(key&&key<256){const bool prior=physical[key].exchange(down);if(down&&!prior){pressTime[key]=now();++pressSerial[key];}}}
inline std::uint64_t physicalSerial(int key){return key>0&&key<256?pressSerial[key].load():0;}
inline double physicalPressedAt(int key){return key>0&&key<256?pressTime[key].load():0;}
inline void pollPhysical(int key){if(key>0&&key<256&&!keyOwners[key])physicalEdge(key,(GetAsyncKeyState(key)&0x8000)!=0);}
inline bool held(int key){return key>0&&key<256&&physical[key];}
inline void syncPhysical(){for(int key=1;key<256;key++)physical[key]=(GetAsyncKeyState(key)&0x8000)!=0;}
inline void stop(){catchOn=jumpOn=rushOn=magsOn=false;if(extraStop)extraStop();}
inline Config config(){Config c;c.catchOn=catchOn||magsOn&&(magsAutoCatch||magsPickup);c.catchAir=catchOn||magsOn&&magsAutoCatch;c.catchLead=magsOn?magsCatchLeadMs.load()*.001:.55;c.catchGrace=magsOn?magsCatchHoldMs.load()*.001:.12;c.fastPass=magsOn&&magsFastPass;c.jumpOn=jumpOn;c.rushOn=rushOn;c.hold=holdRush;c.catchKey=catchBinding==0?VK_LBUTTON:catchBinding==1?'E':VK_MBUTTON;c.jumpKey=jumpBinding;c.rushKey=rushBinding;c.range=rushRange;c.refresh=catchRefresh;c.refreshPeriod=refreshMs.load()*.001;return c;}
inline Vec magsDimensions(){double size=reachTenths.load()*.1;return {size,size,size};}
inline double magsTeleportSize(){return combinedSnapSize(reachTenths.load()*.1);}
inline std::string magsDescription(){std::ostringstream text;text<<"Ball Mags / size "<<std::fixed<<std::setprecision(1)<<reachTenths.load()*.1<<" / teleport "<<magsTeleportSize();return text.str();}
inline void publish(std::string text){std::lock_guard<std::mutex> lock(statusMutex);status=std::move(text);}
inline std::string detail(){std::lock_guard<std::mutex> lock(statusMutex);return status;}
inline json::Value modules(){using namespace basketballUi;auto out=object();
 for(const auto& key:{L"ff3_catch",L"ff3_jump",L"ff3_rush",L"ff3_mags"}){auto m=object();m.o[L"on"]=basketballUi::boolean(key==std::wstring(L"ff3_catch")?catchOn.load():key==std::wstring(L"ff3_jump")?jumpOn.load():key==std::wstring(L"ff3_rush")?rushOn.load():magsOn.load());m.o[L"rows"]=object();out.o[key]=m;}
 out.o[L"ff3_catch"].o[L"rows"].o[L"input"]=number(catchBinding);out.o[L"ff3_jump"].o[L"rows"].o[L"input"]=keys(jumpBinding);
 out.o[L"ff3_rush"].o[L"rows"].o[L"input"]=keys(rushBinding);out.o[L"ff3_rush"].o[L"rows"].o[L"hold"]=basketballUi::boolean(holdRush);out.o[L"ff3_rush"].o[L"rows"].o[L"range"]=number(rushRange);
 out.o[L"ff3_catch"].o[L"rows"].o[L"refresh"]=basketballUi::boolean(catchRefresh);out.o[L"ff3_catch"].o[L"rows"].o[L"refreshMs"]=number(refreshMs);
 auto& m=out.o[L"ff3_mags"].o[L"rows"].o;m[L"teleportSize"]=number(magsTeleportSize());m[L"studs"]=number(reachTenths.load()*.1);m[L"delay"]=number(magDelayMs);m[L"visual"]=basketballUi::boolean(magsVisual);m[L"autoCatch"]=basketballUi::boolean(magsAutoCatch);m[L"pickup"]=basketballUi::boolean(magsPickup);m[L"fastPass"]=basketballUi::boolean(magsFastPass);m[L"catchLead"]=number(magsCatchLeadMs);m[L"holdMs"]=number(magsCatchHoldMs);m[L"catchKey"]=number(catchBinding);return out;
}
inline bool set(const std::wstring& key,const json::Value& value){
 if(key==L"ff3_mags.autoCatch"||key==L"ff3_mags.pickup"||key==L"ff3_mags.fastPass"){if(value.type!=json::Value::Bool)return false;if(key==L"ff3_mags.autoCatch")magsAutoCatch=value.b;else if(key==L"ff3_mags.pickup")magsPickup=value.b;else magsFastPass=value.b;return true;}
 if(key==L"ff3_mags.catchKey"){if(value.type!=json::Value::Number||value.n<0||value.n>2||std::floor(value.n)!=value.n)return false;catchBinding=int(value.n);return true;}
 if(key==L"ff3_mags.catchLead"||key==L"ff3_mags.holdMs"){if(value.type!=json::Value::Number||!std::isfinite(value.n)||std::floor(value.n)!=value.n||std::fmod(value.n,10)!=0)return false;if(key==L"ff3_mags.catchLead"){if(value.n<50||value.n>500)return false;magsCatchLeadMs=int(value.n);}else{if(value.n<60||value.n>300)return false;magsCatchHoldMs=int(value.n);}return true;}
 if(key==L"ff3_catch"||key==L"ff3_jump"||key==L"ff3_rush"||key==L"ff3_mags"||key==L"ff3_rush.hold"||key==L"ff3_catch.refresh"||key==L"ff3_mags.visual"){
  if(value.type!=json::Value::Bool)return false;
  if(key==L"ff3_catch")catchOn=value.b;else if(key==L"ff3_jump")jumpOn=value.b;else if(key==L"ff3_rush")rushOn=value.b;else if(key==L"ff3_mags")magsOn=value.b;else if(key==L"ff3_catch.refresh")catchRefresh=value.b;else if(key==L"ff3_mags.visual")magsVisual=value.b;else holdRush=value.b;return true;
 }
 if(key==L"ff3_jump.input"||key==L"ff3_rush.input"){
  if(value.type!=json::Value::Array||value.a.size()!=1)return false;int code=basketballUi::keyCode(value.a[0]);
  if(code<=0||!basketballUi::validKey(code)||code==VK_INSERT||code==VK_RETURN||code=='W'||code=='A'||code=='S'||code=='D'||code=='Q'||code=='R'||code=='F'||code=='Z')return false;
  if(key==L"ff3_jump.input")jumpBinding=code;else rushBinding=code;return true;
 }
 if(key==L"ff3_mags.studs"){
  if(value.type!=json::Value::Number||!std::isfinite(value.n)||value.n<1||value.n>maxMagSize||std::abs(value.n*10-std::round(value.n*10))>.00001)return false;int n=int(std::lround(value.n*10));
  reachTenths=n;return true;
 }
 if(value.type!=json::Value::Number||!std::isfinite(value.n)||std::floor(value.n)!=value.n)return false;
 if(key==L"ff3_catch.input"&&value.n>=0&&value.n<=2){catchBinding=int(value.n);return true;}
 if(key==L"ff3_rush.range"&&value.n>=10&&value.n<=300){rushRange=int(value.n);return true;}
 if(key==L"ff3_mags.delay"&&value.n>=0&&value.n<=500){magDelayMs=int(value.n);return true;}
 if(key==L"ff3_catch.refreshMs"&&value.n>=300&&value.n<=1500){refreshMs=int(value.n);return true;}
 return false;
}
inline bool save(const std::filesystem::path& file){auto temp=file;temp+=L".tmp";{std::ofstream out(temp,std::ios::binary);out<<winrt::to_string(json::dump(modules()));out.flush();if(!out)return false;}return MoveFileExW(temp.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;}
inline void load(const std::filesystem::path& file){
 auto data=basketballUi::readConfig(file);reachTenths=200;magDelayMs=0;catchRefresh=false;magsVisual=true;refreshMs=700;magsAutoCatch=magsPickup=false;magsFastPass=true;magsCatchLeadMs=250;magsCatchHoldMs=140;
 auto rows=data[L"ff3_mags"][L"rows"],old=rows[L"reach"];
 if(rows[L"studs"].type==json::Value::Null&&old.type==json::Value::Number&&std::isfinite(old.n)&&old.n>=100&&old.n<=300)reachTenths=std::clamp(int(std::lround(old.n*.03))*5,10,100);
 for(const auto& entry:data.o)for(const auto& row:entry.second[L"rows"].o){if(entry.first==L"ff3_mags"&&(row.first==L"shape"||row.first==L"width"||row.first==L"height"||row.first==L"depth"))continue;set(entry.first+L"."+row.first,row.second);}
 // Hidden input preferences from the old Hand Snap mode never start sending
 // catch inputs just because the two ball features are now combined.
 if(rows[L"mode"].type==json::Value::Number&&rows[L"mode"].num()==1)magsAutoCatch=magsPickup=false;
 magDelayMs=0;stop();
}
inline ff3qb::CatchGeometry receivingShape(Vec torso,Vec head,Vec arm,Vec left,Vec right){
 auto valid=[](Vec v,double limit){return ff3qb::finite(v)&&std::min({v.x,v.y,v.z})>.1&&std::max({v.x,v.y,v.z})<=limit;};
 if(!valid(torso,8)||!valid(head,8)||!valid(arm,8)||!valid(left,maxMagSize+.01)||!valid(right,maxMagSize+.01))return {};
 double reach=0;for(Vec part:{left,right})reach=std::max(reach,std::hypot(arm.y+part.y,.5*std::hypot(part.x,part.z))+.6);
 return {torso.x*.5+arm.x*.5+reach,std::max(0.,reach-head.y*.5),reach+head.y*.5,true};
}
inline ff3qb::CatchGeometry receivingShape(HANDLE h,addr rig,const ReachEdits* originals=nullptr){
 auto size=[&](addr part){auto current=ff3Native::partSize(h,part);if(originals)for(const auto& edit:originals->edits)if(edit.original.id==part&&edit.original.rig==rig&&edit.original.primitive==rd<addr>(h,part+Offsets::BasePart::Primitive)&&sameSize(current,edit.applied))return edit.original.size;return current;};
 auto hands=ff3Native::catchParts(h,rig);
 return receivingShape(size(child(h,rig,"Torso")),size(child(h,rig,"Head")),size(child(h,rig,"Left Arm")),size(hands.first),size(hands.second));
}
struct Inputs {
 std::array<bool,256> owned{};std::array<double,256> until{};using Sender=std::function<bool(int,bool)>;Sender send;
 double catchDownAt=-1,catchRestartAt=-1;int cycleKey=0;bool refreshPending=false;unsigned refreshCount=0;
 explicit Inputs(Sender sender=[](int key,bool down){if(key==VK_LBUTTON){INPUT input{};input.type=INPUT_MOUSE;input.mi.dwFlags=down?MOUSEEVENTF_LEFTDOWN:MOUSEEVENTF_LEFTUP;return SendInput(1,&input,sizeof input)==1;}return defense::sendKey(key,down);}):send(std::move(sender)){}
 bool apply(const Config& c,const Scene& s,const Decision& d,double time){
  std::array<bool,256> want{};bool ok=true,active=allowed(s,time);
  const bool catchKeyValid=c.catchKey>0&&c.catchKey<255,jumpKeyValid=c.jumpKey>0&&c.jumpKey<255;
  if(cycleKey!=c.catchKey||!active||!c.catchOn||!catchKeyValid||held(c.catchKey)){catchDownAt=catchRestartAt=-1;refreshPending=false;cycleKey=c.catchKey;}
  if(!c.refresh){catchRestartAt=-1;refreshPending=false;}
  if(catchKeyValid&&!c.catchOn)until[c.catchKey]=0;if(jumpKeyValid&&!c.jumpOn)until[c.jumpKey]=0;
  if(active){if(c.rushOn&&d.moving&&!navigationOwned&&!s.manual&&!held('W')&&!held('A')&&!held('S')&&!held('D')&&(!c.hold||held(c.rushKey))){double f=ff3qb::dot(d.direction,s.cameraForward),r=ff3qb::dot(d.direction,s.cameraRight);want['W']=f>(owned['W']?.25:.38);want['S']=-f>(owned['S']?.25:.38);want['D']=r>(owned['D']?.25:.38);want['A']=-r>(owned['A']?.25:.38);}
   if(c.catchOn&&catchKeyValid){if(d.catchHold)until[c.catchKey]=std::max(until[c.catchKey],time+std::clamp(c.catchGrace,.06,.3));else if(d.catchNow)until[c.catchKey]=time+std::clamp(c.catchGrace,.06,.3);}if(c.jumpOn&&d.jumpNow&&jumpKeyValid)until[c.jumpKey]=time+.065;
  }else until.fill(0);
  if(active&&c.catchOn&&c.refresh&&catchKeyValid&&owned[c.catchKey]&&!held(c.catchKey)&&d.catchHold&&d.eta>.18&&catchDownAt>=0&&time-catchDownAt>=std::clamp(c.refreshPeriod,.3,1.5)){
   catchRestartAt=time+.05;catchDownAt=-1;refreshPending=true;until[c.catchKey]=0;
  }
  for(int key=1;key<255;key++){
   if(held(key)){owned[key]=false;releaseKey(key,1);continue;}if((keyOwners[key]!=0&&keyOwners[key]!=1)){owned[key]=false;continue;}
   bool need=active&&(want[key]||until[key]>time)&&!(key==c.catchKey&&time<catchRestartAt);
   if(need&&!owned[key]&&claimKey(key,1)){if(send(key,true)){owned[key]=true;if(key==c.catchKey){catchDownAt=time;if(refreshPending){++refreshCount;refreshPending=false;}}}else{releaseKey(key,1);ok=false;}}
   else if(!need&&owned[key]){if(send(key,false)){owned[key]=false;releaseKey(key,1);if(key==c.catchKey)catchDownAt=-1;}else ok=false;}
  }return ok;
 }
 void release(){until.fill(0);catchDownAt=catchRestartAt=-1;refreshPending=false;for(int key=1;key<255;key++)if(owned[key]){if(held(key)||(keyOwners[key]!=0&&keyOwners[key]!=1)||send(key,false)){owned[key]=false;releaseKey(key,1);}}}
 ~Inputs(){release();}
};
struct Reader {
 HANDLE h=nullptr;DWORD pid=0;addr own=0,workspace=0;ff3Native::Reader flights;double ground=NAN,standingHeadHeight=NAN,standingRootHeight=NAN,priorAt=0,groundFreezeUntil=0;Vec priorBody;
 std::vector<BallPart> magParts,groundParts,looseParts;
 ~Reader(){if(h)CloseHandle(h);}
 Scene read(const Config& c){
  magParts.clear();groundParts.clear();looseParts.clear();Scene s;s.at=now();auto session=gameModules::router.get();s.pid=session.pid;s.model=session.model;s.activation=held(c.rushKey);s.manual=held('W')||held('A')||held('S')||held('D');
  if(!ff3Native::admitted()||session.studio)return s;s.focused=ff3Native::focused(s.pid);
  if(pid!=s.pid){if(h)CloseHandle(h);h=OpenProcess(PROCESS_VM_READ|SYNCHRONIZE,FALSE,s.pid);pid=s.pid;own=workspace=0;priorAt=0;flights.flightTracker.clear();flights.flightWorkspace=0;flights.ballCatalog.reset();}
  if(!h||WaitForSingleObject(h,0)!=WAIT_TIMEOUT)return s;
  auto root=s.model,ws=child(h,root,"Workspace"),players=child(h,root,"Players");s.player=rd<addr>(h,players+Offsets::Player::LocalPlayer);s.own=rd<addr>(h,s.player+Offsets::Player::ModelInstance);
  auto hum=child(h,s.own,"Humanoid"),head=child(h,s.own,"Head"),body=child(h,s.own,"HumanoidRootPart");
  double health=rd<float>(h,hum+Offsets::Humanoid::Health,NAN);if(!ws||!s.own||!std::isfinite(health)||health<=0)return s;
  auto input=rd<addr>(h,child(h,root,"UserInputService")+Offsets::UserInputService::WindowInputState);s.typing=!input||rd<addr>(h,input+Offsets::WindowInputState::CurrentTextBox)!=0;
  s.menu=rd<unsigned char>(h,child(h,root,"GuiService")+Offsets::GuiService::MenuIsOpen,255)!=0;if((s.typing||s.menu)&&!magsOn)return s;
  s.head=ff3Native::position(h,head);s.velocity=ff3Native::velocity(h,body);s.speed=rd<float>(h,hum+Offsets::Humanoid::Walkspeed,NAN);s.worldGravity=rd<float>(h,ws+Offsets::Workspace::ReadOnlyGravity,NAN);
  if(!ff3qb::finite(s.head)||!ff3qb::finite(s.velocity)||ff3qb::length(s.velocity)>200||!std::isfinite(s.speed)||s.speed<0||s.speed>60||!std::isfinite(s.worldGravity)||s.worldGravity<10||s.worldGravity>300)return s;
  if(own!=s.own||workspace!=ws){priorAt=0;own=s.own;workspace=ws;ground=standingHeadHeight=standingRootHeight=NAN;flights.flightTracker.clear();}
  auto bodyPosition=ff3Native::position(h,body);if(!ff3qb::finite(bodyPosition))return s;s.rootPosition=bodyPosition;
  
  int floor=rd<int>(h,hum+Offsets::Humanoid::FloorMaterial,1792);s.grounded=uprightGround(floor,s.velocity,rd<int>(h,rd<addr>(h,hum+Offsets::Humanoid::HumanoidState)+Offsets::Humanoid::HumanoidStateID,-1),rd<unsigned char>(h,hum+Offsets::Humanoid::PlatformStand,255),rd<unsigned char>(h,hum+Offsets::Humanoid::EvaluateStateMachine,255));
  if(s.grounded&&now()>=groundFreezeUntil){auto foot=child(h,s.own,"LeftFoot");if(!foot)foot=child(h,s.own,"Left Leg");auto p=ff3Native::position(h,foot),size=ff3Native::partSize(h,foot);if(ff3qb::finite(p)&&ff3qb::finite(size)){ground=p.y-size.y*.5;const double height=s.head.y-ground;if(height>2&&height<12)standingHeadHeight=height;const double rootHeight=bodyPosition.y-ground;if(rootHeight>1&&rootHeight<8)standingRootHeight=rootHeight;}}s.groundY=ground;s.standingHeadHeight=standingHeadHeight;s.standingRootHeight=standingRootHeight;
  double jump=rd<float>(h,hum+Offsets::Humanoid::JumpHeight,NAN);if(rd<bool>(h,hum+Offsets::Humanoid::UseJumpPower))s.jumpVelocity=rd<float>(h,hum+Offsets::Humanoid::JumpPower,0);else if(std::isfinite(jump)&&jump>=0)s.jumpVelocity=std::sqrt(2*s.worldGravity*jump);
  if(!std::isfinite(s.jumpVelocity)||s.jumpVelocity<0||s.jumpVelocity>150)s.jumpVelocity=0;
  s.possessed=ff3Native::Reader::possessedBall(h,s.own)!=0;s.shape=receivingShape(h,s.own);s.naturalShape=s.shape;
  auto rotation=rd<defense::Matrix3>(h,rd<addr>(h,ws+Offsets::Workspace::CurrentCamera)+Offsets::Camera::Rotation);s.cameraForward=unit({-rotation.m[2],0,-rotation.m[8]});s.cameraRight=unit({rotation.m[0],0,rotation.m[6]});
  if(ff3qb::length(s.cameraForward)<.5||ff3qb::length(s.cameraRight)<.5||std::abs(ff3qb::dot(s.cameraForward,s.cameraRight))>.2)return s;
  if(c.catchOn||c.jumpOn||magsOn||c.scanLoose){ff3Native::Frame frame;frame.at=s.at;frame.pid=s.pid;frame.head=s.head;frame.groundY=ground;flights.collectBalls(h,ws,frame,true);s.balls=std::move(frame.balls);s.looseBalls=std::move(frame.looseBalls);s.at=frame.at;s.head=ff3Native::position(h,head);auto latest=ff3Native::velocity(h,body);if(ff3qb::finite(latest)&&ff3qb::length(latest)<200)s.velocity=latest;}
  if(magsOn||c.scanLoose){
   std::vector<addr> ids;for(const auto& flight:s.balls)if(receivingBallCandidate(s,flight,ground,now()))ids.push_back(flight.id);
   std::sort(ids.begin(),ids.end());ids.erase(std::unique(ids.begin(),ids.end()),ids.end());
   for(auto id:ids){if(!instanceClassIs(h,id,"Part")||!path(h,id,{"Configuration","Gravity"}))continue;auto primitive=rd<addr>(h,id+Offsets::BasePart::Primitive),parent=rd<addr>(h,id+Offsets::Instance::Parent);if(!primitive||!parent||(rd<unsigned char>(h,primitive+Offsets::Primitive::Flags)&Offsets::PrimitiveFlags::Anchored))continue;
    addr mesh=0;for(auto item:children(h,id))if(instanceClassIs(h,item,"SpecialMesh")&&!defense::textAt(h,item+Offsets::FileMesh::MeshId).empty()){mesh=item;break;}if(!mesh)continue;
    auto scale=ff3Native::vector(rd<shooting::Vec>(h,mesh+Offsets::DataModelMesh::Scale,{NAN,NAN,NAN})),position=ff3Native::position(h,id);auto flags=rd<unsigned char>(h,primitive+Offsets::Primitive::Flags,255);if(flags!=255&&validMeshScale(scale)&&ff3qb::finite(position))magParts.push_back({{id,parent,primitive,ff3Native::partSize(h,id),true,mesh},scale,position,bool(flags&Offsets::PrimitiveFlags::CanCollide)});
   }
   std::sort(magParts.begin(),magParts.end(),[&](const auto& a,const auto& b){return ff3qb::length(a.position-s.head)<ff3qb::length(b.position-s.head);});if(magParts.size()>maxBallEdits)magParts.resize(maxBallEdits);
   // Resting balls are absent from the airborne tracker, so inspect its loose
   // discovery list separately for read-only ground diagnostics.
   auto nearby=s.looseBalls;std::stable_sort(nearby.begin(),nearby.end(),[&](const auto& a,const auto& b){return ff3qb::length(a.position-s.head)<ff3qb::length(b.position-s.head);});
   for(const auto& observed:nearby){if(looseParts.size()>=maxBallEdits)break;const auto id=observed.id;
    if(!instanceClassIs(h,id,"Part")||!path(h,id,{"Configuration","Gravity"})||!ff3Native::looseWorldBall(h,id,ws))continue;
    auto parent=rd<addr>(h,id+Offsets::Instance::Parent);
    auto primitive=rd<addr>(h,id+Offsets::BasePart::Primitive);auto flags=rd<unsigned char>(h,primitive+Offsets::Primitive::Flags,255);auto p=ff3Native::position(h,id),v=ff3Native::velocity(h,id);
    if(!primitive||flags==255||(flags&Offsets::PrimitiveFlags::Anchored)||!ff3qb::finite(p)||!ff3qb::finite(v))continue;
    addr mesh=0;for(auto item:children(h,id))if(instanceClassIs(h,item,"SpecialMesh")&&!defense::textAt(h,item+Offsets::FileMesh::MeshId).empty()){mesh=item;break;}if(!mesh)continue;
    auto scale=ff3Native::vector(rd<shooting::Vec>(h,mesh+Offsets::DataModelMesh::Scale,{NAN,NAN,NAN}));auto size=ff3Native::partSize(h,id);if(validMeshScale(scale)&&ff3qb::finite(size)&&std::min({size.x,size.y,size.z})>0&&std::max({size.x,size.y,size.z})<=maxMagSize){BallPart ball{{id,parent,primitive,size,true,mesh},scale,p,bool(flags&Offsets::PrimitiveFlags::CanCollide)};looseParts.push_back(ball);if(groundBallCandidate(p,v,ground,std::max(2.5,size.y*.5+.75)))groundParts.push_back(ball);}
   }
  }
  if(c.rushOn){auto team=rd<addr>(h,s.player+Offsets::Player::Team);int count=0;for(auto player:children(h,players)){if(++count>128)break;auto otherTeam=rd<addr>(h,player+Offsets::Player::Team);if(player==s.player||!team||!otherTeam||team==otherTeam)continue;auto rig=rd<addr>(h,player+Offsets::Player::ModelInstance);auto ball=ff3Native::Reader::possessedBall(h,rig);if(!ball||rd<float>(h,child(h,rig,"Humanoid")+Offsets::Humanoid::Health,0)<=0)continue;auto part=child(h,rig,"HumanoidRootPart");auto p=ff3Native::position(h,part),v=ff3Native::velocity(h,part);if(ff3qb::finite(p)&&ff3qb::finite(v)&&ff3qb::length(v)<100)s.carriers.push_back({rig,ball,p,v});}}
  s.head=ff3Native::position(h,head);s.rootPosition=ff3Native::position(h,body);s.at=now();
  s.rootVelocity=ff3Native::velocity(h,body);if(ff3qb::finite(s.rootVelocity)&&ff3qb::length(s.rootVelocity)<200)s.velocity=s.rootVelocity;
  s.velocity=observedRootVelocity(priorBody,s.rootPosition,priorAt,s.at,s.velocity);priorBody=s.rootPosition;priorAt=s.at;s.valid=ff3qb::finite(s.head)&&ff3qb::finite(s.rootPosition);return s;
 }
 bool stillSafe(const Scene& s){return h&&ff3Native::admitted()&&ff3Native::focused(s.pid)&&WaitForSingleObject(h,0)==WAIT_TIMEOUT&&rd<addr>(h,s.player+Offsets::Player::ModelInstance)==s.own&&rd<float>(h,child(h,s.own,"Humanoid")+Offsets::Humanoid::Health,0)>0&&!ff3Native::Reader::possessedBall(h,s.own);}
};
// Durable catch-part edits, also used to recover journals from older versions.
struct LegacyMags {
 HANDLE h=nullptr;DWORD pid=0;ReachEdits edits;BallDelay delay;bool journalOwned=false;std::filesystem::path file;
 std::filesystem::path journalFile()const{return file.empty()?recoveryPath:file;}
 static std::wstring created(HANDLE process){FILETIME born{},exit{},kernel{},user{};if(!GetProcessTimes(process,&born,&exit,&kernel,&user))return {};ULARGE_INTEGER time{};time.LowPart=born.dwLowDateTime;time.HighPart=born.dwHighDateTime;return std::to_wstring(time.QuadPart);}
 bool journal(){
  if(journalFile().empty())return true;
  using namespace basketballUi;auto data=object();data.o[L"pid"]=number(pid);auto stamp=created(h);if(stamp.empty())return false;data.o[L"created"]=string(stamp);auto parts=array();
  for(const auto& e:edits.edits){auto p=object();p.o[L"part"]=number(double(e.original.id));p.o[L"rig"]=number(double(e.original.rig));p.o[L"primitive"]=number(double(e.original.primitive));p.o[L"ball"]=basketballUi::boolean(e.original.ball);p.o[L"mesh"]=number(double(e.original.mesh));auto original=array(),applied=array();for(double v:{e.original.size.x,e.original.size.y,e.original.size.z})original.a.push_back(number(v));for(double v:{e.applied.x,e.applied.y,e.applied.z})applied.a.push_back(number(v));p.o[L"original"]=original;p.o[L"applied"]=applied;parts.a.push_back(p);}data.o[L"parts"]=parts;
  auto temp=journalFile();temp+=L".tmp";{std::ofstream out(temp,std::ios::binary);out<<winrt::to_string(json::dump(data));out.flush();if(!out)return false;}
  if(!MoveFileExW(temp.c_str(),journalFile().c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return false;journalOwned=true;return true;
 }
 bool read(const CatchPart& seed,CatchPart& out){if(!h||WaitForSingleObject(h,0)!=WAIT_TIMEOUT)return false;out=seed;auto parent=rd<addr>(h,seed.id+Offsets::Instance::Parent);out.rig=seed.ball&&parent?seed.rig:parent;out.primitive=rd<addr>(h,seed.id+Offsets::BasePart::Primitive);if(out.rig!=seed.rig||out.primitive!=seed.primitive)return true;out.size=ff3Native::partSize(h,seed.id);return ff3qb::finite(out.size);}
 bool write(const CatchPart& part,Vec value){
  CatchPart live;if(!read(part,live)||live.rig!=part.rig||live.primitive!=part.primitive)return false;
  const bool restoring=std::any_of(edits.edits.begin(),edits.edits.end(),[&](const auto& e){return e.original.id==part.id&&sameSize(value,e.original.size);});
  if(!restoring)return false;
  Vec scale;bool separateMesh=part.ball&&part.mesh&&rd<addr>(h,part.mesh+Offsets::Instance::Parent)==part.id;
  if(separateMesh)scale=ff3Native::vector(rd<shooting::Vec>(h,part.mesh+Offsets::DataModelMesh::Scale,{NAN,NAN,NAN}));
  if(part.ball&&!restoring&&(!separateMesh||!ff3qb::finite(scale)))return false;
  if(!journal())return false;shooting::Vec size{float(value.x),float(value.y),float(value.z)};SIZE_T n=0;
  if(!WriteProcessMemory(h,(void*)(part.primitive+Offsets::Primitive::Size),&size,sizeof size,&n)||n!=sizeof size||!sameSize(ff3Native::partSize(h,part.id),value))return false;
  // File-mesh scale is intentionally untouched. The displayed football keeps
  // its original scale while the separate part geometry changes.
  return !separateMesh||sameSize(scale,ff3Native::vector(rd<shooting::Vec>(h,part.mesh+Offsets::DataModelMesh::Scale,{NAN,NAN,NAN})));
 }
 bool restore(){delay.seen.clear();if(h&&WaitForSingleObject(h,0)==WAIT_OBJECT_0)edits.edits.clear();bool ok=edits.restore([&](const auto& a,auto& b){return read(a,b);},[&](const auto& a,Vec b){return write(a,b);});if(edits.edits.empty()&&journalOwned&&!journalFile().empty()){std::error_code error;std::filesystem::remove(journalFile(),error);journalOwned=false;}return ok;}
 ~LegacyMags(){restore();if(h)CloseHandle(h);}
};
inline bool recoverLegacyReach(const std::filesystem::path& file={}){
 const auto source=file.empty()?recoveryPath:file;
 if(source.empty()||!std::filesystem::exists(source))return true;
 try{
  auto data=basketballUi::readConfig(source);double number=data[L"pid"].num();if(number<=0||number>MAXDWORD||std::floor(number)!=number||data[L"created"].str().empty()||data[L"parts"].type!=json::Value::Array||data[L"parts"].a.size()>8)return false;
  HANDLE h=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_VM_OPERATION|SYNCHRONIZE,FALSE,DWORD(number));
  if(!h){if(GetLastError()==ERROR_INVALID_PARAMETER){std::filesystem::remove(source);return true;}return false;}
  if(LegacyMags::created(h)!=data[L"created"].str()||WaitForSingleObject(h,0)!=WAIT_TIMEOUT){CloseHandle(h);std::filesystem::remove(source);return true;}
  std::vector<ReachEdits::Edit> saved;
  for(const auto& p:data[L"parts"].a){auto original=p[L"original"],applied=p[L"applied"];if(original.type!=json::Value::Array||applied.type!=json::Value::Array||original.a.size()!=3||applied.a.size()!=3){CloseHandle(h);return false;}
   CatchPart part{std::uint64_t(p[L"part"].num()),std::uint64_t(p[L"rig"].num()),std::uint64_t(p[L"primitive"].num()),{original.a[0].num(),original.a[1].num(),original.a[2].num()}};Vec size{applied.a[0].num(),applied.a[1].num(),applied.a[2].num()};part.ball=p[L"ball"].boolean();part.mesh=std::uint64_t(p[L"mesh"].num());
   if(part.id<65536||part.rig<65536||part.primitive<65536||!ff3qb::finite(part.size)||!ff3qb::finite(size)||std::min({part.size.x,part.size.y,part.size.z})<=0||std::max({part.size.x,part.size.y,part.size.z})>16||std::max({size.x,size.y,size.z})>maxMagSize){CloseHandle(h);return false;}saved.push_back({part,size});
  }
  LegacyMags recovery;recovery.file=source;recovery.h=h;recovery.pid=DWORD(number);recovery.edits.edits=std::move(saved);recovery.journalOwned=true;return recovery.restore();
 }catch(...){return false;}
}
inline std::filesystem::path groundRecoveryFile(){auto p=recoveryPath;if(!p.empty())p+=L".ground";return p;}
inline bool nonSolidCatchFlags(unsigned char flags){return flags!=255&&!(flags&(Offsets::PrimitiveFlags::CanCollide|Offsets::PrimitiveFlags::Anchored));}
// Practice puts catches and solid bodies in Players. Lease only the two
// non-solid catch groups into Default; their geometry and flags never change.
struct CatchGroupLease {
 struct Entry {addr id=0,rig=0,primitive=0;unsigned original=0;};
 std::vector<Entry> entries;DWORD pid=0;std::wstring stamp;bool owned=false;
 static std::filesystem::path path(){auto p=recoveryPath;if(!p.empty())p+=L".catch-groups";return p;}
 bool save(){
  if(path().empty())return true;using namespace basketballUi;auto data=object();data.o[L"version"]=number(1);data.o[L"pid"]=number(pid);data.o[L"created"]=string(stamp);auto rows=array();
  for(const auto& e:entries){auto row=object();row.o[L"id"]=number(double(e.id));row.o[L"rig"]=number(double(e.rig));row.o[L"primitive"]=number(double(e.primitive));row.o[L"original"]=number(e.original);rows.a.push_back(row);}data.o[L"parts"]=rows;
  auto temp=path();temp+=L".tmp";{std::ofstream out(temp,std::ios::binary);out<<winrt::to_string(json::dump(data));out.flush();if(!out)return false;}return MoveFileExW(temp.c_str(),path().c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0;
 }
 static bool write(HANDLE h,const Entry& e,unsigned group){
  if(group>=32||rd<addr>(h,e.id+Offsets::Instance::Parent)!=e.rig||rd<addr>(h,e.id+Offsets::BasePart::Primitive)!=e.primitive)return false;
  auto flags=rd<unsigned char>(h,e.primitive+Offsets::Primitive::Flags,255);if(!nonSolidCatchFlags(flags))return false;
  unsigned char value=static_cast<unsigned char>(group);SIZE_T n=0;
  return WriteProcessMemory(h,(void*)(e.primitive+physicsGroupOffset),&value,1,&n)&&n==1&&rd<unsigned char>(h,e.primitive+physicsGroupOffset,255)==group&&rd<unsigned char>(h,e.primitive+Offsets::Primitive::Flags,255)==flags;
 }
 bool restore(HANDLE h){
  if(!h)return entries.empty();
  if(WaitForSingleObject(h,0)==WAIT_OBJECT_0)entries.clear();
  for(auto it=entries.begin();it!=entries.end();){
   if(rd<addr>(h,it->id+Offsets::Instance::Parent)!=it->rig||rd<addr>(h,it->id+Offsets::BasePart::Primitive)!=it->primitive){it=entries.erase(it);continue;}
   auto hands=ff3Native::catchParts(h,it->rig);if(it->id!=hands.first&&it->id!=hands.second)return false;
   auto group=rd<unsigned char>(h,it->primitive+physicsGroupOffset,255);if(group==255)return false;
   // A new game-assigned group is no longer ours to overwrite.
   if(group==0&&!write(h,*it,it->original)){++it;continue;}it=entries.erase(it);
  }
  if(entries.empty()&&owned&&!path().empty()){std::error_code ec;std::filesystem::remove(path(),ec);if(ec)return false;owned=false;}
  return entries.empty();
 }
 bool acquire(HANDLE h,const Scene& s,const NativeCollisionPlan& plan){
  const auto session=gameModules::router.get();
  if(!entries.empty()||session.pid!=s.pid||session.model!=s.model||!ff3Native::isPracticeRange(session.placeId)||!plan.complete||!plan.bodies||!plan.catches||!(plan.bodies&plan.catches)||(plan.bodies&1u)||plan.rules.names[0]!="Default"||plan.rules.choose(0,plan.bodies,1u)<0)return false;
  for(const auto& p:plan.parts)if(p.catchPart){if(p.group>=32||p.parent!=s.own||plan.rules.names[p.group]!="Players")return false;entries.push_back({p.id,p.parent,p.primitive,p.group});}
  if(entries.size()!=2){entries.clear();return false;}pid=s.pid;stamp=LegacyMags::created(h);if(stamp.empty()||!save()){entries.clear();return false;}owned=true;
  for(const auto& e:entries)if(!write(h,e,0)){restore(h);return false;}return true;
 }
 static bool recover(){
  if(path().empty()||!std::filesystem::exists(path()))return true;
  try{auto data=basketballUi::readConfig(path());auto count=data[L"pid"].num();const auto& rows=data[L"parts"];
   if(data[L"version"].num()!=1||count<=0||count>MAXDWORD||std::floor(count)!=count||data[L"created"].str().empty()||rows.type!=json::Value::Array||rows.a.empty()||rows.a.size()>2)return false;
   HANDLE h=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_VM_OPERATION|SYNCHRONIZE,FALSE,DWORD(count));if(!h){if(GetLastError()==ERROR_INVALID_PARAMETER){std::filesystem::remove(path());return true;}return false;}
   auto closer=[](void* p){CloseHandle(p);};std::unique_ptr<void,decltype(closer)> owner(h,closer);
   if(LegacyMags::created(h)!=data[L"created"].str()||WaitForSingleObject(h,0)!=WAIT_TIMEOUT){std::filesystem::remove(path());return true;}
   CatchGroupLease lease;lease.pid=DWORD(count);lease.stamp=data[L"created"].str();lease.owned=true;
   auto address=[](const json::Value& v)->addr{return v.type==json::Value::Number&&std::isfinite(v.n)&&v.n>=65536&&v.n<=9007199254740991.&&std::floor(v.n)==v.n?addr(v.n):0;};
   for(const auto& row:rows.a){Entry e{address(row[L"id"]),address(row[L"rig"]),address(row[L"primitive"]),unsigned(row[L"original"].num())};if(!e.id||!e.rig||!e.primitive||row[L"original"].type!=json::Value::Number||row[L"original"].num()<1||row[L"original"].num()>31||std::floor(row[L"original"].num())!=row[L"original"].num())return false;lease.entries.push_back(e);}
   return lease.restore(h);
  }catch(...){return false;}
 }
};
struct Mags {
 HANDLE h=nullptr;DWORD pid=0;addr model=0,own=0;BallEdits edits;NativeCollisionPlan filter;CatchGroupLease catchGroups;BallDelay delay;unsigned skippedBalls=0;bool journalOwned=false;std::wstring journalCache;
 static std::wstring created(HANDLE h){return LegacyMags::created(h);}
 bool journal(){
  if(recoveryPath.empty())return true;using namespace basketballUi;auto data=object();data.o[L"version"]=number(4);data.o[L"pid"]=number(pid);auto stamp=created(h);if(stamp.empty())return false;data.o[L"created"]=string(stamp);auto parts=array();
  for(const auto& e:edits.edits){const auto& p=e.original.part;auto row=object();row.o[L"part"]=number(double(p.id));row.o[L"rig"]=number(double(p.rig));row.o[L"primitive"]=number(double(p.primitive));row.o[L"mesh"]=number(double(p.mesh));row.o[L"visible"]=basketballUi::boolean(e.visible);row.o[L"sizeOwned"]=basketballUi::boolean(e.sizeOwned);row.o[L"meshOwned"]=basketballUi::boolean(e.meshOwned);row.o[L"groupOriginal"]=number(e.original.physicsGroup);row.o[L"groupApplied"]=number(e.groupApplied);row.o[L"groupOwned"]=basketballUi::boolean(e.groupOwned);
   for(auto pair:{std::pair<const wchar_t*,Vec>{L"original",p.size},{L"applied",e.applied},{L"meshOriginal",e.original.meshScale},{L"meshApplied",e.meshApplied}}){auto a=array();for(double n:{pair.second.x,pair.second.y,pair.second.z})a.a.push_back(number(n));row.o[pair.first]=a;}parts.a.push_back(row);
  }for(size_t i=0;i<parts.a.size();++i){parts.a[i].o[L"canCollide"]=basketballUi::boolean(edits.edits[i].original.canCollide);parts.a[i].o[L"collisionOwned"]=basketballUi::boolean(edits.edits[i].collisionOwned);}data.o[L"parts"]=parts;auto text=json::dump(data);if(journalOwned&&text==journalCache&&std::filesystem::exists(recoveryPath))return true;auto temp=recoveryPath;temp+=L".tmp";{std::ofstream out(temp,std::ios::binary);out<<winrt::to_string(text);out.flush();if(!out)return false;}if(!MoveFileExW(temp.c_str(),recoveryPath.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))return false;journalOwned=true;journalCache=std::move(text);return true;
 }
 bool read(const BallPart& seed,BallPart& out){
  if(!h||WaitForSingleObject(h,0)!=WAIT_TIMEOUT)return false;out=seed;const auto& p=seed.part;
  if(!rd<addr>(h,p.id+Offsets::Instance::Parent)||!instanceClassIs(h,p.id,"Part")){out={};return true;}
  out.part.primitive=rd<addr>(h,p.id+Offsets::BasePart::Primitive);out.part.size=ff3Native::partSize(h,p.id);if(!out.part.primitive||!ff3qb::finite(out.part.size))return false;auto flags=rd<unsigned char>(h,out.part.primitive+Offsets::Primitive::Flags,255);if(flags==255)return false;out.canCollide=(flags&Offsets::PrimitiveFlags::CanCollide)!=0;out.physicsGroup=rd<unsigned char>(h,out.part.primitive+physicsGroupOffset,255);if(out.physicsGroup>=32)return false;
  if(rd<addr>(h,p.mesh+Offsets::Instance::Parent)!=p.id){out.part.mesh=0;return true;}
  out.meshScale=ff3Native::vector(rd<shooting::Vec>(h,p.mesh+Offsets::DataModelMesh::Scale,{NAN,NAN,NAN}));return validMeshScale(out.meshScale);
 }
 bool write(const BallPart& original,BallEdits::Field field,Vec value){
  if(!original.part.ball)return false;
  BallPart live;if(!read(original,live)||live.part.id!=original.part.id||live.part.primitive!=original.part.primitive)return false;
  if(field==BallEdits::Field::Size&&!sameSize(value,original.part.size)){
   if(!filter.protects(h,original,live.physicsGroup))return false;
  }
  if(field==BallEdits::Field::MeshScale&&(live.part.mesh!=original.part.mesh||!validMeshScale(value)))return false;bool restoring=field==BallEdits::Field::Size?sameSize(value,original.part.size):sameSize(value,original.meshScale);if(!restoring&&!journal())return false;
  shooting::Vec size{float(value.x),float(value.y),float(value.z)};SIZE_T written=0;addr destination=field==BallEdits::Field::Size?original.part.primitive+Offsets::Primitive::Size:original.part.mesh+Offsets::DataModelMesh::Scale;
  if(!ff3qb::finite(value)||!WriteProcessMemory(h,(void*)destination,&size,sizeof size,&written)||written!=sizeof size)return false;
  auto actual=field==BallEdits::Field::Size?ff3Native::partSize(h,original.part.id):ff3Native::vector(rd<shooting::Vec>(h,original.part.mesh+Offsets::DataModelMesh::Scale,{NAN,NAN,NAN}));return sameSize(actual,value);
 }
 bool writeGroup(const BallPart& original,unsigned group){
  if(!original.part.ball||group>=32||original.physicsGroup>=32)return false;
  BallPart live;if(!read(original,live)||live.part.id!=original.part.id||live.part.primitive!=original.part.primitive)return false;
  const bool restoring=group==original.physicsGroup;
  if(restoring){if(!sameSize(live.part.size,original.part.size))return false;}
  else if(!filter.protects(h,original,group,false)||!journal())return false;
  auto before=rd<unsigned char>(h,original.part.primitive+Offsets::Primitive::Flags,255);if(before==255)return false;
  auto value=static_cast<unsigned char>(group);SIZE_T written=0;
  if(!WriteProcessMemory(h,(void*)(original.part.primitive+physicsGroupOffset),&value,1,&written)||written!=1)return false;
  return rd<unsigned char>(h,original.part.primitive+physicsGroupOffset,255)==group&&rd<unsigned char>(h,original.part.primitive+Offsets::Primitive::Flags,255)==before;
 }
 bool writeCollision(const BallPart& original,bool enabled){
  if(!original.part.ball)return false;BallPart live;if(!read(original,live)||live.part.id!=original.part.id||live.part.primitive!=original.part.primitive)return false;if(enabled&&!sameSize(live.part.size,original.part.size))return false;if(enabled!=original.canCollide&&!journal())return false;
  const auto at=original.part.primitive+Offsets::Primitive::Flags;auto before=rd<unsigned char>(h,at,255);if(before==255)return false;const auto mask=static_cast<unsigned char>(Offsets::PrimitiveFlags::CanCollide);auto after=collisionFlag(before,mask,enabled);SIZE_T written=0;
  if(after!=before&&(!WriteProcessMemory(h,(void*)at,&after,sizeof after,&written)||written!=sizeof after))return false;auto actual=rd<unsigned char>(h,at,255);return actual!=255&&(actual&mask)==(after&mask)&&(actual&~mask)==(before&~mask);
 }
 bool restore(){
  delay.seen.clear();if(h&&WaitForSingleObject(h,0)==WAIT_OBJECT_0)edits.edits.clear();bool ok=edits.restore([&](const auto& a,auto& b){return read(a,b);},[&](const auto& a,auto field,auto value){return write(a,field,value);},[&](const auto& a,bool enabled){return writeCollision(a,enabled);},[&](const auto& a,unsigned group){return writeGroup(a,group);});
  if(edits.edits.empty()&&journalOwned&&!recoveryPath.empty()){std::error_code error;std::filesystem::remove(recoveryPath,error);journalOwned=false;}if(edits.edits.empty())ok=catchGroups.restore(h)&&ok;return ok;
 }
 bool apply(const Scene& s,const std::vector<BallPart>& parts){
  if(pid!=s.pid||model!=s.model||own!=s.own||!h){if(!restore())return false;if(h)CloseHandle(h);h=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_VM_OPERATION|SYNCHRONIZE,FALSE,s.pid);pid=s.pid;model=s.model;own=s.own;}
  if(!h){restore();return false;}const auto ws=child(h,s.model,"Workspace");
  if(!filter.sample(h,s,ws)&&(!catchGroups.acquire(h,s,filter)||!filter.sample(h,s,ws))){restore();return false;}
  auto eligible=eligibleFilteredBalls(parts,[&](const auto& a,auto& b){return read(a,b);},[&](const auto& a){return filter.choose(a);},skippedBalls);
  std::vector<std::uint64_t> ids;for(const auto& p:eligible)ids.push_back(p.part.id);
  auto ready=delay.ready(ids,now(),magDelayMs);std::vector<BallPart> active;
  for(const auto& p:eligible)if(std::find(ready.begin(),ready.end(),p.part.id)!=ready.end())active.push_back(p);
  // Keep solid floor collision; only the existing ball physics group changes.
  bool ok=edits.applyFilteredGeometry(active,magsDimensions(),[&](const auto& a,auto& b){return read(a,b);},[&](const auto& a,auto field,auto value){return write(a,field,value);},[&](const auto& a,unsigned group){return writeGroup(a,group);},[&](const auto& a){return filter.choose(a);});
  if(edits.edits.empty()&&journalOwned&&!recoveryPath.empty()){std::error_code error;std::filesystem::remove(recoveryPath,error);journalOwned=false;}return ok;
 }
 ~Mags(){restore();if(h)CloseHandle(h);}
};
inline bool recoverBallReach(){
 const auto ground=groundRecoveryFile();if(!ground.empty()&&!recoverLegacyReach(ground))return false;
 if(recoveryPath.empty()||!std::filesystem::exists(recoveryPath))return true;
 try{auto data=basketballUi::readConfig(recoveryPath);auto version=data[L"version"].num();if(version!=2&&version!=3&&version!=4)return recoverLegacyReach();double number=data[L"pid"].num();auto parts=data[L"parts"];
  if(!std::isfinite(number)||number<=0||number>MAXDWORD||std::floor(number)!=number||data[L"created"].str().empty()||parts.type!=json::Value::Array||parts.a.size()>maxBallEdits)return false;
  HANDLE h=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION|PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_VM_OPERATION|SYNCHRONIZE,FALSE,DWORD(number));if(!h){if(GetLastError()==ERROR_INVALID_PARAMETER){std::filesystem::remove(recoveryPath);return true;}return false;}
  auto closer=[](void* value){CloseHandle(value);};std::unique_ptr<void,decltype(closer)> owner(h,closer);
  if(Mags::created(h)!=data[L"created"].str()||WaitForSingleObject(h,0)!=WAIT_TIMEOUT){std::filesystem::remove(recoveryPath);return true;}
  auto vector=[](const json::Value& v){if(v.type!=json::Value::Array||v.a.size()!=3)return Vec{NAN,NAN,NAN};for(const auto& n:v.a)if(n.type!=json::Value::Number||!std::isfinite(n.n))return Vec{NAN,NAN,NAN};return Vec{v.a[0].n,v.a[1].n,v.a[2].n};};
  auto address=[](const json::Value& v)->std::uint64_t{if(v.type!=json::Value::Number||!std::isfinite(v.n)||v.n<65536||v.n>9007199254740991.||std::floor(v.n)!=v.n)return 0;return std::uint64_t(v.n);};
  std::vector<BallEdits::Edit> saved;for(const auto& row:parts.a){BallEdits::Edit e;auto& p=e.original.part;p.id=address(row[L"part"]);p.rig=address(row[L"rig"]);p.primitive=address(row[L"primitive"]);p.mesh=address(row[L"mesh"]);p.ball=true;p.size=vector(row[L"original"]);e.original.meshScale=vector(row[L"meshOriginal"]);e.applied=vector(row[L"applied"]);e.meshApplied=vector(row[L"meshApplied"]);e.visible=row[L"visible"].boolean();e.sizeOwned=row[L"sizeOwned"].boolean();e.meshOwned=row[L"meshOwned"].boolean();
   if(!p.id||!p.rig||!p.primitive||!p.mesh||!ff3qb::finite(p.size)||std::min({p.size.x,p.size.y,p.size.z})<=0||std::max({p.size.x,p.size.y,p.size.z})>16||!validCatchSize(e.applied)||!validMeshScale(e.original.meshScale)||!validMeshScale(e.meshApplied)||row[L"visible"].type!=json::Value::Bool||row[L"sizeOwned"].type!=json::Value::Bool||row[L"meshOwned"].type!=json::Value::Bool)return false;if(version>=3){if(row[L"canCollide"].type!=json::Value::Bool||row[L"collisionOwned"].type!=json::Value::Bool)return false;e.original.canCollide=row[L"canCollide"].boolean();e.collisionOwned=row[L"collisionOwned"].boolean();}
   if(version==4){auto original=row[L"groupOriginal"],applied=row[L"groupApplied"];if(original.type!=json::Value::Number||applied.type!=json::Value::Number||!std::isfinite(original.n)||!std::isfinite(applied.n)||original.n<0||original.n>31||applied.n<0||applied.n>31||std::floor(original.n)!=original.n||std::floor(applied.n)!=applied.n||row[L"groupOwned"].type!=json::Value::Bool)return false;e.original.physicsGroup=unsigned(original.n);e.groupApplied=unsigned(applied.n);e.groupOwned=row[L"groupOwned"].boolean();}
   saved.push_back(e);
  }Mags recovery;recovery.h=owner.release();recovery.pid=DWORD(number);recovery.edits.edits=std::move(saved);recovery.journalOwned=true;return recovery.restore();
 }catch(...){return false;}
}
inline bool recoverReach(){return recoverBallReach()&&CatchGroupLease::recover();}
}
#include "ff3_hand_snap_access.hpp"
#include "ff3_hand_snap_fast.hpp"
namespace ff3Play {
inline void worker(){
 Reader reader;Planner planner;Inputs inputs;Mags mags;HandSnap snap;FastHandSnap fastSnap;FastSnapStats snapAccess;double nextRecovery=0,nextDiagnostic=0;
 while(running){
  if(!stopPath.empty()&&std::filesystem::exists(stopPath)){stop();fastSnap.clear();snap.cancel("Hand Snap off");inputs.release();if(recoveryPending)recoveryPending=!recoverReach();bool restored=mags.restore();if(!recoveryPending&&restored&&std::none_of(inputs.owned.begin(),inputs.owned.end(),[](bool v){return v;})){auto done=stopPath;done+=L".done";std::ofstream(done).put('1');}Sleep(20);continue;}
  if(recoveryPending&&now()>=nextRecovery){recoveryPending=!recoverReach();nextRecovery=now()+1;}
  if(!any()||!ff3Native::admitted()){fastSnap.clear();publishVolumes(Scene{},mags.edits,false);snap.cancel("Hand Snap off");inputs.release();planner.reset();bool restored=mags.restore();publish(restored?"Receiving and defense assists off":"Restoring catch reach; retrying");Sleep(20);continue;}
  Config c=config();auto scene=reader.read(c);bool safe=observing(scene,now());std::string magStatus;
  scene.naturalShape=receivingShape(reader.h,scene.own);
  const bool ballScene=backgroundObserving(scene,now());
  const bool enabled=magsOn.load()&&!recoveryPending;
  auto selectedParts=reader.looseParts;
  if(enabled&&snap.busy()&&std::none_of(selectedParts.begin(),selectedParts.end(),[&](const auto& b){return b.part.id==snap.ball;})){
   const auto pinned=HandSnapAccess::observe(reader.h,scene,{},&snap);for(const auto& b:pinned){if(selectedParts.size()>=maxBallEdits)selectedParts.pop_back();selectedParts.push_back(b.part);}
  }
  const bool ballOk=updateBallMags(mags,scene,selectedParts,enabled,now(),false);
  if(magsOn){
   if(recoveryPending)magStatus="Previous ball/catch restoration pending";
   else if(!ballOk)magStatus=mags.edits.edits.empty()?"Ball Mags unavailable; restoration retrying":magsDescription()+" / "+std::to_string(mags.edits.edits.size())+" balls retained; retrying unavailable balls";
   else if(!enabled)magStatus="Ball Mags unavailable";else if(!ballScene)magStatus="Ball Mags paused";
   else magStatus=magsDescription()+" / "+std::to_string(mags.edits.edits.size())+" ground + air balls / player collisions filtered"+(mags.skippedBalls?" / "+std::to_string(mags.skippedBalls)+" unsupported balls skipped":"");
  }
  else if(!ballOk)magStatus="Restoring normal ball physics; retrying";
  if(magsOn&&ballScene)magStatus+=" / found "+std::to_string(scene.looseBalls.size())+" loose, "+std::to_string(selectedParts.size())+" selected, "+std::to_string(mags.edits.edits.size())+" applied"+(enabled&&!scene.focused?" / background active":"");
  if(safe)scene.shape=receivingShape(reader.h,scene.own);
  publishBallGuides(scene,mags.edits,selectedParts,enabled&&ballScene);
  if(safe&&c.catchOn&&!scene.shape.measured){const auto torso=ff3Native::partSize(reader.h,child(reader.h,scene.own,"Torso")),head=ff3Native::partSize(reader.h,child(reader.h,scene.own,"Head")),arm=ff3Native::partSize(reader.h,child(reader.h,scene.own,"Left Arm"));if(ff3qb::finite(torso)&&ff3qb::finite(head)&&ff3qb::finite(arm)&&std::min({torso.x,head.y,arm.y})>.1)scene.shape={torso.x*.5+arm.y+.75,arm.y+.5,arm.y+head.y+.75,true};}
  if(enabled)for(const auto& edit:mags.edits.edits)scene.magReach[edit.original.part.id]=std::max(0.,.5*std::min({edit.applied.x,edit.applied.y,edit.applied.z})-.5*std::max({edit.original.part.size.x,edit.original.part.size.y,edit.original.part.size.z}));
  auto decision=planner.step(c,scene,now());if(movementFrozen){decision.jumpNow=false;decision.moving=false;}
  if(enabled&&magsPickup&&allowed(scene,now())&&scene.shape.measured)for(const auto& ball:reader.groundParts){if(!magsFloorContact(scene,ball))continue;decision.catchHold=true;decision.catchNow=now()-planner.lastCatch>=c.cooldown;decision.ball=ball.part.id;decision.spot=ball.position;decision.eta=0;decision.reason="Hands ready for floor pickup";if(decision.catchNow)planner.lastCatch=now();break;}

  SnapCommand snapCommand;const auto snapTime=now();auto heldBall=ff3Native::Reader::possessedBall(reader.h,scene.own);const auto backpackBall=ff3Native::Reader::possessedBall(reader.h,child(reader.h,scene.player,"Backpack"));heldBall=snapPossessedBall(heldBall,backpackBall,snap);
  if(enabled&&ballOk&&!ff3Native::inputBusy){const auto hand=HandSnapAccess::contact(reader.h,scene);const auto balls=HandSnapAccess::observe(reader.h,scene,selectedParts,&snap);snapCommand=snap.step(scene,balls,hand,magsTeleportSize(),snapTime,inputs.owned[c.catchKey]||held(c.catchKey),heldBall);magStatus=magsDescription()+" / "+snap.result;}
  else {snap.cancel("Teleport paused: ball restoration, QB input or disabled");if(enabled)magStatus=ballOk?snap.result:"Mags: waiting for verified ball geometry";}
  auto inputScene=scene;if(!any()||!reader.stillSafe(scene))inputScene.valid=false;
  const bool inputOk=inputs.apply(c,inputScene,decision,now());if(!inputOk)decision.reason="Windows rejected an assist input";
  if(snapCommand.move){const bool current=magsOn&&!recoveryPending;if(!current||!fastSnap.submit(reader.h,scene,snapCommand,magsTeleportSize())){fastSnap.clear();magStatus="Hand Snap: verified fast tracking unavailable";}}
  else fastSnap.clear();
  snapAccess=fastSnap.snapshot();snap.writes=snapAccess.writes;snap.writeFailures=snapAccess.writeFailures;
  if(snapAccess.lastBall==snap.ball&&snapAccess.lastWriteAt>0){snap.lastMoveAt=snapAccess.lastWriteAt;snap.lastDestination=snapAccess.lastDestination;}
  if(snapCommand.move&&snapAccess.lastWriteAt>0&&now()-snapAccess.lastWriteAt<.1)magStatus=snapAccess.renderedNear?"Hand Snap: ball rendered near right hand; possession unconfirmed":"Hand Snap: fast tracking active; waiting for visible ball movement";
  publish(magStatus.empty()?decision.reason:magStatus+" · "+decision.reason);
  if(!diagnosticPath.empty()&&now()>=nextDiagnostic){nextDiagnostic=now()+.25;try{
   using namespace basketballUi;auto data=object();data.o[L"at"]=number(now());data.o[L"pid"]=number(scene.pid);data.o[L"valid"]=basketballUi::boolean(scene.valid);data.o[L"focused"]=basketballUi::boolean(scene.focused);data.o[L"backgroundMags"]=basketballUi::boolean(enabled&&ballScene&&!scene.focused);data.o[L"menu"]=basketballUi::boolean(scene.menu);data.o[L"typing"]=basketballUi::boolean(scene.typing);data.o[L"possessed"]=basketballUi::boolean(scene.possessed);data.o[L"grounded"]=basketballUi::boolean(scene.grounded);data.o[L"balls"]=number(double(scene.balls.size()));data.o[L"discoveredBalls"]=number(double(scene.looseBalls.size()));data.o[L"carriers"]=number(double(scene.carriers.size()));data.o[L"geometry"]=basketballUi::boolean(scene.shape.measured);data.o[L"handsRequested"]=basketballUi::boolean(decision.catchHold);data.o[L"catchInputHeld"]=basketballUi::boolean(inputs.owned[c.catchKey]);data.o[L"jumpRequested"]=basketballUi::boolean(decision.jumpNow);data.o[L"moving"]=basketballUi::boolean(decision.moving);data.o[L"combinedMags"]=basketballUi::boolean(true);data.o[L"snapRange"]=number(magsTeleportSize());auto snapState=object();snapState.o[L"phase"]=number(int(snap.phase));snapState.o[L"ball"]=number(double(snap.ball));snapState.o[L"attempts"]=number(snap.attempts);snapState.o[L"localWrites"]=number(snap.writes);snapState.o[L"motionBufferWrites"]=number(snapAccess.motionWrites);snapState.o[L"motionBuffer"]=number(double(snapAccess.motionPointer));snapState.o[L"fastMeanIntervalMs"]=number(snapAccess.meanIntervalMs);snapState.o[L"fastMaxIntervalMs"]=number(snapAccess.maximumIntervalMs);snapState.o[L"fastLastWriteAt"]=number(snapAccess.lastWriteAt);snapState.o[L"writeFailures"]=number(snap.writeFailures);snapState.o[L"contactSamples"]=number(snap.contacts);snapState.o[L"movedAwaySamples"]=number(snap.corrections);snapState.o[L"possessionObserved"]=number(snap.possessions);snapState.o[L"unconfirmed"]=number(snap.unconfirmed);snapState.o[L"cancelled"]=number(snap.cancelled);snapState.o[L"readbackNearHand"]=basketballUi::boolean(snapAccess.readbackNear);snapState.o[L"renderedNearHand"]=basketballUi::boolean(snapAccess.renderedNear);auto renderedPosition=array();for(double v:{snapAccess.renderedPosition.x,snapAccess.renderedPosition.y,snapAccess.renderedPosition.z})renderedPosition.a.push_back(number(v));snapState.o[L"renderedPositionAfterWrite"]=renderedPosition;snapState.o[L"backpackBall"]=number(double(backpackBall));snapState.o[L"equippedBall"]=number(double(ff3Native::Reader::possessedBall(reader.h,scene.own)));snapState.o[L"distanceBeforeWrite"]=number(snapAccess.lastDistance);auto origin=array(),destination=array();for(double v:{snapAccess.lastOrigin.x,snapAccess.lastOrigin.y,snapAccess.lastOrigin.z})origin.a.push_back(number(v));for(double v:{snapAccess.lastDestination.x,snapAccess.lastDestination.y,snapAccess.lastDestination.z})destination.a.push_back(number(v));snapState.o[L"originBeforeWrite"]=origin;snapState.o[L"targetPosition"]=destination;snapState.o[L"detail"]=string(widen(snap.result));data.o[L"handSnap"]=snapState;data.o[L"studs"]=number(reachTenths.load()*.1);auto dimensions=magsDimensions();auto volume=array();for(double v:{dimensions.x,dimensions.y,dimensions.z})volume.a.push_back(number(v));data.o[L"dimensions"]=volume;data.o[L"ballVolumes"]=number(double(mags.edits.edits.size()));data.o[L"skippedBalls"]=number(mags.skippedBalls);data.o[L"ballOnly"]=basketballUi::boolean(true);data.o[L"groundBalls"]=number(double(reader.groundParts.size()));data.o[L"groundReachActive"]=basketballUi::boolean(false);data.o[L"catchReachActive"]=basketballUi::boolean(false);data.o[L"reachBalls"]=number(double(reader.looseParts.size()));data.o[L"reachVisual"]=basketballUi::boolean(magsVisual);data.o[L"visibleBall"]=basketballUi::boolean(false);data.o[L"delayMs"]=number(magDelayMs);data.o[L"catchRefreshes"]=number(inputs.refreshCount);data.o[L"reason"]=string(widen(detail()));auto temp=diagnosticPath;temp+=L".tmp";{std::ofstream out(temp,std::ios::binary);out<<winrt::to_string(json::dump(data));}MoveFileExW(temp.c_str(),diagnosticPath.c_str(),MOVEFILE_REPLACE_EXISTING);
  }catch(...){}}
  Sleep(8);
 }
 fastSnap.clear();snap.cancel();inputs.release();mags.restore();
}
}
