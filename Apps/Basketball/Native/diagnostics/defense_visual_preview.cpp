#define NOMINMAX
#include "engine.hpp"
#include "pump_overlay.hpp"
#include <iostream>
// Offline rendering of the production overlay; never attaches to a game or sends input.
int wmain(int argc,wchar_t** argv){
 if(argc<2)return 2;bool dark=argc>2&&std::wstring(argv[2])==L"dark";
 using namespace Gdiplus;using namespace defense;
 GdiplusStartupInput startup;if(GdiplusStartup(&pumpGdi,&startup,nullptr)!=Ok)return 3;
 bool ok=false;
 {
 const int w=1280,h=800;Bitmap canvas(w,h,PixelFormat32bppARGB);Graphics g(&canvas);
 g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
 g.Clear(dark?Color(255,31,39,48):Color(255,139,111,84));
 Pen grain(Color(75,71,48,34),1);for(int y=0;y<h;y+=26)g.DrawLine(&grain,0,y,w,y);
 float m[16]={.055f,0,.012f,0,0,.07f,.035f,-.2f,0,0,1,0,0,0,0,1};
 auto project=[&](Vec p){return PointF((m[0]*p.x+m[2]*p.z+1)*w*.5f,(1-m[5]*p.y-m[6]*p.z-m[7])*h*.5f);};
 Pen courtLine(Color(160,237,221,194),3);
 auto line=[&](Vec a,Vec b){g.DrawLine(&courtLine,project(a),project(b));};
 line({-12,0,-14},{12,0,-14});line({-12,0,-14},{-12,0,20});line({12,0,-14},{12,0,20});line({-6,0,8},{6,0,8});line({-6,0,8},{-6,0,20});line({6,0,8},{6,0,20});
 LiveFrame f;f.scene.valid=true;f.scene.at=10;f.scene.floor=0;
 f.scene.own={1,"self",{4,3,0},{},{0,0,-1},{0,10,20},16,false,true};
 f.scene.actors.push_back({2,"opponent",{0,3,-5},{-6,0,10},{0,0,1},{0,10,20},16,true,true});
 Ball b;b.id=100;b.owner=2;b.p={0,4,-5};f.scene.balls.push_back(b);
 Config c;c.values[Guard]=1;c.values[Reaction]=0;c.values[Idle]=0;Intent i;i.focused=i.activation=true;Planner planner;f.decision=planner.step(c,f.scene,i,10);
 // Neutral silhouettes provide scale; they are not captured gameplay.
 for(const auto* a:{&f.scene.own,&f.scene.actors[0]}){
  auto head=project(a->p+Vec{0,2.2f,0}),feet=project(a->p-Vec{0,3,0});SolidBrush shadow(Color(85,4,10,20)),body(a->id==1?Color(255,39,55,81):Color(255,64,69,80));
  g.FillEllipse(&shadow,feet.X-27,feet.Y-8,54.f,16.f);g.FillRectangle(&body,head.X-18,head.Y+20,36.f,58.f);g.FillEllipse(&body,head.X-12,head.Y-6,24.f,24.f);
  g.FillRectangle(&body,head.X-16,head.Y+74,12.f,std::max(8.f,feet.Y-head.Y-74));g.FillRectangle(&body,head.X+4,head.Y+74,12.f,std::max(8.f,feet.Y-head.Y-74));
 }
 defenseVisual::render(f,10,m,w,h,{0,0},{-1,-1},&g);
 SolidBrush header(Color(245,13,20,32)),ink(Color(255,238,245,255)),muted(Color(255,167,183,204));g.FillRectangle(&header,0,0,w,92);
 Font title(L"Segoe UI",24,FontStyleBold,UnitPixel),caption(L"Segoe UI",14,FontStyleRegular,UnitPixel);
 g.DrawString(L"VANTA / COVERAGE",-1,&title,PointF(28,16),&ink);
 g.DrawString(L"Production overlay renderer · synthetic court fixture · no live gameplay",-1,&caption,PointF(28,53),&muted);
 CLSID png={0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e}};
 ok=canvas.Save(argv[1],&png,nullptr)==Ok;
 }
 defenseVisual::close();closePumpOverlay();std::cout<<(ok?"Production overlay fixture rendered\n":"PNG save failed\n");return ok?0:4;
}

