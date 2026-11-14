#pragma once
namespace defenseVisual {
static PumpSurface court,notice;static HANDLE handle=nullptr;static DWORD process=0;static unsigned seenClick=0;
static void close(){court.close();notice.close();if(handle)CloseHandle(handle);handle=nullptr;process=0;}
static void render(const defense::LiveFrame& f,double t,const float* m,int w,int h,POINT origin,POINT cursor,Gdiplus::Graphics* capture=nullptr){using namespace Gdiplus;using namespace defense;
 auto present=[&](PumpSurface& surface,POINT at){if(capture){Bitmap layer(surface.width,surface.height,surface.width*4,PixelFormat32bppPARGB,(BYTE*)surface.bits);capture->DrawImage(&layer,int(at.x-origin.x),int(at.y-origin.y));}else surface.present(at);};
 if(!pumpGdi){GdiplusStartupInput input;if(GdiplusStartup(&pumpGdi,&input,nullptr)!=Ok)return;}
 auto project=[&](Vec p,PointF&q){float z=m[12]*p.x+m[13]*p.y+m[14]*p.z+m[15];if(!finite(p)||z<.05)return false;float x=(m[0]*p.x+m[1]*p.y+m[2]*p.z+m[3])/z,y=(m[4]*p.x+m[5]*p.y+m[6]*p.z+m[7])/z;if(!std::isfinite(x)||!std::isfinite(y)||fabs(x)>1.3||fabs(y)>1.3)return false;q={(x+1)*w*.5f,(1-y)*h*.5f};return true;};
 float age=(float)std::clamp(t-f.scene.at,0.,.06);auto pos=[&](const Actor&a){return a.p+a.v*age;};
 Identity hover=0;RectF hoverBox{};float best=1e9;
 if(pick)for(auto&a:f.scene.actors)if(a.enemy){PointF top,bottom;auto p=pos(a);if(!project(p+Vec{0,3,0},top)||!project(p-Vec{0,3,0},bottom))continue;float hh=fabs(bottom.Y-top.Y),ww=hh*.65f;RectF box{(top.X+bottom.X)*.5f-ww*.5f,std::min(top.Y,bottom.Y),ww,hh};float gap=distance(a.p,f.scene.own.p);if(box.Contains((float)cursor.x,(float)cursor.y)&&gap<best){hover=a.id;hoverBox=box;best=gap;}}
 unsigned click=clicks.load();if(pick&&click!=seenClick&&hover){selected=hover;pick=false;}seenClick=click;
 auto&d=f.decision;float floor=f.scene.floor+.07f;
 const Color accent(255,135,224,220),ink(245,239,246,248),warning(250,242,177,123);
 struct Stroke{std::vector<PointF> points;Color color;float width;bool fill=false;};std::vector<Stroke> strokes;
 struct Label{PointF point;std::wstring text;Color color;};std::vector<Label> labels;
 auto label=[&](Vec p,const wchar_t* text,Color c,bool left=false){p.y=floor;PointF q;if(project(p,q))labels.push_back({{std::clamp(q.X+(left?-116.f:12.f),4.f,std::max(4.f,w-120.f)),std::clamp(q.Y+8,4.f,std::max(4.f,h-22.f))},text,c});};
 auto stroke=[&](const std::vector<Vec>& world,Color c,float width,bool fill=false){
  Stroke s{{},c,width,fill};for(auto p:world){PointF q;if(!project(p,q))return;s.points.push_back(q);}
  if(s.points.size()>(fill?2u:1u))strokes.push_back(std::move(s));
 };
 auto arc=[&](Vec p,float radius,float angle,float span,Color c,float width){
  p.y=floor;std::vector<Vec> points;for(int k=0;k<=48;k++){float a=angle+span*k/48;points.push_back(p+Vec{cosf(a)*radius,0,sinf(a)*radius});}stroke(points,c,width);
 };
 auto route=[&](Vec a,Vec b){
  a.y=b.y=floor;float span=distance(a,b);if(span<.8f)return;Vec dir=unit(b-a),side{-dir.z,0,dir.x};
  Vec begin=a+dir*.45f,end=b-dir*.35f;
  // The actual walking route: a restrained ribbon, bright center and stride ticks.
  stroke({begin+side*.14f,end+side*.14f,end-side*.14f,begin-side*.14f},Color(35,135,224,220),0,true);
  stroke({begin,end},Color(85,135,224,220),5.f);
  stroke({begin,end},Color(235,135,224,220),2.1f);
  int step=0;for(float x=.85f;x<span-.65f&&step<8;x+=1.05f,++step){
   Vec p=a+dir*x+side*(step%2?.23f:-.23f);
   stroke({p-dir*.12f,p+dir*.12f},Color(190,213,244,242),2.1f);
  }
  if(span>2){Vec p=a+dir*(span*.60f);stroke({p-dir*.32f+side*.22f,p,p-dir*.32f-side*.22f},ink,2.f);}
 };
 auto destination=[&](Vec p){
  p.y=floor;PointF q;if(!project(p,q))return;
  arc(p,.50f,0,6.283185f,Color(210,135,224,220),2.2f);
  arc(p,.72f,.3f,1.1f,Color(105,135,224,220),1.3f);
  arc(p,.72f,3.44f,1.1f,Color(105,135,224,220),1.3f);
  strokes.push_back({{{q.X-3,q.Y},{q.X+3,q.Y}},ink,1.6f});
  strokes.push_back({{{q.X,q.Y-3},{q.X,q.Y+3}},ink,1.6f});
 };
 auto target=actor(f.scene,pick?hover:d.target);
 if(target&&target->enemy&&((on(Coverage)&&d.stance)||pick)){
  Vec p=pos(*target);p.y=floor;Vec axis=unit(flat(target->goal-target->p));
  if(!target->goalValid)axis=unit(flat(f.scene.own.p-target->p));
  float angle=atan2f(axis.z,axis.x),span=2.15f,radius=3.1f;
  // One shallow court ribbon with a clean leading edge. Its fill follows
  // the court plane, and existing avatar masks keep it below player bodies.
  for(int band=0;band<8;band++){
   float inner=radius+band*.11f,outer=inner+.115f;std::vector<Vec> ribbon;
   for(int k=0;k<=48;k++){float a=angle-span*.5f+span*k/48;ribbon.push_back(p+Vec{cosf(a)*inner,0,sinf(a)*inner});}
   for(int k=48;k>=0;k--){float a=angle-span*.5f+span*k/48;ribbon.push_back(p+Vec{cosf(a)*outer,0,sinf(a)*outer});}
   stroke(ribbon,Color((BYTE)(72-band*9),135,224,220),0,true);
  }
  arc(p,radius,angle-span*.5f,span,Color(130,227,242,242),2.5f);
  float covered=pick?1.f:std::clamp(d.coverage,0.f,1.f);
  if(covered>.01f)arc(p,radius,angle-span*.5f,span*covered,accent,2.5f);
  // Small end caps finish the coverage edge without enclosing the player.
  for(float side:{-1.f,1.f}){float a=angle+side*span*.5f;Vec ray{cosf(a),0,sinf(a)};stroke({p+ray*(radius-.10f),p+ray*(radius+.15f)},ink,1.8f);}
 }
 if(on(Route)&&(d.moving||d.stance||d.jump)){
  if(d.moving)route(pos(f.scene.own),d.destination);
  if(d.stance||d.moving){destination(d.destination);label(d.destination,L"GUARD",accent);}
  // Use the same accepted root movement as the planner, not dribble-ball motion.
  // Forecasts are short and bounded, with increasing uncertainty toward the end.
  if(target&&target->enemy&&d.predictionValid&&length(flat(d.predictionVelocity))>2){
   Vec start=d.predictionAnchor,end=predictedOpponent(d,.35f);start.y=end.y=floor;
   float span=distance(start,end);Vec dir=unit(end-start);
   for(float x=.5f;x<span-.25f;x+=.55f){float alpha=230.f-100.f*x/std::max(span,.01f);
    stroke({start+dir*x,start+dir*std::min(x+.27f,span)},Color((BYTE)alpha,246,192,126),2.f);
   }
   for(float dt:{.175f,.35f}){
    Vec point=predictedOpponent(d,dt);float radius=dt<.3f?.38f:.65f;
    for(int k=0;k<8;k++)arc(point,radius,k*.785398f,.49f,Color(dt<.3f?155:220,246,192,126),1.7f);
   }
   PointF a,z;bool left=project(start,a)&&project(end,z)&&z.X<a.X;
   label(end,L"EST. +350 ms",Color(245,246,207,153),left);
  }
 }
 if(d.showMarker){destination(d.marker);if(d.mode==LobTracking||d.mode==Intercepting||d.mode==Rebounding)route(pos(f.scene.own),d.marker);}
 if(d.obstacle&&on(ScreenVisual))if(auto obstacle=actor(f.scene,d.obstacle)){
  Vec p=pos(*obstacle),toward=unit(flat(f.scene.own.p-p));arc(p,1.8f,atan2f(toward.z,toward.x)-.7f,1.4f,warning,2.5f);
 }
 if(strokes.empty())court.hide();else{float x0=(float)w,y0=(float)h,x1=0,y1=0;for(auto&s:strokes)for(auto p:s.points){x0=std::min(x0,p.X);y0=std::min(y0,p.Y);x1=std::max(x1,p.X);y1=std::max(y1,p.Y);}for(auto& l:labels){x0=std::min(x0,l.point.X);y0=std::min(y0,l.point.Y);x1=std::max(x1,l.point.X+115);y1=std::max(y1,l.point.Y+18);}int left=std::clamp((int)floorf(x0)-10,0,w),top=std::clamp((int)floorf(y0)-10,0,h),right=std::clamp((int)ceilf(x1)+10,0,w),bottom=std::clamp((int)ceilf(y1)+10,0,h);
  if(right>left&&bottom>top&&court.prepare(right-left,bottom-top)){Bitmap canvas(court.width,court.height,court.width*4,PixelFormat32bppPARGB,(BYTE*)court.bits);{Graphics g(&canvas);g.Clear(Color(0,0,0,0));g.SetSmoothingMode(SmoothingModeAntiAlias);g.TranslateTransform((REAL)-left,(REAL)-top);g.SetClip(Rect(0,0,w,h));auto state=g.Save();courtDepth::masks.apply(g,w,h);for(size_t k=0;k<strokes.size();k++){auto&s=strokes[k];g.Restore(state);state=g.Save();courtDepth::masks.apply(g,w,h);
 if(s.fill){SolidBrush fill(s.color);g.FillPolygon(&fill,s.points.data(),(INT)s.points.size());continue;}
 Pen shadow(Color(110,8,16,22),s.width+1.5f);shadow.SetLineJoin(LineJoinRound);shadow.SetStartCap(LineCapRound);shadow.SetEndCap(LineCapRound);g.DrawLines(&shadow,s.points.data(),(INT)s.points.size());
 Pen pen(s.color,s.width);pen.SetLineJoin(LineJoinRound);pen.SetStartCap(LineCapRound);pen.SetEndCap(LineCapRound);g.DrawLines(&pen,s.points.data(),(INT)s.points.size());}
 g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);Font font(L"Segoe UI",11,FontStyleBold,UnitPixel);
 for(auto& l:labels){SolidBrush shadow(Color(235,9,16,22)),text(l.color);g.DrawString(l.text.c_str(),-1,&font,PointF(l.point.X+1,l.point.Y+1),&shadow);g.DrawString(l.text.c_str(),-1,&font,l.point,&text);}
 }present(court,{origin.x+left,origin.y+top});}else court.hide();}
 if(pick&&notice.prepare(316,36)){
  Bitmap canvas(notice.width,notice.height,notice.width*4,PixelFormat32bppPARGB,(BYTE*)notice.bits);{Graphics g(&canvas);g.Clear(Color(0,0,0,0));g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);SolidBrush bg(Color(230,16,23,29)),text(ink);g.FillRectangle(&bg,0,0,316,36);Font font(L"Segoe UI",12,FontStyleRegular,UnitPixel);g.DrawString(hover?L"Click to select matchup    /    Esc to cancel":L"Choose a matchup    /    Esc to cancel",-1,&font,PointF(12,10),&text);}present(notice,{origin.x+(w-316)/2,origin.y+54});
 }else notice.hide();
}
static void draw(){using namespace defense;
 auto hide=[](){court.hide();notice.hide();};if((!any()&&!pick)||!gameFocused()){hide();seenClick=clicks;return;}auto f=snapshot();double t=now();if(!f.scene.valid||t-f.scene.at>.15){hide();return;}
 DWORD pid=gamePid.load();if(pid!=process){if(handle)CloseHandle(handle);handle=pid?OpenProcess(PROCESS_VM_READ,FALSE,pid):nullptr;process=pid;}if(!handle){hide();return;}
 float m[16]{};SIZE_T n=0;if(!ReadProcessMemory(handle,(void*)(sceneVisualEngine.load()+Offsets::VisualEngine::ViewMatrix),m,sizeof m,&n)||n!=sizeof m){hide();return;}
 HWND game=GetForegroundWindow();RECT r{};GetClientRect(game,&r);POINT origin{};ClientToScreen(game,&origin);int w=r.right,h=r.bottom;if(w<100||h<100||w>8000||h>5000){hide();return;}
 POINT cursor{};GetCursorPos(&cursor);ScreenToClient(game,&cursor);render(f,t,m,w,h,origin,cursor);
}
}









