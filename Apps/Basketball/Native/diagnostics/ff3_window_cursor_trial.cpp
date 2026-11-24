#define NOMINMAX
#include "../src/ff3_qb_runtime.hpp"
#include <iostream>
#include <iomanip>
// Diagnostic only. This candidate offset is not part of the app. Only an
// observed eight-byte mouse-coordinate pair is written, with a ten-second
// deadline, object checks and ownership-aware restoration. No input is sent.
constexpr addr windowMousePair=0x68;
struct Pair {float x,y;};
bool same(Pair a,Pair b){return memcmp(&a,&b,sizeof a)==0;}
bool readPair(HANDLE h,addr at,Pair& p){SIZE_T n=0;return ReadProcessMemory(h,reinterpret_cast<void*>(at),&p,sizeof p,&n)&&n==sizeof p&&std::isfinite(p.x)&&std::isfinite(p.y);}
bool writePair(HANDLE h,addr at,Pair p){MEMORY_BASIC_INFORMATION r{};SIZE_T n=0;if(VirtualQueryEx(h,reinterpret_cast<void*>(at),&r,sizeof r)!=sizeof r||r.State!=MEM_COMMIT||r.Protect&(PAGE_GUARD|PAGE_NOACCESS)||at+sizeof p>reinterpret_cast<addr>(r.BaseAddress)+r.RegionSize||((r.Protect&255)!=PAGE_READWRITE&&(r.Protect&255)!=PAGE_WRITECOPY))return false;return WriteProcessMemory(h,reinterpret_cast<void*>(at),&p,sizeof p,&n)&&n==sizeof p;}
int main(int argc,char** argv){
 if(argc>1&&std::string(argv[1])=="--self-test"){
  auto* p=static_cast<unsigned char*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));if(!p)return 10;memset(p,0xA5,4096);unsigned char before[4096];memcpy(before,p,4096);const auto at=reinterpret_cast<addr>(p+24);Pair original{},actual{};
  bool ok=readPair(GetCurrentProcess(),at,original)&&writePair(GetCurrentProcess(),at,{10,20})&&readPair(GetCurrentProcess(),at,actual)&&actual.x==10&&actual.y==20;
  for(int i=0;i<4096;++i)if(i<24||i>=32)ok=ok&&p[i]==before[i];ok=ok&&writePair(GetCurrentProcess(),at,original)&&memcmp(p,before,4096)==0;
  DWORD old=0;ok=ok&&VirtualProtect(p,4096,PAGE_READONLY,&old)&&!writePair(GetCurrentProcess(),at,{30,40});DWORD ignored=0;VirtualProtect(p,4096,old,&ignored);VirtualFree(p,0,MEM_RELEASE);
  std::cout<<(ok?"PASS":"FAIL")<<" own-process eight-byte bounds, readback, restoration and protection checks; no game access\n";return ok?0:11;
 }
 const bool trial=argc>1&&std::string(argv[1])=="--trial";HANDLE h=nullptr;DWORD pid=0;addr base=0;if(!connectRoblox(h,pid,base))return 2;CloseHandle(h);
 h=OpenProcess(PROCESS_VM_READ|PROCESS_QUERY_INFORMATION|SYNCHRONIZE|(trial?(PROCESS_VM_WRITE|PROCESS_VM_OPERATION):0),FALSE,pid);if(!h)return 3;
 for(int i=0;i<8;++i){gameModules::publish(inspectBasketball(h,base,pid));Sleep(250);}basketballConnection::entered=true;basketballConnection::releaseCurrent=true;
 if(gameModules::router.open(now(),true,true)!=gameModules::Id::FootballFusion3){CloseHandle(h);return 4;}
 ff3Native::enabled=true;ff3Native::visualOnly=true;ff3Native::Reader reader;
 const auto session=gameModules::router.get();const auto root=session.model,players=child(h,root,"Players"),player=rd<addr>(h,players+Offsets::Player::LocalPlayer),own=rd<addr>(h,player+Offsets::Player::ModelInstance),ui=child(h,root,"UserInputService"),state=rd<addr>(h,ui+Offsets::UserInputService::WindowInputState),service=child(h,root,"MouseService"),input=rd<addr>(h,service+Offsets::MouseService::InputObject);
 auto owner=[&](){const auto current=gameModules::router.get();return current.model==root&&current.pid==pid&&current.version&&WaitForSingleObject(h,0)==WAIT_TIMEOUT&&rd<addr>(h,player+Offsets::Player::ModelInstance)==own&&rd<addr>(h,ui+Offsets::UserInputService::WindowInputState)==state;};
 auto identity=[&](){return owner()&&!rd<addr>(h,state+Offsets::WindowInputState::CurrentTextBox);};
 Pair original{},applied{};bool owned=false;std::cout<<std::setprecision(12);
 auto restore=[&](){Pair actual{};if(owned&&owner()&&readPair(h,state+windowMousePair,actual)&&same(actual,applied)){std::cout<<"RESTORE "<<writePair(h,state+windowMousePair,original)<<'\n';}owned=false;};
 struct RestoreOnExit {std::function<void()> run;~RestoreOnExit(){run();}} cleanup{restore};
 const auto until=now()+60;ff3Native::Frame start;addr heldBall=0;
 while(now()<until&&identity()){
  gameModules::publish(inspectBasketball(h,base,pid));auto f=reader.read(0,0,false,true);auto tool=child(h,own,"Football"),ball=child(h,tool,"Handle");
  POINT physical{};GetCursorPos(&physical);const double x=physical.x-f.origin.x,y=physical.y-f.origin.y;
  if(ff3Native::focused(pid)&&ff3Native::freeCursor()&&f.held&&!f.menuOpen&&!f.textFocus&&ball&&x>f.client.right*.25&&x<f.client.right*.75&&y>f.client.bottom*.6&&y<f.client.bottom*.9&&GetAsyncKeyState(VK_LBUTTON)>=0&&!(GetAsyncKeyState(VK_RBUTTON)&0x8000)){start=f;heldBall=ball;break;}Sleep(50);
 }
 if(!heldBall||!readPair(h,state+windowMousePair,original)){std::cout<<"NOT_READY no focused equipped football\n";CloseHandle(h);return 5;}
 const auto event=ff3Native::vector(rd<shooting::Vec>(h,input+Offsets::MouseService::MousePosition,{NAN,NAN,NAN}));
 if(std::hypot(original.x-event.x,original.y-event.y)>.25){std::cout<<"REJECTED candidate no longer matches input\n";CloseHandle(h);return 6;}
 applied={float(start.client.right*.5),float(start.client.bottom*.2)};
 const auto began=now();double clicked=0;bool wasDown=false;unsigned writes=0;const auto initial=reader.read();std::vector<std::uint64_t> existing;for(const auto& b:initial.balls)existing.push_back(b.id);
 std::cout<<"BEGIN trial="<<trial<<" pid="<<pid<<" state="<<state<<" original="<<original.x<<','<<original.y<<" target="<<applied.x<<','<<applied.y<<" power="<<start.gamePower<<" heldBall="<<heldBall<<'\n'<<std::flush;
 while(now()-began<10&&identity()&&ff3Native::focused(pid)&&!(GetAsyncKeyState(VK_ESCAPE)&0x8000)&&!(GetAsyncKeyState(VK_RBUTTON)&0x8000)){
  gameModules::publish(inspectBasketball(h,base,pid));
  if(trial){if(!writePair(h,state+windowMousePair,applied)){std::cout<<"WRITE_FAILED "<<GetLastError()<<'\n';break;}owned=true;++writes;}
  const bool down=(GetAsyncKeyState(VK_LBUTTON)&0x8000)!=0;if(down&&!wasDown&&!clicked){clicked=now();std::cout<<"CLICK elapsed="<<clicked-began<<'\n';}wasDown=down;
  auto f=reader.read();Pair window{};readPair(h,state+windowMousePair,window);const auto object=ff3Native::vector(rd<shooting::Vec>(h,input+Offsets::MouseService::MousePosition,{NAN,NAN,NAN}));POINT physical{};GetCursorPos(&physical);
  if(!clicked||now()-clicked<1.5){
   const auto trueRay=ff3qb::aimPixelRay(f.matrix,f.cameraPosition,{double(physical.x-f.origin.x),double(physical.y-f.origin.y),true},f.client.right,f.client.bottom),testRay=ff3qb::aimPixelRay(f.matrix,f.cameraPosition,{applied.x,applied.y,true},f.client.right,f.client.bottom);
   std::cout<<"STATE elapsed="<<now()-began<<" click="<<(clicked?now()-clicked:-1)<<" window="<<window.x<<','<<window.y<<" event="<<object.x<<','<<object.y<<" physical="<<physical.x-f.origin.x<<','<<physical.y-f.origin.y<<" realRay="<<trueRay.x<<','<<trueRay.y<<','<<trueRay.z<<" testRay="<<testRay.x<<','<<testRay.y<<','<<testRay.z<<'\n';
  }
  if(clicked)for(const auto& ball:f.balls){const bool mine=ball.id==heldBall||ball.thrower==player||ball.thrower==own;const bool fresh=std::find(existing.begin(),existing.end(),ball.id)==existing.end();
   if(!mine||!fresh)continue;const auto v=ball.velocity;const double elevation=std::atan2(v.y,std::hypot(v.x,v.z))*180/3.141592653589793;
   std::cout<<"BALL elapsed="<<now()-began<<" sinceClick="<<now()-clicked<<" id="<<ball.id<<" thrower="<<ball.thrower<<" p="<<ball.position.x<<','<<ball.position.y<<','<<ball.position.z<<" v="<<v.x<<','<<v.y<<','<<v.z<<" elevation="<<elevation<<" fit="<<ball.model.valid<<" fitError="<<ball.model.error<<'\n';
  }
  if(clicked&&now()-clicked>2)break;Sleep(4);
 }
 restore();std::cout<<"END writes="<<writes<<" clickObserved="<<(clicked!=0)<<'\n';CloseHandle(h);return clicked?0:7;
}
