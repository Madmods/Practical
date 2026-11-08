local function run(open,planted,now,closedAt,range)
 local confirmedStepback=true
 local plan={};local state={stepbackClosedAt=closedAt,shots=0}
 local Vector3={zero=0};local root={Position=0,AssemblyLinearVelocity={Y=0}}
 local settings={farmDunkRange=12,farmShotRange=32,farmShoot=true}
 local distance=range or 26;local autoTimeOn=true;local stepbackShotUntil=2
 local list={};local ping=0;local speed=0;local action="";local own={};local service={}
 local ready=true;local lastShotMotion=0;local lastShot=0;local shotStart=nil;local shotSawAction=false
 local calls=0
 local function shotWindow()return open end
 local function stationaryShot()return planted,0 end
 local function attr()return true end
 local function requestStandingShot()calls+=1;return true end
 local function tick()
  -- Own the movement plan through landing and release; do not restart the drive while planting.
  if confirmedStepback then
   plan.dir=Vector3.zero;plan.reason="Stepback: plant and release";state.visualTarget=root.Position
   local validRange=distance>math.max(18,settings.farmDunkRange+6)and distance<=settings.farmShotRange
   local cancelReason=not validRange and "landing outside shot range"
    or not settings.farmShoot and "shots disabled"
    or not autoTimeOn and "Auto time disabled"
    or now>=stepbackShotUntil and "release window expired"
   if cancelReason then
    confirmedStepback=false;stepbackShotUntil=0;state.stepbackClosedAt=nil
    state.status="Stepback cancelled: "..cancelReason
    return
   end
   local planted
   planted,lastShotMotion=stationaryShot(speed,root.AssemblyLinearVelocity.Y,action,attr(own,"CanMove"),now,lastShotMotion)
   if not planted or not ready then state.status="Stepback committed: waiting for planted feet / shot readiness";return end
   local ok,reason=requestStandingShot(own,root,service,now,true)
   if ok then
    confirmedStepback=false;stepbackShotUntil=0;lastShot=now;shotStart=now;shotSawAction=false
    state.shots=(state.shots or 0)+1;state.status="Stepback committed: shot requested"
   else state.status="Stepback committed: "..tostring(reason)end
   return
  end

 end
 tick()
 return state,plan,calls,confirmedStepback
end
local s,p,c,active=run(true,false,1)
assert(c==0 and active and p.dir==0,"Planting must hold, not resume drive")
s,p,c,active=run(true,true,1)
assert(c==1 and not active and s.shots==1,"Open planted landing shoots once")
s,p,c,active=run(false,true,1)
assert(c==1 and not active,"Chosen stepback shoots even if closeout estimate changes")
s,p,c,active=run(false,true,1,.5)
assert(c==1 and not active,"Closeout does not cancel committed shot")
s,p,c,active=run(true,false,3)
assert(c==0 and not active,"Commitment expires instead of sticking")
s,p,c,active=run(true,true,1,nil,15)
assert(c==0 and not active,"No jump shot in finishing zone")
print("6 stepback commitment checks passed")
