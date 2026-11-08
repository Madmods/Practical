-- Run from the project root with readfile/loadstring available.
-- Tests the actual planner's arrival model, not a copied implementation.
local source=readfile("src/engine/26_blacktop_autofarm.luau")
local first=assert(source:find("local function interceptTime",1,true))
local last=assert(source:find(" -- END FARM ARRIVAL",first,true))
local mt={}
local function v(x,z)return setmetatable({X=x,Z=z},mt)end
mt.__sub=function(a,b)return v(a.X-b.X,a.Z-b.Z)end
mt.__index=function(a,k)
 if k=="Magnitude"then return math.sqrt(a.X*a.X+a.Z*a.Z)end
 if k=="Unit"then local n=math.sqrt(a.X*a.X+a.Z*a.Z);return v(a.X/n,a.Z/n)end
 if k=="Dot"then return function(a,b)return a.X*b.X+a.Z*b.Z end end
end
local intercept=assert(loadstring("return function(flat)\n"..source:sub(first,last-1).."\nreturn interceptTime end"))()(function(a)return a end)
local origin,target=v(0,0),v(12,0)
local toward=intercept(origin,v(16,0),target,0)
local stopped=intercept(origin,v(0,0),target,0)
local away=intercept(origin,v(-16,0),target,0)
assert(toward<stopped and stopped<away,"Momentum reversal must cost time")
assert(intercept(origin,v(0,0),v(1,0),0)==0,"Defender already in reach")
assert(intercept(origin,v(0,0),target,.1)<stopped,"Higher latency requires more clearance")
assert(intercept(origin,v(0,0),target,10)==intercept(origin,v(0,0),target,.35),"Latency reserve must be bounded")
assert(intercept(origin,v(26,0),v(2.2,0),.35)>=0,"Arrival cannot be negative")
assert(math.abs(toward-intercept(origin,v(0,16),v(0,12),0))<1e-9,"Rotation must not alter the estimate")
print("Smart farm arrival scenarios passed")
