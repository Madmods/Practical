#pragma once
// Included after ff3Play's Reader and input helpers are defined.
namespace ff3Play {
// The two world poses were inspected in this client build. Keep the extra
// pose layout version-bound rather than carrying it across an offset update.
inline bool snapPoseLayoutAdmitted(){return Offsets::ClientVersion=="version-2c6fd5e09aaf4cfa";}
inline constexpr addr snapPoseStride=0x48;
inline constexpr addr snapSecondPosition=Offsets::Primitive::Position+snapPoseStride;
struct SnapWorldPoses {
 float rotation[9];shooting::Vec position,linear,angular;
 float secondRotation[9];shooting::Vec secondPosition;
};
static_assert(sizeof(SnapWorldPoses)==120&&offsetof(SnapWorldPoses,secondPosition)==108,"World pose pair layout changed");
inline bool snapRotationValid(const float* r){
 Vec axes[3]={{r[0],r[3],r[6]},{r[1],r[4],r[7]},{r[2],r[5],r[8]}};for(auto axis:axes)if(!ff3qb::finite(axis)||std::abs(ff3qb::length(axis)-1)>.05)return false;
 return std::abs(ff3qb::dot(axes[0],axes[1]))<.05&&std::abs(ff3qb::dot(axes[0],axes[2]))<.05&&std::abs(ff3qb::dot(axes[1],axes[2]))<.05;
}
inline Vec readSnapPalmPosition(HANDLE h,addr primitive,Vec size,bool r6){
 struct Transform{float rotation[9];shooting::Vec position;};Transform pose{};SIZE_T bytes=0;
 if(!h||!primitive||!ReadProcessMemory(h,reinterpret_cast<void*>(primitive+Offsets::Primitive::Rotation),&pose,sizeof pose,&bytes)||bytes!=sizeof pose||!snapRotationValid(pose.rotation))return {NAN,NAN,NAN};
 return snapPalmPosition(ff3Native::vector(pose.position),size,{pose.rotation[1],pose.rotation[4],pose.rotation[7]},r6);
}
inline bool readSnapWorldPoses(HANDLE h,addr primitive,SnapWorldPoses& out){
 if(!snapPoseLayoutAdmitted()||!h||!primitive)return false;SIZE_T bytes=0;if(!ReadProcessMemory(h,reinterpret_cast<void*>(primitive+Offsets::Primitive::Rotation),&out,sizeof out,&bytes)||bytes!=sizeof out)return false;
 const auto a=ff3Native::vector(out.position),b=ff3Native::vector(out.secondPosition);
 return snapRotationValid(out.rotation)&&snapRotationValid(out.secondRotation)&&ff3qb::finite(a)&&ff3qb::finite(b)&&ff3qb::length(a-b)<=maxMagSize*2&&ff3qb::finite(ff3Native::vector(out.linear))&&ff3qb::finite(ff3Native::vector(out.angular));
}
inline Vec snapWorldPosition(HANDLE h,addr primitive){SnapWorldPoses pose{};return readSnapWorldPoses(h,primitive,pose)?ff3Native::vector(pose.secondPosition):Vec{NAN,NAN,NAN};}
inline Vec snapRenderedPosition(HANDLE h,const CatchPart& part){
 const Vec missing{NAN,NAN,NAN};if(!snapPoseLayoutAdmitted()||!part.ball||!part.id||!part.primitive||!part.mesh||rd<addr>(h,part.id+Offsets::BasePart::Primitive)!=part.primitive||rd<addr>(h,part.id+Offsets::Instance::Parent)!=part.rig||rd<addr>(h,part.mesh+Offsets::Instance::Parent)!=part.id)return missing;
 const auto binding=rd<addr>(h,part.id+0x180),table=rd<addr>(h,binding),locator=rd<addr>(h,table-8);if(!binding||rd<unsigned>(h,locator)!=1)return missing;
 const auto image=locator-rd<unsigned>(h,locator+20),type=image+rd<unsigned>(h,locator+12);constexpr char expected[]=".?AVInstancedBindingSpecialMesh@Graphics@RBX@@";char actual[sizeof expected]{};SIZE_T bytes=0;
 if(!ReadProcessMemory(h,reinterpret_cast<void*>(type+16),actual,sizeof actual,&bytes)||bytes!=sizeof actual||memcmp(actual,expected,sizeof expected))return missing;
 const auto a=ff3Native::vector(rd<shooting::Vec>(h,binding+0xac,{NAN,NAN,NAN})),b=ff3Native::vector(rd<shooting::Vec>(h,binding+0xdc,{NAN,NAN,NAN}));return ff3qb::finite(a)&&ff3qb::finite(b)&&ff3qb::length(a-b)<maxMagSize*2?a:missing;
}
#include "ff3_hand_snap_motion.hpp"
inline Vec snapTransferOrigin(HANDLE h,addr primitive){SnapMotionFrames frames{};return readSnapMotionFrames(h,primitive,frames)?ff3Native::vector(frames.b.position):snapWorldPosition(h,primitive);}
struct SnapWriteResult {bool written=false,readbackNear=false,motionWritten=false;addr motionPointer=0;};
template<class Guard> inline SnapWriteResult writeSnapFields(HANDLE h,addr primitive,Vec position,Vec velocity,Guard guard,double fullSize=maxMagSize){
 if(!h||!primitive||!ff3qb::finite(position)||!ff3qb::finite(velocity)||ff3qb::length(velocity)>180||!guard())return {};
 static_assert(Offsets::Primitive::AssemblyLinearVelocity==Offsets::Primitive::Position+sizeof(shooting::Vec),"Ball motion layout requires revalidation");
 SnapWorldPoses poses{};if(!readSnapWorldPoses(h,primitive,poses))return {};SnapMotionFrames currentMotion{};
 if(readSnapMotionFrames(h,primitive,currentMotion)&&!withinSnapDistance(ff3Native::vector(currentMotion.b.position),position,fullSize))return {};
 struct Motion{shooting::Vec position,velocity;};Motion before{poses.position,poses.linear};SIZE_T bytes=0;const auto destination=primitive+Offsets::Primitive::Position,secondDestination=primitive+snapSecondPosition;
 const Motion desired{{float(position.x),float(position.y),float(position.z)},{float(velocity.x),float(velocity.y),float(velocity.z)}};
 auto restore=[&](addr field,const void* after,const void* original,size_t count){if(!count||!guard())return;std::array<unsigned char,sizeof(Motion)> actual{};SIZE_T read=0;if(count<=actual.size()&&ReadProcessMemory(h,reinterpret_cast<void*>(field),actual.data(),count,&read)&&read==count&&!memcmp(actual.data(),after,count)){SIZE_T restored=0;WriteProcessMemory(h,reinterpret_cast<void*>(field),original,count,&restored);}};
 // Update the other world pose as well as the primary position/velocity.
 // Graphics bindings, meshes, ownership and catch flags are never written.
 if(!guard())return {};bytes=0;if(!WriteProcessMemory(h,reinterpret_cast<void*>(secondDestination),&desired.position,sizeof desired.position,&bytes)||bytes!=sizeof desired.position){restore(secondDestination,&desired.position,&poses.secondPosition,bytes);return {};}
 if(!guard()){restore(secondDestination,&desired.position,&poses.secondPosition,sizeof desired.position);return {};}
 bytes=0;if(!WriteProcessMemory(h,reinterpret_cast<void*>(destination),&desired,sizeof desired,&bytes)||bytes!=sizeof desired){
  restore(destination,&desired,&before,bytes);restore(secondDestination,&desired.position,&poses.secondPosition,sizeof desired.position);return {};
 }
 // The optional motion frames are resampled for every write, including after
 // a new ball is acquired or the engine replaces/removes the buffer. Updating
 // only the two stored poses lets interpolation keep displaying the old ball.
 SnapMotionFrames motion{};bool motionWritten=false;
 if(readSnapMotionFrames(h,primitive,motion))motionWritten=writeSnapMotionFrames(h,primitive,motion,position,velocity,guard,fullSize);
 const auto current=ff3Native::vector(rd<shooting::Vec>(h,primitive+Offsets::Primitive::Position,{NAN,NAN,NAN}));
 const auto second=ff3Native::vector(rd<shooting::Vec>(h,secondDestination,{NAN,NAN,NAN}));
 return {true,ff3qb::finite(current)&&ff3qb::finite(second)&&ff3qb::length(current-position)<.5&&ff3qb::length(second-position)<.5,motionWritten,motionWritten?motion.pointer:0};
}

struct HandSnapAccess {
 HANDLE h=nullptr;DWORD pid=0;bool readbackNear=false,renderedNear=false;Vec lastOrigin{NAN,NAN,NAN},lastDestination{NAN,NAN,NAN},renderedPosition{NAN,NAN,NAN};double lastDistance=INFINITY;
 ~HandSnapAccess(){if(h)CloseHandle(h);}
 bool bind(DWORD target){if(pid!=target||h&&WaitForSingleObject(h,0)!=WAIT_TIMEOUT){if(h)CloseHandle(h);h=nullptr;pid=target;}if(!h&&target)h=OpenProcess(PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_VM_OPERATION|SYNCHRONIZE,FALSE,target);return h!=nullptr;}
 static SnapContact contact(HANDLE h,const Scene& s){
  SnapContact out;auto right=child(h,s.own,"RightHand");bool r6=false;if(!right){right=child(h,s.own,"Right Arm");r6=true;}
  if(!right||rd<addr>(h,right+Offsets::Instance::Parent)!=s.own)return out;const auto primitive=rd<addr>(h,right+Offsets::BasePart::Primitive);if(!primitive)return out;
  const auto flags=rd<unsigned char>(h,primitive+Offsets::Primitive::Flags,255);if(flags==255)return out;
  const auto size=ff3Native::partSize(h,right);out={right,primitive,s.own,readSnapPalmPosition(h,primitive,size,r6),ff3Native::velocity(h,right),size,bool(flags&Offsets::PrimitiveFlags::CanTouch)};
  return out.valid(s)?out:SnapContact{};
 }
 static std::vector<SnapBall> observe(HANDLE h,const Scene& s,const std::vector<BallPart>& parts,const HandSnap* controller=nullptr){
  std::vector<SnapBall> out;const auto ws=child(h,s.model,"Workspace");
  auto read=[&](const BallPart& seed,std::uint64_t priorThrower){const auto& p=seed.part;
   if(!p.ball||!p.id||!p.primitive||!p.mesh||rd<addr>(h,p.id+Offsets::Instance::Parent)!=p.rig||rd<addr>(h,p.id+Offsets::BasePart::Primitive)!=p.primitive||rd<addr>(h,p.mesh+Offsets::Instance::Parent)!=p.id||!instanceClassIs(h,p.id,"Part")||!path(h,p.id,{"Configuration","Gravity"})||!ff3Native::looseWorldBall(h,p.id,ws))return;
   const auto flags=rd<unsigned char>(h,p.primitive+Offsets::Primitive::Flags,255);if(flags==255)return;auto current=seed;current.position=snapTransferOrigin(h,p.primitive);current.part.size=ff3Native::partSize(h,p.id);const auto velocity=ff3Native::velocity(h,p.id);if(!ff3qb::finite(current.position)||!ff3qb::finite(velocity))return;
   const auto observation=std::find_if(s.looseBalls.begin(),s.looseBalls.end(),[&](const auto& b){return b.id==p.id&&b.verifiedBall;});const auto thrower=observation==s.looseBalls.end()?priorThrower:observation->thrower;
   out.push_back({current,velocity,thrower,bool(flags&Offsets::PrimitiveFlags::Anchored)});
  };
  for(const auto& p:parts){if(out.size()>=maxBallEdits)break;const auto observation=std::find_if(s.looseBalls.begin(),s.looseBalls.end(),[&](const auto& b){return b.id==p.part.id&&b.verifiedBall;});if(observation!=s.looseBalls.end())read(p,observation->thrower);}
  // A selected ball gets a direct live identity/position read every update;
  // a transient absence from the discovery list does not stop the transfer.
  if(controller&&controller->busy()&&controller->tracked.part.part.id&&std::none_of(out.begin(),out.end(),[&](const auto& b){return b.part.part.id==controller->ball;}))read(controller->tracked.part,controller->tracked.thrower);
  return out;
 }
 bool write(const Scene& s,SnapCommand& command,double range){
  readbackNear=renderedNear=false;renderedPosition={NAN,NAN,NAN};
  if(!command.move||!magsOn||!std::isfinite(range)||range<1||range>maxSnapSize||!bind(s.pid))return false;
  const auto seed=command.ball.part;const auto ws=child(h,s.model,"Workspace");const auto currentHand=contact(h,s);if(!currentHand.valid(s))return false;command.destination=snapHandTarget(currentHand);lastDestination=command.destination;lastOrigin=snapWorldPosition(h,seed.primitive);lastDistance=snapDistance(lastOrigin,currentHand.position);command.velocity=currentHand.velocity;
  auto guard=[&](){
   if(!magsOn||!ff3Native::admitted()||!allowed(s,now())||!ff3Native::focused(s.pid)||WaitForSingleObject(h,0)!=WAIT_TIMEOUT||rd<addr>(h,s.player+Offsets::Player::ModelInstance)!=s.own)return false;
   if(rd<float>(h,child(h,s.own,"Humanoid")+Offsets::Humanoid::Health,0)<=0||ff3Native::Reader::possessedBall(h,s.own))return false;
   if(!seed.ball||!seed.id||!seed.primitive||!seed.mesh||rd<addr>(h,seed.id+Offsets::Instance::Parent)!=seed.rig||rd<addr>(h,seed.id+Offsets::BasePart::Primitive)!=seed.primitive||rd<addr>(h,seed.mesh+Offsets::Instance::Parent)!=seed.id||!instanceClassIs(h,seed.id,"Part")||!path(h,seed.id,{"Configuration","Gravity"})||!ff3Native::looseWorldBall(h,seed.id,ws))return false;
   const auto hand=contact(h,s);const auto position=snapWorldPosition(h,seed.primitive),size=ff3Native::partSize(h,seed.id);
   if(!hand.valid(s)||!ff3qb::finite(position)||!withinSnapDistance(position,hand.position,range)||!ff3qb::finite(size)||std::min({size.x,size.y,size.z})<=.1||std::max({size.x,size.y,size.z})>maxMagSize||ff3qb::length(command.destination-hand.position)>.75)return false;
   return true;
  };
  const auto result=writeSnapFields(h,seed.primitive,command.destination,command.velocity,guard,range);readbackNear=result.readbackNear;renderedPosition=snapRenderedPosition(h,seed);renderedNear=ff3qb::finite(renderedPosition)&&ff3qb::length(renderedPosition-command.destination)<.75;return result.written;
 }
};
}
