local lastStepback,lastShot,shotStart,shotSawAction
local state={shots=0};local plan={};local guardError=60
local retreat={};local confirmedStepback=true;local stepbackShotUntil=9
local Vector3={zero=0}
 local function stepbackAndShoot(service,now)
  if not service or not service.Dribble or not service.Shoot then return false,"Shot controller unavailable"end
  local ok,err=pcall(function()
   service.Dribble:Fire("XX",true,Vector3.zero)
   service.Shoot:Fire({Shoot=true,Input="E"})
  end)
  if not ok then return false,err end
  lastStepback=now;lastShot=now;shotStart=now;shotSawAction=false
  state.shotActionAt=nil;state.stepbackQueuedShot=true
  retreat=nil;confirmedStepback=false;stepbackShotUntil=0
  plan.dir=Vector3.zero;plan.reason="Stepback: E held immediately";guardError=0
  state.status="Stepback: E held, waiting for shot"
  state.shots=(state.shots or 0)+1
  return true
 end

local events={}
local service={Dribble={Fire=function(_,key)events[#events+1]=key end},Shoot={Fire=function(_,args)events[#events+1]=args.Input;assert(args.Shoot==true)end}}
assert(stepbackAndShoot(service,10))
assert(#events==2 and events[1]=="XX"and events[2]=="E","XX must be followed immediately by E")
assert(shotStart==10 and state.stepbackQueuedShot and not shotSawAction,"Shot is held pending acknowledgement")
assert(plan.dir==0 and guardError==0,"No movement while queued")
assert(retreat==nil and not confirmedStepback,"Old settle plan cannot delay queued E")
assert(not stepbackAndShoot({},11),"Missing controllers must fail cleanly")
local calls=0
assert(not stepbackAndShoot({Dribble={Fire=function()error("failed")end},Shoot={Fire=function()calls+=1 end}},11))
assert(calls==0,"Failed stepback must not trigger a shot")
print("Immediate XX -> E checks passed")
