#define NOMINMAX
#include "../src/game_module_ui.hpp"
#include "../src/ff3_play_runtime.hpp"
#include <iostream>
namespace {
using namespace ff3Play;
struct MotionFrame {float rotation[9];shooting::Vec position,linear,angular;};
static_assert(sizeof(MotionFrame)==72,"Motion-frame layout");
struct History {addr pointer=0;MotionFrame a{},b{};};
bool validFrame(const MotionFrame& f){return snapRotationValid(f.rotation)&&ff3qb::finite(ff3Native::vector(f.position))&&ff3qb::finite(ff3Native::vector(f.linear))&&ff3qb::finite(ff3Native::vector(f.angular))&&ff3qb::length(ff3Native::vector(f.linear))<=250&&ff3qb::length(ff3Native::vector(f.angular))<=1000;}
bool history(HANDLE h,addr primitive,History& out){
 if(!snapPoseLayoutAdmitted())return false;out.pointer=rd<addr>(h,primitive+0x1e8);if(out.pointer<65536||out.pointer>0x7fffffffffffULL||out.pointer%8)return false;SIZE_T bytes=0;
 if(!ReadProcessMemory(h,reinterpret_cast<void*>(out.pointer+0x8c),&out.a,sizeof out.a,&bytes)||bytes!=sizeof out.a||!ReadProcessMemory(h,reinterpret_cast<void*>(out.pointer+0x11c),&out.b,sizeof out.b,&bytes)||bytes!=sizeof out.b)return false;
 return validFrame(out.a)&&validFrame(out.b)&&ff3qb::length(ff3Native::vector(out.a.position)-ff3Native::vector(out.b.position))<100;
}
template<class Guard> bool transfer(HANDLE h,addr primitive,const History& before,Vec target,Vec velocity,Guard guard){
 if(!ff3qb::finite(target)||!ff3qb::finite(velocity)||ff3qb::length(velocity)>180||!guard()||rd<addr>(h,primitive+0x1e8)!=before.pointer)return false;
 struct Value {shooting::Vec position,linear;};Value desired{{float(target.x),float(target.y),float(target.z)},{float(velocity.x),float(velocity.y),float(velocity.z)}},originals[2]={{before.a.position,before.a.linear},{before.b.position,before.b.linear}};
 const addr fields[2]={before.pointer+0xb0,before.pointer+0x140};size_t applied=0;
 auto restore=[&](size_t index,size_t count){if(!count||count>sizeof desired||!guard()||rd<addr>(h,primitive+0x1e8)!=before.pointer)return;Value actual{};SIZE_T bytes=0;if(ReadProcessMemory(h,reinterpret_cast<void*>(fields[index]),&actual,count,&bytes)&&bytes==count&&!memcmp(&actual,&desired,count))WriteProcessMemory(h,reinterpret_cast<void*>(fields[index]),&originals[index],count,&bytes);};
 for(;applied<2;++applied){if(!guard()||rd<addr>(h,primitive+0x1e8)!=before.pointer)break;SIZE_T bytes=0;if(!WriteProcessMemory(h,reinterpret_cast<void*>(fields[applied]),&desired,sizeof desired,&bytes)||bytes!=sizeof desired){restore(applied,bytes);break;}}
 if(applied==2)return true;
 while(applied){--applied;restore(applied,sizeof desired);}
 return false;
}
bool selfTest(){
 std::vector<unsigned char> primitive(0x300),buffer(0x300,0x7a);const auto p=reinterpret_cast<addr>(primitive.data()),b=reinterpret_cast<addr>(buffer.data());memcpy(primitive.data()+0x1e8,&b,8);MotionFrame frame{};frame.rotation[0]=frame.rotation[4]=frame.rotation[8]=1;frame.position={3,4,5};memcpy(buffer.data()+0x8c,&frame,sizeof frame);memcpy(buffer.data()+0x11c,&frame,sizeof frame);auto original=buffer;History frames;unsigned passed=0;
 if(history(GetCurrentProcess(),p,frames))++passed;
 if(transfer(GetCurrentProcess(),p,frames,{10,11,12},{1,2,3},[]{return true;}))++passed;
 const auto a=rd<shooting::Vec>(GetCurrentProcess(),b+0xb0),second=rd<shooting::Vec>(GetCurrentProcess(),b+0x140);if(a.x==10&&a.y==11&&a.z==12&&second.x==10&&second.y==11&&second.z==12)++passed;
 bool untouched=true;for(size_t i=0;i<buffer.size();++i)if(!((i>=0xb0&&i<0xc8)||(i>=0x140&&i<0x158)))untouched&=buffer[i]==original[i];if(untouched)++passed;
 const auto after=buffer;if(!transfer(GetCurrentProcess(),p,frames,{9,9,9},{},[]{return false;})&&buffer==after)++passed;
 buffer[0x8c]=0;std::fill(buffer.begin()+0x8c,buffer.begin()+0x8c+36,0);if(!history(GetCurrentProcess(),p,frames))++passed;
 std::cout<<passed<<" motion-buffer self-process checks passed\n";return passed==6;
}
}
int main(int argc,char** argv){
 // Live mutations require the explicit --run flag. No inputs are installed,
 // sent or synthesized. Only one verified loose football is trialled.
 if(argc==2&&std::string(argv[1])=="--self-test")return selfTest()?0:1;
 const bool alongside=argc==2&&std::string(argv[1])=="--with-vanta";
 if(argc!=2||std::string(argv[1])!="--run"&&!alongside){std::cout<<"Explicit --run or --with-vanta required\n";return 2;}
 const auto app=std::filesystem::weakly_canonical(argv[0]).parent_path().parent_path().parent_path();
 if(!alongside&&(!std::filesystem::exists(app/L"ff3-play-stop")||!std::filesystem::exists(app/L"ff3-play-stop.done"))){std::cerr<<"Pause Vanta receiving assists before this exclusive trial\n";return 6;}
 winrt::init_apartment();HANDLE ro=nullptr;DWORD pid=0;addr base=0;if(!connectRoblox(ro,pid,base))return 3;
 for(int i=0;i<5;++i){gameModules::publish(inspectBasketball(ro,base,pid));Sleep(200);}basketballConnection::entered=basketballConnection::releaseCurrent=true;
 if(!snapPoseLayoutAdmitted()||gameModules::router.open(now(),true,true)!=gameModules::Id::FootballFusion3){CloseHandle(ro);return 4;}
 HANDLE h=OpenProcess(PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_VM_OPERATION|SYNCHRONIZE,FALSE,pid);if(!h){CloseHandle(ro);return 5;}
 magsOn=true;Reader reader;Scene scene;BallPart selected;double range=22,started=now(),trialAt=0,nextRead=0,nextInspect=0;unsigned writes=0,failures=0,samples=0,renderedAtHand=0;bool synced=false;std::string reason="No eligible football entered the configured sphere";
 try{auto cfg=basketballUi::readConfig(app/L"ff3-play-settings.json");const auto value=cfg[L"ff3_mags"][L"rows"][L"studs"].num();if(std::isfinite(combinedSnapSize(value)))range=combinedSnapSize(value);}catch(...){}
 auto appBall=[&]()->addr{if(!alongside)return 0;try{const auto state=basketballUi::readConfig(app/L"ff3-play-state.json");const auto time=state[L"at"].num();if(state[L"pid"].num()!=pid||!state[L"valid"].boolean()||!state[L"focused"].boolean()||state[L"possessed"].boolean()||state[L"menu"].boolean()||state[L"typing"].boolean()||!state[L"combinedMags"].boolean()||state[L"snapRange"].num()!=range||!std::isfinite(time)||now()<time||now()-time>.6||state[L"handSnap"][L"phase"].num()!=2)return 0;return addr(state[L"handSnap"][L"ball"].num());}catch(...){return 0;}};
 std::vector<std::array<double,12>> readings;Vec initialVisible{NAN,NAN,NAN};
 while(now()-started<(alongside?180:45)){
  if(now()>=nextInspect){gameModules::publish(inspectBasketball(ro,base,pid));nextInspect=now()+.5;}
  if(now()>=nextRead){scene=reader.read({});nextRead=now()+.05;}if(!observing(scene,now())||scene.possessed||!reader.stillSafe(scene)){if(trialAt){reason="Trial stopped: focus, scene or possession changed";break;}Sleep(10);continue;}
  const auto hand=HandSnapAccess::contact(h,scene);if(!hand.valid(scene)){Sleep(5);continue;}
  if(!selected.part.id){double best=snapRadius(range);const auto activeBall=appBall();for(const auto& candidate:reader.looseParts){History frames;if(alongside&&candidate.part.id!=activeBall||!candidate.canCollide||!history(h,candidate.part.primitive,frames)||ff3qb::length(ff3Native::vector(frames.a.linear))>1||ff3qb::length(ff3Native::vector(frames.b.linear))>1)continue;const auto displayed=snapRenderedPosition(h,candidate.part);const double gap=snapDistance(displayed,hand.position);if(!ff3qb::finite(displayed)||gap>best||snapDistance(ff3Native::vector(frames.b.position),displayed)>1)continue;best=gap;selected=candidate;initialVisible=displayed;}if(!selected.part.id){Sleep(10);continue;}trialAt=now();reason="Motion-buffer trial completed";}
  if(now()-trialAt>3)break;const auto seed=selected.part;History frames;if(!history(h,seed.primitive,frames)){reason="Motion buffer no longer validates";break;}
  auto guard=[&](){return (!alongside||appBall()==seed.id)&&observing(scene,now())&&reader.stillSafe(scene)&&rd<addr>(h,seed.id+Offsets::BasePart::Primitive)==seed.primitive&&rd<addr>(h,seed.id+Offsets::Instance::Parent)==seed.rig&&rd<addr>(h,seed.mesh+Offsets::Instance::Parent)==seed.id&&rd<addr>(h,seed.primitive+Offsets::Primitive::Owner)==seed.id&&ff3Native::looseWorldBall(h,seed.id,reader.workspace)&&HandSnapAccess::contact(h,scene).valid(scene);};
  if(!guard()){reason="Ball identity changed";break;}
  const auto target=snapHandTarget(hand),displayed=snapRenderedPosition(h,seed);if(!withinSnapDistance(ff3Native::vector(frames.b.position),hand.position,range)){reason="Ball left the configured sphere";break;}
  if(!synced&&!alongside){if(ff3qb::finite(displayed)&&snapDistance(displayed,ff3Native::vector(frames.b.position))<1)writeSnapFields(h,seed.primitive,ff3Native::vector(frames.b.position),ff3Native::vector(frames.b.linear),guard);synced=true;}
  if(transfer(h,seed.primitive,frames,target,hand.velocity,guard))++writes;else ++failures;
  const auto visible=snapRenderedPosition(h,seed);++samples;if(ff3qb::finite(visible)&&snapDistance(visible,hand.position)<1)++renderedAtHand;
  if(readings.size()<600&&ff3qb::finite(visible))readings.push_back({now()-trialAt,target.x,target.y,target.z,visible.x,visible.y,visible.z,double(frames.b.position.x),double(frames.b.position.y),double(frames.b.position.z),snapDistance(visible,hand.position),snapDistance(visible,initialVisible)});Sleep(5);
 }
 using namespace basketballUi;auto out=object();out.o[L"pid"]=number(pid);out.o[L"ball"]=number(double(selected.part.id));out.o[L"localMotionBufferWrites"]=number(writes);out.o[L"writeFailures"]=number(failures);out.o[L"renderSamples"]=number(samples);out.o[L"renderedAtHandSamples"]=number(renderedAtHand);out.o[L"agentGameInputs"]=basketballUi::boolean(false);out.o[L"alongsideVanta"]=basketballUi::boolean(alongside);out.o[L"rangeFullWidth"]=number(range);out.o[L"reason"]=string(widen(reason));auto data=array();for(const auto& reading:readings){auto row=array();for(double value:reading)row.a.push_back(number(value));data.a.push_back(row);}out.o[L"samples"]=data;std::cout<<winrt::to_string(json::dump(out));CloseHandle(h);CloseHandle(ro);winrt::uninit_apartment();return 0;
}
