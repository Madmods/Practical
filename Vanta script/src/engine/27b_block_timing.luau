-- Block readiness follows the observed ball lift, not one fixed shot timer.
do (function()
 local sample
 settings._blockReady=function(shooter,started,root,now)
  if not sample or sample.shooter~=shooter or sample.started~=started then
   sample={shooter=shooter,started=started,ball=attr(shooter,"Basketball"),lastAt=now,initialRelease=attr(shooter,"ReleasedShot")==true}
  end
  local held=attr(shooter,"Basketball")
  if typeof(held)=="Instance"then
   sample.ball=held;sample.sawHeld=true
   settings._blockTrackShooter=shooter;settings._blockTrackUntil=now+.25
  end
  if attr(shooter,"ReleasedShot")~=true then sample.sawUnreleased=true end
  local point=goalPosition(sample.ball)
  if not point then settings._blockDecision=nil;return false end
  local delta=flat(point-root.Position);local distance=delta.Magnitude
  local facing=distance<.1 or root.CFrame.LookVector:Dot(delta.Unit)>=.1
  local rise=0
  if sample.position and now>sample.lastAt then rise=math.clamp((point.Y-sample.position.Y)/(now-sample.lastAt),-60,60)end
  sample.position=point;sample.lastAt=now
  local height=point.Y-root.Position.Y
  local delay=settings.blockDelay or .08
  local released=(sample.sawHeld and held~=sample.ball)
   or(attr(shooter,"ReleasedShot")==true and(sample.sawUnreleased or not sample.initialRelease))
  -- Ball lift is a wind-up, not a release. Never anticipate from vertical velocity.
  local ready=released and now-started>=delay and height>=3
   and distance<=(settings.blockRange or 5) and facing and height<=7.5
  settings._blockDecision={position=point,ready=ready,delay=delay,height=height,rise=rise,
   reason=not released and "Waiting for actual release"or "Release observed"}
  return ready
 end
 local previous=featureCleanup
 featureCleanup=function()
  settings._blockTrackShooter=nil;settings._blockTrackUntil=nil;settings._blockReady=nil;settings._blockDecision=nil;settings._blockFacingDirection=nil;settings._blockFacingUntil=nil
  previous()
 end
end)()end
