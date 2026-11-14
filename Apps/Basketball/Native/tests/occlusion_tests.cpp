#define NOMINMAX
#include "engine.hpp"
#include "pump_overlay.hpp"
#include <cassert>
#include <cstdio>
#include <cstdlib>
int main(int argc,char**argv){
 float matrix[16]={.1f,0,0,0,0,.1f,0,0,0,0,1,0,0,0,0,1};float identity[9]={1,0,0,0,1,0,0,0,1};auto p=courtDepth::projectBox(matrix,{0,0,0},{1,2,1},identity);assert(p.size()==4);for(auto q:p){assert(q.x>=.449f&&q.x<=.551f&&q.y>=.399f&&q.y<=.601f);}
 matrix[15]=0;assert(courtDepth::projectBox(matrix,{0,0,0},{1,2,1},identity).empty());
 ULONG_PTR token;Gdiplus::GdiplusStartupInput init;assert(Gdiplus::GdiplusStartup(&token,&init,nullptr)==Gdiplus::Ok);
 {
  unsigned pixels[64*64]{};Gdiplus::Bitmap bitmap(64,64,256,PixelFormat32bppPARGB,(BYTE*)pixels);
  courtDepth::masks.polygons={{{.25f,.25f},{.75f,.25f},{.75f,.75f},{.25f,.75f}}};
  {Gdiplus::Graphics g(&bitmap);courtDepth::masks.apply(g,64,64);Gdiplus::SolidBrush brush(Gdiplus::Color(255,255,255,255));g.FillRectangle(&brush,0,0,64,64);}
  assert((pixels[32*64+32]>>24)==0);assert((pixels[8*64+8]>>24)==255);
  courtDepth::masks.polygons.clear();
 }
 Gdiplus::GdiplusShutdown(token);puts("Character projection and transparent occlusion mask checks passed");
 // Optional read-only live sampling benchmark; no input hooks or game writes.
 if(argc<3)return 0;gamePid=(DWORD)strtoul(argv[1],nullptr,10);HANDLE h=OpenProcess(PROCESS_VM_READ,FALSE,gamePid);if(h){addr base=(addr)strtoull(argv[2],nullptr,10);auto root=rd<addr>(h,rd<addr>(h,base+Offsets::FakeDataModel::Pointer)+Offsets::FakeDataModel::RealDataModel);auto player=rd<addr>(h,child(h,root,"Players")+Offsets::Player::LocalPlayer);sceneRoot=root;sceneOwn=path(h,root,{"Workspace","Characters",nm(h,player)});sceneVisualEngine=rd<addr>(h,base+Offsets::VisualEngine::Pointer);
 if(sceneOwn){std::vector<double> times;for(int i=0;i<100;i++){double t=now();courtDepth::masks.update();times.push_back((now()-t)*1000);Sleep(4);}std::sort(times.begin(),times.end());printf("Live mask: %zu parts, %zu projected polygons, median %.3f ms, p95 %.3f ms\n",courtDepth::masks.parts.size(),courtDepth::masks.polygons.size(),times[50],times[95]);}
 courtDepth::masks.close();CloseHandle(h);}
}
