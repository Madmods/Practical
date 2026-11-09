-- Execute production range, request, priority and landing logic with small Roblox fixtures.
local source=readfile("src/engine/26_blacktop_autofarm.luau")
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
local function flat(v)return V.new(v.X,0,v.Z)end
local function extract(start,finish)
 local a=assert(source:find(start,1,true));local b=assert(source:find(finish,a,true))
 return source:sub(a,b-1)
end
local function load(code,g)
 local f=assert(loadstring(code));setfenv(f,g);return f()
end
local function fixture()
 local own={Action="Idle",CanShoot=true,CanMove=true}
 local root={Position=V.new(0,0,37),AssemblyLinearVelocity=V.new(12,0,0)}
 local events={}
 local arc={};function arc:FindFirstChild()return self end
 local g=setmetatable({Vector3=V,flat=flat,plan={},state={},own=own,root=root,
  settings={farmShotRange=32,farmDunkRange=12,farmShoot=true,farmRandomThrees=false},
  api={GetAutoTime=function()return true end},lastShot=-100,stepbackShotUntil=0,
  attr=function(o,k)return o[k]end,holdsBall=function()return true end,
  busy={Shooting=true,Dunking=true,Layup=true,Stunned=true},
  rimFor=function()return V.zero end,goalPosition=function()return nil end,
  P={GetNetworkPing=function()return .05 end},defenders=function()return {}end,
  RaycastParams={new=function()return {}end},Enum={RaycastFilterType={Include=1}},
  workspace={FindFirstChild=function()return arc end,Raycast=function()return {Position=V.new(0,0,36)}end},
  syncSprint=function(_,wanted)events[#events+1]=wanted and "sprint"or "stop sprint"end,
  captureShot=function()end,
 },{__index=_G})
 g.settings._farmPlantFeet=function()
  events[#events+1]="plant"
  if own.Action~="Dribbling"then root.AssemblyLinearVelocity=V.new(0,root.AssemblyLinearVelocity.Y,0)end
 end
 g.plantFeet=load(extract(" local function plantFeet"," -- END FARM PLANT").."return plantFeet",g)
 g.shotRange=load(extract(" local function shotRange"," -- END FARM SHOT RANGE").."return shotRange",g)
 g.shotWindow=load(extract(" local function shotWindow"," -- Planning estimate only:").."return shotWindow",g)
 g.requestStandingShot=load(extract(" local function requestStandingShot"," local lastShotVisual=").."return requestStandingShot",g)
 g.takeOpenShot=load(extract(" local function takeOpenShot"," -- END FARM OPEN SHOT").."return takeOpenShot",g)
 local service={Shoot={Fire=function(_,args)
  assert(args.Shoot and args.Input=="E")
  assert(flat(root.AssemblyLinearVelocity).Magnitude==0,"E cannot fire with lateral momentum")
  assert(g.settings._farmSprintWanted==false,"Stop sprint before E")
  events[#events+1]="E"
 end}}
 return g,service,events,own,root
end
local g,service,events,own,root=fixture()
assert(g.takeOpenShot(own,root,10,service),"Open three must work with random threes disabled")
assert(g.shotStart==10 and g.state.status=="Open three: shot requested")
assert(table.concat(events,",")=="stop sprint,plant,E","Clear movement and sprint before shooting")
assert(g.state.shotRange==39,"Range includes detected arc plus small buffer")
assert(g.openingUntil==0 and not g.escapePlan,"Shot interrupts possession sprint and arc reset")
g,service,events,own,root=fixture();root.Position=V.new(0,0,40)
assert(not g.takeOpenShot(own,root,10,service)and #events==0,"Do not turn range adaptation into deep heaves")
g,service,events,own,root=fixture();g.workspace.FindFirstChild=function()return nil end
assert(not g.takeOpenShot(own,root,10,service)and g.state.shotRange==32,"Missing arc keeps configured range")
g,service,events,own,root=fixture();root.Position=V.new(0,0,12)
assert(not g.takeOpenShot(own,root,10,service)and #events==0,"Dunk approach keeps its motion")
for _,key in ipairs({"farmMove","retreat","confirmedStepback"})do
 g,service,events,own,root=fixture();g[key]=true
 assert(not g.takeOpenShot(own,root,10,service)and #events==0,"Do not interrupt pending "..key)
end
for _,action in ipairs({"Dribbling","Dunking","Shooting","Layup","Stunned"})do
 g,service,events,own,root=fixture();own.Action=action
 assert(not g.takeOpenShot(own,root,10,service)and #events==0,"Respect native action "..action)
end
g,service,events,own,root=fixture();g.settings.farmShoot=false
assert(not g.takeOpenShot(own,root,10,service)and #events==0)
g,service,events,own,root=fixture();g.holdsBall=function()return false end
assert(not g.requestStandingShot(own,root,service,10)and #events==0,"No E after possession loss")
g,service,events,own,root=fixture();g.defenders=function()return {{position=V.new(0,0,29),velocity=V.new(0,0,20)}}end
assert(not g.takeOpenShot(own,root,10,service)and #events==0,"Moving closeout vetoes shot")
g,service,events,own,root=fixture();root.AssemblyLinearVelocity=V.new(8,5,0)
assert(g.takeOpenShot(own,root,10,service)and not g.shotStart,"Airborne shooter must wait for ground contact")
assert(root.AssemblyLinearVelocity.Y==5 and events[#events]~="E","Braking preserves vertical motion")

-- Execute the actual retreat/confirmed-shot branches, including first-frame completion.
local landing=extract("  -- Let a measured retreat finish;","  local finish=false")
local function land(action,seen,moved,elapsed,covered)
 local g,s,e,o,r=fixture();o.Action=action;r.Position=V.new(0,0,31+moved)
 g.now=10+elapsed;g.own=o;g.root=r;g.service=s;g.action=action;g.distance=r.Position.Magnitude
 g.retreat={at=10,origin=V.new(0,0,31),seen=seen};g.rangeLimit=39;g.open=not covered
 g.ready=true;g.autoTimeOn=true;g.easyDunk=false
 g.stepbackRequirements=function()return 5,.2 end
 g.loadLanding=load("return function()\n"..landing.."end",g)
 g.loadLanding()
 return g,e,r
end
g,events,root=land("Dribbling",true,4,.2)
assert(not g.shotStart and root.AssemblyLinearVelocity.X==12,"Native backstep must retain its motion")
g,events,root=land("Idle",true,5.6,.21)
assert(g.shotStart==10.21 and events[#events]=="E","Shoot on the first completed frame, without settle time")
g,events=land("Idle",false,5.6,.1)
assert(g.shotStart==10.1,"Observed displacement and readiness handle a missed native action event")
g,events=land("Idle",true,5.6,.2,true)
assert(not g.shotStart and g.perimeterNeeded,"Covered landing must reset instead of forcing E")
g,events=land("Idle",false,0,1.2)
assert(not g.shotStart and not g.confirmedStepback,"Unacknowledged XX must time out without E")

-- Install the actual movement wrappers: manual input and boost must not reintroduce a fade.
local movementSource=readfile("src/engine/06_guard_input_movement.luau")
local a=assert(movementSource:find("local function attachMovement()",1,true))
local own={Action="Idle"}
local root={AssemblyLinearVelocity=V.new(14,2,3)}
local sent
local movement={Root=root,MovementVelocity={Velocity=V.new(14,2,3)},Types={Base={}}}
movement.Types.Base.UpdateVelocity=function()movement.MovementVelocity.Velocity=V.new(14,2,3)end
movement.IsSprinting=function()return true end
movement.SendMoveDirection=function(_,direction)sent=direction end
movement.UpdateMoveDirection=function(self)
 self.MoveDirection=V.new(1,0,0);self.lastMoveDirection=self.MoveDirection
 self:SendMoveDirection(self.MoveDirection)
end
g=setmetatable({settings={_farmPlantUntil=10.2,boost=true,boostDuration=.5,boostMultiplier=2},
 keyboard={Controllers={Player={Character={Movement=movement}}}},Vector3=V,flat=flat,
 model=function()return own end,rootOf=function()return root end,attr=function(o,k)return o[k]end,
 typeof=function(value)return getmetatable(value)==mt and "Vector3"or type(value)end,
 os={clock=function()return 10 end},restorePatches=function()end,
 patch=function(object,key,factory)object[key]=factory(object[key])end,
 guardDirection=function()return V.new(1,0,0)end,defensiveInputs=function()end,
 holdsBall=function()return true end,
},{__index=_G})
load(movementSource:sub(a).."\nreturn attachMovement",g)()
assert(g.hookedMovement==movement,g.featureState)
g.settings._farmPlantFeet()
assert(sent.Magnitude==0 and movement.MoveDirection.Magnitude==0 and movement.lastMoveDirection.Magnitude==0)
assert(root.AssemblyLinearVelocity.X==0 and root.AssemblyLinearVelocity.Y==2,"Clear residual motion, preserve vertical velocity")
assert(not movement:IsSprinting(),"Native sprint cannot override shot lock")
movement:UpdateMoveDirection(.016)
assert(sent.Magnitude==0 and movement.MoveDirection.Magnitude==0,"Native/manual input stays zero through movement update")
movement.Types.Base:UpdateVelocity(.016)
assert(flat(movement.MovementVelocity.Velocity).Magnitude==0,"Boost cannot restore horizontal shot velocity")
own.Action="Dribbling";root.AssemblyLinearVelocity=V.new(0,0,16)
movement.Types.Base:UpdateVelocity(.016)
assert(root.AssemblyLinearVelocity.Z==16 and movement.MovementVelocity.Velocity.X==14,"Do not cancel native XX displacement")
own.Action="Dunking";root.AssemblyLinearVelocity=V.new(10,0,0)
g.settings._farmPlantFeet()
assert(root.AssemblyLinearVelocity.X==10,"Finishing actions retain approach")
g.settings._farmPlantUntil=nil;g.settings.boost=false
assert(movement:IsSprinting(),"Releasing the shot lock restores native sprint")
movement.Types.Base:UpdateVelocity(.016)
assert(movement.MovementVelocity.Velocity.X==14,"Normal movement resumes outside shot lock")
print("Production open threes, range, stationary shots and immediate backstep landing passed")
