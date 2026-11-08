-- Meter release assist: defer early key-up, protect the descending floor.
do (function()
 local api=env.PracticalSuiteAPI
 settings.faceTarget=true;settings.shotAssist=false
 settings.shotMeterMin=80;settings.shotMeterMax=100;settings.shotHoldEarly=true;settings.shotProtectFall=true
 settings.timingOffsetMs=0;settings.timingMaxPingMs=1000;settings.timingFreshMs=120
 local state={hooked=false,releases=0,heldCount=0,reason="Off"}
 local hook,meter,links
 local limits={shotMeterMin={1,100},shotMeterMax={1,100},timingOffsetMs={-30,30},timingMaxPingMs={50,1000},timingFreshMs={60,200}}
 local oldNumber,oldOption=api.SetNumber,api.SetOption
 api.SetNumber=function(key,value)
  local range=limits[key];if not range then return oldNumber(key,value) end
  if not finite(value) then return false end
  settings[key]=math.clamp(value,range[1],range[2])
  if key=="shotMeterMin" and settings.shotMeterMin>settings.shotMeterMax then settings.shotMeterMax=settings.shotMeterMin end
  if key=="shotMeterMax" and settings.shotMeterMax<settings.shotMeterMin then settings.shotMeterMin=settings.shotMeterMax end
  return true
 end
 api.SetOption=function(key,value)
  if key=="faceTarget" then settings.faceTarget=true;return value==true end
  return oldOption(key,value)
 end
 local function release(reason)
  if not hook or not meter or meter.sent then return false end
  if active and not active.sent then active.cancelled="Shot Assist sent release" end
  meter.sent=true;meter.pending=false;state.reason=reason;state.releases=state.releases+1
  local ok,err=pcall(hook.original,hook.object,{Shoot=false})
  if not ok then meter.sent=false;state.reason="Release call failed: "..tostring(err);return false end
  recordDefense("meter_release_sent",{reason=reason,progress=meter.progress,minimum=settings.shotMeterMin,maximum=settings.shotMeterMax})
  return true
 end
 local function clear(reason)
  if meter and meter.pending then release(reason or "Meter ended; releasing deferred input") end
  meter=nil
 end
 local function readMeter(now)
  if not meter or not meter.model.Parent then return end
  local offset=meter.model:GetAttribute("meterOffset")
  if typeof(offset)~="Vector2" or meter.model:GetAttribute("meterRotation")~=270 then return end
  local p=-offset.Y;if p<0 or p>1.05 then return end
  if not meter.initialized then
   if p>.25 or meter.model:GetAttribute("ReleasedShot")==true then return end
   meter.initialized=true
  end
  if meter.progress and math.abs(p-meter.progress)<.00001 then return end
  if meter.progress and meter.sampleAt and now-meter.sampleAt>.002 then
   local slope=(p-meter.progress)/(now-meter.sampleAt)
   if math.abs(slope)<20 then meter.slope=slope end
   if p<meter.progress-.005 then meter.falling=true end
  end
  meter.progress=p;meter.sampleAt=now
 end
 local function stopHook()
  clear("Assist stopped; releasing deferred input")
  if links then for _,connection in ipairs(links) do connection:Disconnect() end end;links=nil
  if hook then pcall(function() if rawget(hook.object,"Fire")==hook.wrapper then rawset(hook.object,"Fire",hook.own) end end) end
  hook=nil;state.hooked=false
 end
 api.GetShotAssistState=function()
  return {hooked=state.hooked,releases=state.releases,heldCount=state.heldCount,reason=state.reason,
   progress=meter and meter.progress and meter.progress*100,pending=meter and meter.pending or false,
   target=settings.shotMeterMin,minimum=settings.shotMeterMin,maximum=settings.shotMeterMax,contest=settings.hudPressure,contestEstimated=true,manualInsideRange=true}
 end
 api.ReleaseShot=function() return release("Manual release override") end
 on(Run.Heartbeat,function()
  local now=os.clock()
  local service=keyboard and keyboard.Services and keyboard.Services.InputService
  local shoot=service and service.Shoot
  if not hook or hook.object~=shoot then
   stopHook()
   if type(shoot)~="table" or type(shoot.Fire)~="function" then state.reason="Shot input hook unavailable";return end
   local original,own=shoot.Fire,rawget(shoot,"Fire")
   local wrapper=function(self,payload,...)
    if type(payload)=="table" and payload.Shoot==false then readMeter(os.clock()) end
    if type(payload)=="table" and payload.Shoot==false and meter and not meter.sent then
     if active and active.sent then meter.sent=true;meter.pending=false
     elseif settings.shotAssist and meter.initialized and meter.progress and os.clock()-(meter.sampleAt or 0)<.2 then
      local inside=meter.progress>=settings.shotMeterMin/100 and meter.progress<=settings.shotMeterMax/100
      if inside then
       if active and not active.sent then active.cancelled="Manual release inside assist range" end
       state.reason="Manual release allowed inside range"
      end
      local below=not meter.falling and meter.progress<settings.shotMeterMin/100
      if settings.shotHoldEarly and below then
       if not meter.pending then state.heldCount=state.heldCount+1 end
       meter.pending=true;state.reason="Holding until minimum"
       return
      end
     end
     meter.sent=true;meter.pending=false
    end
    return original(self,payload,...)
   end
   local ok=pcall(function() rawset(shoot,"Fire",wrapper) end)
   if not ok then state.reason="Shot input hook unavailable";return end
   hook={object=shoot,own=own,original=original,wrapper=wrapper};state.hooked=true
   links={}
   table.insert(links,on(keyboard.Services.MeterService.RegisterPacket,function(m,kind)
    if m~=model() then return end
    clear("New meter replaced previous shot")
    if kind~="Bat" then state.reason="Unsupported meter; manual release passes";return end
    meter={model=m,started=os.clock(),pending=false,sent=false};readMeter(os.clock())
   end))
   table.insert(links,on(keyboard.Services.MeterService.UnregisterPacket,function(m)
    if meter and meter.model==m then clear("Meter ended") end
   end))
  end
  if not meter then return end
  readMeter(now)
  if not settings.shotAssist then if meter.pending then release("Assist disabled; releasing deferred input") end;return end
  if meter.sent then return end
  if meter.model~=model() or not meter.model.Parent or now-meter.started>3 then clear("Shot timeout");return end
  if meter.model:GetAttribute("ReleasedShot")==true then meter.sent=true;meter.pending=false;return end
  if not meter.initialized or not meter.progress then return end
  if now-(meter.sampleAt or 0)>.2 then if meter.pending then release("Meter stale; deferred input released") end;return end
  local target=settings.shotMeterMin/100
  if meter.pending and meter.progress>=target then release("Early release held until target");return end
  if settings.shotMeterMax<100 and not meter.falling and meter.progress>=settings.shotMeterMax/100 then release("Maximum meter limit reached");return end
  if settings.shotMeterMax>=100 and active and not active.cancelled and not active.sent then return end
  if settings.shotProtectFall and meter.falling then
   local slope=math.min(meter.slope or 0,0)
   local predicted=meter.progress+slope*((ping or 0)+frameTime+.02)
   if predicted<=target then release("Descending meter floor protected") end
  end
 end)
 local previousCleanup=featureCleanup
 featureCleanup=function() stopHook();previousCleanup() end
end)() end

do (function()
 local outline=create("Highlight",{Name="PracticalContestOutline",FillTransparency=.98,OutlineTransparency=.18,
  DepthMode=Enum.HighlightDepthMode.Occluded,Enabled=false},workspace)
 on(Run.Heartbeat,function()
  local own=model();local percent=settings.hudPressure
  outline.Adornee=own
  outline.Enabled=(not settings._visualV2 or settings.visualShowCoverage)and settings.liveContest and own~=nil and percent~=nil and os.clock()-(settings.hudPressureAt or 0)<.3
  if not outline.Enabled then return end
  local color=percent>=65 and Color3.fromRGB(255,85,95) or percent>=35 and Color3.fromRGB(255,192,71) or Color3.fromRGB(73,151,255)
  outline.OutlineColor=color;outline.FillColor=color
 end)
 local previousCleanup=featureCleanup
 featureCleanup=function() outline:Destroy();previousCleanup() end
end)() end

