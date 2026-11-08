local now=0
local os={clock=function()return now end}
local settings={};local numericLimits={};local featureCleanup=function()end
local state={}
local mt={}
local function vec(x,z)return setmetatable({X=x,Y=0,Z=z},mt)end
mt.__add=function(a,b)return vec(a.X+b.X,a.Z+b.Z)end
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
-- Stable guard positioning: sampled reactions, no idle drift, bounded turning.
do (function()
 settings.guardHuman=true;settings.guardHumanLevel=60
 settings.rimPress=true;settings.rimPressRange=18;settings.rimPressGap=1.3
 numericLimits.guardHumanLevel={0,100};numericLimits.rimPressRange={8,30};numericLimits.rimPressGap={.75,3}
 local aim,observed,identity,lastAt,nextRead,holding=nil,nil,nil,0,0,false
 settings._humanReset=function()
  aim=nil;observed=nil;identity=nil;lastAt=0;nextRead=0;holding=false
 end
 settings._humanize=function(desired,dt,handler,urgent)
  local level=math.clamp(settings.guardHumanLevel or 0,0,100)/100
  local now=os.clock();dt=math.clamp(dt or 1/60,0,.1)
  if not aim or identity~=handler or now-lastAt>.4 or flat(aim-desired).Magnitude>12 then
   aim=desired;observed=desired;identity=handler;holding=false;nextRead=now+(urgent and .025 or .05+.12*level)
  end
  lastAt=now
  if level==0 then aim=desired;observed=desired;return desired end
  -- Snapshot meaningful changes; a stationary handler never creates artificial wandering.
  if now>=nextRead then
   nextRead=now+(urgent and .025+.025*level or .05+.12*level)
   if flat(desired-observed).Magnitude>(urgent and .04 or .12+.12*level)then observed=desired end
  end
  aim=aim:Lerp(observed,1-math.exp(-dt*(urgent and 18-6*level or 12-7*level)))
  return aim
 end
 settings._humanHold=function(distance,deadzone,urgent)
  local level=math.clamp(settings.guardHumanLevel or 0,0,100)/100
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
  settings._humanReset();settings._humanize=nil;settings._humanHold=nil;settings._humanSteer=nil;settings._humanReset=nil
  previous()
 end
end)() end

local position=vec(3,4)
for i=1,120 do now=i/60;assert((settings._humanize(position,1/60,"A",false)-position).Magnitude<.00001,"Stationary handler must not cause wandering")end
settings._humanReset();now=3
settings._humanize(vec(0,0),1/60,"A",false)
now=3.01
assert(settings._humanize(vec(6,0),.01,"A",false).Magnitude==0,"Changes should wait for reaction sample")
now=3.15
local moved=settings._humanize(vec(6,0),.1,"A",false).Magnitude
assert(moved>0 and moved<6,"React smoothly after sampling")
local change=settings._humanize(vec(10,0),.01,"B",false)
assert(change.X==10,"New target must discard stale aim")
assert(settings._humanHold(.4,.45,false))
assert(settings._humanHold(.8,.45,false),"No oscillation at hold threshold")
assert(not settings._humanHold(2,.45,false),"Meaningful separation resumes movement")
local normal=settings._humanSteer(vec(1,0),vec(-1,0),1/60,false)
assert(math.abs(normal.Magnitude-1)<.00001,"Keep native movement magnitude")
assert(math.acos(normal.X)<.12,"Bound turn per frame")
local urgent=settings._humanSteer(vec(1,0),vec(-1,0),1/60,true)
assert(math.acos(urgent.X)>math.acos(normal.X),"Urgent defense turns faster")
settings.guardHumanLevel=0
assert(settings._humanSteer(vec(1,0),vec(-1,0),1/60,false).X==-1,"Zero softness retains direct control")
featureCleanup()
assert(settings._humanize==nil and settings._humanSteer==nil,"Unload removes callbacks")
print("Guard reaction, idle, turning and cleanup checks passed")
