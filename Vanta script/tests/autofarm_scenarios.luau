local mt={}
local V={}
function V.new(x,y,z)return setmetatable({X=x,Y=y,Z=z},mt)end
mt.__add=function(a,b)return V.new(a.X+b.X,a.Y+b.Y,a.Z+b.Z)end
mt.__sub=function(a,b)return V.new(a.X-b.X,a.Y-b.Y,a.Z-b.Z)end
mt.__mul=function(a,n)return V.new(a.X*n,a.Y*n,a.Z*n)end
mt.__div=function(a,n)return V.new(a.X/n,a.Y/n,a.Z/n)end
mt.__index=function(a,k)
 if k=="Magnitude"then return math.sqrt(a.X*a.X+a.Y*a.Y+a.Z*a.Z)end
 if k=="Unit"then return a/a.Magnitude end
 if k=="Dot"then return function(a,b)return a.X*b.X+a.Y*b.Y+a.Z*b.Z end end
end
local Vector3=V
local function flat(v)return V.new(v.X,0,v.Z)end
local settings={farmShotRange=30}
local lastHeading=nil
local courtBounds={minX=-25,maxX=25,minZ=-10,maxZ=40}
 local function segmentGap(point,a,b)
  local ab=flat(b-a);local length2=ab:Dot(ab)
  if length2<.01 then return flat(point-a).Magnitude end
  local t=math.clamp(flat(point-a):Dot(ab)/length2,0,1)
  return flat(point-(a+ab*t)).Magnitude
 end
 local function chooseHeading(root,hoop,list,wantRim,speed)
  local origin=root.Position
  local toHoop=flat(hoop-origin)
  local hoopDir=toHoop.Magnitude>.1 and toHoop.Unit or Vector3.new(0,0,-1)
  local best,bestScore
  for i=0,15 do
   local angle=i*math.pi/8
   local dir=Vector3.new(math.cos(angle),0,math.sin(angle))
   local spot=origin+dir*math.min(speed*.45,9)
   local progress=toHoop.Magnitude-flat(hoop-spot).Magnitude
   local score=progress*wantRim
   local minGap=10
   for _,d in ipairs(list)do
    minGap=math.min(minGap,flat(spot-d.predicted).Magnitude)
    -- Compare simultaneous positions along the route; a defender behind us should not
    -- make every forward path look blocked just because they are close to the origin.
    for sample=1,4 do
     local t=sample*.1
     local ours=origin+dir*speed*t
     local theirs=d.position+d.velocity*t
     local gap=flat(ours-theirs).Magnitude
     if gap<2.6 then score=score-(2.6-gap)*5 end
    end
   end
   score=score+math.min(minGap,7)*.45
   if flat(spot-hoop).Magnitude>settings.farmShotRange+6 then score=score-6 end
   if courtBounds and(spot.X<courtBounds.minX or spot.X>courtBounds.maxX or spot.Z<courtBounds.minZ or spot.Z>courtBounds.maxZ)then score=score-8 end
   if lastHeading then score=score+dir:Dot(lastHeading)*1.2 end
   if not bestScore or score>bestScore then best=dir;bestScore=score end
  end
  lastHeading=best
  return best
 end
 local function canBeat(root,hoop,list,runSpeed,walk)
  local origin=root.Position
  local toHoop=flat(hoop-origin);local distance=toHoop.Magnitude
  if distance<2 or #list==0 then return true end
  local forward=toHoop.Unit
  local target=hoop-forward*2
  for i=1,12 do
   local point=origin+(target-origin)*(i/12)
   local playerTime=flat(point-origin).Magnitude/runSpeed
   for _,d in ipairs(list)do
    local speed=math.clamp(math.max(d.velocity.Magnitude,walk),10,26)
    local defenderTime=math.max(0,flat(point-d.position).Magnitude-2.1)/speed
    if defenderTime<=playerTime+.15 then return false end
   end
  end
  return true
 end






 local function farmChoice(c)
  if c.finish then return "finish" end
  if c.confirmedStepback and c.stepbackWindow and c.shotEnabled and c.inRange and c.open and c.ready and not c.dribbling then return "shoot" end
  if c.smartRetreat and c.stepback and not c.beaten then return "stepback" end
  if c.beaten or c.laneClear then return "drive" end
  -- Ordinary offense keeps creating a drive. Only a completed stepback can choose a jumper.
  if c.stepback then return "stepback" end
  return "create"
 end
  local function stationaryShot(speed,vertical,action,canMove,now,lastMotion)
  local moving=speed>.35 or math.abs(vertical)>1 or action=="Dribbling" or canMove==false
  if moving then return false,now end
  return true,lastMotion -- No extra timer after native movement has actually stopped.
 end

local count=0
local function check(name,condition)assert(condition,name);count+=1;print("PASS",name)end
local base={finish=false,shotEnabled=true,inRange=true,open=true,ready=true,dribbling=false,beaten=false,laneClear=true,stepback=false}
check("clear lane prioritizes dunk drive over jumper",farmChoice(base)=="drive")
base.finish=true;check("viable finish takes priority near rim",farmChoice(base)=="finish")
base.finish=false;base.dribbling=true;check("do not shoot during stepback animation",farmChoice(base)=="drive")
base.dribbling=false;base.open=false;base.beaten=true;check("beaten defender means attack",farmChoice(base)=="drive")
base.beaten=false;base.laneClear=false;base.stepback=true;check("pressure with valid retreat creates stepback",farmChoice(base)=="stepback")
base.stepback=false;check("blocked lane without safe retreat creates angle",farmChoice(base)=="create")
base.open=true;base.ready=false;check("shot readiness still respected",farmChoice(base)=="create")
base.ready=true;base.inRange=false;check("out of range never forces shot",farmChoice(base)=="create")
local root={Position=V.new(0,0,0)};local hoop=V.new(0,0,25)
check("undefended path wins race",canBeat(root,hoop,{},18,13.5))
local blocker={position=V.new(0,0,8),predicted=V.new(0,0,8),velocity=V.new(0,0,0)}
check("defender in lane blocks straight race",not canBeat(root,hoop,{blocker},18,13.5))
local dir=chooseHeading(root,hoop,{blocker},1.8,18)
check("blocked lane chooses forward progress",dir.Z>0)
check("blocked lane offsets around defender",math.abs(dir.X)>.1)
lastHeading=nil
local clear=chooseHeading(root,hoop,{},1.8,18)
check("empty lane attacks basket",clear.Z>.9)
print("All "..count.." scenario checks passed")

local ready,last=stationaryShot(2,0,"Idle",true,1,0)
check("slow backward glide blocks shot",not ready and last==1)
ready,last=stationaryShot(0,0,"Idle",true,1.1,last)
check("stationary native state has no extra timer",ready)
ready,last=stationaryShot(0,0,"Idle",true,1.25,last)
check("sustained planted feet authorize shot",ready)
ready=stationaryShot(0,4,"Idle",true,2,0)
check("airborne player cannot start jumper",not ready)
ready=stationaryShot(0,0,"Dribbling",true,2,0)
check("dribble animation blocks jumper even at zero speed",not ready)
base.laneClear=false;base.beaten=false;base.open=true;base.ready=true;base.inRange=true
check("blocked drive does not trigger an unplanned jumper",farmChoice(base)=="create")
print("Final total: "..count.." passed")

base.confirmedStepback=true;base.laneClear=true;base.stepbackWindow=true
check("completed stepback takes open shot before resuming drive",farmChoice(base)=="shoot")
base.dribbling=true
check("stepback window does not bypass native dribble lock",farmChoice(base)=="drive")
base.dribbling=false;base.stepbackWindow=false
check("ordinary open lane still prioritizes dunk drive",farmChoice(base)=="drive")
print("Final scenario total: "..count)

base.stepback=true;base.smartRetreat=true;base.beaten=false;base.laneClear=true
check("safe spaced-defender pull-up overrides an open lane",farmChoice(base)=="stepback")
base.smartRetreat=false
check("unsafe pull-up keeps attacking",farmChoice(base)=="drive")
base.smartRetreat=true;base.beaten=true
check("beaten defender retains dunk priority",farmChoice(base)=="drive")
base.beaten=false;base.finish=true
check("in-range finish wins over pull-up",farmChoice(base)=="finish")
print("Final scenario total: "..count)

base.finish=false;base.beaten=false;base.smartRetreat=false;base.stepback=false;base.laneClear=false;base.stepbackWindow=true;base.confirmedStepback=false
check("a window without a confirmed stepback cannot shoot",farmChoice(base)~="shoot")
base.confirmedStepback=true;base.open=false
check("confirmed stepback cannot shoot into a closing guard",farmChoice(base)~="shoot")
print("Final scenario total: "..count)
