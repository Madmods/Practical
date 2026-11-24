#define NOMINMAX
#include "../src/ff3_qb_overlay.hpp"
#include <iostream>

int main(){
 using namespace ff3OobVisual;using namespace Gdiplus;winrt::init_apartment();unsigned checks=0;
 auto check=[&](bool ok,const char* message){++checks;if(!ok){std::cerr<<"FAIL "<<checks<<": "<<message<<'\n';std::exit(1);}};
 Scene scene;scene.field={true,{0,0,0},{1,0,0},{0,0,1},80,180};scene.allowance=12;scene.ground=0;
 for(double angle:{0.,.45,1.5707963267948966})for(double margin:{0.,.5,2.,12.,20.}){
  auto test=scene;test.allowance=margin;test.field.right={std::cos(angle),0,std::sin(angle)};test.field.forward={-std::sin(angle),0,std::cos(angle)};test.field.center={130,3,-90};
  auto shape=contour(test);check(shape.outer.size()==36&&shape.inner.size()==36,"complete contour has bounded geometry");
  for(size_t i=0;i<shape.outer.size();++i){auto p=shape.outer[i];auto d=p-test.field.closest(p,0);d.y=0;
   check(ff3qb::boundaryAllows(p,test.field,margin)&&std::abs(ff3qb::length(d)-margin)<1e-6,"outline matches the solver's exact radial allowance on every side and corner");
   auto inner=shape.inner[i];check(test.field.contains(inner,-1e-6)&&std::abs(inner.y-.09)<1e-6,"field outline stays on measured perimeter and ground plane");
  }
 }
 {auto test=scene;test.practice=true;check(!visible(test)&&contour(test).outer.empty(),"practice never invents a field");test=scene;test.menu=true;check(!visible(test),"menus hide the boundary");test=scene;test.typing=true;check(!visible(test),"typing hides the boundary");test=scene;test.field.known=false;check(!visible(test),"unknown field is hidden");test=scene;test.allowance=-.5;check(!visible(test),"negative allowances cannot render");test=scene;test.ground=NAN;check(std::abs(planeHeight(test)-.09)<1e-6,"unknown character foot plane uses measured field height");}
 float clipMatrix[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,0};Screen a,b;
 check(segment({-100,0,10},{100,0,10},clipMatrix,1000,700,a,b)&&std::abs(a.x)<.001&&std::abs(b.x-1000)<.001,"long field edges survive with both endpoints offscreen");
 check(!segment({-1,0,-2},{1,0,-1},clipMatrix,1000,700,a,b),"behind-camera edges disappear");
 check(segment({0,0,-2},{0,0,10},clipMatrix,1000,700,a,b)&&a.valid&&b.valid,"near-plane crossings are clipped without flashes");
 check(!segment({NAN,0,0},{0,0,1},clipMatrix,1000,700,a,b),"invalid world geometry rejected");
 auto fill=polygon({{-100,-100,5},{100,-100,5},{100,100,5},{-100,100,5}},clipMatrix,1000,700);check(fill.size()==4,"offscreen allowance bands clip into the viewport");
 for(auto p:fill)check(p.X>=0&&p.X<=1000&&p.Y>=0&&p.Y<=700,"clipped fill coordinates are bounded");
 ULONG_PTR token=0;GdiplusStartupInput input;check(GdiplusStartup(&token,&input,nullptr)==Ok,"GDI+ started");
 {
  ff3Native::Frame frame;frame.client={0,0,1440,900};frame.groundY=0;
  auto unit=[](Vec p){return p*(1/ff3qb::length(p));};auto cross=[](Vec a,Vec b){return Vec{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};};
  Vec eye{290,350,460},forward=unit(Vec{}-eye),right=unit(cross(forward,{0,1,0})),up=cross(right,forward);double scale=1/std::tan(42*3.141592653589793/360),aspect=1440./900;
  auto row=[&](int offset,Vec axis,double factor){frame.matrix[offset]=float(axis.x*factor);frame.matrix[offset+1]=float(axis.y*factor);frame.matrix[offset+2]=float(axis.z*factor);frame.matrix[offset+3]=float(-ff3qb::dot(axis,eye)*factor);};row(0,right,scale/aspect);row(4,up,scale);row(12,forward,1);
  PumpSurface surface;check(ff3Visual::renderBoundary(surface,frame,scene)&&surface.bits&&!surface.visible,"boundary renders without a held ball or selected receiver and never shows a test window");
  const size_t bytes=size_t(surface.width)*surface.height*4;auto pixels=static_cast<unsigned char*>(surface.bits);std::vector<unsigned char> first(pixels,pixels+bytes);
  check(std::any_of(first.begin(),first.end(),[](auto v){return v!=0;}),"visible boundary produces pixels");
  auto center=ff3qb::project({0,0,0},frame.matrix,1440,900);check(center.valid&&pixels[(size_t(center.y)*surface.width+size_t(center.x))*4+3]==0,"playing surface is left transparent");
  ff3Visual::renderField(surface,frame,false,nullptr,false);ff3Visual::renderBalls(surface,frame,false,false);check(std::equal(first.begin(),first.end(),pixels),"beam and ball compositors preserve the boundary layer");
  auto changed=scene;changed.allowance=0;check(ff3Visual::renderBoundary(surface,frame,changed)&&!std::equal(first.begin(),first.end(),pixels),"moving slider to zero immediately updates the contour");
  changed=scene;changed.allowance=20;check(ff3Visual::renderBoundary(surface,frame,changed)&&!std::equal(first.begin(),first.end(),pixels),"extending the slider immediately changes the rendered allowance");
  changed.menu=true;check(!ff3Visual::renderBoundary(surface,frame,changed)&&std::all_of(pixels,pixels+bytes,[](auto v){return v==0;}),"hiding clears every old boundary pixel");
  std::vector<double> times;for(int i=0;i<24;++i){double began=now();ff3Visual::renderBoundary(surface,frame,scene);if(i>3)times.push_back((now()-began)*1000);}std::sort(times.begin(),times.end());std::cout<<"OOB raster median "<<times[times.size()/2]<<" ms, p95 "<<times[times.size()*95/100]<<" ms at 1440 x 900\n";
  {
   Bitmap preview(1440,900,PixelFormat32bppPARGB);Graphics g(&preview);g.Clear(Color(255,12,18,29));g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
   auto quad=[&](double x,double z,double x2,double z2,Color color){auto points=polygon({{x,0,z},{x2,0,z},{x2,0,z2},{x,0,z2}},frame.matrix,1440,900);if(points.size()>=3){SolidBrush brush(color);g.FillPolygon(&brush,points.data(),INT(points.size()));}};
   quad(-136,-236,136,236,Color(255,26,35,49));quad(-123,-223,123,223,Color(255,32,42,57));quad(-110,-210,110,210,Color(255,24,34,47));quad(-100,-200,100,200,Color(255,28,55,50));
   for(int i=0;i<12;++i)quad(-80,-180+i*30,80,-150+i*30,i%2?Color(255,33,83,66):Color(255,29,75,62));
   quad(-80,-180,80,-150,Color(255,28,54,68));quad(-80,150,80,180,Color(255,28,54,68));
   auto line=[&](Vec p,Vec q,Color color,float thickness){Screen a,b;if(segment(p,q,frame.matrix,1440,900,a,b)){Pen pen(color,thickness);g.DrawLine(&pen,float(a.x),float(a.y),float(b.x),float(b.y));}};
   for(int z=-150;z<=150;z+=30){line({-80,0,double(z)},{80,0,double(z)},Color(100,211,232,225),1);for(int x:{-27,27})for(int zz=z+6;zz<z+30&&zz<150;zz+=6)line({double(x)-1.2,0,double(zz)},{double(x)+1.2,0,double(zz)},Color(105,211,232,225),1);}
   check(draw(g,scene,frame.matrix,1440,900),"production renderer draws on the stadium preview");
   Font title(L"Segoe UI",24,FontStyleBold,UnitPixel),caption(L"Segoe UI",13,FontStyleRegular,UnitPixel);SolidBrush white(Color(255,231,238,250)),muted(Color(255,143,158,180));
   g.DrawString(L"OOB BOUNDARY",-1,&title,PointF(48,42),&white);g.DrawString(L"FIELD PERIMETER  /  OUTSIDE ALLOWANCE +12 STUDS",-1,&caption,PointF(49,82),&muted);
   g.DrawString(L"Illustrative stadium view using the actual overlay renderer",-1,&caption,PointF(49,850),&muted);
   CLSID png={0x557cf406,0x1a04,0x11d3,{0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e}};check(preview.Save(L"build/ff3-oob-preview.png",&png,nullptr)==Ok,"preview PNG saved");
  }
  surface.close();
 }
 GdiplusShutdown(token);winrt::uninit_apartment();std::cout<<checks<<" OOB geometry, solver agreement, clipping and compositing checks passed (no game inputs or writes)\n";
}
