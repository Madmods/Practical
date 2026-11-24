#pragma once
#include "ff3_mouse_aim.hpp"
namespace ff3qb {
inline Vec cursorAimPoint(Vec head,Vec camera,Vec launch,const float* matrix,int width,int height,double z=0){
 const auto world=mouseAimWorld(head,camera,launch);const auto screen=projectAim(world,matrix,width,height);
 if(!screen.valid||!std::isfinite(z)||std::abs(z)>100||std::abs(screen.x)>width*8.||std::abs(screen.y)>height*8.)return {NAN,NAN,NAN};
 return {screen.x,screen.y,z};
}
}
