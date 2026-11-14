#pragma once
#include "ff3_qb_runtime.hpp"
#include "pump_overlay.hpp"

namespace ff3Visual {
inline void rounded(Gdiplus::Graphics& g,Gdiplus::RectF r,float radius,Gdiplus::Color fill,Gdiplus::Color edge){
 using namespace Gdiplus;GraphicsPath p;float d=radius*2;
 p.AddArc(r.X,r.Y,d,d,180,90);p.AddArc(r.GetRight()-d,r.Y,d,d,270,90);p.AddArc(r.GetRight()-d,r.GetBottom()-d,d,d,0,90);p.AddArc(r.X,r.GetBottom()-d,d,d,90,90);p.CloseFigure();SolidBrush b(fill);Pen line(edge,1);g.FillPath(&b,&p);g.DrawPath(&line,&p);
}
inline void text(Gdiplus::Graphics& g,const std::wstring& value,float x,float y,float size,Gdiplus::Color color,bool bold=false,float width=400){
 using namespace Gdiplus;Font font(L"Segoe UI",size,bold?FontStyleBold:FontStyleRegular,UnitPixel);SolidBrush brush(color);StringFormat format;format.SetTrimming(StringTrimmingEllipsisCharacter);format.SetFormatFlags(StringFormatFlagsNoWrap);g.DrawString(value.c_str(),-1,&font,RectF(x,y,width,size*1.8f),&format,&brush);
}
inline std::wstring number(double value,int precision=0){std::wostringstream out;out<<std::fixed<<std::setprecision(precision)<<value;return out.str();}
inline Gdiplus::Color accent(const ff3Native::Frame& f){return !f.coverageKnown?Gdiplus::Color(255,151,180,255):f.margin>.15?Gdiplus::Color(255,115,232,186):Gdiplus::Color(255,255,191,115);}
inline void renderHud(PumpSurface& surface,const ff3Native::Frame& f,bool present=true){
 using namespace Gdiplus;
 const int width=448,height=170;if(f.client.right<width+20||f.client.bottom<height+40||!surface.prepare(width,height))return;
 Bitmap canvas(surface.width,surface.height,surface.width*4,PixelFormat32bppPARGB,(BYTE*)surface.bits);Graphics g(&canvas);g.Clear(Color(0,0,0,0));g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
 const auto color=accent(f);rounded(g,{2,2,444,164},15,Color(235,10,17,31),Color(105,119,153,211));
 text(g,L"QUARTERBACK",18,13,10,Color(255,151,174,212),true);
 text(g,basketballUi::widen(ff3qb::presetName(f.mode)),302,12,11,color,true,125);
 text(g,f.name.empty()?L"Find your receiver":basketballUi::widen(f.name),18,32,20,Color(255,239,245,255),true,265);
 text(g,!f.ready?L"WAITING":!f.coverageKnown?L"UNSCOUTED":f.margin>.15?L"OPEN ESTIMATE":L"CONTESTED",302,36,10,color,true,128);
 const std::wstring values[]={f.ready?number(f.power):L"--",f.ready?number(f.solution.angle)+L" deg":L"--",f.ready?number(f.solution.time,2)+L" s":L"--",f.ready?number(f.distance)+L" st":L"--"};
 const wchar_t* labels[]={L"POWER / 95",L"ANGLE",L"FLIGHT",L"DISTANCE"};
 for(int i=0;i<4;i++){float x=18.f+106*i;text(g,labels[i],x,65,9,Color(255,133,151,181));text(g,values[i],x,79,19,i==0?color:Color(255,228,236,250),true,103);}
 Pen base(Color(80,151,180,255),3),bar(color,3);g.DrawLine(&base,18.f,112.f,428.f,112.f);if(f.ready)g.DrawLine(&bar,18.f,112.f,18.f+410.f*float(std::clamp(f.power/95.,0.,1.)),112.f);
 auto detail=f.ready?(basketballUi::widen(f.route)+L"  /  "+(f.coverageKnown?L"margin "+number(f.margin,2)+L"s":L"coverage unknown")):basketballUi::widen(f.reason);
 text(g,detail,18,122,11,Color(255,174,191,217),false,410);
 text(g,ff3Native::automaticAngle?L"AUTO ANGLE  /  Aim near a receiver, click to pass":ff3Native::autoPower?L"R  raise angle     F  lower angle  /  Power follows":L"GAME POWER  /  R and F control the game's power",18,142,10,Color(255,126,153,195),false,410);
 if(present)surface.present({f.origin.x+(f.client.right-width)/2,f.origin.y+f.client.bottom-height-28});
}
inline void renderField(PumpSurface& surface,const ff3Native::Frame& f,bool present=true){
 using namespace Gdiplus;const int w=f.client.right,h=f.client.bottom;
 std::vector<ff3qb::Screen> points,catchRing;
 double left=f.receiver.x-110,top=f.receiver.y-50,right=f.receiver.x+110,bottom=f.receiver.y+50;
 auto bounds=[&](ff3qb::Screen p){if(p.valid){left=std::min(left,p.x-18);top=std::min(top,p.y-18);right=std::max(right,p.x+18);bottom=std::max(bottom,p.y+18);}};
 for(int i=0;i<=64;i++){auto p=ff3qb::project(ff3qb::point(f.solution,f.gravity,f.solution.time*i/64.),f.matrix,w,h);points.push_back(p);bounds(p);}
 for(int i=0;i<=40;i++){double a=i*6.283185307/40;auto p=ff3qb::project(f.solution.catchPoint+ff3qb::Vec{3*cos(a),-3,3*sin(a)},f.matrix,w,h);catchRing.push_back(p);bounds(p);}
 if(ff3Native::showCandidates)for(const auto& c:f.candidates)bounds(c.screen);
 auto launch=ff3qb::project(f.solution.origin,f.matrix,w,h);bounds(launch);
 const int x=std::clamp(int(std::floor(left))-8,0,w),y=std::clamp(int(std::floor(top))-8,0,h),x1=std::clamp(int(std::ceil(right))+8,0,w),y1=std::clamp(int(std::ceil(bottom))+8,0,h);
 if(x1<=x||y1<=y||!surface.prepare(x1-x,y1-y)){surface.hide();return;}
 Bitmap canvas(surface.width,surface.height,surface.width*4,PixelFormat32bppPARGB,(BYTE*)surface.bits);Graphics g(&canvas);g.Clear(Color(0,0,0,0));g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);g.TranslateTransform(REAL(-x),REAL(-y));g.SetClip(Rect(0,0,w,h));
 Color color=accent(f);Pen shadow(Color(210,5,10,20),5),glow(Color(45,color.GetR(),color.GetG(),color.GetB()),8),line(color,2),muted(Color(100,164,188,228),1);
 auto segment=[&](const auto& path,Pen& pen){for(size_t i=1;i<path.size();i++)if(path[i-1].valid&&path[i].valid)g.DrawLine(&pen,PointF(REAL(path[i-1].x),REAL(path[i-1].y)),PointF(REAL(path[i].x),REAL(path[i].y)));};
 segment(points,shadow);segment(points,glow);segment(points,line);segment(catchRing,glow);segment(catchRing,line);
 if(ff3Native::showCandidates)for(const auto& c:f.candidates)if(!c.selected&&c.screen.valid){float cx=float(c.screen.x),cy=float(c.screen.y);g.DrawEllipse(&muted,cx-6,cy-6,12.f,12.f);}
 // Moving time dots show direction without flashing the entire trajectory.
 SolidBrush dots(Color(255,237,246,255));double phase=std::fmod(now()*.22,1.);
 for(int i=0;i<4;i++){double t=std::fmod(phase+i*.25,1.);auto p=ff3qb::project(ff3qb::point(f.solution,f.gravity,f.solution.time*t),f.matrix,w,h);if(p.valid)g.FillEllipse(&dots,REAL(p.x)-2,REAL(p.y)-2,4.f,4.f);}
 const float rx=float(f.receiver.x),ry=float(f.receiver.y),r=17,len=7;
 for(int dx:{-1,1})for(int dy:{-1,1}){g.DrawLine(&shadow,rx+dx*r,ry+dy*r,rx+dx*(r-len),ry+dy*r);g.DrawLine(&shadow,rx+dx*r,ry+dy*r,rx+dx*r,ry+dy*(r-len));g.DrawLine(&line,rx+dx*r,ry+dy*r,rx+dx*(r-len),ry+dy*r);g.DrawLine(&line,rx+dx*r,ry+dy*r,rx+dx*r,ry+dy*(r-len));}
 rounded(g,{rx-105,ry-47,210,24},7,Color(225,10,18,32),Color(90,color.GetR(),color.GetG(),color.GetB()));text(g,basketballUi::widen(f.name),rx-96,ry-44,12,color,true,191);
 if(points.back().valid){float cx=float(points.back().x),cy=float(points.back().y);g.DrawEllipse(&shadow,cx-6,cy-6,12.f,12.f);g.DrawEllipse(&line,cx-6,cy-6,12.f,12.f);g.DrawLine(&line,cx-11,cy,cx+11,cy);g.DrawLine(&line,cx,cy-11,cx,cy+11);Pen lead(Color(130,color.GetR(),color.GetG(),color.GetB()),1);lead.SetDashStyle(DashStyleDash);g.DrawLine(&lead,rx,ry,cx,cy);}
 if(present)surface.present({f.origin.x+x,f.origin.y+y});
}
// The renderer owns both small transparent surfaces, independent of input and UI.
inline void worker(){
 using namespace Gdiplus;ULONG_PTR token=0;GdiplusStartupInput startup;if(GdiplusStartup(&token,&startup,nullptr)!=Ok)return;PumpSurface field,hud;
 while(running){
  MSG message{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}
  auto f=ff3Native::snapshot();bool live=ff3Native::enabled&&ff3Native::admitted()&&ff3Native::focused(f.pid)&&now()>=f.at&&now()-f.at<.15;
  if(!live){field.hide();hud.hide();Sleep(30);continue;}
  if(!f.client.right){f.window=GetForegroundWindow();GetClientRect(f.window,&f.client);ClientToScreen(f.window,&f.origin);}
  if(f.client.right>8000||f.client.bottom>5000){field.hide();hud.hide();Sleep(30);continue;}
  if(ff3Native::showHud)renderHud(hud,f);else hud.hide();
  if(ff3Native::visuals&&ff3Native::eligible(f,now()))renderField(field,f);else field.hide();Sleep(16);
 }
 field.close();hud.close();GdiplusShutdown(token);
}
}
