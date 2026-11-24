#pragma once
namespace ff3Visual {
inline bool drawMouseAim(Gdiplus::Graphics& g,const ff3Native::Frame& f){
 using namespace Gdiplus;if(!ff3Native::visualOnly||!f.ready||f.afterRelease||f.menuOpen||f.textFocus)return false;
 const int width=f.client.right,height=f.client.bottom;const auto point=ff3qb::mouseAimWorld(f.head,f.cameraPosition,f.solution.velocity);
 const auto target=ff3qb::projectAim(point,f.matrix,width,height);const ff3qb::Screen cursor{double(f.cursor.x-f.origin.x),double(f.cursor.y-f.origin.y),true};const auto guide=ff3qb::mouseAimGuide(target,cursor,width,height);
 const Color color=guide.aligned?Color(255,110,244,190):Color(255,133,221,255);Pen shadow(Color(220,7,13,22),4),edge(color,1.8f);edge.SetStartCap(LineCapRound);edge.SetEndCap(LineCapRound);
 const float x=REAL(guide.marker.valid?guide.marker.x:width*.5),y=REAL(guide.marker.valid?guide.marker.y:32);
 auto line=[&](float ax,float ay,float bx,float by){g.DrawLine(&shadow,ax,ay,bx,by);g.DrawLine(&edge,ax,ay,bx,by);};
 std::wstring caption;
 if(guide.onScreen){
  g.DrawEllipse(&shadow,x-12,y-12,24.f,24.f);g.DrawEllipse(&edge,x-12,y-12,24.f,24.f);
  for(int sign:{-1,1}){line(x+sign*16,y,x+sign*22,y);line(x,y+sign*16,x,y+sign*22);}
  SolidBrush center(color);g.FillEllipse(&center,x-2,y-2,4.f,4.f);caption=guide.aligned?L"AIM ALIGNED":L"MOVE MOUSE HERE";
 }else if(guide.marker.valid){
  double dx=target.x-width*.5,dy=target.y-height*.5,len=std::max(1.,std::hypot(dx,dy));dx/=len;dy/=len;
  line(x-REAL(dx*16),y-REAL(dy*16),x,y);line(x,y,x-REAL(dx*8-dy*5),y-REAL(dy*8+dx*5));line(x,y,x-REAL(dx*8+dy*5),y-REAL(dy*8-dx*5));
  caption=std::abs(dy)>=std::abs(dx)?dy<0?L"LOOK UP":L"LOOK DOWN":dx<0?L"LOOK LEFT":L"LOOK RIGHT";
 }else caption=L"FACE THE THROW ARC";
 caption+=L"  ·  "+std::to_wstring(int(std::lround(f.power)))+L" POWER";Font font(L"Segoe UI",11,FontStyleBold,UnitPixel);RectF size;g.MeasureString(caption.c_str(),-1,&font,PointF(0,0),&size);
 const float bx=REAL(std::clamp(double(x+27),6.,std::max(6.,width-size.Width-14.))),by=REAL(std::clamp(double(y-10),6.,std::max(6.,height-size.Height-10.)));SolidBrush background(Color(225,9,19,28)),ink(color);g.FillRectangle(&background,bx-5,by-3,size.Width+10,size.Height+6);g.DrawString(caption.c_str(),-1,&font,PointF(bx,by),&ink);return true;
}
}
