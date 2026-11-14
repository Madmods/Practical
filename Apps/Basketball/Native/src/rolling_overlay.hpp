#pragma once
#include "rolling_logic.hpp"
static PumpSurface rollingSurface;
static void renderRollingPlan(const rolling::Plan&plan,const float*m,int w,int h,POINT origin,float dpi){
 using namespace Gdiplus;if(!plan.valid){rollingSurface.hide();return;}if(!pumpGdi){GdiplusStartupInput in;if(GdiplusStartup(&pumpGdi,&in,nullptr)!=Ok)return;}
 auto project=[&](shooting::Vec p,PointF&out){float z=m[12]*p.x+m[13]*p.y+m[14]*p.z+m[15];if(!std::isfinite(z)||z<.01f)return false;float x=(m[0]*p.x+m[1]*p.y+m[2]*p.z+m[3])/z,y=(m[4]*p.x+m[5]*p.y+m[6]*p.z+m[7])/z;if(!std::isfinite(x)||!std::isfinite(y)||fabs(x)>1.5||fabs(y)>1.5)return false;out={(x+1)*w*.5f,(1-y)*h*.5f};return true;};
 float left=(float)w,top=(float)h,right=0,bottom=0;auto bound=[&](PointF p){left=std::min(left,p.X);right=std::max(right,p.X);top=std::min(top,p.Y);bottom=std::max(bottom,p.Y);};
 auto path=[&](const std::vector<shooting::Vec>&world){std::vector<PointF>p;for(auto v:world){PointF q;if(!project(v,q))return std::vector<PointF>{};p.push_back(q);bound(q);}return p;};
 auto circle=[&](shooting::Vec center,float radius,float a0=0.f,float span=6.2831853f){std::vector<shooting::Vec>v;int n=std::max(8,(int)(span*12));for(int i=0;i<=n;i++){float a=a0+span*i/n;v.push_back(center+shooting::Vec{radius*cosf(a),0,radius*sinf(a)});}return path(v);};
 auto d=plan.point-plan.start;d.y=0;float distance=rolling::flatLength(d);auto dir=distance>.01f?d*(1/distance):shooting::Vec{};shooting::Vec side{-dir.z,0,dir.x};auto start=plan.start+dir*std::min(1.8f,distance*.2f),end=plan.point-dir*std::min(1.28f,distance*.2f);
 auto route=path({start,end}),ball=path(plan.ballPath);
 auto targetOuter=circle(plan.point,1.45f),targetInner=circle(plan.point,1.18f);std::vector<std::vector<PointF>> sectors;for(int i=0;i<3;i++)sectors.push_back(circle(plan.point,1.66f,i*2.094395f+.16f,1.65f));
 auto crossX=path({plan.point+shooting::Vec{-.22f,0,0},plan.point+shooting::Vec{.22f,0,0}}),crossZ=path({plan.point+shooting::Vec{0,0,-.22f},plan.point+shooting::Vec{0,0,.22f}});
 auto ballGround=plan.ball.p;ballGround.y=plan.point.y;auto ballCollar=circle(ballGround,.9f);PointF ballPoint{},at;if(!project(plan.point,at)){rollingSurface.hide();return;}bool showBall=project(ballGround,ballPoint);
 float margin=plan.otherTime<0?99:plan.otherTime-plan.time;bool ahead=margin>.15f;Color state=ahead?Color(255,112,246,182):Color(255,255,197,108),blue(255,91,199,255),gold(255,255,203,122);
 float targetRight=at.X,targetTop=at.Y;for(auto p:targetOuter){targetRight=std::max(targetRight,p.X);targetTop=std::min(targetTop,p.Y);}
 float lx=targetRight+20*dpi;if(lx+178*dpi>w-4)lx=at.X-198*dpi;RectF label{std::clamp(lx,2.f,std::max(2.f,w-182*dpi)),std::clamp(targetTop-8*dpi,2.f,std::max(2.f,h-79*dpi)),178*dpi,75*dpi};bound({label.X,label.Y});bound({label.GetRight(),label.GetBottom()});
 bool ballTag=showBall&&hypotf(at.X-ballPoint.X,at.Y-ballPoint.Y)>150*dpi;RectF ballLabel{std::clamp(ballPoint.X-57*dpi,2.f,std::max(2.f,w-118*dpi)),std::clamp(ballPoint.Y+19*dpi,2.f,std::max(2.f,h-25*dpi)),114*dpi,22*dpi};if(ballTag){bound({ballLabel.X,ballLabel.Y});bound({ballLabel.GetRight(),ballLabel.GetBottom()});}
 int x0=std::clamp((int)floorf(left)-13,0,w),y0=std::clamp((int)floorf(top)-13,0,h),x1=std::clamp((int)ceilf(right)+13,0,w),y1=std::clamp((int)ceilf(bottom)+13,0,h);if(x1<=x0||y1<=y0||!rollingSurface.prepare(x1-x0,y1-y0)){rollingSurface.hide();return;}
 auto&s=rollingSurface;Bitmap canvas(s.width,s.height,s.width*4,PixelFormat32bppPARGB,(BYTE*)s.bits);
 {Graphics g(&canvas);g.Clear(Color(0,0,0,0));g.SetSmoothingMode(SmoothingModeAntiAlias);g.TranslateTransform((REAL)-x0,(REAL)-y0);g.SetClip(Rect(0,0,w,h));courtDepth::masks.apply(g,w,h);
  auto stroke=[&](const auto&pts,Color c,float width,bool dashed=false){if(pts.size()<2)return;Pen pen(c,width*dpi);pen.SetLineJoin(LineJoinRound);pen.SetStartCap(LineCapRound);pen.SetEndCap(LineCapRound);if(dashed)pen.SetDashStyle(DashStyleDash);g.DrawLines(&pen,pts.data(),(INT)pts.size());};
  auto accent=[&](const auto&pts,Color c,float width){stroke(pts,Color(225,3,10,18),width+3);stroke(pts,Color(45,c.GetR(),c.GetG(),c.GetB()),width+6);stroke(pts,c,width);};
  stroke(ball,Color(210,4,12,21),4.5f);stroke(ball,Color(210,255,203,122),1.6f,true);accent(ballCollar,gold,2);
  // A single quiet guide; no animated chevrons or filled ribbon.
  stroke(route,Color(205,3,10,18),4.5f);stroke(route,Color(215,114,206,255),1.7f);

  if(targetOuter.size()>2&&targetInner.size()>2){GraphicsPath band(FillModeAlternate);band.AddPolygon(targetOuter.data(),(INT)targetOuter.size());band.AddPolygon(targetInner.data(),(INT)targetInner.size());SolidBrush fill(Color(65,state.GetR(),state.GetG(),state.GetB()));g.FillPath(&fill,&band);}
  stroke(targetOuter,Color(170,state.GetR(),state.GetG(),state.GetB()),1.2f);stroke(targetInner,Color(115,state.GetR(),state.GetG(),state.GetB()),1);for(auto&a:sectors)accent(a,state,2.8f);accent(crossX,Color(255,234,255,245),1.8f);accent(crossZ,Color(255,234,255,245),1.8f);
  auto rounded=[&](RectF r,float radius){GraphicsPath p;p.AddArc(r.X,r.Y,2*radius,2*radius,180,90);p.AddArc(r.GetRight()-2*radius,r.Y,2*radius,2*radius,270,90);p.AddArc(r.GetRight()-2*radius,r.GetBottom()-2*radius,2*radius,2*radius,0,90);p.AddArc(r.X,r.GetBottom()-2*radius,2*radius,2*radius,90,90);p.CloseFigure();SolidBrush bg(Color(244,10,21,33));g.FillPath(&bg,&p);Pen border(Color(130,100,143,169),1);g.DrawPath(&border,&p);};
  rounded(label,7*dpi);g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);Font smallFont(L"Segoe UI",9*dpi,FontStyleBold,UnitPixel),etaFont(L"Segoe UI",22*dpi,FontStyleBold,UnitPixel),detailFont(L"Segoe UI",10*dpi,FontStyleRegular,UnitPixel);SolidBrush stateBrush(state),white(Color(255,235,250,255)),muted(Color(255,161,188,207));StringFormat fmt;fmt.SetLineAlignment(StringAlignmentCenter);
  RectF title(label.X+12*dpi,label.Y+6*dpi,154*dpi,15*dpi),eta(label.X+12*dpi,label.Y+20*dpi,103*dpi,30*dpi),dist(label.X+112*dpi,label.Y+27*dpi,55*dpi,19*dpi),detail(label.X+12*dpi,label.Y+54*dpi,154*dpi,15*dpi);
  g.DrawString(ahead?L"MEET THE BALL":L"CONTESTED",-1,&smallFont,title,&fmt,&stateBrush);wchar_t text[64];swprintf_s(text,L"%.2f s",plan.time);g.DrawString(text,-1,&etaFont,eta,&fmt,&white);swprintf_s(text,L"%.1f st",distance);g.DrawString(text,-1,&detailFont,dist,&fmt,&muted);
  Pen separator(Color(90,110,154,177),1);g.DrawLine(&separator,label.X+12*dpi,label.Y+52*dpi,label.GetRight()-12*dpi,label.Y+52*dpi);if(plan.otherTime<0)swprintf_s(text,L"Estimated arrival");else if(margin>=0)swprintf_s(text,L"Estimated lead  +%.2f s",margin);else swprintf_s(text,L"Other player  %.2f s sooner",-margin);g.DrawString(text,-1,&detailFont,detail,&fmt,&muted);
  if(ballTag){rounded(ballLabel,5*dpi);StringFormat center;center.SetAlignment(StringAlignmentCenter);center.SetLineAlignment(StringAlignmentCenter);SolidBrush goldBrush(gold);g.DrawString(L"CLOSEST BALL",-1,&smallFont,ballLabel,&center,&goldBrush);}
 }s.present({origin.x+x0,origin.y+y0});
}
