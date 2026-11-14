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
inline Gdiplus::Color accent(const ff3Native::Frame& f){return !f.coverageKnown?Gdiplus::Color(255,151,180,255):f.margin>ff3qb::catchSafetyMargin?Gdiplus::Color(255,115,232,186):Gdiplus::Color(255,255,191,115);}
inline void renderHud(PumpSurface& surface,const ff3Native::Frame& f,bool present=true){
 using namespace Gdiplus;
 const int width=520,height=206;if(f.client.right<width+20||f.client.bottom<height+40||!surface.prepare(width,height))return;
 Bitmap canvas(surface.width,surface.height,surface.width*4,PixelFormat32bppPARGB,(BYTE*)surface.bits);Graphics g(&canvas);g.Clear(Color(0,0,0,0));g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
 const bool connecting=ff3Native::packetStatus&&!ff3Native::packetStatus().first;
 const auto color=accent(f);const Color white(255,235,243,248),muted(255,134,151,168);
 rounded(g,{3,3,514,198},16,Color(244,10,16,24),Color(90,108,135,153));
 text(g,L"VANTA  /  QUARTERBACK",20,15,9,muted,true,280);
 text(g,ff3Native::force95?L"ADAPTIVE BEAM":basketballUi::widen(ff3qb::presetName(f.mode)),388,14,10,color,true,112);
 text(g,f.name.empty()?L"Select a receiver":basketballUi::widen(f.name),20,32,22,white,true,340);
 text(g,f.locked?L"LOCKED  /  Q":L"CURSOR  /  Q",389,39,9,muted,false,112);
 Pen divider(Color(50,108,135,153),1);g.DrawLine(&divider,20.f,65.f,500.f,65.f);
 // Power is the visual anchor; trajectory and placement sit beside it.
 Pen track(Color(55,119,160,166),4),meter(Color(255,142,229,213),4);track.SetStartCap(LineCapRound);meter.SetStartCap(LineCapRound);meter.SetEndCap(LineCapRound);
 g.DrawArc(&track,22.f,80.f,76.f,76.f,135.f,270.f);
 if(f.ready)g.DrawArc(&meter,22.f,80.f,76.f,76.f,135.f,270.f*float(std::clamp(f.power/95.,0.,1.)));
 text(g,f.ready?number(f.power):L"--",39,92,29,white,true,58);text(g,L"POWER / 95",29,130,8,muted,false,70);
 const std::wstring values[]={f.ready?number(f.solution.angle,1)+L" deg":L"--",f.ready?number(f.solution.time,2)+L" s":L"--",f.ready?number(std::hypot(f.fitOffset.x,f.fitOffset.z),1)+L" st":L"--",f.ready?(f.fitOffset.y>=3?L"JUMP +"+number(f.fitOffset.y,1):L"HEAD"):L"--"};
 const wchar_t* labels[]={L"ELEVATION",L"FLIGHT TIME",L"PLACEMENT",L"CATCH HEIGHT"};
 for(int i=0;i<4;i++){float x=122.f+(i%2)*190,y=78.f+(i/2)*42;text(g,labels[i],x,y,8,muted,false,174);text(g,values[i],x,y+12,17,i==3?color:white,true,174);}
 const auto status=connecting?L"SETTING UP":f.fitBlocked?L"UNREACHABLE":!f.ready?L"STANDBY":!f.coverageKnown?L"INCOMPLETE":f.coverageCount==0?L"NO COVERAGE":f.margin>ff3qb::catchSafetyMargin?L"OPEN ESTIMATE":L"CONTESTED";
 rounded(g,{18,164,118,23},6,Color(20,color.GetR(),color.GetG(),color.GetB()),Color(50,color.GetR(),color.GetG(),color.GetB()));text(g,status,27,169,9,color,true,105);
 std::wstring detail=f.ready?basketballUi::widen(f.route)+L" / "+number(f.distance)+L" st":basketballUi::widen(f.reason);
 if(connecting)detail=ff3Native::packetStatus().second;
 text(g,detail,148,169,10,muted,false,233);text(g,L"Z  PASS   R/F  ANGLE",385,170,8,muted,false,120);
 if(present)surface.present({f.origin.x+(f.client.right-width)/2,f.origin.y+f.client.bottom-height-28});
}
inline void renderField(PumpSurface& surface,const ff3Native::Frame& f,bool present=true){
 using namespace Gdiplus;const int w=f.client.right,h=f.client.bottom;
 std::vector<ff3qb::Screen> points,catchRing;
 double left=f.receiver.x-110,top=f.receiver.y-50,right=f.receiver.x+110,bottom=f.receiver.y+50;
 auto bounds=[&](ff3qb::Screen p){if(p.valid){left=std::min(left,p.x-90);top=std::min(top,p.y-60);right=std::max(right,p.x+90);bottom=std::max(bottom,p.y+52);}};
 if(f.ready)for(int i=0;i<=64;i++){auto p=ff3qb::project(ff3qb::point(f.solution,f.gravity,f.solution.time*i/64.),f.matrix,w,h);points.push_back(p);bounds(p);}
 if(f.ready)for(int i=0;i<=40;i++){double a=i*6.283185307/40;auto p=ff3qb::project(f.solution.catchPoint+ff3qb::Vec{3*cos(a),-3,3*sin(a)},f.matrix,w,h);catchRing.push_back(p);bounds(p);}
 if(ff3Native::showCandidates)for(const auto& c:f.candidates)bounds(c.screen);
 if(f.ready){auto launch=ff3qb::project(f.solution.origin,f.matrix,w,h);bounds(launch);}
 const int x=std::clamp(int(std::floor(left))-8,0,w),y=std::clamp(int(std::floor(top))-8,0,h),x1=std::clamp(int(std::ceil(right))+8,0,w),y1=std::clamp(int(std::ceil(bottom))+8,0,h);
 if(x1<=x||y1<=y||!surface.prepare(x1-x,y1-y)){surface.hide();return;}
 Bitmap canvas(surface.width,surface.height,surface.width*4,PixelFormat32bppPARGB,(BYTE*)surface.bits);Graphics g(&canvas);g.Clear(Color(0,0,0,0));g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);g.TranslateTransform(REAL(-x),REAL(-y));g.SetClip(Rect(0,0,w,h));
 Color color=accent(f);Pen shadow(Color(170,5,10,20),4),glow(Color(30,color.GetR(),color.GetG(),color.GetB()),9),line(color,2),muted(Color(100,164,188,228),1);line.SetStartCap(LineCapRound);line.SetEndCap(LineCapRound);
 auto segment=[&](const auto& path,Pen& pen){for(size_t i=1;i<path.size();i++)if(path[i-1].valid&&path[i].valid)g.DrawLine(&pen,PointF(REAL(path[i-1].x),REAL(path[i-1].y)),PointF(REAL(path[i].x),REAL(path[i].y)));};
 segment(points,shadow);segment(points,glow);segment(points,line);segment(catchRing,glow);segment(catchRing,line);
 if(ff3Native::showCandidates)for(const auto& c:f.candidates)if(!c.selected&&c.screen.valid){float cx=float(c.screen.x),cy=float(c.screen.y);g.DrawEllipse(&muted,cx-6,cy-6,12.f,12.f);}
 // Moving time dots show direction without flashing the entire trajectory.
 SolidBrush dots(Color(255,237,246,255));double phase=.125;
 if(f.ready)for(int i=0;i<4;i++){double t=phase+i*.25;auto p=ff3qb::project(ff3qb::point(f.solution,f.gravity,f.solution.time*t),f.matrix,w,h);if(p.valid)g.FillEllipse(&dots,REAL(p.x)-2,REAL(p.y)-2,4.f,4.f);}
 const float rx=float(f.receiver.x),ry=float(f.receiver.y),r=17,len=7;
 for(int dx:{-1,1})for(int dy:{-1,1}){g.DrawLine(&shadow,rx+dx*r,ry+dy*r,rx+dx*(r-len),ry+dy*r);g.DrawLine(&shadow,rx+dx*r,ry+dy*r,rx+dx*r,ry+dy*(r-len));g.DrawLine(&line,rx+dx*r,ry+dy*r,rx+dx*(r-len),ry+dy*r);g.DrawLine(&line,rx+dx*r,ry+dy*r,rx+dx*r,ry+dy*(r-len));}
 rounded(g,{rx-105,ry+29,210,24},7,Color(225,10,18,32),Color(90,color.GetR(),color.GetG(),color.GetB()));text(g,(f.locked?L"LOCKED / ":L"")+basketballUi::widen(f.name),rx-96,ry+32,12,color,true,191);
 if(!points.empty()&&points.back().valid){float cx=float(points.back().x),cy=float(points.back().y);g.DrawEllipse(&shadow,cx-6,cy-6,12.f,12.f);g.DrawEllipse(&line,cx-6,cy-6,12.f,12.f);g.DrawLine(&line,cx-11,cy,cx+11,cy);g.DrawLine(&line,cx,cy-11,cx,cy+11);rounded(g,{cx-71,cy-40,142,26},6,Color(235,10,16,24),Color(75,color.GetR(),color.GetG(),color.GetB()));text(g,(f.fitOffset.y>=3?L"JUMP CATCH":L"CATCH POINT")+std::wstring(L" / ")+number(f.solution.time,1)+L"s",cx-62,cy-34,9,color,true,126);Pen lead(Color(130,color.GetR(),color.GetG(),color.GetB()),1);lead.SetDashStyle(DashStyleDash);g.DrawLine(&lead,rx,ry,cx,cy);}
 if(present)surface.present({f.origin.x+x,f.origin.y+y});
}
// Observed flight history and a short, separately dashed gravity forecast.
inline void renderBalls(PumpSurface& surface,const ff3Native::Frame& f,bool present=true){
 using namespace Gdiplus;int w=f.client.right,h=f.client.bottom;double left=w,top=h,right=0,bottom=0;
 struct Path{std::vector<ff3qb::Screen> trail,future;ff3qb::Screen ball;};std::vector<Path> paths;
 auto project=[&](ff3qb::Vec p){auto q=ff3qb::project(p,f.matrix,w,h);if(q.valid){left=std::min(left,q.x-12);top=std::min(top,q.y-12);right=std::max(right,q.x+12);bottom=std::max(bottom,q.y+12);}return q;};
 for(const auto& flight:f.balls){Path path;for(auto p:flight.trail)path.trail.push_back(project(p));path.ball=project(flight.position);
  if(flight.gravity>0)for(int i=0;i<=20;i++){double t=i*.03;path.future.push_back(project(flight.position+flight.velocity*t+ff3qb::Vec{0,-.5*flight.gravity*t*t,0}));}paths.push_back(path);
 }
 int x=std::clamp(int(left),0,w),y=std::clamp(int(top),0,h),x1=std::clamp(int(right)+1,0,w),y1=std::clamp(int(bottom)+1,0,h);
 if(x1<=x||y1<=y||!surface.prepare(x1-x,y1-y)){surface.hide();return;}
 Bitmap canvas(surface.width,surface.height,surface.width*4,PixelFormat32bppPARGB,(BYTE*)surface.bits);Graphics g(&canvas);g.Clear(Color(0,0,0,0));g.SetSmoothingMode(SmoothingModeAntiAlias);g.TranslateTransform(REAL(-x),REAL(-y));
 for(const auto& path:paths){
  for(size_t i=1;i<path.trail.size();i++)if(path.trail[i-1].valid&&path.trail[i].valid){Pen shadow(Color(150,4,12,20),4),trail(Color(BYTE(50+190*i/path.trail.size()),100,218,255),2);g.DrawLine(&shadow,REAL(path.trail[i-1].x),REAL(path.trail[i-1].y),REAL(path.trail[i].x),REAL(path.trail[i].y));g.DrawLine(&trail,REAL(path.trail[i-1].x),REAL(path.trail[i-1].y),REAL(path.trail[i].x),REAL(path.trail[i].y));}
  Pen forecast(Color(130,145,215,245),1);forecast.SetDashStyle(DashStyleDash);for(size_t i=1;i<path.future.size();i++)if(i%3!=0&&path.future[i-1].valid&&path.future[i].valid)g.DrawLine(&forecast,REAL(path.future[i-1].x),REAL(path.future[i-1].y),REAL(path.future[i].x),REAL(path.future[i].y));
  if(path.ball.valid){Pen ring(Color(255,187,240,255),1.5f);SolidBrush core(Color(240,225,250,255));g.DrawEllipse(&ring,REAL(path.ball.x)-5,REAL(path.ball.y)-5,10.f,10.f);g.FillEllipse(&core,REAL(path.ball.x)-2,REAL(path.ball.y)-2,4.f,4.f);}
 }
 if(present)surface.present({f.origin.x+x,f.origin.y+y});
}
// The renderer owns both small transparent surfaces, independent of input and UI.
inline void worker(){
 using namespace Gdiplus;ULONG_PTR token=0;GdiplusStartupInput startup;if(GdiplusStartup(&token,&startup,nullptr)!=Ok)return;PumpSurface field,hud,balls;
 while(running){
  MSG message{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}
  auto f=ff3Native::snapshot();bool live=(ff3Native::enabled||ff3Native::ballTrails)&&ff3Native::admitted()&&ff3Native::focused(f.pid)&&now()>=f.at&&now()-f.at<.15;
  if(!live){field.hide();hud.hide();balls.hide();Sleep(30);continue;}
  if(!f.client.right){f.window=GetForegroundWindow();GetClientRect(f.window,&f.client);ClientToScreen(f.window,&f.origin);}
  if(f.client.right>8000||f.client.bottom>5000){field.hide();hud.hide();balls.hide();Sleep(30);continue;}
  if(ff3Native::enabled&&ff3Native::showHud)renderHud(hud,f);else hud.hide();
  if(ff3Native::enabled&&ff3Native::visuals&&f.target&&f.receiver.valid&&!f.textFocus&&f.revision==ff3Native::revision)renderField(field,f);else field.hide();if(ff3Native::ballTrails&&!f.balls.empty())renderBalls(balls,f);else balls.hide();Sleep(16);
 }
 field.close();hud.close();balls.close();GdiplusShutdown(token);
}
}
