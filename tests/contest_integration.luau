local settings={}
local mt={}
local function vec(x,y,z)return setmetatable({X=x,Y=y,Z=z},mt)end
mt.__add=function(a,b)return vec(a.X+b.X,a.Y+b.Y,a.Z+b.Z)end
mt.__sub=function(a,b)return vec(a.X-b.X,a.Y-b.Y,a.Z-b.Z)end
mt.__mul=function(a,b)return vec(a.X*b,a.Y*b,a.Z*b)end
mt.__div=function(a,b)return vec(a.X/b,a.Y/b,a.Z/b)end
mt.__index=function(a,k)
 if k=="Magnitude"then return math.sqrt(a.X*a.X+a.Y*a.Y+a.Z*a.Z)end
 if k=="Unit"then return a/a.Magnitude end
 if k=="Dot"then return function(a,b)return a.X*b.X+a.Y*b.Y+a.Z*b.Z end end
end
local Vector3={new=vec,zero=vec(0,0,0)}
local function flat(v)return vec(v.X,0,v.Z)end
local function typeof(v)return type(v)=="table"and v.__instance and "Instance"or type(v)end
local function attr(m,k)return m and m[k]end
-- Shared calibrated contest estimate. Current geometry reproduces the user's test settings.
-- Future positions assume a delayed, accelerating closeout; this is not server feedback.
do (function()
 local C={MultipleDefenderWeight=.25,CloseRangeMultiplier=5.3,JumpMultiplier=1.18,
  DistanceDropPerStud=.93,IgnoreSameTeam=true,CloseRangeDistance=5,DistanceFalloffStart=6.5,
  CloseRangeBonusGain=3,ZeroContestDistance=12,FullContestDistance=2,Curve=1.6,GuardMultiplier=1.12,
  StrengthPercent=87,ReactionTime=.18,Acceleration=60,MaxSpeed=30}
 local M={config=C}
 local samples=setmetatable({},{__mode="k"})
 local function known(v)return v~=nil and v~=0 and v~=""and v~=false end
 local function capped(v,limit)
  v=flat(v or Vector3.zero);return v.Magnitude>limit and v.Unit*limit or v
 end
 function M.pressure(distance,guarding,jumping)
  local closeness=math.clamp((C.ZeroContestDistance-distance)/(C.ZeroContestDistance-C.FullContestDistance),0,1)
  local weight=math.clamp((C.CloseRangeDistance-distance)/(C.CloseRangeDistance-C.FullContestDistance),0,1)
  local multiplier=1+(C.CloseRangeMultiplier-1)*C.CloseRangeBonusGain*weight
  local extra=math.max(0,distance-C.DistanceFalloffStart)
  local score=math.clamp(closeness^C.Curve*multiplier*(1-C.DistanceDropPerStud)^extra,0,1)
  if guarding then score*=C.GuardMultiplier end
  if jumping then score*=C.JumpMultiplier end
  return math.clamp(score,0,1)
 end
 function M.combine(scores)
  table.sort(scores,function(a,b)return a>b end)
  local total=scores[1]or 0
  for i=2,#scores do total+=(1-total)*scores[i]*C.MultipleDefenderWeight end
  return math.clamp(total*C.StrengthPercent,0,100)
 end
 function M.collect(shooter)
  local r=rootOf(shooter);if not r then return {}end
  local list={};local now=os.clock();local team=attr(shooter,"Team");local court=attr(shooter,"CourtNumber")
  local owner=game:GetService("Players"):FindFirstChild(shooter.Name)
  for _,other in ipairs(defenseCharacters())do
   local dr=rootOf(other);local t=attr(other,"Team");local c=attr(other,"CourtNumber")
   local same=C.IgnoreSameTeam and known(team)and known(t)and team==t
   local op=game:GetService("Players"):FindFirstChild(other.Name)
   if C.IgnoreSameTeam and owner and op and owner.Team and not owner.Neutral and not op.Neutral and owner.Team==op.Team then same=true end
   local hum=other:FindFirstChildOfClass("Humanoid")
   if other~=shooter and dr and not same and not(known(court)and known(c)and court~=c)
    and(not hum or hum.Health>0)and math.abs(dr.Position.Y-r.Position.Y)<=9 then
    local velocity=dr.AssemblyLinearVelocity or Vector3.zero;local prev=samples[other]
    if prev and now-prev.at>=.025 and now-prev.at<.3 then
     local measured=(dr.Position-prev.position)/(now-prev.at)
     if measured.Magnitude<60 and flat(velocity).Magnitude<.5 then velocity=measured end
    end
    if not prev or now-prev.at>=.025 then samples[other]={position=dr.Position,at=now}end
    list[#list+1]={character=other,position=dr.Position,velocity=capped(velocity,C.MaxSpeed),speed=hum and hum.WalkSpeed}
   end
  end
  return list
 end
 local function normalized(list,origin)
  local result={}
  for _,d in ipairs(list or {})do
   local instance=typeof(d)=="Instance"
   local r=instance and(d:IsA("BasePart")and d or rootOf(d))or nil
   local character=instance and(r and r.Parent)or(d.character or d.model)
   local position=r and r.Position or not instance and d.position
   if position and math.abs(position.Y-origin.Y)<=9 then
    local hum=character and character:FindFirstChildOfClass("Humanoid")
    local velocity=capped(r and r.AssemblyLinearVelocity or not instance and d.velocity,C.MaxSpeed)
    result[#result+1]={position=position,velocity=velocity,
     speed=math.clamp(math.max(16,velocity.Magnitude,not instance and d.speed or 0,hum and hum.WalkSpeed or 0),16,C.MaxSpeed),
     guarding=character and attr(character,"HoldingG")==true or not instance and d.guarding==true,
     jumping=character and(attr(character,"Action")=="Blocking"or hum and hum.FloorMaterial==Enum.Material.Air and position.Y-origin.Y>.5)or not instance and d.jumping==true}
   end
  end
  return result
 end
 local function score(origin,list)
  local scores={};local gap=math.huge
  for _,d in ipairs(list)do
   local distance=flat(d.position-origin).Magnitude;gap=math.min(gap,distance)
   scores[#scores+1]=M.pressure(distance,d.guarding,d.jumping)
  end
  return M.combine(scores),gap
 end
 local function advance(list,origin,t,dt,reaction)
  for _,d in ipairs(list)do
   if t>=reaction then
    local delta=flat(origin-d.position)
    local wanted=delta.Magnitude>.01 and delta.Unit*math.min(d.speed,delta.Magnitude/math.max(dt,.001))or Vector3.zero
    local change=wanted-d.velocity;local maxChange=C.Acceleration*dt
    if change.Magnitude>maxChange then change=change.Unit*maxChange end
    d.velocity=capped(d.velocity+change,d.speed)
   end
   d.position+=d.velocity*dt
  end
 end
 function M.evaluate(origin,list,horizon,ownVelocity,reaction)
  local data=normalized(list,origin);local velocity=capped(ownVelocity,C.MaxSpeed)
  local time=math.clamp(horizon or 0,0,3);local t=0
  while t<time do
   local dt=math.min(.04,time-t)
   advance(data,origin+velocity*t,t,dt,reaction or C.ReactionTime);t+=dt
  end
  local percent,gap=score(origin+velocity*time,data)
  return percent,{gap=gap,count=#data,horizon=time,estimated=true}
 end
 function M.timeToContest(origin,list,limit,ownVelocity,maxTime)
  local data=normalized(list,origin);local velocity=capped(ownVelocity,C.MaxSpeed)
  local t=0;local maximum=maxTime or 2
  while true do
   local percent=score(origin+velocity*t,data)
   if percent>limit then return t end
   if t>=maximum then return maximum end -- conservative finite planning horizon
   local dt=math.min(.05,maximum-t);advance(data,origin+velocity*t,t,dt,C.ReactionTime);t+=dt
  end
 end
 function M.radius(limit,guarding,jumping)
  local low,high=0,C.ZeroContestDistance
  for _=1,25 do local mid=(low+high)/2
   if M.pressure(mid,guarding,jumping)*C.StrengthPercent>limit then low=mid else high=mid end
  end
  return high
 end
 settings._contestModel=M
end)()end

local M=settings._contestModel
local origin=vec(0,0,0)
local function defender(x,v)return {position=vec(x,0,0),velocity=vec(v or 0,0,0)}end
local sample=defender(9.039024353027344);sample.guarding=true
local snapshot=M.evaluate(origin,{sample},0)
assert(math.abs(snapshot-.016244592083613668)<1e-10,"Must reproduce supplied calibration snapshot")
assert(M.evaluate(origin,{defender(2)},0)==87,"Preserve 87% strength after clamping")
assert(M.evaluate(origin,{defender(12)},0)==0)
local baseline=M.evaluate(origin,{defender(7)},0)
assert(M.evaluate(origin,{defender(7),defender(8)},0)>baseline,"Combine multiple defenders")
local d=defender(7);d.guarding=true
assert(M.evaluate(origin,{d},0)>baseline,"Guard multiplier")
d.guarding=false;d.jumping=true
assert(M.evaluate(origin,{d},0)>baseline,"Jump multiplier")
assert(M.evaluate(origin,{defender(9,-16)},.3)>M.evaluate(origin,{defender(9,16)},.3),"Approaching defender must be riskier than retreating defender")
assert(M.evaluate(origin,{defender(9)},.4,vec(-10,0,0))<M.evaluate(origin,{defender(9)},.4,vec(10,0,0)),"Shooter movement changes future contest")
assert(M.evaluate(origin,{defender(9)},.3,nil,0)>M.evaluate(origin,{defender(9)},.3,nil,.4),"Reaction time affects future closeout")
local r=M.radius(10)
assert(math.abs(M.pressure(r)*87-10)<.001,"Visual radius must match calibrated threshold")
local arrival=M.timeToContest(origin,{defender(9,-16)},10)
assert(arrival>0 and arrival<1,"Time to calibrated contest threshold")
local _,empty=M.evaluate(origin,{},0);assert(empty.count==0,"Missing data remains distinguishable")
local previous=0
for distance=15,0,-.05 do
 local estimate=M.evaluate(origin,{defender(distance)},0)
 assert(estimate>=previous-1e-8 and estimate<=87,"Distance calibration monotonic and bounded")
 previous=estimate
end
local character={HoldingG=true,FindFirstChildOfClass=function()return nil end}
local root={__instance=true,Parent=character,Position=vec(9.039024353027344,0,0),AssemblyLinearVelocity=Vector3.zero,IsA=function(_,kind)return kind=="BasePart"end}
assert(math.abs(M.evaluate(origin,{root},0)-snapshot)<1e-10,"Catch-and-shoot root input matches structured defender input")

local state={}
settings.farmStepbackAggression=70
 local function shotWindow(position,list,ping,stepbackShot)
  if #list==0 then return false,math.huge,0 end
  local limit=stepbackShot and (5+20*math.clamp(settings.farmStepbackAggression or 70,0,100)/100)or 10
  local estimator=settings._contestModel
  local percent,detail=estimator.evaluate(position,list,.3+ping,Vector3.zero)
  local arrival=estimator.timeToContest(position,list,limit,Vector3.zero)
  return percent<=limit,detail.gap,arrival,percent
 end
 local function stepbackWindow(position,list,ping)
  if #list==0 then return false,math.huge,0 end
  local limit=5+20*math.clamp(settings.farmStepbackAggression or 70,0,100)/100
  local duration=state.observedStepbackDuration or .4
  local percent,detail=settings._contestModel.evaluate(position,list,duration+.3+ping,Vector3.zero)
  local arrival=settings._contestModel.timeToContest(position,list,limit,Vector3.zero)
  return percent<=limit,detail.gap,math.max(0,arrival-duration),percent
 end

local ok,gap,arrival,percent=shotWindow(origin,{defender(10,10)},0,false)
assert(ok and percent<=10,"Clear retreating defender permits shot")
assert(not shotWindow(origin,{defender(3)},0,false),"Close defender rejects ordinary shot")
assert(not stepbackWindow(origin,{},0),"No defender data cannot authorize stepback")
settings.farmStepbackAggression=0
local low=5
settings.farmStepbackAggression=100
assert(settings._contestModel.config.StrengthPercent==87,"Aggression never changes calibrated percentages")
print("Shared model and autofarm integration checks passed")
