#pragma once
#include "ff3_qb_runtime.hpp"
#include "pump_overlay.hpp"
#include "ff3_oob_visual.hpp"
#include "ff3_mouse_aim_visual.hpp"

namespace ff3Visual {
inline std::function<bool(PumpSurface&,const ff3Native::Frame&,HANDLE,bool)> extraRenderer;
inline Gdiplus::Color accent(const ff3Native::Frame& f){switch(ff3qb::throwCheck(f.hudCoverageKnown,f.hudReachEstimated,f.risk)){case ff3qb::ThrowCheck::Clear:return Gdiplus::Color(255,115,232,186);case ff3qb::ThrowCheck::Tight:return Gdiplus::Color(255,255,191,115);case ff3qb::ThrowCheck::Intercept:return Gdiplus::Color(255,255,125,143);default:return Gdiplus::Color(255,151,160,180);}}

inline const wchar_t* clearanceLabel(const ff3Native::Frame& f){
 return ff3qb::throwCheckLabel(ff3qb::throwCheck(f.hudCoverageKnown,f.hudReachEstimated,f.risk));
}
inline void renderField(PumpSurface& surface,const ff3Native::Frame& f,bool present=true,const std::vector<ff3qb::Vec>* eased=nullptr,bool clear=true){
 using namespace Gdiplus;const int w=f.client.right,h=f.client.bottom;if(!surface.prepare(w,h)){surface.hide();return;}
 Bitmap canvas(surface.width,surface.height,surface.width*4,PixelFormat32bppPARGB,(BYTE*)surface.bits);Graphics g(&canvas);if(clear)g.Clear(Color(0,0,0,0));g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);g.SetClip(Rect(0,0,w,h));
 const auto riskColor=accent(f);const Color violet(255,156,196,255),mint(255,105,244,186),amber(255,255,191,95);const double ground=f.groundY+.08;
 auto world=[&](ff3qb::Vec p){return ff3qb::project(p,f.matrix,w,h);};auto floor=[&](ff3qb::Vec p){p.y=ground;return world(p);};
 auto line=[&](ff3qb::Screen a,ff3qb::Screen b,Color c,float width,bool dash=false){if(!a.valid||!b.valid)return;Pen shadow(Color(170,7,11,19),width+2),pen(c,width);pen.SetStartCap(LineCapRound);pen.SetEndCap(LineCapRound);if(dash)pen.SetDashStyle(DashStyleDash);g.DrawLine(&shadow,REAL(a.x),REAL(a.y),REAL(b.x),REAL(b.y));g.DrawLine(&pen,REAL(a.x),REAL(a.y),REAL(b.x),REAL(b.y));};
 auto ring=[&](ff3qb::Vec center,double radius,Color c,float width,bool fill=false){GraphicsPath path;ff3qb::Screen last;bool complete=true;for(int i=0;i<=64;++i){double a=i*6.28318530718/64;auto q=floor(center+ff3qb::Vec{radius*std::cos(a),0,radius*std::sin(a)});if(!q.valid)complete=false;if(i&&last.valid&&q.valid)path.AddLine(REAL(last.x),REAL(last.y),REAL(q.x),REAL(q.y));else path.StartFigure();last=q;}if(fill&&complete){path.CloseFigure();PathGradientBrush brush(&path);brush.SetCenterColor(Color(65,c.GetR(),c.GetG(),c.GetB()));Color edge(5,c.GetR(),c.GetG(),c.GetB());int count=1;brush.SetSurroundColors(&edge,&count);g.FillPath(&brush,&path);}Pen shadow(Color(75,7,11,19),width+1.5f),pen(c,width);g.DrawPath(&shadow,&path);g.DrawPath(&pen,&path);};
 auto arrow=[&](ff3qb::Screen a,ff3qb::Screen b,Color c){line(a,b,c,3);if(!a.valid||!b.valid)return;double dx=b.x-a.x,dy=b.y-a.y,len=std::hypot(dx,dy);if(len<5)return;dx/=len;dy/=len;line(b,{b.x-dx*11+dy*5,b.y-dy*11-dx*5,true},c,3);line(b,{b.x-dx*11-dy*5,b.y-dy*11+dx*5,true},c,3);};
 auto tag=[&](ff3qb::Vec point,const std::wstring& text,Color color){const auto at=world(point);if(!at.valid)return;Font font(L"Segoe UI",11,FontStyleBold,UnitPixel);RectF size;g.MeasureString(text.c_str(),-1,&font,PointF(0,0),&size);const float x=float(std::clamp(at.x-size.Width*.5,5.,std::max(5.,w-size.Width-5.))),y=float(std::clamp(at.y-20,5.,std::max(5.,h-size.Height-5.)));SolidBrush bg(Color(215,11,21,31)),fg(color);g.FillRectangle(&bg,x-6,y-2,size.Width+12,size.Height+4);g.DrawString(text.c_str(),-1,&font,PointF(x,y),&fg);};
 if(!f.afterRelease&&ff3Native::showReceivers){auto receivers=f.receiverVisuals;std::stable_sort(receivers.begin(),receivers.end(),[&](const auto& a,const auto& b){if((a.rig==f.target)!=(b.rig==f.target))return a.rig==f.target;if(a.known!=b.known)return a.known;return a.space>b.space;});if(receivers.size()>5)receivers.resize(5);for(const auto& wr:receivers){const auto p=wr.position+wr.velocity*std::clamp(now()-f.at,0.,.12);const auto color=!wr.known?Color(225,158,172,189):wr.space>=10?Color(235,110,234,193):wr.space>=5?Color(235,255,198,126):Color(235,255,142,159);if(std::isfinite(ground))ring(p,wr.rig==f.target?2.:1.3,color,wr.rig==f.target?2.f:1.4f);wchar_t space[80];if(wr.known)swprintf_s(space,wr.space>=10?L"  OPEN · %.0f st":L"  SPACE %.0f st",wr.space);else wcscpy_s(space,L"  SPACE UNKNOWN");auto name=basketballUi::widen(wr.name.substr(0,18));tag(p+ff3qb::Vec{0,2.1,0},name+space,color);if(ff3Native::showRoutes&&ff3qb::length(wr.velocity)>2){const auto to=p+wr.velocity*.5;arrow(floor(p),floor(to),Color(155,color.GetR(),color.GetG(),color.GetB()));}}}
 if(!f.afterRelease&&f.ready){const auto point=f.solution.catchPoint;if(ff3Native::showWindow){auto direction=ff3qb::Vec{f.targetVelocity.x,0,f.targetVelocity.z};const double speed=ff3qb::length(direction);if(speed>.001)direction=direction*(1/speed);if(ff3qb::length(direction)<.5)direction={1,0,0};const ff3qb::Vec right{-direction.z,0,direction.x};const double along=std::clamp(ff3qb::length(f.targetVelocity)*.08+(f.receiverReachKnown?f.receiverReach.radius:1.2),1.2,4.),across=std::clamp(f.receiverReachKnown?f.receiverReach.radius:1.2,.7,3.);ff3qb::Screen last;for(int i=0;i<=48;++i){const double a=i*6.28318530718/48;const auto q=world(point+direction*(std::cos(a)*along)+right*(std::sin(a)*across));if(i)line(last,q,Color(210,132,222,255),1.8f);last=q;}}
  if(ff3Native::showRace){std::wstring label=L"ARRIVAL ESTIMATE UNAVAILABLE";if(f.hudCoverageKnown){const auto& trace=f.risk.intable?f.risk.projected:f.risk.limit;wchar_t value[100];if(trace.catcher>=0&&trace.time>0)swprintf_s(value,L"EST. CONTACT %.2fs · DB WINDOW %.2fs",f.solution.time,trace.time);else swprintf_s(value,L"EST. CONTACT %.2fs · NO DB WINDOW",f.solution.time);label=value;}tag(point+ff3qb::Vec{0,2,0},label,riskColor);}
 }
 // A compact body marker distinguishes the actual selected WR from the catch
 // point, even while the planner is searching. No extra text/card is needed.
 if(ff3Native::visuals&&f.target&&!f.afterRelease){
  const Color selected=f.locked?Color(255,105,244,186):Color(255,132,215,255);
  if(std::isfinite(ground))ring(f.selectedPosition,1.8,Color(170,selected.GetR(),selected.GetG(),selected.GetB()),1.6f);
  auto top=world(f.selectedPosition+ff3qb::Vec{0,1.5,0}),bottom=world(f.selectedPosition-ff3qb::Vec{0,4.5,0});
  if(top.valid&&bottom.valid){
   double height=std::max(14.,std::abs(bottom.y-top.y)),half=std::clamp(height*.28,8.,42.),cx=(top.x+bottom.x)*.5;
   double y0=std::min(top.y,bottom.y)-3,y1=std::max(top.y,bottom.y)+3,arm=std::clamp(height*.18,6.,14.);
   for(int xsign:{-1,1})for(int ysign:{-1,1}){
    ff3qb::Screen corner{cx+xsign*half,ysign<0?y0:y1,true};
    line(corner,{corner.x-xsign*arm,corner.y,true},selected,2.0f);
    line(corner,{corner.x,corner.y-ysign*arm,true},selected,2.0f);
   }
   PointF diamond[4]={{REAL(cx),REAL(y0-17)},{REAL(cx+5),REAL(y0-11)},{REAL(cx),REAL(y0-5)},{REAL(cx-5),REAL(y0-11)}};
   Pen edge(selected,2);if(f.locked){SolidBrush fill(selected);g.FillPolygon(&fill,diamond,4);}g.DrawPolygon(&edge,diamond,4);
  }
 }
 if(ff3Native::visuals&&f.ready){
  auto flight=ff3qb::groundFlight(f.solution,f.gravity,f.groundY);if(!flight.valid)flight=f.solution;
  auto curvePoint=[&](double u){if(eased&&eased->size()==65){double index=std::clamp(u,0.,1.)*64;size_t i=std::min<size_t>(63,size_t(index));return (*eased)[i]+((*eased)[i+1]-(*eased)[i])*(index-i);}return ff3qb::point(f.solution,f.gravity,f.solution.time*u);};
  GraphicsPath beam;ff3qb::Screen last;for(int i=0;i<=96;++i){auto q=world(curvePoint(i/96.));if(i&&last.valid&&q.valid)beam.AddLine(REAL(last.x),REAL(last.y),REAL(q.x),REAL(q.y));else beam.StartFigure();last=q;}
  Pen glow(Color(f.afterRelease?20:52,riskColor.GetR(),riskColor.GetG(),riskColor.GetB()),8),shadow(Color(140,8,11,21),4.2f),core(Color(f.afterRelease?155:248,riskColor.GetR(),riskColor.GetG(),riskColor.GetB()),f.afterRelease?1.7f:2.5f);core.SetLineJoin(LineJoinRound);core.SetStartCap(LineCapRound);core.SetEndCap(LineCapRound);if(f.afterRelease)core.SetDashStyle(DashStyleDash);g.DrawPath(&glow,&beam);g.DrawPath(&shadow,&beam);g.DrawPath(&core,&beam);
  auto catchSpot=curvePoint(1);auto catchScreen=world(catchSpot),catchGround=floor(catchSpot);
  // A missed-catch continuation has a different weight from the intended pass.
  if(flight.time>f.solution.time){GraphicsPath tail;ff3qb::Screen prev;for(int i=0;i<=40;++i){double u=i/40.;auto p=ff3qb::point(flight,f.gravity,f.solution.time+(flight.time-f.solution.time)*u)+(catchSpot-f.solution.catchPoint)*(1-u);auto q=world(p);if(i&&prev.valid&&q.valid)tail.AddLine(REAL(prev.x),REAL(prev.y),REAL(q.x),REAL(q.y));prev=q;}Pen fade(Color(90,156,196,255),1.3f);fade.SetDashStyle(DashStyleDash);g.DrawPath(&fade,&tail);}
  for(double u:{.25,.5,.75}){auto q=world(curvePoint(u));if(!q.valid)continue;SolidBrush dot(Color(235,223,250,255));g.FillEllipse(&dot,REAL(q.x-2),REAL(q.y-2),4.f,4.f);}
  if(std::isfinite(ground)){
   ring(catchSpot,2.5,mint,2.2f,true);ring(catchSpot,3.2,Color(100,105,244,186),1.f);line(catchGround,catchScreen,Color(150,105,244,186),1.2f,true);
   ring(flight.catchPoint,2.2,Color(170,192,178,255),1.5f);auto landing=floor(flight.catchPoint);
  }
  if(catchScreen.valid){Pen halo(Color(65,105,244,186),5),marker(mint,2);g.DrawEllipse(&halo,REAL(catchScreen.x-7),REAL(catchScreen.y-7),14.f,14.f);g.DrawEllipse(&marker,REAL(catchScreen.x-7),REAL(catchScreen.y-7),14.f,14.f);SolidBrush center(Color(245,225,255,245));g.FillEllipse(&center,REAL(catchScreen.x-2),REAL(catchScreen.y-2),4.f,4.f);}
  if(!f.afterRelease){
   const auto label=clearanceLabel(f);const bool clear=std::wstring(label)==L"CLEAR";
   Color cue=riskColor;
   // Always visible even when the intended catch is off screen. This cue
   // follows Aim beam independently of the optional bottom Throw HUD.
   const float x=REAL(std::clamp(catchScreen.valid?catchScreen.x-57.:w*.5-57.,3.,std::max(3.,double(w)-117))),y=REAL(std::clamp(catchScreen.valid?catchScreen.y-38.:h*.72,3.,std::max(3.,double(h)-27)));
   GraphicsPath badge;badge.AddArc(x,y,12.f,12.f,180.f,90.f);badge.AddArc(x+102,y,12.f,12.f,270.f,90.f);badge.AddArc(x+102,y+12,12.f,12.f,0.f,90.f);badge.AddArc(x,y+12,12.f,12.f,90.f,90.f);badge.CloseFigure();
   SolidBrush bg(Color(224,12,22,29)),ink(cue);Pen border(Color(120,cue.GetR(),cue.GetG(),cue.GetB()),1);g.FillPath(&bg,&badge);g.DrawPath(&border,&badge);
   if(clear){Pen tick(cue,1.8f);g.DrawLine(&tick,x+9,y+12,x+12,y+15);g.DrawLine(&tick,x+12,y+15,x+18,y+8);}else g.FillEllipse(&ink,x+11,y+9,5.f,5.f);
   Font font(L"Segoe UI",11,FontStyleBold,UnitPixel);StringFormat format;format.SetLineAlignment(StringAlignmentCenter);g.DrawString(label,-1,&font,RectF(x+24,y,88,24),&format,&ink);
  }
  if(ff3Native::showThreats&&std::isfinite(ground)){
   auto baseline=catchSpot-f.fitOffset;auto from=floor(baseline);
   if(ff3qb::length(f.fitOffset)>.5){ring(baseline,1.4,Color(200,180,190,212),1.5f);arrow(from,catchGround,violet);}
   if(f.receiverReachKnown){auto wr=f.receiverReach.position+f.targetVelocity*(f.solution.time+std::clamp(f.ping*.0005+.05,0.,.25));ring(wr,1.2,Color(210,105,244,186),1.8f);arrow(floor(wr),catchGround,mint);}
   const auto& trace=f.risk.intable?f.risk.projected:f.risk.limit;
   if(trace.catcher>=0&&trace.catcher<int(f.hudDefenders.size())){
    const auto& db=f.hudDefenders[trace.catcher];auto at=ff3qb::point(f.solution,f.gravity,trace.time);double horizon=trace.time+std::clamp(f.ping*.0005+.05,0.,.25);double reach=ff3qb::reachSectionRadius(db,at.y,horizon);
    const bool measured=trace.catcher<int(f.hudShapeMeasured.size())&&f.hudShapeMeasured[trace.catcher];
    auto dbGround=floor(db.position),cross=world(at);ring(db.position,db.radius,Color(105,255,191,95),1.3f,false);
    // Compact clearance cue at the critical time, without giant world cages.
    auto dbTop=world(db.position+ff3qb::Vec{0,db.above+ff3qb::jumpRise(db.jumpHeight,db.worldGravity,horizon,db.velocity.y),0});
    auto dbBase=world(db.position-ff3qb::Vec{0,db.below,0});line(dbBase,dbTop,Color(135,255,191,95),1.5f,!measured);
    if(reach>=0){double direction=std::atan2(at.z-db.position.z,at.x-db.position.x);auto edgePoint=db.position+ff3qb::Vec{std::cos(direction)*reach,0,std::sin(direction)*reach};auto delta=edgePoint-at;delta.y=0;auto nearEdge=at+delta*std::min(1.,8./std::max(.001,ff3qb::length(delta)));line(floor(nearEdge),floor(at),riskColor,1.6f,true);}
    line(floor(at),cross,Color(125,riskColor.GetR(),riskColor.GetG(),riskColor.GetB()),1.2f,true);
    GraphicsPath segment;ff3qb::Screen last;for(int i=0;i<=18;++i){double t=std::clamp(trace.time-.12+.24*i/18,0.,flight.time);auto visible=t<=f.solution.time?curvePoint(t/std::max(.001,f.solution.time)):ff3qb::point(f.solution,f.gravity,t)+(catchSpot-f.solution.catchPoint)*std::clamp(1-(t-f.solution.time)/std::max(.001,flight.time-f.solution.time),0.,1.);auto q=world(visible);if(i&&last.valid&&q.valid)segment.AddLine(REAL(last.x),REAL(last.y),REAL(q.x),REAL(q.y));last=q;}Pen threat(riskColor,3.5f);g.DrawPath(&threat,&segment);

   }
  }
 }
 if(!ff3Native::visuals&&ff3Native::showThreats&&f.ready&&!f.afterRelease){const auto& trace=f.risk.intable?f.risk.projected:f.risk.limit;if(trace.catcher>=0&&trace.catcher<int(f.hudDefenders.size())){const auto& db=f.hudDefenders[trace.catcher];ring(db.position,std::clamp(db.radius,1.,4.),Color(205,255,191,115),1.7f);tag(db.position+ff3qb::Vec{0,3,0},L"LIMITING DEFENDER · ESTIMATE",riskColor);ff3qb::Screen last;for(int i=0;i<=12;++i){auto q=world(ff3qb::point(f.solution,f.gravity,std::clamp(trace.time-.1+.2*i/12,0.,f.solution.time)));if(i)line(last,q,riskColor,3);last=q;}}}
 drawMouseAim(g,f);
 if(present)surface.present(f.origin);
}
inline bool renderBoundary(PumpSurface& surface,const ff3Native::Frame& view,const ff3OobVisual::Scene& scene){
 using namespace Gdiplus;const int w=view.client.right,h=view.client.bottom;if(!surface.prepare(w,h))return false;
 Bitmap canvas(surface.width,surface.height,surface.width*4,PixelFormat32bppPARGB,(BYTE*)surface.bits);Graphics g(&canvas);g.Clear(Color(0,0,0,0));g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);g.SetClip(Rect(0,0,w,h));
 return ff3OobVisual::draw(g,scene,view.matrix,w,h);
}
// Full measured flight and forward prediction to the retained field plane.
inline void renderBalls(PumpSurface& surface,const ff3Native::Frame& f,bool present=true,bool clear=true){
 using namespace Gdiplus;int w=f.client.right,h=f.client.bottom;if(!surface.prepare(w,h)){surface.hide();return;}
 Bitmap canvas(surface.width,surface.height,surface.width*4,PixelFormat32bppPARGB,(BYTE*)surface.bits);Graphics g(&canvas);if(clear)g.Clear(Color(0,0,0,0));g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);g.SetClip(Rect(0,0,w,h));
 auto project=[&](ff3qb::Vec p){return ff3qb::project(p,f.matrix,w,h);};
 auto draw=[&](const std::vector<ff3qb::Vec>& path,Color color,float width){Pen shadow(Color(100,8,12,25),width+2),pen(color,width);pen.SetLineJoin(LineJoinRound);pen.SetStartCap(LineCapRound);pen.SetEndCap(LineCapRound);GraphicsPath shape;for(size_t i=1;i<path.size();i++){auto a=project(path[i-1]),b=project(path[i]);if(!a.valid||!b.valid){shape.StartFigure();continue;}shape.AddLine(REAL(a.x),REAL(a.y),REAL(b.x),REAL(b.y));}g.DrawPath(&shadow,&shape);g.DrawPath(&pen,&shape);};
 for(const auto& ball:f.balls){auto prediction=ff3qb::forecast(ball,f.groundY);if(!prediction.valid){if(ball.trail.size()>1)draw(ball.trail,Color(180,157,220,240),1.8f);continue;}
  const bool own=ball.id&&ball.id==f.throwView.ball;
  draw(prediction.path,Color(own?65:28,144,219,245),own?13.f:8.f);draw(prediction.path,Color(own?255:200,165,229,248),own?4.f:2.f);
  if(ff3Native::ballHighlights){auto current=project(ball.position);if(current.valid){SolidBrush dot(Color(255,165,239,255));g.FillEllipse(&dot,REAL(current.x-5),REAL(current.y-5),10.f,10.f);}}
  if(ff3Native::futureMarkers){auto future=project((ball.position+ball.velocity*.25+ff3qb::Vec{0,-.5*ball.gravity*.25*.25,0}));if(future.valid){Pen mark(Color(220,255,207,135),1.8f);g.DrawEllipse(&mark,REAL(future.x-6),REAL(future.y-6),12.f,12.f);}}
  if(ff3Native::landingMarkers){std::vector<ff3qb::Vec> ring;for(int i=0;i<=64;i++){double a=i*6.283185307179586/64;ring.push_back({prediction.landing.x+std::cos(a)*2.5,prediction.landing.y-ball.radius+.08,prediction.landing.z+std::sin(a)*2.5});}draw(ring,Color(235,208,212,255),2);}
 }
 if(present)surface.present(f.origin);
}
// Loose-ball markers do not depend on a fitted trajectory. Floor balls remain
// visible after the flight tracker finishes; current positions are read at render rate.
inline bool renderLooseBalls(PumpSurface& surface,const ff3Native::Frame& f,HANDLE handle=nullptr,bool clear=true){
 using namespace Gdiplus;const int w=f.client.right,h=f.client.bottom;if(f.looseBalls.empty()||!surface.prepare(w,h))return false;
 Bitmap canvas(surface.width,surface.height,surface.width*4,PixelFormat32bppPARGB,(BYTE*)surface.bits);Graphics g(&canvas);if(clear)g.Clear(Color(0,0,0,0));g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);g.SetClip(Rect(0,0,w,h));
 struct Visible{ff3qb::Vec p;ff3qb::Screen screen;double distance;bool floor;};std::vector<Visible> visible;
 for(const auto& ball:f.looseBalls){const auto p=handle?ff3Native::position(handle,ball.id):ball.position;auto screen=ff3qb::project(p,f.matrix,w,h);if(!screen.valid)continue;visible.push_back({p,screen,ff3qb::finite(f.head)?ff3qb::length(p-f.head):INFINITY,std::isfinite(f.groundY)&&p.y<=f.groundY+1.35});}
 std::stable_sort(visible.begin(),visible.end(),[](const auto& a,const auto& b){return a.distance<b.distance;});
 for(size_t i=0;i<visible.size();++i){const auto& ball=visible[i];const Color color=ball.floor?Color(235,149,230,204):Color(235,158,216,245);SolidBrush dot(color);Pen edge(Color(210,8,15,24),3.2f);const float x=float(ball.screen.x),y=float(ball.screen.y);g.DrawEllipse(&edge,x-3.8f,y-3.8f,7.6f,7.6f);g.FillEllipse(&dot,x-2.5f,y-2.5f,5.f,5.f);
  if(ball.floor){GraphicsPath ring;for(int k=0;k<24;++k){const double a=k*6.283185307179586/24,b=(k+1)*6.283185307179586/24;ff3qb::Screen u,v;if(ff3OobVisual::segment(ball.p+ff3qb::Vec{std::cos(a)*.7,-.2,std::sin(a)*.7},ball.p+ff3qb::Vec{std::cos(b)*.7,-.2,std::sin(b)*.7},f.matrix,w,h,u,v))ring.AddLine(float(u.x),float(u.y),float(v.x),float(v.y));else ring.StartFigure();}Pen outline(Color(150,149,230,204),1.2f);g.DrawPath(&outline,&ring);}
  if(i<3){wchar_t text[48];if(std::isfinite(ball.distance))swprintf_s(text,L"BALL  %.0f st",ball.distance);else wcscpy_s(text,L"BALL");Font font(L"Segoe UI",10,FontStyleBold,UnitPixel);RectF box;g.MeasureString(text,-1,&font,PointF(0,0),&box);const float left=std::clamp(x+9.f,5.f,std::max(5.f,float(w)-box.Width-9)),top=std::clamp(y-9.f,5.f,std::max(5.f,float(h)-box.Height-5));SolidBrush back(Color(175,10,17,27)),ink(color);g.FillRectangle(&back,left-3,top-1,box.Width+6,box.Height+2);g.DrawString(text,-1,&font,PointF(left,top),&ink);}
 }
 return !visible.empty();
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
  if(f.studio)return ff3Native::studioView&&ff3Native::studioView(f);
  if(pid!=f.pid){if(handle)CloseHandle(handle);pid=f.pid;handle=OpenProcess(PROCESS_VM_READ|SYNCHRONIZE,FALSE,pid);}
  if(!handle||!f.cameraMatrix||WaitForSingleObject(handle,0)!=WAIT_TIMEOUT)return false;
  if(!matrix(handle,f.cameraMatrix,f.matrix))return false;
  auto window=GetForegroundWindow();DWORD owner=0;GetWindowThreadProcessId(window,&owner);if(owner!=f.pid)return false;
  POINT origin{};RECT client{};if(!GetClientRect(window,&client)||!ClientToScreen(window,&origin))return false;
  f.client=client;f.origin=origin;GetCursorPos(&f.cursor);
  if(f.camera&&f.workspace){if(rd<addr>(handle,f.workspace+Offsets::Workspace::CurrentCamera)!=f.camera)return false;f.cameraPosition=ff3Native::vector(rd<shooting::Vec>(handle,f.camera+Offsets::Camera::Position,{NAN,NAN,NAN}));if(!ff3qb::finite(f.cameraPosition))return false;}
  return true;
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
// Visual Aim may bridge a presentation gap. Camera Aim must only show the
// backend's clickable plan, with the same freshness window as its input gate.
struct PreviewPlan {
 ff3Native::Frame retained;double validAt=0;
 void clear(){retained={};validAt=0;}
 ff3Native::Frame select(const ff3Native::Frame& live,double time){
  // Camera Aim must draw the same plan the click handler can commit. A
  // renderer-only cache used to show an armed beam over an unready backend.
  if(ff3Native::wheelMethod()&&!ff3Native::cursorOnly){clear();auto display=live;if(!live.committed&&(time<live.at||time-live.at>=.1))display.ready=false;return display;}
  if(ff3Native::wheelMethod()&&!ff3Native::cursorOnly){clear();auto display=live;if(!display.committed&&(time<display.at||time-display.at>=.1))display.ready=false;return display;}
  if(!live.held||!live.tool||live.textFocus||live.pid!=retained.pid||live.model!=retained.model||live.own!=retained.own||live.tool!=retained.tool||live.revision!=retained.revision||time<validAt)clear();
  if(live.ready&&live.solution.valid&&live.target){retained=live;validAt=time;return live;}
  if(retained.ready&&live.target==retained.target&&time-validAt<=.18){auto display=retained;display.balls=live.balls;display.looseBalls=live.looseBalls;display.discoveredBalls=live.discoveredBalls;display.cameraMatrix=live.cameraMatrix;display.client=live.client;display.origin=live.origin;return display;}
  return live;
 }
};
// Freeze the clicked plan in world coordinates. Never attach it to the QB's
// new head position or silently turn a release prediction into a measured arc.
inline bool releasedFrame(const ff3Native::Frame& live,ff3Native::Frame& display,double time,const std::shared_ptr<ff3Native::Frame>& retained={}){
 const auto& saved=live.releaseVisual?live.releaseVisual:retained;
 if(!saved||live.held||live.pid!=saved->pid||live.model!=saved->model||live.own!=saved->own||time<saved->at||time-saved->at>15)return false;
 display=*saved;display.releaseVisual.reset();display.afterRelease=true;display.balls=live.balls;display.looseBalls=live.looseBalls;display.discoveredBalls=live.discoveredBalls;display.throwView=live.throwView;display.cameraMatrix=live.cameraMatrix;display.client=live.client;display.origin=live.origin;std::copy(std::begin(live.matrix),std::end(live.matrix),display.matrix);return true;
}
struct ReleaseBeam {
 std::shared_ptr<ff3Native::Frame> saved,finished;std::uint64_t ball=0;double landingAt=0;
 void clear(){saved.reset();ball=0;landingAt=0;}
 void finish(){finished=saved;clear();}
 bool select(const ff3Native::Frame& live,ff3Native::Frame& display,double time){
  if(live.releaseVisual&&live.releaseVisual==finished)return false;
  if(live.releaseVisual&&live.releaseVisual!=saved){finished.reset();saved=live.releaseVisual;ball=0;auto ground=ff3qb::groundFlight(saved->solution,saved->gravity,saved->groundY);landingAt=saved->at+(ground.valid?ground.time:15);}
  if(!saved||live.held||live.pid!=saved->pid||live.model!=saved->model||live.own!=saved->own||time<saved->at||time-saved->at>15){clear();return false;}
  const auto& shot=live.throwView;bool sameThrow=shot.releasedAt>=saved->at-.02&&shot.releasedAt<=saved->at+.2;
  if(sameThrow&&shot.ballVerified)ball=shot.ball;
  if(sameThrow&&shot.phase==ff3qb::ThrowView::Done&&shot.result!="Result unavailable"){finish();return false;}
  for(const auto& candidate:live.balls)if(ball&&candidate.id==ball){
   if(candidate.landed||candidate.ended||(candidate.thrower&&candidate.thrower!=live.localPlayer&&candidate.thrower!=live.own)){finish();return false;}
   auto observed=candidate;if(observed.model.valid)observed.at=observed.model.observedTime(observed.position);auto ground=ff3qb::forecast(observed,live.groundY);
   if(ground.valid)landingAt=time+ground.remaining;
  }
  if(time>landingAt+.25){finish();return false;}
  return releasedFrame(live,display,time,saved);
 }
};
inline void worker(){
 using namespace Gdiplus;ULONG_PTR token=0;GdiplusStartupInput startup;if(GdiplusStartup(&token,&startup,nullptr)!=Ok)return;PumpSurface overlay;ViewReader view;FramePacer pacer;PreviewPlan preview;ReleaseBeam releaseBeam;ff3qb::CurveTracker curve;unsigned curveRevision=0;addr curveTool=0,curveModel=0;double epoch=now();unsigned frames=0;
 while(running){
  const double started=now();MSG message{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}
  // Reproject backend geometry with the current camera. Physical previews
  // share click eligibility instead of advertising a renderer-only cache.
  auto f=ff3Native::snapshot();const bool extras=ff3Native::extraVisuals&&ff3Native::extraVisuals();bool live=(ff3Native::enabled||ff3Native::ballTrails||extras)&&ff3Native::admitted()&&ff3Native::focused(f.pid)&&now()>=f.at&&now()-f.at<.6;
  if(!live){overlay.hide();preview.clear();curve.reset();ff3Native::renderHz=0;epoch=now();frames=0;Sleep(30);continue;}
  // Capture current field/settings before a retained throw replaces the display
  // frame. The OOB outline always follows the live slider, even after release.
  const ff3OobVisual::Scene boundaryScene{f.field,ff3Native::boundaryHalfStuds.load()*.5,f.fieldSurfaceY,f.practiceRange,f.menuOpen,f.textFocus};
  const bool markersAllowed=!f.menuOpen&&!f.textFocus;
  const bool boundary=ff3Native::enabled&&ff3Native::oobVisual&&ff3OobVisual::visible(boundaryScene);
  if(!ff3Native::enabled||!ff3Native::visuals)releaseBeam.clear();
  ff3Native::Frame released;if(ff3Native::enabled&&ff3Native::visuals&&releaseBeam.select(f,released,now())){f=std::move(released);preview.clear();}else f=preview.select(f,now());
  if(f.client.right>8000||f.client.bottom>5000||!view.refresh(f)){overlay.hide();Sleep(16);continue;}
  // Forecasts stay anchored to observed progress, not render-clock time.
  if(f.ready&&!f.afterRelease){if(f.headPrimitive&&!f.committed){auto head=ff3Native::vector(rd<shooting::Vec>(view.handle,f.headPrimitive+Offsets::Primitive::Position,{NAN,NAN,NAN}));if(ff3qb::finite(head))f.head=head;}auto predicted=projectedThrow(f,now());if(predicted.valid)f.solution=predicted;else f.ready=false;}
  if(f.selectedPrimitive&&!f.afterRelease){auto selected=ff3Native::vector(rd<shooting::Vec>(view.handle,f.selectedPrimitive+Offsets::Primitive::Position,{NAN,NAN,NAN}));if(ff3qb::finite(selected))f.selectedPosition=selected;}
  const bool field=ff3Native::enabled&&(ff3Native::visualOnly||ff3Native::visuals||ff3Native::showReceivers||ff3Native::showWindow||ff3Native::showRace||ff3Native::showThreats)&&(!f.receiverVisuals.empty()||f.target)&&!f.textFocus&&(f.afterRelease||f.revision==ff3Native::revision);
  if(!ff3Native::ballTrails)f.balls.erase(std::remove_if(f.balls.begin(),f.balls.end(),[&](const auto& ball){return !f.afterRelease||ball.id!=f.throwView.ball;}),f.balls.end());
  const bool balls=(ff3Native::ballTrails||f.afterRelease)&&std::any_of(f.balls.begin(),f.balls.end(),[](const auto& ball){return ball.model.valid||ball.trail.size()>1;});
  const bool markers=ff3Native::ballTrails&&ff3Native::looseMarkers&&markersAllowed&&!f.looseBalls.empty();
  if(!field&&!balls&&!extras&&!boundary&&!markers){overlay.hide();ff3Native::renderHz=0;epoch=now();frames=0;Sleep(16);continue;}
  if(f.afterRelease||f.revision!=curveRevision||f.tool!=curveTool||f.model!=curveModel){curve.reset();curveRevision=f.revision;curveTool=f.tool;curveModel=f.model;}
  const bool physicalPreview=ff3Native::wheelMethod()&&!ff3Native::cursorOnly;
  // Camera Aim stabilizes the physical plan upstream. Render that actual
  // parabola; blending two different arcs advertises an unthrowable curve.
  if(field&&f.ready&&!f.afterRelease&&!physicalPreview)curve.update(f.solution,f.gravity,f.target,now(),f.head);
  else if(physicalPreview||!field||!f.ready)curve.reset();
  const bool boundaryDrawn=boundary&&renderBoundary(overlay,f,boundaryScene);
  if(field)renderField(overlay,f,false,!physicalPreview&&!f.afterRelease&&curve.target==f.target?&curve.points:nullptr,!boundaryDrawn);
  if(balls)renderBalls(overlay,f,false,!field&&!boundaryDrawn);
  const bool looseDrawn=markers&&renderLooseBalls(overlay,f,view.handle,!field&&!balls&&!boundaryDrawn);
  const bool extraDrawn=extras&&extraRenderer&&extraRenderer(overlay,f,view.handle,!field&&!balls&&!boundaryDrawn&&!looseDrawn);
  if(field||balls||looseDrawn||extraDrawn||boundaryDrawn)overlay.present(f.origin);else overlay.hide();
  ff3Native::renderMs=(now()-started)*1000;++frames;if(now()-epoch>=.5){ff3Native::renderHz=frames/(now()-epoch);frames=0;epoch=now();}
  pacer.wait(started);
 }
 overlay.close();GdiplusShutdown(token);
}
}
