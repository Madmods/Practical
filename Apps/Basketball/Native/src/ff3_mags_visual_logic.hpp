#pragma once
#include "ff3_reach_edits.hpp"
namespace ff3MagsVisual {
using ff3qb::Vec;
struct BallRange {Vec center,size;bool applied=true;std::array<double,9> rotation{1,0,0,0,1,0,0,0,1};bool inRange=false;Vec hand{NAN,NAN,NAN};};
inline std::vector<Vec> ring(const BallRange& ball,Vec right,Vec up){
 if(!ff3qb::finite(ball.center)||!ff3Play::validCatchSize(ball.size))return {};double radius=std::max({ball.size.x,ball.size.y,ball.size.z})*.5;
 right=ff3Play::unit(right);up=ff3Play::unit(up);if(ff3qb::length(right)<.5||ff3qb::length(up)<.5||std::abs(ff3qb::dot(right,up))>.1)return {};
 std::vector<Vec> points;points.reserve(73);for(int i=0;i<=72;i++){double angle=i*6.283185307179586/72;points.push_back(ball.center+(right*std::cos(angle)+up*std::sin(angle))*radius);}return points;
}
struct Box {Vec center,size;std::array<double,9> rotation{1,0,0,0,1,0,0,0,1};bool applied=false;};
inline bool valid(const Box& b){
 if(!ff3qb::finite(b.center)||!ff3qb::finite(b.size)||std::min({b.size.x,b.size.y,b.size.z})<=.1||std::max({b.size.x,b.size.y,b.size.z})>ff3Play::maxMagSize+.01)return false;
 for(double r:b.rotation)if(!std::isfinite(r))return false;
 Vec axes[3]={{b.rotation[0],b.rotation[3],b.rotation[6]},{b.rotation[1],b.rotation[4],b.rotation[7]},{b.rotation[2],b.rotation[5],b.rotation[8]}};
 for(auto axis:axes)if(std::abs(ff3qb::length(axis)-1)>.05)return false;
 return std::abs(ff3qb::dot(axes[0],axes[1]))<.05&&std::abs(ff3qb::dot(axes[0],axes[2]))<.05&&std::abs(ff3qb::dot(axes[1],axes[2]))<.05;
}
inline std::array<Vec,8> corners(const Box& b){
 std::array<Vec,8> out{};for(int i=0;i<8;i++){double x=(i&1?.5:-.5)*b.size.x,y=(i&2?.5:-.5)*b.size.y,z=(i&4?.5:-.5)*b.size.z;out[i]=b.center+Vec{b.rotation[0]*x+b.rotation[1]*y+b.rotation[2]*z,b.rotation[3]*x+b.rotation[4]*y+b.rotation[5]*z,b.rotation[6]*x+b.rotation[7]*y+b.rotation[8]*z};}return out;
}
inline Box volume(const BallRange& ball){Box box;box.center=ball.center;box.size=ball.size;box.rotation=ball.rotation;box.applied=ball.applied;return box;}
inline Vec axis(const Box& box,int i){return {box.rotation[i],box.rotation[3+i],box.rotation[6+i]};}
inline Vec closestPoint(const Box& box,Vec p){
 Vec result=box.center,delta=p-box.center;double halves[3]={box.size.x*.5,box.size.y*.5,box.size.z*.5};for(int i=0;i<3;i++){auto a=axis(box,i);result=result+a*std::clamp(ff3qb::dot(delta,a),-halves[i],halves[i]);}return result;
}
inline bool overlaps(const Box& a,const Box& b){
 if(!valid(a)||!valid(b))return false;std::vector<Vec> axes;for(int i=0;i<3;i++){axes.push_back(axis(a,i));axes.push_back(axis(b,i));}
 for(int i=0;i<3;i++)for(int j=0;j<3;j++){auto x=axis(a,i),y=axis(b,j);Vec c{x.y*y.z-x.z*y.y,x.z*y.x-x.x*y.z,x.x*y.y-x.y*y.x};if(ff3qb::length(c)>1e-7)axes.push_back(ff3Play::unit(c));}
 const double ah[3]={a.size.x*.5,a.size.y*.5,a.size.z*.5},bh[3]={b.size.x*.5,b.size.y*.5,b.size.z*.5};
 for(auto direction:axes){double ar=0,br=0;for(int i=0;i<3;i++){ar+=ah[i]*std::abs(ff3qb::dot(axis(a,i),direction));br+=bh[i]*std::abs(ff3qb::dot(axis(b,i),direction));}if(std::abs(ff3qb::dot(b.center-a.center,direction))>ar+br+1e-6)return false;}return true;
}
struct Clip {double x=0,y=0,w=0;};
inline Clip project(Vec p,const float* m){return {m[0]*p.x+m[1]*p.y+m[2]*p.z+m[3],m[4]*p.x+m[5]*p.y+m[6]*p.z+m[7],m[12]*p.x+m[13]*p.y+m[14]*p.z+m[15]};}
inline double plane(Clip p,int side){return side==0?p.w-.02:side==1?p.w+p.x:side==2?p.w-p.x:side==3?p.w+p.y:p.w-p.y;}
inline Clip mix(Clip a,Clip b,double t){return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.w+(b.w-a.w)*t};}
inline bool finite(Clip p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.w);}
inline Vec cross3(Vec a,Vec b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline bool cameraEye(const float* m,Vec& eye){Vec a{m[0],m[1],m[2]},b{m[4],m[5],m[6]},c{m[12],m[13],m[14]};double det=ff3qb::dot(a,cross3(b,c));if(!std::isfinite(det)||std::abs(det)<1e-8)return false;eye=(cross3(b,c)*(-m[3])+cross3(c,a)*(-m[7])+cross3(a,b)*(-m[15]))*(1/det);return ff3qb::finite(eye);}
inline std::vector<Vec> sphereCircle(Vec center,double radius,Vec a,Vec b){std::vector<Vec> points;if(!ff3qb::finite(center)||!std::isfinite(radius)||radius<=0)return points;for(int i=0;i<=72;i++){double angle=i*6.283185307179586/72;points.push_back(center+(a*std::cos(angle)+b*std::sin(angle))*radius);}return points;}
inline std::vector<Vec> sphereContour(const BallRange& ball,Vec eye){
 double radius=std::max({ball.size.x,ball.size.y,ball.size.z})*.5,dist=ff3qb::length(eye-ball.center);if(!ff3qb::finite(eye)||!ff3qb::finite(ball.center)||!ff3Play::validCatchSize(ball.size)||!std::isfinite(dist)||dist<=radius+1e-5)return {};
 auto normal=(eye-ball.center)*(1/dist),a=cross3({0,1,0},normal);if(ff3qb::length(a)<.01)a=cross3({1,0,0},normal);a=ff3Play::unit(a);auto b=ff3Play::unit(cross3(normal,a));return sphereCircle(ball.center+normal*(radius*radius/dist),radius*std::sqrt(1-radius*radius/(dist*dist)),a,b);
}
inline ff3qb::Screen screen(Clip p,int width,int height){if(!finite(p)||p.w<.019999)return {};return {std::clamp((p.x/p.w+1)*width*.5,0.,double(width)),std::clamp((1-p.y/p.w)*height*.5,0.,double(height)),true};}
inline bool segment(Vec a,Vec b,const float* m,int width,int height,ff3qb::Screen& start,ff3qb::Screen& end){
 if(width<=0||height<=0)return false;auto p=project(a,m),q=project(b,m);if(!finite(p)||!finite(q))return false;double lo=0,hi=1;
 for(int side=0;side<5;side++){double fa=plane(p,side),fb=plane(q,side);if(fa<0&&fb<0)return false;if(fa>=0&&fb>=0)continue;double t=fa/(fa-fb);if(fa<0)lo=std::max(lo,t);else hi=std::min(hi,t);if(lo>hi)return false;}
 start=screen(mix(p,q,lo),width,height);end=screen(mix(p,q,hi),width,height);return start.valid&&end.valid;
}
inline std::vector<ff3qb::Screen> polygon(const std::vector<Vec>& world,const float* m,int width,int height){
 std::vector<Clip> points;for(auto p:world){auto q=project(p,m);if(!finite(q))return {};points.push_back(q);}
 for(int side=0;side<5&&!points.empty();side++){std::vector<Clip> output;Clip previous=points.back();double fp=plane(previous,side);for(auto current:points){double fc=plane(current,side);if((fp>=0)!=(fc>=0))output.push_back(mix(previous,current,fp/(fp-fc)));if(fc>=0)output.push_back(current);previous=current;fp=fc;}points=std::move(output);}
 std::vector<ff3qb::Screen> out;for(auto p:points){auto q=screen(p,width,height);if(!q.valid)return {};out.push_back(q);}return out;
}
inline std::vector<Vec> footprint(const Box& box,double ground){
 if(!std::isfinite(ground)||!valid(box))return {};auto vertices=corners(box);std::vector<Vec> points(vertices.begin(),vertices.end());for(auto& p:points)p.y=ground;
 std::sort(points.begin(),points.end(),[](Vec a,Vec b){return a.x==b.x?a.z<b.z:a.x<b.x;});points.erase(std::unique(points.begin(),points.end(),[](Vec a,Vec b){return std::hypot(a.x-b.x,a.z-b.z)<1e-7;}),points.end());if(points.size()<3)return {};
 auto cross=[](Vec a,Vec b,Vec c){return (b.x-a.x)*(c.z-a.z)-(b.z-a.z)*(c.x-a.x);};std::vector<Vec> hull;
 for(auto p:points){while(hull.size()>=2&&cross(hull[hull.size()-2],hull.back(),p)<=0)hull.pop_back();hull.push_back(p);}auto lower=hull.size();
 for(auto it=points.rbegin()+1;it!=points.rend();++it){while(hull.size()>lower&&cross(hull[hull.size()-2],hull.back(),*it)<=0)hull.pop_back();hull.push_back(*it);}if(!hull.empty())hull.pop_back();return hull;
}
inline std::vector<Vec> floorSection(const Box& box,double ground){
 if(!std::isfinite(ground)||!valid(box))return {};auto v=corners(box);std::vector<Vec> points;
 for(int i=0;i<8;i++)for(int bit:{1,2,4})if(!(i&bit)){auto a=v[i],b=v[i|bit];double da=a.y-ground,db=b.y-ground;if(std::abs(da)<1e-7)points.push_back({a.x,ground,a.z});if(da*db<0){double t=da/(da-db);auto p=a+(b-a)*t;p.y=ground;points.push_back(p);}if(std::abs(db)<1e-7)points.push_back({b.x,ground,b.z});}
 std::sort(points.begin(),points.end(),[](Vec a,Vec b){return a.x==b.x?a.z<b.z:a.x<b.x;});points.erase(std::unique(points.begin(),points.end(),[](Vec a,Vec b){return ff3qb::length(a-b)<1e-6;}),points.end());if(points.size()<3)return {};
 auto cross=[](Vec a,Vec b,Vec c){return (b.x-a.x)*(c.z-a.z)-(b.z-a.z)*(c.x-a.x);};std::vector<Vec> hull;for(auto p:points){while(hull.size()>1&&cross(hull[hull.size()-2],hull.back(),p)<=0)hull.pop_back();hull.push_back(p);}auto lower=hull.size();for(auto it=points.rbegin()+1;it!=points.rend();++it){while(hull.size()>lower&&cross(hull[hull.size()-2],hull.back(),*it)<=0)hull.pop_back();hull.push_back(*it);}if(!hull.empty())hull.pop_back();return hull;
}
}
