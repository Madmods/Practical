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

local settings={farmRandomThrees=true,farmDunkRange=12,farmShotRange=40,smartDribbleThreeDistance=0}
local state,plan,busy,rimHits={},{},{},{}
local own={};local root={Position=Vector3.new(0,0,25)}
local hasBall=true
local function holdsBall()return hasBall end
local function attr(_,key)if key=="Hand"then return "R"end;if key=="Action"then return "Idle"end end
local function ballPart()return nil end
local function rimFor()return Vector3.new(0,0,0)end
local function releaseShot(service)service.Shoot:Fire({Shoot=false})end
local rng={NextNumber=function(_,a,b)return a or .1 end}
local api={GetAutoTime=function()return true end}
local shotStart,lastShot,shotSawAction,spaceHeld,clearBall,guardError
local calls={}
local service={Dribble={Fire=function(_,keys,sprint)calls[#calls+1]={keys=keys,sprint=sprint}end},Shoot={Fire=function(_,data)calls[#calls+1]=data end}}
 local threeRun=nil
 local threeMatch=false
 local threeBudget,threeUsed,threeNext=0,0,0
 local function cancelThree(service)
  if threeRun and threeRun.stage=="shot"then releaseShot(service)end
  threeRun=nil
 end
 local function randomThree(own,root,now,service)
  if not settings.farmRandomThrees or not holdsBall(own)then cancelThree(service);return false end
  if clearBall or rimHits[ballPart(attr(own,"Basketball"))]then cancelThree(service);return false end
  if not service or not service.Dribble or not service.Shoot then return false end
  if threeRun then
   plan.dir=Vector3.zero;plan.reason="Autofarm: optional three combo"
   if now<threeRun.next then return true end
   if threeRun.stage=="doubleX"then
    local ok=pcall(function()service.Dribble:Fire("XX",true,Vector3.zero)end)
    if not ok then cancelThree(service);state.status="Three combo: double-X input failed";return true end
    threeRun.stage="pressE";threeRun.next=now+.05
    state.status="Three combo: double X";return true
   elseif threeRun.stage=="pressE"then
    local ok=pcall(function()service.Shoot:Fire({Shoot=true,Input="E"})end)
    if not ok then cancelThree(service);state.status="Three combo: E input failed";return true end
    shotStart=now;lastShot=now;shotSawAction=false
    threeRun.stage="shot";threeRun.next=now+1.8
    state.status="Three combo: E held for Auto time";return true
   end
   releaseShot(service);threeRun=nil;return true
  end
  if threeUsed>=threeBudget or now<threeNext then return false end
  local hoop=rimFor(own,root,now)
  if not hoop then return false end
  local distance=flat(root.Position-hoop).Magnitude
  local arc=settings.smartDribbleThreeDistance or 0
  -- Do not interrupt an immediate finish. Use a detected arc when available.
  if distance<=settings.farmDunkRange+4 or distance>settings.farmShotRange or(arc>0 and distance<arc-5)then return false end
  local action=attr(own,"Action")
  if busy[action]or action=="Dribbling"or attr(own,"CanDribble")==false or spaceHeld or shotStart then return false end
  local hand=attr(own,"Hand")
  if hand~="R"and hand~="L"then return false end
  if not(api.GetAutoTime and api.GetAutoTime())then return false end
  threeNext=now+rng:NextNumber(12,25)
  if rng:NextNumber()>.7 then return false end
  if api.CancelCombo then api.CancelCombo()end
  local ok=pcall(function()service.Dribble:Fire(hand=="R"and "Z"or "C",true,flat(hoop-root.Position).Unit)end)
  if not ok then state.status="Three combo: crossover input failed";return false end
  threeUsed=threeUsed+1;threeRun={stage="doubleX",next=now+.06}
  state.threeAttempts=threeUsed;state.threeBudget=threeBudget
  plan.dir=flat(hoop-root.Position).Unit;plan.reason="Autofarm: optional three crossover";guardError=60
  state.status=string.format("Three attempt %d/%d: sprint crossover",threeUsed,threeBudget)
  return true
 end

threeBudget=2;threeNext=0
assert(randomThree(own,root,10,service));assert(calls[1].keys=="Z"and calls[1].sprint)
randomThree(own,root,10.07,service);assert(calls[2].keys=="XX"and calls[2].sprint)
randomThree(own,root,10.13,service);assert(calls[3].Shoot==true and calls[3].Input=="E")
settings.farmRandomThrees=false;randomThree(own,root,10.14,service);assert(calls[4].Shoot==false and threeRun==nil)
settings.farmRandomThrees=true;threeUsed=threeBudget;threeNext=0;shotStart=nil
assert(not randomThree(own,root,99,service));assert(#calls==4)
threeUsed=0;hasBall=false;assert(not randomThree(own,root,99,service))
print("PASS: sprint crossover, sprint XX, immediate E, cancel releases shot, per-game cap, possession guard")
