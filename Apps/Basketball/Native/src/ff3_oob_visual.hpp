#pragma once
#include "ff3_qb_logic.hpp"
#include <gdiplus.h>
#include <cwchar>

namespace ff3OobVisual {
using ff3qb::Vec;
using ff3qb::Screen;
struct Scene {
 ff3qb::FieldBounds field;
 double allowance=2,ground=NAN;
 bool practice=false,menu=false,typing=false;
};
inline bool visible(const Scene& scene){return !scene.practice&&!scene.menu&&!scene.typing&&scene.field.valid()&&ff3qb::validBoundaryMargin(scene.allowance);}
inline double planeHeight(const Scene& scene){return (std::isfinite(scene.ground)?scene.ground:scene.field.center.y)+.09;}
struct Contour {std::vector<Vec> inner,outer;};
// The solver uses distance to the rectangle, so the outside corners are quarter
// circles rather than a larger rectangle with an excessive diagonal allowance.
inline Contour contour(const Scene& scene){
 Contour result;if(!visible(scene))return result;
 const auto& field=scene.field;const double height=planeHeight(scene);
 const double corners[4][3]={{1,-1,-1.5707963267948966},{1,1,0},{-1,1,1.5707963267948966},{-1,-1,3.141592653589793}};
 for(const auto& corner:corners)for(int i=0;i<=8;++i){
  auto base=field.center+field.right*(corner[0]*field.halfWidth)+field.forward*(corner[1]*field.halfLength);base.y=height;
  const double angle=corner[2]+i*1.5707963267948966/8;
  result.inner.push_back(base);result.outer.push_back(base+(field.right*std::cos(angle)+field.forward*std::sin(angle))*scene.allowance);
 }
 return result;
}
struct Clip {double x,y,w;};
inline Clip clip(Vec p,const float* m){return {m[0]*p.x+m[1]*p.y+m[2]*p.z+m[3],m[4]*p.x+m[5]*p.y+m[6]*p.z+m[7],m[12]*p.x+m[13]*p.y+m[14]*p.z+m[15]};}
inline bool finite(Clip p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.w);}
inline double side(Clip p,int i){return i==0?p.w-.02:i==1?p.w+p.x:i==2?p.w-p.x:i==3?p.w+p.y:p.w-p.y;}
inline Clip mix(Clip a,Clip b,double t){return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.w+(b.w-a.w)*t};}
inline Screen screen(Clip p,int w,int h){if(!finite(p)||p.w<.019999)return {};return {std::clamp((1+p.x/p.w)*w*.5,0.,double(w)),std::clamp((1-p.y/p.w)*h*.5,0.,double(h)),true};}
inline bool segment(Vec a,Vec b,const float* matrix,int w,int h,Screen& start,Screen& end){
 if(w<=0||h<=0||!ff3qb::finite(a)||!ff3qb::finite(b))return false;
 auto p=clip(a,matrix),q=clip(b,matrix);if(!finite(p)||!finite(q))return false;double lo=0,hi=1;
 for(int i=0;i<5;++i){double x=side(p,i),y=side(q,i);if(x<0&&y<0)return false;if(x>=0&&y>=0)continue;double t=x/(x-y);if(x<0)lo=std::max(lo,t);else hi=std::min(hi,t);if(lo>hi)return false;}
 start=screen(mix(p,q,lo),w,h);end=screen(mix(p,q,hi),w,h);return start.valid&&end.valid;
}
inline std::vector<Gdiplus::PointF> polygon(std::vector<Vec> world,const float* matrix,int w,int h){
 if(w<=0||h<=0)return {};std::vector<Clip> points;
 for(auto p:world){if(!ff3qb::finite(p))return {};auto q=clip(p,matrix);if(!finite(q))return {};points.push_back(q);}
 for(int i=0;i<5&&!points.empty();++i){std::vector<Clip> out;auto previous=points.back();double a=side(previous,i);
  for(auto current:points){double b=side(current,i);if((a>=0)!=(b>=0))out.push_back(mix(previous,current,a/(a-b)));if(b>=0)out.push_back(current);previous=current;a=b;}points=std::move(out);
 }
 std::vector<Gdiplus::PointF> result;for(auto p:points){auto q=screen(p,w,h);if(!q.valid)return {};result.emplace_back(float(q.x),float(q.y));}return result;
}
inline bool draw(Gdiplus::Graphics& g,const Scene& scene,const float* matrix,int w,int h){
 using namespace Gdiplus;if(!visible(scene)||w<=0||h<=0)return false;
 const auto shape=contour(scene);if(shape.outer.empty())return false;
 const Color cyan(220,105,222,238),violet(245,184,162,255);
 bool drawn=false;Screen badge;double best=-1;
 auto stroke=[&](Vec a,Vec b,Color color,float width,bool glow){
  Screen p,q;if(!segment(a,b,matrix,w,h,p,q))return;
  Pen halo(Color(30,color.GetR(),color.GetG(),color.GetB()),width+5),shadow(Color(170,6,12,23),width+2),core(color,width);
  halo.SetStartCap(LineCapRound);halo.SetEndCap(LineCapRound);core.SetStartCap(LineCapRound);core.SetEndCap(LineCapRound);
  if(glow)g.DrawLine(&halo,float(p.x),float(p.y),float(q.x),float(q.y));
  g.DrawLine(&shadow,float(p.x),float(p.y),float(q.x),float(q.y));g.DrawLine(&core,float(p.x),float(p.y),float(q.x),float(q.y));drawn=true;
 };
 // Fill only the allowance strip, leaving the entire playing surface clear.
 if(scene.allowance>0)for(size_t i=0;i<shape.outer.size();++i){size_t j=(i+1)%shape.outer.size();auto points=polygon({shape.inner[i],shape.inner[j],shape.outer[j],shape.outer[i]},matrix,w,h);if(points.size()<3)continue;SolidBrush tint(Color(22,158,140,246));g.FillPolygon(&tint,points.data(),INT(points.size()));drawn=true;}
 for(int i=0;i<4;++i){auto a=shape.inner[i*9],b=shape.inner[((i+1)%4)*9];stroke(a,b,cyan,1.3f,scene.allowance==0);
  // A short, brighter return at each corner makes the full perimeter legible.
  auto d=b-a;double length=ff3qb::length(d);if(length>0){auto arm=d*(std::min(8.,length*.12)/length);stroke(a,a+arm,Color(255,173,246,250),2.1f,false);stroke(b-arm,b,Color(255,173,246,250),2.1f,false);}
 }
 for(size_t i=0;i<shape.outer.size();++i){size_t j=(i+1)%shape.outer.size();if(scene.allowance>0)stroke(shape.outer[i],shape.outer[j],violet,1.7f,true);
  // Use a clipped straight edge for a single small legend. This survives a
  // camera inside the stadium with every original corner outside the viewport.
  if(i%9==8){Screen p,q;if(segment(shape.outer[i],shape.outer[j],matrix,w,h,p,q)){double length=std::hypot(q.x-p.x,q.y-p.y),cx=(p.x+q.x)*.5,cy=(p.y+q.y)*.5;double score=std::min(length,800.)+cy*.25-std::abs(cx-w*.5)*.3;if(length>70&&cy>35&&cy<h-12&&score>best){best=score;badge={cx,cy,true};}}}
 }
 if(badge.valid&&w>=240&&h>=160){
  const float bw=178,bh=34,x=float(std::clamp(badge.x-bw*.5,12.,w-bw-12.)),y=float(std::clamp(badge.y-49.,12.,h-bh-12.));
  GraphicsPath pill;pill.AddArc(x,y,12.f,12.f,180.f,90.f);pill.AddArc(x+bw-12,y,12.f,12.f,270.f,90.f);pill.AddArc(x+bw-12,y+bh-12,12.f,12.f,0.f,90.f);pill.AddArc(x,y+bh-12,12.f,12.f,90.f,90.f);pill.CloseFigure();
  SolidBrush bg(Color(226,13,20,32));Pen border(Color(105,153,160,196),1),leader(Color(100,184,162,255),1);g.DrawLine(&leader,x+bw*.5f,y+bh,float(badge.x),float(badge.y));g.FillPath(&bg,&pill);g.DrawPath(&border,&pill);
  Font font(L"Segoe UI",10,FontStyleBold,UnitPixel),valueFont(L"Segoe UI",11,FontStyleBold,UnitPixel);SolidBrush fieldInk(cyan),limitInk(violet);StringFormat format;format.SetLineAlignment(StringAlignmentCenter);
  Pen fieldKey(cyan,2),limitKey(violet,2);g.DrawLine(&fieldKey,x+11,y+17,x+23,y+17);g.DrawString(L"FIELD",-1,&font,RectF(x+29,y,39,bh),&format,&fieldInk);
  g.DrawLine(&limitKey,x+78,y+17,x+90,y+17);wchar_t text[40];swprintf_s(text,L"OOB +%.1f st",scene.allowance);g.DrawString(text,-1,&valueFont,RectF(x+96,y,bw-99,bh),&format,&limitInk);
 }
 return drawn;
}
}
