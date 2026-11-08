local state={}
local mt={}
local function vec(x,z)return setmetatable({X=x,Y=0,Z=z},mt)end
mt.__add=function(a,b)return vec(a.X+b.X,a.Z+b.Z)end
mt.__sub=function(a,b)return vec(a.X-b.X,a.Z-b.Z)end
mt.__mul=function(a,b)return vec(a.X*b,a.Z*b)end
mt.__index=function(v,k)
 if k=="Magnitude"then return math.sqrt(v.X*v.X+v.Z*v.Z)end
 if k=="Unit"then local m=v.Magnitude;return vec(v.X/m,v.Z/m)end
 if k=="Dot"then return function(a,b)return a.X*b.X+a.Z*b.Z end end
end
local function flat(v)return v end
 local function shotWindow(position,list,ping)
  if #list==0 then return false,math.huge,0 end
  local gap,arrival=math.huge,math.huge
  for _,d in ipairs(list)do
   local offset=flat(position-d.position);local distance=offset.Magnitude
   local speed=math.clamp(math.max(16,flat(d.velocity).Magnitude),16,26)
   local closing=distance>.01 and math.clamp(d.velocity:Dot(offset.Unit),-speed,speed)or 0
   local remaining=math.max(0,distance-3.5)
   local acceleration=60
   local accelerateFor=(speed-closing)/acceleration
   local accelerationDistance=closing*accelerateFor+.5*acceleration*accelerateFor^2
   local travel=0
   if remaining>0 then
    if remaining<=accelerationDistance then travel=(-closing+math.sqrt(closing^2+2*acceleration*remaining))/acceleration
    else travel=accelerateFor+(remaining-accelerationDistance)/speed end
   end
   gap=math.min(gap,distance);arrival=math.min(arrival,travel)
  end
  return gap>=7 and arrival>.58+ping,gap,arrival
 end
 -- Planning estimate only: reaction plus momentum reversal, not instant full-speed pursuit.
 local function stepbackWindow(position,list,ping)
  if #list==0 then return false,math.huge,0 end
  local reaction=.18;local moveDuration=state.observedStepbackDuration or .4;local acceleration=60
  local minimumGap,minimumArrival=math.huge,math.huge
  for _,d in ipairs(list)do
   local projected=d.position+d.velocity*reaction
   local delta=flat(position-projected);local gap=delta.Magnitude
   local speed=math.clamp(math.max(16,d.velocity.Magnitude),16,26)
   local initial=gap>.01 and math.clamp(d.velocity:Dot(delta.Unit),-speed,speed)or 0
   local remaining=math.max(0,gap-3.5)
   local accelerateFor=(speed-initial)/acceleration
   local accelerationDistance=initial*accelerateFor+.5*acceleration*accelerateFor^2
   local travel
   if remaining==0 then travel=0
   elseif remaining<=accelerationDistance then
    travel=(-initial+math.sqrt(initial^2+2*acceleration*remaining))/acceleration
   else travel=accelerateFor+(remaining-accelerationDistance)/speed end
   minimumArrival=math.min(minimumArrival,math.max(0,reaction+travel-moveDuration))
   minimumGap=math.min(minimumGap,gap)
  end
  return minimumGap>=7 and minimumArrival>.4+ping,minimumGap,minimumArrival
 end

local point=vec(0,0)
local function defender(distance,speed)return {{position=vec(distance,0),velocity=vec(speed,0)}}end
local _,_,away=shotWindow(point,defender(12,12),0)
local _,_,toward=shotWindow(point,defender(12,-12),0)
assert(away>toward)
assert(shotWindow(point,defender(12,12),0))
assert(not shotWindow(point,defender(5,0),0))
assert(not shotWindow(point,{},0))
assert(stepbackWindow(point,defender(15,12),0))
assert(not stepbackWindow(point,defender(5,-12),0))
print("Closeout model: 6 scenarios passed")
