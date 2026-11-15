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
 auto ring=[&](ff3qb::Vec center,double radius,Color c,float width,bool dashed=false){ff3qb::Screen last;for(int i=0;i<=64;i++){double a=i*6.283185307179586/64;auto q=project(center+ff3qb::Vec{std::cos(a)*radius,0,std::sin(a)*radius});if(i)line(last,q,c,width,dashed);last=q;}};
 if(f.ready){
  GraphicsPath beam;ff3qb::Screen previous;
  for(int i=0;i<=64;i++){auto p=curve&&curve->points.size()==65?curve->points[i]:ff3qb::point(f.solution,f.gravity,f.solution.time*i/64.);auto q=ff3qb::project(p,f.matrix,w,h);if(i&&previous.valid&&q.valid)beam.AddLine(REAL(previous.x),REAL(previous.y),REAL(q.x),REAL(q.y));else beam.StartFigure();previous=q;}
  Pen glow(Color(42,color.GetR(),color.GetG(),color.GetB()),8),edge(Color(130,10,15,25),4.5f),core(color,2.5f);core.SetLineJoin(LineJoinRound);g.DrawPath(&glow,&beam);g.DrawPath(&edge,&beam);g.DrawPath(&core,&beam);
 }
 if(f.ready&&std::isfinite(ground)){
  ff3qb::Screen previous;GraphicsPath lane;
  for(int i=0;i<=64;i++){auto p=curve&&curve->points.size()==65?curve->points[i]:ff3qb::point(f.solution,f.gravity,f.solution.time*i/64.);auto q=project(p);if(i&&previous.valid&&q.valid)lane.AddLine(REAL(previous.x),REAL(previous.y),REAL(q.x),REAL(q.y));else lane.StartFigure();previous=q;}
  Pen laneShadow(Color(90,8,12,22),6),laneGlow(Color(35,color.GetR(),color.GetG(),color.GetB()),5),laneStroke(color,2.2f);laneStroke.SetLineJoin(LineJoinRound);g.DrawPath(&laneShadow,&lane);g.DrawPath(&laneGlow,&lane);g.DrawPath(&laneStroke,&lane);
  auto landing=f.solution.catchPoint,unfitted=landing-f.fitOffset;
  ring(landing,2.6,color,2);ring(landing,3.1,Color(55,color.GetR(),color.GetG(),color.GetB()),1);
  auto end=project(landing),base=project(unfitted);if(ff3qb::length(f.fitOffset)>.5){ring(unfitted,1.2,Color(145,181,188,208),1,true);line(base,end,Color(220,190,195,255),1.6f,true);}
  if(end.valid){float x=std::clamp(float(end.x)-66,4.f,std::max(4.f,w-140.f)),y=std::clamp(float(end.y)+13,4.f,std::max(4.f,h-60.f));rounded(g,{x,y,132,34},10,Color(225,18,21,32),Color(80,color.GetR(),color.GetG(),color.GetB()));text(g,L"CATCH / "+number(f.solution.time,2)+L" s",x+10,y+7,10,color,true,115);}
  if(ff3Native::showCandidates){
   for(const auto& d:f.defenders)if(ff3qb::length(d.position-landing)<85)ring(d.position,2.0,Color(150,255,125,143),1.2f);
   const int index=f.coverageTrace.catcher;
   if(index>=0&&index<int(f.defenders.size())){
    const auto& d=f.defenders[index];double t=f.coverageTrace.time+std::clamp(f.ping*.0005+.05,0.,.25),speed=std::clamp(std::max(d.speed,std::hypot(d.velocity.x,d.velocity.z)),0.,45.);
    double reach=d.radius+speed*t+std::min(d.diveReach,std::max(0.,t-.1)*std::max(0.,45-speed));
    ring(d.position,reach,Color(105,255,125,143),1,true);
    auto db=project(d.position),closestPoint=project(f.coverageTrace.point);line(db,closestPoint,Color(160,255,125,143),1,true);
    if(db.valid){float x=std::clamp(float(db.x)-176,4.f,std::max(4.f,w-175.f)),y=std::clamp(float(db.y)-54,4.f,std::max(4.f,h-55.f));rounded(g,{x,y,164,40},10,Color(220,25,20,30),Color(60,255,125,143));text(g,L"DB REACH / "+number(t,2)+L" s",x+10,y+6,9,Color(255,244,160,173),true,148);std::wstring how=f.margin<=1?L"OVERLAP":f.coverageTrace.point.y>d.position.y+d.above+ff3qb::jumpRise(d.jumpHeight,d.worldGravity,t,d.velocity.y)?L"OVER":L"SIDE";text(g,how+L" / clearance "+number(f.margin,1)+L" st",x+10,y+23,8,Color(255,192,182,198),false,148);}
   }
  }
 }
 // Receiver card stays compact; the aim path itself is projected onto turf.
 if(f.receiver.valid){float x=std::clamp(float(f.receiver.x)-95,4.f,std::max(4.f,w-194.f)),y=std::clamp(float(f.receiver.y)+24,4.f,std::max(4.f,h-54.f));rounded(g,{x,y,190,44},14,Color(235,18,21,32),Color(75,190,195,255));text(g,basketballUi::widen(f.name),x+12,y+7,11,Color(255,235,239,252),true,147);text(g,f.locked?L"Q":L"",x+168,y+9,9,Color(255,190,195,255),true,14);text(g,!f.ready?L"READING THROW":!std::isfinite(ground)?L"WAITING FOR FIELD PLANE":(f.margin>1?L"OPEN EST. / ":L"CONTESTED / ")+number(f.power)+L" P / "+number(f.solution.angle)+L" deg",x+12,y+27,8,color,true,171);}
 if(present)surface.present(f.origin);
}
// Full measured flight and forward prediction to the retained field plane.
inline void renderBalls(PumpSurface& surface,const ff3Native::Frame& f,bool present=true){
 using namespace Gdiplus;int w=f.client.right,h=f.client.bottom;if(!surface.prepare(w,h)){surface.hide();return;}
 Bitmap canvas(surface.width,surface.height,surface.width*4,PixelFormat32bppPARGB,(BYTE*)surface.bits);Graphics g(&canvas);g.Clear(Color(0,0,0,0));g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);g.SetClip(Rect(0,0,w,h));
 auto project=[&](ff3qb::Vec p){return ff3qb::project(p,f.matrix,w,h);};
 auto draw=[&](const std::vector<ff3qb::Vec>& path,Color color,float width){Pen shadow(Color(100,8,12,25),width+2),pen(color,width);pen.SetLineJoin(LineJoinRound);pen.SetStartCap(LineCapRound);pen.SetEndCap(LineCapRound);GraphicsPath shape;for(size_t i=1;i<path.size();i++){auto a=project(path[i-1]),b=project(path[i]);if(!a.valid||!b.valid){shape.StartFigure();continue;}shape.AddLine(REAL(a.x),REAL(a.y),REAL(b.x),REAL(b.y));}g.DrawPath(&shadow,&shape);g.DrawPath(&pen,&shape);};
 for(const auto& ball:f.balls){auto prediction=ff3qb::forecast(ball,f.groundY);draw(ball.trail,Color(120,172,181,255),1.5f);if(!prediction.valid)continue;
  draw(prediction.path,Color(35,190,195,255),7);draw(prediction.path,Color(240,190,195,255),2.4f);
  std::vector<ff3qb::Vec> ring;for(int i=0;i<=64;i++){double a=i*6.283185307179586/64;ring.push_back({prediction.landing.x+std::cos(a)*2.5,prediction.landing.y-.52,prediction.landing.z+std::sin(a)*2.5});}draw(ring,Color(235,208,212,255),2);
  auto spot=project(prediction.landing);if(spot.valid){float x=std::clamp(float(spot.x)-65,4.f,std::max(4.f,w-136.f)),y=std::clamp(float(spot.y)+12,4.f,std::max(4.f,h-45.f));rounded(g,{x,y,130,29},12,Color(230,23,23,40),Color(110,190,195,255));text(g,L"LAND ~ "+number(prediction.remaining,2)+L" s",x+12,y+8,10,Color(255,213,216,255),true,112);}
 }
 if(present)surface.present(f.origin);
}
// The renderer owns both small transparent surfaces, independent of input and UI.
inline void worker(){
 using namespace Gdiplus;ULONG_PTR token=0;GdiplusStartupInput startup;if(GdiplusStartup(&token,&startup,nullptr)!=Ok)return;PumpSurface field,balls;ff3qb::CurveTracker curve;
 while(running){
  MSG message{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}
  auto f=ff3Native::snapshot();bool live=(ff3Native::enabled||ff3Native::ballTrails)&&ff3Native::admitted()&&ff3Native::focused(f.pid)&&now()>=f.at&&now()-f.at<.15;
  if(!live){field.hide();balls.hide();curve.reset();Sleep(30);continue;}
  if(!f.client.right){f.window=GetForegroundWindow();GetClientRect(f.window,&f.client);ClientToScreen(f.window,&f.origin);}
  if(f.client.right>8000||f.client.bottom>5000){field.hide();balls.hide();Sleep(30);continue;}
  curve.update(f.solution,f.gravity,f.target,now());
  if(ff3Native::enabled&&ff3Native::visuals&&f.target&&f.receiver.valid&&!f.textFocus&&f.revision==ff3Native::revision)renderField(field,f,true,&curve);else field.hide();if(ff3Native::ballTrails&&!f.balls.empty())renderBalls(balls,f);else balls.hide();Sleep(16);
 }
 field.close();balls.close();GdiplusShutdown(token);
}
}
