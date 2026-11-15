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

inline void renderField(PumpSurface& surface,const ff3Native::Frame& f,bool present=true,const ff3qb::CurveTracker* curve=nullptr){
 using namespace Gdiplus;const int w=f.client.right,h=f.client.bottom;
 if(!surface.prepare(w,h)){surface.hide();return;}
 Bitmap canvas(surface.width,surface.height,surface.width*4,PixelFormat32bppPARGB,(BYTE*)surface.bits);Graphics g(&canvas);g.Clear(Color(0,0,0,0));g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);g.SetClip(Rect(0,0,w,h));
 const auto color=accent(f);const double ground=f.groundY+.08;
 auto project=[&](ff3qb::Vec p){p.y=ground;return ff3qb::project(p,f.matrix,w,h);};
 auto line=[&](ff3qb::Screen a,ff3qb::Screen b,Color c,float width,bool dashed=false){if(!a.valid||!b.valid)return;Pen p(c,width);p.SetStartCap(LineCapRound);p.SetEndCap(LineCapRound);if(dashed)p.SetDashStyle(DashStyleDash);g.DrawLine(&p,REAL(a.x),REAL(a.y),REAL(b.x),REAL(b.y));};
 auto ring=[&](ff3qb::Vec center,double radius,Color c,float width,bool dashed=false){GraphicsPath path;ff3qb::Screen last;for(int i=0;i<=48;i++){double a=i*6.283185307179586/48;auto q=project(center+ff3qb::Vec{std::cos(a)*radius,0,std::sin(a)*radius});if(i&&last.valid&&q.valid)path.AddLine(REAL(last.x),REAL(last.y),REAL(q.x),REAL(q.y));else path.StartFigure();last=q;}Pen pen(c,width);pen.SetLineJoin(LineJoinRound);if(dashed)pen.SetDashStyle(DashStyleDash);g.DrawPath(&pen,&path);};
 if(f.ready){
  GraphicsPath beam;ff3qb::Screen previous;
  for(int i=0;i<=64;i++){auto p=curve&&curve->points.size()==65?curve->points[i]:ff3qb::point(f.solution,f.gravity,f.solution.time*i/64.);auto q=ff3qb::project(p,f.matrix,w,h);if(i&&previous.valid&&q.valid)beam.AddLine(REAL(previous.x),REAL(previous.y),REAL(q.x),REAL(q.y));else beam.StartFigure();previous=q;}
  Pen glow(Color(35,172,163,255),10),edge(Color(120,16,18,32),4.8f),core(Color(245,210,204,255),2.4f);core.SetLineJoin(LineJoinRound);core.SetStartCap(LineCapRound);core.SetEndCap(LineCapRound);g.DrawPath(&glow,&beam);g.DrawPath(&edge,&beam);g.DrawPath(&core,&beam);
 }
 if(f.ready&&std::isfinite(ground)){
  auto landing=curve&&curve->points.size()==65?curve->points.back():f.solution.catchPoint;auto unfitted=landing-f.fitOffset;
  ring(landing,2.6,color,2);ring(landing,3.1,Color(55,color.GetR(),color.GetG(),color.GetB()),1);
  auto end=project(landing),base=project(unfitted);if(ff3qb::length(f.fitOffset)>.5){ring(unfitted,1.2,Color(145,181,188,208),1,true);line(base,end,Color(220,190,195,255),1.6f,true);}
  if(ff3Native::showCandidates){
   for(const auto& d:f.defenders)if(ff3qb::length(d.position-landing)<85)ring(d.position,2.0,Color(150,255,125,143),1.2f);

  }
 }
 // Text stays in the single HTML HUD; field visuals contain geometry only.
 if(present)surface.present(f.origin);
}
// Full measured flight and forward prediction to the retained field plane.
inline void renderBalls(PumpSurface& surface,const ff3Native::Frame& f,bool present=true){
 using namespace Gdiplus;int w=f.client.right,h=f.client.bottom;if(!surface.prepare(w,h)){surface.hide();return;}
 Bitmap canvas(surface.width,surface.height,surface.width*4,PixelFormat32bppPARGB,(BYTE*)surface.bits);Graphics g(&canvas);g.Clear(Color(0,0,0,0));g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);g.SetClip(Rect(0,0,w,h));
 auto project=[&](ff3qb::Vec p){return ff3qb::project(p,f.matrix,w,h);};
 auto draw=[&](const std::vector<ff3qb::Vec>& path,Color color,float width){Pen shadow(Color(100,8,12,25),width+2),pen(color,width);pen.SetLineJoin(LineJoinRound);pen.SetStartCap(LineCapRound);pen.SetEndCap(LineCapRound);GraphicsPath shape;for(size_t i=1;i<path.size();i++){auto a=project(path[i-1]),b=project(path[i]);if(!a.valid||!b.valid){shape.StartFigure();continue;}shape.AddLine(REAL(a.x),REAL(a.y),REAL(b.x),REAL(b.y));}g.DrawPath(&shadow,&shape);g.DrawPath(&pen,&shape);};
 for(const auto& ball:f.balls){auto prediction=ff3qb::forecast(ball,f.groundY);if(!prediction.valid)continue;
  draw(prediction.path,Color(28,144,219,245),8);draw(prediction.path,Color(225,165,229,248),2.4f);
  std::vector<ff3qb::Vec> ring;for(int i=0;i<=64;i++){double a=i*6.283185307179586/64;ring.push_back({prediction.landing.x+std::cos(a)*2.5,prediction.landing.y-.52,prediction.landing.z+std::sin(a)*2.5});}draw(ring,Color(235,208,212,255),2);
 }
 if(present)surface.present(f.origin);
}
// Projection is sampled on this rendering thread, not from the slower planner.
struct ViewReader {
 HANDLE handle=nullptr;DWORD pid=0;
 ~ViewReader(){if(handle)CloseHandle(handle);}
 static bool matrix(HANDLE process,addr address,float* out){
  struct Matrix{float v[16];};Matrix fallback;for(auto& value:fallback.v)value=NAN;
  auto m=rd<Matrix>(process,address,fallback);for(auto value:m.v)if(!std::isfinite(value))return false;
  std::copy(std::begin(m.v),std::end(m.v),out);return true;
 }
 bool refresh(ff3Native::Frame& f){
  if(pid!=f.pid){if(handle)CloseHandle(handle);pid=f.pid;handle=OpenProcess(PROCESS_VM_READ|SYNCHRONIZE,FALSE,pid);}
  if(!handle||!f.cameraMatrix||WaitForSingleObject(handle,0)!=WAIT_TIMEOUT)return false;
  if(!matrix(handle,f.cameraMatrix,f.matrix))return false;
  auto window=GetForegroundWindow();DWORD owner=0;GetWindowThreadProcessId(window,&owner);if(owner!=f.pid)return false;
  POINT origin{};RECT client{};if(!GetClientRect(window,&client)||!ClientToScreen(window,&origin))return false;
  f.client=client;f.origin=origin;return true;
 }
};
// The renderer owns the transparent surfaces, independent of input and UI.
inline void worker(){
 using namespace Gdiplus;ULONG_PTR token=0;GdiplusStartupInput startup;if(GdiplusStartup(&token,&startup,nullptr)!=Ok)return;PumpSurface field,balls;ff3qb::CurveTracker curve;ViewReader view;
 while(running){
  MSG message{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}
  auto f=ff3Native::snapshot();bool live=(ff3Native::enabled||ff3Native::ballTrails)&&ff3Native::admitted()&&ff3Native::focused(f.pid)&&now()>=f.at&&now()-f.at<.15;
  if(!live){field.hide();balls.hide();curve.reset();Sleep(30);continue;}
  if(!f.client.right){f.window=GetForegroundWindow();GetClientRect(f.window,&f.client);ClientToScreen(f.window,&f.origin);}
  if(f.client.right>8000||f.client.bottom>5000){field.hide();balls.hide();Sleep(30);continue;}
  if(!view.refresh(f)){field.hide();balls.hide();Sleep(16);continue;}
  for(auto& ball:f.balls)if(ball.model.valid)ball.at=now();
  curve.update(f.solution,f.gravity,f.target,now());
  if(ff3Native::enabled&&ff3Native::visuals&&f.target&&!f.textFocus&&f.revision==ff3Native::revision)renderField(field,f,true,&curve);else field.hide();if(ff3Native::ballTrails&&!f.balls.empty())renderBalls(balls,f);else balls.hide();Sleep(16);
 }
 field.close();balls.close();GdiplusShutdown(token);
}
}
