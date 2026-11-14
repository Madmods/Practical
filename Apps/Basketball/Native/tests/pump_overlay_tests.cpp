#define NOMINMAX
#include "engine.hpp"
#include <cassert>
#include <cstdio>
#include <fstream>
#ifdef OLD_OVERLAY
static HWND testGame;
static HWND testForeground(){return testGame;}
#define gameFocused() true
#define GetForegroundWindow() testForeground()
#include "../../../../Archives/Previous-Organization/Backups/backup-overlay-smooth-20261110-161004/pump_overlay.hpp"
#undef gameFocused
#undef GetForegroundWindow
#else
#include "pump_overlay.hpp"
#endif
int main(){
 SetProcessDPIAware();
 PumpVisual v;v.valid=true;v.state=pump::Watching;
 for(int i=0;i<=48;i++){float a=i*6.2831853f/48;v.ownRing.push_back({.5f+.045f*cosf(a),.65f+.025f*sinf(a)});v.targetRing.push_back({.65f+.04f*cosf(a),.47f+.022f*sinf(a)});}v.ownPoint={.5f,.65f};v.targetPoint={.65f,.47f};
#ifndef OLD_OVERLAY
 v.targetVisible=true;v.targetMin={.613f,.24f};v.targetMax={.687f,.48f};v.gap=8;v.state=pump::Jumped;v.jumpConfirmed=true;
#endif
#ifdef OLD_OVERLAY
 testGame=CreateWindowExW(0,L"STATIC",L"Overlay benchmark",WS_POPUP,-10000,-10000,2560,1440,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
 pumpOn=true;pumpVisuals=true;
#else
 // Projection rejects invalid cameras/positions instead of drawing stale geometry.
 float m[16]={.1f,0,0,0,0,.1f,0,0,0,0,.1f,0,0,0,0,1};PumpVisual p;p.ownGround=0;p.targetGround=0;
 assert(projectPumpVisual(p,m,{0,2.85f,0},{}));assert(p.ownRing.size()==73);assert(std::abs(p.ownPoint.first-.5f)<.0001f);
 p.targetPrimitive=1;assert(projectPumpVisual(p,m,{0,2.85f,0},{3,2.85f,0}));assert(p.targetVisible&&p.targetMin.first<p.targetMax.first);
 auto beforeJump=p.targetRing;assert(projectPumpVisual(p,m,{0,2.85f,0},{3,7.85f,0}));assert(p.targetRing==beforeJump);assert(!p.targetAccents.empty());
 pump::GroundPlane floor;assert(std::abs(floor.update(.025f,0,0)-.025f)<.001f);
 for(int i=1;i<=8;i++)assert(std::abs(floor.update(.025f+i*.4f,20,i*.02)-.025f)<.001f);
 assert(std::abs(floor.update(3.5f,0,.18)-.025f)<.001f);assert(std::abs(floor.update(3.5f,0,.24)-.025f)<.001f);
 assert(std::abs(floor.update(1.5f,-20,.3)-.025f)<.001f);floor.update(.025f,0,.5);assert(std::abs(floor.update(.025f,0,.52)-.025f)<.001f);
 // Reacquire a stable changed floor, not the apex of a normal jump.
 floor.update(4,0,1);floor.update(4,0,1.1);assert(std::abs(floor.update(4,0,1.5)-4)<.001f);
 m[15]=0;assert(!projectPumpVisual(p,m,{0,2.85f,0},{}));m[15]=1;assert(!projectPumpVisual(p,m,{NAN,0,0},{}));
 // Angled synthetic camera exercises true world-plane accents in visual QA.
 float camera[16]={.065f,0,0,0,0,.12f,-.045f,.15f,0,0,1,0,0,0,0,1};
 v.ownGround=.025f;v.targetGround=.025f;v.targetPrimitive=1;
 assert(projectPumpVisual(v,camera,{-4,4.34f,3},{4,3.77f,-3}));
#endif
 std::vector<double> times;
#ifndef OLD_OVERLAY
 PumpFrameClock clock;double deadline=now(),lastFrame=0;std::vector<double> intervals;
#endif
 DWORD startGdi=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);
 for(int i=0;i<200;i++){
#ifndef OLD_OVERLAY
  while(now()<deadline){MSG msg{};while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}clock.waitUntil(deadline);}
  double frame=now();if(i>20)intervals.push_back((frame-lastFrame)*1000);lastFrame=frame;deadline=std::max(deadline+1./240,frame);
#endif
  double t=now();
#ifdef OLD_OVERLAY
  v.at=t;pumpVisual=v;drawPumpOverlay();
#else
  renderPumpVisual(v,2560,1440,{-10000,-10000},1);
#endif
  if(i>=20)times.push_back((now()-t)*1000);
 }
 std::sort(times.begin(),times.end());
 printf("render_ms median=%.3f p95=%.3f\n",times[times.size()/2],times[times.size()*95/100]);
#ifndef OLD_OVERLAY
 std::sort(intervals.begin(),intervals.end());printf("240Hz pacing median=%.3f ms p95=%.3f ms high_resolution=%d\n",intervals[intervals.size()/2],intervals[intervals.size()*95/100],clock.highResolution);
 printf("court_surface=%dx%d status_surface=%dx%d full_screen=2560x1440\n",pumpCourtSurface.width,pumpCourtSurface.height,pumpStatusSurface.width,pumpStatusSurface.height);
 // Save cropped ring pixels for visual QA, no game capture.
 BITMAPFILEHEADER fh{};BITMAPINFOHEADER ih{};ih.biSize=sizeof ih;ih.biWidth=pumpCourtSurface.width;ih.biHeight=-pumpCourtSurface.height;ih.biPlanes=1;ih.biBitCount=32;ih.biCompression=BI_RGB;fh.bfType=0x4D42;fh.bfOffBits=sizeof fh+sizeof ih;fh.bfSize=fh.bfOffBits+pumpCourtSurface.width*pumpCourtSurface.height*4;
 std::ofstream out("pump-overlay-qa.bmp",std::ios::binary);out.write((char*)&fh,sizeof fh);out.write((char*)&ih,sizeof ih);out.write((char*)pumpCourtSurface.bits,fh.bfSize-fh.bfOffBits);out.close();
 // Exercise resize/re-show and resource disposal.
 pumpCourtSurface.hide();pumpStatusSurface.hide();renderPumpVisual(v,1920,1080,{-10000,-10000},1.5f);renderPumpVisual(v,2560,1440,{-10000,-10000},1);
#endif
 closePumpOverlay();
#ifdef OLD_OVERLAY
 DestroyWindow(testGame);
#endif
 DWORD endGdi=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);printf("GDI objects before=%lu after=%lu\n",startGdi,endGdi);assert(endGdi<=startGdi+2);
 return 0;
}
