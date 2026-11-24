#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <cstring>
#include <cstdint>
#include "ff3_direct_aim.hpp"
#include <functional>
namespace ff3qb {
struct AimCameraTransform {AimRotation rotation;float x=0,y=0,z=0;};
struct AimViewSnapshot {AimCameraTransform camera;Vec mouse;bool valid=false;};
// A bounded user-initiated operation supplies the identity check. This helper
// never searches memory, changes page protection, installs a hook or executes
// code in the target. The explicit camera transform and cursor spans are
// the only writable data; restoring also restores the original camera eye.
class DirectAimMemory {
 HANDLE handle_=nullptr;
 static bool writable(HANDLE h,uintptr_t address,size_t size){
  if(address<0x10000||!size||address+size<address)return false;MEMORY_BASIC_INFORMATION region{};
  if(VirtualQueryEx(h,reinterpret_cast<void*>(address),&region,sizeof region)!=sizeof region||region.State!=MEM_COMMIT||region.Protect&(PAGE_GUARD|PAGE_NOACCESS))return false;
  const auto protection=region.Protect&255;return (protection==PAGE_READWRITE||protection==PAGE_WRITECOPY||protection==PAGE_EXECUTE_READWRITE||protection==PAGE_EXECUTE_WRITECOPY)&&address+size<=reinterpret_cast<uintptr_t>(region.BaseAddress)+region.RegionSize;
 }
 template<class T>bool read(uintptr_t address,T& value){SIZE_T count=0;return ReadProcessMemory(handle_,reinterpret_cast<void*>(address),&value,sizeof value,&count)&&count==sizeof value;}
 template<class T>bool write(uintptr_t address,const T& value){SIZE_T count=0;return WriteProcessMemory(handle_,reinterpret_cast<void*>(address),&value,sizeof value,&count)&&count==sizeof value;}
public:
 explicit DirectAimMemory(DWORD pid){if(pid)handle_=OpenProcess(PROCESS_QUERY_INFORMATION|PROCESS_VM_READ|PROCESS_VM_WRITE|PROCESS_VM_OPERATION|SYNCHRONIZE,FALSE,pid);}
 ~DirectAimMemory(){if(handle_)CloseHandle(handle_);}
 HANDLE handle()const{return handle_;}
 bool captureView(uintptr_t cameraAddress,uintptr_t mouseAddress,AimViewSnapshot& snapshot){
  static_assert(sizeof(AimCameraTransform)==48);
  snapshot={};if(cameraAddress<mouseAddress+12&&mouseAddress<cameraAddress+48)return false;
  if(!handle_||!read(cameraAddress,snapshot.camera)||!aimRotationValid(snapshot.camera.rotation)||!finite(Vec{snapshot.camera.x,snapshot.camera.y,snapshot.camera.z})||!cursor(mouseAddress,snapshot.mouse))return false;
  return snapshot.valid=true;
 }
 bool restoreView(uintptr_t cameraAddress,uintptr_t mouseAddress,const AimViewSnapshot& snapshot,const std::function<bool()>& cameraIdentity,const std::function<bool()>& mouseIdentity){
  if(!handle_||!snapshot.valid||!aimRotationValid(snapshot.camera.rotation)||!finite(Vec{snapshot.camera.x,snapshot.camera.y,snapshot.camera.z})||!cameraIdentity||!cameraIdentity()||!writable(handle_,cameraAddress,sizeof(AimCameraTransform)))return false;
  // A click may replace the primary InputObject. That must not strand the
  // camera at the throw angle; restore the cursor only while its object lives.
  if(!cameraIdentity()||!write(cameraAddress,snapshot.camera))return false;
  if(mouseIdentity&&mouseIdentity())writeCursor(mouseAddress,snapshot.mouse,mouseIdentity);
  return true;
 }
 bool cursor(uintptr_t address,Vec& value){struct Point{float x,y,z;};Point p{};if(!handle_||!read(address,p))return false;value={p.x,p.y,p.z};return finite(value);}
 bool writeCursor(uintptr_t address,Vec value,const std::function<bool()>& identity){
  struct Point{float x,y,z;};if(!handle_||WaitForSingleObject(handle_,0)!=WAIT_TIMEOUT||!finite(value)||std::abs(value.x)>100000||std::abs(value.y)>100000||std::abs(value.z)>100||!identity||!identity()||!writable(handle_,address,sizeof(Point)))return false;
  const Point p{float(value.x),float(value.y),float(value.z)};return identity()&&write(address,p);
 }
 bool restoreCursor(uintptr_t address,Vec expected,Vec original,const std::function<bool()>& identity){
  struct Point{float x,y,z;};if(!identity||!identity())return false;Point current{},owned{float(expected.x),float(expected.y),float(expected.z)};
  if(!read(address,current)||memcmp(&current,&owned,sizeof current))return false;return writeCursor(address,original,identity);
 }
 bool apply(uintptr_t rotationAddress,uintptr_t mouseAddress,const DirectAimPose& pose,const std::function<bool()>& identity){
  struct Point {float x,y,z;};static_assert(sizeof(AimRotation)==36&&sizeof(Point)==12);
  if(!handle_||WaitForSingleObject(handle_,0)!=WAIT_TIMEOUT||!pose.valid||!aimRotationValid(pose.rotation)||!finite(pose.mouse)||!identity||!identity())return false;
  if(rotationAddress<mouseAddress+sizeof(Point)&&mouseAddress<rotationAddress+sizeof(AimRotation))return false;
  if(!writable(handle_,rotationAddress,sizeof(AimRotation))||!writable(handle_,mouseAddress,sizeof(Point)))return false;
  AimRotation previous;Point oldMouse{};
  if(!read(rotationAddress,previous)||!aimRotationValid(previous)||!read(mouseAddress,oldMouse)||!std::isfinite(oldMouse.x)||!std::isfinite(oldMouse.y)||!std::isfinite(oldMouse.z)||!identity())return false;
  if(!write(rotationAddress,pose.rotation))return false;
  const Point point{float(pose.mouse.x),float(pose.mouse.y),float(pose.mouse.z)};
  if(!identity()||!write(mouseAddress,point)){
   // Restore a partial rotation write only while it is still ours and the
   // same objects remain current. Never overwrite a replacement camera.
   AimRotation actual;if(identity()&&read(rotationAddress,actual)&&memcmp(&actual,&pose.rotation,sizeof actual)==0)write(rotationAddress,previous);return false;
  }
  return true; // Render/ray acknowledgement is a separate caller requirement.
 }
 DirectAimMemory(const DirectAimMemory&)=delete;DirectAimMemory& operator=(const DirectAimMemory&)=delete;
};
}
