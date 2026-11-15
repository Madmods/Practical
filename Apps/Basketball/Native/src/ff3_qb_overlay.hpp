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
inline Gdiplus::Color accent(const ff3Native::Frame& f){return !f.hudCoverageKnown?Gdiplus::Color(255,151,180,255):f.risk.intable?Gdiplus::Color(255,255,125,143):f.risk.possible?Gdiplus::Color(255,255,191,115):Gdiplus::Color(255,115,232,186);}

// Labels use a dark backing and consistent screen-space sizing at any distance.
inline void label(Gdiplus::Graphics& g,ff3qb::Screen anchor,const std::wstring& title,const std::wstring& detail,Gdiplus::Color color,int w,int h,float dx=14,float dy=-38){
 if(!anchor.valid)return;float width=std::clamp(float(std::max(title.size()*8,detail.size()*7)+24),150.f,330.f),height=detail.empty()?30.f:49.f;
 float x=std::clamp(float(anchor.x)+dx,8.f,std::max(8.f,w-width-8)),y=std::clamp(float(anchor.y)+dy,8.f,std::max(8.f,h-height-8));
 rounded(g,{x,y,width,height},7,Gdiplus::Color(225,13,17,29),Gdiplus::Color(170,color.GetR(),color.GetG(),color.GetB()));
 text(g,title,x+10,y+5,14,color,true,width-20);if(!detail.empty())text(g,detail,x+10,y+26,12,Gdiplus::Color(240,223,228,239),false,width-20);
}
inline void renderField(PumpSurface& surface,const ff3Native::Frame& f,bool present=true){
 using namespace Gdiplus;const int w=f.client.right,h=f.client.bottom;if(!surface.prepare(w,h)){surface.hide();return;}
 Bitmap canvas(surface.width,surface.height,surface.width*4,PixelFormat32bppPARGB,(BYTE*)surface.bits);Graphics g(&canvas);g.Clear(Color(0,0,0,0));g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);g.SetClip(Rect(0,0,w,h));
 const auto riskColor=accent(f);const Color violet(255,192,178,255),mint(255,105,244,186),amber(255,255,191,95);const double ground=f.groundY+.08;
 auto world=[&](ff3qb::Vec p){return ff3qb::project(p,f.matrix,w,h);};auto floor=[&](ff3qb::Vec p){p.y=ground;return world(p);};
 auto line=[&](ff3qb::Screen a,ff3qb::Screen b,Color c,float width,bool dash=false){if(!a.valid||!b.valid)return;Pen shadow(Color(170,7,11,19),width+2),pen(c,width);pen.SetStartCap(LineCapRound);pen.SetEndCap(LineCapRound);if(dash)pen.SetDashStyle(DashStyleDash);g.DrawLine(&shadow,REAL(a.x),REAL(a.y),REAL(b.x),REAL(b.y));g.DrawLine(&pen,REAL(a.x),REAL(a.y),REAL(b.x),REAL(b.y));};
 auto ring=[&](ff3qb::Vec center,double radius,Color c,float width,bool fill=false){GraphicsPath path;ff3qb::Screen last;bool complete=true;for(int i=0;i<=64;++i){double a=i*6.28318530718/64;auto q=floor(center+ff3qb::Vec{radius*std::cos(a),0,radius*std::sin(a)});if(!q.valid)complete=false;if(i&&last.valid&&q.valid)path.AddLine(REAL(last.x),REAL(last.y),REAL(q.x),REAL(q.y));else path.StartFigure();last=q;}if(fill&&complete){path.CloseFigure();SolidBrush brush(Color(40,c.GetR(),c.GetG(),c.GetB()));g.FillPath(&brush,&path);}Pen shadow(Color(150,7,11,19),width+3),pen(c,width);g.DrawPath(&shadow,&path);g.DrawPath(&pen,&path);};
 auto arrow=[&](ff3qb::Screen a,ff3qb::Screen b,Color c){line(a,b,c,3);if(!a.valid||!b.valid)return;double dx=b.x-a.x,dy=b.y-a.y,len=std::hypot(dx,dy);if(len<5)return;dx/=len;dy/=len;line(b,{b.x-dx*11+dy*5,b.y-dy*11-dx*5,true},c,3);line(b,{b.x-dx*11-dy*5,b.y-dy*11+dx*5,true},c,3);};
 if(f.ready){
  auto flight=ff3qb::groundFlight(f.solution,f.gravity,f.groundY);if(!flight.valid)flight=f.solution;
  GraphicsPath beam;ff3qb::Screen last;for(int i=0;i<=112;++i){auto q=world(ff3qb::point(flight,f.gravity,flight.time*i/112.));if(i&&last.valid&&q.valid)beam.AddLine(REAL(last.x),REAL(last.y),REAL(q.x),REAL(q.y));else beam.StartFigure();last=q;}
  Pen glow(Color(f.afterRelease?35:65,175,151,255),14),shadow(Color(210,8,11,21),7),core(f.afterRelease?Color(200,192,178,255):Color(255,236,225,255),f.afterRelease?2.8f:4.f);core.SetLineJoin(LineJoinRound);core.SetStartCap(LineCapRound);core.SetEndCap(LineCapRound);if(f.afterRelease)core.SetDashStyle(DashStyleDash);g.DrawPath(&glow,&beam);g.DrawPath(&shadow,&beam);g.DrawPath(&core,&beam);
  // Flight-time ticks make height and arrival easier to read without extra cards.
  for(double u:{.25,.5,.75}){auto q=world(ff3qb::point(f.solution,f.gravity,f.solution.time*u));if(!q.valid)continue;SolidBrush dot(Color(255,236,225,255));g.FillEllipse(&dot,REAL(q.x-3),REAL(q.y-3),6.f,6.f);text(g,number(f.solution.time*u,1)+L"s",REAL(q.x+7),REAL(q.y-18),12,Color(255,242,236,255),true,70);}
  auto catchSpot=f.solution.catchPoint;auto catchScreen=world(catchSpot),catchGround=floor(catchSpot);
  if(std::isfinite(ground)){
   ring(catchSpot,2.5,mint,2.8f,true);ring(catchSpot,3.2,Color(145,105,244,186),1.3f);line(catchGround,catchScreen,Color(210,105,244,186),2,true);
   ring(flight.catchPoint,2.2,Color(170,192,178,255),1.5f);auto landing=floor(flight.catchPoint);
   if(landing.valid&&catchGround.valid&&std::hypot(landing.x-catchGround.x,landing.y-catchGround.y)>75)label(g,landing,L"GROUND LANDING",L"If the pass is not caught",violet,w,h,14,16);
  }
  if(catchScreen.valid){Pen marker(mint,3);g.DrawEllipse(&marker,REAL(catchScreen.x-11),REAL(catchScreen.y-11),22.f,22.f);for(int sign:{-1,1}){line({catchScreen.x+sign*15,catchScreen.y,true},{catchScreen.x+sign*21,catchScreen.y,true},mint,2);line({catchScreen.x,catchScreen.y+sign*15,true},{catchScreen.x,catchScreen.y+sign*21,true},mint,2);}}
  const auto title=f.afterRelease?L"RELEASE TARGET":L"CATCH HERE";
  auto detail=basketballUi::widen(f.name)+L"  |  "+number(f.solution.time,2)+L"s  |  "+number(catchSpot.y-f.groundY,1)+L" st high";
  label(g,catchScreen,title,detail,mint,w,h,24,-65);
  label(g,{18,18,true},f.afterRelease?L"YOUR THROW · RELEASE PLAN":L"QB AIM · "+basketballUi::widen(ff3Native::force95?"Beam":f.passLabel.empty()?ff3qb::presetName(f.mode):f.passLabel),f.afterRelease?L"Dashed violet: plan   Cyan: measured ball":L"Violet: arc   Green: catch   Amber: DB reach",violet,w,h,0,0);
  if(f.afterRelease&&f.throwView.phase==ff3qb::ThrowView::Done)label(g,catchGround,basketballUi::widen(f.throwView.result),basketballUi::widen(f.throwView.resultBy),riskColor,w,h,24,20);
  if(ff3Native::showCandidates&&std::isfinite(ground)){
   auto baseline=catchSpot-f.fitOffset;auto from=floor(baseline);
   if(ff3qb::length(f.fitOffset)>.5){ring(baseline,1.4,Color(200,180,190,212),1.5f);arrow(from,catchGround,violet);label(g,from,L"SMART FIT",L"Shift "+number(std::hypot(f.fitOffset.x,f.fitOffset.z),1)+L" st  |  lift "+number(f.fitOffset.y,1)+L" st",violet,w,h,-200,32);}
   if(f.receiverReachKnown){auto wr=f.receiverReach.position+f.targetVelocity*(f.solution.time+std::clamp(f.ping*.0005+.05,0.,.25));ring(wr,1.2,Color(210,105,244,186),1.8f);arrow(floor(wr),catchGround,mint);}
   const auto& trace=f.risk.intable?f.risk.projected:f.risk.limit;
   if(f.hudCoverageKnown&&trace.catcher>=0&&trace.catcher<int(f.hudDefenders.size())){
    const auto& db=f.hudDefenders[trace.catcher];auto at=ff3qb::point(f.solution,f.gravity,std::min(trace.time,f.solution.time));double horizon=trace.time+std::clamp(f.ping*.0005+.05,0.,.25);double reach=ff3qb::reachSectionRadius(db,at.y,horizon);
    auto dbGround=floor(db.position),cross=world(at);ring(db.position,2.4,amber,2,true);
    label(g,world(db.position+ff3qb::Vec{0,2,0}),L"LIMITING DB",L"Reach at +"+number(trace.time,2)+L"s",amber,w,h,-175,-50);
    if(reach>=0){double direction=std::atan2(at.z-db.position.z,at.x-db.position.x);GraphicsPath boundary;ff3qb::Screen last;for(int i=0;i<=40;++i){double a=direction-.45+.9*i/40;auto q=floor(db.position+ff3qb::Vec{std::cos(a)*reach,0,std::sin(a)*reach});if(i&&last.valid&&q.valid)boundary.AddLine(REAL(last.x),REAL(last.y),REAL(q.x),REAL(q.y));else boundary.StartFigure();last=q;}Pen glowReach(Color(40,255,191,95),9),edge(amber,2.8f);g.DrawPath(&glowReach,&boundary);g.DrawPath(&edge,&boundary);}
    line(dbGround,floor(at),Color(200,255,191,95),2,true);line(floor(at),cross,riskColor,2,true);
    GraphicsPath segment;ff3qb::Screen last;for(int i=0;i<=18;++i){double t=std::clamp(trace.time-.12+.24*i/18,0.,f.solution.time);auto q=world(ff3qb::point(f.solution,f.gravity,t));if(i&&last.valid&&q.valid)segment.AddLine(REAL(last.x),REAL(last.y),REAL(q.x),REAL(q.y));last=q;}Pen threat(riskColor,5);g.DrawPath(&threat,&segment);
    auto reason=reach<0?L"ABOVE DB REACH":f.risk.intable?L"PROJECTED INTERCEPTION":f.risk.possible?L"TURN / DIVE RISK":L"OUTSIDE DB REACH";
    label(g,cross,reason,L"At "+number(trace.time,2)+L"s  |  gap "+number(f.risk.intable?f.risk.projectedMargin:f.risk.margin,1)+L" st",riskColor,w,h,24,24);
   }
  }
 }
 if(present)surface.present(f.origin);
}
// Full measured flight and forward prediction to the retained field plane.
inline void renderBalls(PumpSurface& surface,const ff3Native::Frame& f,bool present=true,bool clear=true){
 using namespace Gdiplus;int w=f.client.right,h=f.client.bottom;if(!surface.prepare(w,h)){surface.hide();return;}
 Bitmap canvas(surface.width,surface.height,surface.width*4,PixelFormat32bppPARGB,(BYTE*)surface.bits);Graphics g(&canvas);if(clear)g.Clear(Color(0,0,0,0));g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);g.SetClip(Rect(0,0,w,h));
 auto project=[&](ff3qb::Vec p){return ff3qb::project(p,f.matrix,w,h);};
 auto draw=[&](const std::vector<ff3qb::Vec>& path,Color color,float width){Pen shadow(Color(100,8,12,25),width+2),pen(color,width);pen.SetLineJoin(LineJoinRound);pen.SetStartCap(LineCapRound);pen.SetEndCap(LineCapRound);GraphicsPath shape;for(size_t i=1;i<path.size();i++){auto a=project(path[i-1]),b=project(path[i]);if(!a.valid||!b.valid){shape.StartFigure();continue;}shape.AddLine(REAL(a.x),REAL(a.y),REAL(b.x),REAL(b.y));}g.DrawPath(&shadow,&shape);g.DrawPath(&pen,&shape);};
 for(const auto& ball:f.balls){auto prediction=ff3qb::forecast(ball,f.groundY);if(!prediction.valid)continue;
  const bool own=ball.id&&ball.id==f.throwView.ball;
  draw(prediction.path,Color(own?65:28,144,219,245),own?13.f:8.f);draw(prediction.path,Color(own?255:200,165,229,248),own?4.f:2.f);
  if(own){auto current=project(ball.position);if(current.valid){SolidBrush dot(Color(255,165,239,255));g.FillEllipse(&dot,REAL(current.x-5),REAL(current.y-5),10.f,10.f);}label(g,current,L"YOUR BALL",L"Measured flight",Color(255,165,239,255),w,h,20,-45);}
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
inline ff3qb::Solution projectedThrow(const ff3Native::Frame& f,double time){return ff3Native::predictedThrow(f,time);}
// One upload for both QB and ball geometry, with work-aware 144 Hz pacing.
// High-resolution waitable timers avoid a fixed Sleep(16) after each raster.
struct FramePacer {
 HANDLE timer=CreateWaitableTimerExW(nullptr,nullptr,0x00000002,TIMER_ALL_ACCESS);
 ~FramePacer(){if(timer)CloseHandle(timer);}
 void wait(double started){double remaining=1./144-(now()-started);if(remaining<=0){SwitchToThread();return;}LARGE_INTEGER due;due.QuadPart=-std::max<LONGLONG>(1,LONGLONG(remaining*10000000));if(timer&&SetWaitableTimer(timer,&due,0,nullptr,nullptr,FALSE))WaitForSingleObject(timer,20);else Sleep(DWORD(std::max(1.,remaining*1000)));}
};
// Bridge a brief invalid search result without flashing the surface. This
// cache is presentation-only and is never passed to the input/throw helper.
struct PreviewPlan {
 ff3Native::Frame retained;double validAt=0;
 void clear(){retained={};validAt=0;}
 ff3Native::Frame select(const ff3Native::Frame& live,double time){
  if(!live.held||!live.tool||live.textFocus||live.pid!=retained.pid||live.model!=retained.model||live.own!=retained.own||live.tool!=retained.tool||live.revision!=retained.revision||time<validAt)clear();
  if(live.ready&&live.solution.valid&&live.target){retained=live;validAt=time;return live;}
  if(retained.ready&&live.target==retained.target&&time-validAt<=.18){auto display=retained;display.balls=live.balls;display.cameraMatrix=live.cameraMatrix;display.client=live.client;display.origin=live.origin;return display;}
  return live;
 }
};
// Freeze the clicked plan in world coordinates. Never attach it to the QB's
// new head position or silently turn a release prediction into a measured arc.
inline bool releasedFrame(const ff3Native::Frame& live,ff3Native::Frame& display,double time){
 const auto& saved=live.releaseVisual;
 if(!saved||live.held||live.throwView.phase==ff3qb::ThrowView::Hidden||live.pid!=saved->pid||live.model!=saved->model||live.own!=saved->own||live.revision!=saved->revision||time<saved->at||time-saved->at>15)return false;
 display=*saved;display.releaseVisual.reset();display.afterRelease=true;display.balls=live.balls;display.throwView=live.throwView;display.cameraMatrix=live.cameraMatrix;display.client=live.client;display.origin=live.origin;std::copy(std::begin(live.matrix),std::end(live.matrix),display.matrix);return true;
}
inline void worker(){
 using namespace Gdiplus;ULONG_PTR token=0;GdiplusStartupInput startup;if(GdiplusStartup(&token,&startup,nullptr)!=Ok)return;PumpSurface overlay;ViewReader view;FramePacer pacer;PreviewPlan preview;double epoch=now();unsigned frames=0;
 while(running){
  const double started=now();MSG message{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}
  // Presentation may bridge a slow planner update; click eligibility retains
  // its separate 100 ms limit. Reproject the retained geometry with live camera.
  auto f=ff3Native::snapshot();bool live=(ff3Native::enabled||ff3Native::ballTrails)&&ff3Native::admitted()&&ff3Native::focused(f.pid)&&now()>=f.at&&now()-f.at<.30;
  if(!live){overlay.hide();preview.clear();ff3Native::renderHz=0;epoch=now();frames=0;Sleep(30);continue;}
  ff3Native::Frame released;if(ff3Native::enabled&&ff3Native::visuals&&releasedFrame(f,released,now())){f=std::move(released);preview.clear();}else f=preview.select(f,now());
  if(f.client.right>8000||f.client.bottom>5000||!view.refresh(f)){overlay.hide();Sleep(16);continue;}
  for(auto& ball:f.balls)if(ball.model.valid)ball.at=now();
  if(f.ready&&!f.afterRelease){if(f.headPrimitive){auto head=ff3Native::vector(rd<shooting::Vec>(view.handle,f.headPrimitive+Offsets::Primitive::Position,{NAN,NAN,NAN}));if(ff3qb::finite(head))f.head=head;}auto predicted=projectedThrow(f,now());if(predicted.valid)f.solution=predicted;else f.ready=false;}
  const bool field=ff3Native::enabled&&ff3Native::visuals&&f.ready&&f.target&&!f.textFocus&&f.revision==ff3Native::revision;
  if(!ff3Native::ballTrails)f.balls.erase(std::remove_if(f.balls.begin(),f.balls.end(),[&](const auto& ball){return !f.afterRelease||ball.id!=f.throwView.ball;}),f.balls.end());
  const bool balls=(ff3Native::ballTrails||f.afterRelease)&&std::isfinite(f.groundY)&&std::any_of(f.balls.begin(),f.balls.end(),[](const auto& ball){return ball.model.valid;});
  if(!field&&!balls){overlay.hide();ff3Native::renderHz=0;epoch=now();frames=0;Sleep(16);continue;}
  if(field)renderField(overlay,f,false);
  if(balls)renderBalls(overlay,f,false,!field);
  if(field||balls)overlay.present(f.origin);else overlay.hide();
  ff3Native::renderMs=(now()-started)*1000;++frames;if(now()-epoch>=.5){ff3Native::renderHz=frames/(now()-epoch);frames=0;epoch=now();}
  pacer.wait(started);
 }
 overlay.close();GdiplusShutdown(token);
}
}
