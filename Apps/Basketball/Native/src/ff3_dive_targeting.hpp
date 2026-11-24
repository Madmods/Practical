#pragma once
#include "ff3_velocity_motion.hpp"
namespace ff3Assist {
struct DiveContact {bool valid=false;std::uint64_t ball=0;Vec contact;double eta=0;};
inline DiveContact smartDive(const ff3Play::Scene& scene,const ff3qb::BallObservation& ball,double speed,double time,double horizon=.32){
 DiveContact out;if(!ff3Play::allowed(scene,time)||!ball.id||!ball.verifiedBall||ball.anchored||!ff3qb::finite(ball.position)||!ff3qb::finite(ball.velocity)||!std::isfinite(speed)||speed<0||speed>60||!std::isfinite(horizon)||horizon<.025||horizon>.45)return out;
 const auto shape=scene.naturalShape.measured?scene.naturalShape:ff3qb::CatchGeometry{2.5,2.5,4,true};
 Vec position=scene.head,velocity=scene.velocity;DiveVelocity control;
 if(!control.begin(position,velocity,looseTarget(scene,ball,.04),time+.025,speed,scene.cameraForward))return out;
 const double floor=std::isfinite(scene.groundY)?scene.groundY+(std::isfinite(scene.standingHeadHeight)?scene.standingHeadHeight:5.):-INFINITY;
 for(double elapsed=.008;elapsed<=horizon+.0001;elapsed+=.008){
  const auto target=looseTarget(scene,ball,elapsed+.016);if(control.active&&!control.retarget(target))return out;
  if(elapsed<.025){position=position+velocity*.008+Vec{0,-.5*scene.worldGravity*.008*.008,0};velocity.y-=scene.worldGravity*.008;if(position.y<floor){position.y=floor;velocity.y=std::max(0.,velocity.y);}continue;}
  auto command=control.active?control.sample(position,velocity,time+elapsed):VelocitySample{true,false,{},velocity};if(!command.valid)return out;
  position=position+command.velocity*.008+Vec{0,-.5*scene.worldGravity*.008*.008,0};velocity=command.velocity;velocity.y-=scene.worldGravity*.008;
  if(position.y<floor){position.y=floor;velocity.y=std::max(0.,velocity.y);}
  const auto contact=looseTarget(scene,ball,elapsed);const double posture=scene.grounded?std::clamp(position.y-contact.y,0.,2.5)*ease(elapsed/.08):0;const double height=contact.y-(position.y-posture);
  if(distance(position,contact)<=std::max(.5,shape.radius-.2)&&height<=shape.above-.1&&height>=-shape.below+.1){out={true,ball.id,contact,elapsed};return out;}
  // The pulse ends naturally; continue predicting the resulting dive momentum.
  if(command.complete)control.cancel();
 }
 return out;
}

}
