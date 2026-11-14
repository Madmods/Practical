#define NOMINMAX
#include "engine.hpp"
#include "pump_overlay.hpp"
#include <cassert>
#include <cstdio>
#include <fstream>
static void capture(){auto&s=flightSurface;BITMAPFILEHEADER f{};BITMAPINFOHEADER i{};i.biSize=sizeof i;i.biWidth=s.width;i.biHeight=-s.height;i.biPlanes=1;i.biBitCount=32;f.bfType=0x4d42;f.bfOffBits=sizeof f+sizeof i;f.bfSize=f.bfOffBits+s.width*s.height*4;std::ofstream out("ball-arc-qa.bmp",std::ios::binary);out.write((char*)&f,sizeof f);out.write((char*)&i,sizeof i);out.write((char*)s.bits,f.bfSize-f.bfOffBits);}
int main(){
 SetProcessDPIAware();float m[16]={.055f,0,0,0,0,.06f,-.04f,-.25f,0,0,1,0,0,0,0,1};
 auto arc=flight::predict({true,{-6,6,4},{12,19,-5},45,0,0},0,.75f);assert(arc.valid);
 DWORD before=GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS);std::vector<double>times;
 for(int i=0;i<100;i++){double t=now();renderFlightArc(arc,m,2560,1440,{-10000,-10000},1);if(i>10)times.push_back((now()-t)*1000);}
 capture();std::sort(times.begin(),times.end());printf("Ball arc renderer median %.3f ms p95 %.3f ms\n",times[times.size()/2],times[times.size()*95/100]);
 ballLandingOn=false;renderFlightArc(arc,m,1920,1080,{-10000,-10000},1.5f);ballLandingOn=true;renderFlightArc({},m,1920,1080,{-10000,-10000},1);assert(!flightSurface.visible);closeFlightOverlay();closePumpOverlay();assert(GetGuiResources(GetCurrentProcess(),GR_GDIOBJECTS)<=before+2);puts("Flight overlay resize/toggle/cleanup passed");
}
