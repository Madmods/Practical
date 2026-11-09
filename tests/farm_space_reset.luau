-- Executes production selectors with small Vector3 fixtures; no Roblox session needed.
local source=readfile("src/engine/23_smart_dribble.luau")
local farm=readfile("src/engine/26_blacktop_autofarm.luau")
local mt={}
local V={}
function V.new(x,y,z)return setmetatable({X=x,Y=y,Z=z},mt)end
mt.__add=function(a,b)return V.new(a.X+b.X,a.Y+b.Y,a.Z+b.Z)end
mt.__sub=function(a,b)return V.new(a.X-b.X,a.Y-b.Y,a.Z-b.Z)end
mt.__mul=function(a,b)return V.new(a.X*b,a.Y*b,a.Z*b)end
mt.__div=function(a,b)return V.new(a.X/b,a.Y/b,a.Z/b)end
mt.__index=function(a,k)
 if k=="Magnitude"then return math.sqrt(a.X*a.X+a.Y*a.Y+a.Z*a.Z)end
 if k=="Unit"then return a/a.Magnitude end
 if k=="Dot"then return function(a,b)return a.X*b.X+a.Y*b.Y+a.Z*b.Z end end
end
V.zero=V.new(0,0,0)
local function flat(a)return V.new(a.X,0,a.Z)end
local start=assert(source:find("local measuredDribbleProfiles=",1,true))
local finish=assert(source:find("-- END FARM DRIBBLE SELECTOR",start,true))
local selectMove=assert(loadstring("return function(Vector3,flat)\n"..source:sub(start,finish-1).."\nreturn farmDribble end"))()(V,flat)
local function context()
 return {origin=V.zero,target=V.new(-10,0,0),facing=V.new(0,0,-1),right=V.new(1,0,0),
  velocity=V.zero,hand="R",defenders={},ping=.05,allowed=function()return true end,
  now=10,failed={},lastAt=0}
end
local c=context()
local left=assert(selectMove(c))
assert(left.keys=="Z"and left.sprint and left.destination.X< -6,"Right-hand Z must attack actual local left")
c=context();c.hand="L";c.target=V.new(10,0,0)
c.failed.ZX=12;c.failed.ZXC=12;c.failed.XC=12
local right=assert(selectMove(c))
assert(right.keys=="C"and right.sprint and right.destination.X>8,"Left-hand C must attack actual local right")
c=context();c.target=V.new(0,0,10);c.facing=V.new(-1,0,0);c.right=V.new(0,0,-1)
local rotated=assert(selectMove(c))
assert(rotated.keys==left.keys and math.abs(rotated.destination.Z+left.destination.X)<.001,"Rotate with player facing")
c=context();c.allowed=function(p)return p.X>=-.5 end
assert(selectMove(c)==nil,"Do not cross the sideline to reach an attractive target")
c=context();c.defenders={{position=V.new(-4,0,0),velocity=V.zero}}
local blocked=selectMove(c)
assert(not blocked or blocked.keys~="Z"or not blocked.sprint,"Reject a crossover through the defender")
c=context();c.defenders={{position=V.new(-4,0,-6),velocity=V.new(0,0,16)}}
local closing=selectMove(c)
assert(not closing or closing.keys~="Z"or not closing.sprint,"Account for a defender moving into the route")
c=context();c.failed.Z=12
assert(not selectMove(c)or selectMove(c).keys~="Z","Rejected input must back off")
c=context();c.hand="?"
assert(selectMove(c)==nil,"Unknown hand cannot invent a move")
c=context();c.target=V.new(0,0,10)
assert(assert(selectMove(c)).keys=="X","Measured stepback may create reset space")
c=context();c.velocity=V.new(-28,0,0);c.allowed=function(p)return p.X>=-7.1 end
local drift=selectMove(c)
assert(not drift or drift.keys~="Z"or not drift.sprint,"Residual momentum must respect boundaries")

start=assert(farm:find(" local function shouldReset",1,true))
finish=assert(farm:find(" -- END FARM RESET POLICY",start,true))
local reset=assert(loadstring(farm:sub(start,finish-1).."\nreturn shouldReset"))()
local weak={quality="uncertain",futureMargin=-.1}
local since,wants=reset(10,nil,weak,false,false)
assert(since==10 and not wants,"Allow time to create an opening")
since,wants=reset(11.3,since,weak,false,false)
assert(wants,"Marginal lanes must eventually allow an arc reset")
since,wants=reset(12,10,{quality="good",futureMargin=.2},false,false)
assert(since==nil and not wants,"Exploit a genuinely open drive")
since,wants=reset(12,10,{quality="good",futureMargin=-.6},false,false)
assert(wants,"A covered finish must not prevent resetting")
since,wants=reset(12,10,weak,true,false)
assert(since==nil and not wants,"Take a real scoring opportunity")
since,wants=reset(12,nil,{quality="good",futureMargin=.2},false,true)
assert(wants,"Covered landing may explicitly request a reset")
start=assert(farm:find(" local function movePhase",1,true))
finish=assert(farm:find(" -- END FARM MOVE LIFECYCLE",start,true))
local phase=assert(loadstring("return function(flat)\n"..farm:sub(start,finish-1).."\nreturn movePhase end"))()(flat)
local function pending()return {at=10,origin=V.zero,hand="R",move=left,seen=false}end
local p=pending()
assert(phase(p,10.1,V.zero,"R","Idle")=="wait","Do not stack inputs before acknowledgement")
assert(phase(p,10.3,V.zero,nil,"Idle")=="wait","Missing hand attribute is not acknowledgement")
assert(phase(p,11.5,V.zero,"R","Idle")=="timeout","A rejected move cannot lock movement forever")
p=pending()
assert(phase(p,10.2,V.zero,"L","Dribbling")=="wait","Confirmed crossover must finish before driving")
assert(phase(p,10.8,left.destination,"L","Idle")=="complete","Read the lane after native landing")
p=pending()
assert(phase(p,11.5,V.zero,"R","Dribbling")=="timeout","Stuck native action has a bounded wait")
p=pending()
assert(phase(p,10.8,V.new(3,0,0),"R","Idle")=="timeout","Drift opposite the move is not acknowledgement")
p=pending()
assert(phase(p,10.1,V.zero,"R","Dribbling")=="wait")
assert(phase(p,10.22,left.destination,"L","Idle")=="complete","Native completion must not wait for profile duration")
c=context();c.history={{keys=left.keys,package=left.package,at=9.8}}
local varied=assert(selectMove(c))
assert(varied.keys~=left.keys and varied.destination.X<0,"Choose another safe escape instead of repeating Z")
print("Farm space/reset: selector, variety and fast native-completion scenarios passed")
