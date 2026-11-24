#define NOMINMAX
#include "../src/ff3_qb_runtime.hpp"
#include <iostream>
#include <iomanip>
// Narrow read-only inspection of mouse/input objects. No hooks, writes,
// synthetic inputs, executable-memory inspection or transport access.
bool readablePrivate(HANDLE h,addr pointer){MEMORY_BASIC_INFORMATION m{};return pointer>=0x10000&&VirtualQueryEx(h,reinterpret_cast<void*>(pointer),&m,sizeof m)==sizeof m&&m.State==MEM_COMMIT&&m.Type==MEM_PRIVATE&&!(m.Protect&(PAGE_GUARD|PAGE_NOACCESS));}
void candidates(HANDLE h,const char* label,addr object,size_t first,size_t end,ff3qb::Vec pixel,ff3qb::Vec eye,ff3qb::Vec ray){
 if(!readablePrivate(h,object))return;
 for(size_t offset=first;offset+12<=end;offset+=4){const auto value=ff3Native::vector(rd<shooting::Vec>(h,object+offset,{NAN,NAN,NAN}));if(!ff3qb::finite(value)||ff3qb::length(value)>100000)continue;
  const bool cursor=ff3qb::finite(pixel)&&std::hypot(value.x-pixel.x,value.y-pixel.y)<.25;
  const auto delta=value-eye;const auto distance=ff3qb::dot(delta,ray),gap=ff3qb::length(delta-ray*distance);
  const bool world=ff3qb::finite(ray)&&distance>1&&distance<1100&&gap<.5;
  if(cursor||world)std::cout<<"CANDIDATE "<<label<<" object="<<object<<" offset="<<offset<<" value="<<value.x<<','<<value.y<<','<<value.z<<" pixelMatch="<<cursor<<" rayMatch="<<world<<" rayDistance="<<distance<<" rayGap="<<gap<<'\n';
 }
}
int main(){
 HANDLE h=nullptr;DWORD pid=0;addr base=0;if(!connectRoblox(h,pid,base))return 2;CloseHandle(h);h=OpenProcess(PROCESS_VM_READ|PROCESS_QUERY_INFORMATION|SYNCHRONIZE,FALSE,pid);if(!h)return 3;
 for(int i=0;i<8;++i){gameModules::publish(inspectBasketball(h,base,pid));Sleep(250);}basketballConnection::entered=true;basketballConnection::releaseCurrent=true;
 if(gameModules::router.open(now(),true,true)!=gameModules::Id::FootballFusion3){const auto s=gameModules::router.get();std::cout<<"UNAVAILABLE pid="<<s.pid<<" place="<<s.placeId<<" universe="<<s.universeId<<" model="<<s.model<<" process="<<s.process<<" opened="<<s.opened<<" version="<<s.version<<" loaded="<<s.loaded<<" player="<<s.player<<" world="<<s.world<<" renderer="<<s.renderer<<'\n';CloseHandle(h);return 4;}
 ff3Native::enabled=true;ff3Native::visualOnly=true;ff3Native::Reader reader;std::cout<<std::setprecision(12);
 for(int sample=0;sample<12&&WaitForSingleObject(h,0)==WAIT_TIMEOUT;++sample){
  gameModules::publish(inspectBasketball(h,base,pid));const auto session=gameModules::router.get();const auto root=session.model,players=child(h,root,"Players"),player=rd<addr>(h,players+Offsets::Player::LocalPlayer),mouse=rd<addr>(h,player+Offsets::Player::Mouse);
  const auto service=child(h,root,"MouseService"),input=rd<addr>(h,service+Offsets::MouseService::InputObject),input2=rd<addr>(h,service+Offsets::MouseService::InputObject2),ui=child(h,root,"UserInputService"),window=rd<addr>(h,ui+Offsets::UserInputService::WindowInputState);
  const auto f=reader.read();const auto point=ff3Native::vector(rd<shooting::Vec>(h,input+Offsets::MouseService::MousePosition,{NAN,NAN,NAN}));
  const auto ray=ff3qb::aimPixelRay(f.matrix,f.cameraPosition,{point.x,point.y,true},f.client.right,f.client.bottom);POINT cursor{};GetCursorPos(&cursor);
  const auto mouseKind=defense::classOf(h,mouse),inputKind=defense::classOf(h,input),secondKind=defense::classOf(h,input2);
  std::cout<<"SAMPLE "<<sample<<" pid="<<pid<<" focused="<<ff3Native::focused(pid)<<" player="<<player<<" mouse="<<mouse<<" mouseClass="<<mouseKind<<" input="<<input<<" inputClass="<<inputKind<<" input2="<<input2<<" input2Class="<<secondKind<<" windowState="<<window<<" pixel="<<point.x<<','<<point.y<<" windows="<<cursor.x-f.origin.x<<','<<cursor.y-f.origin.y<<" eye="<<f.cameraPosition.x<<','<<f.cameraPosition.y<<','<<f.cameraPosition.z<<" ray="<<ray.x<<','<<ray.y<<','<<ray.z<<'\n';
  if(inputKind=="InputObject")candidates(h,"input",input,0x90,0x180,point,f.cameraPosition,ray);
  if(secondKind=="InputObject")candidates(h,"input2",input2,0x90,0x180,point,f.cameraPosition,ray);
  if(mouseKind=="PlayerMouse"||mouseKind=="Mouse"){
   candidates(h,"mouse",mouse,0x90,0x180,point,f.cameraPosition,ray);
   for(size_t offset=0x90;offset<0x140;offset+=8){const auto ref=rd<addr>(h,mouse+offset);if(ref==input||ref==input2||ref==root||ref==f.workspace||!readablePrivate(h,ref))continue;const auto kind=defense::classOf(h,ref);if(!kind.empty())continue;candidates(h,"mouse-data",ref,0,0x80,point,f.cameraPosition,ray);}
  }
  if(window&&!rd<addr>(h,window+Offsets::WindowInputState::CurrentTextBox))candidates(h,"window-input",window,0,0x180,point,f.cameraPosition,ray);
  Sleep(200);
 }CloseHandle(h);return 0;
}
