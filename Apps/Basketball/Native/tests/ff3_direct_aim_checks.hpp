#pragma once
template<class Check>void directAimChecks(Check check){
 using namespace ff3qb;
 auto rotation=[](Vec forward){forward=aimUnit(forward);const auto right=aimUnit(aimCross(forward,{0,1,0})),up=aimCross(right,forward),back=forward*-1;AimRotation r;const Vec columns[3]={right,up,back};for(int c=0;c<3;++c){r.m[c]=float(columns[c].x);r.m[c+3]=float(columns[c].y);r.m[c+6]=float(columns[c].z);}return r;};
 auto projection=[](const AimRotation& r,Vec eye,double fov,int width,int height){
  std::array<float,16> m{};const double f=1/std::tan(fov*3.141592653589793/360),aspect=double(width)/height,nearPlane=.1,farPlane=10000,a=-(farPlane+nearPlane)/(farPlane-nearPlane),b=-2*farPlane*nearPlane/(farPlane-nearPlane);
  const auto right=aimColumn(r,0),up=aimColumn(r,1),back=aimColumn(r,2);const Vec axes[4]={right*(f/aspect),up*f,back*a,back*-1};
  for(int row=0;row<4;++row){m[row*4]=float(axes[row].x);m[row*4+1]=float(axes[row].y);m[row*4+2]=float(axes[row].z);m[row*4+3]=float(-dot(axes[row],eye)+(row==2?b:0));}return m;
 };
 const auto old=rotation({.2,-.1,-1});const Vec eye{16,22,40},head{3,8,0};
 for(double fov:{55.,70.,100.})for(auto size:{std::pair<int,int>{1280,720},{1024,768}}){
  const auto matrix=projection(old,eye,fov,size.first,size.second);
  for(auto fraction:{std::pair<double,double>{.5,.5},{.2,.2},{.8,.2},{.2,.8},{.8,.8}})for(Vec launch:std::vector<Vec>{{50,10,-100},{0,75,-50},{-70,45,80},{10,90,1},{0,90,0}}){
   const Screen pixel{fraction.first*size.first,fraction.second*size.second,true};const auto pose=directAimPose(old,matrix.data(),eye,head,launch,pixel,size.first,size.second);
   check(pose.valid&&aimRotationValid(pose.rotation));const auto updated=projection(pose.rotation,eye,fov,size.first,size.second);const auto observed=projectAim(pose.endpoint,updated.data(),size.first,size.second);
   check(observed.valid&&std::hypot(observed.x-pixel.x,observed.y-pixel.y)<.05);
   check(length(aimUnit(pose.endpoint-head)-aimUnit(launch))<1e-8&&pose.mouse.x==pixel.x&&pose.mouse.y==pixel.y);
  }
 }
 const auto matrix=projection(old,eye,70,1280,720);const auto pose=directAimPose(old,matrix.data(),eye,head,{0,40,-80},{640,360,true},1280,720);
 // Renderer orientation/eye may still describe the previous frame. Camera
 // intrinsics still produce the right local pixel ray for the new pose.
 for(auto pixel:std::vector<Screen>{{640,360,true},{280,550,true},{1010,190,true}}){
  const auto newRotation=rotation({-.7,.4,1});const Vec newEye{-25,30,70};
  auto flick=directAimPose(newRotation,matrix.data(),newEye,head,{40,32,-65},pixel,1280,720);
  const auto rendered=projection(flick.rotation,newEye,70,1280,720);const auto at=projectAim(flick.endpoint,rendered.data(),1280,720);
  check(flick.valid&&at.valid&&std::hypot(at.x-pixel.x,at.y-pixel.y)<.05);
 }
 check(pose.valid);AimRotation invalid;check(!directAimPose(invalid,matrix.data(),eye,head,{0,40,-80},{640,360,true},1280,720).valid);
 auto singular=matrix;singular.fill(0);check(!directAimPose(old,singular.data(),eye,head,{0,40,-80},{640,360,true},1280,720).valid);
 for(Screen bad:std::vector<Screen>{{},{-10,200,true},{1400,300,true},{640,NAN,true}})check(!directAimPose(old,matrix.data(),eye,head,{0,40,-80},bad,1280,720).valid);
 check(!directAimPose(old,matrix.data(),eye,head,{0,40,-80},{640,360,true},1280,720,NAN).valid);
 // Exercise real process read/write APIs only against this test's own pages.
 SYSTEM_INFO info{};GetSystemInfo(&info);const auto bytes=size_t(info.dwPageSize)*2;
 auto* storage=static_cast<unsigned char*>(VirtualAlloc(nullptr,bytes,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(storage!=nullptr);memset(storage,0xA5,bytes);
 const auto camera=reinterpret_cast<uintptr_t>(storage+0xC0),mouse=reinterpret_cast<uintptr_t>(storage+info.dwPageSize+0xBC);
 struct Point{float x,y,z;};const Point originalPoint{90,40,0};memcpy(reinterpret_cast<void*>(camera),&old,sizeof old);memcpy(reinterpret_cast<void*>(mouse),&originalPoint,sizeof originalPoint);
 std::vector<unsigned char> before(storage,storage+bytes);DirectAimMemory memory(GetCurrentProcessId());check(memory.handle()!=nullptr);
 check(!memory.apply(camera,mouse,pose,[](){return false;}));check(memcmp(storage,before.data(),bytes)==0);
 check(memory.apply(camera,mouse,pose,[](){return true;}));
 check(memcmp(reinterpret_cast<void*>(camera),&pose.rotation,sizeof pose.rotation)==0);Point point{};memcpy(&point,reinterpret_cast<void*>(mouse),sizeof point);check(point.x==pose.mouse.x&&point.y==pose.mouse.y&&point.z==pose.mouse.z);
 bool outsideUnchanged=true;for(size_t i=0;i<bytes;++i){const auto address=reinterpret_cast<uintptr_t>(storage+i);if(address>=camera&&address<camera+36||address>=mouse&&address<mouse+12)continue;outsideUnchanged=outsideUnchanged&&storage[i]==before[i];}check(outsideUnchanged);
 memcpy(storage,before.data(),bytes);check(!memory.apply(camera,camera+8,pose,[](){return true;}));check(memcmp(storage,before.data(),bytes)==0);
 DWORD previous=0;check(VirtualProtect(storage+info.dwPageSize,info.dwPageSize,PAGE_READONLY,&previous)!=FALSE);check(!memory.apply(camera,mouse,pose,[](){return true;}));check(memcmp(storage,before.data(),bytes)==0);DWORD ignored=0;check(VirtualProtect(storage+info.dwPageSize,info.dwPageSize,previous,&ignored)!=FALSE);
 int identityChecks=0;check(!memory.apply(camera,mouse,pose,[&](){return ++identityChecks!=3;}));check(memcmp(storage,before.data(),bytes)==0); // Partial write was restored while still owned.
 // A click transaction restores the entire original camera transform, even
 // if the controller moved its eye while the temporary throw rotation ran.
 AimViewSnapshot saved;check(memory.captureView(camera,mouse,saved));
 check(memory.apply(camera,mouse,pose,[](){return true;}));
 const Point movedEye{80,30,12};memcpy(reinterpret_cast<void*>(camera+36),&movedEye,sizeof movedEye);
 check(memory.restoreView(camera,mouse,saved,[](){return true;},[](){return true;}));check(memcmp(storage,before.data(),bytes)==0);
 check(memory.apply(camera,mouse,pose,[](){return true;}));
 check(!memory.restoreView(camera,mouse,saved,[](){return false;},[](){return true;}));check(memcmp(reinterpret_cast<void*>(camera),&pose.rotation,sizeof pose.rotation)==0);
 // Replacing an input object prevents cursor restoration, independently of
 // camera restoration. Focus, FOV, type and neighbors stay outside the span.
 check(memory.writeCursor(mouse,{250,180,0},[](){return true;}));
 check(memory.restoreView(camera,mouse,saved,[](){return true;},[](){return false;}));
 check(memcmp(reinterpret_cast<void*>(camera),&saved.camera,sizeof saved.camera)==0);Point kept{};memcpy(&kept,reinterpret_cast<void*>(mouse),sizeof kept);check(kept.x==250&&kept.y==180);
 memcpy(storage,before.data(),bytes);AimViewSnapshot overlap;check(!memory.captureView(camera,camera+36,overlap));
 bool viewBounds=true;for(size_t i=0;i<bytes;++i){const auto address=reinterpret_cast<uintptr_t>(storage+i);if(address>=camera&&address<camera+48||address>=mouse&&address<mouse+12)continue;viewBounds=viewBounds&&storage[i]==before[i];}check(viewBounds);
 // The cursor-only path never writes the camera span or adjacent values.
 const Vec cursorTarget{220,-120,0};check(memory.writeCursor(mouse,cursorTarget,[](){return true;}));Vec readCursor;check(memory.cursor(mouse,readCursor)&&length(readCursor-cursorTarget)<1e-6);
 check(memcmp(reinterpret_cast<void*>(camera),&old,sizeof old)==0);
 outsideUnchanged=true;for(size_t i=0;i<bytes;++i){const auto address=reinterpret_cast<uintptr_t>(storage+i);if(address>=mouse&&address<mouse+12)continue;outsideUnchanged=outsideUnchanged&&storage[i]==before[i];}check(outsideUnchanged);
 check(!memory.restoreCursor(mouse,{1,2,0},{90,40,0},[](){return true;}));check(memory.cursor(mouse,readCursor)&&length(readCursor-cursorTarget)<1e-6);
 check(memory.restoreCursor(mouse,cursorTarget,{90,40,0},[](){return true;}));check(memcmp(storage,before.data(),bytes)==0);
 check(!memory.writeCursor(mouse,{100001,0,0},[](){return true;})&&!memory.writeCursor(mouse,cursorTarget,[](){return false;}));check(memcmp(storage,before.data(),bytes)==0);
 check(VirtualFree(storage,0,MEM_RELEASE)!=FALSE);
}
