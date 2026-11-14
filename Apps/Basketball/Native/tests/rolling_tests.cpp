#define NOMINMAX
#include "engine.hpp"
#include "pump_overlay.hpp"
#include <cassert>
#include <cstdio>
#include <fstream>
int main(){
 using namespace rolling;
 MotionSpeed measured;float observed=0;for(int i=0;i<100;i++)observed=measured.observe({(i/4)*.064f*24,0,0},i*.016,0);assert(fabs(observed-24)<.05);assert(measured.observe({40,0,0},2,20)==20);
 auto intercept=earliest({{10,0,0},{4,0,0},0},{{0,0,0},16});assert(intercept.valid);assert(fabs(intercept.time-.75)<.001);assert(fabs(intercept.point.x-13)<.01);
 auto angle=earliest({{0,0,0},{8,0,0},0},{{0,0,10},16});assert(angle.valid&&angle.point.x>4);assert(flatLength(angle.point-shooting::Vec{0,0,10})<=16*angle.time+1.001);
 assert(!earliest({{10,0,0},{40,0,0},0},{{0,0,0},16}).valid);
 auto stopped=at({{0,0,0},{10,0,0},5},4);assert(fabs(stopped.x-10)<.001);
 auto slow=earliest({{20,0,0},{10,0,0},5},{{0,0,0},16});assert(slow.valid);
 assert(!earliest({{0,0,0},{10,0,0},0},{{NAN,0,0},16}).valid);
 // An airborne ball above the runner cannot be caught at t=0.
 Ball bouncing{{0,8,0},{6,-5,0},0,45,0,.75f,.55f};auto airborne=earliest(bouncing,{{0,0,0},16,4.5f});assert(airborne.valid&&airborne.time>.2f&&airborne.point.y<=4.501f);
 float impact=(-5+sqrtf(25+90*7.25f))/45;auto after=at(bouncing,impact+.12f);assert(after.y>.75f);for(int i=0;i<200;i++)assert(at(bouncing,i*.03f).y>=.749f);
 auto high=earliest({{0,12,0},{0,0,0},0,45,0,.75f,.55f},{{0,0,0},16,4.5f});assert(high.valid&&high.time>.5f);
 BounceMotion bounce;bounce.update(1,{0,1.5f,0},45,0,.75f);bounce.update(1.04,{.1f,1.0f,0},45,0,.75f);assert(bounce.update(1.08,{.2f,1.2f,0},45,0,.75f));assert(bounce.valid&&bounce.v.y>0&&bounce.restitution>=.2f&&bounce.restitution<=.8f);
 puts("Bounce interception: airborne height gate, rebound, apex and floor continuity passed");
 auto p=make({{0,0,0},{8,0,0},1},{{0,0,10},16},{{{4,0,0},16}});assert(p.valid&&p.otherTime>=0&&p.otherTime<p.time);
 puts("Rolling intercept math: angle, earliest arrival, slowing/stopping, unreachable and rival race passed");
 SetProcessDPIAware();float m[16]={.06f,0,0,-.1f,0,.07f,-.04f,.1f,0,0,1,0,0,0,0,1};
 renderRollingPlan(p,m,1920,1080,{-10000,-10000},1);
 auto&s=rollingSurface;BITMAPFILEHEADER fh{};BITMAPINFOHEADER ih{};ih.biSize=sizeof ih;ih.biWidth=s.width;ih.biHeight=-s.height;ih.biPlanes=1;ih.biBitCount=32;fh.bfType=0x4d42;fh.bfOffBits=sizeof fh+sizeof ih;fh.bfSize=fh.bfOffBits+s.width*s.height*4;std::ofstream out("rolling-qa.bmp",std::ios::binary);out.write((char*)&fh,sizeof fh);out.write((char*)&ih,sizeof ih);out.write((char*)s.bits,fh.bfSize-fh.bfOffBits);out.close();
 // A beam drawn across the avatar must be clipped, including the glow stroke.
 unsigned pixels[64*64]{};{Gdiplus::Bitmap bitmap(64,64,256,PixelFormat32bppPARGB,(BYTE*)pixels);Gdiplus::Graphics g(&bitmap);courtDepth::masks.polygons={{{.25f,.25f},{.75f,.25f},{.75f,.75f},{.25f,.75f}}};courtDepth::masks.apply(g,64,64);Gdiplus::Pen glow(Gdiplus::Color(100,20,250,250),12),beam(Gdiplus::Color(255,20,250,250),3);g.DrawLine(&glow,0,32,63,32);g.DrawLine(&beam,0,32,63,32);}
 assert((pixels[32*64+32]>>24)==0);assert((pixels[32*64+8]>>24)>0);puts("Beam plus glow character occlusion passed");courtDepth::masks.polygons.clear();renderRollingPlan({},m,1920,1080,{-10000,-10000},1);assert(!rollingSurface.visible);closeFlightOverlay();closePumpOverlay();
}
