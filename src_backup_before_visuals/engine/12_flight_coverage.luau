-- Stabilized flight tracking and opponent coverage.
do (function()
 settings.guardCoverage=true
 local samples=setmetatable({},{__mode="k"})
 local selected
 settings._flightBall=function(folder,occupied,root,now)
  local best,score
  if not folder then settings._flightVelocity=nil;return nil end
  for _,ball in ipairs(folder:GetChildren()) do
   local position=goalPosition(ball)
   if position and not occupied[ball] and flat(position-root.Position).Magnitude<=math.max(settings.landingRange,settings.reboundChaseRange) then
    local sample=samples[ball]
    if not sample then sample={position=position,time=now};samples[ball]=sample
    elseif (position-sample.position).Magnitude>.015 and now-sample.time>.005 then
     local dt=now-sample.time
     local velocity=(position-sample.position)/dt
     if velocity.Magnitude<180 and dt<.4 then
      local alpha=math.clamp(1-math.exp(-14*dt),.15,.8)
      sample.velocity=sample.velocity and sample.velocity:Lerp(velocity,alpha) or velocity
      sample.moved=now
     else sample.velocity=nil;sample.moved=nil end
     sample.position=position;sample.time=now
    end
    if sample.velocity and sample.velocity.Magnitude>1 and now-(sample.moved or 0)<.3 then
     local rank=flat(position-root.Position).Magnitude-(ball==selected and 15 or 0)
     if not score or rank<score then best=ball;score=rank end
    end
   end
  end
  selected=best;settings._flightSelected=best;settings._flightVelocity=best and samples[best].velocity or nil
  return best
 end
 local outline=Instance.new("Highlight");outline.Name="GuardCoverage";outline.FillTransparency=.98;outline.OutlineTransparency=.12;outline.Parent=visualFolder
 on(Run.Heartbeat,function()
  local me=model();local ownRoot=rootOf(me);local guarded=settings.denyReceiver and settings._denyReceiverTarget or target;local otherRoot=rootOf(guarded)
  outline.Enabled=(not settings._visualV2 or settings.visualShowCoverage)and settings.guardCoverage and (settings.guard or settings.denyReceiver)and ownRoot~=nil and otherRoot~=nil and (otherRoot.Position-ownRoot.Position).Magnitude<settings.guardRange
  if outline.Enabled then
   outline.Adornee=guarded
   local delta=flat(ownRoot.Position-otherRoot.Position)
   local passer=rootOf(settings._denyReceiverBallHandler)
   local goal=settings.denyReceiver and settings._denyReceiverBallHandler~=guarded and passer and passer.Position or goalPosition(attr(guarded,"Goal"))
   local side=goal and flat(goal-otherRoot.Position)
   local alignment=side and side.Magnitude>.01 and delta.Magnitude>.01 and delta.Unit:Dot(side.Unit) or 0
   local error=math.abs(delta.Magnitude-settings.guardGap)
   local good=alignment>.6 and error<1.5 and delta.Magnitude<8
   local moderate=alignment>.1 and delta.Magnitude<10
   outline.OutlineColor=good and Color3.fromRGB(65,210,150) or moderate and Color3.fromRGB(255,190,65) or Color3.fromRGB(245,95,85)
   outline.FillColor=outline.OutlineColor
  end
 end)
 local previousCleanup=featureCleanup
 featureCleanup=function()
  settings._flightBall=nil
  outline:Destroy();previousCleanup()
 end
end)() end

