#pragma once
#include "ff3_mouse_aim.hpp"
namespace ff3qb {
struct AimRotation {float m[9]{};};
inline Vec aimCross(Vec a,Vec b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline Vec aimUnit(Vec v){const auto n=length(v);return finite(v)&&n>1e-9?v*(1/n):Vec{NAN,NAN,NAN};}
inline Vec aimColumn(const AimRotation& r,int c){return {r.m[c],r.m[c+3],r.m[c+6]};}
inline Vec aimApply(const AimRotation& r,Vec v){return aimColumn(r,0)*v.x+aimColumn(r,1)*v.y+aimColumn(r,2)*v.z;}
inline bool aimRotationValid(const AimRotation& r){
 const auto x=aimColumn(r,0),y=aimColumn(r,1),z=aimColumn(r,2);
 return finite(x)&&finite(y)&&finite(z)&&std::abs(length(x)-1)<.005&&std::abs(length(y)-1)<.005&&std::abs(length(z)-1)<.005&&std::abs(dot(x,y))<.005&&std::abs(dot(x,z))<.005&&std::abs(dot(y,z))<.005&&dot(aimCross(x,y),z)>.995;
}
// Derive the pixel ray from the actual renderer matrix, including its current
// aspect/FOV, instead of assuming a fixed field of view or degree convention.
inline Vec aimPixelRay(const float* matrix,Vec eye,Screen pixel,int width,int height){
 if(!matrix||!finite(eye)||!aimInViewport(pixel,width,height))return {NAN,NAN,NAN};
 double a[4][8]{};
 for(int row=0;row<4;++row)for(int col=0;col<4;++col){if(!std::isfinite(matrix[row*4+col]))return {NAN,NAN,NAN};a[row][col]=matrix[row*4+col];a[row][col+4]=row==col?1:0;}
 for(int col=0;col<4;++col){int pivot=col;for(int row=col+1;row<4;++row)if(std::abs(a[row][col])>std::abs(a[pivot][col]))pivot=row;
  if(std::abs(a[pivot][col])<1e-12)return {NAN,NAN,NAN};for(int k=0;k<8;++k)std::swap(a[pivot][k],a[col][k]);const auto divisor=a[col][col];for(auto& value:a[col])value/=divisor;
  for(int row=0;row<4;++row)if(row!=col){const auto factor=a[row][col];for(int k=0;k<8;++k)a[row][k]-=factor*a[col][k];}
 }
 const double clip[4]={pixel.x*2/width-1,1-pixel.y*2/height,.5,1};double world[4]{};
 for(int row=0;row<4;++row)for(int col=0;col<4;++col)world[row]+=a[row][col+4]*clip[col];
 if(!std::isfinite(world[3])||std::abs(world[3])<1e-10)return {NAN,NAN,NAN};
 const auto ray=aimUnit(Vec{world[0]/world[3],world[1]/world[3],world[2]/world[3]}-eye);
 const auto projected=projectAim(eye+ray*100,matrix,width,height);
 return projected.valid&&std::hypot(projected.x-pixel.x,projected.y-pixel.y)<.1?ray:Vec{NAN,NAN,NAN};
}
struct DirectAimPose {AimRotation rotation;Vec mouse{NAN,NAN,NAN},endpoint{NAN,NAN,NAN};bool valid=false;};
inline Vec aimLocalPixelRay(const float* matrix,Screen pixel,int width,int height){
 if(!matrix||!aimInViewport(pixel,width,height))return {NAN,NAN,NAN};
 for(int i=0;i<16;++i)if(!std::isfinite(matrix[i]))return {NAN,NAN,NAN};
 const Vec x{matrix[0],matrix[1],matrix[2]},y{matrix[4],matrix[5],matrix[6]},back{-matrix[12],-matrix[13],-matrix[14]};
 const double sx=length(x),sy=length(y),sw=length(back);
 if(sx<1e-8||sy<1e-8||sw<1e-8||std::abs(dot(x,y)/(sx*sy))>.005||std::abs(dot(x,back)/(sx*sw))>.005||std::abs(dot(y,back)/(sy*sw))>.005)return {NAN,NAN,NAN};
 // Only the renderer's intrinsics belong here. Its orientation/eye can lag
 // the camera object by a frame; mixing those poses makes moving aim chase.
 return aimUnit({(pixel.x*2/width-1)*sw/sx,(1-pixel.y*2/height)*sw/sy,-1});
}
inline DirectAimPose directAimPose(const AimRotation& current,const float* matrix,Vec eye,Vec head,Vec launch,Screen pixel,int width,int height,double mouseZ=0){
 DirectAimPose out;if(!aimRotationValid(current)||!std::isfinite(mouseZ)||std::abs(mouseZ)>100)return out;
 const auto local=aimLocalPixelRay(matrix,pixel,width,height);out.endpoint=mouseAimWorld(head,eye,launch);
 if(!finite(local)||!finite(out.endpoint))return out;
 const auto desired=aimUnit(out.endpoint-eye);
 if(!finite(local)||!finite(desired)||local.z>=-.01)return out;
 const auto localUp=aimUnit(Vec{0,1,0}-local*local.y);if(!finite(localUp))return out;
 // Of all rotations that put this ray under this pixel, choose the one with
 // camera up closest to world up. Near vertical, preserve the previous up.
 auto globalUp=Vec{0,1,0}-desired*desired.y;
 if(length(globalUp)<.001){const auto prior=aimColumn(current,1);globalUp=prior-desired*dot(prior,desired);}
 globalUp=aimUnit(globalUp);if(!finite(globalUp))return out;
 const auto localSide=aimCross(local,localUp),globalSide=aimCross(desired,globalUp);
 const Vec localAxes[3]={{1,0,0},{0,1,0},{0,0,1}};
 for(int c=0;c<3;++c){const auto v=desired*dot(local,localAxes[c])+globalUp*dot(localUp,localAxes[c])+globalSide*dot(localSide,localAxes[c]);out.rotation.m[c]=float(v.x);out.rotation.m[c+3]=float(v.y);out.rotation.m[c+6]=float(v.z);}
 out.mouse={pixel.x,pixel.y,mouseZ};out.valid=aimRotationValid(out.rotation)&&length(aimApply(out.rotation,local)-desired)<1e-5;return out;
}
}
