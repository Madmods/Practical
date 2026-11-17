#pragma once
// Explicit, local-only Studio adapter. This includes the production solver;
// no injection, client-memory reader, or packet transport runs in this mode.
#include "ff3_qb_logic.hpp"
#include "webui/json_lite.hpp"
#include "capture_privacy.hpp"
#include <fstream>
#include <filesystem>
#include <thread>
#include <sstream>
#include <chrono>
#include <random>
#include <cctype>

namespace studioLab {
using ff3qb::Vec;
inline json::Value number(double n){json::Value v;v.type=json::Value::Number;v.n=n;return v;}
inline json::Value boolean(bool b){json::Value v;v.type=json::Value::Bool;v.b=b;return v;}
inline json::Value string(const std::wstring& s){json::Value v;v.type=json::Value::String;v.s=s;return v;}
inline json::Value object(){json::Value v;v.type=json::Value::Object;return v;}
inline json::Value vector(Vec p){json::Value v;v.type=json::Value::Array;v.a={number(p.x),number(p.y),number(p.z)};return v;}
inline double bounded(const json::Value& v,double low,double high){if(v.type!=json::Value::Number||!std::isfinite(v.n)||v.n<low||v.n>high)throw std::runtime_error("Invalid numeric field");return v.n;}
inline Vec vector(const json::Value& v){if(v.type!=json::Value::Array||v.a.size()!=3)throw std::runtime_error("Invalid vector");return {bounded(v.a[0],-10000,10000),bounded(v.a[1],-10000,10000),bounded(v.a[2],-10000,10000)};}
inline double time(){return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();}
inline ff3qb::Defender actor(const json::Value& v){
 ff3qb::Defender d;d.position=vector(v[L"position"]);d.velocity=vector(v[L"velocity"]);
 if(ff3qb::length(d.velocity)>150)throw std::runtime_error("Velocity outside lab limits");
 d.speed=bounded(v[L"speed"],0,45);d.jumpHeight=bounded(v[L"jumpHeight"],0,40);d.diveReach=bounded(v[L"diveReach"],0,25);
 d.radius=bounded(v[L"radius"],.1,12);d.above=bounded(v[L"above"],.1,12);d.below=bounded(v[L"below"],.1,12);d.worldGravity=bounded(v[L"worldGravity"],10,300);return d;
}
struct Planner {
 ff3qb::BeamMemory memory;ff3qb::Plan last;std::wstring identity,settings;double at=0;
 json::Value plan(const json::Value& q){
  if(q[L"protocol"].num()!=1)throw std::runtime_error("Unsupported lab protocol");
  auto origin=vector(q[L"origin"]);auto receiver=actor(q[L"receiver"]);auto config=q[L"settings"];
  const double gravity=bounded(q[L"gravity"],1,300),ground=bounded(q[L"ground"],-500,500);
  const bool beam=config[L"beam"].boolean(),fit=config[L"smartFit"].boolean(true);
  const double height=bounded(config[L"height"],-8,12),lead=bounded(config[L"lead"],-12,12);
  if(q[L"defenders"].type!=json::Value::Array||q[L"defenders"].a.size()>32)throw std::runtime_error("Invalid defenders");
  std::vector<ff3qb::Defender> defenders;
  for(auto& item:q[L"defenders"].a){auto d=actor(item);d.speed=ff3qb::movementPotential(d.speed,d.velocity);defenders.push_back(d);}
  const auto key=q[L"session"].str()+L":"+q[L"target"].str(),configKey=json::dump(config);const double t=time();
  if(key!=identity||configKey!=settings||t-at>1){memory={};last={};identity=key;settings=configKey;at=0;}
  receiver.redirect=fit&&receiver.speed>0;
  auto adjust=ff3qb::leadAdjustment(origin,receiver.position,receiver.velocity,lead);adjust.y=height;
  auto next=ff3qb::planCoverageThrow(origin,receiver.position+adjust,receiver.velocity,gravity,0,defenders,fit,beam,receiver.jumpHeight,receiver.worldGravity,last.offset,last.power,last.lob,last.solution.valid?last.solution.angle:NAN,true,at?t-at:.1,&receiver,height,&memory,t,ground);
  at=t;last=next;
  auto out=object();out.o[L"protocol"]=number(1);out.o[L"sequence"]=q[L"sequence"];out.o[L"target"]=q[L"target"];out.o[L"valid"]=boolean(next.solution.valid);out.o[L"planner"]=string(L"Vanta native ff3_qb_logic.hpp");
  if(!next.solution.valid){out.o[L"reason"]=string(L"No reachable solution at these settings");return out;}
  auto risk=ff3qb::riskForecast(next.solution,gravity,defenders,0,ground);auto full=ff3qb::groundFlight(next.solution,gravity,ground);
  out.o[L"origin"]=vector(next.solution.origin);out.o[L"velocity"]=vector(next.solution.velocity);out.o[L"catchPoint"]=vector(next.solution.catchPoint);
  out.o[L"power"]=number(next.power);out.o[L"angle"]=number(next.solution.angle);out.o[L"catchTime"]=number(next.solution.time);out.o[L"groundTime"]=number(full.valid?full.time:next.solution.time);
  out.o[L"margin"]=number(next.margin);out.o[L"intable"]=boolean(risk.intable);out.o[L"possible"]=boolean(risk.possible);out.o[L"threatPoint"]=vector(risk.limit.point);out.o[L"threatIndex"]=number(risk.limit.catcher+1);
  out.o[L"groundPoint"]=vector(full.valid?full.catchPoint:next.solution.catchPoint);out.o[L"offset"]=vector(next.offset);out.o[L"gravity"]=number(gravity);out.o[L"computeMs"]=number((time()-t)*1000);return out;
 }
};
inline std::atomic<bool> active{true};inline std::atomic<int> requests{0};inline HWND status=nullptr;
inline LRESULT CALLBACK procedure(HWND w,UINT m,WPARAM wp,LPARAM lp){
 if(m==WM_CLOSE){DestroyWindow(w);return 0;}
 if(m==WM_DESTROY){active=false;capturePrivacy::detach(w);PostQuitMessage(0);return 0;}
 if(m==WM_TIMER){auto s=L"Local Studio bridge active\r\nNative planner requests: "+std::to_wstring(requests.load())+L"\r\n\r\nOpen the QB Lab place and press Play.\r\nUse the lab panel to spawn bots and change test settings.\r\nClosing this window disconnects the lab.";SetWindowTextW(status,s.c_str());return 0;}
 return DefWindowProcW(w,m,wp,lp);
}
inline void reply(SOCKET s,int code,const std::string& body){
 std::string wire="HTTP/1.1 "+std::to_string(code)+(code==200?" OK":" Error")+"\r\nContent-Type: application/json\r\nCache-Control: no-store\r\nConnection: close\r\nContent-Length: "+std::to_string(body.size())+"\r\n\r\n"+body;
 size_t off=0;while(off<wire.size()){int n=send(s,wire.data()+off,int(wire.size()-off),0);if(n<=0)break;off+=n;}
}
inline void serve(SOCKET listener,std::string token,const std::filesystem::path& folder){
 winrt::init_apartment(winrt::apartment_type::multi_threaded);Planner planner;
 while(active){
  fd_set set;FD_ZERO(&set);FD_SET(listener,&set);timeval wait{0,200000};if(select(0,&set,nullptr,nullptr,&wait)<=0)continue;
  SOCKET s=accept(listener,nullptr,nullptr);if(s==INVALID_SOCKET)continue;
  DWORD timeout=1500;setsockopt(s,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<const char*>(&timeout),sizeof timeout);setsockopt(s,SOL_SOCKET,SO_SNDTIMEO,reinterpret_cast<const char*>(&timeout),sizeof timeout);
  try{
   std::string data;char buf[4096];size_t split=std::string::npos;
   while(data.size()<8192&&(split=data.find("\r\n\r\n"))==std::string::npos){int n=recv(s,buf,sizeof buf,0);if(n<=0)throw std::runtime_error("Header incomplete");data.append(buf,n);}
   if(split==std::string::npos)throw std::runtime_error("Header too large");
   auto header=data.substr(0,split);std::transform(header.begin(),header.end(),header.begin(),[](unsigned char c){return char(std::tolower(c));});
   auto lengthAt=header.find("\r\ncontent-length:");if(lengthAt==std::string::npos||header.find("transfer-encoding:")!=std::string::npos)throw std::runtime_error("Length required");
   auto count=std::stoul(header.substr(lengthAt+17));if(count>60000)throw std::runtime_error("Body too large");
   auto body=data.substr(split+4);while(body.size()<count){int n=recv(s,buf,int(std::min<size_t>(sizeof buf,count-body.size())),0);if(n<=0)throw std::runtime_error("Body incomplete");body.append(buf,n);}
   // Consume the bounded body before rejecting, so closing the socket doesn't
   // reset the connection and discard the HTTP error response.
   if(header.rfind("post /plan http/1.1\r\n",0)!=0||header.find("\r\norigin:")!=std::string::npos){reply(s,403,"{\"error\":\"Lab endpoint only\"}");closesocket(s);continue;}
   if(header.find("\r\nx-vanta-lab: "+token+"\r\n")==std::string::npos&&header.substr(header.find_last_of('\n')+1)!="x-vanta-lab: "+token){reply(s,403,"{\"error\":\"Invalid lab token\"}");closesocket(s);continue;}
   auto query=json::parse(std::wstring(body.begin(),body.begin()+count));auto answer=planner.plan(query);auto wide=json::dump(answer);std::string output(wide.begin(),wide.end());reply(s,200,output);++requests;
   std::ofstream log(folder/L"last-plan.json",std::ios::trunc);log<<output;
  }catch(...){reply(s,400,"{\"error\":\"Invalid lab request\"}");}
  closesocket(s);
 }
 winrt::uninit_apartment();
}
inline int run(HINSTANCE instance,const std::filesystem::path& folder){
 WSADATA ws{};if(WSAStartup(MAKEWORD(2,2),&ws))return 1;
 std::ifstream file(folder/L"vanta-lab-connection.json");std::string text((std::istreambuf_iterator<char>(file)),{});auto config=json::parse(std::wstring(text.begin(),text.end()));
 std::string token;auto key=config[L"token"].str();token.assign(key.begin(),key.end());
 int port=int(config[L"port"].num());if(token.size()!=48||token.find_first_not_of("0123456789abcdef")!=std::string::npos||port<1024||port>65535){MessageBoxW(nullptr,L"Missing or invalid vanta-lab-connection.json next to Vanta.exe.",L"Vanta Studio lab",MB_OK);WSACleanup();return 2;}
 SOCKET listener=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);BOOL exclusive=TRUE;setsockopt(listener,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,reinterpret_cast<char*>(&exclusive),sizeof exclusive);
 sockaddr_in address{};address.sin_family=AF_INET;address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);address.sin_port=htons(static_cast<u_short>(port));
 if(bind(listener,reinterpret_cast<sockaddr*>(&address),sizeof address)||listen(listener,4)){closesocket(listener);WSACleanup();MessageBoxW(nullptr,L"The local lab port is already in use. Close the other lab window.",L"Vanta Studio lab",MB_OK);return 3;}
 active=true;std::thread server(serve,listener,token,folder);
 WNDCLASSW wc{};wc.lpfnWndProc=procedure;wc.hInstance=instance;wc.lpszClassName=L"VantaStudioLab";wc.hbrBackground=GetSysColorBrush(COLOR_WINDOW);wc.hCursor=LoadCursor(nullptr,IDC_ARROW);RegisterClassW(&wc);
 HWND window=CreateWindowW(wc.lpszClassName,L"Vanta | Studio test connection",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,510,250,nullptr,nullptr,instance,nullptr);
 if(!window){active=false;server.join();closesocket(listener);WSACleanup();return 4;}
 capturePrivacy::attach(window);status=CreateWindowW(L"STATIC",L"Waiting for Studio...",WS_CHILD|WS_VISIBLE,22,20,460,190,window,nullptr,instance,nullptr);
 SendMessageW(status,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);SetTimer(window,1,250,nullptr);ShowWindow(window,SW_SHOW);
 MSG msg{};while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}active=false;server.join();closesocket(listener);WSACleanup();return 0;
}
}
