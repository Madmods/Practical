#pragma once
#include "defense_logic.hpp"
#include "flight_logic.hpp"
#include <set>
namespace defense {
struct LiveFrame {Scene scene;Decision decision;std::string selectedName,detail;bool goalsInferred=false;int metadata=0;};
inline std::mutex frameMutex;inline LiveFrame frame;
inline LiveFrame snapshot(){std::lock_guard<std::mutex> lock(frameMutex);return frame;}
inline void publish(LiveFrame f){std::lock_guard<std::mutex> lock(frameMutex);frame=std::move(f);}
inline std::string textAt(HANDLE h,addr a){auto n=rd<addr>(h,a+16),cap=rd<addr>(h,a+24);if(!n||n>120||cap<n||cap>4096)return {};if(cap>=16)a=rd<addr>(h,a);char s[121]{};SIZE_T z=0;return ReadProcessMemory(h,(void*)a,s,n,&z)&&z==n?std::string(s,n):std::string{};}
inline std::string classOf(HANDLE h,addr o){auto d=rd<addr>(h,o+Offsets::Instance::ClassDescriptor);return textAt(h,rd<addr>(h,d+Offsets::Instance::ClassName));}
inline bool partClass(const std::string&s){return s=="Part"||s=="MeshPart"||s=="UnionOperation"||s=="WedgePart"||s=="TrussPart";}
inline std::string lower(std::string s){for(auto&c:s)c=(char)tolower((unsigned char)c);return s;}
struct Matrix3{float m[9]{};};
inline Vec forward(Matrix3 r){return {-r.m[2],-r.m[5],-r.m[8]};}
#include "defense_courts.hpp"
struct Reader {
 HANDLE h=nullptr;DWORD pid=0;addr own=0,ws=0;double scanAt=-10,goalsAt=-10;bool wasPlaying=false,seenPlaying=false;unsigned clickSerial=0;
 struct Rig{addr id,primitive,hum,goal,playing,teamPlayer;std::string name;bool enemy;};std::vector<Rig> rigs;
 struct Item{addr id,primitive,owner,shot,lob,boardSound;};std::vector<Item> items;
 struct Geometry{addr primitive;bool board;};std::vector<Geometry> geometry;
 struct History{Vec p{},v{};double at=0;};std::map<addr,History> actors;
 std::map<addr,ShotMeter> shotMeters;
 struct FlightState{Ball b;flight::Track track;Vec oldVelocity{};bool hadVelocity=false,soundPlaying=false;double seen=0;};std::map<addr,FlightState> flights;
 pump::GroundPlane ground;
 CourtRoster courtRoster;
 ~Reader(){close();}void close(){if(h)CloseHandle(h);h=nullptr;pid=0;own=ws=0;scanAt=goalsAt=-10;rigs.clear();items.clear();geometry.clear();actors.clear();shotMeters.clear();flights.clear();ground={};courtRoster={};seenPlaying=false;}
 addr value(addr o,const char*name,const char*type){addr v=path(h,o,{"Attributes",name});return v&&classOf(h,v)==type?v:0;}
 bool practiceContext(CourtMember membership,addr goal){
  return practiceCourtContext(true,courtRoster.practiceLayout(h),child(h,child(h,own,"Attributes"),"InGame")!=0,membership,goal);
 }
 bool practiceRig(const Rig& r,bool context,addr ownGoal){
  const addr goal=r.goal?rd<addr>(h,r.goal+Offsets::Misc::Value):0;
  if(r.id==own)return false;
  if(r.teamPlayer)return gymPracticePartner(context,true,child(h,child(h,r.id,"Attributes"),"InGame")!=0,ownGoal,goal,rd<addr>(h,scenePlayer.load()+Offsets::Player::Team),rd<addr>(h,r.teamPlayer+Offsets::Player::Team));
  return r.id!=own&&practiceCourtOpponent(context,practiceOpponent(r.name,r.teamPlayer!=0,r.hum!=0,goal!=0,on(PracticeAI)),ownGoal,goal);
 }
 Vec position(addr primitive){return rd<Vec>(h,primitive+Offsets::Primitive::Position,{NAN,NAN,NAN});}
 bool pointOf(addr o,Vec&p){if(!o)return false;if(partClass(classOf(h,o))){p=position(rd<addr>(h,o+Offsets::BasePart::Primitive));return finite(p);}for(const char*name:{"Rim","Goal","Hoop","Ring"}){auto c=child(h,o,name);if(c&&partClass(classOf(h,c))){p=position(rd<addr>(h,c+Offsets::BasePart::Primitive));return finite(p);}}return false;}
 void scanGeometry(double t){if(t-goalsAt<4)return;goalsAt=t;geometry.clear();std::vector<std::pair<addr,int>> queue{{ws,0}};std::set<addr> visited;for(size_t i=0;i<queue.size()&&i<4500;i++){auto [o,depth]=queue[i];if(!visited.insert(o).second)continue;auto name=lower(nm(h,o));if(name=="characters"||name=="basketballs"||name=="camera"||name=="terrain")continue;bool board=name.find("backboard")!=std::string::npos;bool rim=name=="rim"||name=="hoop"||name=="goal"||name=="ring";if((board||rim)&&partClass(classOf(h,o))){auto p=rd<addr>(h,o+Offsets::BasePart::Primitive);auto size=rd<Vec>(h,p+Offsets::Primitive::Size);if(p&&finite(position(p))&&size.x>0&&size.x<40&&size.y>0&&size.y<30&&size.z>0&&size.z<40)geometry.push_back({p,board});}if(depth<10&&queue.size()<5000)for(auto c:children(h,o))queue.push_back({c,depth+1});}}
 void scan(double t,addr model){own=model;ws=child(h,sceneRoot.load(),"Workspace");rigs.clear();items.clear();scanAt=t;std::map<std::string,addr> teams;for(auto p:children(h,child(h,sceneRoot.load(),"Players")))teams[nm(h,p)]=p;
  courtRoster.scan(h,ws,teams);const auto members=courtRoster.read(h);const auto ownMember=members.get(scenePlayer.load());
  const addr ownGoalValue=value(own,"Goal","ObjectValue"),ownGoal=ownGoalValue?rd<addr>(h,ownGoalValue+Offsets::Misc::Value):0;
  const bool practice=practiceContext(ownMember,ownGoal);
  for(auto m:children(h,child(h,ws,"Characters"))){
   auto name=nm(h,m);if(name=="InvisCharacter")continue;
   auto hum=child(h,m,"Humanoid"),prim=rd<addr>(h,child(h,m,"HumanoidRootPart")+Offsets::BasePart::Primitive);
   if(!hum||!prim||rd<float>(h,hum+Offsets::Humanoid::Health)<=0)continue;
   addr teamPlayer=teams.count(name)?teams[name]:0;
   auto goal=value(m,"Goal","ObjectValue"),playing=value(m,"InGame","BoolValue");
   Rig candidate{m,prim,hum,goal,playing,teamPlayer,name,false};
   if(courtRoster.available&&m!=own&&!sameCourt(ownMember,members.get(teamPlayer))&&!practiceRig(candidate,practice,ownGoal))continue;
   // Unmapped placeholder rigs with no explicit court assignment cannot crowd out players.
   if(m!=own&&!teamPlayer&&(!goal||!rd<addr>(h,goal+Offsets::Misc::Value)))continue;
   rigs.push_back({m,prim,hum,goal,playing,teamPlayer,name,false});
  }
  std::stable_sort(rigs.begin(),rigs.end(),[&](const Rig&a,const Rig&b){
   auto priority=[&](const Rig&r){return r.id==own?0:r.teamPlayer?1:2;};return priority(a)<priority(b);
  });
  if(rigs.size()>64)rigs.resize(64);
  const auto courtBall=courtRoster.ball(h,ownMember);
  for(auto b:children(h,child(h,ws,"Basketballs"))){if(courtRoster.available&&b!=courtBall&&!practice)continue;if(!partClass(classOf(h,b)))continue;auto prim=rd<addr>(h,b+Offsets::BasePart::Primitive);if(prim)items.push_back({b,prim,value(b,"Character","ObjectValue"),value(b,"LastShotType","StringValue"),value(b,"Lob","BoolValue"),path(h,b,{"Attachment","Sounds","Backboard"})});if(items.size()>=32)break;}
 }
 LiveFrame read(double t){LiveFrame out;DWORD current=gamePid.load();if(current!=pid){close();pid=current;h=pid?OpenProcess(PROCESS_VM_READ|SYNCHRONIZE,FALSE,pid):nullptr;}if(!h||WaitForSingleObject(h,0)!=WAIT_TIMEOUT){out.detail="Waiting for supported Roblox process";return out;}auto model=sceneOwn.load();if(!model){out.detail="Waiting for character";return out;}if(model!=own){ground={};actors.clear();shotMeters.clear();flights.clear();seenPlaying=false;selected=0;goalsAt=-10;}if(t-scanAt>.3||model!=own)scan(t,model);scanGeometry(t);
  Scene&s=out.scene;s.at=t;addr ownHum=0,ownPrim=0;float clearance=3;const addr currentTeam=rd<addr>(h,scenePlayer.load()+Offsets::Player::Team);const auto ownGoalValue=value(model,"Goal","ObjectValue");const addr assignedGoal=ownGoalValue?rd<addr>(h,ownGoalValue+Offsets::Misc::Value):0;
  const auto courtMembers=courtRoster.read(h);const auto ownMembership=courtMembers.get(scenePlayer.load());const addr currentCourtBall=courtRoster.ball(h,ownMembership);
  const bool practice=practiceContext(ownMembership,assignedGoal);bool practicePresent=false;
  for(auto&r:rigs){const bool practiceBot=practiceRig(r,practice,assignedGoal);if(courtRoster.available&&r.id!=own&&!sameCourt(ownMembership,courtMembers.get(r.teamPlayer))&&!practiceBot)continue;if(rd<float>(h,r.hum+Offsets::Humanoid::Health)<=0)continue;Actor a;a.id=r.id;a.name=r.name;a.p=position(r.primitive);if(!finite(a.p))continue;a.v=rd<Vec>(h,r.primitive+Offsets::Primitive::AssemblyLinearVelocity);auto it=actors.find(a.id);if(it!=actors.end()){double dt=t-it->second.at;if(dt>.008&&dt<.15){Vec measured=(a.p-it->second.p)*(float)(1/dt);if(finite(measured)&&length(measured)<100)a.v=it->second.v*.3f+measured*.7f;}}if(!finite(a.v)||length(a.v)>150)a.v={};actors[a.id]={a.p,a.v,t};a.enemy=opposingMatchup(currentTeam,r.teamPlayer?rd<addr>(h,r.teamPlayer+Offsets::Player::Team):0,assignedGoal,r.goal?rd<addr>(h,r.goal+Offsets::Misc::Value):0,r.id==own);if(courtRoster.available)a.enemy=practiceBot||courtOpponent(ownMembership,courtMembers.get(r.teamPlayer),r.id==own);if(!courtRoster.available&&r.id!=own&&practiceOpponent(r.name,r.teamPlayer!=0,r.hum!=0,r.goal&&rd<addr>(h,r.goal+Offsets::Misc::Value)!=0,on(PracticeAI)))a.enemy=true;a.facing=unit(flat(forward(rd<Matrix3>(h,r.primitive+Offsets::Primitive::Rotation))));a.speed=std::clamp(rd<float>(h,r.hum+Offsets::Humanoid::Walkspeed,16),8.f,40.f);if(!std::isfinite(a.speed))a.speed=16;
   auto action=value(r.id,"Action","StringValue");auto actionName=action?textAt(h,action+Offsets::Misc::Value):std::string{};a.shooting=actionName=="Shooting"||actionName=="Dunking";
   auto released=value(r.id,"ReleasedShot","BoolValue");a.shotReleased=released&&rd<unsigned char>(h,released+Offsets::Misc::Value)==1;
   if(r.goal)a.goalValid=pointOf(rd<addr>(h,r.goal+Offsets::Misc::Value),a.goal);
   if(r.id==own){s.own=a;ownHum=r.hum;ownPrim=r.primitive;if(r.playing){s.playingKnown=true;s.playing=rd<unsigned char>(h,r.playing+Offsets::Misc::Value)==1;}float hip=rd<float>(h,r.hum+Offsets::Humanoid::HipHeight,NAN);auto size=rd<Vec>(h,r.primitive+Offsets::Primitive::Size);if(std::isfinite(hip)&&hip>0&&hip<10&&size.y>.1&&size.y<10)clearance=hip+size.y*.5f;}else {s.actors.push_back(a);practicePresent=practicePresent||practiceBot;}
  }
  const bool practiceSession=practice&&practicePresent;
  if(courtRoster.available&&!practiceSession){const bool permitted=!s.playingKnown||s.playing;s.playingKnown=true;s.playing=permitted&&ownMembership.valid();}
  if(!s.own.id){out.detail="Character transform unavailable";return out;}s.floor=ground.update(s.own.p.y-clearance,s.own.v.y,t);int material=rd<int>(h,ownHum+Offsets::Humanoid::FloorMaterial);s.grounded=material>0&&material!=256&&std::abs(s.own.v.y)<2&&std::abs(s.own.p.y-clearance-s.floor)<.4f;
  s.actors.erase(std::remove_if(s.actors.begin(),s.actors.end(),[&](auto&a){return std::abs(a.p.y-s.own.p.y)>14||distance(a.p,s.own.p)>75;}),s.actors.end());
  auto goalFor=[&](Actor&a){if(a.goalValid)return;float best=80;for(auto&g:geometry)if(!g.board){auto p=position(g.primitive);float height=p.y-s.floor,dist=distance(a.p,p);if(height>6&&height<22&&dist<best){a.goal=p;a.goalValid=true;best=dist;out.goalsInferred=true;}}};goalFor(s.own);for(auto&a:s.actors)goalFor(a);
  auto camera=rd<addr>(h,ws+Offsets::Workspace::CurrentCamera);auto r=rd<Matrix3>(h,camera+Offsets::Camera::Rotation);s.cameraForward=unit(flat(forward(r)));s.cameraRight=unit(flat(Vec{r.m[0],r.m[3],r.m[6]}));
  float gravity=rd<float>(h,ws+Offsets::Workspace::ReadOnlyGravity,NAN);if(!std::isfinite(gravity)||gravity<1||gravity>300){out.detail="Gravity unavailable";return out;}
  for(auto&it:items){addr holder=it.owner?rd<addr>(h,it.owner+Offsets::Misc::Value):0;if(courtRoster.available&&(!currentCourtBall||it.id!=currentCourtBall)){
    if(!practiceSession)continue;if(holder&&!actor(s,holder)){flights.erase(it.id);continue;}const auto previous=flights.find(it.id);
    const bool knownHolder=holder&&actor(s,holder);
    const bool knownFlight=!holder&&previous!=flights.end()&&t>=previous->second.seen&&t-previous->second.seen<=.15&&actor(s,previous->second.b.previousOwner)&&(previous->second.b.owner||t-previous->second.b.releasedAt<4);
    if(!knownHolder&&!knownFlight)continue;
   }Vec p=position(it.primitive);if(!finite(p)||distance(p,s.own.p)>100)continue;auto&f=flights[it.id];Ball b=f.b;b.id=it.id;b.p=p;b.gravity=gravity;if(holder&&!actor(s,holder))holder=0;b.owner=holder;b.heldRiseSpeed=0;double sampleDt=t-f.seen;if(holder&&holder==f.b.owner&&sampleDt>.008&&sampleDt<.12){float rise=(p.y-f.b.p.y)/(float)sampleDt;if(std::isfinite(rise)&&std::abs(rise)<100)b.heldRiseSpeed=rise;}f.seen=t;
   if(holder){if(holder!=f.b.owner){++b.flight;b.boardHit=false;b.meterObserved=false;}b.previousOwner=holder;b.releasedAt=-10;b.lob=b.shot=false;f.track.reset();b.v={};b.velocityValid=false;}
   else{if(f.b.owner){++b.flight;b.previousOwner=f.b.owner;b.releasedAt=t;b.boardHit=false;f.track.reset();f.hadVelocity=false;}auto fit=f.track.add(t,p,gravity);b.velocityValid=fit.valid;b.v=fit.valid?fit.v:Vec{};
    std::string kind=it.shot?lower(textAt(h,it.shot+Offsets::Misc::Value)):"";if(!kind.empty())out.metadata++;if(kind.find("lob")!=std::string::npos||it.lob&&rd<unsigned char>(h,it.lob+Offsets::Misc::Value)==1)b.lob=true;
    if(kind=="shot"||kind=="layup"||kind=="dunk")b.shot=true;auto owner=actor(s,b.previousOwner);if(kind.empty()&&b.velocityValid&&t-b.releasedAt<.3&&owner&&owner->goalValid){Vec toward=unit(flat(owner->goal-p));if(b.v.y>6&&dot(unit(flat(b.v)),toward)>.6&&p.y>owner->p.y+1)b.shot=true;}
    bool sound=it.boardSound&&classOf(h,it.boardSound)=="Sound"&&rd<unsigned char>(h,it.boardSound+Offsets::Sound::IsPlaying)==1;if(sound&&!f.soundPlaying&&b.previousOwner&&t-b.releasedAt<3){b.boardHit=true;b.boardAt=t;if(b.velocityValid&&owner&&dot(unit(flat(owner->p-p)),unit(flat(b.v)))>.35f)b.lob=true;}f.soundPlaying=sound;
    if(b.velocityValid&&f.hadVelocity){float reversal=dot(flat(f.oldVelocity),flat(b.v));for(auto&g:geometry)if(g.board){Vec bp=position(g.primitive),sz=rd<Vec>(h,g.primitive+Offsets::Primitive::Size);auto rot=rd<Matrix3>(h,g.primitive+Offsets::Primitive::Rotation);Vec rel=p-bp;Vec local{rel.x*rot.m[0]+rel.y*rot.m[3]+rel.z*rot.m[6],rel.x*rot.m[1]+rel.y*rot.m[4]+rel.z*rot.m[7],rel.x*rot.m[2]+rel.y*rot.m[5]+rel.z*rot.m[8]};if(std::abs(local.x)<sz.x*.5f+2&&std::abs(local.y)<sz.y*.5f+2&&std::abs(local.z)<sz.z*.5f+2&&reversal<0&&length(flat(f.oldVelocity))>3){b.boardHit=true;b.boardAt=t;if(!b.shot)b.lob=true;break;}}}if(b.velocityValid){f.oldVelocity=b.v;f.hadVelocity=true;}
   }f.b=b;s.balls.push_back(b);
  }
  // Sample only current enemy ball holders after ownership has been resolved.
  // Same version-checked meter path/layout as the application's own shot meter.
  std::set<addr> metered;
  for(auto& a:s.actors)if(a.enemy&&heldBall(s,a.id)){
   auto gradient=path(h,a.id,{"HumanoidRootPart","BatMeter","Meter_Fill","FillGradient"});
   if(!gradient||classOf(h,gradient)!="UIGradient")gradient=0;
   float fill=gradient?-rd<float>(h,gradient+0x12c,NAN):NAN;
   auto& meter=shotMeters[a.id];a.shotMeterActive=meter.update(gradient,fill,t);a.shotMeterFill=std::isfinite(fill)?fill:0;a.shotMeterAge=a.shotMeterActive?std::max(0.,t-meter.activeSince):0;a.shotMeterAdvancing=a.shotMeterActive&&t-meter.lastRise<=.07;
   // A first observed charge increase is enough to close space, but never to
   // jump: blocks still require confirmed advancing charge and the lift gates.
   a.shotMeterRising=meter.rises>0&&t-meter.lastRise<=.07&&fill>.02f&&fill<.65f;
   if(a.shotMeterActive)for(auto& b:s.balls)if(b.owner==a.id){b.meterObserved=true;flights[b.id].b.meterObserved=true;}metered.insert(a.id);
  }
  for(auto it=shotMeters.begin();it!=shotMeters.end();)if(!metered.count(it->first))it=shotMeters.erase(it);else ++it;
  for(auto it=flights.begin();it!=flights.end();)if(t-it->second.seen>3)it=flights.erase(it);else ++it;
  for(auto it=actors.begin();it!=actors.end();)if(t-it->second.at>3)it=actors.erase(it);else ++it;
  s.valid=finite(s.own.p)&&length(s.cameraForward)>.5f&&length(s.cameraRight)>.5f&&std::abs(dot(s.cameraForward,s.cameraRight))<.2f;
  if(s.playingKnown){if(s.playing&&seenPlaying&&!wasPlaying&&on(SelectAtStart)&&(on(Offball)||on(Lock)))pick=true;wasPlaying=s.playing;seenPlaying=true;}
  if(auto a=actor(s,selected.load());a&&a->enemy)out.selectedName=a->name;else if(selected)selected=0;
  out.detail=!s.valid?"Camera basis unavailable":items.empty()?"No basketball ownership data":out.goalsInferred?"Goal: nearest rim estimate | Flight: observed motion":"Goal: mirrored data | Flight: observed motion";return out;
 }
};
inline bool sendKey(int key,bool down){INPUT i{};if(key==VK_MBUTTON||key==VK_XBUTTON1||key==VK_XBUTTON2){i.type=INPUT_MOUSE;i.mi.dwFlags=key==VK_MBUTTON?(down?MOUSEEVENTF_MIDDLEDOWN:MOUSEEVENTF_MIDDLEUP):(down?MOUSEEVENTF_XDOWN:MOUSEEVENTF_XUP);i.mi.mouseData=key==VK_XBUTTON1?XBUTTON1:key==VK_XBUTTON2?XBUTTON2:0;}else{UINT scan=MapVirtualKeyW(key,MAPVK_VK_TO_VSC_EX);if(!scan)return false;i.type=INPUT_KEYBOARD;i.ki.wScan=scan&255;i.ki.dwFlags=KEYEVENTF_SCANCODE|(down?0:KEYEVENTF_KEYUP)|((scan&0xff00)?KEYEVENTF_EXTENDEDKEY:0);}return SendInput(1,&i,sizeof i)==1;}
using KeySender=bool(*)(int,bool);inline KeySender inputSender=sendKey;
struct Inputs {
 std::array<bool,256> owned{};std::array<double,256> until{};bool failure=false;
 void pulse(int k,double t,bool repeatHeld=false){if(k<=0||k>=255||k==shootKey.load())return;if(held(k)){if(repeatHeld){bool up=inputSender(k,false),down=inputSender(k,true);failure=failure||!up||!down;}return;}until[k]=t+.065;}
 void apply(const Config&c,const Scene&s,const Intent&i,const Decision&d,double t){std::array<bool,256> want{};bool allowed=i.focused&&!i.chat&&s.valid&&t-s.at<.12&&!pick;
  if(allowed){if(d.moving){float f=dot(d.direction,s.cameraForward),r=dot(d.direction,s.cameraRight);want['W']=movementAxis(f,owned['W']);want['S']=movementAxis(-f,owned['S']);want['D']=movementAxis(r,owned['D']);want['A']=movementAxis(-r,owned['A']);
    // Never send a contradictory direction to an explicitly held movement key.
    if(held('W'))want['S']=false;if(held('S'))want['W']=false;if(held('A'))want['D']=false;if(held('D'))want['A']=false;}
   want[VK_LSHIFT]=d.sprint;int guard=c.values[NativeGuard];if(guard>0&&guard<255&&guard!=shootKey.load())want[guard]=d.stance;
   if(d.jump)pulse(c.values[JumpKey],t);if(d.steal)pulse(c.values[StealKey],t,true);
  }else until.fill(0);
  for(int k=1;k<255;k++){bool edge=changed[k].exchange(false);if(edge&&!held(k))owned[k]=false;bool need=allowed&&(want[k]||until[k]>t);if(need&&!owned[k]&&!held(k)){if(inputSender(k,true))owned[k]=true;else failure=true;}
   else if(!need&&owned[k]){if(held(k)){owned[k]=false;continue;}if(inputSender(k,false))owned[k]=false;else failure=true;}}
 }
 void release(){until.fill(0);for(int k=1;k<255;k++)if(owned[k]){if(!held(k))inputSender(k,false);owned[k]=false;}}
 ~Inputs(){release();}
};
inline void worker(){Reader reader;Planner planner;Inputs inputs;bool previousSwitch=false;std::wstring logPath;wchar_t exe[MAX_PATH];if(GetModuleFileNameW(nullptr,exe,MAX_PATH)){logPath=exe;logPath.resize(logPath.find_last_of(L"\\")+1);logPath+=L"defense-trace.csv";}int logged=0,lastMode=-1;double lastLog=-10;while(running){double start=now();if(!any()&&!pick&&!selected){inputs.release();planner.reset();LiveFrame idle;idle.decision.reason="Enable a defensive feature to begin";publish(std::move(idle));Sleep(30);continue;}
  auto f=reader.read(start);Config c;c.snapshot();Intent intent;intent.focused=gameFocused();intent.activation=held(get(Activation));intent.catchHeld=held(get(InterceptKey));intent.manual=held('W')||held('A')||held('S')||held('D');intent.chat=chat;bool switchHeld=held(get(SwitchKey));if(switchHeld&&!previousSwitch&&intent.focused&&!intent.chat&&(on(Offball)||on(Lock))){std::vector<const Actor*> opponents;for(auto&a:f.scene.actors)if(a.enemy)opponents.push_back(&a);std::sort(opponents.begin(),opponents.end(),[&](auto a,auto b){return distance(a->p,f.scene.own.p)<distance(b->p,f.scene.own.p);});if(!opponents.empty()){size_t next=0;for(size_t n=0;n<opponents.size();n++)if(opponents[n]->id==selected)next=(n+1)%opponents.size();selected=opponents[next]->id;}}previousSwitch=switchHeld;intent.selected=selected;
  if(pick){f.decision.mode=Selecting;f.decision.reason="Click an opponent to select your matchup";inputs.release();}else{f.decision=planner.step(c,f.scene,intent,now());inputs.apply(c,f.scene,intent,f.decision,now());if(inputs.failure)f.decision.reason="Windows rejected an input request";}
  if(logged<4000&&!logPath.empty()&&(f.decision.mode!=lastMode||f.decision.jump||f.decision.steal||f.decision.moving&&start-lastLog>.25)){
   std::ofstream out(logPath.c_str(),logged?std::ios::app:std::ios::out);
   if(!logged)out<<"time,mode,fresh,opponents,balls,target,move,jump_request,steal_request,reason,jump_owner_present,meter_fill,meter_advancing,release_flag,ball_height,ball_distance,block_gate\n";
   const Ball* jumpBall=nullptr;for(auto& b:f.scene.balls)if(f.decision.jump&&b.id==f.decision.ball){jumpBall=&b;break;}
   auto shooter=jumpBall?actor(f.scene,jumpBall->owner?jumpBall->owner:jumpBall->previousOwner):nullptr;
   out.precision(12);out<<start<<","<<f.decision.mode<<","<<f.scene.valid<<","<<f.scene.actors.size()<<","<<f.scene.balls.size()<<","<<f.decision.target<<","<<f.decision.moving<<","<<f.decision.jump<<","<<f.decision.steal<<",\""<<f.decision.reason<<"\","<<(jumpBall&&jumpBall->owner)<<","<<(shooter?shooter->shotMeterFill:-1)<<","<<(shooter&&shooter->shotMeterAdvancing)<<","<<(shooter&&shooter->shotReleased)<<","<<(jumpBall?jumpBall->p.y-f.scene.own.p.y:-1)<<","<<(jumpBall?distance(jumpBall->p,f.scene.own.p):-1)<<",\""<<f.decision.blockReason<<"\"\n";
   lastMode=f.decision.mode;lastLog=start;++logged;
  }
  publish(std::move(f));double elapsed=now()-start;if(elapsed<.02)Sleep((DWORD)std::max(1.,(.02-elapsed)*1000));
 }inputs.release();publish({});}
}





