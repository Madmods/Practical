#pragma once
#include "ff3_qb_overlay.hpp"
#include "ff3_mags_visual_logic.hpp"
namespace ff3MagsVisual {
using LateBallSampler=std::function<bool(size_t,BallRange&,float*)>;
inline bool drawBalls(PumpSurface& surface,const ff3Native::Frame& f,const std::vector<BallRange>& balls,bool clear=true,const LateBallSampler& lateSample={}){
 using namespace Gdiplus;const int width=f.client.right,height=f.client.bottom;
 if(width<=0||height<=0||width>8000||height>5000||!surface.prepare(width,height))return false;
 Bitmap bitmap(surface.width,surface.height,surface.width*4,PixelFormat32bppPARGB,(BYTE*)surface.bits);Graphics g(&bitmap);
 if(clear)g.Clear(Color(0,0,0,0));g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);g.SetClip(Rect(0,0,width,height));
 const float scale=float(std::clamp(height/1000.,1.,1.35));Font font(L"Segoe UI",10.5f*scale,FontStyleRegular,UnitPixel);bool drawn=false;
 for(size_t index=0;index<balls.size();++index){
  BallRange ball=balls[index];float matrix[16];std::copy(std::begin(f.matrix),std::end(f.matrix),matrix);
  if(lateSample&&!lateSample(index,ball,matrix))continue;if(!valid(volume(ball)))continue;
  auto center=project(ball.center,matrix);if(!finite(center)||center.w<=.02)continue;
  Vec eye;const bool eyeKnown=cameraEye(matrix,eye);const double radius=std::max({ball.size.x,ball.size.y,ball.size.z})*.5;
  const bool inside=eyeKnown&&ff3qb::length(eye-ball.center)<=radius*1.02;
  Color color=!ball.applied?Color(255,255,189,110):ball.inRange?Color(255,111,244,181):Color(255,115,220,255);
  auto tint=[&](int alpha){return Color(BYTE(alpha),color.GetR(),color.GetG(),color.GetB());};
  auto right=ff3Play::unit(Vec{matrix[0],matrix[1],matrix[2]}),up=ff3Play::unit(Vec{matrix[4],matrix[5],matrix[6]});
  auto boundary=eyeKnown?sphereContour(ball,eye):ring(ball,right,up);
  GraphicsPath outline,front,back;
  auto append=[&](GraphicsPath& path,Vec a,Vec b){ff3qb::Screen p,q;if(!segment(a,b,matrix,width,height,p,q)){path.StartFigure();return;}path.StartFigure();path.AddLine(REAL(p.x),REAL(p.y),REAL(q.x),REAL(q.y));};
  if(!inside){
   std::vector<PointF> fill;bool whole=boundary.size()>3;
   for(auto p:boundary){auto clip=project(p,matrix);if(!finite(clip)||clip.w<=.02||plane(clip,1)<0||plane(clip,2)<0||plane(clip,3)<0||plane(clip,4)<0){whole=false;break;}auto q=screen(clip,width,height);fill.emplace_back(REAL(q.x),REAL(q.y));}
   if(whole){SolidBrush wash(tint(3));g.FillPolygon(&wash,fill.data(),INT(fill.size()));}
   for(size_t i=1;i<boundary.size();++i)append(outline,boundary[i-1],boundary[i]);
   for(const auto& axes:{std::pair<Vec,Vec>{{1,0,0},{0,1,0}},{{0,0,1},{0,1,0}},{{1,0,0},{0,0,1}}}){
    auto circle=sphereCircle(ball.center,radius,axes.first,axes.second);
    for(size_t i=1;i<circle.size();++i){auto middle=(circle[i-1]+circle[i])*.5;bool nearer=!eyeKnown||ff3qb::dot(middle-ball.center,eye-ball.center)>radius*radius;append(nearer?front:back,circle[i-1],circle[i]);}
   }
   Pen rear(tint(15),.7f*scale),depth(tint(45),.8f*scale),edge(tint(ball.applied&&ball.inRange?210:165),1.15f*scale);
   if(back.GetPointCount())g.DrawPath(&rear,&back);if(front.GetPointCount())g.DrawPath(&depth,&front);if(outline.GetPointCount()){g.DrawPath(&edge,&outline);drawn=true;}
  }
  if(plane(center,1)>=0&&plane(center,2)>=0&&plane(center,3)>=0&&plane(center,4)>=0){
   auto p=screen(center,width,height);Pen marker(tint(180),.85f*scale);g.DrawEllipse(&marker,REAL(p.x-8*scale),REAL(p.y-8*scale),16*scale,16*scale);
   std::wostringstream text;text<<std::fixed<<std::setprecision(std::abs(ball.size.x-std::round(ball.size.x))<.001?0:1)<<ball.size.x<<L" st";
   const float w=56*scale,h=20*scale,x=REAL(std::clamp(p.x+12*scale,2.,std::max(2.,double(width-w-2)))),y=REAL(std::clamp(p.y-17*scale,2.,std::max(2.,double(height-h-2))));
   SolidBrush bg(Color(145,7,15,23)),ink(Color(240,221,240,247));g.FillRectangle(&bg,x,y,w,h);g.DrawString(text.str().c_str(),-1,&font,PointF(x+5*scale,y+2*scale),&ink);drawn=true;
  }
 }
 return drawn;
}
}
