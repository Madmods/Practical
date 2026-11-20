#define NOMINMAX
#include "../src/ff3_mags_overlay.hpp"
#include <iostream>
// Explicit --apply runs the app's selective-collision reader/writer at 20 studs, then
// restores it. It never installs hooks or sends catch, jump or throw inputs.
int main(int argc,char** argv){
 if(argc>1&&std::string(argv[1])=="--apply"){
  HANDLE snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);PROCESSENTRY32W entry{sizeof entry};bool appRunning=false;
  if(snapshot==INVALID_HANDLE_VALUE)return 8;
  if(Process32FirstW(snapshot,&entry))do{if(!_wcsicmp(entry.szExeFile,L"Vanta.exe"))appRunning=true;}while(Process32NextW(snapshot,&entry));CloseHandle(snapshot);
  if(appRunning){std::cerr<<"Close Vanta before this bounded write test so only one editor owns ball restoration.\n";return 8;}
 }
 winrt::init_apartment();HANDLE h=nullptr;DWORD pid=0;addr base=0;if(!connectRoblox(h,pid,base))return 2;
 for(int i=0;i<5;++i){gameModules::publish(inspectBasketball(h,base,pid));Sleep(200);}basketballConnection::entered=true;basketballConnection::releaseCurrent=true;
 if(gameModules::router.open(now(),true,true)!=gameModules::Id::FootballFusion3){CloseHandle(h);return 3;}
 const auto initial=inspectBasketball(h,base,pid);ff3Play::magsOn=ff3Play::magsVisual=true;ff3Play::reachTenths=200;ff3Play::magDelayMs=0;
 ff3Play::Reader reader;auto scene=reader.read({});auto hands=ff3Native::catchParts(h,scene.own);auto left=ff3Native::partSize(h,hands.first),right=ff3Native::partSize(h,hands.second);
 auto leftGroup=rd<unsigned char>(h,rd<addr>(h,hands.first+Offsets::BasePart::Primitive)+ff3Play::physicsGroupOffset,255),rightGroup=rd<unsigned char>(h,rd<addr>(h,hands.second+Offsets::BasePart::Primitive)+ff3Play::physicsGroupOffset,255);
 std::cout<<"pid="<<pid<<" valid="<<scene.valid<<" ballParts="<<reader.looseParts.size()<<" size=20 handSizeWrites=0"<<std::endl;
 if(argc<2||std::string(argv[1])!="--apply"){CloseHandle(h);return scene.valid?0:4;}
 wchar_t exe[MAX_PATH]{};GetModuleFileNameW(nullptr,exe,MAX_PATH);auto folder=std::filesystem::path(exe).parent_path();ff3Play::recoveryPath=folder/L"ff3-ball-probe-recovery.json";if(!ff3Play::recoverReach()){CloseHandle(h);return 5;}
 int holdSeconds=5;if(argc>2)holdSeconds=std::clamp(std::atoi(argv[2]),5,60);std::error_code ignored;std::filesystem::remove(folder/L"ff3-ball-probe.stop",ignored);
 ff3Play::Mags mags;std::vector<ff3Play::BallEdits::Edit> originals;bool applied=false,meshVerified=false,handsUnchanged=true,collisionVerified=false,otherFlagsPreserved=true;double start=now(),appliedAt=0,nextWaiting=0;std::string reason="No eligible focused loose football observed";
 while(now()-start<60+holdSeconds&&WaitForSingleObject(h,0)==WAIT_TIMEOUT&&!std::filesystem::exists(folder/L"ff3-ball-probe.stop")){
  auto session=inspectBasketball(h,base,pid);gameModules::publish(session);if(session.model!=initial.model){reason="Session changed";break;}scene=reader.read({});
  if(!ff3Play::observing(scene,now())){Sleep(50);continue;}
  handsUnchanged=handsUnchanged&&ff3Play::sameSize(ff3Native::partSize(h,hands.first),left)&&ff3Play::sameSize(ff3Native::partSize(h,hands.second),right);if(!handsUnchanged){reason="Catch parts changed externally";break;}
  std::map<std::uint64_t,unsigned char> before;for(const auto& p:reader.looseParts)before[p.part.id]=rd<unsigned char>(h,p.part.primitive+Offsets::Primitive::Flags,255);
  if(!mags.apply(scene,reader.looseParts)){reason="Application failed";break;}
  if(!mags.edits.edits.empty()){
   bool verified=true,allSolid=true;for(const auto& e:mags.edits.edits){auto scale=ff3Native::vector(rd<shooting::Vec>(h,e.original.part.mesh+Offsets::DataModelMesh::Scale,{NAN,NAN,NAN}));auto flags=rd<unsigned char>(h,e.original.part.primitive+Offsets::Primitive::Flags,255);const auto mask=Offsets::PrimitiveFlags::CanCollide;allSolid=allSolid&&flags!=255&&bool(flags&mask);if(before.count(e.original.part.id)&&before[e.original.part.id]!=255)otherFlagsPreserved=otherFlagsPreserved&&((flags&(Offsets::PrimitiveFlags::CanTouch|Offsets::PrimitiveFlags::CanQuery))==(before[e.original.part.id]&(Offsets::PrimitiveFlags::CanTouch|Offsets::PrimitiveFlags::CanQuery)));verified=verified&&rd<unsigned char>(h,e.original.part.primitive+ff3Play::physicsGroupOffset,255)==e.groupApplied&&ff3Play::sameSize(ff3Native::partSize(h,e.original.part.id),{20,20,20})&&ff3Play::sameSize(scale,e.original.meshScale);}
   if(verified&&otherFlagsPreserved&&!applied){applied=meshVerified=collisionVerified=true;appliedAt=now();originals=mags.edits.edits;std::cout<<"applied20=1 normalCollisionStatePreserved=1 touchAndQueryPreserved="<<otherFlagsPreserved<<" normalMeshVerified=1 balls="<<originals.size()<<std::endl;}

  }
  if(!applied&&now()-start>=nextWaiting){nextWaiting=now()-start+5;std::cout<<"waitingForPass=1 ground="<<reader.ground<<" airborne="<<scene.balls.size()<<" eligible="<<reader.looseParts.size()<<std::endl;}
  if(applied&&now()-appliedAt>=holdSeconds){reason="Completed bounded player-filtered 20-stud verification with original collision states";break;}Sleep(30);
 }
 bool restored=mags.restore();for(int i=0;!restored&&i<30;++i){Sleep(100);restored=mags.restore();}unsigned survivors=0;bool survivorOriginals=true;
 for(const auto& e:originals){ff3Play::BallPart live;if(mags.read(e.original,live)&&live.part.id==e.original.part.id&&live.part.primitive==e.original.part.primitive){++survivors;survivorOriginals=survivorOriginals&&ff3Play::sameSize(live.part.size,e.original.part.size)&&live.physicsGroup==e.original.physicsGroup&&live.canCollide==e.original.canCollide&&(live.part.mesh!=e.original.part.mesh||ff3Play::sameSize(live.meshScale,e.original.meshScale));}}
 bool groupsRestored=rd<unsigned char>(h,rd<addr>(h,hands.first+Offsets::BasePart::Primitive)+ff3Play::physicsGroupOffset,255)==leftGroup&&rd<unsigned char>(h,rd<addr>(h,hands.second+Offsets::BasePart::Primitive)+ff3Play::physicsGroupOffset,255)==rightGroup;
 using namespace basketballUi;auto data=object();data.o[L"catchGroupsRestored"]=basketballUi::boolean(groupsRestored);data.o[L"pid"]=number(pid);data.o[L"applied20Verified"]=basketballUi::boolean(applied);data.o[L"normalCollisionStatePreservedVerified"]=basketballUi::boolean(collisionVerified);data.o[L"selectiveCollisionVerified"]=basketballUi::boolean(applied&&collisionVerified);data.o[L"touchAndQueryFlagsPreserved"]=basketballUi::boolean(otherFlagsPreserved);data.o[L"normalMeshVerified"]=basketballUi::boolean(meshVerified);data.o[L"restorationComplete"]=basketballUi::boolean(restored);data.o[L"survivingBalls"]=number(survivors);data.o[L"survivorOriginalsVerified"]=basketballUi::boolean(survivorOriginals);data.o[L"catchPartsUnchanged"]=basketballUi::boolean(handsUnchanged);data.o[L"inputsOrPacketsSent"]=basketballUi::boolean(false);data.o[L"serverCatchOutcomesVerified"]=basketballUi::boolean(false);data.o[L"reason"]=string(widen(reason));{std::ofstream out(folder/L"ff3-ball-mags-live.json");out<<winrt::to_string(json::dump(data));}
 std::cout<<"restored="<<restored<<" handsUnchanged="<<handsUnchanged<<" survivors="<<survivors<<" originalSizesVerified="<<survivorOriginals<<std::endl;CloseHandle(h);return applied&&collisionVerified&&otherFlagsPreserved&&restored&&handsUnchanged&&groupsRestored&&survivorOriginals?0:6;
}
