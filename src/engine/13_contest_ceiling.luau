-- Contest-aware shooting controls. Geometry is an estimate, never server contest.
do (function()
 settings.contestCeiling=false;settings.contestLimit=30;settings.contestForecast=.2
 settings.contestRelease=false;settings.contestBlockStart=true;settings.contestMinMeter=90
 settings.contestMaxMeter=100;settings.contestHold=.08;settings.contestRisingOnly=true
 for k,v in pairs({contestLimit={5,100},contestForecast={0,.5},contestMinMeter={1,100},contestMaxMeter={1,100},contestHold={0,.3}})do numericLimits[k]=v end
 local state={estimated=true,current=0,forecast=0,trend="STEADY",reason="Disabled",released=false}
 api.GetContestCeilingState=function()return state end
 local samples=setmetatable({},{__mode="k"})
 local shot
 local aboveSince
 local lastValue,lastAt,lastUpdate
 local function estimate(now)
  local me=model();local r=rootOf(me);local estimator=settings._contestModel
  if not r or not estimator then return nil end
  local list=estimator.collect(me)
  local current=estimator.evaluate(r.Position,list,0)
  local future=estimator.evaluate(r.Position,list,settings.contestForecast,r.AssemblyLinearVelocity)
  return current,future
 end

 if keyboard then
  on(keyboard.Services.MeterService.RegisterPacket,function(m,kind)
   if m==model()then shot=kind=="Bat" and {model=m,started=os.clock()} or nil;aboveSince=nil;state.released=false end
  end)
  on(keyboard.Services.MeterService.UnregisterPacket,function(m)if shot and shot.model==m then shot=nil;aboveSince=nil end end)
 end
 local service=keyboard and keyboard.Services.InputService
 local signal=service and service.Shoot
 local original=signal and signal.Fire
 local wrapper
 if original then
  wrapper=function(self,payload,...)
   if settings.contestCeiling and settings.contestBlockStart and type(payload)=="table" and payload.Shoot==true and not payload.UseSpace then
    local current=estimate(os.clock())
    if current and current>=settings.contestLimit then state.reason="New shot blocked by estimated contest ceiling";return end
   end
   return original(self,payload,...)
  end
  signal.Fire=wrapper
 end
 on(Run.Heartbeat,function()
  local now=os.clock()
  if lastUpdate and now-lastUpdate<.05 then return end;lastUpdate=now
  if not settings.contestCeiling then state.reason="Disabled";aboveSince=nil;return end
  local current,future=estimate(now)
  if not current then state.reason="No character";return end
  local slope=lastValue and lastAt and (current-lastValue)/math.max(now-lastAt,.01) or 0
  state.current=current;state.forecast=future;state.trend=(future-current>3 or slope>15)and "CLOSING" or (future-current< -3 or slope< -15)and "OPENING" or "STEADY"
  lastValue=current;lastAt=now
  local threat=math.max(current,future)>=settings.contestLimit
  state.reason=threat and "Estimated ceiling threatened" or "Below estimated ceiling"
  if not shot or not settings.contestRelease or state.released then aboveSince=nil;return end
  local offset=shot.model:GetAttribute("meterOffset")
  if typeof(offset)~="Vector2" or shot.model:GetAttribute("meterRotation")~=270 or shot.model:GetAttribute("ReleasedShot")==true then return end
  local p=-offset.Y
  if not shot.initialized then if p>.25 then return end;shot.initialized=true end
  if shot.lastProgress~=p then
   shot.maxProgress=math.max(shot.maxProgress or p,p)
   shot.falling=shot.falling or p<shot.maxProgress-.01;shot.lastProgress=p;shot.updated=now
  end
  local minimum,maximum=settings.contestMinMeter,settings.contestMaxMeter
  if minimum>maximum then state.reason="Invalid allowed timing range";return end
  if settings.shotAssist then minimum=math.max(minimum,settings.shotMeterMin);maximum=math.min(maximum,settings.shotMeterMax)end
  if not threat then aboveSince=nil;return end
  aboveSince=aboveSince or now
  if not shot.updated or now-shot.updated>.12 then state.reason="Ceiling threatened · stale meter";return end
  if p*100<minimum or p*100>maximum or minimum>maximum then state.reason="Ceiling threatened · outside allowed timing";return end
  if settings.contestRisingOnly and shot.falling then state.reason="Ceiling threatened · falling meter";return end
  if now-aboveSince<settings.contestHold then return end
  if api.ReleaseShot()then state.released=true;state.reason="Contest release requested within timing range" end
 end)
 local previousCleanup=featureCleanup
 featureCleanup=function()if signal and signal.Fire==wrapper then signal.Fire=original end;previousCleanup()end
end)() end


