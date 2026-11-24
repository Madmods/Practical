#pragma once
#include "ff3_qb_logic.hpp"
namespace ff3qb {
// Mouse.Hit ends 1000 studs from the camera when the ray hits the sky.
// Intersect that sphere with the desired launch ray from the QB; extending
// the ray 1000 studs from the head instead introduces camera parallax.
inline Vec mouseAimWorld(Vec head,Vec camera,Vec velocity,double rayLength=1000){
 const double speed=length(velocity);if(!finite(head)||!finite(camera)||!finite(velocity)||!std::isfinite(rayLength)||rayLength<100||rayLength>10000||speed<1||speed>150)return {NAN,NAN,NAN};
 const auto direction=velocity*(1/speed),offset=head-camera;const double along=dot(offset,direction),discriminant=along*along+rayLength*rayLength-dot(offset,offset);
 if(discriminant<0||!std::isfinite(discriminant))return {NAN,NAN,NAN};const double distance=-along+std::sqrt(discriminant);return distance>0?head+direction*distance:Vec{NAN,NAN,NAN};
}
// Keep the off-screen coordinates for a directional cue. Never clamp an
// off-screen throw into a different on-screen trajectory for actual input.
inline Screen projectAim(Vec point,const float* matrix,int width,int height){
 if(!finite(point)||!matrix||width<100||height<100)return {};const double w=matrix[12]*point.x+matrix[13]*point.y+matrix[14]*point.z+matrix[15];if(!std::isfinite(w)||w<.01)return {};
 const double x=(matrix[0]*point.x+matrix[1]*point.y+matrix[2]*point.z+matrix[3])/w,y=(matrix[4]*point.x+matrix[5]*point.y+matrix[6]*point.z+matrix[7])/w;
 return std::isfinite(x)&&std::isfinite(y)?Screen{(x+1)*width*.5,(1-y)*height*.5,true}:Screen{};
}
inline bool aimInViewport(Screen point,int width,int height,double inset=1){return point.valid&&std::isfinite(point.x)&&std::isfinite(point.y)&&point.x>=inset&&point.y>=inset&&point.x<width-inset&&point.y<height-inset;}
struct MouseAimGuide {Screen target,marker;bool onScreen=false,aligned=false;};
inline MouseAimGuide mouseAimGuide(Screen target,Screen cursor,int width,int height){
 MouseAimGuide out;out.target=target;if(!target.valid||width<100||height<100)return out;out.onScreen=aimInViewport(target,width,height,24);
 if(out.onScreen){out.marker=target;out.aligned=cursor.valid&&std::hypot(cursor.x-target.x,cursor.y-target.y)<=4;return out;}
 const double cx=width*.5,cy=height*.5,dx=target.x-cx,dy=target.y-cy;const double scale=std::min((cx-28)/std::max(.0001,std::abs(dx)),(cy-28)/std::max(.0001,std::abs(dy)));out.marker={cx+dx*scale,cy+dy*scale,true};return out;
}
// Follow current aim instead of requiring the receiver to stay at the point
// sampled before the initial mouse movement. The game's own cursor must agree.
struct CursorAimTracker {
 double acknowledged=-1;unsigned confirmations=0;
 bool update(Screen desired,Screen windows,Screen game,double time){
  // Match dispatch's four-pixel bound. Renderer/input rounding can settle
  // just over two pixels apart even while the ray is stable.
  if(!desired.valid||!windows.valid||!game.valid||!std::isfinite(time)||std::hypot(windows.x-desired.x,windows.y-desired.y)>=4||std::hypot(game.x-desired.x,game.y-desired.y)>=4){acknowledged=-1;confirmations=0;return false;}
  if(acknowledged<0||time<acknowledged){acknowledged=time;confirmations=1;return false;}++confirmations;return confirmations>=2&&time-acknowledged>=.008;
 }
};
}
