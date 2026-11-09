-- Exercise the production backstep request; no duplicate implementation.
local source=readfile("src/engine/26_blacktop_autofarm.luau")
local first=assert(source:find(" local function stepbackAndShoot",1,true))
local last=assert(source:find(" -- END FARM QUICK STEPBACK",first,true))
local snippet=source:sub(first,last-1).."\nreturn stepbackAndShoot"
local mt={}
local function v(x,z)return setmetatable({X=x,Y=0,Z=z},mt)end
mt.__sub=function(a,b)return v(a.X-b.X,a.Z-b.Z)end
mt.__mul=function(a,b)return v(a.X*b,a.Z*b)end
mt.__index=function(a,k)if k=="Magnitude"then return math.sqrt(a.X*a.X+a.Z*a.Z)end end
local function fixture()
 local own={Action="Idle",CanDribble=true}
 local root={Position=v(0,0),CFrame={LookVector=v(0,-1)}}
 local events={}
 local state={};local plan={}
 local globals=setmetatable({
  model=function()return own end,rootOf=function()return root end,
  holdsBall=function()return true end,attr=function(o,k)return o[k]end,
  settings={farmShoot=true,farmDunkRange=12,farmShotRange=40},
  api={GetAutoTime=function()return true end},busy={Shooting=true},
  rimFor=function()return v(0,-25)end,flat=function(a)return a end,
  P={GetNetworkPing=function()return 0 end},safeMove=function()return true end,
  stepbackWindow=function()return true end,defenders=function()return {}end,
  shotRange=function()return 40 end,
  plantFeet=function()plan.dir=v(0,0)end,
  Vector3={zero=v(0,0)},state=state,plan=plan,
 },{__index=_G})
 local init=assert(loadstring(snippet));setfenv(init,globals)
 local service={Dribble={Fire=function(_,key)events[#events+1]=key end},
  Shoot={Fire=function(_,args)assert(args.Shoot and args.Input=="E");events[#events+1]="E"end}}
 return init(),globals,service,events,own
end
local shoot,g,service,events=fixture()
assert(shoot(service,10))
assert(#events==1 and events[1]=="XX","Wait for native XX completion before E to avoid a fadeaway")
assert(not g.shotStart and g.retreat.at==10,"Track the backstep until the landing shot")
assert(g.plan.dir.Magnitude==0,"Backstep owns movement")
shoot,g,service,events=fixture();g.stepbackWindow=function()return false end
assert(not shoot(service,10)and #events==0,"Covered projected landing must not queue a shot")
shoot,g,service,events=fixture();g.safeMove=function()return false end
assert(not shoot(service,10)and #events==0,"Out-of-bounds backstep must not queue a shot")
shoot,g,service,events=fixture();service.Dribble.Fire=function()error("rejected")end
assert(not shoot(service,10)and #events==0 and not g.shotStart,"Failed XX must not send E")
shoot,g,service,events=fixture();g.settings.farmShoot=false
assert(not shoot(service,10)and #events==0,"Respect shots disabled")
shoot,g,service,events=fixture();g.api.GetAutoTime=function()return false end
assert(not shoot(service,10)and #events==0,"Require Auto time")
local own
shoot,g,service,events,own=fixture();own.Action="Dribbling"
assert(not shoot(service,10)and #events==0,"Do not stack a backstep into an unfinished move")
assert(not shoot({},10),"Missing controller fails cleanly")

first=assert(source:find(" local function syncSprint",1,true))
last=assert(source:find(" -- END FARM SPRINT",first,true))
local sprint=assert(loadstring("return function(plan)\n"..source:sub(first,last-1).."\nreturn syncSprint end"))()({})
local calls={};service={Sprint={Fire=function(_,value)calls[#calls+1]=value end}}
sprint(service,false,0);assert(#calls==0,"Do not release sprint we never owned")
sprint(service,true,1);sprint(service,true,1.1);assert(#calls==1 and calls[1],"Start sprint without spamming each frame")
sprint(service,true,1.4);assert(#calls==2 and calls[2],"Refresh native sprint on a sustained drive")
sprint(service,false,1.5);sprint(service,false,1.6)
assert(#calls==3 and calls[3]==false,"Release sprint once for a queued shot")
sprint(service,true,1.7);assert(#calls==4 and calls[4],"Resume sprint immediately after the shot/dribble")
print("Production quick-stepback and native sprint transitions passed")
