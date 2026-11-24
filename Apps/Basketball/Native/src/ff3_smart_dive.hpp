#pragma once
#include "ff3_distance_dive.hpp"
namespace ff3Assist {
inline double diveAirWindow(const ff3Play::Scene& scene){
 if(scene.grounded)return .5;
 const auto root=motionPosition(scene);if(!ff3qb::finite(root)||!std::isfinite(scene.groundY)||scene.worldGravity<10||scene.worldGravity>300)return 0;
 const double height=root.y-scene.groundY-.8;if(height<=0)return 0;
 const double v=scene.velocity.y,t=(v+std::sqrt(v*v+2*scene.worldGravity*height))/scene.worldGravity;
 return std::clamp(t,0.,.6);
}
inline DistanceDiveTarget plannedDiveTarget(const ff3Play::Scene& scene,const ff3qb::BallObservation& ball,double time,double range,double extra,double speed,const ContactReach& contact,bool smart,bool steering,Vec heading,double horizon=.5){
 const double window=std::min({horizon,.6,diveAirWindow(scene)});if(window<.06)return {};
 heading=unit(flat(heading));if(ff3qb::length(heading)<.5)heading=unit(scene.cameraForward);
 if(smart){auto baseline=scene;if(!steering){baseline.velocity=heading*speed;baseline.velocity.y=scene.velocity.y;}return distanceDiveTarget(baseline,ball,time,range,extra,speed,contact,window,true,heading,steering);}
 auto target=distanceDiveTarget(scene,ball,time,range,extra,speed,contact,window);
 if(!target.valid||ff3qb::dot(flat(target.point-motionPosition(scene)),heading)>=0)return target;
 // The activation's movement heading is a hard forward half-plane. A pass
 // behind may only be targeted at a future contact ahead or to either side.
 return distanceDiveTarget(scene,ball,time,range,extra,speed,contact,window,true,heading,steering);
}
inline const ff3qb::BallObservation* closestDiveBall(const ff3Play::Scene& scene,bool practice,double range){
 const ff3qb::BallObservation* live=nullptr,*loose=nullptr;double liveGap=range,looseGap=range;bool livePass=false;
 for(const auto& ball:scene.looseBalls){if(!looseActionCandidate(scene,ball,practice))continue;const double gap=ff3qb::length(ball.position-scene.head);const bool flying=!floorLoose(scene,ball)||ff3qb::length(ball.velocity)>6;
  livePass|=flying&&gap<=250;
  if(flying&&gap<liveGap){live=&ball;liveGap=gap;}else if(!flying&&gap<looseGap){loose=&ball;looseGap=gap;}}
 return livePass?live:loose;
}
}
