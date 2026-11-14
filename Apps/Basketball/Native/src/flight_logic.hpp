#pragma once
#include "shooting_logic.hpp"
#include <deque>
#include <vector>
namespace flight {
using shooting::Vec;
struct Sample{double at;Vec p;};
struct Estimate{bool valid=false;Vec p{},v{};float gravity=45,error=0;double at=0;};
struct Track{
 std::deque<Sample> points;Estimate estimate;
 void reset(){points.clear();estimate={};}
 Estimate add(double t,Vec p,float gravity,float minimumSpeed=2){
  if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z)){reset();return {};}
  if(!points.empty()){
   double dt=t-points.back().at;if(dt<=0)return estimate;
   if(dt>.3||shooting::length(p-points.back().p)>180*dt+1)reset();
   else if(shooting::length(p-points.back().p)<.008f){if(dt>.1)estimate.valid=false;return estimate;}
  }
  // A discontinuity (rim, board, catch, new launch) invalidates the old arc.
  if(estimate.valid){float dt=(float)(t-estimate.at);auto predicted=estimate.p+estimate.v*dt+Vec{0,-.5f*estimate.gravity*dt*dt,0};if(shooting::length(predicted-p)>.65f)reset();}
  points.push_back({t,p});while(points.size()>14||(!points.empty()&&t-points.front().at>.18))points.pop_front();
  estimate={};if(points.size()<3||t-points.front().at<.035)return estimate;
  // Fit velocity to measured positions, correcting for known gravity. This also
  // works for client-animated balls whose AssemblyLinearVelocity remains zero.
  double sx=0,ss=0;Vec sy{},sxy{};float n=(float)points.size();
  for(auto&s:points){float dt=(float)(s.at-t);Vec q=s.p+Vec{0,.5f*gravity*dt*dt,0};sx+=dt;ss+=dt*dt;sy=sy+q;sxy=sxy+q*dt;}
  double den=ss-sx*sx/n;if(den<1e-7)return estimate;
  Vec v=(sxy-sy*(float)(sx/n))*(float)(1/den),intercept=(sy-v*(float)sx)*(1/n);float error=0;
  for(auto&s:points){float dt=(float)(s.at-t);auto predicted=intercept+v*dt+Vec{0,-.5f*gravity*dt*dt,0};error=std::max(error,shooting::length(predicted-s.p));}
  if(!std::isfinite(shooting::length(v))||shooting::length(v)>180||error>.55f)return estimate;
  estimate={shooting::length(v)>minimumSpeed,p,v,gravity,error,t};return estimate;
 }
};
struct Arc{bool valid=false;double time=0;Vec landing{};std::vector<Vec> points;};
inline Arc predict(const Estimate&e,float floor,float radius){
 Arc a;if(!e.valid||!std::isfinite(floor)||e.gravity<1||e.gravity>300)return a;
 double height=e.p.y-floor-radius;if(height<-.15)return a;
 double disc=e.v.y*e.v.y+2*e.gravity*std::max(0.,height);double t=(e.v.y+sqrt(disc))/e.gravity;
 if(!std::isfinite(t)||t<.06||t>4)return a;
 a.time=t;a.landing=e.p+e.v*(float)t+Vec{0,(float)(-.5*e.gravity*t*t),0};a.landing.y=floor+.03f;
 if(shooting::length(a.landing-e.p)>160)return {};
 for(int i=0;i<=48;i++){float dt=(float)t*i/48;a.points.push_back(e.p+e.v*dt+Vec{0,-.5f*e.gravity*dt*dt,0});}a.valid=true;return a;
}
}
