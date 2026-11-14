#pragma once
#include "ff3_qb_logic.hpp"
#include "basketball_ui_bridge.hpp"
#include <iomanip>
#include <functional>
namespace ff3Native {
using ff3qb::Vec;
inline std::atomic<bool> enabled{false},autoPower{true},practice{true},visuals{true};
inline std::atomic<int> arc{0};
inline std::atomic<int> passMode{0},manualAngle{35},selectionRadius{180};
inline std::atomic<bool> automaticAngle{true},smartFit{true},showHud{true},showCandidates{true};
inline std::atomic<unsigned> revision{0};
inline std::filesystem::path logPath;
inline HHOOK mouse=nullptr;
inline HHOOK keyboard=nullptr;
inline constexpr bool packetMode=true;
inline std::atomic<ULONGLONG> physicalClickAt{0};
struct Frame {
 unsigned revision=0;
 double at=0,gravity=0,power=0,gamePower=0,ping=0,margin=0,distance=0;DWORD pid=0;addr model=0,own=0,tool=0,target=0;
 HWND window=nullptr;RECT client{};POINT origin{},cursor{};float matrix[16]{};
 ff3qb::Solution solution;ff3qb::Screen aim,receiver;
 Vec gameMouse{NAN,NAN,NAN};
 Vec head{},targetPosition{},targetVelocity{};
 Vec fitOffset{};bool coverageKnown=false,wheelVisible=false;
 int mode=0;std::string route;
 struct Candidate{ff3qb::Screen screen;std::string name;bool selected=false;};std::vector<Candidate> candidates;
 std::string name,reason="Enable QB aim to begin";bool held=false,ready=false,textFocus=false;
};
inline std::mutex mutex;inline Frame current,requested;inline ff3qb::ClickGate clicks;
inline std::function<void(const Frame&)> frameSink;
inline std::function<void()> packetStop;
inline std::function<std::pair<bool,std::wstring>()> packetStatus;
inline unsigned throwCount=0;inline std::string lastAction;inline double actionAt=0;
inline void record(const Frame& f,const char* action,bool sent=false){
 {std::lock_guard<std::mutex> lock(mutex);lastAction=action;actionAt=now();if(sent)++throwCount;}
 if(!logPath.empty()){std::ofstream out(logPath,std::ios::app);if(out)out<<std::setprecision(12)<<now()<<','<<action<<','<<f.pid<<','<<f.target<<','<<f.power<<','<<f.gravity<<','<<f.ping<<','<<f.solution.angle<<','<<f.solution.time<<','<<f.solution.origin.x<<','<<f.solution.origin.y<<','<<f.solution.origin.z<<','<<f.solution.velocity.x<<','<<f.solution.velocity.y<<','<<f.solution.velocity.z<<','<<f.aim.x<<','<<f.aim.y<<','<<f.gameMouse.x<<','<<f.gameMouse.y<<'\n';}
}
inline Frame snapshot(){std::lock_guard<std::mutex> lock(mutex);return current;}
inline void publish(Frame f){{std::lock_guard<std::mutex> lock(mutex);current=f;}if(frameSink)frameSink(f);}
inline bool admitted(){return basketballConnection::entered&&basketballConnection::releaseCurrent&&gameModules::router.active(now(),true)==gameModules::Id::FootballFusion3;}
inline bool focused(DWORD pid){DWORD currentPid=0;GetWindowThreadProcessId(GetForegroundWindow(),&currentPid);return pid&&pid==currentPid;}
inline bool freeCursor(){CURSORINFO info{sizeof info};return GetCursorInfo(&info)&&ff3qb::cursorModeAllowed((info.flags&CURSOR_SHOWING)!=0,(GetAsyncKeyState(VK_RBUTTON)&0x8000)!=0,false);}
inline Vec vector(shooting::Vec v){return {v.x,v.y,v.z};}
inline Vec position(HANDLE h,addr part){auto primitive=rd<addr>(h,part+Offsets::BasePart::Primitive);return vector(rd<shooting::Vec>(h,primitive+Offsets::Primitive::Position,{NAN,NAN,NAN}));}
inline Vec velocity(HANDLE h,addr part){return vector(rd<shooting::Vec>(h,rd<addr>(h,part+Offsets::BasePart::Primitive)+Offsets::Primitive::AssemblyLinearVelocity,{NAN,NAN,NAN}));}
inline bool descendant(HANDLE h,addr part,addr parent){return part&&parent&&rd<addr>(h,part+Offsets::Instance::Parent)==parent;}
struct Reader {
 HANDLE handle=nullptr;DWORD pid=0;addr model=0,moduleBase=0,retained=0;double scanAt=-10;
 struct Actor{addr rig=0,player=0,team=0;bool npc=false;};std::vector<Actor> actors;
 ~Reader(){if(handle)CloseHandle(handle);}
 Frame read(addr forced=0){
  Frame f;f.at=now();f.revision=revision;f.mode=passMode;auto session=gameModules::router.get();f.pid=session.pid;f.model=session.model;
  if(!admitted()){f.reason="Connect to Football Fusion 3";return f;}
  if(pid!=f.pid){if(handle)CloseHandle(handle);handle=OpenProcess(PROCESS_VM_READ|SYNCHRONIZE,FALSE,f.pid);pid=f.pid;model=moduleBase=0;}
  if(!handle||WaitForSingleObject(handle,0)!=WAIT_TIMEOUT){f.reason="Waiting for the game process";return f;}
  auto h=handle;auto root=f.model;auto ws=child(h,root,"Workspace"),players=child(h,root,"Players");
  auto player=rd<addr>(h,players+Offsets::Player::LocalPlayer);f.own=rd<addr>(h,player+Offsets::Player::ModelInstance);
  auto head=child(h,f.own,"Head"),hum=child(h,f.own,"Humanoid");
  const float health=rd<float>(h,hum+Offsets::Humanoid::Health,NAN);
  if(!f.own||!std::isfinite(health)||health<=0){f.reason="Waiting for your live character";return f;}
  auto ui=child(h,root,"UserInputService"),input=rd<addr>(h,ui+Offsets::UserInputService::WindowInputState);
  f.textFocus=!input||rd<addr>(h,input+Offsets::WindowInputState::CurrentTextBox)!=0;
  if(f.textFocus){f.reason="Typing or input data unavailable";return f;}
  f.tool=child(h,f.own,"Football");auto ball=child(h,f.tool,"Handle");
  // Match renamed equipped footballs by their physics-bearing handle.
  if(!ball)for(auto item:children(h,f.own))if(defense::classOf(h,item)=="Tool"){
   auto handlePart=child(h,item,"Handle"),value=path(h,handlePart,{"Configuration","Gravity"});
   if(handlePart&&value&&defense::classOf(h,value)=="NumberValue"){f.tool=item;ball=handlePart;break;}
  }
  f.held=f.tool&&defense::classOf(h,f.tool)=="Tool"&&descendant(h,f.tool,f.own)&&descendant(h,ball,f.tool);
  if(!f.held){f.reason="Equip a football";return f;}
  const auto gravity=path(h,ball,{"Configuration","Gravity"});
  f.gravity=-rd<double>(h,gravity+Offsets::Misc::Value,NAN);
  if(!gravity||defense::classOf(h,gravity)!="NumberValue"||!std::isfinite(f.gravity)||f.gravity<1||f.gravity>300){f.reason="Ball physics unavailable";return f;}
  for(auto gui:children(h,child(h,player,"PlayerGui")))if(nm(h,gui)=="BallGui"&&rd<unsigned char>(h,gui+Offsets::GuiObject::ScreenGui_Enabled)==1){
   auto frame=child(h,gui,"Frame0"),display=child(h,frame,"Disp");auto text=defense::textAt(h,display+Offsets::GuiObject::Text);
   int value=ff3qb::visiblePower(1,rd<unsigned char>(h,frame+Offsets::GuiObject::Visible),text);
   if(value<0){f.reason="Visible throw power is invalid";return f;}if(!value)continue;
   if(f.power&&f.power!=value){f.reason="Power displays disagree";return f;}f.power=value;
  }
  if(!f.power){f.reason="Waiting for visible throw power";return f;}
  f.gamePower=f.power;f.wheelVisible=true;f.mode=passMode;
  auto ping=path(h,root,{"Stats","PerformanceStats","Ping"});double pingValue=rd<double>(h,ping+0x98,NAN);
  if(!std::isfinite(pingValue)||pingValue<1||pingValue>400){f.reason="Waiting for usable ping";return f;}f.ping=pingValue;
  f.window=GetForegroundWindow();if(!focused(f.pid)){f.reason="Focus Football Fusion 3";return f;}
  if(!GetClientRect(f.window,&f.client)||!ClientToScreen(f.window,&f.origin)||!GetCursorPos(&f.cursor)){f.reason="Waiting for viewport";return f;}
  const int width=f.client.right,height=f.client.bottom;
  auto camera=rd<addr>(h,ws+Offsets::Workspace::CurrentCamera);Vec cameraPosition=vector(rd<shooting::Vec>(h,camera+Offsets::Camera::Position,{NAN,NAN,NAN}));
  if(!packetMode&&(!freeCursor()||rd<int>(h,player+Offsets::Player::CameraMode,-1)!=0||ff3qb::length(cameraPosition-position(h,head))<2.0)){f.reason="Use third-person with shift-lock off; release right mouse";return f;}
  if(f.cursor.x<f.origin.x||f.cursor.y<f.origin.y||f.cursor.x>=f.origin.x+width||f.cursor.y>=f.origin.y+height){f.reason="Move the cursor into the game";return f;}
  auto mouseService=child(h,root,"MouseService"),mouseInput=rd<addr>(h,mouseService+Offsets::MouseService::InputObject);
  f.gameMouse=vector(rd<shooting::Vec>(h,mouseInput+Offsets::MouseService::MousePosition,{NAN,NAN,NAN}));
  if(!packetMode&&(!ff3qb::finite(f.gameMouse)||f.gameMouse.x<0||f.gameMouse.x>width||f.gameMouse.y<0||f.gameMouse.y>height)){f.reason="Game cursor data unavailable";return f;}
  // Project using camera view/projection from the same version-gated process.
  // Resolve once per process, independently of Basketball's cleared scene.
  if(!moduleBase){HANDLE modules=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32,pid);MODULEENTRY32W entry{sizeof entry};if(modules!=INVALID_HANDLE_VALUE){if(Module32FirstW(modules,&entry))moduleBase=(addr)entry.modBaseAddr;CloseHandle(modules);}}
  const auto visual=rd<addr>(h,moduleBase+Offsets::VisualEngine::Pointer);
  struct Matrix{float m[16];};Matrix invalid;for(auto& v:invalid.m)v=NAN;const auto matrix=rd<Matrix>(h,visual+Offsets::VisualEngine::ViewMatrix,invalid);std::copy(std::begin(matrix.m),std::end(matrix.m),f.matrix);
  if(!ff3qb::finite(cameraPosition)||!visual||!std::all_of(std::begin(f.matrix),std::end(f.matrix),[](float v){return std::isfinite(v);})){f.reason="Camera data unavailable";return f;}
  const auto ownTeam=rd<addr>(h,player+Offsets::Player::Team);const bool practicePlace=session.placeId==81310542478972ULL;
  if(model!=root||f.at-scanAt>.25){
   actors.clear();model=root;scanAt=f.at;
   for(auto p:children(h,players)){auto rig=rd<addr>(h,p+Offsets::Player::ModelInstance);if(p!=player&&rig)actors.push_back({rig,p,rd<addr>(h,p+Offsets::Player::Team),false});}
   if(practicePlace)for(auto group:children(h,child(h,ws,"npcwr")))for(auto rig:children(h,group))if(defense::classOf(h,rig)=="Model"&&child(h,rig,"WR"))actors.push_back({rig,0,0,true});
  }
  Vec origin=position(h,head);f.head=origin;double best=std::min(double(selectionRadius.load()),std::min(width,height)*.35);
  std::vector<ff3qb::Defender> defenders;
  if(ownTeam)for(const auto& a:actors)if(!a.npc&&rd<addr>(h,a.player+Offsets::Player::Team)!=ownTeam){
   auto hrp=child(h,a.rig,"HumanoidRootPart"),humanoid=child(h,a.rig,"Humanoid");
   auto p=position(h,hrp)+Vec{0,1.5,0},v=velocity(h,hrp);
   if(hrp&&rd<float>(h,humanoid+Offsets::Humanoid::Health,0)>0&&ff3qb::finite(p)&&ff3qb::finite(v)&&ff3qb::length(v)<=40)defenders.push_back({p,v});
  }
  Vec targetPosition{},targetVelocity{};
  for(const auto& a:actors){
   if(forced&&a.rig!=forced)continue;
   if(a.npc){if(!practice||!practicePlace||!rd<bool>(h,child(h,a.rig,"WR")+Offsets::Misc::Value))continue;}
   else if(ownTeam){if(rd<addr>(h,a.player+Offsets::Player::Team)!=ownTeam)continue;}
   else if(!practice||!practicePlace||rd<addr>(h,a.player+Offsets::Player::Team)!=0)continue;
   auto hrp=child(h,a.rig,"HumanoidRootPart"),targetHum=child(h,a.rig,"Humanoid");auto hp=rd<float>(h,targetHum+Offsets::Humanoid::Health,NAN);
   if(!std::isfinite(hp)||hp<=0||!hrp)continue;
   Vec p=position(h,hrp)+Vec{0,1.5,0},v=velocity(h,hrp);if(!ff3qb::finite(p)||!ff3qb::finite(v)||ff3qb::length(v)>40||std::abs(v.y)>8)continue;v.y=0;
   auto screen=ff3qb::project(p,f.matrix,width,height);if(!screen.valid)continue;
   auto top=ff3qb::project(p+Vec{0,2,0},f.matrix,width,height),bottom=ff3qb::project(p-Vec{0,3,0},f.matrix,width,height);
   double distance=top.valid&&bottom.valid?ff3qb::selectionDistance(f.cursor.x-f.origin.x,f.cursor.y-f.origin.y,bottom.x,bottom.y,top.x,top.y,std::clamp(std::abs(top.y-bottom.y)*.3,7.,26.),a.rig==retained):std::hypot(screen.x-(f.cursor.x-f.origin.x),screen.y-(f.cursor.y-f.origin.y));
   f.candidates.push_back({screen,nm(h,a.rig),false});
   if(!forced&&distance>=best)continue;
   if(ff3qb::length(p-origin)<6||ff3qb::length(p-origin)>350)continue;
   best=distance;f.target=a.rig;f.name=nm(h,a.rig);f.receiver=screen;targetPosition=p;targetVelocity=v;
  }
  if(!f.target){retained=0;f.reason="Aim near a teammate or practice receiver";return f;}
  retained=f.target;for(auto& c:f.candidates)c.selected=c.screen.x==f.receiver.x&&c.screen.y==f.receiver.y;
  f.targetPosition=targetPosition;f.targetVelocity=targetVelocity;
  // Normal throws spawn forward of the head. Iterate the direction-dependent
  // offset rather than using a hand position displaced by the animation.
  Vec launch=origin;ff3qb::Solution solution;ff3qb::Plan planned;
  for(int i=0;i<3;i++){planned=ff3qb::planThrow(launch,targetPosition,targetVelocity,f.gravity,std::clamp(f.ping*.0005+.05,0.,.25),f.mode,automaticAngle,manualAngle,autoPower,f.gamePower,arc==1,smartFit,defenders);solution=planned.solution;if(!solution.valid)break;launch=origin+solution.velocity*(5./planned.power);}
  if(!solution.valid){f.reason=autoPower&&!automaticAngle?"Angle needs more than 95 power: raise angle or use Auto":"No reachable throw at these settings";return f;}
  f.power=planned.power;f.fitOffset=planned.offset;f.targetPosition=targetPosition+planned.offset;f.margin=planned.margin;f.coverageKnown=planned.coverageKnown;f.distance=ff3qb::length(targetPosition-origin);f.route=ff3qb::routeName(origin,targetPosition,targetVelocity);
  f.solution=solution;
  // A distant point minimizes camera/head parallax. Normal mouse hit testing
  // still belongs to the game, so geometry can affect the actual throw.
  f.aim=ff3qb::project(origin+solution.velocity*(1000./f.power),f.matrix,width,height);
  if(!packetMode&&!f.aim.valid){f.reason="Turn the camera toward the throw arc";return f;}
  if(!descendant(h,f.tool,f.own)||rd<addr>(h,player+Offsets::Player::ModelInstance)!=f.own||rd<std::uint64_t>(h,root+Offsets::DataModel::GameId)!=gameModules::registry[1].universe){f.reason="Game state changed";return f;}
  f.ready=true;f.reason="Ready: click to throw to "+f.name;return f;
 }
};
inline bool eligible(const Frame& f,double time){return enabled&&f.revision==revision&&f.ready&&time>=f.at&&time-f.at<.10&&admitted()&&focused(f.pid)&&(packetMode||freeCursor());}
inline bool sameIdentity(const Frame& a,const Frame& b){return a.pid&&a.pid==b.pid&&a.model&&a.model==b.model&&a.own&&a.own==b.own&&a.tool&&a.tool==b.tool&&a.target&&a.target==b.target&&a.revision==b.revision;}
inline bool dispatchConsistent(const Frame& request,const Frame& aimed,const Frame& verify,POINT cursor){
 if(!sameIdentity(request,aimed)||!sameIdentity(aimed,verify)||!verify.ready||!aimed.ready||verify.power!=aimed.power||verify.gravity!=aimed.gravity||verify.window!=aimed.window||verify.origin.x!=aimed.origin.x||verify.origin.y!=aimed.origin.y||verify.client.right!=aimed.client.right||verify.client.bottom!=aimed.client.bottom)return false;
 const POINT expected{aimed.origin.x+LONG(std::lround(aimed.aim.x)),aimed.origin.y+LONG(std::lround(aimed.aim.y))};
 return verify.aim.valid&&aimed.aim.valid&&ff3qb::cameraStable(aimed.matrix,verify.matrix)&&std::hypot(verify.aim.x-aimed.aim.x,verify.aim.y-aimed.aim.y)<8&&std::hypot(double(cursor.x-expected.x),double(cursor.y-expected.y))<4&&std::hypot(verify.gameMouse.x-aimed.aim.x,verify.gameMouse.y-aimed.aim.y)<4;
}
inline LRESULT CALLBACK mouseProc(int n,WPARAM w,LPARAM l){
 if(packetMode){
  if(n==HC_ACTION&&enabled&&w==WM_LBUTTONDOWN){auto* event=(MSLLHOOKSTRUCT*)l;if(!(event->flags&LLMHF_INJECTED)){std::lock_guard<std::mutex> lock(mutex);if(current.pid&&focused(current.pid)&&admitted()){requested=current;physicalClickAt=GetTickCount64();}}}
  return CallNextHookEx(mouse,n,w,l);
 }
 if(n==HC_ACTION){auto* event=(MSLLHOOKSTRUCT*)l;if(!(event->flags&LLMHF_INJECTED)){
  std::lock_guard<std::mutex> lock(mutex);
  if(w==WM_LBUTTONUP&&clicks.up())return 1;
  if(w==WM_LBUTTONDOWN){
   const double distance=std::hypot(double(event->pt.x-current.cursor.x),double(event->pt.y-current.cursor.y));
   if(clicks.down(distance<40&&eligible(current,now()),now())){requested=current;return 1;}
  }
 }}return CallNextHookEx(mouse,n,w,l);
}
inline bool angleKeyOwned[2]={false,false};
inline LRESULT CALLBACK keyboardProc(int n,WPARAM w,LPARAM l){
 if(n==HC_ACTION){auto* e=(KBDLLHOOKSTRUCT*)l;int index=e->vkCode=='R'?0:e->vkCode=='F'?1:-1;
  if(index>=0&&!(e->flags&LLKHF_INJECTED)){
   bool down=w==WM_KEYDOWN||w==WM_SYSKEYDOWN,up=w==WM_KEYUP||w==WM_SYSKEYUP;
   if(up&&angleKeyOwned[index]){angleKeyOwned[index]=false;return 1;}
   auto f=snapshot();
   if(down&&enabled&&!automaticAngle&&autoPower&&f.held&&f.wheelVisible&&!f.textFocus&&now()-f.at<.15&&focused(f.pid)&&admitted()&&!(GetAsyncKeyState(VK_CONTROL)&0x8000)&&!(GetAsyncKeyState(VK_MENU)&0x8000)){
    angleKeyOwned[index]=true;manualAngle=std::clamp(manualAngle.load()+(index==0?1:-1),5,75);++revision;return 1;
   }
   if(down&&angleKeyOwned[index])return 1;
  }
 }return CallNextHookEx(keyboard,n,w,l);
}
inline bool install(){if(!mouse)mouse=SetWindowsHookExW(WH_MOUSE_LL,mouseProc,GetModuleHandleW(nullptr),0);if(!keyboard)keyboard=SetWindowsHookExW(WH_KEYBOARD_LL,keyboardProc,GetModuleHandleW(nullptr),0);return mouse&&keyboard;}
inline void uninstall(){if(mouse)UnhookWindowsHookEx(mouse);if(keyboard)UnhookWindowsHookEx(keyboard);mouse=keyboard=nullptr;angleKeyOwned[0]=angleKeyOwned[1]=false;std::lock_guard<std::mutex> lock(mutex);clicks={};requested={};}
inline void stop(){enabled=false;++revision;physicalClickAt=0;{std::lock_guard<std::mutex> lock(mutex);clicks.cancel();current={};current.reason="QB aim off";}if(packetStop)packetStop();}
inline bool moveTo(POINT point){const int x=GetSystemMetrics(SM_XVIRTUALSCREEN),y=GetSystemMetrics(SM_YVIRTUALSCREEN),w=GetSystemMetrics(SM_CXVIRTUALSCREEN),h=GetSystemMetrics(SM_CYVIRTUALSCREEN);if(w<2||h<2)return false;INPUT in{};in.type=INPUT_MOUSE;in.mi.dwFlags=MOUSEEVENTF_MOVE|MOUSEEVENTF_ABSOLUTE|MOUSEEVENTF_VIRTUALDESK;in.mi.dx=LONG(std::lround((point.x-x)*65535./(w-1)));in.mi.dy=LONG(std::lround((point.y-y)*65535./(h-1)));return SendInput(1,&in,sizeof in)==1;}
inline void tapPower(){INPUT in[2]{};in[0].type=in[1].type=INPUT_KEYBOARD;in[0].ki.wVk=in[1].ki.wVk='R';in[1].ki.dwFlags=KEYEVENTF_KEYUP;if(SendInput(2,in,sizeof(INPUT))==1)SendInput(1,&in[1],sizeof(INPUT));}
inline void worker(){
 Reader reader;double powerAt=-10;
 while(running){
  if(!enabled||!admitted()){Frame idle;idle.reason=enabled?"Connect to Football Fusion 3":"QB aim off";publish(idle);{std::lock_guard<std::mutex> lock(mutex);clicks.cancel();}Sleep(30);continue;}
  auto f=reader.read();
  if(packetMode){
   const auto clicked=physicalClickAt.load();
   if(clicked&&GetTickCount64()-clicked<350){Frame locked;{std::lock_guard<std::mutex> lock(mutex);locked=requested;}if(locked.ready){auto fresh=reader.read(locked.target);if(sameIdentity(locked,fresh))f=fresh;else{f.ready=false;f.reason="Throw target changed";}}else{f.ready=false;f.reason="No ready receiver at click";}}
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
inline json::Value modules(){using namespace basketballUi;auto out=object(),m=object(),rows=object();m.o[L"on"]=basketballUi::boolean(enabled);rows.o[L"arc"]=number(arc);rows.o[L"preset"]=number(passMode);rows.o[L"angle"]=number(manualAngle);rows.o[L"radius"]=number(selectionRadius);rows.o[L"autoangle"]=basketballUi::boolean(automaticAngle);rows.o[L"fit"]=basketballUi::boolean(smartFit);rows.o[L"hud"]=basketballUi::boolean(showHud);rows.o[L"candidates"]=basketballUi::boolean(showCandidates);rows.o[L"power"]=basketballUi::boolean(autoPower);rows.o[L"practice"]=basketballUi::boolean(practice);rows.o[L"visuals"]=basketballUi::boolean(visuals);m.o[L"rows"]=rows;out.o[L"ff3_qb"]=m;return out;}
inline bool set(const std::wstring& key,const json::Value& v){
 if(key==L"ff3_qb.arc"){if(v.type!=json::Value::Number||(v.n!=0&&v.n!=1))return false;arc=int(v.n);++revision;return true;}
 if(key==L"ff3_qb.preset"||key==L"ff3_qb.angle"||key==L"ff3_qb.radius"){
  if(v.type!=json::Value::Number||!std::isfinite(v.n)||std::floor(v.n)!=v.n)return false;
  if(key==L"ff3_qb.preset"){if(v.n<0||v.n>5)return false;passMode=int(v.n);}
  else if(key==L"ff3_qb.angle"){if(v.n<5||v.n>75)return false;manualAngle=int(v.n);}
  else{if(v.n<40||v.n>350)return false;selectionRadius=int(v.n);}++revision;return true;
 }
 if(v.type!=json::Value::Bool)return false;
 if(key==L"ff3_qb"){enabled=v.b;++revision;if(!v.b)stop();else{std::lock_guard<std::mutex> lock(mutex);current={};current.reason="Waiting for game data";}return true;}
 if(key==L"ff3_qb.autoangle")automaticAngle=v.b;else if(key==L"ff3_qb.fit")smartFit=v.b;else if(key==L"ff3_qb.hud")showHud=v.b;else if(key==L"ff3_qb.candidates")showCandidates=v.b;else if(key==L"ff3_qb.power")autoPower=v.b;else if(key==L"ff3_qb.practice")practice=v.b;else if(key==L"ff3_qb.visuals")visuals=v.b;else return false;++revision;return true;
}
inline json::Value report(){using namespace basketballUi;auto f=snapshot();auto out=object();out.o[L"type"]=string(L"ff3Live");out.o[L"modules"]=modules();out.o[L"reason"]=string(widen(f.reason));if(enabled&&packetMode&&packetStatus){const auto state=packetStatus();if(!state.first)out.o[L"reason"]=string(state.second);}out.o[L"target"]=string(widen(f.name));out.o[L"power"]=number(f.power);out.o[L"gamePower"]=number(f.gamePower);out.o[L"held"]=basketballUi::boolean(f.held);out.o[L"wheel"]=basketballUi::boolean(f.wheelVisible);out.o[L"ready"]=basketballUi::boolean(f.ready);out.o[L"route"]=string(widen(f.route));out.o[L"preset"]=string(widen(ff3qb::presetName(f.mode)));out.o[L"distance"]=number(f.distance);out.o[L"coverageKnown"]=basketballUi::boolean(f.coverageKnown);out.o[L"margin"]=number(f.margin);out.o[L"fitOffset"]=number(ff3qb::length(f.fitOffset));out.o[L"angle"]=number(f.solution.angle);out.o[L"flight"]=number(f.solution.time);{std::lock_guard<std::mutex> lock(mutex);out.o[L"lastAction"]=string(now()-actionAt<3?widen(lastAction):L"");out.o[L"throws"]=number(throwCount);}return out;}
inline void load(const std::filesystem::path& file){auto cfg=basketballUi::readConfig(file);for(auto key:{L"arc",L"power",L"practice",L"visuals",L"preset",L"angle",L"radius",L"autoangle",L"fit",L"hud",L"candidates"})if(cfg[key].type!=json::Value::Null)set(L"ff3_qb."+std::wstring(key),cfg[key]);enabled=false;}
inline bool save(const std::filesystem::path& file){auto data=modules()[L"ff3_qb"][L"rows"];auto temp=file;temp+=L".tmp";{std::ofstream out(temp,std::ios::binary);out<<winrt::to_string(json::dump(data));out.flush();if(!out)return false;}return MoveFileExW(temp.c_str(),file.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;}
}
