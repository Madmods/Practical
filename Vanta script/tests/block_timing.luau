local settings={blockDelay=.08,blockRange=5};local featureCleanup=function()end
local mt={}
local function v(x,y,z)return setmetatable({X=x,Y=y,Z=z},mt)end
mt.__sub=function(a,b)return v(a.X-b.X,a.Y-b.Y,a.Z-b.Z)end
mt.__index=function(a,k)
 if k=="Magnitude"then return math.sqrt(a.X*a.X+a.Y*a.Y+a.Z*a.Z)end
 if k=="Unit"then return v(a.X/a.Magnitude,a.Y/a.Magnitude,a.Z/a.Magnitude)end
 if k=="Dot"then return function(a,b)return a.X*b.X+a.Y*b.Y+a.Z*b.Z end end
end
local function flat(a)return v(a.X,0,a.Z)end
local function attr(a,k)return a[k]end
local function typeof(a)return type(a)=="table"and a.instance and "Instance"or type(a)end
local function goalPosition(a)return a and a.position end
local ball={instance=true,position=v(3,2,0)}
local shooter={Basketball=ball}
local root={Position=v(0,0,0),CFrame={LookVector=v(1,0,0)}}
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

assert(not settings._blockReady(shooter,0,root,.01))
ball.position=v(3,4,0)
assert(not settings._blockReady(shooter,0,root,.2),"Rising held ball is wind-up, not release")
ball.position=v(3,5,0)
assert(not settings._blockReady(shooter,0,root,.3),"Long wind-up still cannot trigger")
shooter.Basketball=nil
assert(settings._blockReady(shooter,0,root,.32),"Observed ownership release permits block")
ball.position=v(-3,4,0)
assert(not settings._blockReady(shooter,0,root,.34),"No backward block")
ball.position=v(8,4,0)
assert(not settings._blockReady(shooter,0,root,.36),"Respect reach")
ball.position=v(3,1,0)
assert(not settings._blockReady(shooter,0,root,.38),"No low-ball dive")
featureCleanup();assert(settings._blockReady==nil)
print("Release-gated block checks passed")
