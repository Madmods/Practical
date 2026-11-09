local function on(signal,callback)
    local c=signal:Connect(function(...)
        local ok,err=pcall(callback,...)
        if not ok and os.clock()-lastErrorAt>1 then
            lastErrorAt=os.clock()
            status.Text="Observer error - still listening; see console."
            warn("[Peak V2] "..tostring(err))
        end
    end)
    table.insert(connections,c) return c
end
local function model()
    local folder=workspace:FindFirstChild("Characters")
    return (folder and folder:FindFirstChild(P.Name)) or P.Character
end
local function save()
    env.PracticalPeakDiagnostics=records
    if writefile then
        pcall(function() writefile("PracticalPeakResults.json",Http:JSONEncode({version=2,
            initialExtraDelay=0.0192117,currentExtraDelay=extraDelay,shots=records,total=total,perfect=perfect})) end)
        local ok=pcall(function() writefile("PracticalPeakProfiles-v2.json",Http:JSONEncode({version=2,profiles=profiles})) end)
        persistence=ok and "profiles saved" or "save unavailable"
    end
end
local function disconnectShot(s)
    if s and s.connection then s.connection:Disconnect(); s.connection=nil end
end
local featureCleanup
local function cleanup()
    if not alive then return end alive=false
    disconnectShot(active)
    if featureCleanup then pcall(featureCleanup) end
    for _,c in ipairs(connections) do c:Disconnect() end
    save() gui:Destroy()
end
env.PracticalPeakCleanup=cleanup
on(close.Activated,cleanup)
on(toggle.Activated,function()
    if not keyboard then status.Text="Waiting for the game controller." return end
    enabled=not enabled
    toggle.Text=enabled and "Auto Time · ON" or "Auto Time · OFF"
    toggle.BackgroundColor3=enabled and Color3.fromRGB(32,116,84) or Color3.fromRGB(43,49,60)
    -- Enabling never attaches to an already-running shot.
    disconnectShot(active) active=nil
    status.Text=enabled and "Armed · waiting for a NEW Bat meter packet." or "Disabled."
end)
on(Input.InputBegan,function(input,processed)
    if input.KeyCode==Enum.KeyCode.T and Input:IsKeyDown(Enum.KeyCode.LeftControl)
        and Input:IsKeyDown(Enum.KeyCode.LeftAlt) then panel.Visible=not panel.Visible end
end)
local function updatePing()
    local ok,value=pcall(function() return P:GetNetworkPing() end)
    if ok and finite(value) and value>=0 and value<1 then
        if ping then pingJitter=pingJitter*0.8+math.abs(value-ping)*0.2 end
        ping=value
    else ping=nil end
    pingAt=os.clock()
end
local function rate(samples)
    if #samples<4 then return nil end
    local first=math.max(1,#samples-8)
    local slopes={}
    for i=first,#samples-1 do
        for j=i+1,#samples do
            local dt=samples[j].t-samples[i].t
            if dt>=0.05 then table.insert(slopes,(samples[j].p-samples[i].p)/dt) end
        end
    end
    if #slopes<3 then return nil end
    table.sort(slopes)
    local middle=math.floor((#slopes+1)/2)
    local velocity=slopes[middle]
    if #slopes%2==0 then velocity=(velocity+slopes[middle+1])/2 end
    if not finite(velocity) or velocity<0.1 or velocity>20 then return nil end
    return velocity
end
local function sample(s)
    if not alive or s~=active or not s.model.Parent then return end
    local offset=s.model:GetAttribute("meterOffset")
    local rotation=s.model:GetAttribute("meterRotation")
    if typeof(offset)~="Vector2" or rotation~=270 then return end
    local p=-offset.Y
    if p<0 or p>1.05 then return end
    local now=os.clock()
    if not s.initialized then
        -- Reject a previous shot's frozen offset while packet attributes arrive.
        if p>0.25 or s.model:GetAttribute("ReleasedShot")==true then return end
        s.initialized=true
    end
    if s.last and math.abs(p-s.last.p)<0.00001 then return end
    s.max=math.max(s.max,p)
    local point={t=now,p=p}
    table.insert(s.trace,{time=now-s.started,progress=p})
    if s.last and p<s.last.p-0.01 then s.falling=true end
    if not s.sent and s.falling then
        s.cancelled="peak passed before prediction was ready"
        status.Text="Skipped shot: peak already passed. Still armed."
    end
    s.last=point
    if not s.sent and not s.falling then
        local latest=s.samples[#s.samples]
        if latest and point.t-latest.t<0.002 then
            -- Replication bursts are one arrival, not independent speed evidence.
            s.samples[#s.samples]=point
        else table.insert(s.samples,point) end
        if #s.samples>16 then table.remove(s.samples,1) end
        s.velocity=rate(s.samples)
    end
end
local function begin(m,meterType,window)
    if not alive or (not enabled and not (settings.shotAssist and settings.shotMeterMax>=100)) or m~=model() then return end
    disconnectShot(active) active=nil
    if meterType~="Bat" then status.Text="Unsupported meter: "..tostring(meterType)..". This test supports Bat only." return end
    if type(window)~="number" then status.Text="Skipped: no valid green-window packet." return end
    sequence=sequence+1
    local s={id=sequence,model=m,window=window,started=os.clock(),samples={},trace={},max=0,assistOnly=not enabled}
    active=s
    s.connection=m:GetAttributeChangedSignal("meterOffset"):Connect(function() sample(s) end)
    sample(s)
    status.Text="Shot "..sequence.." · measuring live fill speed."
end
local names={"Very Early","Early","Slightly Early","Good","Perfect","Slightly Late","Late","Very Late"}
local function result(m,contest,index)
    if m~=model() then return end
    local now=os.clock()
    while #awaiting>0 and now-awaiting[1].sentAt>3 do table.remove(awaiting,1) end
    local s=table.remove(awaiting,1)
    if not s then return end
    local label=names[tonumber(index)] or tostring(index)
    local offset=m:GetAttribute("meterOffset")
    local final=typeof(offset)=="Vector2" and -offset.Y or nil
    total=total+1
    if label=="Perfect" then perfect=perfect+1 end
    lastResult=label
    local record={id=s.id,result=label,contest=tonumber(contest),window=s.window,
        frameTime=s.frameTime,pingJitter=s.jitter,profile=s.profileKey,predictionAtSend=s.forecast,
        progressAtSend=s.sendPhase,rate=s.velocity,ping=s.ping,extraDelay=s.extra,
        finalProgress=final,trace=s.trace}
    -- Good is ambiguous around the peak. Do not train delay from those misses.
    -- A confirmed Perfect can retain a bounded successful operating point.
    if label=="Perfect" and s.profileKey then
        local profile=profiles[s.profileKey]
        if not profile then profile={delay=s.extra,confirmations=0}; profiles[s.profileKey]=profile end
        profile.confirmations=profile.confirmations+1
        -- Keep the delay that actually succeeded rather than resolving an
        -- ambiguous final offset onto a guessed early/late branch.
        profile.delay=math.clamp(profile.delay*0.9+s.extra*0.1,0,0.06)
        record.confirmedDelay=profile.delay
    end
    table.insert(records,record)
    if #records>60 then table.remove(records,1) end
    s.done=true
    disconnectShot(s)
    if active==s then active=nil end
    status.Text=label.." · "..math.floor((s.ping or 0)*1000+0.5).." ms ping · armed for next meter."
    save()
end
task.spawn(function()
    local ok,err=pcall(function()
        keyboard=require(P.PlayerScripts.Aero.Modules.Components.Input.Keyboard)
        on(keyboard.Services.MeterService.RegisterPacket,begin)
        on(keyboard.Services.MeterService.UnregisterPacket,function(m)
            if active and active.model==m then disconnectShot(active); active=nil end
        end)
        on(keyboard.Services.InterfaceService.ShowFeedback,result)
    end)
    if not alive then return end
    if ok then status.Text="Connected · enable Auto Time. No learning step."
    else keyboard=nil; status.Text="Connection failed · see executor console."; warn(tostring(err)) end
end)
on(Run.Heartbeat,function()
    local now=os.clock()
    if lastFrame then
        local dt=now-lastFrame
        if dt>0 and dt<0.15 then frameTime=frameTime*0.85+dt*0.15 end
    end
    lastFrame=now
    metrics.Text=string.format("Perfect %d/%d · last %s\nPing %s · jitter %.0f ms · %s",perfect,total,lastResult,
        ping and (tostring(math.floor(ping*1000+0.5)).." ms") or "unavailable",pingJitter*1000,persistence)
    while #awaiting>0 and now-awaiting[1].sentAt>3 do
        local expired=table.remove(awaiting,1)
        table.insert(records,{id=expired.id,result="No feedback",trace=expired.trace})
        disconnectShot(expired)
        if active==expired then active=nil end
        status.Text="Feedback timed out · still armed."
    end
    if now-pingAt>0.1 then updatePing() end
    local s=active
    if not s then return end
    if s.assistOnly and not enabled and not (settings.shotAssist and settings.shotMeterMax>=100) then disconnectShot(s);active=nil;return end
    if s.model~=model() or not s.model.Parent or now-s.started>3 then disconnectShot(s); active=nil; return end
    if s.sent or s.cancelled or not s.last or not s.velocity or not ping then return end
    if s.model:GetAttribute("ReleasedShot")==true then disconnectShot(s); active=nil; return end
    local age=now-s.last.t
    if ping*1000>(settings.timingMaxPingMs or 1000) then status.Text="Auto time · ping above your limit";return end
    if age>(settings.timingFreshMs or 120)/1000 then status.Text="Waiting for fresh meter updates · no release sent." return end
    -- Forecast the normalized fill at effective server release time.
    -- The peak target is inferred from captured rise/fall and Perfect results.
    local phase=s.last.p+s.velocity*age
    local key=profileKey(s.velocity,ping)
    local profile=profiles[key]
    local delay=profile and profile.delay or extraDelay
    local forecast=phase+s.velocity*math.max(0,ping+delay-(settings.timingOffsetMs or 0)/1000)
    -- V2's half-frame advance released below-target forecasts on this capture.
    -- Wait for the modeled arrival phase itself to reach the peak.
    if forecast>=1 then
        s.sent=true; s.sentAt=now; s.sendPhase=phase; s.ping=ping; s.extra=delay
        s.profileKey=key; s.frameTime=frameTime; s.jitter=pingJitter; s.forecast=forecast
        table.insert(awaiting,s)
        local ok,err=pcall(function() keyboard.Services.InputService.Shoot:Fire({Shoot=false}) end)
        if not ok then
            for i=#awaiting,1,-1 do if awaiting[i]==s then table.remove(awaiting,i) end end
            s.cancelled=tostring(err); status.Text="Release call failed · see console."; warn(tostring(err))
        else status.Text="Release sent · waiting for actual game result." end
    end
end)

