local now=0
local os={clock=function()return now end}
local settings={};local numericLimits={};local featureCleanup=function()end
local state={}
local mt={}
local function vec(x,z)return setmetatable({X=x,Y=0,Z=z},mt)end
mt.__add=function(a,b)return vec(a.X+b.X,a.Z+b.Z)end
mt.__unm=function(a)return vec(-a.X,-a.Z)end
mt.__sub=function(a,b)return vec(a.X-b.X,a.Z-b.Z)end
mt.__mul=function(a,b)return vec(a.X*b,a.Z*b)end
mt.__index=function(v,k)
 if k=="Magnitude"then return math.sqrt(v.X*v.X+v.Z*v.Z)end
 if k=="Unit"then local m=v.Magnitude;return vec(v.X/m,v.Z/m)end
 if k=="Lerp"then return function(a,b,t)return a+(b-a)*t end end
 if k=="Dot"then return function(a,b)return a.X*b.X+a.Z*b.Z end end
end
local function flat(v)return v end

local Vector3={zero=vec(0,0),new=function(x,y,z)return vec(x,z)end}
local Random={new=function()return {NextNumber=function(_,a,b)if a then return (a+b)/2 end;return .6 end}end}
-- Guard positioning with sampled reactions, bounded turning and occasional idle stance changes.
do (function()
 settings.guardHuman=true;settings.guardHumanLevel=60;settings.guardIdleMovement=true
 settings.rimPress=true;settings.rimPressRange=18;settings.rimPressGap=1.3
 numericLimits.guardHumanLevel={0,100};numericLimits.rimPressRange={8,30};numericLimits.rimPressGap={.75,3}
 local aim,observed,identity,lastAt,nextRead,holding=nil,nil,nil,0,0,false
 local rng=Random.new()
 local idle={offset=Vector3.zero,target=Vector3.zero,nextAt=0,quietSince=nil}
 local function resetIdle(now)
  idle.offset=Vector3.zero;idle.target=Vector3.zero;idle.quietSince=nil
  idle.nextAt=(now or os.clock())+rng:NextNumber(.6,1.2)
  settings._humanIdleActive=false
 end
 local function idleOffset(desired,handlerPosition,ownPosition,velocity,basketDirection,urgent,level,now,dt)
  if urgent or level==0 or not settings.guardIdleMovement or not handlerPosition or not ownPosition
   or not basketDirection or flat(velocity or Vector3.zero).Magnitude>2
   or flat(desired-ownPosition).Magnitude>4.5 or flat(ownPosition-handlerPosition).Magnitude<2.8 then
   resetIdle(now);return Vector3.zero
  end
  idle.quietSince=idle.quietSince or now
  if now-idle.quietSince>.4 and now>=idle.nextAt then
   local tangent=Vector3.new(-basketDirection.Z,0,basketDirection.X)
   local amplitude=math.max(1.25,(settings.guardDeadzone or .65)+.65)+.4*level
   local direction=rng:NextNumber()<.5 and -1 or 1
   local nextTarget=idle.target.Magnitude>.5 and rng:NextNumber()<.3 and Vector3.zero or tangent*direction*amplitude
   if (nextTarget-idle.target).Magnitude<.75 then nextTarget=idle.target.Magnitude>.5 and Vector3.zero or -tangent*direction*amplitude end
   idle.target=nextTarget
   idle.nextAt=now+rng:NextNumber(1.4,3)
   holding=false
  end
  idle.offset=idle.offset:Lerp(idle.target,1-math.exp(-dt*7))
  settings._humanIdleActive=idle.offset.Magnitude>.05
  return idle.offset
 end
 settings._humanReset=function()
  aim=nil;observed=nil;identity=nil;lastAt=0;nextRead=0;holding=false;resetIdle()
 end
 settings._humanize=function(desired,dt,handler,urgent,handlerPosition,ownPosition,velocity,basketDirection)
  local level=math.clamp(settings.guardHumanLevel or 0,0,100)/100
  local now=os.clock();dt=math.clamp(dt or 1/60,0,.1)
  if not aim or identity~=handler or now-lastAt>.4 or flat(aim-desired).Magnitude>12 then
   resetIdle(now);aim=desired;observed=desired;identity=handler;holding=false;nextRead=now+(urgent and .025 or .05+.12*level)
  end
  lastAt=now
  if level==0 then resetIdle(now);aim=desired;observed=desired;return desired end
  -- Snapshot tracking independently from the small lateral idle offset.
  if now>=nextRead then
   nextRead=now+(urgent and .025+.025*level or .05+.12*level)
   if flat(desired-observed).Magnitude>(urgent and .04 or .12+.12*level)then observed=desired end
  end
  aim=aim:Lerp(observed,1-math.exp(-dt*(urgent and 18-6*level or 12-7*level)))
  return aim+idleOffset(desired,handlerPosition,ownPosition,velocity,basketDirection,urgent,level,now,dt)
 end
 settings._humanHold=function(distance,deadzone,urgent)
  local level=math.clamp(settings.guardHumanLevel or 0,0,100)/100
  if settings._humanIdleActive then return distance<=.25 end
  if level==0 then return distance<=deadzone end
  local enter=deadzone+(urgent and .05 or .2)*level
  local leave=enter+(urgent and .12 or .45+.5*level)
  if distance<=enter then holding=true elseif distance>=leave then holding=false end
  return holding
 end
 settings._humanSteer=function(current,desired,dt,urgent)
  local level=math.clamp(settings.guardHumanLevel or 0,0,100)/100
  if desired.Magnitude<.001 then return Vector3.zero end
  desired=flat(desired).Unit
  if level==0 or current.Magnitude<.001 then return desired end
  current=flat(current).Unit
  local angle=math.atan2(current.X*desired.Z-current.Z*desired.X,current:Dot(desired))
  local maxTurn=(urgent and 15-5*level or 11-7*level)*math.clamp(dt or 1/60,0,.1)
  local turn=math.clamp(angle,-maxTurn,maxTurn)
  local c,s=math.cos(turn),math.sin(turn)
  return Vector3.new(current.X*c-current.Z*s,0,current.X*s+current.Z*c)
 end
 local previous=featureCleanup
 featureCleanup=function()
  settings._humanReset();settings._humanize=nil;settings._humanHold=nil;settings._humanSteer=nil;settings._humanReset=nil;settings._humanIdleActive=nil
  previous()
 end
end)() end
local desired=vec(3.2,0);local handler=vec(0,0);local velocity=vec(0,0);local side=vec(1,0)
local largest=0
for i=1,240 do
 now=i/60
 local point=settings._humanize(desired,1/60,"A",false,handler,desired,velocity,side)
 local offset=point-desired
 assert(math.abs(offset.X)<.00001,"Idle movement must remain lateral")
 assert(offset.Magnitude<=1.8,"Default shuffle must stay bounded")
 largest=math.max(largest,offset.Magnitude)
end
assert(largest>.8,"Idle handler should produce visible stance adjustment")
now+=1/60
assert((settings._humanize(desired,1/60,"A",true,handler,desired,velocity,side)-desired).Magnitude<.0001,"Urgency cancels idle offset")
assert(not settings._humanIdleActive)
for i=1,240 do
 now+=1/60
 assert((settings._humanize(desired,1/60,"A",false,handler,desired,vec(5,0),side)-desired).Magnitude<.0001,"Moving handler cancels idle shuffle")
end
settings.guardIdleMovement=false
for i=1,240 do now+=1/60;assert((settings._humanize(desired,1/60,"A",false,handler,desired,velocity,side)-desired).Magnitude<.0001)end
settings._humanReset();assert(not settings._humanIdleActive)
featureCleanup();assert(settings._humanize==nil)
print("Idle shuffle bounds, direction, cancellation and cleanup checks passed")
