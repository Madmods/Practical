#pragma once
#include <gdiplus.h>
// Approximate avatar occlusion only. No game depth buffer is available externally.
namespace courtDepth {
struct Point {float x,y;};
inline float cross(Point a,Point b,Point c){return (b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);}
inline std::vector<Point> hull(std::vector<Point> p){
 if(p.size()<3)return {};std::sort(p.begin(),p.end(),[](Point a,Point b){return a.x<b.x||(a.x==b.x&&a.y<b.y);});std::vector<Point>h;
 for(auto q:p){while(h.size()>1&&cross(h[h.size()-2],h.back(),q)<=0)h.pop_back();h.push_back(q);}size_t k=h.size()+1;
 for(int i=(int)p.size()-2;i>=0;i--){while(h.size()>=k&&cross(h[h.size()-2],h.back(),p[i])<=0)h.pop_back();h.push_back(p[i]);}if(!h.empty())h.pop_back();return h.size()>=3?h:std::vector<Point>{};
}
inline std::vector<Point> projectBox(const float*m,shooting::Vec position,shooting::Vec half,const float*r){
 std::vector<Point>pts;for(float x:{-half.x,half.x})for(float y:{-half.y,half.y})for(float z:{-half.z,half.z}){
  shooting::Vec p=position+shooting::Vec{r[0]*x+r[1]*y+r[2]*z,r[3]*x+r[4]*y+r[5]*z,r[6]*x+r[7]*y+r[8]*z};float w=m[12]*p.x+m[13]*p.y+m[14]*p.z+m[15];if(!std::isfinite(w)||w<.03f)return {};
  float xx=(m[0]*p.x+m[1]*p.y+m[2]*p.z+m[3])/w,yy=(m[4]*p.x+m[5]*p.y+m[6]*p.z+m[7])/w;if(!std::isfinite(xx)||!std::isfinite(yy)||fabs(xx)>3||fabs(yy)>3)return {};pts.push_back({(xx+1)*.5f,(1-yy)*.5f});
 }return hull(std::move(pts));
}
struct Masks {
 HANDLE process=nullptr;DWORD pid=0;addr root=0,own=0;double scanAt=-10;struct Part{addr primitive;shooting::Vec half;};std::vector<Part>parts;std::vector<std::vector<Point>>polygons;
 void close(){if(process)CloseHandle(process);process=nullptr;pid=0;root=own=0;parts.clear();polygons.clear();scanAt=-10;}
 void update(){
  DWORD current=gamePid.load();if(pid!=current){close();pid=current;process=pid?OpenProcess(PROCESS_VM_READ,FALSE,pid):nullptr;}polygons.clear();if(!process)return;
  addr model=sceneOwn.load(),world=sceneRoot.load();if(!model||!world)return;double t=now();
  if(t-scanAt>.35||world!=root||model!=own){root=world;own=model;scanAt=t;parts.clear();shooting::Vec me,velocity;if(!positionOf(process,own,me,velocity))return;
   std::vector<std::pair<float,addr>> actors;for(auto a:children(process,path(process,root,{"Workspace","Characters"}))){if(nm(process,a)=="InvisCharacter")continue;shooting::Vec p,v;if(!positionOf(process,a,p,v))continue;float distance=shooting::length(p-me);if(distance>80||fabs(p.y-me.y)>25)continue;actors.push_back({a==own?-100.f:distance,a});}
   std::sort(actors.begin(),actors.end());if(actors.size()>6)actors.resize(6);
   for(auto&a:actors)for(auto part:children(process,a.second)){auto name=nm(process,part);
    bool body=name=="Head"||name=="UpperTorso"||name=="LowerTorso"||name=="Torso"||name=="Left Arm"||name=="Right Arm"||name=="Left Leg"||name=="Right Leg"||name=="LeftUpperArm"||name=="LeftLowerArm"||name=="LeftHand"||name=="RightUpperArm"||name=="RightLowerArm"||name=="RightHand"||name=="LeftUpperLeg"||name=="LeftLowerLeg"||name=="LeftFoot"||name=="RightUpperLeg"||name=="RightLowerLeg"||name=="RightFoot";
    if(!body)continue;addr primitive=rd<addr>(process,part+Offsets::BasePart::Primitive);if(!primitive)continue;auto size=rd<shooting::Vec>(process,primitive+Offsets::Primitive::Size);
    if(!std::isfinite(size.x)||!std::isfinite(size.y)||!std::isfinite(size.z)||size.x<=0||size.y<=0||size.z<=0||size.x>12||size.y>12||size.z>12)continue;
    parts.push_back({primitive,size*.5f});
   }
  }
  float matrix[16];SIZE_T bytes=0;addr camera=sceneVisualEngine.load();if(!camera||!ReadProcessMemory(process,(void*)(camera+Offsets::VisualEngine::ViewMatrix),matrix,64,&bytes)||bytes!=64)return;
  for(auto&part:parts){struct Transform{float r[9];shooting::Vec p;};auto tr=rd<Transform>(process,part.primitive+Offsets::Primitive::Rotation);bool valid=true;for(int i=0;i<3;i++){float norm=0;for(int j=0;j<3;j++)norm+=tr.r[i*3+j]*tr.r[i*3+j];if(!std::isfinite(norm)||norm<.8f||norm>1.2f)valid=false;}if(!valid)continue;auto polygon=projectBox(matrix,tr.p,part.half,tr.r);if(!polygon.empty())polygons.push_back(std::move(polygon));}
 }
 void apply(Gdiplus::Graphics&g,int w,int h)const{
  if(polygons.empty())return;Gdiplus::GraphicsPath mask(Gdiplus::FillModeWinding);
  for(auto&polygon:polygons){std::vector<Gdiplus::PointF>p;for(auto q:polygon)p.emplace_back(q.x*w,q.y*h);// Cover the thin antialias/glow fringe as well as the projected body.
   Gdiplus::PointF center{};for(auto q:p){center.X+=q.X/p.size();center.Y+=q.Y/p.size();}for(auto&q:p){float dx=q.X-center.X,dy=q.Y-center.Y,len=sqrtf(dx*dx+dy*dy);if(len>.01f){q.X+=dx/len*1.5f;q.Y+=dy/len*1.5f;}}mask.AddPolygon(p.data(),(INT)p.size());}
  g.SetClip(&mask,Gdiplus::CombineModeExclude);
 }
};
static Masks masks;
}
