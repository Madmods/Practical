#pragma once
#include "ff3_qb_logic.hpp"
namespace ff3Input {
struct Point {double x=0,y=0;};
struct Button {std::uint64_t instance=0;int power=0,z=0,order=0;Point position,size;double rotation=0;};
struct Wheel {std::uint64_t instance=0;int displayOrder=0,siblingOrder=0;bool enabled=false,visible=false;};
inline std::uint64_t topWheel(const std::vector<Wheel>& wheels){
 const Wheel* top=nullptr;for(const auto& wheel:wheels)if(wheel.instance&&wheel.enabled&&wheel.visible&&(!top||wheel.displayOrder>top->displayOrder||(wheel.displayOrder==top->displayOrder&&wheel.siblingOrder>top->siblingOrder)))top=&wheel;
 return top?top->instance:0;
}
inline bool finite(Point p){return std::isfinite(p.x)&&std::isfinite(p.y);}
inline bool contains(const Button& b,Point p,double inset=0){
 if(!finite(p)||!finite(b.position)||!finite(b.size)||!std::isfinite(b.rotation)||b.size.x<=0||b.size.y<=0)return false;
 const double angle=b.rotation*3.14159265358979323846/180.,dx=p.x-b.position.x-b.size.x/2,dy=p.y-b.position.y-b.size.y/2;
 return std::abs(dx*std::cos(angle)+dy*std::sin(angle))<b.size.x/2-inset&&std::abs(-dx*std::sin(angle)+dy*std::cos(angle))<b.size.y/2-inset;
}
inline int powerIndex(const std::string& name){
 if(name.empty()||name.size()>3||name.find_first_not_of("0123456789")!=std::string::npos)return 0;
 int value=0;for(char c:name)value=value*10+c-'0';return value>=1&&value<=100?value:0;
}
// Sample the visible button away from overlapping siblings. Window dimensions
// are client pixels; AbsolutePosition already includes Roblox's GUI inset.
inline bool selectPower(const std::vector<Button>& buttons,int power,int width,int height,Point& point,std::uint64_t& instance){
 if(power<1||power>95||width<2||height<2)return false;
 for(const auto& b:buttons)if(b.power==power&&b.instance){
  const double angle=b.rotation*3.14159265358979323846/180.;
  for(double x:{0.,-.75,.75,-.95,.95,-.5,.5})for(double y:{0.,-.7,.7,-.9,.9}){
   const double lx=x*b.size.x/2,ly=y*b.size.y/2;
   Point p{b.position.x+b.size.x/2+lx*std::cos(angle)-ly*std::sin(angle),b.position.y+b.size.y/2+lx*std::sin(angle)+ly*std::cos(angle)};
   p.x=std::round(p.x);p.y=std::round(p.y);
   if(p.x<1||p.y<1||p.x>=width-1||p.y>=height-1||!contains(b,p,.25))continue;
   bool covered=false;for(const auto& other:buttons)if(other.instance!=b.instance&&(other.z>b.z||(other.z==b.z&&other.order>b.order))&&contains(other,p,-.5)){covered=true;break;}
   if(!covered){point=p;instance=b.instance;return true;}
  }
 }return false;
}
inline double blend(double t){t=std::clamp(t,0.,1.);return t*t*t*(10+t*(-15+6*t));}
inline Point interpolate(Point start,Point end,double t){const double s=blend(t);return{start.x+(end.x-start.x)*s,start.y+(end.y-start.y)*s};}
}
