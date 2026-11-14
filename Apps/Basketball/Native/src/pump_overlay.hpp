#pragma once
#include <gdiplus.h>
#pragma comment(lib,"gdiplus.lib")
#include "court_occlusion.hpp"
inline const wchar_t* pumpLabel(int state){static const wchar_t*s[]={L"Off",L"Hold a ball",L"Tap shot key to bait",L"Watching defender",L"Jump committed",L"Stand still",L"Another defender is covering",L"Opening",L"Shot requested",L"Window ended",L"Enable Auto time",L"Waiting for character"};return s[std::clamp(state,0,11)];}
static LRESULT CALLBACK pumpOverlayProc(HWND h,UINT msg,WPARAM w,LPARAM l){if(msg==WM_NCHITTEST)return HTTRANSPARENT;if(msg==WM_MOUSEACTIVATE)return MA_NOACTIVATE;return DefWindowProcW(h,msg,w,l);}
// Small reusable surfaces: never clear/upload an entire monitor for two rings.
struct PumpSurface {
 HWND window=nullptr;HDC dc=nullptr;HBITMAP bitmap=nullptr;HGDIOBJ old=nullptr;void*bits=nullptr;int width=0,height=0;bool visible=false;
 void hide(){if(visible){ShowWindow(window,SW_HIDE);visible=false;}}
 void freeBitmap(){if(dc){if(old)SelectObject(dc,old);if(bitmap)DeleteObject(bitmap);DeleteDC(dc);}dc=nullptr;bitmap=nullptr;old=nullptr;bits=nullptr;width=height=0;}
 void close(){if(window)DestroyWindow(window);window=nullptr;visible=false;freeBitmap();}
 bool prepare(int w,int h){
  if(!window){WNDCLASSW wc{};wc.lpfnWndProc=pumpOverlayProc;wc.hInstance=GetModuleHandleW(nullptr);wc.lpszClassName=L"VantaPumpVisual";RegisterClassW(&wc);window=CreateWindowExW(WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW|WS_EX_TOPMOST,wc.lpszClassName,L"",WS_POPUP,0,0,1,1,nullptr,nullptr,wc.hInstance,nullptr);if(!window)return false;}
  w=(w+63)&~63;h=(h+63)&~63;
  // Keep capacity while moving; grow in blocks, shrink only on a large change.
  if(dc&&w<=width&&h<=height&&w*2>=width&&h*2>=height)return true;
  freeBitmap();BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=w;bi.bmiHeader.biHeight=-h;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;dc=CreateCompatibleDC(nullptr);bitmap=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&bits,nullptr,0);if(!dc||!bitmap){freeBitmap();return false;}old=SelectObject(dc,bitmap);width=w;height=h;return true;
 }
 void present(POINT origin){POINT src{};SIZE size{width,height};BLENDFUNCTION blend{AC_SRC_OVER,0,255,AC_SRC_ALPHA};if(UpdateLayeredWindow(window,nullptr,&origin,&size,dc,&src,0,&blend,ULW_ALPHA)&&!visible){ShowWindow(window,SW_SHOWNOACTIVATE);visible=true;}}
};
static PumpSurface pumpCourtSurface,pumpStatusSurface;
static ULONG_PTR pumpGdi=0;static HANDLE pumpRead=nullptr;static DWORD pumpReadPid=0;
static void closePumpOverlay(){pumpCourtSurface.close();pumpStatusSurface.close();if(pumpRead)CloseHandle(pumpRead);pumpRead=nullptr;pumpReadPid=0;if(pumpGdi)Gdiplus::GdiplusShutdown(pumpGdi);pumpGdi=0;}
static bool projectPumpVisual(PumpVisual&v,const float*matrix,shooting::Vec own,shooting::Vec target){
 auto project=[&](shooting::Vec p,std::pair<float,float>&out){float w=matrix[12]*p.x+matrix[13]*p.y+matrix[14]*p.z+matrix[15];if(!std::isfinite(w)||w<.01f)return false;float x=(matrix[0]*p.x+matrix[1]*p.y+matrix[2]*p.z+matrix[3])/w,y=(matrix[4]*p.x+matrix[5]*p.y+matrix[6]*p.z+matrix[7])/w;if(!std::isfinite(x)||!std::isfinite(y)||std::abs(x)>1.5||std::abs(y)>1.5)return false;out={(x+1)*.5f,(1-y)*.5f};return true;};
 auto circle=[&](shooting::Vec p,float radius,std::vector<std::pair<float,float>>&pts){pts.clear();pts.reserve(73);for(int i=0;i<=72;i++){float a=i*6.2831853f/72;std::pair<float,float>q;if(!project(p+shooting::Vec{radius*cosf(a),0,radius*sinf(a)},q)){pts.clear();return;}pts.push_back(q);}};
 // A camera-projected character envelope, not a server hitbox or mesh edit.
 v.targetVisible=false;
 if(v.targetPrimitive){v.targetMin={2.f,2.f};v.targetMax={-2.f,-2.f};bool valid=true;
  for(float x:{-1.55f,1.55f})for(float y:{-2.85f,3.f})for(float z:{-1.f,1.f}){std::pair<float,float>q;if(!project(target+shooting::Vec{x,y,z},q)){valid=false;continue;}v.targetMin.first=std::min(v.targetMin.first,q.first);v.targetMin.second=std::min(v.targetMin.second,q.second);v.targetMax.first=std::max(v.targetMax.first,q.first);v.targetMax.second=std::max(v.targetMax.second,q.second);}
  v.targetVisible=valid&&v.targetMax.first>v.targetMin.first&&v.targetMax.second>v.targetMin.second;
 }
 // All decoration, including the link, is constructed in the world ground plane.
 own.y=v.ownGround;target.y=v.targetGround;
 v.ownRing.clear();v.targetRing.clear();v.ownInner.clear();v.targetInner.clear();v.ownAccents.clear();v.targetAccents.clear();v.groundLink.clear();
 auto line=[&](const std::vector<shooting::Vec>&world,std::vector<std::pair<float,float>>&out){for(auto p:world){std::pair<float,float>q;if(!project(p,q)){out.clear();return;}out.push_back(q);}};
 auto marker=[&](shooting::Vec center,float radius,shooting::Vec toward,auto&outer,auto&inner,auto&accents){
  circle(center,radius,outer);circle(center,radius-.13f,inner);
  // Four quiet arc segments and four radial registration marks.
  for(int k=0;k<4;k++){float start=k*1.5707963f+.18f;std::vector<shooting::Vec> arc;
   for(int j=0;j<=9;j++){float a=start+j*.075f;arc.push_back(center+shooting::Vec{(radius+.06f)*cosf(a),0,(radius+.06f)*sinf(a)});}accents.emplace_back();line(arc,accents.back());
   float a=k*1.5707963f;accents.emplace_back();line({center+shooting::Vec{(radius+.14f)*cosf(a),0,(radius+.14f)*sinf(a)},center+shooting::Vec{(radius+.31f)*cosf(a),0,(radius+.31f)*sinf(a)}},accents.back());
  }
  shooting::Vec d=toward-center;d.y=0;float length=shooting::length(d);
  if(length>.1f){d=d*(1/length);shooting::Vec side{-d.z,0,d.x};auto tip=center+d*(radius-.27f),base=center+d*(radius-.62f);accents.emplace_back();line({base+side*.23f,tip,base-side*.23f},accents.back());}
 };
 marker(own,1.9f,v.targetPrimitive?target:own,v.ownRing,v.ownInner,v.ownAccents);project(own,v.ownPoint);
 if(v.targetPrimitive&&std::isfinite(target.y)){
  marker(target,2.05f,own,v.targetRing,v.targetInner,v.targetAccents);project(target,v.targetPoint);
  auto d=target-own;float span=sqrtf(d.x*d.x+d.z*d.z);
  if(span>4.6f){shooting::Vec direction{d.x/span,0,d.z/span},side{-direction.z,0,direction.x};auto a=own+direction*2.25f,b=target-direction*2.4f;std::vector<shooting::Vec> link;
   for(int i=0;i<=24;i++){float t=i/24.f;link.push_back(a*(1-t)+b*t+side*(sinf(t*3.1415926f)*.35f));}line(link,v.groundLink);}
 }
 return !v.ownRing.empty();
}
static bool samplePumpVisual(PumpVisual&v){
 DWORD pid=gamePid.load();if(pid!=pumpReadPid){if(pumpRead)CloseHandle(pumpRead);pumpRead=pid?OpenProcess(PROCESS_VM_READ,FALSE,pid):nullptr;pumpReadPid=pid;}if(!pumpRead)return false;
 float matrix[16];SIZE_T count=0;addr camera=sceneVisualEngine.load();if(!camera||!ReadProcessMemory(pumpRead,(void*)(camera+Offsets::VisualEngine::ViewMatrix),matrix,sizeof matrix,&count)||count!=sizeof matrix)return false;
 auto own=rd<shooting::Vec>(pumpRead,v.ownPrimitive+Offsets::Primitive::Position,{NAN,NAN,NAN});auto target=v.targetPrimitive?rd<shooting::Vec>(pumpRead,v.targetPrimitive+Offsets::Primitive::Position,{NAN,NAN,NAN}):shooting::Vec{};
 return projectPumpVisual(v,matrix,own,target);
}
// Also used by the isolated renderer benchmark; no input or game access here.
static void renderPumpVisual(const PumpVisual&v,int w,int h,POINT origin,float dpi){
 using namespace Gdiplus;
 if(!pumpGdi){GdiplusStartupInput in;if(GdiplusStartup(&pumpGdi,&in,nullptr)!=Ok)return;}
 bool open=v.state==pump::Opening||v.shotSent,bait=v.jumpConfirmed||v.state==pump::Jumped;Color c=open?Color(255,101,232,178):bait?Color(255,255,192,96):Color(255,147,168,255);
 auto point=[&](std::pair<float,float>p){return PointF(p.first*w,p.second*h);};
 bool beam=!v.groundLink.empty()&&v.state>=pump::Watching&&v.state<=pump::Shooting;
 float left=(float)w,top=(float)h,right=0,bottom=0;auto bounds=[&](const auto&pts){for(auto p:pts){auto q=point(p);left=std::min(left,q.X);top=std::min(top,q.Y);right=std::max(right,q.X);bottom=std::max(bottom,q.Y);}};bounds(v.ownRing);bounds(v.targetRing);for(auto&a:v.ownAccents)bounds(a);for(auto&a:v.targetAccents)bounds(a);if(beam)bounds(v.groundLink);
 RectF targetBox{},tagBox{};
 if(v.targetVisible){auto lo=point(v.targetMin),hi=point(v.targetMax);float pad=5*dpi;
  targetBox={lo.X-pad,lo.Y-pad,hi.X-lo.X+2*pad,hi.Y-lo.Y+2*pad};
  float tw=184*dpi;tagBox={std::clamp((lo.X+hi.X-tw)*.5f,2.f,std::max(2.f,w-tw-2)),std::max(2.f,targetBox.Y-33*dpi),tw,26*dpi};
  left=std::min(left,std::min(targetBox.X,tagBox.X));right=std::max(right,std::max(targetBox.GetRight(),tagBox.GetRight()));top=std::min(top,std::min(targetBox.Y,tagBox.Y));bottom=std::max(bottom,targetBox.GetBottom());
 }
 int x0=std::clamp((int)floor(left)-10,0,w),y0=std::clamp((int)floor(top)-(beam?30:10),0,h),x1=std::clamp((int)ceil(right)+10,0,w),y1=std::clamp((int)ceil(bottom)+10,0,h);
 if(x1>x0&&y1>y0&&pumpCourtSurface.prepare(x1-x0,y1-y0)){
  auto&s=pumpCourtSurface;Bitmap canvas(s.width,s.height,s.width*4,PixelFormat32bppPARGB,(BYTE*)s.bits);
  {Graphics g(&canvas);g.Clear(Color(0,0,0,0));g.SetSmoothingMode(SmoothingModeAntiAlias);g.TranslateTransform((REAL)-x0,(REAL)-y0);g.SetClip(Rect(0,0,w,h));courtDepth::masks.apply(g,w,h);
   auto stroke=[&](const auto&raw,Color color,float width){if(raw.size()<2)return;std::vector<PointF>pts;pts.reserve(raw.size());for(auto p:raw)pts.push_back(point(p));Pen pen(color,width*dpi);pen.SetLineJoin(LineJoinRound);pen.SetStartCap(LineCapRound);pen.SetEndCap(LineCapRound);g.DrawLines(&pen,pts.data(),(INT)pts.size());};
   auto ring=[&](const auto&outer,const auto&inner,const auto&accents,Color color,bool target){
    if(outer.size()<3)return;
    std::vector<PointF>a,b;for(auto p:outer)a.push_back(point(p));for(auto p:inner)b.push_back(point(p));
    GraphicsPath band(FillModeAlternate);band.AddPolygon(a.data(),(INT)a.size());if(b.size()>2)band.AddPolygon(b.data(),(INT)b.size());SolidBrush tint(Color(target?95:70,color.GetR(),color.GetG(),color.GetB()));g.FillPath(&tint,&band);
    stroke(outer,Color(220,5,9,18),5.f);stroke(outer,Color(target?255:225,color.GetR(),color.GetG(),color.GetB()),2.3f);stroke(inner,Color(160,color.GetR(),color.GetG(),color.GetB()),1.1f);
    for(auto&accent:accents){stroke(accent,Color(65,color.GetR(),color.GetG(),color.GetB()),4.f);stroke(accent,Color(255,color.GetR(),color.GetG(),color.GetB()),2.3f);}
   };
   ring(v.ownRing,v.ownInner,v.ownAccents,Color(255,130,157,255),false);ring(v.targetRing,v.targetInner,v.targetAccents,c,true);
   if(v.targetVisible){
    float x=targetBox.X,y=targetBox.Y,bw=targetBox.Width,bh=targetBox.Height,r=std::min(9*dpi,bw*.12f);
    GraphicsPath body;body.AddArc(x,y,2*r,2*r,180,90);body.AddArc(x+bw-2*r,y,2*r,2*r,270,90);body.AddArc(x+bw-2*r,y+bh-2*r,2*r,2*r,0,90);body.AddArc(x,y+bh-2*r,2*r,2*r,90,90);body.CloseFigure();
    LinearGradientBrush tint(PointF(x,y),PointF(x,y+bh),Color(open?27:18,c.GetR(),c.GetG(),c.GetB()),Color(3,c.GetR(),c.GetG(),c.GetB()));g.FillPath(&tint,&body);
    Pen edge(Color(140,c.GetR(),c.GetG(),c.GetB()),1);g.DrawPath(&edge,&body);
    float len=std::min(19*dpi,std::min(bw,bh)*.23f);int alpha=bait?int(215+30*sin(now()*5)):235;
    for(int layer=0;layer<2;layer++){Pen corners(Color(layer?alpha:25,c.GetR(),c.GetG(),c.GetB()),(layer?2.5f:7.f)*dpi);corners.SetStartCap(LineCapRound);corners.SetEndCap(LineCapRound);corners.SetLineJoin(LineJoinRound);
     for(int ix=0;ix<2;ix++)for(int iy=0;iy<2;iy++){float cx=x+ix*bw,cy=y+iy*bh,sx=ix?-1.f:1.f,sy=iy?-1.f:1.f;PointF pts[3]={{cx,cy+sy*len},{cx,cy},{cx+sx*len,cy}};g.DrawLines(&corners,pts,3);}}
    SolidBrush tag(Color(232,17,23,37));g.FillRectangle(&tag,tagBox);Pen underline(Color(200,c.GetR(),c.GetG(),c.GetB()),1.5f*dpi);g.DrawLine(&underline,tagBox.X,tagBox.GetBottom(),tagBox.GetRight(),tagBox.GetBottom());
    const wchar_t*title=v.shotSent?L"SHOT SENT":v.state==pump::Opening?L"OPENING":v.state==pump::Moving?L"JUMPED / KEEP STILL":v.state==pump::Covered?L"JUMPED / COVERED":bait?L"JUMP DETECTED":v.state==pump::Shooting?L"SHOT REQUESTED":v.state==pump::Watching?L"WATCHING FAKE":L"TARGET";
    g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);Font font(L"Segoe UI",10*dpi,FontStyleBold,UnitPixel);SolidBrush text(c);StringFormat fmt;fmt.SetLineAlignment(StringAlignmentCenter);RectF label(tagBox.X+10*dpi,tagBox.Y,tagBox.Width-49*dpi,tagBox.Height);g.DrawString(title,-1,&font,label,&fmt,&text);
    wchar_t distance[24];swprintf_s(distance,L"%.0f st",v.gap);Font distanceFont(L"Segoe UI",10*dpi,FontStyleRegular,UnitPixel);SolidBrush muted(Color(240,183,195,219));fmt.SetAlignment(StringAlignmentFar);RectF value(tagBox.GetRight()-43*dpi,tagBox.Y,34*dpi,tagBox.Height);g.DrawString(distance,-1,&distanceFont,value,&fmt,&muted);
   }
   if(beam){std::vector<PointF>pts;for(auto q:v.groundLink)pts.push_back(point(q));Pen pen(Color(235,c.GetR(),c.GetG(),c.GetB()),2.f*dpi);pen.SetDashStyle(DashStyleDash);pen.SetDashOffset((REAL)(-now()*3));g.DrawLines(&pen,pts.data(),(INT)pts.size());}
  }s.present({origin.x+x0,origin.y+y0});
 }else pumpCourtSurface.hide();
 // Status has its own small surface and only repaints when its contents change.
 float boxW=300*dpi,boxH=38*dpi;static int lastState=-1;static float lastDpi=0;bool dirty=!pumpStatusSurface.visible||lastState!=v.state||lastDpi!=dpi;
 if(pumpStatusSurface.prepare((int)ceil(boxW+2),(int)ceil(boxH+2))){auto&s=pumpStatusSurface;if(dirty){Bitmap canvas(s.width,s.height,s.width*4,PixelFormat32bppPARGB,(BYTE*)s.bits);{Graphics g(&canvas);g.Clear(Color(0,0,0,0));g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);float x=1,y=1,radius=12*dpi;GraphicsPath box;box.AddArc(x,y,radius*2,radius*2,180,90);box.AddArc(x+boxW-radius*2,y,radius*2,radius*2,270,90);box.AddArc(x+boxW-radius*2,y+boxH-radius*2,radius*2,radius*2,0,90);box.AddArc(x,y+boxH-radius*2,radius*2,radius*2,90,90);box.CloseFigure();SolidBrush bg(Color(215,17,23,37));g.FillPath(&bg,&box);Pen border(Color(120,c.GetR(),c.GetG(),c.GetB()),1);g.DrawPath(&border,&box);SolidBrush accent(c);g.FillEllipse(&accent,x+14*dpi,y+15*dpi,8*dpi,8*dpi);Font font(L"Segoe UI",13*dpi,FontStyleRegular,UnitPixel);SolidBrush text(Color(255,236,243,255));RectF area(x+36*dpi,y,boxW-47*dpi,boxH);StringFormat f;f.SetLineAlignment(StringAlignmentCenter);g.DrawString(pumpLabel(v.state),-1,&font,area,&f,&text);}lastState=v.state;lastDpi=dpi;}
  static POINT lastOrigin{};POINT pos{origin.x+(LONG)((w-boxW)/2),origin.y+h-(LONG)(120*dpi)};if(dirty||pos.x!=lastOrigin.x||pos.y!=lastOrigin.y)s.present(pos);lastOrigin=pos;
 }
}
static void drawPumpOverlay(){
 auto hide=[](){pumpCourtSurface.hide();pumpStatusSurface.hide();};
 if(!pumpOn||!pumpVisuals||!gameFocused()){hide();return;}
 PumpVisual v;{std::lock_guard<std::mutex>lock(pumpVisualMutex);v=pumpVisual;}if(now()-v.at>.2||!v.valid||!samplePumpVisual(v)){hide();return;}
 HWND game=GetForegroundWindow();RECT rc{};GetClientRect(game,&rc);POINT origin{};ClientToScreen(game,&origin);int w=rc.right,h=rc.bottom;if(w<100||h<100||w>8000||h>5000){hide();return;}
 renderPumpVisual(v,w,h,origin,GetDpiForWindow(game)/96.f);
}

#include "flight_overlay.hpp"
#include "pump_frame.hpp"
#include "defense_overlay.hpp"
// Owns all overlay windows/resources on a separate thread. UI work and input
// hooks can no longer postpone an overlay frame. Read positions AFTER pacing.
static void pumpVisualWorker(){
 PumpFrameClock clock;double deadline=now(),monitorAt=-10;int hz=120;
 while(running){
  MSG msg{};while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}
  if(!running)break;
  double t=now();if(t<deadline){clock.waitUntil(deadline);continue;}
  bool active=((pumpOn&&pumpVisuals)||ballArcOn||rollingOn||defense::any()||defense::pick)&&gameFocused();
  if(active&&t-monitorAt>1){monitorAt=t;MONITORINFOEXW info{};info.cbSize=sizeof info;DEVMODEW mode{};mode.dmSize=sizeof mode;
   if(GetMonitorInfoW(MonitorFromWindow(GetForegroundWindow(),MONITOR_DEFAULTTONEAREST),&info)&&EnumDisplaySettingsW(info.szDevice,ENUM_CURRENT_SETTINGS,&mode)&&mode.dmDisplayFrequency>1)hz=std::clamp((int)mode.dmDisplayFrequency,60,240);
  }
  if(active)courtDepth::masks.update();else courtDepth::masks.polygons.clear();
  drawPumpOverlay();drawFlightOverlay();defenseVisual::draw();double period=active?1./hz:.05;deadline+=period;
  // Drop missed frames rather than rendering old snapshots back-to-back.
  double end=now();if(deadline<=end)deadline=end;
 }
 defenseVisual::close();courtDepth::masks.close();closeFlightOverlay();closePumpOverlay();
}
