#pragma once
#include "ff3_play_runtime.hpp"
#include "ff3_qb_overlay.hpp"
#include "ff3_mags_visual_logic.hpp"
#include "ff3_ball_reach_renderer.hpp"
#include "ff3_snap_visual.hpp"
namespace ff3MagsVisual {
inline bool draw(PumpSurface& surface,const ff3Native::Frame& f,const std::vector<Box>& boxes,bool clear=true){
 using namespace Gdiplus;const int width=f.client.right,height=f.client.bottom;if(width<=0||height<=0||width>8000||height>5000||!surface.prepare(width,height))return false;
 Bitmap bitmap(surface.width,surface.height,surface.width*4,PixelFormat32bppPARGB,(BYTE*)surface.bits);Graphics g(&bitmap);if(clear)g.Clear(Color(0,0,0,0));g.SetSmoothingMode(SmoothingModeAntiAlias);g.SetClip(Rect(0,0,width,height));
 bool drawn=false;int index=0;
 for(const auto& box:boxes){if(!valid(box))continue;const auto v=corners(box);Color color=box.applied?(index++%2?Color(245,193,155,255):Color(245,98,230,255)):Color(225,255,190,100);
  const int faces[6][4]={{0,1,3,2},{4,6,7,5},{0,4,5,1},{2,3,7,6},{0,2,6,4},{1,5,7,3}};
  for(const auto& face:faces){auto clipped=polygon({v[face[0]],v[face[1]],v[face[2]],v[face[3]]},f.matrix,width,height);if(clipped.size()<3)continue;std::vector<PointF> points;for(auto p:clipped)points.emplace_back(REAL(p.x),REAL(p.y));SolidBrush fill(Color(7,color.GetR(),color.GetG(),color.GetB()));g.FillPolygon(&fill,points.data(),INT(points.size()));drawn=true;}
  auto line=[&](Vec a,Vec b,bool dashed){ff3qb::Screen p,q;if(!segment(a,b,f.matrix,width,height,p,q))return;Pen shadow(Color(130,5,12,22),dashed?2.5f:4),glow(Color(30,color.GetR(),color.GetG(),color.GetB()),6),edge(Color(dashed?160:235,color.GetR(),color.GetG(),color.GetB()),dashed?1.2f:1.8f);edge.SetStartCap(LineCapRound);edge.SetEndCap(LineCapRound);if(dashed)edge.SetDashStyle(DashStyleDash);if(!dashed)g.DrawLine(&glow,REAL(p.x),REAL(p.y),REAL(q.x),REAL(q.y));g.DrawLine(&shadow,REAL(p.x),REAL(p.y),REAL(q.x),REAL(q.y));g.DrawLine(&edge,REAL(p.x),REAL(p.y),REAL(q.x),REAL(q.y));drawn=true;};
  for(int i=0;i<8;i++)for(int bit:{1,2,4})if(!(i&bit))line(v[i],v[i|bit],false);
  auto ground=footprint(box,f.groundY+.08);for(size_t i=0;i<ground.size();i++)line(ground[i],ground[(i+1)%ground.size()],true);
 }
 return drawn;
}
inline bool sampleBallTransform(HANDLE handle,addr primitive,BallRange& ball){
 struct Transform {float rotation[9];shooting::Vec position;};
 static_assert(Offsets::Primitive::Position==Offsets::Primitive::Rotation+sizeof(float)*9,"Read rotation and position together");
 static_assert(sizeof(Transform)==48,"Native transform layout");
 Transform missing;for(auto& value:missing.rotation)value=NAN;missing.position={NAN,NAN,NAN};
 const auto measured=rd<Transform>(handle,primitive+Offsets::Primitive::Rotation,missing);
 ball.center=ff3Native::vector(measured.position);for(int i=0;i<9;++i)ball.rotation[i]=measured.rotation[i];
 return valid(volume(ball));
}
struct Source {
 DWORD pid=0;addr model=0,own=0,gui=0,input=0,palmPart=0,palmPrimitive=0;bool r6Palm=false;Vec palmSize;double scanned=-10;
 std::pair<addr,addr> catches;
 bool render(PumpSurface& surface,const ff3Native::Frame& f,HANDLE h,bool clear){
  if(!ff3Play::magsOn||!ff3Play::magsVisual.load()||!ff3Native::admitted()||!ff3Native::focused(f.pid)||!h||!f.own||rd<addr>(h,f.localPlayer+Offsets::Player::ModelInstance)!=f.own)return false;
  if(pid!=f.pid||model!=f.model||own!=f.own||now()-scanned>.25){pid=f.pid;model=f.model;own=f.own;gui=child(h,model,"GuiService");input=child(h,model,"UserInputService");catches=ff3Native::catchParts(h,f.own);palmPart=child(h,f.own,"RightHand");r6Palm=!palmPart;if(!palmPart)palmPart=child(h,f.own,"Right Arm");palmPrimitive=rd<addr>(h,palmPart+Offsets::BasePart::Primitive);palmSize=ff3Native::partSize(h,palmPart);scanned=now();}
  auto state=rd<addr>(h,input+Offsets::UserInputService::WindowInputState);if(!state||rd<addr>(h,state+Offsets::WindowInputState::CurrentTextBox)!=0||rd<unsigned char>(h,gui+Offsets::GuiService::MenuIsOpen,255)!=0)return false;
  const auto snapshot=ff3Play::volumeSnapshot();if(snapshot.pid!=f.pid||snapshot.model!=f.model||now()<snapshot.at||now()-snapshot.at>.3)return false;
  struct Identity {addr id,primitive,mesh,rig;};
  std::vector<BallRange> balls;std::vector<Identity> identities;std::vector<Box> hands;
  for(auto part:{catches.first,catches.second}){if(!part||rd<addr>(h,part+Offsets::Instance::Parent)!=f.own)continue;auto primitive=rd<addr>(h,part+Offsets::BasePart::Primitive);Box box;box.center=ff3Native::position(h,part);box.size=ff3Native::partSize(h,part);defense::Matrix3 invalid;for(auto& x:invalid.m)x=NAN;auto rotation=rd<defense::Matrix3>(h,primitive+Offsets::Primitive::Rotation,invalid);for(int i=0;i<9;i++)box.rotation[i]=rotation.m[i];if(valid(box))hands.push_back(box);}
  auto measure=[&](BallRange& ball){ball.inRange=false;ball.hand={NAN,NAN,NAN};auto box=volume(ball);double nearest=INFINITY;for(const auto& hand:hands){const bool contact=overlaps(box,hand);double gap=ff3qb::length(hand.center-closestPoint(box,hand.center));if((contact&&!ball.inRange)||(contact==ball.inRange&&gap<nearest)){nearest=gap;ball.hand=hand.center;}ball.inRange=ball.inRange||contact;}};
  for(const auto& part:snapshot.parts){
   auto parent=rd<addr>(h,part.id+Offsets::Instance::Parent);if(!parent||rd<addr>(h,part.id+Offsets::BasePart::Primitive)!=part.primitive)continue;
   auto center=ff3Play::snapRenderedPosition(h,part),size=ff3Native::partSize(h,part.id);
   if(!ff3qb::finite(center)||!ff3qb::finite(size))continue;
   if(part.guide){if(!ff3Play::sameSize(size,part.size))continue;}
   else if(!ff3Play::validCatchSize(size)||!ff3Play::sameSize(size,ff3Play::magsDimensions()))continue;
   if(!part.ball||!part.mesh||rd<addr>(h,part.mesh+Offsets::Instance::Parent)!=part.id)continue;
   BallRange ball{center,part.guide?ff3Play::magsDimensions():size,!part.guide};defense::Matrix3 invalid;for(auto& x:invalid.m)x=NAN;auto rotation=rd<defense::Matrix3>(h,part.primitive+Offsets::Primitive::Rotation,invalid);for(int i=0;i<9;i++)ball.rotation[i]=rotation.m[i];auto box=volume(ball);if(part.guide)box.size=size;if(!valid(box))continue;
   measure(ball);balls.push_back(ball);identities.push_back({part.id,part.primitive,part.mesh,part.rig});
  }
  ff3Native::Frame display;display.client=f.client;display.groundY=f.groundY;display.at=now();std::copy(std::begin(f.matrix),std::end(f.matrix),display.matrix);
  return drawBalls(surface,display,balls,clear,[&](size_t index,BallRange& ball,float* projection){
   const auto& part=identities[index];
   if(!rd<addr>(h,part.id+Offsets::Instance::Parent)||rd<addr>(h,part.id+Offsets::BasePart::Primitive)!=part.primitive||rd<addr>(h,part.mesh+Offsets::Instance::Parent)!=part.id)return false;
   if(!sampleBallTransform(h,part.primitive,ball)||!ff3Visual::ViewReader::matrix(h,f.cameraMatrix,projection))return false;
   ball.center=ff3Play::snapRenderedPosition(h,{part.id,part.rig,part.primitive,ball.size,true,part.mesh});if(!ff3qb::finite(ball.center))return false;
   measure(ball);return true;
  });
 }
};
}
