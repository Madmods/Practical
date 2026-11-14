#pragma once
#include "flight_logic.hpp"
#include "rolling_logic.hpp"
struct FlightLive {
 HANDLE process=nullptr;DWORD pid=0;addr workspace=0,own=0,ownPrimitive=0,ownHumanoid=0,chosen=0;double scanAt=-10,lastOwnAt=0;shooting::Vec lastOwn{};pump::GroundPlane ground;
 struct Ball{addr model=0,primitive=0,ownerValue=0;float radius=.75f;};std::vector<Ball> balls;std::map<addr,flight::Track> tracks,rollTracks;rolling::Plan rollingPlan,rawRoll;addr chosenRoll=0,solvedBall=0;std::map<addr,rolling::InterceptMotion> interceptMotion;rolling::PlanDisplay display;double solveAt=-10,raceAt=-10;float rivalTime=-1;
 struct Drag{double at=0;float speed=0,value=0;};std::map<addr,Drag> drag;std::map<addr,rolling::BounceMotion> bounceMotion;
 struct Peer{addr primitive,humanoid;};std::vector<Peer> peers;std::map<addr,rolling::MotionSpeed> speeds;
 void close(){if(process)CloseHandle(process);process=nullptr;pid=0;own=0;balls.clear();tracks.clear();rollTracks.clear();drag.clear();bounceMotion.clear();speeds.clear();peers.clear();rollingPlan={};rawRoll={};display={};interceptMotion.clear();solvedBall=0;solveAt=raceAt=-10;ground={};scanAt=-10;chosen=chosenRoll=0;}
 flight::Arc sample(float matrix[16]){
  rollingPlan={};
  DWORD current=gamePid.load();if(current!=pid){close();pid=current;process=pid?OpenProcess(PROCESS_VM_READ,FALSE,pid):nullptr;}if(!process)return {};
  double t=now();addr model=sceneOwn.load(),root=sceneRoot.load(),camera=sceneVisualEngine.load();if(!model||!root||!camera)return {};
  if(t-scanAt>.15||model!=own)workspace=child(process,root,"Workspace");addr ws=workspace;
  if(t-scanAt>.15||model!=own){scanAt=t;if(model!=own){own=model;ground={};lastOwnAt=0;rawRoll={};display={};interceptMotion.clear();solvedBall=0;solveAt=raceAt=-10;tracks.clear();rollTracks.clear();drag.clear();bounceMotion.clear();speeds.clear();chosen=chosenRoll=0;}ownPrimitive=rd<addr>(process,child(process,own,"HumanoidRootPart")+Offsets::BasePart::Primitive);ownHumanoid=child(process,own,"Humanoid");balls.clear();
   for(auto b:children(process,child(process,ws,"Basketballs"))){if(balls.size()>=32)break;addr primitive=rd<addr>(process,b+Offsets::BasePart::Primitive);if(!primitive)continue;auto size=rd<shooting::Vec>(process,primitive+Offsets::Primitive::Size);float r=std::clamp(size.y*.5f,.15f,2.f);balls.push_back({b,primitive,path(process,b,{"Attributes","Character"}),r});}
   peers.clear();for(auto a:children(process,child(process,ws,"Characters"))){if(a==own||nm(process,a)=="InvisCharacter")continue;auto hum=child(process,a,"Humanoid");if(!hum||rd<float>(process,hum+Offsets::Humanoid::Health)<=0)continue;addr primitive=rd<addr>(process,child(process,a,"HumanoidRootPart")+Offsets::BasePart::Primitive);if(primitive)peers.push_back({primitive,hum});if(peers.size()>=24)break;}
   for(auto it=rollTracks.begin();it!=rollTracks.end();)if(std::none_of(balls.begin(),balls.end(),[&](auto&b){return b.model==it->first;})){drag.erase(it->first);bounceMotion.erase(it->first);interceptMotion.erase(it->first);it=rollTracks.erase(it);}else ++it;
   for(auto it=tracks.begin();it!=tracks.end();)if(std::none_of(balls.begin(),balls.end(),[&](auto&b){return b.model==it->first;}))it=tracks.erase(it);else ++it;
  }
  auto pos=rd<shooting::Vec>(process,ownPrimitive+Offsets::Primitive::Position,{NAN,NAN,NAN});auto size=rd<shooting::Vec>(process,ownPrimitive+Offsets::Primitive::Size);float hip=rd<float>(process,ownHumanoid+Offsets::Humanoid::HipHeight,NAN);
  if(!std::isfinite(pos.y)||!std::isfinite(hip)||hip<.1f||hip>10||size.y<.1f||size.y>10)return {};
  float vy=0;if(lastOwnAt>0&&t-lastOwnAt>.001&&t-lastOwnAt<.2)vy=(pos.y-lastOwn.y)/(float)(t-lastOwnAt);lastOwn=pos;lastOwnAt=t;
  float floor=ground.update(pos.y-hip-size.y*.5f,vy,t);float gravity=rd<float>(process,ws+Offsets::Workspace::ReadOnlyGravity,NAN);if(!std::isfinite(gravity)||gravity<1||gravity>300)return {};
  flight::Arc best;float score=1e9f,rollScore=1e9f;addr selected=0,selectedRoll=0;rolling::Ball selectedBall;bool selectedReady=false;
  auto runSpeed=[&](addr hum,shooting::Vec position){float v=rd<float>(process,hum+Offsets::Humanoid::Walkspeed,0);return speeds[hum].observe(position,t,v);};
  rolling::Runner runner{pos,runSpeed(ownHumanoid,pos),std::clamp(hip+size.y*.5f+.5f,2.5f,6.f)};
  std::vector<rolling::Runner> otherRunners;if(rollingOn)for(auto&r:peers){auto q=rd<shooting::Vec>(process,r.primitive+Offsets::Primitive::Position,{NAN,NAN,NAN});if(std::isfinite(q.y)&&fabs(q.y-pos.y)<10&&shooting::length(q-pos)<80)otherRunners.push_back({q,runSpeed(r.humanoid,q)});}
  for(auto&b:balls){auto&p=tracks[b.model];if(b.ownerValue&&rd<addr>(process,b.ownerValue+Offsets::Misc::Value)){p.reset();rollTracks[b.model].reset();drag.erase(b.model);bounceMotion.erase(b.model);interceptMotion.erase(b.model);continue;}
   auto ballPos=rd<shooting::Vec>(process,b.primitive+Offsets::Primitive::Position,{NAN,NAN,NAN});float distance=shooting::length(ballPos-pos);if(!std::isfinite(distance)||distance>120){p.reset();rollTracks[b.model].reset();continue;}
   auto&motion=bounceMotion[b.model];if(motion.update(t,ballPos,gravity,floor,b.radius)){p.reset();rollTracks[b.model].reset();drag.erase(b.model);}
   auto estimate=p.add(t,ballPos,gravity);
   if(rollingOn){auto&fit=interceptMotion[b.model];fit.update(t,ballPos,gravity,floor,b.radius);
    if((fit.valid||fit.waiting)&&distance<rollScore){rollScore=distance;selectedBall=fit.ball;selectedRoll=b.model;selectedReady=fit.valid;}
   }
   if(!estimate.valid||t-estimate.at>.1)continue;
   // Rolling balls do not get a misleading airborne landing prediction.
   if(ballPos.y-floor-b.radius<.2f&&estimate.v.y<2)continue;
   auto arc=flight::predict(estimate,floor,b.radius);if(!arc.valid)continue;float rank=distance-(b.model==chosen?15.f:0.f);if(rank<score){score=rank;best=std::move(arc);selected=b.model;}
  }
  chosen=selected;chosenRoll=selectedRoll;rolling::Plan candidate;
  if(selectedRoll&&selectedReady){bool switched=solvedBall!=selectedRoll;
   if(switched||t-solveAt>=1./60){rawRoll=rolling::make(selectedBall,runner,{});solveAt=t;}
   if(switched||t-raceAt>=.1){rivalTime=-1;for(auto&r:otherRunners){auto q=rolling::earliest(selectedBall,r);if(q.valid&&(rivalTime<0||q.time<rivalTime))rivalTime=q.time;}raceAt=t;}
   solvedBall=selectedRoll;candidate=rawRoll;candidate.time=std::max(0.f,candidate.time-(float)(t-solveAt));candidate.otherTime=rivalTime<0?-1:std::max(0.f,rivalTime-(float)(t-raceAt));
  }
  rollingPlan=display.update(selectedRoll,candidate,t,pos);
  if(rollingPlan.valid){rollingPlan.start.y=rollingPlan.point.y=floor+.04f;for(auto&q:rollingPlan.ballPath)q.y=floor+.04f;}
  if(!best.valid&&!rollingPlan.valid)return {};SIZE_T bytes=0;if(!ReadProcessMemory(process,(void*)(camera+Offsets::VisualEngine::ViewMatrix),matrix,64,&bytes)||bytes!=64){rollingPlan={};return {};}return best;
 }
};
#include "rolling_overlay.hpp"
static FlightLive flightLive;static PumpSurface flightSurface;
static void closeFlightOverlay(){rollingSurface.close();flightSurface.close();flightLive.close();}
static void renderFlightArc(const flight::Arc&arc,const float*m,int w,int h,POINT origin,float dpi){
 using namespace Gdiplus;if(!arc.valid){flightSurface.hide();return;}
 if(!pumpGdi){GdiplusStartupInput in;if(GdiplusStartup(&pumpGdi,&in,nullptr)!=Ok)return;}
 auto project=[&](shooting::Vec p,PointF&out){float z=m[12]*p.x+m[13]*p.y+m[14]*p.z+m[15];if(!std::isfinite(z)||z<.01f)return false;float x=(m[0]*p.x+m[1]*p.y+m[2]*p.z+m[3])/z,y=(m[4]*p.x+m[5]*p.y+m[6]*p.z+m[7])/z;if(!std::isfinite(x)||!std::isfinite(y)||fabs(x)>1.5||fabs(y)>1.5)return false;out={(x+1)*w*.5f,(1-y)*h*.5f};return true;};
 std::vector<std::vector<PointF>> paths;paths.emplace_back();float left=(float)w,top=(float)h,right=0,bottom=0;
 auto bound=[&](PointF p){left=std::min(left,p.X);right=std::max(right,p.X);top=std::min(top,p.Y);bottom=std::max(bottom,p.Y);};
 for(auto p:arc.points){PointF q;if(project(p,q)){paths.back().push_back(q);bound(q);}else if(!paths.back().empty())paths.emplace_back();}
 std::vector<PointF> ring,inner,countdown;std::vector<std::vector<PointF>> accents;PointF landing;bool marker=ballLandingOn&&project(arc.landing,landing);
 auto circle=[&](float radius,std::vector<PointF>&out){for(int i=0;i<=72;i++){float a=i*6.2831853f/72;PointF q;if(!project(arc.landing+shooting::Vec{radius*cosf(a),0,radius*sinf(a)},q)){out.clear();return false;}out.push_back(q);bound(q);}return true;};
 float markerRight=0,markerBottom=0;
 if(marker){marker=circle(1.8f,ring)&&circle(1.53f,inner)&&circle(.4f+1.03f*std::clamp((float)arc.time/1.5f,0.f,1.f),countdown);
  if(marker){for(auto p:ring){markerRight=std::max(markerRight,p.X);markerBottom=std::max(markerBottom,p.Y);}
   auto add=[&](std::vector<shooting::Vec>world){std::vector<PointF>pts;for(auto p:world){PointF q;if(!project(p,q))return;pts.push_back(q);bound(q);}accents.push_back(std::move(pts));};
   for(int k=0;k<6;k++){std::vector<shooting::Vec>pts;for(int j=0;j<=10;j++){float a=k*1.0471975f+.13f+j*.065f;pts.push_back(arc.landing+shooting::Vec{1.96f*cosf(a),0,1.96f*sinf(a)});}add(pts);}
   add({arc.landing+shooting::Vec{-.24f,0,0},arc.landing+shooting::Vec{.24f,0,0}});add({arc.landing+shooting::Vec{0,0,-.24f},arc.landing+shooting::Vec{0,0,.24f}});
   if(arc.points.size()>1){auto d=arc.points.back()-arc.points[arc.points.size()-2];d.y=0;float len=shooting::length(d);if(len>.001f){d=d*(1/len);shooting::Vec side{-d.z,0,d.x};auto tip=arc.landing+d*1.12f,tail=arc.landing+d*.72f;add({tail+side*.22f,tip,tail-side*.22f});}}
  }
 }
 RectF label{};if(marker){float lx=markerRight+18*dpi,ly=landing.Y-18*dpi;if(lx+164*dpi>w-4){lx=landing.X-82*dpi;ly=markerBottom+17*dpi;}label={std::clamp(lx,2.f,std::max(2.f,w-168*dpi)),std::clamp(ly,2.f,std::max(2.f,h-49*dpi)),164*dpi,45*dpi};bound({label.X,label.Y});bound({label.GetRight(),label.GetBottom()});}
 int x0=std::clamp((int)floorf(left)-14,0,w),y0=std::clamp((int)floorf(top)-14,0,h),x1=std::clamp((int)ceilf(right)+14,0,w),y1=std::clamp((int)ceilf(bottom)+14,0,h);if(x1<=x0||y1<=y0||!flightSurface.prepare(x1-x0,y1-y0)){flightSurface.hide();return;}
 auto&s=flightSurface;Bitmap canvas(s.width,s.height,s.width*4,PixelFormat32bppPARGB,(BYTE*)s.bits);
 {Graphics g(&canvas);g.Clear(Color(0,0,0,0));g.SetSmoothingMode(SmoothingModeAntiAlias);g.TranslateTransform((REAL)-x0,(REAL)-y0);g.SetClip(Rect(0,0,w,h));courtDepth::masks.apply(g,w,h);
  auto stroke=[&](const std::vector<PointF>&pts,Color c,float width){if(pts.size()<2)return;Pen p(c,width*dpi);p.SetLineJoin(LineJoinRound);p.SetStartCap(LineCapRound);p.SetEndCap(LineCapRound);g.DrawLines(&p,pts.data(),(INT)pts.size());};
  for(auto&path:paths){stroke(path,Color(210,4,12,24),7);stroke(path,Color(65,68,200,255),11);stroke(path,Color(255,86,211,255),3.2f);stroke(path,Color(240,225,251,255),1.f);}
  if(marker){bool imminent=arc.time<.35;Color accent=imminent?Color(255,255,201,106):Color(255,98,229,255);GraphicsPath band(FillModeAlternate);band.AddPolygon(ring.data(),(INT)ring.size());band.AddPolygon(inner.data(),(INT)inner.size());SolidBrush tint(Color(65,accent.GetR(),accent.GetG(),accent.GetB()));g.FillPath(&tint,&band);
   stroke(ring,Color(215,4,12,24),5);stroke(ring,Color(180,accent.GetR(),accent.GetG(),accent.GetB()),1.4f);stroke(inner,Color(120,accent.GetR(),accent.GetG(),accent.GetB()),1);
   stroke(countdown,Color(200,4,12,24),4);stroke(countdown,Color(230,accent.GetR(),accent.GetG(),accent.GetB()),1.6f);
   for(auto&path:accents){stroke(path,Color(225,4,12,24),5.5f);stroke(path,Color(55,accent.GetR(),accent.GetG(),accent.GetB()),9);stroke(path,accent,2.6f);}
   // A compact HUD label sits beside the marker, leaving its impact point clear.
   SolidBrush bg(Color(242,9,22,35));g.FillRectangle(&bg,label);Pen edge(accent,2*dpi);g.DrawLine(&edge,label.X,label.Y,label.X,label.GetBottom());
   g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);Font title(L"Segoe UI",9*dpi,FontStyleBold,UnitPixel),valueFont(L"Segoe UI",16*dpi,FontStyleBold,UnitPixel);SolidBrush text(Color(255,231,250,255)),muted(Color(255,162,190,207));StringFormat fmt;fmt.SetLineAlignment(StringAlignmentCenter);RectF titleBox(label.X+12*dpi,label.Y+3*dpi,145*dpi,15*dpi);g.DrawString(imminent?L"LANDING SOON":L"EST. LANDING",-1,&title,titleBox,&fmt,&muted);wchar_t value[32];swprintf_s(value,L"%.2f s",arc.time);RectF valueBox(label.X+12*dpi,label.Y+17*dpi,145*dpi,25*dpi);g.DrawString(value,-1,&valueFont,valueBox,&fmt,&text);
  }

 }s.present({origin.x+x0,origin.y+y0});
}
static void drawFlightOverlay(){
 auto hide=[](){flightSurface.hide();rollingSurface.hide();};
 if((!ballArcOn&&!rollingOn)||!gameFocused()){hide();flightLive.tracks.clear();flightLive.rollTracks.clear();flightLive.drag.clear();flightLive.interceptMotion.clear();flightLive.display={};flightLive.rawRoll={};flightLive.solvedBall=0;flightLive.chosen=flightLive.chosenRoll=0;return;}
 float matrix[16];auto arc=flightLive.sample(matrix);if(!arc.valid&&!flightLive.rollingPlan.valid){hide();return;}
 HWND game=GetForegroundWindow();RECT rc{};GetClientRect(game,&rc);POINT origin{};ClientToScreen(game,&origin);if(rc.right<100||rc.bottom<100||rc.right>8000||rc.bottom>5000){hide();return;}
 float dpi=GetDpiForWindow(game)/96.f;
 if(ballArcOn&&arc.valid)renderFlightArc(arc,matrix,rc.right,rc.bottom,origin,dpi);else flightSurface.hide();
 if(rollingOn&&flightLive.rollingPlan.valid)renderRollingPlan(flightLive.rollingPlan,matrix,rc.right,rc.bottom,origin,dpi);else rollingSurface.hide();
}
