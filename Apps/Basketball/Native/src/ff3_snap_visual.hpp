#pragma once
#include "ff3_ball_reach_renderer.hpp"
#include "ff3_hand_snap.hpp"
namespace ff3MagsVisual {
// size is the full sphere diameter, exactly like the size-changer setting.
struct SnapRangePreview {Vec center,hand;double size=0;bool selected=false,eligible=false;};
inline bool validSnapPreview(const SnapRangePreview& p){return ff3qb::finite(p.center)&&ff3qb::finite(p.hand)&&std::isfinite(ff3Play::snapRadius(p.size));}
inline bool snapPreviewContains(const SnapRangePreview& p){return validSnapPreview(p)&&ff3Play::withinSnapDistance(p.center,p.hand,p.size);}
inline BallRange snapPreviewBall(const SnapRangePreview& p){BallRange ball{p.center,{p.size,p.size,p.size},true};ball.inRange=p.eligible&&snapPreviewContains(p);ball.hand=p.hand;return ball;}
using LateSnapSampler=std::function<bool(size_t,SnapRangePreview&,float*)>;
inline bool drawSnapRanges(PumpSurface& surface,const ff3Native::Frame& frame,const std::vector<SnapRangePreview>& previews,bool clear=true,const LateSnapSampler& lateSample={}){
 std::vector<BallRange> balls;for(const auto& p:previews)balls.push_back(snapPreviewBall(p));
 // Share the exact size-changer renderer and its full-size-to-radius conversion.
 return drawBalls(surface,frame,balls,clear,[&](size_t i,BallRange& ball,float* matrix){auto p=previews[i];if(lateSample&&!lateSample(i,p,matrix))return false;if(!validSnapPreview(p))return false;ball=snapPreviewBall(p);return true;});
}
}
