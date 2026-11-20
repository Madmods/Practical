#pragma once
#include "studio_lab.hpp"
#include "ff3_qb_runtime.hpp"
#include <UIAutomation.h>
#include <wincrypt.h>
#include <wrl/client.h>
#pragma comment(lib,"crypt32.lib")

namespace studioConnection {
using namespace studioLab;
using Microsoft::WRL::ComPtr;
inline std::string ascii(const std::wstring& v){return winrt::to_string(v);}
inline std::string header(const std::string& h,const std::string& name){
 std::istringstream stream(h);std::string line;while(std::getline(stream,line)){auto split=line.find(':');if(split==std::string::npos)continue;auto key=line.substr(0,split);std::transform(key.begin(),key.end(),key.begin(),[](unsigned char c){return char(std::tolower(c));});if(key!=name)continue;auto value=line.substr(split+1);auto first=value.find_first_not_of(" \t\r");auto end=value.find_last_not_of(" \t\r");return first==std::string::npos?"":value.substr(first,end-first+1);}return "";
}
inline std::string acceptKey(const std::string& key){
 HCRYPTPROV provider=0;HCRYPTHASH hash=0;BYTE digest[20]{};DWORD length=20;std::string source=key+"258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
 if(!CryptAcquireContextW(&provider,nullptr,nullptr,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT))return "";
 bool ok=CryptCreateHash(provider,CALG_SHA1,0,0,&hash)&&CryptHashData(hash,reinterpret_cast<const BYTE*>(source.data()),DWORD(source.size()),0)&&CryptGetHashParam(hash,HP_HASHVAL,digest,&length,0);
 if(hash)CryptDestroyHash(hash);CryptReleaseContext(provider,0);if(!ok)return "";
 char out[64]{};length=sizeof out;if(!CryptBinaryToStringA(digest,20,CRYPT_STRING_BASE64|CRYPT_STRING_NOCRLF,out,&length))return "";return out;
}
inline bool sendAll(SOCKET s,const char* p,size_t n){while(n){int sent=send(s,p,int(n),0);if(sent<=0)return false;p+=sent;n-=sent;}return true;}
inline bool readAll(SOCKET s,char* p,size_t n){while(n){int got=recv(s,p,int(n),0);if(got<=0)return false;p+=got;n-=got;}return true;}
inline bool sendFrame(SOCKET s,const std::string& body,unsigned char opcode=1){
 if(body.size()>60000)return false;std::string frame;frame+=char(0x80|opcode);if(body.size()<126)frame+=char(body.size());else{frame+=char(126);frame+=char(body.size()>>8);frame+=char(body.size());}frame+=body;return sendAll(s,frame.data(),frame.size());
}
struct Slot {std::mutex mutex;SOCKET socket=INVALID_SOCKET;std::thread worker;
 void disconnect(){std::lock_guard<std::mutex> l(mutex);if(socket!=INVALID_SOCKET)shutdown(socket,SD_BOTH);}
 bool send(const json::Value& message){std::lock_guard<std::mutex> l(mutex);return socket!=INVALID_SOCKET&&sendFrame(socket,ascii(json::dump(message)));}
};
class Service {
 std::atomic<bool> on_{false};SOCKET listener_=INVALID_SOCKET;std::thread accept_;Slot server_,view_;
 std::mutex dataMutex_;json::Value world_,camera_,pending_;double worldAt_=0,cameraAt_=0,pendingAt_=0;
 std::string token_;std::filesystem::path folder_;std::wstring lastSession_;std::uint64_t model_=0;
 ff3qb::BeamMemory memory_;ff3qb::Plan last_;ff3qb::RoutePassLatch route_;
 std::mutex viewMutex_;double generation_=-1;
 double plannedAt_=0,searchAt_=0,boundsAt_=0,diagnosticAt_=0;unsigned revision_=0;addr target_=0;
 HWND window_=nullptr;DWORD pid_=0;RECT viewport_{};ComPtr<IUIAutomation> automation_;
 ff3Native::Frame released_;bool hadBall_=false;
 void stream(Slot& slot,bool view,SOCKET socket){
  winrt::init_apartment(winrt::apartment_type::multi_threaded);std::string fragmented;
  while(on_){
   unsigned char prefix[2]{};if(!readAll(socket,reinterpret_cast<char*>(prefix),2))break;
   bool fin=(prefix[0]&128)!=0,mask=(prefix[1]&128)!=0;unsigned op=prefix[0]&15;size_t size=prefix[1]&127;
   if((prefix[0]&0x70)||!mask||size==127)break;
   if(size==126){unsigned char ext[2];if(!readAll(socket,reinterpret_cast<char*>(ext),2))break;size=(size_t(ext[0])<<8)|ext[1];}
   if(size>60000||(op>=8&&(!fin||size>125)))break;
   unsigned char key[4];if(!readAll(socket,reinterpret_cast<char*>(key),4))break;std::string body(size,'\0');if(!readAll(socket,body.data(),size))break;for(size_t i=0;i<size;i++)body[i]^=key[i%4];
   if(op==8)break;if(op==9){std::lock_guard<std::mutex> l(slot.mutex);sendFrame(socket,body,10);continue;}if(op==10)continue;
   if(op!=0&&op!=1)break;if(op==1){if(!fragmented.empty())break;fragmented=body;}else{if(fragmented.empty())break;fragmented+=body;}
   if(fragmented.size()>60000)break;if(!fin)continue;
   auto message=json::parse(winrt::to_hstring(fragmented).c_str());fragmented.clear();if(message.type!=json::Value::Object||message[L"protocol"].num()!=2)continue;
   std::lock_guard<std::mutex> lock(dataMutex_);
   if(view&&message[L"kind"].str()==L"camera"){camera_=message;cameraAt_=now();}
   else if(!view&&message[L"kind"].str()==L"state"){world_=message;worldAt_=now();}
   else if(!view&&message[L"kind"].str()==L"throw"){pending_=message;pendingAt_=now();if(message[L"snapshot"].type==json::Value::Object){world_=message[L"snapshot"];worldAt_=now();}}
  }
  {std::lock_guard<std::mutex> l(slot.mutex);if(slot.socket==socket)slot.socket=INVALID_SOCKET;closesocket(socket);}
  {std::lock_guard<std::mutex> l(dataMutex_);if(view)cameraAt_=0;else{worldAt_=0;pending_={};}}
  winrt::uninit_apartment();
 }
 void acceptLoop(){
  while(on_){
   fd_set f;FD_ZERO(&f);FD_SET(listener_,&f);timeval timeout{0,200000};if(select(0,&f,nullptr,nullptr,&timeout)<=0)continue;
   SOCKET socket=accept(listener_,nullptr,nullptr);if(socket==INVALID_SOCKET)continue;
   DWORD wait=1500;setsockopt(socket,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<char*>(&wait),sizeof wait);setsockopt(socket,SOL_SOCKET,SO_SNDTIMEO,reinterpret_cast<char*>(&wait),sizeof wait);
   BOOL noDelay=TRUE;setsockopt(socket,IPPROTO_TCP,TCP_NODELAY,reinterpret_cast<char*>(&noDelay),sizeof noDelay);
   std::string request;char c;while(request.size()<8192&&request.find("\r\n\r\n")==std::string::npos){if(recv(socket,&c,1,0)!=1)break;request+=c;}
   bool view=request.rfind("GET /studio/view HTTP/1.1\r\n",0)==0,server=request.rfind("GET /studio/server HTTP/1.1\r\n",0)==0;
   auto key=header(request,"sec-websocket-key");auto upgrade=header(request,"upgrade");std::transform(upgrade.begin(),upgrade.end(),upgrade.begin(),[](unsigned char v){return char(std::tolower(v));});
   if((!view&&!server)||header(request,"x-vanta-lab")!=token_||!header(request,"origin").empty()||key.size()!=24||upgrade!="websocket"||header(request,"sec-websocket-version")!="13"){
    reply(socket,403,"{\"error\":\"Authorized Studio bridge required\"}");closesocket(socket);continue;
   }
   auto accept=acceptKey(key);if(accept.empty()){closesocket(socket);continue;}
   auto response="HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: "+accept+"\r\n\r\n";
   if(!sendAll(socket,response.data(),response.size())){closesocket(socket);continue;}
   Slot& slot=view?view_:server_;slot.disconnect();if(slot.worker.joinable())slot.worker.join();
   {std::lock_guard<std::mutex> l(slot.mutex);slot.socket=socket;}
   slot.worker=std::thread([this,&slot,view,socket](){stream(slot,view,socket);});
  }
 }
 static bool isStudio(HWND w,DWORD& pid){
  if(!IsWindowVisible(w))return false;GetWindowThreadProcessId(w,&pid);HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);if(!process)return false;
  wchar_t path[2048];DWORD length=2048;bool ok=QueryFullProcessImageNameW(process,0,path,&length)&&_wcsicmp(std::filesystem::path(path).filename().c_str(),L"RobloxStudioBeta.exe")==0;CloseHandle(process);return ok;
 }
 void bounds(){
  std::lock_guard<std::mutex> guard(viewMutex_);
  if(now()-boundsAt_<.25)return;boundsAt_=now();DWORD pid=0;HWND w=GetForegroundWindow();
  if(!isStudio(w,pid)){
   if(window_&&isStudio(window_,pid))w=window_;
   else{struct Find{HWND w=nullptr;DWORD pid=0;} found;EnumWindows([](HWND h,LPARAM p)->BOOL{auto& f=*reinterpret_cast<Find*>(p);if(isStudio(h,f.pid)){f.w=h;return FALSE;}return TRUE;},reinterpret_cast<LPARAM>(&found));w=found.w;pid=found.pid;}
  }
  window_=w;pid_=w?pid:0;viewport_={};if(!w||IsIconic(w))return;
  if(!automation_)CoCreateInstance(CLSID_CUIAutomation,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&automation_));
  if(!automation_)return;ComPtr<IUIAutomationElement> root,element;ComPtr<IUIAutomationCondition> condition;
  VARIANT value;VariantInit(&value);value.vt=VT_BSTR;value.bstrVal=SysAllocString(L"QEngineWidget");
  automation_->CreatePropertyCondition(UIA_AutomationIdPropertyId,value,&condition);VariantClear(&value);
  if(!condition||FAILED(automation_->ElementFromHandle(w,&root))||!root)return;
  if(FAILED(root->FindFirst(TreeScope_Descendants,condition.Get(),&element))||!element)return;BOOL offscreen=FALSE;element->get_CurrentIsOffscreen(&offscreen);if(offscreen)return;element->get_CurrentBoundingRectangle(&viewport_);
 }
 bool projectView(ff3Native::Frame& f,const json::Value& c,double received){
  std::lock_guard<std::mutex> guard(viewMutex_);
  if(now()-received>.2||c[L"session"].str()!=lastSession_||viewport_.right-viewport_.left<100||viewport_.bottom-viewport_.top<100)return false;
  try{
   auto position=vector(c[L"position"]),right=vector(c[L"right"]),up=vector(c[L"up"]),look=vector(c[L"look"]);
   auto size=c[L"viewport"];if(size.a.size()!=2)return false;double width=bounded(size.a[0],100,8000),height=bounded(size.a[1],100,5000),fov=bounded(c[L"fov"],10,120);
   if(std::abs(ff3qb::length(right)-1)>.01||std::abs(ff3qb::length(up)-1)>.01||std::abs(ff3qb::length(look)-1)>.01)return false;
   double scale=1/std::tan(fov*3.141592653589793/360);right=right*(scale*height/width);up=up*scale;
   float m[16]={float(right.x),float(right.y),float(right.z),float(-ff3qb::dot(right,position)),float(up.x),float(up.y),float(up.z),float(-ff3qb::dot(up,position)),0,0,1,0,float(look.x),float(look.y),float(look.z),float(-ff3qb::dot(look,position))};
   std::copy(m,m+16,f.matrix);f.origin={viewport_.left,viewport_.top};f.client={0,0,viewport_.right-viewport_.left,viewport_.bottom-viewport_.top};f.window=window_;f.pid=pid_;f.textFocus=c[L"textFocus"].boolean();return true;
  }catch(...){return false;}
 }
 ff3Native::Frame frame(const json::Value& q,const json::Value& camera,double received,double cameraReceived){
  using namespace ff3Native;Frame f;f.studio=true;f.at=now();f.revision=revision;f.pid=pid_;f.model=model_;f.own=1;f.localPlayer=1;f.reason="Waiting for Studio Play";
  bool projected=projectView(f,camera,cameraReceived);gameModules::Snapshot s;s.studio=true;s.sampledAt=f.at;s.pid=pid_;s.model=model_;s.character=1;s.process=pid_!=0;s.version=s.opened=s.process;s.loaded=s.player=s.world=f.at-received<.5&&q.type==json::Value::Object;s.renderer=projected;
  gameModules::publish(s);if(!s.loaded||!projected)return f;
  f.head=vector(q[L"head"]);f.held=q[L"held"].boolean();f.tool=f.held?1:0;f.wheelVisible=f.held;f.gamePower=q[L"power"].num(95);f.gravity=bounded(q[L"gravity"],1,300);f.groundY=bounded(q[L"ground"],-500,500);f.ping=0;GetCursorPos(&f.cursor);
  if(q[L"actors"].type!=json::Value::Array||q[L"actors"].a.size()>32)return f;
  struct Actor{addr id;std::string name;bool wr;ff3qb::Defender reach;};std::vector<Actor> actors;
  for(auto& a:q[L"actors"].a){auto d=actor(a);addr id=addr(bounded(a[L"id"],1,1000000));actors.push_back({id,ascii(a[L"name"].str()),a[L"wr"].boolean(),d});}
  addr chosen=lockedTarget;bool found=false;for(auto& a:actors)if(a.id==chosen&&a.wr)found=true;if(!found){chosen=0;lockedTarget=0;}
  double nearest=selectionRadius.load();
  if(!chosen)for(auto& a:actors)if(a.wr){auto head=ff3qb::project(a.reach.position,f.matrix,f.client.right,f.client.bottom);auto feet=ff3qb::project(a.reach.position-Vec{0,4.5,0},f.matrix,f.client.right,f.client.bottom);if(!head.valid||!feet.valid)continue;double radius=std::clamp(std::abs(feet.y-head.y)*.22,5.,18.);auto distance=ff3qb::selectionDistance(f.cursor.x-f.origin.x,f.cursor.y-f.origin.y,feet.x,feet.y,head.x,head.y,radius,false);if(distance<nearest){nearest=distance;chosen=a.id;}}
  f.target=chosen;f.locked=lockedTarget!=0;f.mode=passMode;f.coverageKnown=f.hudCoverageKnown=true;
  const Actor* wr=nullptr;for(auto& a:actors)if(a.id==chosen)wr=&a;else{auto d=a.reach;d.speed=ff3qb::movementPotential(d.speed,d.velocity);d.diveReach=diveReach;f.defenders.push_back(d);f.hudDefenders.push_back(d);f.hudShapeMeasured.push_back(true);}
  f.coverageCount=int(f.defenders.size());f.coverageDetail="Studio catchers / measured test profile";
  if(wr){f.name=wr->name;f.selectedPosition=wr->reach.position;f.targetVelocity=wr->reach.velocity;f.targetPosition=wr->reach.position;f.receiverReach=wr->reach;f.receiverReach.diveReach=diveReach;f.receiverReach.redirect=smartFit&&wr->reach.speed>0;f.receiverReachKnown=true;f.receiver=ff3qb::project(f.selectedPosition,f.matrix,f.client.right,f.client.bottom);}
  if(!f.held){
   if(q[L"ball"].type==json::Value::Object){auto b=q[L"ball"];ff3qb::BallFlight flight;flight.id=1;flight.gravity=f.gravity;double age=b[L"age"].num();flight.at=f.at;flight.model={true,f.at-age,f.gravity,0,vector(b[L"origin"]),vector(b[L"velocity"]),f.at-age};flight.position=flight.model.pointAt(f.at);flight.velocity=flight.model.velocity+Vec{0,-f.gravity*age,0};flight.fieldY=f.groundY;f.balls.push_back(flight);f.releaseVisual=released_.ready?std::make_shared<Frame>(released_):nullptr;f.throwView.phase=ff3qb::ThrowView::Flight;f.throwView.ball=1;f.throwView.name=released_.name;f.throwView.air=age;}
   else{f.throwView.phase=ff3qb::ThrowView::Done;f.throwView.result=ascii(q[L"result"].str());f.throwView.name=released_.name;}
   f.reason="Studio ball in flight";return f;
  }
  if(!enabled||!admitted()){f.reason=enabled?"Open the Studio workspace in Vanta":"QB aim off";return f;}
  if(!wr){f.reason="Aim near a receiver; Q locks selection";return f;}if(f.textFocus){f.reason="Finish typing to aim";return f;}
  if(target_!=chosen||revision_!=revision){last_={};memory_={};route_={};plannedAt_=0;searchAt_=0;target_=chosen;revision_=revision;}
  const double step=plannedAt_?f.at-plannedAt_:.1;bool searching=f.at>=searchAt_;
  const bool powerFirst=automaticAngle&&autoPower&&f.mode==0;
  auto style=route_.update(ff3qb::chooseRoutePass(f.head,wr->reach.position,f.targetVelocity,f.hudDefenders),f.at);
  bool routeAuto=powerFirst&&!force95&&style!=ff3qb::RoutePass::Fit;
  f.leadOffset=routeAuto?Vec{}:ff3qb::leadAdjustment(f.head,wr->reach.position,f.targetVelocity,selectedLead());f.leadOffset.y=routeAuto?0:selectedHeight();auto aim=wr->reach.position+f.leadOffset;
  ff3qb::Plan planned;
  if(powerFirst){
   if(routeAuto)planned=ff3qb::planRoutePass(style,f.head,wr->reach.position,f.targetVelocity,f.gravity,0,f.defenders,smartFit,&f.receiverReach,last_.solution.valid?last_.solution.angle:NAN,step,&memory_.elevation,f.at,f.groundY);
   if(planned.solution.valid&&(!smartFit||ff3qb::estimatedClear(planned,true))){f.passLabel=ff3qb::routePassName(style);f.fixedPowerRoute=style!=ff3qb::RoutePass::Head;}
   else planned=ff3qb::planCoverageThrow(f.head,aim,f.targetVelocity,f.gravity,0,f.defenders,smartFit,force95,f.receiverReach.jumpHeight,f.receiverReach.worldGravity,last_.offset,last_.power,last_.lob,last_.solution.valid?last_.solution.angle:NAN,searching,step,&f.receiverReach,f.leadOffset.y,&memory_,f.at,f.groundY);
  }else planned=ff3qb::planThrow(f.head,aim,f.targetVelocity,f.gravity,0,f.mode,automaticAngle,manualAngle,autoPower,f.gamePower,arc==1,smartFit,f.defenders,NAN,last_.offset,false,5,f.groundY);
  plannedAt_=f.at;if(searching)searchAt_=f.at+.1;
  if(!planned.solution.valid){f.reason="No reachable arc; normal throw remains available";return f;}
  last_=planned;f.solution=planned.solution;f.power=planned.power;f.fitOffset=planned.offset;f.margin=planned.margin;f.targetPosition=aim+planned.offset;f.distance=ff3qb::length(wr->reach.position-f.head);f.selectedLob=planned.lob;f.guarded=smartFit;
  f.risk=ff3qb::riskForecast(f.solution,f.gravity,f.hudDefenders,0,f.groundY);f.ready=true;f.reason="Studio throw control ready";f.aim=ff3qb::project(f.solution.catchPoint,f.matrix,f.client.right,f.client.bottom);return f;
 }
public:
 ~Service(){stop();}
 bool start(const std::filesystem::path& folder){
  folder_=folder;std::ifstream file(folder/L"vanta-lab-connection.json");std::string bytes((std::istreambuf_iterator<char>(file)),{});auto config=json::parse(winrt::to_hstring(bytes).c_str());token_=ascii(config[L"token"].str());int port=int(config[L"port"].num());if(token_.size()!=48||token_.find_first_not_of("0123456789abcdef")!=std::string::npos||port<1024||port>65535)return false;
  WSADATA data{};if(WSAStartup(MAKEWORD(2,2),&data))return false;listener_=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);BOOL exclusive=TRUE;setsockopt(listener_,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,reinterpret_cast<char*>(&exclusive),sizeof exclusive);
  sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);address.sin_port=htons(u_short(port));if(bind(listener_,reinterpret_cast<sockaddr*>(&address),sizeof address)||listen(listener_,4)){closesocket(listener_);listener_=INVALID_SOCKET;WSACleanup();return false;}
  on_=true;accept_=std::thread([this](){acceptLoop();});return true;
 }
 void stop(){if(!on_.exchange(false))return;server_.disconnect();view_.disconnect();if(accept_.joinable())accept_.join();server_.disconnect();view_.disconnect();if(server_.worker.joinable())server_.worker.join();if(view_.worker.joinable())view_.worker.join();closesocket(listener_);listener_=INVALID_SOCKET;WSACleanup();}
 std::pair<bool,std::wstring> status(){std::lock_guard<std::mutex> l(dataMutex_);bool ready=now()-worldAt_<.5&&now()-cameraAt_<.2;return {ready,ready?L"Studio throw control ready":L"Waiting for Studio Play connection"};}
 bool view(ff3Native::Frame& f){std::lock_guard<std::mutex> l(dataMutex_);return projectView(f,camera_,cameraAt_);}
 ff3Native::Frame read(){
  bounds();json::Value q,c,pending;double wa,ca,pa;{std::lock_guard<std::mutex> l(dataMutex_);q=world_;c=camera_;wa=worldAt_;ca=cameraAt_;pending=pending_;pa=pendingAt_;pending_={};}
  auto session=q[L"session"].str();{std::lock_guard<std::mutex> guard(viewMutex_);if(session!=lastSession_||generation_!=q[L"generation"].num()){lastSession_=session;model_=std::max<size_t>(1,std::hash<std::wstring>{}(session));generation_=q[L"generation"].num();memory_={};last_={};released_={};ff3Native::lockedTarget=0;}}
  ff3Native::Frame f;
  try{f=frame(q,c,wa,ca);}catch(...){f.studio=true;f.at=now();f.reason="Waiting for complete Studio state";}
  if(pending.type==json::Value::Object){auto response=object();response.o[L"protocol"]=number(2);response.o[L"kind"]=string(L"throw");response.o[L"id"]=pending[L"id"];response.o[L"session"]=q[L"session"];
   bool assisted=f.ready&&now()-pa<.35&&ff3Native::enabled&&ff3Native::admitted();response.o[L"assisted"]=studioLab::boolean(assisted);
   if(assisted){response.o[L"origin"]=vector(f.solution.origin);response.o[L"velocity"]=vector(f.solution.velocity);response.o[L"power"]=number(f.power);response.o[L"angle"]=number(f.solution.angle);response.o[L"target"]=number(double(f.target));released_=f;released_.releaseVisual.reset();ff3Native::record(f,"Studio assisted throw",true);}
   server_.send(response);
  }
  if(now()-diagnosticAt_>.5){diagnosticAt_=now();auto d=object();d.o[L"ready"]=studioLab::boolean(f.ready);d.o[L"held"]=studioLab::boolean(f.held);d.o[L"target"]=number(double(f.target));d.o[L"angle"]=number(f.solution.angle);d.o[L"power"]=number(f.power);d.o[L"reason"]=string(basketballUi::widen(f.reason));d.o[L"worldAge"]=number(now()-wa);d.o[L"cameraAge"]=number(now()-ca);d.o[L"renderHz"]=number(ff3Native::renderHz);d.o[L"viewport"]=vector({double(f.origin.x),double(f.origin.y),double(f.client.right)});std::ofstream out(folder_/L"studio-status.json");out<<ascii(json::dump(d));}
  return f;
 }
};
}
