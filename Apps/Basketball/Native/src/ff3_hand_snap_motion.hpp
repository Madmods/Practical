#pragma once
// Included inside ff3Play after snapPoseLayoutAdmitted/snapRotationValid.
// This optional layout was checked in one admitted client build only. It is
// motion data, not an ownership handle. Never write its pointer or metadata.
struct SnapMotionFrame {float rotation[9];shooting::Vec position,linear,angular;};
struct SnapMotionFrames {addr pointer=0;SnapMotionFrame a{},b{};};
static_assert(sizeof(SnapMotionFrame)==72&&offsetof(SnapMotionFrame,position)==36,"Motion frame layout changed");
inline bool snapMotionFrameValid(const SnapMotionFrame& frame){
 return snapRotationValid(frame.rotation)&&ff3qb::finite(ff3Native::vector(frame.position))&&ff3qb::finite(ff3Native::vector(frame.linear))&&ff3qb::finite(ff3Native::vector(frame.angular))&&ff3qb::length(ff3Native::vector(frame.linear))<=250&&ff3qb::length(ff3Native::vector(frame.angular))<=1000;
}
inline bool readSnapMotionFrames(HANDLE h,addr primitive,SnapMotionFrames& out){
 out={};if(!h||!primitive||!snapPoseLayoutAdmitted())return false;out.pointer=rd<addr>(h,primitive+0x1e8);
 if(out.pointer<65536||out.pointer>0x7fffffffffffULL||out.pointer%8)return false;SIZE_T bytes=0;
 if(!ReadProcessMemory(h,reinterpret_cast<void*>(out.pointer+0x8c),&out.a,sizeof out.a,&bytes)||bytes!=sizeof out.a||!ReadProcessMemory(h,reinterpret_cast<void*>(out.pointer+0x11c),&out.b,sizeof out.b,&bytes)||bytes!=sizeof out.b)return false;
 return rd<addr>(h,primitive+0x1e8)==out.pointer&&snapMotionFrameValid(out.a)&&snapMotionFrameValid(out.b)&&ff3qb::length(ff3Native::vector(out.a.position)-ff3Native::vector(out.b.position))<=100;
}
template<class Guard> inline bool writeSnapMotionFrames(HANDLE h,addr primitive,const SnapMotionFrames& before,Vec position,Vec velocity,Guard guard,double fullSize=maxMagSize){
 if(!h||!primitive||!snapPoseLayoutAdmitted()||!before.pointer||!snapMotionFrameValid(before.a)||!snapMotionFrameValid(before.b)||!ff3qb::finite(position)||!ff3qb::finite(velocity)||ff3qb::length(velocity)>180||!withinSnapDistance(ff3Native::vector(before.b.position),position,fullSize))return false;
 struct Motion{shooting::Vec position,linear;};const Motion desired{{float(position.x),float(position.y),float(position.z)},{float(velocity.x),float(velocity.y),float(velocity.z)}};
 const Motion originals[2]={{before.a.position,before.a.linear},{before.b.position,before.b.linear}};const addr fields[2]={before.pointer+0xb0,before.pointer+0x140};
 auto valid=[&](){return guard()&&rd<addr>(h,primitive+0x1e8)==before.pointer;};
 auto restore=[&](size_t index,size_t count){if(!count||count>sizeof desired||!valid())return;Motion actual{};SIZE_T bytes=0;if(ReadProcessMemory(h,reinterpret_cast<void*>(fields[index]),&actual,count,&bytes)&&bytes==count&&!memcmp(&actual,&desired,count))WriteProcessMemory(h,reinterpret_cast<void*>(fields[index]),&originals[index],count,&bytes);};
 size_t applied=0;for(;applied<2;++applied){if(!valid())break;SIZE_T bytes=0;if(!WriteProcessMemory(h,reinterpret_cast<void*>(fields[applied]),&desired,sizeof desired,&bytes)||bytes!=sizeof desired){restore(applied,bytes);break;}}
 if(applied!=2){while(applied)restore(--applied,sizeof desired);return false;}
 return valid();
}
