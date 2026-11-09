-- VANTA_STARTUP_WAIT: readiness gate for manual execution, autoexec and teleports.
do
 if not game:IsLoaded()then game.Loaded:Wait()end
 local players=game:GetService("Players")
 while not players.LocalPlayer do task.wait(.1)end
 players.LocalPlayer:WaitForChild("PlayerGui")
 -- Allow replicated controllers and interface scripts time to initialize.
 task.wait(10)
end

-- Experimental peak prediction for the captured Bat meter, rotation 270.
-- No manual learning, no perfect-shot requirement, no GUI/property modification.
local env=(getgenv and getgenv()) or _G
if env.PracticalAutoTimeCleanup then pcall(env.PracticalAutoTimeCleanup) end
if env.PracticalMeterResearchStop then pcall(env.PracticalMeterResearchStop) end
if env.PracticalPeakCleanup then pcall(env.PracticalPeakCleanup) end
local P=game:GetService("Players").LocalPlayer
local Run=game:GetService("RunService")
local Input=game:GetService("UserInputService")
-- Shared keyboard / mouse binding support.
local function bindingValid(key)
 if typeof(key)~="EnumItem"then return false end
 if key.EnumType==Enum.KeyCode then return key~=Enum.KeyCode.Unknown end
 return key.EnumType==Enum.UserInputType and (key==Enum.UserInputType.MouseButton1 or key==Enum.UserInputType.MouseButton2 or key==Enum.UserInputType.MouseButton3)
end
local function bindingMatches(input,key)
 return key~=nil and (input.KeyCode==key or input.UserInputType==key)
end
local function bindingDown(key)
 if typeof(key)=="EnumItem"and key.EnumType==Enum.UserInputType then return Input:IsMouseButtonPressed(key)end
 return Input:IsKeyDown(key)
end

local Http=game:GetService("HttpService")
local alive,enabled=true,false
local connections={}
local active
local awaiting={}
local records={}
local ping,pingAt=nil,0
-- Median additional effective release delay from 10 recorded manual shots.
-- This is an empirical initial estimate, NOT a recovered server timing formula.
local extraDelay=0.0192117
local profiles={}
local pingJitter=0
local frameTime=1/60
local lastFrame
local total,perfect=0,0
local lastResult="—"
local persistence="memory only"
local function finite(v) return type(v)=="number" and v==v and math.abs(v)<math.huge end
if readfile then
    local ok,saved=pcall(function() return Http:JSONDecode(readfile("PracticalPeakProfiles-v2.json")) end)
    if ok and type(saved)=="table" and saved.version==2 and type(saved.profiles)=="table" then
        for key,profile in pairs(saved.profiles) do
            if type(key)=="string" and #key<80 and type(profile)=="table"
                and finite(profile.delay) and profile.delay>=0 and profile.delay<=0.06
                and finite(profile.confirmations) and profile.confirmations>=1 then
                profiles[key]={delay=profile.delay,confirmations=math.min(profile.confirmations,1000)}
            end
        end
    end
end
local function profileKey(velocity,currentPing)
    return "Bat/"..math.floor(velocity*4+0.5).."/"..math.floor(currentPing*1000/20+0.5)
end
local keyboard
local sequence=0
local gui=Instance.new("ScreenGui") gui.Name="PracticalSuite" gui.ResetOnSpawn=false gui.Parent=P.PlayerGui
local function create(class,props,parent)
    local o=Instance.new(class)
    for k,v in pairs(props) do o[k]=v end
    o.Parent=parent return o
end
local colors={bg=Color3.fromRGB(17,20,26),card=Color3.fromRGB(26,31,40),line=Color3.fromRGB(48,57,72),
    text=Color3.fromRGB(229,235,245),muted=Color3.fromRGB(142,156,178),accent=Color3.fromRGB(63,181,143)}
local panel=create("Frame",{Size=UDim2.fromOffset(450,622),Position=UDim2.new(0,24,0.5,-311),BackgroundColor3=colors.bg,BorderSizePixel=0},gui)
create("UICorner",{CornerRadius=UDim.new(0,12)},panel)
create("UIStroke",{Color=colors.line,Thickness=1},panel)
local function label(parent,value,y,height,size)
    return create("TextLabel",{Size=UDim2.new(1,-32,0,height),Position=UDim2.fromOffset(16,y),BackgroundTransparency=1,
        Text=value,TextSize=size or 13,TextColor3=colors.text,Font=Enum.Font.GothamMedium,TextWrapped=true,TextXAlignment=Enum.TextXAlignment.Left},parent)
end
local header=label(panel,"PRACTICAL  /  CONTROL",10,30,17) header.Active=true
label(panel,"Suite 2.3  ·  Peak V2.1 / movement / defense",39,22,11).TextColor3=colors.muted
local pages,nav={},{}
for i,name in ipairs({"Shooting","Movement","Defense","Stats"}) do
    local button=create("TextButton",{Size=UDim2.fromOffset(98,32),Position=UDim2.fromOffset(16+(i-1)*106,75),
        BackgroundColor3=colors.card,BorderSizePixel=0,Text=name,TextSize=12,TextColor3=colors.text,Font=Enum.Font.GothamMedium},panel)
    create("UICorner",{CornerRadius=UDim.new(0,6)},button)
    nav[name]=button
    pages[name]=create("ScrollingFrame",{ScrollBarThickness=3,CanvasSize=UDim2.fromOffset(0,name=="Defense" and 1820 or 380),BorderSizePixel=0,Position=UDim2.fromOffset(0,119),Size=UDim2.new(1,0,0,380),BackgroundTransparency=1,Visible=i==1},panel)
end
local function row(parent,title,y)
    local b=create("TextButton",{Size=UDim2.new(1,-32,0,40),Position=UDim2.fromOffset(16,y),BackgroundColor3=colors.card,
        BorderSizePixel=0,Text=title,TextSize=13,TextColor3=colors.text,Font=Enum.Font.GothamMedium},parent)
    create("UICorner",{CornerRadius=UDim.new(0,7)},b) return b
end
local toggle=row(pages.Shooting,"Auto Time · OFF",8)
label(pages.Shooting,"Uses the unchanged V2.1 release predictor.\nBat meter, rotation 270. No manual learning.",60,52,12).TextColor3=colors.muted
local contestLabel=label(pages.Shooting,"Last shot contest: waiting for game feedback",144,45,15)
local contestToggle=row(pages.Shooting,"Contest display · ON",203)
label(pages.Shooting,"Contest comes from actual shot feedback; it is not a live pre-shot estimate.",255,45,11).TextColor3=colors.muted
local status=label(panel,"Connecting...",508,47,12)
local metrics=label(pages.Stats,"No results yet",8,95,14)
label(pages.Stats,"Movement features are experimental. The timing predictor is backed up separately.",129,65,12).TextColor3=colors.muted
local featureStatus=label(pages.Stats,"Feature controller: waiting",205,80,12)
local close=row(panel,"Unload  ·  Ctrl + Alt + T hides panel",570)
local settings={boost=false,boostMultiplier=1.15,boostDuration=0.25,stamina=false,staminaFloor=100,
    latencyCompensation=false,guard=false,guardRange=20,guardGap=5,guardStrength=1,guardReaction=0.16,guardPrediction=0.08,
    guardHold=true,contest=true,visuals=true,closeout=true,closeGap=1.5,closeReaction=.12,
    steal=false,stealRange=3.5,stealDelay=.18,stealCooldown=1.4,
    blockCommitFilter=false,
    block=false,blockRange=5,blockDelay=.22,jumpCooldown=1.2,
    rebound=false,reboundRange=5,reboundHeight=7,
    guardSprint=true,sprintDistance=2.5,faceTarget=true,guardDeadzone=.65,
    stealAngle=55,stealMaxSpeed=12,blockOnce=true,
    reboundChase=false,reboundChaseRange=12,reboundLead=.15,liveContest=true,assignmentLock=false,fakeFilter=true,smartPosition=true,landing=true,manualResume=.12}
local guardKey=Enum.KeyCode.G
local boostToggle=row(pages.Movement,"Dribble boost · OFF (experimental)",0)
local staminaToggle=row(pages.Movement,"Infinite stamina · unavailable / unverified",188)
staminaToggle.Active=false
staminaToggle.AutoButtonColor=false
label(pages.Movement,"The old stamina bypass had no effect and has been removed.",294,24,11).TextColor3=colors.muted
local guardToggle=row(pages.Defense,"Auto guard · OFF (experimental)",0)
local holdToggle=row(pages.Defense,"Activation · hold G",320)
local function setting(parent,title,key,y,minimum,maximum,step,format)
    local caption=label(parent,title,y,21,12)
    local value=create("TextLabel",{Size=UDim2.fromOffset(105,25),Position=UDim2.new(1,-155,0,y+24),BackgroundTransparency=1,
        TextColor3=colors.text,TextSize=12,Font=Enum.Font.GothamMedium},parent)
    local function refresh() value.Text=string.format(format,key=="guardStrength" and settings[key]*100 or settings[key]) end
    local buttons={}
    for i,delta in ipairs({-step,step}) do
        local b=create("TextButton",{Size=UDim2.fromOffset(30,25),Position=UDim2.new(1,i==1 and -190 or -45,0,y+24),
            BackgroundColor3=colors.card,BorderSizePixel=0,Text=i==1 and "−" or "+",TextColor3=colors.text,TextSize=18},parent)
        buttons[#buttons+1]={button=b,delta=delta}
    end
    return {refresh=refresh,buttons=buttons,key=key,min=minimum,max=maximum,widgets={caption,value,buttons[1].button,buttons[2].button}}
end
local defenseAdvanced=create("Frame",{Visible=false,Size=UDim2.fromScale(1,1),BackgroundTransparency=1},pages.Defense)
local settingRows={
    setting(pages.Movement,"Glide multiplier","boostMultiplier",52,1,1.5,0.05,"%.2fx"),
    setting(pages.Movement,"Glide tail duration","boostDuration",112,0.1,0.5,0.05,"%.2f s"),

    setting(defenseAdvanced,"Acquire range (studs)","guardRange",47,4,30,2,"%.0f"),
    setting(defenseAdvanced,"Stand-off distance (studs)","guardGap",100,2,8,0.25,"%.2f"),
    setting(defenseAdvanced,"Turn responsiveness (full-speed input)","guardStrength",153,0.1,1,0.1,"%.0f%%"),
    setting(defenseAdvanced,"Reaction delay","guardReaction",206,0,0.4,0.02,"%.2f s"),
    setting(defenseAdvanced,"Movement prediction","guardPrediction",267,0,0.2,0.02,"%.2f s")}

local visualToggle=row(defenseAdvanced,"Guard floor visuals · ON",370)
local closeoutToggle=row(defenseAdvanced,"Shot closeout · ON",418)
local stealToggle=row(defenseAdvanced,"Auto steal · OFF (experimental)",574)
local blockToggle=row(defenseAdvanced,"Auto block · OFF (experimental)",792)
local reboundToggle=row(defenseAdvanced,"Nearby auto rebound · OFF (experimental)",954)
local extraRows={
 setting(defenseAdvanced,"Closeout distance","closeGap",466,.75,4,.25,"%.2f"),
 setting(defenseAdvanced,"Closeout reaction","closeReaction",519,0,.4,.02,"%.2f s"),
 setting(defenseAdvanced,"Steal ball reach","stealRange",622,1,6,.25,"%.2f"),
 setting(defenseAdvanced,"Opportunity hold time","stealDelay",675,.05,.5,.025,"%.3f s"),
 setting(defenseAdvanced,"Steal cooldown","stealCooldown",728,.6,3,.2,"%.1f s"),
 setting(defenseAdvanced,"Block reach","blockRange",840,2,8,.5,"%.1f"),
 setting(defenseAdvanced,"Block shot delay","blockDelay",893,.05,.5,.025,"%.3f s"),
 setting(defenseAdvanced,"Rebound reach","reboundRange",1002,2,10,.5,"%.1f"),
 setting(defenseAdvanced,"Rebound height above root","reboundHeight",1055,3,12,.5,"%.1f"),
 setting(defenseAdvanced,"Jump cooldown","jumpCooldown",1108,.6,2,.1,"%.1f s")}
local sprintToggle=row(defenseAdvanced,"Guard sprint · ON",1170)
local facingToggle=row(defenseAdvanced,"Face handler · ON",1218)
local onceToggle=row(defenseAdvanced,"One block attempt per shot · ON",1266)
local chaseToggle=row(defenseAdvanced,"Chase nearby rebounds · OFF",1314)
local moreRows={
 setting(defenseAdvanced,"Sprint only beyond distance","sprintDistance",1362,.5,8,.5,"%.1f"),
 setting(defenseAdvanced,"Position dead zone","guardDeadzone",1415,.25,2,.1,"%.2f"),
 setting(defenseAdvanced,"Steal forward angle","stealAngle",1468,15,85,5,"%.0f degrees"),
 setting(defenseAdvanced,"Maximum handler speed for steal","stealMaxSpeed",1521,2,25,1,"%.0f"),
 setting(defenseAdvanced,"Rebound pursuit range","reboundChaseRange",1574,4,20,1,"%.0f"),
 setting(defenseAdvanced,"Rebound trajectory lookahead","reboundLead",1627,0,.35,.025,"%.3f s")}
for _,r in ipairs(moreRows) do table.insert(extraRows,r) end
label(defenseAdvanced,"Steal/block/rebound use native actions. Results depend on the server; no independent hand placement.",1690,65,11).TextColor3=colors.muted
local latencyToggle=row(pages.Shooting,"Online latency compensation - OFF",360)
pages.Shooting.CanvasSize=UDim2.fromOffset(0,414)
local latencyMetrics=label(pages.Stats,"Latency compensation: OFF",295,75,12)
local liveToggle=row(pages.Shooting,"Live contest estimate · ON",312)
local relaxedPreset=row(pages.Defense,"Response · Balanced",312)
for i,button in ipairs({guardToggle,stealToggle,blockToggle,reboundToggle,holdToggle}) do
 button.Parent=pages.Defense;button.Position=UDim2.fromOffset(16,(i-1)*47)
end
-- Keep one distance control; advanced heuristics use the selected response preset.
local gapRow=settingRows[4]
for i,widget in ipairs(gapRow.widgets) do
 widget.Parent=pages.Defense
 widget.Position=UDim2.new(widget.Position.X.Scale,widget.Position.X.Offset,0,i==1 and 240 or 264)
end
pages.Defense.CanvasSize=UDim2.fromOffset(0,405)
local defenseHint=label(pages.Defense,"Hold G to engage. Actions below are attempts, not confirmed results.",356,45,11)
local liveHud=create("Frame",{Size=UDim2.fromOffset(300,74),Position=UDim2.new(.5,-150,0,60),
 BackgroundColor3=colors.bg,BackgroundTransparency=.12,BorderSizePixel=0},gui)
create("UICorner",{CornerRadius=UDim.new(0,9)},liveHud)
local liveTitle=label(liveHud,"ESTIMATED CONTEST · waiting",3,25,13)
local liveDetail=label(liveHud,"Distance / positioning estimate",29,21,11)
local liveTrack=create("Frame",{Size=UDim2.new(1,-32,0,4),Position=UDim2.fromOffset(16,59),BackgroundColor3=colors.line,BorderSizePixel=0},liveHud)
local liveFill=create("Frame",{Size=UDim2.fromScale(0,1),BackgroundColor3=colors.accent,BorderSizePixel=0},liveTrack)
for _,r in ipairs(extraRows) do table.insert(settingRows,r) end
local lastErrorAt=0
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
-- Session-local, bounded learning. Trace timestamps are observed locally, not server truth.
settings._latency={buckets={}}
function settings._latency.bucket(currentPing)
    return math.floor(currentPing / .020 + .5)
end
function settings._latency.adjust(currentPing,jitter)
    if not settings.latencyCompensation or not finite(currentPing) then return 0,0,nil end
    local key=settings._latency.bucket(currentPing)
    local bucket=settings._latency.buckets[key]
    return bucket and bucket.correction or 0,math.clamp(jitter or 0,0,.012),key
end
function settings._latency.learn(s,record)
    if not s.compensated or not settings.latencyCompensation or s.cancelled then return end
    -- Require a near-full, bracketed peak. Frozen/truncated/ambiguous traces cannot train.
    local trace=s.trace
    local peakIndex
    for i=2,#trace-1 do
        if not peakIndex or trace[i].progress>trace[peakIndex].progress then peakIndex=i end
    end
    if not peakIndex then return end
    local before,peak,after=trace[peakIndex-1],trace[peakIndex],trace[peakIndex+1]
    if peak.progress<.98 or peak.progress>1.02 or before.progress>=peak.progress
        or after.progress>=peak.progress or after.time-before.time>.12 then return end
    local peakAt=s.started+peak.time
    -- Effective arrival estimate excludes the adaptive advance itself. Positive error = late.
    local error=s.sentAt+s.ping+s.baseDelay-s.offset-peakAt
    if not finite(error) or math.abs(error)>.080 then return end
    local bucket=settings._latency.buckets[s.pingBucket] or {correction=0,count=0,error=0}
    local target=math.clamp(s.correction+error+s.jitterMargin,-.020,.020)
    bucket.error=bucket.error*.8+error*.2
    bucket.correction=math.clamp(bucket.correction*.8+target*.2,-.020,.020)
    bucket.count=bucket.count+1
    settings._latency.buckets[s.pingBucket]=bucket
    record.estimatedPeakTime=peak.time
    record.estimatedReleaseError=error
    record.learnedCorrection=bucket.correction
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
        finalProgress=final,trace=s.trace,latencyCompensated=s.compensated,
        latencyCorrection=s.correction,jitterMargin=s.jitterMargin,pingBucket=s.pingBucket}
    settings._latency.learn(s,record)
    -- Good is ambiguous around the peak. Do not train delay from those misses.
    -- A confirmed Perfect can retain a bounded successful operating point.
    if label=="Perfect" and s.profileKey and not s.compensated then
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
    local correction,margin,bucket=settings._latency.adjust(ping,pingJitter)
    local learned=bucket and settings._latency.buckets[bucket]
    latencyMetrics.Text=settings.latencyCompensation and string.format(
        "Latency: %+.1f ms correction / +%.1f ms jitter margin\nBucket %s ms / %d usable peaks",correction*1000,margin*1000,
        bucket and tostring(bucket*20) or "unavailable",learned and learned.count or 0) or "Latency compensation: OFF"
    latencyToggle.Text=settings.latencyCompensation and "Online latency compensation - ON" or "Online latency compensation - OFF"
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
    local baseDelay=delay
    delay=math.clamp(delay+correction+margin,0,.080)
    local forecast=phase+s.velocity*math.max(0,ping+delay-(settings.timingOffsetMs or 0)/1000)
    -- V2's half-frame advance released below-target forecasts on this capture.
    -- Wait for the modeled arrival phase itself to reach the peak.
    if forecast>=1 then
        s.sent=true; s.sentAt=now; s.sendPhase=phase; s.ping=ping; s.extra=delay
        s.compensated=settings.latencyCompensation==true
        s.baseDelay=baseDelay; s.offset=(settings.timingOffsetMs or 0)/1000
        s.correction=correction; s.jitterMargin=margin; s.pingBucket=bucket
        s.profileKey=key; s.frameTime=frameTime; s.jitter=pingJitter; s.forecast=forecast
        table.insert(awaiting,s)
        local ok,err=pcall(function() keyboard.Services.InputService.Shoot:Fire({Shoot=false}) end)
        if not ok then
            for i=#awaiting,1,-1 do if awaiting[i]==s then table.remove(awaiting,i) end end
            s.cancelled=tostring(err); status.Text="Release call failed · see console."; warn(tostring(err))
        else status.Text="Release sent · waiting for actual game result." end
    end
end)

-- Feature layer: instance-local reversible overrides; no guessed remote commands.
for name,button in pairs(nav) do
    on(button.Activated,function()
        for pageName,page in pairs(pages) do
            page.Visible=pageName==name
            nav[pageName].BackgroundColor3=pageName==name and Color3.fromRGB(39,89,76) or colors.card
        end
    end)
end
nav.Shooting.BackgroundColor3=Color3.fromRGB(39,89,76)
-- Persist numeric tuning only; execution never silently enables an action assist.
local function saveTuning()
    if writefile then pcall(function()
        local values={}
        for _,r in ipairs(settingRows) do values[r.key]=settings[r.key] end
        writefile("PracticalDefenseTuning-v1.json",Http:JSONEncode(values))
    end) end
end
local responseRelaxed=false
on(relaxedPreset.Activated,function()
    responseRelaxed=not responseRelaxed
    local values=responseRelaxed and {guardReaction=.24,guardPrediction=.05,guardStrength=.4,
        guardDeadzone=.9,sprintDistance=4,closeReaction=.15,closeGap=2,
        stealAngle=35,stealMaxSpeed=8,stealDelay=.2,stealCooldown=2,
        blockDelay=.12,jumpCooldown=1.6,reboundLead=.1}
        or {guardReaction=.16,guardPrediction=.08,guardStrength=1,
        guardDeadzone=.65,sprintDistance=2.5,closeReaction=.08,closeGap=1.5,
        stealAngle=45,stealMaxSpeed=10,stealDelay=.12,stealCooldown=1.6,
        blockDelay=.08,jumpCooldown=1.2,reboundLead=.15}
    for key,value in pairs(values) do settings[key]=value end
    for _,r in ipairs(settingRows) do r.refresh() end
    relaxedPreset.Text=responseRelaxed and "Response · Relaxed" or "Response · Balanced"
    saveTuning()
end)
settings.blockDelay=.08;settings.closeReaction=.08;settings.reboundChase=true
settings.stealAngle=45;settings.stealMaxSpeed=10;settings.stealDelay=.12;settings.stealCooldown=1.6
for _,r in ipairs(settingRows) do
    r.refresh()
    for _,item in ipairs(r.buttons) do
        on(item.button.Activated,function()
            settings[r.key]=math.clamp(settings[r.key]+item.delta,r.min,r.max)
            r.refresh()
            saveTuning()
        end)
    end
end
local function featureToggle(button,key,title)
    on(button.Activated,function()
        settings[key]=not settings[key]
        button.Text=title..(settings[key] and " · ON" or " · OFF")
        button.BackgroundColor3=settings[key] and Color3.fromRGB(32,100,77) or colors.card
    end)
end
featureToggle(boostToggle,"boost","Dribble boost (experimental)")
featureToggle(latencyToggle,"latencyCompensation","Online latency compensation")

featureToggle(guardToggle,"guard","Auto guard (experimental)")
featureToggle(contestToggle,"contest","Contest display")
featureToggle(visualToggle,"visuals","Guard floor visuals")
featureToggle(closeoutToggle,"closeout","Shot closeout")
featureToggle(stealToggle,"steal","Auto steal (experimental)")
featureToggle(blockToggle,"block","Auto block (experimental)")
featureToggle(reboundToggle,"rebound","Nearby auto rebound (experimental)")
featureToggle(sprintToggle,"guardSprint","Guard sprint")
featureToggle(facingToggle,"faceTarget","Face handler")
featureToggle(onceToggle,"blockOnce","One block attempt per shot")
featureToggle(chaseToggle,"reboundChase","Chase nearby rebounds")
featureToggle(liveToggle,"liveContest","Live contest estimate")
on(holdToggle.Activated,function()
    settings.guardHold=not settings.guardHold
    holdToggle.Text=settings.guardHold and "Activation · hold G" or "Activation · automatic in range"
end)
local dragOrigin,dragPosition
on(header.InputBegan,function(input)
    if input.UserInputType==Enum.UserInputType.MouseButton1 then dragOrigin=input.Position; dragPosition=panel.Position end
end)
on(Input.InputEnded,function(input)
    if input.UserInputType==Enum.UserInputType.MouseButton1 then dragOrigin=nil end
end)
on(Input.InputChanged,function(input)
    if dragOrigin and input.UserInputType==Enum.UserInputType.MouseMovement then
        local delta=input.Position-dragOrigin
        panel.Position=UDim2.new(dragPosition.X.Scale,dragPosition.X.Offset+delta.X,dragPosition.Y.Scale,dragPosition.Y.Offset+delta.Y)
    end
end)
local boostUntil=0
-- Boost is triggered by recognized dribble state, not a keypress timer.
local patches={}
local hookedMovement
local featureState="Waiting for character controller"
local guardReason="Waiting for movement callback"
local guardCalls=0
local lastGuardStateSave=0
local target,targetSince,targetPosition,targetVelocity
local targetMeasuredAt=0
local steering=Vector3.zero
local lastAcquire=0
local nativeContest
local soloTest
local lockedAssignment,courtBounds,manualUntil
local reportEvents={}
local function recordDefense(kind,data)
    table.insert(reportEvents,{time=os.clock(),kind=kind,data=data or {},localSimulation=soloTest and soloTest.active or false})
    if #reportEvents>500 then table.remove(reportEvents,1) end
end
local function attr(m,key)
    if m==nil then return nil end
    if soloTest and soloTest.active and m==soloTest.dummy then return soloTest.attributes[key] end
    -- The game consumes this mirrored table, including ObjectValue-backed Goal.
    if keyboard and keyboard.Controllers and keyboard.Controllers.Attributes then
        local ok,attributes=pcall(function() return keyboard.Controllers.Attributes:Get(m) end)
        if ok and type(attributes)=="table" and attributes[key]~=nil then return attributes[key] end
    end
    local value=m:GetAttribute(key)
    if value~=nil then return value end
    local folder=m:FindFirstChild("Attributes")
    local child=folder and folder:FindFirstChild(key)
    if child and child:IsA("ValueBase") then return child.Value end
    return nil
end
local function holdsBall(m)
    local value=attr(m,"Basketball")
    return value~=nil and value~=false and value~=0 and value~=""
end
local function rootOf(m) return m and m:FindFirstChild("HumanoidRootPart") end
local function flat(v) return Vector3.new(v.X,0,v.Z) end
local function goalPosition(value)
    if typeof(value)=="Vector3" then return value end
    if typeof(value)=="CFrame" then return value.Position end
    if typeof(value)=="Instance" then
        if value:IsA("BasePart") then return value.Position end
        if value:IsA("Attachment") then return value.WorldPosition end
        if value:IsA("Model") and value.PrimaryPart then return value.PrimaryPart.Position end
    end
end
local function defenseCharacters()
    if soloTest and soloTest.active then return {model(),soloTest.dummy} end
    local folder=workspace:FindFirstChild("Characters")
    return folder and folder:GetChildren() or {}
end
local function chooseHandler(ownModel,range)
    if soloTest and soloTest.active then
        local r=rootOf(ownModel);local other=rootOf(soloTest.dummy)
        if r and other and holdsBall(soloTest.dummy) and flat(r.Position-other.Position).Magnitude<=range then return soloTest.dummy end
        return nil
    end
    local ownRoot=rootOf(ownModel)
    local folder=workspace:FindFirstChild("Characters")
    if not ownRoot or not folder or holdsBall(ownModel) then return nil end
    local tutorialName=workspace:GetAttribute("TutorialAI")
    if type(tutorialName)=="string" and not (settings.assignmentLock and lockedAssignment) then
        local tutorialAI=folder:FindFirstChild(tutorialName)
        local r=rootOf(tutorialAI)
        if tutorialAI and tutorialAI~=ownModel and r and holdsBall(tutorialAI)
            and flat(r.Position-ownRoot.Position).Magnitude<=range then return tutorialAI end
    end
    if settings.assignmentLock and lockedAssignment then
        local r=rootOf(lockedAssignment)
        if not lockedAssignment.Parent then lockedAssignment=nil
        elseif r
            and flat(r.Position-ownRoot.Position).Magnitude<=range then return lockedAssignment end
        if lockedAssignment then return nil end
    end
    local nearest,distance=nil,range
    for _,candidate in ipairs(folder:GetChildren()) do
        if candidate~=ownModel and holdsBall(candidate) then
            local r=rootOf(candidate)
            local other=game:GetService("Players"):GetPlayerFromCharacter(candidate)
                or game:GetService("Players"):FindFirstChild(candidate.Name)
            local sameTeam=other and P.Team~=nil and other.Team==P.Team
            local a,b=attr(ownModel,"Team"),attr(candidate,"Team")
            if a~=nil and b~=nil and a==b then sameTeam=true end
            if r and not sameTeam then
                local d=flat(r.Position-ownRoot.Position).Magnitude
                if d<=distance then nearest=candidate; distance=d end
            end
        end
    end
    return nearest
end
local guardAim,shotTarget,shotSince
local reboundAim,reboundAimAt
local guardError=0
local guardClosing=false
local guardDriving,guardEngaged=false,false
local sprintOwned,guardOwned=false,false
local function defensiveInputs(engaged,moving,forceRelease)
    if not keyboard or not keyboard.Services or not keyboard.Services.InputService then return end
    local service=keyboard.Services.InputService
    local sprint=engaged and moving and (settings._farmAttack or settings.guardSprint~=false) and guardError>=(settings.sprintDistance or 0)
    if sprint and not sprintOwned then
        local ok=pcall(function() service.Sprint:Fire(true) end)
        sprintOwned=ok
    elseif not sprint and sprintOwned then
        pcall(function() service.Sprint:Fire(Input:IsKeyDown(Enum.KeyCode.LeftShift) or Input:IsKeyDown(Enum.KeyCode.RightShift)) end)
        sprintOwned=false
    end
    -- A held guard key must not keep the native defensive stance active during a lob run/jump.
    local lobActive=settings.antiOop and (settings.oopDriving or settings._oopActiveAt and os.clock()-settings._oopActiveAt<.25)
    local guard=engaged and not settings._farmAttack and not lobActive and not forceRelease and not settings._guardReleased and bindingDown(guardKey)
    if guard and not guardOwned then
        local ok=pcall(function() service.HoldG:Fire({HoldingG=true}) end)
        guardOwned=ok
    elseif not guard and guardOwned then
        pcall(function() service.HoldG:Fire({HoldingG=not settings._farmAttack and not forceRelease and not lobActive and bindingDown(guardKey)}) end)
        guardOwned=false
    end
end
-- Complete one native guard-button tap after key-up to clear a sticky stance.
settings._guardReleaseTapTick=function(now)
    local service=keyboard and keyboard.Services and keyboard.Services.InputService
    if settings._guardTapReleaseAt and now>=settings._guardTapReleaseAt then
        settings._guardTapReleaseAt=nil
        if service then pcall(function()service.HoldG:Fire({HoldingG=false})end)end
    end
    if settings._guardTapAt and now>=settings._guardTapAt then
        settings._guardTapAt=nil
        if (settings.guard or settings.denyReceiver)and settings._guardReleased and service and not Input:GetFocusedTextBox() then
            pcall(function()service.HoldG:Fire({HoldingG=true})end)
            settings._guardTapReleaseAt=now+.03
        end
    end
end
-- Release is event-driven; do not wait for the next movement update.
on(Input.InputBegan,function(input,processed)
    if bindingMatches(input,guardKey) and not processed and not Input:GetFocusedTextBox() then
        settings._guardTapAt=nil
        if settings._guardTapReleaseAt then settings._guardTapReleaseAt=nil;pcall(function()keyboard.Services.InputService.HoldG:Fire({HoldingG=false})end)end
        settings._guardReleased=false
    end
end)
on(Input.InputEnded,function(input)
    if not bindingMatches(input,guardKey) then return end
    settings._guardReleased=true
    if settings._denyReceiverReset then settings._denyReceiverReset("Guard key released")end
    steering=Vector3.zero;guardAim=nil;guardClosing=false;guardError=0
    if settings._clearGuardVector then settings._clearGuardVector() end
    guardDriving=false;guardEngaged=false;guardReason="Guard key released"
    defensiveInputs(false,false,true)
    if (settings.guard or settings.denyReceiver)and not Input:GetFocusedTextBox()then settings._guardTapAt=os.clock()end
end)
on(Input.WindowFocusReleased,function()
    settings._guardTapAt=nil
    if settings._guardTapReleaseAt then settings._guardTapReleaseAt=nil;pcall(function()keyboard.Services.InputService.HoldG:Fire({HoldingG=false})end)end
    settings._guardReleased=true
    steering=Vector3.zero;guardAim=nil;guardClosing=false;guardError=0
    if settings._clearGuardVector then settings._clearGuardVector() end
    guardDriving=false;guardEngaged=false
    defensiveInputs(false,false,true)
end)
local function restorePatches()
    settings._guardTapAt=nil
    if settings._guardTapReleaseAt then settings._guardTapReleaseAt=nil;pcall(function()keyboard.Services.InputService.HoldG:Fire({HoldingG=false})end)end
    for i=#patches,1,-1 do
        local p=patches[i]
        pcall(function() if rawget(p.object,p.key)==p.wrapper then rawset(p.object,p.key,p.own) end end)
    end
    settings._clearGuardVector=nil;settings._farmPlantFeet=nil
    patches={}; hookedMovement=nil; target=nil; steering=Vector3.zero
    guardDriving=false;guardEngaged=false
    defensiveInputs(false,false)
end
local function patch(object,key,factory)
    assert(type(object)=="table" and type(object[key])=="function",key.." unavailable")
    local original=object[key]
    local own=rawget(object,key)
    local wrapper=factory(original)
    rawset(object,key,wrapper)
    table.insert(patches,{object=object,key=key,own=own,wrapper=wrapper})
end
local function manualTakeover(now)
    local moving=Input:IsKeyDown(Enum.KeyCode.W) or Input:IsKeyDown(Enum.KeyCode.A)
        or Input:IsKeyDown(Enum.KeyCode.S) or Input:IsKeyDown(Enum.KeyCode.D)
    if moving then manualUntil=now+(settings.manualResume or 0);return true end
    return manualUntil~=nil and now<manualUntil
end
local function shotCommitted(character,since,now)
    if not settings.fakeFilter then return true end
    -- Persistence is a heuristic; it cannot prove intent before release.
    return attr(character,"ReleasedShot")==true or now-(since or now)>=.18
end
local function clampCourt(position)
    if settings._courtClamp then position=settings._courtClamp(position,2.5)end
    if not courtBounds then return position end
    return Vector3.new(math.clamp(position.X,courtBounds.minX+.75,courtBounds.maxX-.75),position.Y,
        math.clamp(position.Z,courtBounds.minZ+.75,courtBounds.maxZ-.75))
end
settings.antiPush=true
settings.rimBlock=true
settings.adaptiveGuard=true
settings._adaptiveGuardState={}
local function predictLanding(position,velocity,floorY,gravity)
    if gravity<=0 or position.Y<floorY then return nil end
    local time=(velocity.Y+math.sqrt(velocity.Y*velocity.Y+2*gravity*(position.Y-floorY)))/gravity
    if time<0 or time>3 then return nil end
    return Vector3.new(position.X+velocity.X*time,floorY,position.Z+velocity.Z*time),time
end
local function guardDirection(movement,dt)
    guardCalls=guardCalls+1;settings.reboundDriving=false
    local function blocked(reason) guardAim=nil;guardClosing=false;guardReason=reason;if settings._humanReset then settings._humanReset()end;return nil end
    local ownModel=model()
    if settings._blockFacingUntil and os.clock()<settings._blockFacingUntil and ownModel and not holdsBall(ownModel)then
        guardReason="Block: hold position and facing";return Vector3.zero
    end
    if settings._farmAttack and settings._farmDirection then local direction,why=settings._farmDirection();if direction then guardReason=why or "Autofarm";guardAim=nil;return direction end end
    if settings._oopDirection then local direction=settings._oopDirection();if direction then return direction end end
    if settings._farmDirection then local direction,why=settings._farmDirection();if direction then guardReason=why or "Autofarm";guardAim=nil;return direction end end
    if settings.antiOop and settings._oopActiveAt and os.clock()-settings._oopActiveAt<.25 then
        guardReason="Lob controller owns positioning";return Vector3.zero
    end
    if settings._interceptDirection then
        local intercept=settings._interceptDirection()
        if intercept then guardReason="Pass interception pursuit";return intercept end
    end
    if settings.denyReceiver and settings._denyReceiverDirection then local denial=settings._denyReceiverDirection(dt);if denial then return denial end end
    if settings._recoveryDirection then
        local recovery=settings._recoveryDirection()
        if recovery then guardReason="Recovering loose ball";guardAim=nil;return recovery end
    end
    if settings.rebound and settings.reboundChase and reboundAim and os.clock()-(reboundAimAt or 0)<.2 and ownModel and not holdsBall(ownModel) and not Input:GetFocusedTextBox() and not manualTakeover(os.clock()) and attr(ownModel,"CanMove")~=false then
        local r=rootOf(ownModel)
        if r then local delta=flat(reboundAim-r.Position);guardError=delta.Magnitude;guardReason="Rebound pursuit";settings.reboundDriving=true;return delta.Magnitude>1 and delta.Unit or Vector3.zero end
    end
    if not settings.guard then steering=Vector3.zero;return blocked("Auto guard OFF") end
    if not ownModel then return blocked("Character unavailable") end
    if Input:GetFocusedTextBox() then steering=Vector3.zero;return blocked("Typing") end
    if settings._guardReleased or not bindingDown(guardKey) then steering=Vector3.zero;return blocked("Hold guard key to engage") end
    if holdsBall(ownModel) then return blocked("You have the ball") end
    if attr(ownModel,"CanMove")==false then return blocked("Game movement lock") end
    -- Respect manual movement, shot/forced motion and the native CanMove checks.
    local action=attr(ownModel,"Action")
    if action=="Shooting" or action=="Dunking" then return blocked("Own shot action") end
    local forced=attr(ownModel,"ForceMoveDirection")
    if action=="Moving" and typeof(forced)=="Vector3" and forced.Magnitude>0.01 then return blocked("Forced movement") end
    local ownRoot=rootOf(ownModel)
    local otherRoot=rootOf(target)
    if not ownRoot then return blocked("Own root unavailable") end
    if not otherRoot then
        if settings.rebound and settings.reboundChase and reboundAim and os.clock()-(reboundAimAt or 0)<.15 then
            guardAim=reboundAim;guardClosing=false
            local delta=flat(reboundAim-ownRoot.Position);guardError=delta.Magnitude
            guardReason="Pursuing nearby rebound"
            return delta.Magnitude>(settings.guardDeadzone or .45) and delta.Unit or Vector3.zero
        end
        return blocked("No ball handler in range")
    end
    if not holdsBall(target) and not (settings.assignmentLock and target==lockedAssignment) then return blocked("Target no longer has ball") end
    if flat(otherRoot.Position-ownRoot.Position).Magnitude>settings.guardRange then target=nil; return blocked("Handler outside range") end
    if os.clock()-(targetSince or os.clock())<settings.guardReaction then return blocked("Reaction delay") end
    local side=goalPosition(attr(target,"Goal"))
    if not side then featureState="Guard waiting for handler basket (Goal)";return blocked("Handler Goal unavailable") end
    local predicted=otherRoot.Position+(targetVelocity or Vector3.zero)*settings.guardPrediction
    side=flat(side-predicted)
    if side.Magnitude<0.01 then return blocked("Invalid basket direction") end
    local shooting=attr(target,"Action")=="Shooting" or attr(target,"Action")=="Dunking"
    if not shooting then shotTarget=nil;shotSince=nil
    elseif shotTarget~=target then shotTarget=target;shotSince=os.clock() end
    guardClosing=settings.closeout and shooting and os.clock()-(shotSince or os.clock())>=settings.closeReaction and shotCommitted(target,shotSince,os.clock())
    local retreatSpeed=-flat(targetVelocity or Vector3.zero):Dot(side.Unit)
    local trackingRetreat=settings.adaptiveGuard and retreatSpeed>2.5
    local gap=guardClosing and settings.closeGap or settings.guardGap
    if settings.adaptiveGuard and not guardClosing and not shooting then
        local velocity=flat(targetVelocity or Vector3.zero)
        local towardRim=math.max(0,velocity:Dot(side.Unit))
        local relative=flat(otherRoot.Position-ownRoot.Position)
        local closing=relative.Magnitude>.1 and math.max(0,-velocity:Dot(relative.Unit))or 0
        local coverage=flat(ownRoot.Position-otherRoot.Position):Dot(side.Unit)
        local threat=math.clamp(towardRim/18,0,1)*math.clamp((settings.guardGap+2-coverage)/4,0,1)
        local desiredGap=settings.guardGap+math.clamp(threat*3+closing*.07,0,4)
        if settings._adaptiveGuardState.target~=target then settings._adaptiveGuardState.gap=settings.guardGap;settings._adaptiveGuardState.target=target end
        settings._adaptiveGuardState.gap=settings._adaptiveGuardState.gap+(desiredGap-settings._adaptiveGuardState.gap)*(1-math.exp(-math.clamp(dt or 1/60,0,.1)*(desiredGap>settings._adaptiveGuardState.gap and 12 or 6)))
        gap=settings._adaptiveGuardState.gap
    else settings._adaptiveGuardState.gap=settings.guardGap;settings._adaptiveGuardState.target=target end
    -- Follow separation immediately; this is movement pressure, not a guessed block.
    if trackingRetreat and not guardClosing then
        gap=math.min(gap,math.max(3.2,settings.guardGap-math.min(2.5,retreatSpeed*.18)))
        settings._adaptiveGuardState.gap=gap
    end
    -- Preserve a containment cushion against a moving rim attacker.
    local rimContain=false
    if settings.rimPress and side.Magnitude<=(settings.rimPressRange or 18) then
        local velocity=flat(targetVelocity or Vector3.zero)
        local driveSpeed=math.max(0,velocity:Dot(side.Unit))
        local lateralSpeed=(velocity-side.Unit*velocity:Dot(side.Unit)).Magnitude
        local coverage=flat(ownRoot.Position-otherRoot.Position):Dot(side.Unit)
        rimContain=settings.adaptiveGuard and not shooting and(driveSpeed>2 or lateralSpeed>4 or coverage<1)
        if rimContain then
            local cushion=math.clamp(2.4+driveSpeed*.13+lateralSpeed*.07,2.4,5)
            gap=math.min(gap,cushion,math.max(1,side.Magnitude-1.5))
        else gap=math.min(gap,settings.rimPressGap or 1.3)end
    end
    local avoidContact=settings.antiPush and side.Magnitude<=(settings.rimPressRange or 18)
    if avoidContact then gap=math.max(gap,3.2)end
    if settings.rimBlock and settings._rimStopAttempt and shooting then
        settings._rimStopAttempt(ownModel,ownRoot,target,os.clock(),keyboard and keyboard.Services and keyboard.Services.InputService)
    end
    local desiredPosition=predicted+side.Unit*gap
    if settings.smartPosition then
        -- Follow the drive angle while retaining basket-side coverage.
        local velocity=targetVelocity or Vector3.zero
        local lateralVelocity=velocity-side.Unit*velocity:Dot(side.Unit)
        local lead=lateralVelocity*(rimContain and .12 or .045)
        local maxLead=rimContain and 1.4 or .65
        if lead.Magnitude>maxLead then lead=lead.Unit*maxLead end
        desiredPosition=clampCourt(desiredPosition+lead)
    end
    if settings._defenseRoute then desiredPosition=settings._defenseRoute(ownModel,target,desiredPosition) end
    if settings.guardHuman and settings._humanize then desiredPosition=settings._humanize(desiredPosition,dt,target,rimContain or trackingRetreat or guardClosing,otherRoot.Position,ownRoot.Position,targetVelocity,side.Unit)end
    desiredPosition=clampCourt(desiredPosition)
    if avoidContact then
        local separation=flat(desiredPosition-otherRoot.Position)
        if separation.Magnitude<3.2 then desiredPosition=otherRoot.Position+(separation.Magnitude>.1 and separation.Unit or side.Unit)*3.2 end
    end
    guardAim=desiredPosition
    local difference=flat(desiredPosition-ownRoot.Position)
    local desired=Vector3.zero
    guardError=difference.Magnitude
    local humanHold=settings.guardHuman and settings._humanHold and settings._humanHold(difference.Magnitude,settings.guardDeadzone or .45,rimContain or trackingRetreat or guardClosing)
    if difference.Magnitude<=(settings.guardHuman and settings._humanIdleActive and .25 or settings.guardDeadzone or .45) or humanHold then steering=Vector3.zero;guardReason="Holding basket-side position";return steering end
    -- Prioritize getting onto the basket line before matching lateral motion.
    local lateral=difference-side.Unit*difference:Dot(side.Unit)
    desired=(difference+lateral*0.6).Unit
    if avoidContact then
        local away=flat(ownRoot.Position-otherRoot.Position)
        if away.Magnitude>.1 then
            local approach=desired:Dot(-away.Unit)
            if away.Magnitude<3.2 then desired=away.Unit
            elseif approach>0 and away.Magnitude<4.5 then
                -- Remove the collision component; slide around the body instead.
                local tangent=desired+away.Unit*approach
                desired=tangent.Magnitude>.1 and tangent.Unit or Vector3.zero
            end
        end
    end
    local blend=1-math.exp(-math.clamp(dt or 1/60,0,0.1)*(((rimContain or trackingRetreat or guardClosing)and 18 or 8)+settings.guardStrength*24))
    if settings.guardHuman and settings._humanSteer and not(avoidContact and flat(ownRoot.Position-otherRoot.Position).Magnitude<4.5)then
        steering=settings._humanSteer(steering,desired,dt,rimContain or trackingRetreat or guardClosing)
    else steering=avoidContact and desired or steering:Lerp(desired,blend)end
    -- Smooth turning, not input magnitude: pursue at full native movement speed.
    if steering.Magnitude>0.001 then steering=steering.Unit end
    guardReason=(guardClosing and "Closing shot · " or "Tracking ")..target.Name
    if settings._courtSteer then steering=settings._courtSteer(ownRoot,steering)end
    return steering
end
local function attachMovement()
    if not keyboard or not keyboard.Controllers or not keyboard.Controllers.Player then return end
    local characterState=keyboard.Controllers.Player.Character
    local movement=characterState and characterState.Movement
    if movement==hookedMovement then return end
    restorePatches()
    if type(movement)~="table" or not movement.Types or not movement.Types.Base then
        featureState="Movement controller unavailable" return
    end
    local ok,err=pcall(function()
        local lastDribbleStamp
        local lastDribbleAction=false
        patch(movement.Types.Base,"UpdateVelocity",function(original)
            return function(self,dt,...)
                original(self,dt,...)
                local m=model()
                if not m then return end
                local action=attr(m,"Action")
                if settings._farmPlantUntil and os.clock()<settings._farmPlantUntil and settings._farmPlantFeet then
                    settings._farmPlantFeet()
                    return
                end
                local dribbling=action=="Dribbling"
                local nativeVelocity=attr(m,"ServerVelocity")
                local scriptedDribble=dribbling and typeof(nativeVelocity)=="Vector3"
                local stamp=movement.lastDribble
                if (dribbling and not lastDribbleAction) or (dribbling and stamp~=lastDribbleStamp)
                    or (lastDribbleAction and not dribbling) then
                    boostUntil=os.clock()+settings.boostDuration
                end
                lastDribbleAction=dribbling
                lastDribbleStamp=stamp
                if not settings.boost or (not scriptedDribble and os.clock()>=boostUntil) or not holdsBall(m)
                    or (attr(m,"CanMove")==false and not scriptedDribble)
                    or action=="Shooting" or action=="Dunking" then return end
                local actuator=movement.MovementVelocity
                if not actuator then featureState="Boost actuator unavailable" return end
                local velocity=actuator.Velocity
                if typeof(velocity)~="Vector3" then return end
                local horizontal=flat(velocity)
                if horizontal.Magnitude<0.1 then return end
                local boosted=horizontal*settings.boostMultiplier
                if scriptedDribble then
                    -- Bound against the native target, avoiding repeated-frame compounding.
                    local targetSpeed=flat(nativeVelocity).Magnitude*settings.boostMultiplier
                    if boosted.Magnitude>targetSpeed then boosted=boosted.Unit*targetSpeed end
                end
                if boosted.Magnitude>27 then boosted=boosted.Unit*27 end
                actuator.Velocity=Vector3.new(boosted.X,velocity.Y,boosted.Z)
            end
        end)
        patch(movement,"IsSprinting",function(original)
            return function(self,...)
                if settings._farmPlantUntil and os.clock()<settings._farmPlantUntil then return false end
                if settings._farmAttack and settings._farmSprintWanted~=nil then return settings._farmSprintWanted end
                if guardDriving and (settings._farmAttack or settings.guardSprint~=false) and guardError>=(settings.sprintDistance or 0) then return true end
                return original(self,...)
            end
        end)
        if movement.Types.Guard and type(movement.Types.Guard.UpdateTurn)=="function" then
            patch(movement.Types.Guard,"UpdateTurn",function(original)
                return function(self,dt,...)
                    original(self,dt,...)
                    local own=model();local r=rootOf(target)
                    local lob=settings._oopFacePosition and os.clock()-(settings._oopFaceAt or 0)<.2 and settings._oopFacePosition
                    local denial=settings.denyReceiver and settings._denyReceiverFacePosition and os.clock()-(settings._denyReceiverFaceAt or 0)<.2 and settings._denyReceiverFacePosition
                    local aim=settings._blockFacingUntil and os.clock()<settings._blockFacingUntil and self.Root and(self.Root.Position+settings._blockFacingDirection*8)or lob or denial or r and r.Position
                    if own and aim and (lob or (settings.guard or settings.denyReceiver)and guardEngaged) and attr(own,"CanTurn")~=false then
                        local movementRoot=movement.Root
                        local orientation=movement.MovementOrientation
                        if movementRoot and orientation and flat(aim-movementRoot.Position).Magnitude>0.05 then
                            orientation.CFrame=CFrame.lookAt(movementRoot.Position,Vector3.new(aim.X,movementRoot.Position.Y,aim.Z))
                        end
                    end
                end
            end)
        end
        local guardVector
        -- Clear cached input before E, and brake only after native dribble motion ends.
        settings._farmPlantFeet=function()
            local action=attr(model(),"Action")
            if action=="Dunking"or action=="Layup"then return end
            guardVector=Vector3.zero;guardDriving=false;steering=Vector3.zero
            movement.MoveDirection=Vector3.zero;movement.lastMoveDirection=Vector3.zero
            movement.forwardValue=0;movement.rightValue=0
            movement:SendMoveDirection(Vector3.zero)
            if action=="Dribbling"then return end
            local actuator=movement.MovementVelocity
            if actuator and typeof(actuator.Velocity)=="Vector3"then
                actuator.Velocity=Vector3.new(0,actuator.Velocity.Y,0)
            end
            local root=movement.Root or rootOf(model())
            if root then root.AssemblyLinearVelocity=Vector3.new(0,root.AssemblyLinearVelocity.Y,0)end
        end
        settings._clearGuardVector=function()
            if (not guardVector and not guardEngaged) or settings.recoveryDriving or settings.reboundDriving or settings.interceptDriving or settings.oopDriving then return end
            guardVector=nil
            movement.MoveDirection=Vector3.zero;movement.lastMoveDirection=Vector3.zero
            movement.forwardValue=0;movement.rightValue=0
            pcall(function()movement:SendMoveDirection(Vector3.zero)end)
        end
        patch(movement,"SendMoveDirection",function(original)
            return function(self,direction,...)
                if settings._farmPlantUntil and os.clock()<settings._farmPlantUntil then direction=Vector3.zero;guardVector=Vector3.zero end
                return original(self,guardVector or direction,...)
            end
        end)
        patch(movement,"UpdateMoveDirection",function(original)
            return function(self,dt,...)
                local successful,direction=pcall(guardDirection,self,dt)
                guardVector=successful and direction or nil
                guardEngaged=guardVector~=nil
                guardDriving=guardEngaged and guardVector.Magnitude>0.01
                defensiveInputs(guardVector~=nil,guardDriving)
                local planned=guardVector
                guardVector=nil
                original(self,dt,...)
                guardVector=planned
                if settings._farmPlantUntil and os.clock()<settings._farmPlantUntil then
                    settings._farmPlantFeet();guardVector=nil;return
                end
                local lob=settings._oopFacePosition;local faceAt=settings._oopFaceAt or 0
                if not lob or os.clock()-faceAt>=.2 then lob=settings.denyReceiver and settings._denyReceiverFacePosition;faceAt=settings._denyReceiverFaceAt or 0 end
                if lob and os.clock()-faceAt<.2 and self.Root and self.MovementOrientation and attr(model(),"CanTurn")~=false and flat(lob-self.Root.Position).Magnitude>.05 then
                    self.MovementOrientation.CFrame=CFrame.lookAt(self.Root.Position,Vector3.new(lob.X,self.Root.Position.Y,lob.Z))
                end
                if guardVector then
                    local manual=Input:IsKeyDown(Enum.KeyCode.W) or Input:IsKeyDown(Enum.KeyCode.A) or Input:IsKeyDown(Enum.KeyCode.S) or Input:IsKeyDown(Enum.KeyCode.D)
                    local native=self.MoveDirection
                    if manual and typeof(native)=="Vector3" and flat(native).Magnitude>.01 then
                        -- Player steering stays stronger than the guard correction.
                        -- Normalize the result to keep the native movement magnitude.
                        local input=flat(native)
                        local mixed=input+flat(guardVector)*.65
                        guardVector=mixed.Magnitude>.001 and mixed.Unit*math.min(1,input.Magnitude) or Vector3.zero
                        guardDriving=guardVector.Magnitude>.01
                        guardReason="Guarding with manual steering"
                        defensiveInputs(true,guardDriving)
                    end
                    self.MoveDirection=guardVector
                    self.lastMoveDirection=guardVector
                    self:SendMoveDirection(guardVector)
                    if self.Root then
                        self.forwardValue=self.Root.CFrame.LookVector:Dot(guardVector)
                        self.rightValue=self.Root.CFrame.RightVector:Dot(guardVector)
                    end
                elseif not successful then featureState="Guard error: "..tostring(direction) end
                if not guardVector and settings._smartOffenseDirection then
                    local offenseOk,offenseDirection=pcall(settings._smartOffenseDirection)
                    if offenseOk and typeof(offenseDirection)=="Vector3" then
                        self.MoveDirection=offenseDirection;self.lastMoveDirection=offenseDirection
                        settings._smartOffenseMoving=offenseDirection.Magnitude>.01
                        self:SendMoveDirection(offenseDirection)
                        settings._smartOffenseAppliedAt=os.clock()
                        if self.Root then
                            self.forwardValue=self.Root.CFrame.LookVector:Dot(offenseDirection)
                            self.rightValue=self.Root.CFrame.RightVector:Dot(offenseDirection)
                        end
                    end
                end
                guardVector=nil
            end
        end)
    end)
    if ok then hookedMovement=movement; featureState="Native movement hooks attached · effects unverified"
    else restorePatches(); featureState="Feature hooks unavailable: "..tostring(err) end
end
-- Local-only presentation: noncolliding floor ribbons and destination rings.
local visualFolder=Instance.new("Folder") visualFolder.Name="PracticalGuardVisuals" visualFolder.Parent=workspace
local function visualPart(name,color)
 return create("Part",{Name=name,Anchored=true,CanCollide=false,CanTouch=false,CanQuery=false,
    Material=Enum.Material.Neon,Color=color,Transparency=.3,Size=Vector3.new(.08,.025,.08)},visualFolder)
end
local route=visualPart("Destination route",colors.accent)
local handlerLine=visualPart("Handler line",Color3.fromRGB(104,151,235))
local marker=visualPart("Destination",colors.accent)
marker.Shape=Enum.PartType.Cylinder
local function hideVisuals() route.Transparency=1;handlerLine.Transparency=1;marker.Transparency=1 end
local function floorPoint(position,own)
 local params=RaycastParams.new();params.FilterType=Enum.RaycastFilterType.Exclude
 local excluded={visualFolder};local chars=workspace:FindFirstChild("Characters")
 if chars then table.insert(excluded,chars) end
 local proxy=workspace:FindFirstChild("ProxyCharacter");if proxy then table.insert(excluded,proxy) end
 params.FilterDescendantsInstances=excluded
 local hit=workspace:Raycast(position+Vector3.new(0,3,0),Vector3.new(0,-20,0),params)
 return hit and hit.Position+Vector3.new(0,.06,0) or position-Vector3.new(0,2.8,0)
end
local function ribbon(part,a,b,width)
 local length=(b-a).Magnitude
 if length<.05 then part.Transparency=1;return end
 part.Size=Vector3.new(width,.035,length);part.CFrame=CFrame.lookAt((a+b)/2,b);part.Transparency=.35
end
local landingPrediction
local updateLanding=(function()
 settings.landingBeam=true;settings.landingZone=true;settings.landingBeamWidth=.065;settings.landingZoneScale=1;settings.landingRange=60
 local color=Color3.fromRGB(92,166,255)
 local nodes,attachments,beams,ring={},{},{},{}
 for i=1,15 do
  nodes[i]=visualPart("Trajectory node",color);nodes[i].Transparency=1
  attachments[i]=create("Attachment",{},nodes[i])
  if i>1 then beams[i-1]=create("Beam",{Name="Estimated ball flight",Attachment0=attachments[i-1],Attachment1=attachments[i],
   FaceCamera=true,Width0=.065,Width1=.065,Segments=1,LightEmission=.55,
   Color=ColorSequence.new(color),Transparency=NumberSequence.new(.2),Enabled=false},visualFolder) end
 end
 for i=1,40 do ring[i]=visualPart("Landing zone",color) end
 local smoothPoint,lastPredictionTime,lastPosition
 local center=visualPart("Landing center",color);center.Shape=Enum.PartType.Cylinder
 local function hide()
  for _,beam in ipairs(beams) do beam.Enabled=false end
  for _,part in ipairs(ring) do part.Transparency=1 end
  center.Transparency=1;landingPrediction=nil
 end
 hide()
 return function(position,velocity,ownRoot,now,ballUpdated)
  if settings.antiOop and settings._oopActiveAt and now-settings._oopActiveAt<.25 then hide();smoothPoint=nil;return end
  if not settings.landing or not position or not velocity or not ballUpdated or now-ballUpdated>.35 then hide();smoothPoint=nil;return end
  if flat(position-ownRoot.Position).Magnitude>settings.landingRange then hide();smoothPoint=nil;return end
  local floor=floorPoint(ownRoot.Position,model())
  local gravity=workspace.Gravity
  local point,time=predictLanding(position,velocity,floor.Y+.65,gravity)
  if not point then hide();smoothPoint=nil;return end
  if lastPosition and (position-lastPosition).Magnitude>15 then smoothPoint=nil end
  local dt=math.clamp(now-(lastPredictionTime or now-.03),.001,.15)
  smoothPoint=smoothPoint and smoothPoint:Lerp(point,1-math.exp(-8*dt)) or point
  point=smoothPoint;lastPredictionTime=now;lastPosition=position
  local speed=flat(velocity).Magnitude
  local radius=math.clamp(.65+speed*time*.2+(ping or 0)*speed,.65,4)*(settings.landingZoneScale or 1)
  landingPrediction={position=point,time=time,radius=radius,estimated=true}
  if not settings.landingBeam or settings._visualV2 then for _,beam in ipairs(beams) do beam.Enabled=false end end
  if not settings.landingZone or settings._visualV2 then center.Transparency=1;for _,part in ipairs(ring) do part.Transparency=1 end end
  if settings.landingBeam and not settings._visualV2 then
   for i,node in ipairs(nodes) do
    local t=time*(i-1)/14
    node.Position=position+velocity*t-Vector3.new(0,.5*gravity*t*t,0)
   end
   for _,beam in ipairs(beams) do beam.Width0=settings.landingBeamWidth;beam.Width1=settings.landingBeamWidth;beam.Enabled=true end
  end
  if settings.landingZone and not settings._visualV2 then
   local ground=Vector3.new(point.X,floor.Y+.035,point.Z)
   center.Size=Vector3.new(.025,.35,.35);center.CFrame=CFrame.new(ground)*CFrame.Angles(0,0,math.pi/2);center.Transparency=.15
   for i,part in ipairs(ring) do
    local a=(i-1)*math.pi/20;local b=(i-.14)*math.pi/20
    ribbon(part,ground+Vector3.new(math.cos(a)*radius,0,math.sin(a)*radius),ground+Vector3.new(math.cos(b)*radius,0,math.sin(b)*radius),.055)
    part.Transparency=.2
   end
  end
 end
end)()

local lastSteal,lastJump=-math.huge,-math.huge
local opportunitySince,opportunityTarget
local blockedShotTarget,blockedShotSince
local observedShooter,observedShotSince
local jumpReleaseAt
local actionCounts={steal=0,block=0,rebound=0}
local lastObservedAction=""
local trackedBall,ballPosition,ballTime,ballVelocity
local assistState="Defensive actions idle"
local function updateLiveContest(own)
 liveHud.Visible=false
 if not settings.liveContest then settings.livePressure=nil;settings.hudPressure=nil;return end
 local shooter=holdsBall(own) and own or chooseHandler(own,30)
 local sr=rootOf(shooter);local goal=shooter and goalPosition(attr(shooter,"Goal"))
 settings.livePressure=nil;settings.hudPressure=nil
 local highest,nearest=0,math.huge
 local chars=workspace:FindFirstChild("Characters")
 if sr and chars then
   for _,defender in ipairs(defenseCharacters()) do
     local dr=rootOf(defender)
     local sameTeam=attr(shooter,"Team")~=nil and attr(shooter,"Team")==attr(defender,"Team")
     if defender~=shooter and dr and not sameTeam then
       local delta=flat(dr.Position-sr.Position);local distance=delta.Magnitude
       nearest=math.min(nearest,distance)
       local facing=.5
       if goal and distance>.01 and flat(goal-sr.Position).Magnitude>.01 then
         facing=math.clamp((flat(goal-sr.Position).Unit:Dot(delta.Unit)+1)/2,0,1)
       end
       -- Presentation heuristic only: not the game's contest formula.
       local estimate=math.clamp(1-distance/10,0,1)*(.35+.65*facing)
       if math.abs(dr.Position.Y-sr.Position.Y)>5 then estimate=estimate*.5 end
       highest=math.max(highest,estimate)
     end
   end
 end
 if not sr then
   liveTitle.Text="ESTIMATED CONTEST · no ball handler";liveDetail.Text="Waiting for possession";liveFill.Size=UDim2.fromScale(0,1);return
 end
 settings.hudPressure=highest*100;settings.hudPressureAt=os.clock()
 if shooter==own then settings.livePressure=highest*100;settings.livePressureAt=os.clock() end
 local color=highest>.65 and Color3.fromRGB(239,100,96) or highest>.3 and Color3.fromRGB(243,185,83) or colors.accent
 liveTitle.Text=string.format("ESTIMATED CONTEST · %.0f%%",highest*100)
 liveTitle.TextColor3=color;liveFill.BackgroundColor3=color;liveFill.Size=UDim2.fromScale(highest,1)
 liveDetail.Text=shooter.Name..(nearest<math.huge and string.format(" · closest %.1f studs",nearest) or " · no defenders")
end
local function updateAssists(now)
 local service=keyboard and keyboard.Services and keyboard.Services.InputService
 if jumpReleaseAt and now>=jumpReleaseAt then
    jumpReleaseAt=nil
    if service then service.Shoot:Fire({Shoot=false}) end
 end
 local own=model();local root=rootOf(own)
 updateLiveContest(own)
 reboundAim=nil
 if not own or not root then hideVisuals();if updateLanding then updateLanding(nil,nil,nil,now,nil) end;return end
 local engaged=((settings.guard or settings.denyReceiver)and (not settings.guardHold or bindingDown(guardKey))) or settings.steal or settings.block or settings.rebound
 if settings.visuals and not settings._visualV2 and engaged and guardEngaged and guardAim then
    local a=floorPoint(root.Position,own);local b=floorPoint(guardAim,own)
    ribbon(route,a,b,.13)
    if rootOf(target) then ribbon(handlerLine,a,floorPoint(rootOf(target).Position,own),.045) else handlerLine.Transparency=1 end
    marker.Size=Vector3.new(.045,1.25,1.25)
    marker.CFrame=CFrame.new(b)*CFrame.Angles(0,0,math.pi/2);marker.Transparency=.55
    route.Color=guardClosing and Color3.fromRGB(255,163,76) or colors.accent
 else hideVisuals() end
 -- Discover loose balls directly, including shots never observed in a hand.
 local chars=workspace:FindFirstChild("Characters")
 local occupied={}
 local shooterCandidate,shooterDistance=nil,settings.guardRange
 if chars then for _,character in ipairs(defenseCharacters()) do
    local held=attr(character,"Basketball")
    if typeof(held)=="Instance" then occupied[held]=true end
    local r=rootOf(character);local a=attr(character,"Action")
    local sameTeam=attr(own,"Team")~=nil and attr(own,"Team")~=0 and attr(own,"Team")==attr(character,"Team")
    if character~=own and not sameTeam and r and (a=="Shooting" or a=="Dunking") then
       local distance=flat(r.Position-root.Position).Magnitude
       if distance<shooterDistance then shooterCandidate=character;shooterDistance=distance end
    end
 end end
 if not shooterCandidate and settings.block and observedShooter==settings._blockTrackShooter
  and now<(settings._blockTrackUntil or 0)then shooterCandidate=observedShooter end
 if shooterCandidate~=observedShooter then observedShooter=shooterCandidate;observedShotSince=now end
 local nearestBall,nearestDistance=nil,math.max(settings.reboundChaseRange or 12,settings.landing and settings.landingRange or 0)
 local balls=soloTest and soloTest.active and soloTest.balls or workspace:FindFirstChild("Basketballs")
 if balls then for _,candidate in ipairs(balls:GetChildren()) do
    local position=goalPosition(candidate)
    if position and not occupied[candidate] then
       local distance=flat(position-root.Position).Magnitude
       if distance<nearestDistance then nearestBall=candidate;nearestDistance=distance end
    end
 end end
 local candidate=nearestBall or (target and attr(target,"Basketball"))
 if settings._flightBall then candidate=settings._flightBall(balls,occupied,root,now) end
 if not candidate then trackedBall=nil;ballPosition=nil;ballTime=nil;ballVelocity=nil end
 if typeof(candidate)=="Instance" and candidate~=trackedBall then
    if updateLanding then updateLanding(nil,nil,root,now,nil) end
    trackedBall=candidate;ballPosition=nil;ballTime=nil;ballVelocity=nil
 end
 local ownAction=attr(own,"Action")
 if ownAction~=lastObservedAction then
    if ownAction=="Blocking" or ownAction=="Rebounding" or ownAction=="Stealing" then
        assistState="Game action observed · "..ownAction;recordDefense("action_observed",{action=ownAction})
    end
    lastObservedAction=ownAction
 end
 local ball=trackedBall and trackedBall.Parent and goalPosition(trackedBall)
 if ball then
    if not ballPosition then ballPosition=ball;ballTime=now
    elseif (ball-ballPosition).Magnitude>.01 and now-ballTime>.002 then
        local measured=(ball-ballPosition)/(now-ballTime)
        ballVelocity=ballVelocity and ballVelocity:Lerp(measured,math.clamp(1-math.exp(-12*(now-ballTime)),.1,.8)) or measured
        ballPosition=ball;ballTime=now
    end
 end
 if settings._flightVelocity and settings._flightSelected==trackedBall then ballVelocity=settings._flightVelocity;ballTime=now end
 if updateLanding then updateLanding(not occupied[trackedBall] and ball or nil,ballVelocity,root,now,ballTime) end
 if not engaged or Input:GetFocusedTextBox() or holdsBall(own) or attr(own,"CanMove")==false then
    opportunitySince=nil;opportunityTarget=nil;return
 end
 if manualTakeover(now) then return end
 if not service then return end
 local targetRoot=rootOf(target)
 local shooting=target and (attr(target,"Action")=="Shooting" or attr(target,"Action")=="Dunking")
 local distance=targetRoot and flat(targetRoot.Position-root.Position).Magnitude or math.huge
 local stealPosition=target and goalPosition(attr(target,"Basketball"))
 -- A reach opportunity is only observable geometry, not a server success signal.
 local reach=settings.steal and targetRoot and holdsBall(target) and not shooting
    and stealPosition and flat(stealPosition-root.Position).Magnitude>.05 and flat(stealPosition-root.Position).Magnitude<=settings.stealRange
    and math.abs(stealPosition.Y-root.Position.Y)<3.5
    and root.CFrame.LookVector:Dot(flat(stealPosition-root.Position).Unit)>math.cos(math.rad(settings.stealAngle or 69.5))
    and (not targetVelocity or targetVelocity.Magnitude<=(settings.stealMaxSpeed or 40))
 if reach then
    if opportunityTarget~=target then opportunityTarget=target;opportunitySince=now end
    if now-(opportunitySince or now)>=settings.stealDelay and now-lastSteal>=settings.stealCooldown then
       settings._stealAttempt(service,own,target);recordDefense("attempt_sent",{action="Steal",touch=false,localHitbox=false});actionCounts.steal=actionCounts.steal+1;lastSteal=now;opportunitySince=now;assistState="Steal sent · result not confirmed"
    end
 else opportunitySince=nil;opportunityTarget=nil end
 local jumpReason
 local blockRoot=rootOf(observedShooter)
 local lobReserved=settings.antiOop and settings._oopOwnsBall and (settings._oopOwnsBall(trackedBall)or settings._oopOwnsBall(observedShooter and attr(observedShooter,"Basketball"))or settings._oopActiveAt and now-settings._oopActiveAt<.2)
 if settings.antiOop and not lobReserved and settings._oopOwnsBall then
  local balls=workspace:FindFirstChild("Basketballs")
  if balls then for _,candidateBall in ipairs(balls:GetChildren())do
   local position=goalPosition(candidateBall)
   if position and flat(position-root.Position).Magnitude<=(settings.oopRange or 18)+4 and settings._oopOwnsBall(candidateBall)then lobReserved=true;break end
  end end
 end
 if settings.block and not lobReserved and blockRoot and observedShotSince
    and (settings._blockReady and settings._blockReady(observedShooter,observedShotSince,root,now)) and (not settings.blockCommitFilter or shotCommitted(observedShooter,observedShotSince,now))
    and flat(blockRoot.Position-root.Position).Magnitude<=settings.blockRange
    and (not settings.blockOnce or blockedShotTarget~=observedShooter or blockedShotSince~=observedShotSince) then jumpReason="Block" end
 if settings.rebound and not lobReserved and settings.reboundChase and landingPrediction and ball and ballVelocity and ballVelocity.Y<0 and not occupied[trackedBall] then
    local point=landingPrediction.position
    if flat(point-root.Position).Magnitude<=settings.reboundChaseRange then reboundAim=point;reboundAimAt=now end
 end
 if settings.rebound and not lobReserved and ball and ballVelocity and ballVelocity.Y<-.5
    and flat(ball-root.Position).Magnitude<=math.max(settings.reboundRange,settings.reboundChase and settings.reboundChaseRange or 0)
    and ball.Y-root.Position.Y>1 and ball.Y-root.Position.Y<=settings.reboundHeight then
    local possessed=false;local chars=workspace:FindFirstChild("Characters")
    if chars then for _,m in ipairs(defenseCharacters()) do if attr(m,"Basketball")==trackedBall then possessed=true;break end end end
    if not possessed then
       if settings.reboundChase then
         local lead=flat(ballVelocity)*(settings.reboundLead or .15)
         if lead.Magnitude>3 then lead=lead.Unit*3 end
         local predicted=ball+lead
         if flat(predicted-root.Position).Magnitude<=settings.reboundChaseRange then
           reboundAim=Vector3.new(predicted.X,root.Position.Y,predicted.Z);reboundAimAt=now
         end
       end
       if flat(ball-root.Position).Magnitude<=settings.reboundRange then jumpReason="Rebound" end
    end
 end
 -- Early rebound positioning: head for the rim spot while a shot is still in the air.
 if settings.rebound and settings.reboundChase and settings.reboundEarly and not lobReserved and not reboundAim and ball and settings._reboundEarlySpot then
    local ok,spot=pcall(settings._reboundEarlySpot,root,ball,trackedBall)
    if ok and spot then reboundAim=spot;reboundAimAt=now end
 end
 -- Space is context-sensitive and can become a dive near a low loose ball.
 -- Never dispatch an automated block/rebound unless its ball is above reach.
 local jumpBall=jumpReason=="Block"and settings._blockDecision and settings._blockDecision.position or ball
 if jumpReason and(not jumpBall or jumpBall.Y-root.Position.Y<(jumpReason=="Block"and 1.8 or 3))then jumpReason=nil end
 local action=attr(own,"Action")
 local grounded=false
 if jumpReason then
    local params=RaycastParams.new();params.FilterType=Enum.RaycastFilterType.Exclude
    local excluded={visualFolder};local chars=workspace:FindFirstChild("Characters")
    if chars then table.insert(excluded,chars) end
    local proxy=workspace:FindFirstChild("ProxyCharacter");if proxy then table.insert(excluded,proxy) end
    params.FilterDescendantsInstances=excluded
    grounded=workspace:Raycast(root.Position,Vector3.new(0,-4.5,0),params)~=nil
 end
 if jumpReason and grounded and now-lastJump>=settings.jumpCooldown and action~="Blocking" and action~="Rebounding"
    and attr(own,"CanJump")~=false then
    -- Native jump is context-sensitive. No teleport, hand placement or result override.
    -- Match native Space press: Shoot(UseSpace), then Jump, then release Shoot.
    if jumpReason=="Block"then
       settings._blockFacingDirection=flat(root.CFrame.LookVector);settings._blockFacingUntil=now+.45
    end
    service.Shoot:Fire({Shoot=true,Input="E",UseSpace=true})
    print("[Practical Lob] OTHER JUMP: "..tostring(jumpReason));service.Jump:Fire();settings._lastDefenseJump={source=jumpReason,time=now};recordDefense("attempt_sent",{action=jumpReason});jumpReleaseAt=now+.08;lastJump=now
    actionCounts[jumpReason=="Block" and "block" or "rebound"]=actionCounts[jumpReason=="Block" and "block" or "rebound"]+1
    assistState=jumpReason.." input sent · waiting for game action"
    if jumpReason=="Block" then blockedShotTarget=observedShooter;blockedShotSince=observedShotSince end
 end
end
featureCleanup=function()
    if jumpReleaseAt and keyboard then pcall(function() keyboard.Services.InputService.Shoot:Fire({Shoot=false}) end) end
    jumpReleaseAt=nil;restorePatches();visualFolder:Destroy()
end
local lastFeatureUpdate=0
on(Run.Heartbeat,function()
    local now=os.clock()
    settings._guardReleaseTapTick(now)
    if now-lastFeatureUpdate<1/30 then return end
    lastFeatureUpdate=now
    attachMovement()
    if (not(settings.guard or settings.denyReceiver)or (settings._guardReleased or not bindingDown(guardKey))) and not settings.recoveryDriving and not settings.reboundDriving and not settings.interceptDriving and not settings.oopDriving then
        guardDriving=false;guardEngaged=false; defensiveInputs(false,false)
    end
    if (settings.guard or settings.denyReceiver or settings.steal or settings.block or settings.rebound or settings.assignmentLock) and now-lastAcquire>1/30 then
        lastAcquire=now
        local ownModel=model()
        local chosen=ownModel and chooseHandler(ownModel,settings.guardRange)
        if settings.assignmentLock and chosen and not lockedAssignment then lockedAssignment=chosen end
        if chosen~=target then
            target=chosen; targetSince=now; targetPosition=nil; targetVelocity=Vector3.zero
        end
        local r=rootOf(target)
        if r then
            local elapsed=now-targetMeasuredAt
            if targetPosition and elapsed>0 and elapsed<0.5 then
                local v=flat(r.Position-targetPosition)/elapsed
                if v.Magnitude<40 then targetVelocity=(targetVelocity or v):Lerp(v,0.5) end
            end
            targetPosition=r.Position; targetMeasuredAt=now
        end
    end
    updateAssists(now)
    defenseHint.Text=assistState
    env.PracticalDefenseActions={state=assistState,counts=actionCounts,
        observedAction=lastObservedAction,shooter=observedShooter and observedShooter.Name,
        ball=trackedBall and trackedBall.Name,ballVelocityY=ballVelocity and ballVelocity.Y}
    if writefile and now-lastGuardStateSave>=1 then pcall(function()
        writefile("PracticalDefenseActions.json",Http:JSONEncode(env.PracticalDefenseActions))
    end) end
    contestLabel.Visible=settings.contest
    if settings.contest then
        contestLabel.Text=nativeContest and string.format("Last shot contest: %.0f%%",nativeContest) or "Last shot contest: waiting for feedback"
    end
    local ownModel=model()
    local stamina=ownModel and attr(ownModel,"Stamina")
    local max=ownModel and attr(ownModel,"MaxStamina")
    featureStatus.Text=assistState.."\n"..guardReason.."\nTarget: "..(target and target.Name or "none in range")..
        "\nActual stamina: "..tostring(stamina or "unknown").." / "..tostring(max or "unknown")
end)
task.spawn(function()
    while alive and not keyboard do task.wait(0.1) end
    if not alive then return end
    on(keyboard.Services.InterfaceService.ShowFeedback,function(m,contest)
        if m==model() then
            local value=tonumber(contest)
            if finite(value) then nativeContest=value end
        end
    end)
end)

-- Small live status file: enough to identify a guard blocker without another full capture.
on(Run.Heartbeat,function()
    local now=os.clock()
    if now-lastGuardStateSave<1 then return end
    lastGuardStateSave=now
    local own=model()
    local info={version="2.3",enabled=settings.guard,holdG=settings.guardHold,
        g=bindingDown(guardKey),reason=guardReason,hookCalls=guardCalls,
        hooksAttached=hookedMovement~=nil,featureStatus=featureState,
        target=target and target.Name or "none",range=settings.guardRange}
    if own then
        info.action=tostring(attr(own,"Action"))
        info.canMove=attr(own,"CanMove")
        info.hasBall=holdsBall(own)
        local forced=attr(own,"ForceMoveDirection")
        info.forcedType=typeof(forced)
        if typeof(forced)=="Vector3" then info.forcedMagnitude=forced.Magnitude end
    end
    if settings.guard then status.Text="Guard · "..guardReason end
    env.PracticalGuardState=info
    if writefile then pcall(function() writefile("PracticalGuardState.json",Http:JSONEncode(info)) end) end
end)

-- Local rehearsal only: the server does not know this opponent or ball.
local soloButton=row(pages.Defense,"Start ALL-ASSIST solo test",408)
pages.Defense.CanvasSize=UDim2.fromOffset(0,462)
local soloInfo=create("Frame",{Size=UDim2.fromOffset(370,92),Position=UDim2.new(.5,-185,1,-115),
    BackgroundColor3=colors.bg,BackgroundTransparency=.08,BorderSizePixel=0,Visible=false},gui)
create("UICorner",{CornerRadius=UDim.new(0,10)},soloInfo)
local soloTitle=label(soloInfo,"LOCAL REHEARSAL",4,24,14)
local soloDetail=label(soloInfo,"Hold G to run; release to pause.",30,53,11)
local function vlist(v) return {v.X,v.Y,v.Z} end
local function saveSoloTest(test,complete)
    local data={version=2,localSimulation=true,serverSuccessTest=false,complete=complete,
        elapsed=test.elapsed,stage=test.stage,frames=test.frames}
    env.PracticalGuardSoloResults=data
    if writefile then pcall(function() writefile("PracticalGuardSoloResults.json",Http:JSONEncode(data)) end) end
end
local function syncSoloToggles()
    for _,item in ipairs({{guardToggle,"guard","Auto guard"},{stealToggle,"steal","Auto steal"},
        {blockToggle,"block","Auto block"},{reboundToggle,"rebound","Auto rebound"}}) do
        item[1].Text=item[3]..(settings[item[2]] and " · ON" or " · OFF")
        item[1].BackgroundColor3=settings[item[2]] and Color3.fromRGB(32,100,77) or colors.card
    end
    holdToggle.Text=settings.guardHold and "Activation · hold G" or "Activation · automatic in range"
end
local function stopSoloTest(completed)
    local test=soloTest
    if not test then return end
    test.active=false
    saveSoloTest(test,completed==true)
    for key,value in pairs(test.previousSettings) do settings[key]=value end
    target=nil;steering=Vector3.zero;guardAim=nil;reboundAim=nil;trackedBall=nil
    guardEngaged=false;guardDriving=false;defensiveInputs(false,false)
    soloTest=nil;test.folder:Destroy();soloInfo.Visible=false
    soloButton.Text="Start ALL-ASSIST solo test"
    syncSoloToggles()
    status.Text=completed and "Local rehearsal saved · attempts only, not server results." or "Solo test stopped; previous settings restored."
end
local cleanupBeforeSolo=featureCleanup
featureCleanup=function() stopSoloTest(false);if cleanupBeforeSolo then cleanupBeforeSolo() end end
local function startSoloTest()
    if soloTest then stopSoloTest(false);return end
    local own=model();local r=rootOf(own)
    if not r then status.Text="Character unavailable.";return end
    if holdsBall(own) then status.Text="Shoot or drop your real ball before starting the solo test.";return end
    local forward=flat(r.CFrame.LookVector)
    if forward.Magnitude<.1 then forward=Vector3.new(0,0,-1) else forward=forward.Unit end
    local folder=create("Folder",{Name="Practical_LOCAL_AllAssistTest"},workspace)
    local dummy=create("Model",{Name="LOCAL_TEST_OPPONENT"},folder)
    local function part(name,size,color,parent)
        return create("Part",{Name=name,Size=size,Anchored=true,CanCollide=false,CanTouch=false,CanQuery=false,
            Color=color,Material=Enum.Material.SmoothPlastic},parent)
    end
    local root=part("HumanoidRootPart",Vector3.new(2,2,1),Color3.fromRGB(51,68,97),dummy);root.Transparency=1
    dummy.PrimaryPart=root
    local torso=part("Jersey",Vector3.new(2.1,2.2,1.1),Color3.fromRGB(50,105,206),dummy)
    local head=part("Head",Vector3.new(1.1,1.1,1.1),Color3.fromRGB(190,134,92),dummy);head.Shape=Enum.PartType.Ball
    local left=part("LeftArm",Vector3.new(.55,2,.55),head.Color,dummy)
    local right=part("RightArm",Vector3.new(.55,2,.55),head.Color,dummy)
    local legL=part("LeftLeg",Vector3.new(.85,2,.85),Color3.fromRGB(27,34,48),dummy)
    local legR=part("RightLeg",Vector3.new(.85,2,.85),Color3.fromRGB(27,34,48),dummy)
    local balls=create("Folder",{Name="SimulatedBalls"},folder)
    local ball=part("LOCAL_TEST_BALL",Vector3.new(1.3,1.3,1.3),Color3.fromRGB(213,115,46),balls);ball.Shape=Enum.PartType.Ball
    local initial=CFrame.lookAt(r.Position+forward*7,r.Position)
    root.CFrame=initial;torso.CFrame=initial;head.CFrame=initial*CFrame.new(0,1.8,0)
    left.CFrame=initial*CFrame.new(-1.35,0,0);right.CFrame=initial*CFrame.new(1.35,0,0)
    legL.CFrame=initial*CFrame.new(-.5,-2,0);legR.CFrame=initial*CFrame.new(.5,-2,0)
    ball.Position=r.Position+forward*7+Vector3.new(0,-1,0)
    local billboard=create("BillboardGui",{Size=UDim2.fromOffset(280,45),StudsOffset=Vector3.new(0,3.7,0),AlwaysOnTop=true},root)
    local tag=create("TextLabel",{Size=UDim2.fromScale(1,1),BackgroundTransparency=1,Text="LOCAL SIMULATION",TextSize=14,
        Font=Enum.Font.GothamBold,TextColor3=Color3.new(1,1,1)},billboard)
    local previous={}
    for _,key in ipairs({"guard","steal","block","rebound","guardHold","reboundChase"}) do previous[key]=settings[key] end
    soloTest={active=true,folder=folder,dummy=dummy,balls=balls,ball=ball,root=root,
        attributes={Team="LOCAL_TEST_TEAM",Goal=r.Position-forward*18,Basketball=ball,Action="Dribbling",CanMove=true},
        origin=r.Position,forward=forward,right=Vector3.new(-forward.Z,0,forward.X),
        parts={torso,head,left,right,legL,legR},tag=tag,
        elapsed=0,lastTick=os.clock(),lastSample=0,lastSave=0,stage="",stageStart=0,frames={},previousSettings=previous,
        initialCounts={steal=actionCounts.steal,block=actionCounts.block,rebound=actionCounts.rebound}}
    for _,key in ipairs({"guard","steal","block","rebound","guardHold","reboundChase"}) do settings[key]=true end
    syncSoloToggles();soloButton.Text="Stop ALL-ASSIST solo test";soloInfo.Visible=true
    soloTitle.Text="LOCAL REHEARSAL · HOLD G"
    soloDetail.Text="Guard → steal → block → rebound. Native input attempts; no server opponent or confirmed results."
end
on(soloButton.Activated,startSoloTest)
on(Run.Heartbeat,function()
    local test=soloTest
    if not test or not test.active then return end
    local now=os.clock();local dt=math.clamp(now-test.lastTick,0,.1);test.lastTick=now
    local own=model();local ownRoot=rootOf(own)
    if not ownRoot or holdsBall(own) then stopSoloTest(false);return end
    if not bindingDown(guardKey) or Input:GetFocusedTextBox() then
        soloTitle.Text="LOCAL REHEARSAL · PAUSED"
        soloDetail.Text="Hold G to continue. This dummy cannot validate server steals or blocks."
        return
    end
    test.elapsed=test.elapsed+dt
    local t=test.elapsed
    local stage=t<12 and "GUARD" or t<24 and "STEAL" or t<42 and "BLOCK" or t<60 and "REBOUND" or "DONE"
    if stage=="DONE" then stopSoloTest(true);return end
    if stage~=test.stage then
        test.stage=stage;test.stageStart=t;test.origin=ownRoot.Position
        observedShooter=nil;observedShotSince=nil;blockedShotTarget=nil;blockedShotSince=nil
        opportunitySince=nil;opportunityTarget=nil;lastJump=-math.huge;lastSteal=-math.huge
    end
    local u=t-test.stageStart;local cycle=u%3
    local position=test.origin+test.forward*7
    local holding=true;local action="Dribbling"
    if stage=="GUARD" then
        position=position+test.right*math.sin(u*1.7)*4-test.forward*(1-math.cos(u*.9))*2
    elseif stage=="STEAL" then
        position=test.origin+test.forward*2.5+test.right*math.sin(u)*.25
        -- Reset nearby for repeated opportunities, rather than chase the defender.
        if cycle<dt then test.origin=ownRoot.Position end
    elseif stage=="BLOCK" then
        position=test.origin+test.forward*3.2
        action=cycle<1.1 and "Shooting" or ""
        holding=cycle<.8
        if cycle<dt then test.origin=ownRoot.Position end
    elseif stage=="REBOUND" then
        position=test.origin+test.forward*5;holding=false;action=""
    end
    local cf=CFrame.lookAt(position,Vector3.new(ownRoot.Position.X,position.Y,ownRoot.Position.Z))
    test.root.CFrame=cf
    local body=test.parts
    body[1].CFrame=cf
    body[2].CFrame=cf*CFrame.new(0,1.8,0)
    local armsUp=stage=="BLOCK" and cycle<1.1
    body[3].CFrame=cf*CFrame.new(-1.35,armsUp and 1.1 or 0,0)*CFrame.Angles(0,0,armsUp and -.5 or .1)
    body[4].CFrame=cf*CFrame.new(1.35,armsUp and 1.1 or 0,0)*CFrame.Angles(0,0,armsUp and .5 or -.1)
    body[5].CFrame=cf*CFrame.new(-.5,-2,0);body[6].CFrame=cf*CFrame.new(.5,-2,0)
    local ballPosition=position+test.right*.6-test.forward*.5+Vector3.new(0,-.8-math.abs(math.sin(u*8))*.35,0)
    if stage=="BLOCK" and not holding then
        local flight=cycle-.8
        ballPosition=position-test.forward*(flight*3)+Vector3.new(0,3+8*flight-7*flight*flight,0)
    elseif stage=="REBOUND" then
        local fall=(u%2)
        ballPosition=test.origin+test.forward*2+test.right*math.sin(u*.5)+Vector3.new(0,9-4*fall-1.5*fall*fall,0)
    end
    test.ball.Position=ballPosition
    test.attributes.Goal=test.origin-test.forward*18
    test.attributes.Basketball=holding and test.ball or nil;test.attributes.Action=action
    test.tag.Text="LOCAL SIM · "..stage
    local counts=actionCounts
    soloTitle.Text=string.format("LOCAL REHEARSAL · %s · %.0f / 60s",stage,t)
    soloDetail.Text=string.format("Inputs sent: steal %d · block %d · rebound %d\n%s · simulated opponent; results unverified",
        counts.steal-test.initialCounts.steal,counts.block-test.initialCounts.block,counts.rebound-test.initialCounts.rebound,assistState)
    if now-test.lastSample>=.1 then
        test.lastSample=now
        table.insert(test.frames,{time=t,stage=stage,ownPosition=vlist(ownRoot.Position),targetPosition=vlist(position),
            ballPosition=vlist(ballPosition),guardReason=guardReason,assistState=assistState,observedAction=attr(own,"Action"),
            inputs={steal=counts.steal-test.initialCounts.steal,block=counts.block-test.initialCounts.block,rebound=counts.rebound-test.initialCounts.rebound}})
    end
    if now-test.lastSave>1 then test.lastSave=now;saveSoloTest(test,false) end
end)

-- Public integration surface for the replacement UI. No internal controller references required.
local numericLimits={manualResume={0,1},landingBeamWidth={.02,.2},landingZoneScale={.5,2},landingRange={8,120}}
local api
-- Keep profile/UI implementation locals in their own function. Only the API and
-- numeric limits are shared with later features; the main chunk has a 200-register budget.
do (function()
local currentProfile="Match"
for _,r in ipairs(settingRows) do numericLimits[r.key]={r.min,r.max} end
local savedProfiles={Match={},Practice={}}
local profileKeys={"guardGap","guardReaction","guardStrength","guardPrediction","closeGap","closeReaction",
    "stealDelay","stealCooldown","blockDelay","jumpCooldown","reboundLead","manualResume",
    "assignmentLock","fakeFilter","smartPosition","landing","guardHold","guardSprint"}
for _,name in ipairs({"Match","Practice"}) do
    for _,key in ipairs(profileKeys) do savedProfiles[name][key]=settings[key] end
end
savedProfiles.Practice.guardReaction=.12;savedProfiles.Practice.manualResume=.15
if readfile then
    local ok,value=pcall(function() return Http:JSONDecode(readfile("PracticalUserProfiles-v1.json")) end)
    if ok and type(value)=="table" then
        for name,profile in pairs(savedProfiles) do
            local saved=value[name]
            if type(saved)=="table" then for key,default in pairs(profile) do
                local v=saved[key]
                if type(default)=="boolean" and type(v)=="boolean" then profile[key]=v
                elseif finite(v) and numericLimits[key] then profile[key]=math.clamp(v,numericLimits[key][1],numericLimits[key][2]) end
            end end
        end
    end
end
local function exportMatchReport()
    local windowAttempts={live={},simulation={}}
    for _,event in ipairs(reportEvents) do
        if event.kind=="attempt_sent" then
            local group=event.localSimulation and windowAttempts.simulation or windowAttempts.live
            local name=event.data.action;group[name]=(group[name] or 0)+1
        end
    end
    local report={version=1,attempts=actionCounts,events=reportEvents,shots=records,
        attemptsIncludeSimulation=true,recentEventWindowAttempts=windowAttempts,eventLimit=500,
        profile=currentProfile,serverStealsBlocksReboundsConfirmed=false,
        note="Possession changes and animations are observations; they do not prove a steal or block."}
    env.PracticalMatchReport=report
    if writefile then
        local ok=pcall(function() writefile("PracticalMatchReport.json",Http:JSONEncode(report)) end)
        if not ok then return false,"Report write failed; available in getgenv().PracticalMatchReport" end
    end
    return true,"Report saved / available in memory"
end
local function switchAssignment()
    local own=model();local r=rootOf(own);if not r then return false end
    local candidates={}
    for _,character in ipairs(defenseCharacters()) do
        local root=rootOf(character)
        local same=attr(own,"Team")~=nil and attr(own,"Team")~=0 and attr(own,"Team")==attr(character,"Team")
        if character~=own and not same and root and flat(root.Position-r.Position).Magnitude<=settings.guardRange then table.insert(candidates,character) end
    end
    table.sort(candidates,function(a,b) return a.Name<b.Name end)
    local nextIndex=1
    for i,candidate in ipairs(candidates) do if candidate==lockedAssignment then nextIndex=i%#candidates+1;break end end
    lockedAssignment=candidates[nextIndex];target=nil;targetSince=os.clock()
    recordDefense("assignment_changed",{name=lockedAssignment and lockedAssignment.Name or "none"})
    return lockedAssignment~=nil
end
local function setCourtBounds(a,b)
    if typeof(a)~="Vector3" or typeof(b)~="Vector3" then return false end
    if math.abs(a.X-b.X)<6 or math.abs(a.Z-b.Z)<6 then return false end
    courtBounds={minX=math.min(a.X,b.X),maxX=math.max(a.X,b.X),minZ=math.min(a.Z,b.Z),maxZ=math.max(a.Z,b.Z)}
    return true
end
local function applyProfile(name)
    if not savedProfiles[name] then return false end
    for key,value in pairs(savedProfiles[name]) do settings[key]=value end
    currentProfile=name;lockedAssignment=nil
    for _,r in ipairs(settingRows) do r.refresh() end
    syncSoloToggles()
    return true
end
local function saveProfile(name)
    if not savedProfiles[name] then return false end
    for _,key in ipairs(profileKeys) do savedProfiles[name][key]=settings[key] end
    if writefile then pcall(function() writefile("PracticalUserProfiles-v1.json",Http:JSONEncode(savedProfiles)) end) end
    return true
end
local customUICleanup
api={Version="3.2",
    RegisterUICleanup=function(fn)local previous=customUICleanup;customUICleanup=function()if previous then pcall(previous)end;fn()end end,
    SetGuardKey=function(key)
        if not bindingValid(key) then return false end
        guardKey=key;return true
    end,
    SetAutoTime=function(value)
        if value and not keyboard then return false,"Waiting for game controller" end
        if enabled~=value then
            enabled=value==true;disconnectShot(active);active=nil
            toggle.Text=enabled and "Auto Time · ON" or "Auto Time · OFF"
        end
        return true
    end,
    GetAutoTime=function() return enabled end,
    Check=function() return keyboard~=nil and hookedMovement~=nil,
        keyboard and (hookedMovement and "Controllers connected" or "Movement controller pending") or "Game controller pending" end,
    Unload=cleanup,ApplyProfile=applyProfile,SaveProfile=saveProfile,SwitchAssignment=switchAssignment,
    SetPanelVisible=function(visible) panel.Visible=visible==true end,
    SetCourtBounds=setCourtBounds,ClearCourtBounds=function() courtBounds=nil end,ExportReport=exportMatchReport,
    GetState=function() return {profile=currentProfile,assignment=lockedAssignment and lockedAssignment.Name,
        guard=guardReason,action=assistState,prediction=landingPrediction,counts=actionCounts} end,
    SetOption=function(key,value)
        if type(settings[key])~="boolean" or type(value)~="boolean" then return false end
        settings[key]=value;if key=="assignmentLock" then lockedAssignment=nil end
        if key=="denyReceiver"and not value and settings._denyReceiverReset then settings._denyReceiverReset()end
        syncSoloToggles();return true
    end,
    GetOption=function(key) return settings[key] end,
    SetNumber=function(key,value)
        local limits=numericLimits[key]
        if not limits or not finite(value) then return false end
        settings[key]=math.clamp(value,limits[1],limits[2])
        for _,r in ipairs(settingRows) do if r.key==key then r.refresh() end end
        return true
    end,
    StartSolo=startSoloTest,StopSolo=function() stopSoloTest(false) end}
env.PracticalSuiteAPI=api
nav.Stats.Text="Tools"
pages.Stats.CanvasSize=UDim2.fromOffset(0,810)
local profileButton=row(pages.Stats,"Profile · Match (click to switch)",310)
local saveButton=row(pages.Stats,"Save current profile",357)
local assignmentButton=row(pages.Stats,"Assignment lock · OFF  /  B switches opponent",404)
local predictionButton=row(pages.Stats,"Landing marker + uncertainty ring · ON",451)
local fakeButton=row(pages.Stats,"Shot-fake filter · ON (estimated)",498)
local boundsButton=row(pages.Stats,"Court edges · mark first corner",545)
local reportButton=row(pages.Stats,"Export match report",592)
label(pages.Stats,"Court setup: stand at opposite corners and click twice. Right-click clears. Bounds use world X/Z.\nManual movement pauses assists, then resumes after a short delay. Landing and shot-fake detection are estimates.",645,125,11)
on(profileButton.Activated,function()
    applyProfile(currentProfile=="Match" and "Practice" or "Match")
    profileButton.Text="Profile · "..currentProfile.." (click to switch)"
    assignmentButton.Text="Assignment lock · "..(settings.assignmentLock and "ON" or "OFF").." / B switches opponent"
    predictionButton.Text="Landing marker + uncertainty ring · "..(settings.landing and "ON" or "OFF")
    fakeButton.Text="Shot-fake filter · "..(settings.fakeFilter and "ON" or "OFF").." (estimated)"
end)
on(saveButton.Activated,function() saveProfile(currentProfile);status.Text="Saved "..currentProfile.." profile." end)
on(assignmentButton.Activated,function()
    settings.assignmentLock=not settings.assignmentLock;lockedAssignment=nil
    assignmentButton.Text="Assignment lock · "..(settings.assignmentLock and "ON" or "OFF").." / B switches opponent"
end)
featureToggle(predictionButton,"landing","Landing marker + uncertainty ring")
featureToggle(fakeButton,"fakeFilter","Shot-fake filter (estimated)")
local firstCorner
on(boundsButton.Activated,function()
    local r=rootOf(model());if not r then return end
    if not firstCorner then firstCorner=r.Position;boundsButton.Text="Court edges · mark opposite corner"
    elseif setCourtBounds(firstCorner,r.Position) then firstCorner=nil;boundsButton.Text="Court edges · SET (click to redo)"
    else status.Text="Corners must span at least 6 studs along both axes." end
end)
on(boundsButton.MouseButton2Click,function() firstCorner=nil;courtBounds=nil;boundsButton.Text="Court edges · mark first corner" end)
on(reportButton.Activated,function() local _,message=exportMatchReport();status.Text=message end)
on(Input.InputBegan,function(input,processed)
    if not processed and not Input:GetFocusedTextBox() and input.KeyCode==Enum.KeyCode.B and settings.assignmentLock then switchAssignment() end
end)
local previousPossession=attr(model(),"Basketball")
on(Run.Heartbeat,function()
    local held=attr(model(),"Basketball")
    if held~=previousPossession then
        recordDefense("possession_changed",{hasBall=holdsBall(model()),afterRecentSteal=os.clock()-lastSteal<1.5})
        previousPossession=held
    end
end)
local cleanupBeforeTools=featureCleanup
featureCleanup=function()
    if customUICleanup then pcall(customUICleanup);customUICleanup=nil end
    if customUICleanup then pcall(customUICleanup);customUICleanup=nil end
    if customUICleanup then pcall(customUICleanup);customUICleanup=nil end
    if customUICleanup then pcall(customUICleanup);customUICleanup=nil end
    if customUICleanup then pcall(customUICleanup);customUICleanup=nil end
    if customUICleanup then pcall(customUICleanup);customUICleanup=nil end
    if customUICleanup then pcall(customUICleanup);customUICleanup=nil end
    if customUICleanup then pcall(customUICleanup);customUICleanup=nil end
    if customUICleanup then pcall(customUICleanup);customUICleanup=nil end
    if customUICleanup then pcall(customUICleanup);customUICleanup=nil end
    if customUICleanup then pcall(customUICleanup);customUICleanup=nil end
    if customUICleanup then pcall(customUICleanup);customUICleanup=nil end
    if customUICleanup then pcall(customUICleanup);customUICleanup=nil end
    if customUICleanup then pcall(customUICleanup);customUICleanup=nil end
    if customUICleanup then pcall(customUICleanup);customUICleanup=nil end
    if customUICleanup then pcall(customUICleanup);customUICleanup=nil end
    if customUICleanup then pcall(customUICleanup);customUICleanup=nil end
    if customUICleanup then pcall(customUICleanup);customUICleanup=nil end
    if customUICleanup then pcall(customUICleanup);customUICleanup=nil end
    if customUICleanup then pcall(customUICleanup);customUICleanup=nil end
    if customUICleanup then pcall(customUICleanup);customUICleanup=nil end
    exportMatchReport()
    if env.PracticalSuiteAPI==api then env.PracticalSuiteAPI=nil end
    cleanupBeforeTools()
end
end)()end

-- Shared HUD dock: screen cards register here and are stacked in ONE corner so they never overlap.
-- Frames keep their own visibility logic; the dock only decides where they sit.
do (function()
 -- Card style shared with the menu UI (same palette, font, radius, outline and accent line).
 local S={lines={},accent=Color3.fromRGB(72,134,255),accent2=Color3.fromRGB(134,98,255),c={
  bg=Color3.fromRGB(10,12,18),surface=Color3.fromRGB(17,20,29),raised=Color3.fromRGB(23,27,38),
  line=Color3.fromRGB(31,37,51),stroke=Color3.fromRGB(40,47,64),strokeHi=Color3.fromRGB(68,80,106),
  text=Color3.fromRGB(240,242,248),text2=Color3.fromRGB(226,230,240),sub=Color3.fromRGB(178,186,206),
  muted=Color3.fromRGB(160,170,192),green=Color3.fromRGB(104,228,176),amber=Color3.fromRGB(255,200,102),
  danger=Color3.fromRGB(255,125,138)}}
 settings._hudStyle=S
 local FAMILY="rbxasset://fonts/families/BuilderSans.json"
 function S.font(weight)return Font.new(FAMILY,weight or Enum.FontWeight.Regular)end
 local weightFor={[Enum.Font.GothamBold]=Enum.FontWeight.Bold,[Enum.Font.GothamMedium]=Enum.FontWeight.SemiBold}
 function S.fontify(root)
  for _,object in ipairs(root:GetDescendants())do
   if object:IsA("TextLabel")or object:IsA("TextButton")then
    pcall(function()object.FontFace=S.font(weightFor[object.Font]or Enum.FontWeight.Medium)end)
   end
  end
 end
 local EDGE=NumberSequence.new({NumberSequenceKeypoint.new(0,1),NumberSequenceKeypoint.new(.5,0),NumberSequenceKeypoint.new(1,1)})
 function S.skin(frame,radius,hairline)
  frame.BackgroundColor3=S.c.surface;frame.BackgroundTransparency=.05;frame.BorderSizePixel=0
  local corner=frame:FindFirstChildOfClass("UICorner")or Instance.new("UICorner")
  corner.CornerRadius=UDim.new(0,radius or 14);corner.Parent=frame
  local stroke=frame:FindFirstChildOfClass("UIStroke")or Instance.new("UIStroke")
  stroke.Color=S.c.stroke;stroke.Thickness=1;stroke.Transparency=0
  stroke.ApplyStrokeMode=Enum.ApplyStrokeMode.Border;stroke.Parent=frame
  if hairline then
   local line=Instance.new("Frame");line.Name="AccentLine";line.BackgroundColor3=Color3.new(1,1,1)
   line.BorderSizePixel=0;line.Position=UDim2.fromOffset(16,0);line.Size=UDim2.new(1,-32,0,1)
   local gradient=Instance.new("UIGradient");gradient.Color=ColorSequence.new(S.accent,S.accent2)
   gradient.Transparency=EDGE;gradient.Parent=line;line.Parent=frame
   S.lines[#S.lines+1]=gradient
  end
  S.fontify(frame)
  return stroke
 end
 -- The menu calls this whenever you change its accent theme, so cards stay matched.
 function S.setAccent(a,b)
  S.accent,S.accent2=a,b
  for i=#S.lines,1,-1 do
   local gradient=S.lines[i]
   if gradient.Parent then gradient.Color=ColorSequence.new(a,b)else table.remove(S.lines,i)end
  end
 end
 api.SetUIAccent=S.setAccent
 settings.hudCorner="Bottom right"
 local corners={["Bottom right"]=true,["Bottom left"]=true,["Top right"]=true,["Top left"]=true}
 local entries={}
 settings._hudRegister=function(frame,order)
  entries[#entries+1]={frame=frame,order=order,y=nil}
  table.sort(entries,function(a,b)return a.order<b.order end)
 end
 api.SetHudCorner=function(name)
  if not corners[name] then return false,"Unknown HUD corner" end
  settings.hudCorner=name
  for _,entry in ipairs(entries)do entry.y=nil end
  return true
 end
 api.GetHudCorner=function()return settings.hudCorner end
 settings._hudRegister(soloInfo,30)
 local connection=(Run.RenderStepped or Run.Heartbeat):Connect(function(dt)
  local name=settings.hudCorner
  local right=name:find("right",1,true)~=nil
  local bottom=name:find("Bottom",1,true)~=nil
  local margin,base,gap=20,bottom and 130 or 80,10
  local used=0
  local blend=1-math.exp(-math.clamp(dt or 1/60,0,.1)*14)
  for _,entry in ipairs(entries)do
   local frame=entry.frame
   local shown=frame.Parent~=nil and frame.Visible
   if shown then
    local gui=frame:FindFirstAncestorWhichIsA("ScreenGui")
    if gui and not gui.Enabled then shown=false end
   end
   if shown and not entry.skinned then entry.skinned=true;S.skin(frame,14,true)end
   if shown then
    -- Slide only when the stack reflows; a card that just appeared snaps into its slot.
    entry.y=entry.y and entry.y+(used-entry.y)*blend or used
    frame.AnchorPoint=Vector2.new(right and 1 or 0,bottom and 1 or 0)
    local offsetY=bottom and -(base+entry.y) or (base+entry.y)
    frame.Position=UDim2.new(right and 1 or 0,right and -margin or margin,bottom and 1 or 0,offsetY)
    used=used+frame.Size.Y.Offset+gap
   else entry.y=nil end
  end
 end)
 local previous=featureCleanup
 featureCleanup=function()
  connection:Disconnect();settings._hudRegister=nil;settings._hudStyle=nil
  api.SetHudCorner=nil;api.GetHudCorner=nil;api.SetUIAccent=nil
  previous()
 end
end)() end
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

-- Native steal requests; experimental touch and resizing paths removed.
settings._stealAttempt=function(service)service.Steal:Fire()end

-- Additional court tools. Runs in an IIFE to preserve the engine's local limit.
do (function()
 settings.ballRecovery=false;settings.recoveryRange=12;settings.recoveryWindow=3
 settings.passingLanes=false;settings.laneRange=60;settings.laneClearance=3
 settings.screenAwareness=false;settings.screenRange=7
 settings.comboBuilder=false;settings.comboInterval=.25;settings.ballTrail=false
 for key,limit in pairs({recoveryRange={4,24},recoveryWindow={1,5},laneRange={10,100},laneClearance={1,6},screenRange={3,12},comboInterval={.08,1}}) do numericLimits[key]=limit end
 local state={recovery="idle",combo="idle",lanes={},screen=nil,opportunity=nil}
 local combo={"Z","X","C","V"}
 local comboKey=Enum.KeyCode.J
 local sequenceText="Z@0.20 > X@0.25 > C@0.20 > V@0.30"
 settings.comboRepeats=1;settings.comboSprint=false;numericLimits.comboRepeats={1,4}
 local token=0
 local alive=true
 local rootFolder=Instance.new("Folder");rootFolder.Name="PracticalCourtTools";rootFolder.Parent=workspace
 local laneParts={}
 local trails={}
 local recent={}
 local recentUntil=0
 local function teammate(a,b)
  local pa=game:GetService("Players"):FindFirstChild(a.Name)
  local pb=game:GetService("Players"):FindFirstChild(b.Name)
  if pa and pb and pa.Team~=nil and pb.Team~=nil then return pa.Team==pb.Team end
  local x,y=attr(a,"Team"),attr(b,"Team")
  return x~=nil and y~=nil and x~=0 and x~="" and x==y
 end
 local function ballPart(value)
  if typeof(value)~="Instance" then return nil end
  if value:IsA("BasePart") then return value end
  if value:IsA("Model") then return value.PrimaryPart or value:FindFirstChildWhichIsA("BasePart",true) end
 end
 local function liveCharacters()
  local list={}
  for _,m in ipairs(defenseCharacters()) do
   local r=rootOf(m)
   if m.Name~="InvisCharacter" and r and r.Position.Y>-100 then list[#list+1]=m end
  end
  return list
 end
 local function noteSteal()
  if not settings.ballRecovery then return end
  local me=model();local r=rootOf(me);if not r then return end
  recent={};recentUntil=os.clock()+settings.recoveryWindow
  for _,m in ipairs(liveCharacters()) do
   local ball=ballPart(attr(m,"Basketball"))
   if m~=me and ball and (ball.Position-r.Position).Magnitude<=settings.recoveryRange then recent[ball]=true end
  end
 end
 local service=keyboard and keyboard.Services and keyboard.Services.InputService
 local stealSignal=service and service.Steal
 local oldFire=stealSignal and stealSignal.Fire
 local wrappedFire
 if oldFire then
  wrappedFire=function(self,...) noteSteal();return oldFire(self,...) end
  stealSignal.Fire=wrappedFire
 end
 local oldAttempt=settings._stealAttempt
 settings._stealAttempt=function(...) noteSteal();return oldAttempt(...) end
 settings._recoveryDirection=function()
  state.recovery="idle"
  if not settings.ballRecovery or os.clock()>recentUntil or Input:GetFocusedTextBox() or manualTakeover(os.clock()) then return nil end
  local me=model();local r=rootOf(me)
  if not r or holdsBall(me) or attr(me,"CanMove")==false or attr(me,"Action")=="Stealing" or attr(me,"Action")=="Shooting" or attr(me,"Action")=="Dunking" then return nil end
  local occupied={}
  for _,m in ipairs(liveCharacters()) do local b=ballPart(attr(m,"Basketball"));if b then occupied[b]=true end end
  local best,distance
  for ball in pairs(recent) do
   if ball.Parent and not occupied[ball] then
    local d=flat(ball.Position-r.Position).Magnitude
    if d<=settings.recoveryRange and math.abs(ball.Position.Y-r.Position.Y)<6 and (not distance or d<distance) then best=ball;distance=d end
   end
  end
  if not best then return nil end
  local delta=flat(best.Position-r.Position)
  state.recovery="pursuing loose ball"
  guardError=delta.Magnitude
  settings.recoveryDriving=false
  return nil -- Recovery is a marker only; never pull the player toward a loose ball.
 end
 local board=Instance.new("ScreenGui");board.Name="CourtAwareness";board.ResetOnSpawn=false;board.Parent=P.PlayerGui
 local card=Instance.new("Frame");card.Name="CourtStatus";card.Size=UDim2.fromOffset(240,48);card.Position=UDim2.new(1,-256,1,-145);card.BackgroundColor3=Color3.fromRGB(18,22,30);card.BackgroundTransparency=.2;card.Parent=board
 if settings._hudRegister then settings._hudRegister(card,20)end
 local label=Instance.new("TextLabel");label.BackgroundTransparency=1;label.Size=UDim2.new(1,-16,1,0);label.Position=UDim2.fromOffset(8,0);label.TextColor3=Color3.fromRGB(226,230,240);label.TextSize=13;label.Font=Enum.Font.GothamMedium;label.TextWrapped=true;label.Parent=card
 local corner=Instance.new("UICorner");corner.CornerRadius=UDim.new(0,6);corner.Parent=card
 local screenHighlight=Instance.new("Highlight");screenHighlight.FillTransparency=.97;screenHighlight.OutlineTransparency=.15;screenHighlight.OutlineColor=Color3.fromRGB(255,185,65);screenHighlight.FillColor=screenHighlight.OutlineColor;screenHighlight.Parent=rootFolder
 local function cancelCombo()token=token+1;state.combo="idle"end
 api.CancelCombo=cancelCombo
 api.SetComboSlot=function(slot,value)
  if slot<1 or slot>4 or slot%1~=0 or type(value)~="string" or not (value=="None" or (#value<=3 and value:match("^[ZXCV]+$"))) then return false end
  combo[slot]=value;cancelCombo();return true
 end
 api.SetComboKey=function(key)
  if not bindingValid(key) then return false end
  comboKey=key;settings.comboPlayKey=key;return true
 end
 local function parseSequence(text)
  if type(text)~="string" or #text>300 then return nil,"Sequence is too long" end
  local result={}
  for rawPiece in (text..">"):gmatch("(.-)>") do
   local piece=rawPiece:match("^%s*(.-)%s*$"):upper()
   local keys,delay=piece:match("^([A-Z]+)%s*@%s*([%d%.]+)$")
   if not keys then keys=piece;delay=nil end
   if delay then delay=tonumber(delay) else delay=settings.comboInterval end
   if not delay or delay<.08 or delay>1.5 then return nil,"Step delay must be 0.08–1.50 seconds" end
   if not (keys=="WAIT" or (#keys>=1 and #keys<=4 and keys:match("^[ZXCV]+$"))) then return nil,"Use Z/X/C/V combinations or WAIT, separated by >" end
   result[#result+1]={keys=keys,delay=delay}
   if #result>12 then return nil,"Maximum 12 steps" end
  end
  if #result==0 then return nil,"Add a combo step" end
  return result
 end
 api.SetComboSequence=function(text)
  if text=="" then cancelCombo();sequenceText="";return true end
  local steps,err=parseSequence(text)
  if not steps then return false,err end
  cancelCombo();sequenceText=text;return true
 end
 api.GetComboSequence=function()return sequenceText end
 api.PlayCombo=function(preset)
  if not settings.comboBuilder then return false,"Enable combo builder first" end
  if not service or not service.Dribble then return false,"Dribble controller unavailable" end
  local me=model()
  if not me or not holdsBall(me) then return false,"You need possession" end
  if Input:GetFocusedTextBox() then return false,"Finish typing first" end
  local sequence,err=parseSequence(preset and preset.sequence or sequenceText)
  if not sequence then return false,err end
  cancelCombo();local mine=token;local repeats=settings.comboRepeats;local sprint=settings.comboSprint
  if preset then repeats=preset.repeats;sprint=preset.sprint end
  task.spawn(function()
   for loop=1,repeats do for index,step in ipairs(sequence) do
    if not alive or mine~=token or not settings.comboBuilder or model()~=me or not holdsBall(me) or Input:GetFocusedTextBox() then if mine==token then state.combo="idle" end;return end
    local action=attr(me,"Action")
    if action=="Shooting" or action=="Dunking" then state.combo="idle";return end
    state.combo=string.format("%d/%d · %s",index,#sequence,step.keys)
    if step.keys~="WAIT" then
     local movement=keyboard.Controllers.Player.Character.Movement
     local ok,failure=pcall(function() service.Dribble:Fire(step.keys,sprint or Input:IsKeyDown(Enum.KeyCode.LeftShift) or Input:IsKeyDown(Enum.KeyCode.RightShift),movement.lastMoveDirection or Vector3.zero) end)
     if not ok then state.combo=tostring(failure);return end
    end
    task.wait(step.delay)
   end end
   if mine==token then state.combo="idle" end
  end)
  return true,"Combo playing"
 end
 api.GetCourtToolsState=function()return state end
 on(Input.InputBegan,function(input,processed)
  if processed or Input:GetFocusedTextBox() then return end
  if bindingMatches(input,comboKey) then api.PlayCombo()
  elseif input.KeyCode==Enum.KeyCode.Z or input.KeyCode==Enum.KeyCode.X or input.KeyCode==Enum.KeyCode.C or input.KeyCode==Enum.KeyCode.V then cancelCombo() end
 end)
 local last=0
 on(Run.Heartbeat,function()
  if not alive or os.clock()-last<.1 then return end;last=os.clock()
  local me=model();local r=rootOf(me)
  board.Enabled=false;local visibleScreen
  for _,part in ipairs(laneParts) do part.Transparency=1 end
  state.lanes={};state.screen=nil;state.opportunity=nil
  if not r then screenHighlight.Enabled=false;return end
  local characters=liveCharacters()
  local opponents={}
  for _,m in ipairs(characters) do if m~=me and not teammate(me,m) then opponents[#opponents+1]=m end end
  if settings.passingLanes and holdsBall(me) then
   local index=0
   for _,mate in ipairs(characters) do
    local mr=rootOf(mate)
    if mate~=me and teammate(me,mate) and (mr.Position-r.Position).Magnitude<=settings.laneRange and index<10 then
     index=index+1
     if not laneParts[index] then
      local part=Instance.new("Part");part.Name="PassingLane";part.Anchored=true;part.CanCollide=false;part.CanTouch=false;part.CanQuery=false;part.Material=Enum.Material.Neon;part.Parent=rootFolder;laneParts[index]=part
     end
     local a,b=r.Position+Vector3.new(0,.5,0),mr.Position+Vector3.new(0,.5,0)
     local delta=b-a;local blocked=false
     for _,enemy in ipairs(opponents) do
      local ep=rootOf(enemy).Position
      local t=math.clamp((ep-a):Dot(delta)/math.max(delta:Dot(delta),.001),0,1)
      if t>.05 and t<.95 and (ep-(a+delta*t)).Magnitude<settings.laneClearance then blocked=true;break end
     end
     local params=RaycastParams.new();params.FilterType=Enum.RaycastFilterType.Exclude;params.FilterDescendantsInstances={me,mate,rootFolder,ballPart(attr(me,"Basketball"))}
     if workspace:Raycast(a,delta,params) then blocked=true end
     local part=laneParts[index];ribbon(part,a,b,.055);part.Transparency=settings._visualV2 and 1 or .3;part.Color=blocked and Color3.fromRGB(255,105,85) or Color3.fromRGB(65,205,160)
     state.lanes[#state.lanes+1]={player=mate.Name,blocked=blocked}
    end
   end
  end
  local message={}
  if settings.screenAwareness then
   local nearest,dist
   for _,enemy in ipairs(opponents) do
    local er=rootOf(enemy);local d=flat(er.Position-r.Position).Magnitude
    local screening=attr(enemy,"Screening")==true or attr(enemy,"Action")=="Screening" or attr(enemy,"Action")=="Screen"
    if screening and d<=settings.screenRange and (not dist or d<dist) then nearest=enemy;dist=d end
   end
   if nearest then
    local localPos=r.CFrame:PointToObjectSpace(rootOf(nearest).Position)
    state.screen={player=nearest.Name,distance=dist,side=localPos.X<0 and "LEFT" or "RIGHT"}
    message[#message+1]="SCREEN "..state.screen.side.." · "..string.format("%.1f studs",dist)
    visibleScreen=nearest
   end
  end
  screenHighlight.Adornee=visibleScreen
  local showScreen=visibleScreen~=nil and (not settings._visualV2 or settings.visualShowScreen)
  if screenHighlight.Enabled~=showScreen then screenHighlight.Enabled=showScreen end
  label.Text=table.concat(message,"\n");board.Enabled=#message>0 and not settings._visualV2
  local occupied={}
  for _,m in ipairs(characters) do local ball=ballPart(attr(m,"Basketball"));if ball then occupied[ball]=true end end
  local folder=workspace:FindFirstChild("Basketballs")
  local valid={}
  if settings.ballTrail and folder then for _,ball in ipairs(folder:GetDescendants()) do
   if ball:IsA("BasePart") and not occupied[ball] and (ball.Position-r.Position).Magnitude<=settings.landingRange then
    valid[ball]=true
    if not trails[ball] then
     local a,b=Instance.new("Attachment"),Instance.new("Attachment");a.Position=Vector3.new(-.12,0,0);b.Position=Vector3.new(.12,0,0);a.Parent=ball;b.Parent=ball
     local trail=Instance.new("Trail");trail.Attachment0=a;trail.Attachment1=b;trail.Lifetime=.35;trail.MinLength=.08;trail.Color=ColorSequence.new(Color3.fromRGB(70,160,255));trail.Transparency=NumberSequence.new(.3,1);trail.Parent=ball;trails[ball]={a,b,trail}
    end
   end
  end end
  for ball,objects in pairs(trails) do if not valid[ball] then for _,obj in ipairs(objects) do obj:Destroy() end;trails[ball]=nil end end
 end)
 local previousCleanup=featureCleanup
 featureCleanup=function()
  alive=false;cancelCombo();settings._recoveryDirection=nil
  if stealSignal and stealSignal.Fire==wrappedFire then stealSignal.Fire=oldFire end
  for _,objects in pairs(trails) do for _,obj in ipairs(objects) do obj:Destroy() end end
  board:Destroy();rootFolder:Destroy();previousCleanup()
 end
end)() end

-- Stabilized flight tracking and opponent coverage.
do (function()
 settings.guardCoverage=true
 local samples=setmetatable({},{__mode="k"})
 local selected
 settings._flightBall=function(folder,occupied,root,now)
  local best,score
  if not folder then settings._flightVelocity=nil;return nil end
  for _,ball in ipairs(folder:GetChildren()) do
   local position=goalPosition(ball)
   if position and not occupied[ball] and flat(position-root.Position).Magnitude<=math.max(settings.landingRange,settings.reboundChaseRange) then
    local sample=samples[ball]
    if not sample then sample={position=position,time=now};samples[ball]=sample
    elseif (position-sample.position).Magnitude>.015 and now-sample.time>.005 then
     local dt=now-sample.time
     local velocity=(position-sample.position)/dt
     if velocity.Magnitude<180 and dt<.4 then
      local alpha=math.clamp(1-math.exp(-14*dt),.15,.8)
      sample.velocity=sample.velocity and sample.velocity:Lerp(velocity,alpha) or velocity
      sample.moved=now
     else sample.velocity=nil;sample.moved=nil end
     sample.position=position;sample.time=now
    end
    if sample.velocity and sample.velocity.Magnitude>1 and now-(sample.moved or 0)<.3 then
     local rank=flat(position-root.Position).Magnitude-(ball==selected and 15 or 0)
     if not score or rank<score then best=ball;score=rank end
    end
   end
  end
  selected=best;settings._flightSelected=best;settings._flightVelocity=best and samples[best].velocity or nil
  return best
 end
 local outline=Instance.new("Highlight");outline.Name="GuardCoverage";outline.FillTransparency=.98;outline.OutlineTransparency=.12;outline.Parent=visualFolder
 on(Run.Heartbeat,function()
  local me=model();local ownRoot=rootOf(me);local guarded=settings.denyReceiver and settings._denyReceiverTarget or target;local otherRoot=rootOf(guarded)
  outline.Enabled=(not settings._visualV2 or settings.visualShowCoverage)and settings.guardCoverage and (settings.guard or settings.denyReceiver)and ownRoot~=nil and otherRoot~=nil and (otherRoot.Position-ownRoot.Position).Magnitude<settings.guardRange
  if outline.Enabled then
   outline.Adornee=guarded
   local delta=flat(ownRoot.Position-otherRoot.Position)
   local passer=rootOf(settings._denyReceiverBallHandler)
   local goal=settings.denyReceiver and settings._denyReceiverBallHandler~=guarded and passer and passer.Position or goalPosition(attr(guarded,"Goal"))
   local side=goal and flat(goal-otherRoot.Position)
   local alignment=side and side.Magnitude>.01 and delta.Magnitude>.01 and delta.Unit:Dot(side.Unit) or 0
   local error=math.abs(delta.Magnitude-settings.guardGap)
   local good=alignment>.6 and error<1.5 and delta.Magnitude<8
   local moderate=alignment>.1 and delta.Magnitude<10
   outline.OutlineColor=good and Color3.fromRGB(65,210,150) or moderate and Color3.fromRGB(255,190,65) or Color3.fromRGB(245,95,85)
   outline.FillColor=outline.OutlineColor
  end
 end)
 local previousCleanup=featureCleanup
 featureCleanup=function()
  settings._flightBall=nil
  outline:Destroy();previousCleanup()
 end
end)() end

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
  local me=model();local r=rootOf(me)
  if not r then return nil end
  local ownCharacter=keyboard and keyboard.Controllers.Player.Character
  local proxy=ownCharacter and ownCharacter.ProxyCharacter
  local ownVelocity=proxy and proxy.AssemblyLinearVelocity or Vector3.zero
  local goal=goalPosition(attr(me,"Goal"))
  local current,future=0,0
  for _,other in ipairs(defenseCharacters())do
   local dr=rootOf(other)
   local a,b=attr(me,"Team"),attr(other,"Team")
   local same=a~=nil and b~=nil and a~=0 and a==b
   local player=game:GetService("Players"):FindFirstChild(other.Name)
   if player and P.Team~=nil and player.Team==P.Team then same=true end
   if other~=me and other.Name~="InvisCharacter" and dr and dr.Position.Y>-100 and not same then
    local previous=samples[other]
    local velocity=Vector3.zero
    if previous and now-previous.time>.005 and now-previous.time<.5 then
     velocity=(dr.Position-previous.position)/(now-previous.time)
     if velocity.Magnitude>60 then velocity=Vector3.zero end
    end
    samples[other]={position=dr.Position,time=now}
    local function pressure(position,origin)
     local delta=flat(position-origin);local distance=delta.Magnitude;local facing=.5
     if goal and distance>.01 and flat(goal-origin).Magnitude>.01 then facing=math.clamp((flat(goal-origin).Unit:Dot(delta.Unit)+1)/2,0,1)end
     local value=math.clamp(1-distance/10,0,1)*(.35+.65*facing)*100
     if math.abs(position.Y-origin.Y)>5 then value=value*.5 end
     return value
    end
    current=math.max(current,pressure(dr.Position,r.Position))
    future=math.max(future,pressure(dr.Position+velocity*settings.contestForecast,r.Position+ownVelocity*settings.contestForecast))
   end
  end
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


-- Drive cutoff, screen routing, and predicted pass interception.
do (function()
 settings.driveCutoff=false;settings.screenNavigation=false;settings.passIntercept=false
 settings.passInterceptAuto=false;settings.passInterceptMove=true;settings.passInterceptJump=false;settings.passInterceptShots=false
 settings.passInterceptReach=4;numericLimits.passInterceptReach={2,6}
 local state={intercept="idle",screen=nil,cutoff=false}
 api.GetDefenseNavigationState=function()return state end
 local activeIntent
 local requestUntil=0
 local currentFlight
 local lastNative=-math.huge
 local releaseJumpAt
 local screenSide,screenHeldUntil,screenObstacle
 local service
 local originalSteal
 local stealWrapper
 local function ballHeld(ball)
  for _,m in ipairs(defenseCharacters())do if attr(m,"Basketball")==ball then return true end end
  return false
 end
 local function flightCandidate(now)
  local own=model();local r=rootOf(own)
  if not r or holdsBall(own) or not settings._flightSelected or not settings._flightVelocity then return nil end
  local ball=settings._flightSelected
  local kind=tostring(attr(ball,"LastShotType")or ""):lower()
  if settings.antiOop and (kind:find("lob",1,true) or attr(ball,"Lob")==true or attr(ball,"SelfLob")==true or settings._oopOwnsBall and settings._oopOwnsBall(ball))then return nil end
  if not ball.Parent or ballHeld(ball) or (not settings.passInterceptShots and ball:GetAttribute("MidShot")==true)then return nil end
  local p=goalPosition(ball);local v=settings._flightVelocity
  if not p or v.Magnitude<3 or flat(p-r.Position).Magnitude>18 then return nil end
  local best,bestTime,bestDistance
  -- Short linear extrapolation is conservative for passes; no server trajectory is claimed.
  for i=0,15 do
   local t=i*.04;local point=p+v*t
   local horizontal=flat(point-r.Position).Magnitude
   local height=point.Y-r.Position.Y
   local maxHeight=settings.passInterceptJump and 8 or 4
   if height>-.5 and height<maxHeight then
    local required=math.max(0,horizontal-settings.passInterceptReach)
    local reachable=required<=14*t+.75
    if reachable and (not bestDistance or horizontal<bestDistance)then best=point;bestTime=t;bestDistance=horizontal end
   end
  end
  if not best then return nil end
  return {ball=ball,point=best,time=bestTime,distance=bestDistance,position=p,velocity=v,at=now}
 end
 local function arm(now,denial)
  local candidate=flightCandidate(now)
  if not candidate then state.intercept="No reachable moving loose ball";return false end
  activeIntent={ball=candidate.ball,untilTime=now+1.1,denial=denial==true};state.intercept="Tracking intercept";return true
 end
 settings._cancelReceiverIntercept=function()
  if activeIntent and activeIntent.denial then activeIntent=nil;settings.interceptDriving=false end
 end
 local function bindService()
  local nextService=keyboard and keyboard.Services and keyboard.Services.InputService
  if not nextService or not nextService.Steal or type(nextService.Steal.Fire)~="function"then return false end
  if service==nextService and service.Steal.Fire==stealWrapper then return true end
  if service and service.Steal.Fire==stealWrapper then service.Steal.Fire=originalSteal end
  service=nextService;originalSteal=service.Steal.Fire
  local native=originalSteal
  settings._requestLooseCatch=function()
   if settings.steal or Input:IsKeyDown(Enum.KeyCode.R)then return native(nextService.Steal)end
  end
  stealWrapper=function(self,...)
   if settings.passIntercept and Input:IsKeyDown(Enum.KeyCode.R)and not Input:GetFocusedTextBox()then
    requestUntil=os.clock()+1.2;arm(os.clock());return
   end
   lastNative=os.clock();return native(self,...)
  end
  service.Steal.Fire=stealWrapper;return true
 end
 bindService()
 on(Input.InputBegan,function(input,processed)
  if not processed and input.KeyCode==Enum.KeyCode.R and settings.passIntercept and not Input:GetFocusedTextBox()then
   requestUntil=os.clock()+1.2;arm(os.clock())
  end
 end)
 settings._interceptDirection=function()
  settings.interceptDriving=false
  if activeIntent and not settings.steal and not Input:IsKeyDown(Enum.KeyCode.R)and os.clock()>=requestUntil then activeIntent=nil;settings.interceptDriving=false;return nil end
  if activeIntent and activeIntent.denial and (not settings.steal or not settings.denyReceiver or not settings.denyCatch or settings._guardReleased or not bindingDown(guardKey))then activeIntent=nil;return nil end
  local denial=activeIntent and activeIntent.denial
  if not(settings.passIntercept or denial)or not(denial or settings.passInterceptMove)or not activeIntent or not currentFlight or currentFlight.ball~=activeIntent.ball or os.clock()-currentFlight.at>.15 or manualTakeover(os.clock()) or Input:GetFocusedTextBox()then return nil end
  local own=model();local r=rootOf(own)
  if not r or holdsBall(own)or attr(own,"CanMove")==false then return nil end
  local action=attr(own,"Action");if action=="Shooting"or action=="Dunking"or action=="Stealing"then return nil end
  local delta=flat(currentFlight.point-r.Position)
  guardError=delta.Magnitude;settings.interceptDriving=true
  return delta.Magnitude>1 and delta.Unit or Vector3.zero
 end
 settings._defenseRoute=function(own,other,desired)
  state.cutoff=false;state.screen=nil
  local r,dr=rootOf(own),rootOf(other)
  if not r or not dr then return desired end
  if settings.driveCutoff and holdsBall(other) and not guardClosing then
   local goal=goalPosition(attr(other,"Goal"));local direction=goal and flat(goal-dr.Position)
   local velocity=flat(targetVelocity or Vector3.zero)
   if direction and direction.Magnitude>.1 and velocity:Dot(direction.Unit)>1 then
    local prediction=dr.Position+velocity*.18
    local toGoal=flat(goal-prediction)
    if toGoal.Magnitude>.1 then
     local point=prediction+toGoal.Unit*settings.guardGap
     local adjustment=flat(point-desired)
     if adjustment.Magnitude>2.5 then adjustment=adjustment.Unit*2.5 end
     desired=clampCourt(desired+adjustment);state.cutoff=true
    end
   end
  end
  if not settings.screenNavigation then return desired end
  local path=flat(desired-r.Position)
  if path.Magnitude<1 then return desired end
  local meTeam=attr(own,"Team")
  for _,m in ipairs(defenseCharacters())do
   local sr=rootOf(m);local team=attr(m,"Team")
   local same=meTeam~=nil and meTeam~=0 and team==meTeam
   if m~=own and m~=other and sr and not same and (attr(m,"Screening")==true or attr(m,"Action")=="Screening" or attr(m,"Action")=="Screen" or flat(sr.Position-r.Position).Magnitude<7)then
    local t=math.clamp(flat(sr.Position-r.Position):Dot(path)/math.max(path:Dot(path),.01),0,1)
    local separation=flat(sr.Position-(r.Position+path*t)).Magnitude
    if t>=0 and t<=1 and separation<3.8 and flat(sr.Position-r.Position).Magnitude<13 then
     local perpendicular=Vector3.new(-path.Unit.Z,0,path.Unit.X)
     local left=sr.Position+perpendicular*4.6+path.Unit*1.8
     local right=sr.Position-perpendicular*4.6+path.Unit*1.8
     local lcost=flat(left-r.Position).Magnitude+flat(desired-left).Magnitude
     local rcost=flat(right-r.Position).Magnitude+flat(desired-right).Magnitude
     local params=RaycastParams.new();params.FilterType=Enum.RaycastFilterType.Exclude
     local characters=workspace:FindFirstChild("Characters")
     local excluded={visualFolder};if characters then table.insert(excluded,characters)end
     local proxy=workspace:FindFirstChild("ProxyCharacter");if proxy then table.insert(excluded,proxy)end
     params.FilterDescendantsInstances=excluded
     local leftBlocked=workspace:Raycast(r.Position,left-r.Position,params)~=nil
     local rightBlocked=workspace:Raycast(r.Position,right-r.Position,params)~=nil
     if leftBlocked and rightBlocked then return desired end
     if leftBlocked then screenSide=-1;screenHeldUntil=os.clock()+.65
     elseif rightBlocked then screenSide=1;screenHeldUntil=os.clock()+.65
     elseif screenObstacle~=m or not screenSide or os.clock()>(screenHeldUntil or 0)then screenSide=lcost<=rcost and 1 or -1;screenHeldUntil=os.clock()+.65 end
     screenObstacle=m
     state.screen=m.Name
     return clampCourt(screenSide==1 and left or right)
    end
   end
  end
  return desired
 end
 local last=0
 on(Run.Heartbeat,function()
  local now=os.clock()
  bindService()
  if releaseJumpAt and now>=releaseJumpAt then releaseJumpAt=nil;pcall(function()service.Shoot:Fire({Shoot=false,UseSpace=true})end)end
  if now-last<.03 then return end;last=now
  local denialHeld=settings.steal and settings.denyReceiver and settings.denyCatch and not settings._guardReleased and bindingDown(guardKey)
  local denialWanted=denialHeld and settings._denyReceiverCatchAt and now-settings._denyReceiverCatchAt<.2
  if activeIntent and activeIntent.denial and not denialHeld then activeIntent=nil;settings.interceptDriving=false end
  local interceptEnabled=settings.passIntercept or denialWanted or denialHeld and activeIntent and activeIntent.denial
  currentFlight=interceptEnabled and flightCandidate(now)or nil
  if not service or not originalSteal then state.intercept="Native input unavailable";return end
  if not interceptEnabled then activeIntent=nil;requestUntil=0;state.intercept="disabled";return end
  if Input:GetFocusedTextBox()then activeIntent=nil;state.intercept="Paused while typing";return end
  local manualRequested=Input:IsKeyDown(Enum.KeyCode.R)or now<requestUntil
  local requested=settings.passInterceptAuto and settings.steal or manualRequested or denialWanted
  if activeIntent and not settings.steal and not manualRequested then activeIntent=nil;settings.interceptDriving=false;state.intercept="Automatic steals off · manual R remains available"end
  if not activeIntent and requested and now-math.max(lastNative,lastSteal)>1.4 and currentFlight then arm(now,denialWanted and not settings.passInterceptAuto and not Input:IsKeyDown(Enum.KeyCode.R))end
  if not activeIntent then state.intercept=requested and "Waiting for reachable loose flight"or "Hold R or enable automatic attempts"end
  if not activeIntent then return end
  if settings._oopOwnsBall and settings._oopOwnsBall(activeIntent.ball)then activeIntent=nil;state.intercept="Yielding lob to anti alley-oop";return end
  local own=model();local r=rootOf(own)
  if not r or holdsBall(own)or now>activeIntent.untilTime or ballHeld(activeIntent.ball)then activeIntent=nil;state.intercept="finished";return end
  local action=attr(own,"Action")
  if action=="Shooting"or action=="Dunking"or action=="Blocking"or action=="Rebounding"or action=="Stealing"then return end
  if not currentFlight or currentFlight.ball~=activeIntent.ball then return end
  if Input:GetFocusedTextBox()or attr(own,"CanMove")==false or now-math.max(lastNative,lastSteal)<1.4 then return end
  local delta=currentFlight.position-r.Position
  local lead=math.clamp(.1+(ping or 0),.08,.22)
  local projected=currentFlight.position+(currentFlight.velocity or Vector3.zero)*lead-r.Position
  local near=flat(delta).Magnitude<=settings.passInterceptReach or (flat(projected).Magnitude<=settings.passInterceptReach and projected.Y>-.5 and projected.Y<4)
  if near and delta.Y>-.5 and delta.Y<4 then
   lastNative=now;lastSteal=now;originalSteal(service.Steal);recordDefense("attempt_sent",{action="Pass intercept"});state.intercept="Native intercept requested · possession unconfirmed";activeIntent=nil;requestUntil=0
  elseif delta.Y>=4 then
   state.intercept="High pass: waiting for reachable catch height"
  end
 end)
 local previousCleanup=featureCleanup
 featureCleanup=function()
  settings._interceptDirection=nil;settings._defenseRoute=nil;settings._requestLooseCatch=nil;settings._cancelReceiverIntercept=nil
  if service and service.Steal.Fire==stealWrapper then service.Steal.Fire=originalSteal end
  if releaseJumpAt then pcall(function()service.Shoot:Fire({Shoot=false,UseSpace=true})end)end
  activeIntent=nil;previousCleanup()
 end
end)() end


-- Selected native join spot; no leave requests or movement overrides.
if api then (function()
 settings.autoJoin=false
 local selected,lastAttempt,stopped=nil,-math.huge,false
 local state={status="Choose a spot",attempts=0}
 local function spots()
  local ok,value=pcall(function()return require(P.PlayerScripts.Aero.Modules.UI.Park.Spots)end)
  if ok and type(value)=="table" then return value end
 end
 local function nearest(module)
  local ok,value=pcall(function()return module:GetNearestSpotPad()end)
  if ok and type(value)=="table" and value.Court and value.SpotType and value.SpotIndex then return value end
 end
 local function same(a,b)
  return a and b and a.Court==b.Court and a.SpotType==b.SpotType and a.SpotIndex==b.SpotIndex
 end
 api.LockJoinSpot=function()
  local module=spots();local pad=module and nearest(module)
  if not pad then return false,"Stand next to the desired join spot first" end
  selected={Court=pad.Court,SpotType=pad.SpotType,SpotIndex=pad.SpotIndex}
  state.status="Locked "..tostring(pad.Court).." / "..tostring(pad.SpotType).." / "..tostring(pad.SpotIndex)
  return true,state.status
 end
 api.GetAutoJoinState=function()return {status=state.status,attempts=state.attempts,selected=selected and table.clone(selected)}end
 local connection=Run.Heartbeat:Connect(function()
  if stopped or not settings.autoJoin then return end
  local own=model()
  if not own then state.status="Waiting for character";return end
  if attr(own,"OnSpot") or attr(own,"OnCourt") or attr(own,"InGame") then state.status="Joined / in game";return end
  if not selected then state.status="Lock a spot first";return end
  if Input:GetFocusedTextBox()then state.status="Paused while typing";return end
  local now=os.clock();if now-lastAttempt<.25 then return end
  lastAttempt=now
  local module=spots()
  if not module or not same(selected,nearest(module))then state.status="Paused: return to selected spot";return end
  local ok=pcall(function()module.Services.SpotService.JoinSpot:Fire(table.clone(selected))end)
  if ok then state.attempts=state.attempts+1;state.status="Join requested" else state.status="Join service unavailable" end
 end)
 local previous=featureCleanup
 featureCleanup=function()stopped=true;settings.autoJoin=false;connection:Disconnect();previous()end
end)() end

-- Auto guard no longer owns rim blocks or jumps.
if api then
 settings.dunkStopper=false
 api.SetDunkKey=function()return false,"Auto guard rim blocking was removed"end
end

-- Nearest-opponent receiver denial: normal movement and existing loose-pass catch input.
do (function()
 settings.denyReceiver=false;settings.denyCatch=false
 settings.denyRange=18;settings.denyGap=2.5;settings.denySwitchMargin=3
 settings.denyDelay=.2;settings.denySmoothing=8;settings.denyDeadzone=1.2
 settings.denySoft=true;settings.denyLead=.03
 numericLimits.denyLead={0,.15}
 numericLimits.denyDelay={0,.6};numericLimits.denySmoothing={2,30};numericLimits.denyDeadzone={.3,3}
 numericLimits.denyRange={6,35};numericLimits.denyGap={1,5};numericLimits.denySwitchMargin={0,8}
 local marked,carrier,carrierAt,markAt
 local selected,picking,selectedName
 local pickDeadline
 local function restoreMouse()pickDeadline=nil end
 local previousPosition,previousAt,velocity
 local observations={}
 local state={status="Disabled",mode="Off ball"}
 local function clear(reason)
  if settings._cancelReceiverIntercept then settings._cancelReceiverIntercept()end
  marked=nil;carrier=nil;carrierAt=nil;markAt=nil
  previousPosition=nil;previousAt=nil;velocity=Vector3.zero
  observations={}
  settings._denyReceiverTarget=nil;settings._denyReceiverBallHandler=nil
  settings._denyReceiverFacePosition=nil;settings._denyReceiverFaceAt=nil
  settings._denyReceiverCatchAt=nil
  state={status=reason or "Disabled",mode="Off ball"}
 end
 clear()
 local function sameCourt(a,b)
  local n,m=attr(a,"CourtNumber"),attr(b,"CourtNumber")
  if n~=nil and m~=nil and n~=0 and m~=0 and n~=m then return false end
  local x,y=attr(a,"Court"),attr(b,"Court")
  return not(x~=nil and x~=0 and x~=""and y~=nil and y~=0 and y~=""and x~=y)
 end
 local function enemy(own,other)
  if not other or other==own or not rootOf(other)or not sameCourt(own,other)then return false end
  local a,b=attr(own,"Team"),attr(other,"Team")
  if a~=nil and b~=nil and a~=0 and b~=0 and a~=""and b~=""then return a~=b end
  local players=game:GetService("Players")
  local ownPlayer=players:GetPlayerFromCharacter(own)or P
  local otherPlayer=players:GetPlayerFromCharacter(other)or players:FindFirstChild(other.Name)
  return ownPlayer and otherPlayer and ownPlayer.Team~=nil and otherPlayer.Team~=nil and ownPlayer.Team~=otherPlayer.Team or false
 end
 local function choose(own,root,now)
  local characters=defenseCharacters();local occupied={};local present={}
  local activeCarrier,carrierDistance
  for _,other in ipairs(characters)do
   present[other]=true
   local held=attr(other,"Basketball");if held then occupied[held]=true end
   if enemy(own,other)and holdsBall(other)then
    local distance=flat(rootOf(other).Position-root.Position).Magnitude
    if distance<=80 and (not carrierDistance or distance<carrierDistance or other==carrier)then
     activeCarrier=other;carrierDistance=distance
    end
   end
  end
  if activeCarrier then carrier=activeCarrier;carrierAt=now
  elseif not carrier or not present[carrier]or not enemy(own,carrier)or now-(carrierAt or 0)>.85 then carrier=nil end
  if selected and(not present[selected]or not enemy(own,selected))then selected=nil;clear("Matchup unavailable; following ball handler")end
  if selected and holdsBall(selected)then carrier=selected;carrierAt=now end
  if not carrier or not selected then return nil,occupied end
  -- An explicit game-long matchup must not switch to the ball handler merely
  -- because the selected opponent moved outside the old proximity radius.
  return selected,occupied
 end
 api.SetReceiverMatchup=function(character)
  local own=model();local present=false
  for _,candidate in ipairs(defenseCharacters())do if candidate==character then present=true;break end end
  if not own or not present or not enemy(own,character)then return false,"Select an opposing player on your court"end
  clear();selected=character;selectedName=character.Name;picking=false;restoreMouse()
  settings.assignmentLock=false;lockedAssignment=nil
  return true,"Matchup selected: "..character.Name.." · same player on and off ball"
 end
 api.BeginReceiverSelection=function()
  pickDeadline=nil
  picking=true;return true,"Click an opposing player in the game to choose your matchup"
 end
 api.CancelReceiverSelection=function()picking=false;restoreMouse();return true end
 api.GetReceiverSelectionHover=function()
  if not picking then return nil end
  local hit=P:GetMouse().Target;local own=model()
  if not hit or not own then return nil end
  for _,character in ipairs(defenseCharacters())do
   if enemy(own,character)then
    local proxy=attr(character,"ProxyCharacter")
    if hit==character or hit:IsDescendantOf(character)or typeof(proxy)=="Instance"and(hit==proxy or hit:IsDescendantOf(proxy))then
     return character,typeof(proxy)=="Instance"and proxy or character
    end
   end
  end
 end
 api.CycleReceiverMatchup=function()
  local own=model();local candidates={}
  if not own then return false,"Character unavailable"end
  for _,character in ipairs(defenseCharacters())do if enemy(own,character)then candidates[#candidates+1]=character end end
  table.sort(candidates,function(a,b)return a.Name<b.Name end)
  if #candidates==0 then return false,"No opposing players detected on this court"end
  local index=1
  for i,character in ipairs(candidates)do if character==selected then index=i%#candidates+1;break end end
  return api.SetReceiverMatchup(candidates[index])
 end
 api.ClearReceiverMatchup=function()
  selected=nil;selectedName=nil;picking=false;restoreMouse();clear("Following ball handler")
  settings.assignmentLock=false;lockedAssignment=nil;target=nil
  return true,"Matchup cleared; Auto guard follows the ball handler"
 end
 local function requestCatch(own,root,receiver,occupied,now)
  settings._denyReceiverCatchAt=nil
  if not settings.denyCatch or not settings.steal then return false end
  local ball=settings._flightSelected;local motion=settings._flightVelocity
  if not ball or not ball.Parent or not motion or motion.Magnitude<3 or occupied[ball]then return false end
  local kind=tostring(attr(ball,"LastShotType")or ""):lower()
  if attr(ball,"MidShot")==true or kind:find("lob",1,true)or attr(ball,"Lob")==true or attr(ball,"SelfLob")==true then return false end
  local position=goalPosition(ball);if not position then return false end
  local toReceiver=flat(receiver-position);local horizontal=flat(motion)
  if horizontal.Magnitude<3 or horizontal:Dot(toReceiver)<=0 then return false end
  local receiverTime=math.clamp(toReceiver:Dot(horizontal)/math.max(horizontal:Dot(horizontal),.01),0,.7)
  if flat(position+motion*receiverTime-receiver).Magnitude>6 then return false end
  local toOwn=flat(root.Position-position)
  local catchTime=math.clamp(toOwn:Dot(horizontal)/math.max(horizontal:Dot(horizontal),.01),0,.45)
  local point=position+motion*catchTime
  if flat(point-root.Position).Magnitude>6 or point.Y-root.Position.Y<-.5 or point.Y-root.Position.Y>4 then return false end
  settings._denyReceiverCatchAt=now
  return true
 end
 settings._denyReceiverReset=clear
 settings._denyReceiverDirection=function(dt)
  local now=os.clock()
  if not settings.denyReceiver then clear();return nil end
  if settings._guardReleased or not bindingDown(guardKey)or Input:GetFocusedTextBox()then clear("Hold your guard key");return nil end
  local own=model();local root=rootOf(own)
  if not own or not root then clear("Waiting for your character");return nil end
  if holdsBall(own)then clear("You have possession");return nil end
  if not selected then clear("Select a matchup, or let Auto guard follow the ball");return nil end
  local action=attr(own,"Action")
  if attr(own,"CanMove")==false or action=="Shooting"or action=="Dunking"or action=="Stealing"then
   settings._denyReceiverCatchAt=nil;state.status="Game movement locked";return nil
  end
  local chosen,occupied=choose(own,root,now)
  if chosen~=marked then
   marked=chosen;markAt=now;previousPosition=nil;previousAt=nil;velocity=Vector3.zero;steering=Vector3.zero;observations={}
  end
  local receiverRoot=rootOf(marked);local carrierRoot=rootOf(carrier)
  settings._denyReceiverTarget=marked;settings._denyReceiverBallHandler=carrier
  settings._denyReceiverCatchAt=nil
  if not receiverRoot or not carrierRoot then
   guardAim=nil;guardClosing=false;guardError=0
   settings._denyReceiverFacePosition=nil;settings._denyReceiverFaceAt=nil
   state={status=carrierRoot and "No off-ball opponent in range"or "No enemy ball handler detected",mode="Off ball"}
   guardReason=state.status
   return nil
  end
  if marked==carrier and holdsBall(marked)then
   if settings._cancelReceiverIntercept then settings._cancelReceiverIntercept()end
   settings._denyReceiverTarget=nil;settings._denyReceiverFacePosition=nil;settings._denyReceiverFaceAt=nil
   if target~=marked then target=marked;targetSince=now-(settings.guardReaction or 0)end
   state={status=settings.guard and "Selected matchup has ball; Auto guard takes over"or "Selected matchup has ball; enable Auto guard",mode="On ball",target=marked.Name,handler=carrier.Name}
   return nil
  end
  if previousPosition and now-(previousAt or now)>=.02 and now-previousAt<.3 then
   local observed=flat(receiverRoot.Position-previousPosition)/(now-previousAt)
   if observed.Magnitude<40 then velocity=velocity:Lerp(observed,.4)else velocity=Vector3.zero end
  end
  if not previousAt or now-previousAt>=.02 then previousPosition=receiverRoot.Position;previousAt=now end
  local lead=settings.denySoft and(settings.denyLead or .03)or math.clamp(settings.guardPrediction or .08,.04,.2)+math.clamp(ping or 0,0,.12)*.5
  local predicted=receiverRoot.Position+velocity*lead
  local onBall=marked==carrier and holdsBall(marked)
  local source=not onBall and goalPosition(attr(carrier,"Basketball"))or nil
  source=source or (onBall and goalPosition(attr(marked,"Goal")))or carrierRoot.Position
  local axis=flat(source-predicted)
  if axis.Magnitude<.25 then
   axis=flat(root.Position-predicted)
   if axis.Magnitude<.25 then return Vector3.zero end
  end
  local gap=onBall and (settings.guardGap or 3)or math.min(settings.denyGap,axis.Magnitude*.45)
  local desired=clampCourt(predicted+axis.Unit*gap)
  -- Follow timestamped observations rather than reacting to the latest position.
  -- Keep one sample before the cutoff; never restart a timer on every frame.
  observations[#observations+1]={at=now,point=desired,face=carrierRoot.Position}
  local cutoff=now-(settings.denySoft and(settings.denyDelay or .2)or 0)
  while #observations>1 and observations[2].at<=cutoff do table.remove(observations,1)end
  while #observations>90 do table.remove(observations,1)end
  local sample=observations[1]
  if sample.at>cutoff then
   settings._denyReceiverFacePosition=nil;settings._denyReceiverFaceAt=nil
   settings._denyReceiverCatchAt=nil;steering=Vector3.zero
   state={status="Waiting for response delay",mode="Off ball",target=marked.Name,handler=carrier.Name}
   return Vector3.zero
  end
  desired=sample.point
  guardAim=desired;guardClosing=false
  local delta=flat(desired-root.Position);guardError=delta.Magnitude
  local catching=not onBall and requestCatch(own,root,receiverRoot.Position,occupied,now)
  settings._denyReceiverFacePosition=sample.face;settings._denyReceiverFaceAt=now
  state={status=catching and "Pass in your lane"or onBall and "Receiver caught: covering ball"or "Denying receiver",mode=onBall and "On ball"or "Off ball",
   target=marked.Name,handler=carrier.Name,distance=flat(receiverRoot.Position-root.Position).Magnitude,aim=desired,catchReady=catching}
  guardReason=state.status.." · "..marked.Name
  local tolerance=settings.denySoft and(settings.denyDeadzone or 1.2)or(settings.guardDeadzone or .45)
  if delta.Magnitude<=math.max(.3,tolerance)then steering=Vector3.zero;return steering end
  local response=settings.denySoft and(settings.denySmoothing or 8)or(8+(settings.guardStrength or 1)*24)
  local blend=1-math.exp(-math.clamp(dt or 1/60,0,.1)*response)
  steering=steering:Lerp(delta.Unit,blend)
  return steering.Magnitude>.001 and steering.Unit or Vector3.zero
 end
 api.GetReceiverDenialState=function()
  local result=table.clone(state);result.selected=selected and selected.Name;result.picking=picking==true
  return result
 end
 -- Match membership must be explicit: waiting on a join spot is not a game.
 local matchKey,matchCandidate,candidateAt,missingAt,readySince
 local matchPrompted=false
 local matchRevision=0
 api.GetMatchupPickerState=function()
  local own=model();local opponents={}
  if own then for _,character in ipairs(defenseCharacters())do
   if enemy(own,character)then opponents[#opponents+1]={character=character,name=character.Name}end
  end end
  table.sort(opponents,function(a,b)return a.name<b.name end)
  return {revision=matchRevision,active=matchKey~=nil,opponents=opponents,selected=selected and selected.Name}
 end
 local function updateMatch(now)
  local own=model()
  local function value(name)
   local v=own and attr(own,name);if v==nil then v=attr(P,name)end;return v
  end
  local inGame,onCourt=value("InGame"),value("OnCourt")
  local active=inGame==true or (inGame==nil and onCourt==true)
  local court=value("CourtNumber")or value("Court")
  local key=active and tostring(court or "current")or nil
  if key then
   missingAt=nil
   if key~=matchCandidate then matchCandidate=key;candidateAt=now;readySince=nil end
   if key~=matchKey and now-(candidateAt or now)>=1.5 then
    matchKey=key;matchPrompted=false
    selected=nil;selectedName=nil;picking=false;restoreMouse();clear("New game: choose your matchup")
    settings.assignmentLock=false;lockedAssignment=nil;target=nil
   end
   -- InGame/OnCourt become true during introductions. Membership alone must
   -- not arm world selection: wait for movement control and real opponents.
   local action=tostring(value("Action")or ""):lower()
   local ready=own and rootOf(own)and value("CanMove")==true
    and value("Intro")~=true and value("InIntro")~=true and value("Cutscene")~=true
    and (action==""or action=="idle"or action=="running"or action=="guarding"or action=="dribbling")
   local opponentReady=false
   if ready then for _,character in ipairs(defenseCharacters())do
    if enemy(own,character)then opponentReady=true;break end
   end end
   if ready and opponentReady then readySince=readySince or now else readySince=nil end
   if matchKey==key and not matchPrompted and readySince and now-readySince>=1.25 then
    matchPrompted=true;matchRevision=matchRevision+1
   end
  else
   matchCandidate=nil;candidateAt=nil;readySince=nil
   missingAt=missingAt or now
   if matchKey and now-missingAt>=3 then
    matchKey=nil;matchPrompted=false;selected=nil;selectedName=nil;picking=false;restoreMouse();clear("Game ended")
    settings.assignmentLock=false;lockedAssignment=nil;target=nil
   end
  end
 end
 local selectInput=Input.InputBegan:Connect(function(input,processed)
  if picking and input.KeyCode and input.KeyCode==Enum.KeyCode.Escape then picking=false;restoreMouse();return end
  if picking and input.UserInputType and input.UserInputType==Enum.UserInputType.MouseButton2 then picking=false;restoreMouse();return end
  if not picking or processed or Input:GetFocusedTextBox()or input.UserInputType~=Enum.UserInputType.MouseButton1 then return end
  local hit=P:GetMouse().Target
  if not hit then return end
  for _,character in ipairs(defenseCharacters())do
   local proxy=attr(character,"ProxyCharacter")
   local proxyHit=typeof(proxy)=="Instance"and(hit==proxy or hit:IsDescendantOf(proxy))
   if hit==character or hit:IsDescendantOf(character)or proxyHit then
    local ok,message=api.SetReceiverMatchup(character);state.status=message
    if ok then return end
   end
  end
 end)
 local connection=Run.Heartbeat:Connect(function()
  updateMatch(os.clock())
  if not selected and selectedName then
   local own=model()
   for _,candidate in ipairs(defenseCharacters())do
    if candidate.Name==selectedName and enemy(own,candidate)then selected=candidate;break end
   end
  end
  if picking and pickDeadline and os.clock()>=pickDeadline then picking=false;restoreMouse()end
  if selected then
   local own=model();local present=false
   for _,character in ipairs(defenseCharacters())do if character==selected then present=true;break end end
   if not own or not present or not enemy(own,selected)then selected=nil;picking=false;restoreMouse();clear("Matchup unavailable; following ball handler")end
  end
  if not settings.denyReceiver and marked then clear()end
 end)
 local selectionBlur=Input.WindowFocusReleased and Input.WindowFocusReleased.Connect and Input.WindowFocusReleased:Connect(function()
  picking=false;restoreMouse()
 end)
 local previous=featureCleanup
 featureCleanup=function()
  connection:Disconnect();selectInput:Disconnect();if selectionBlur then selectionBlur:Disconnect()end;selected=nil;picking=false;restoreMouse();clear();settings._denyReceiverDirection=nil;settings._denyReceiverReset=nil;previous()
 end
end)()end

-- Catch-and-shoot spacing estimates; no automatic pass or shot inputs.
if api then (function()
 settings.passVision=false;settings.passSpeed=80;settings.catchRelease=.5
 numericLimits.passSpeed={40,140};numericLimits.catchRelease={.2,1}
 local snapshots=setmetatable({},{__mode="k"})
 local results={}
 local preferred
 local function evaluate(origin,receiver,velocity,defenders,latency)
  local distance=flat(receiver-origin).Magnitude
  local travel=distance/settings.passSpeed+math.clamp(latency or 0,0,.3)
  local predicted=receiver+velocity*math.min(travel,.8)
  travel=flat(predicted-origin).Magnitude/settings.passSpeed+math.clamp(latency or 0,0,.3)
  local nearest,arrival,lane=math.huge,math.huge,false
  local nearestDefender
  local path=flat(predicted-origin)
  for _,d in ipairs(defenders)do
   local gap=flat(d.position-predicted).Magnitude
   if gap<nearest then nearest=gap;nearestDefender=d.position end
   local speed=math.clamp(math.max(16,d.speed or 0),16,28)
   arrival=math.min(arrival,math.max(0,gap-5)/speed+.12)
   local t=math.clamp(flat(d.position-origin):Dot(path)/math.max(path:Dot(path),.01),0,1)
   if t>.05 and t<.95 then
    local across=math.max(0,flat(d.position-(origin+path*t)).Magnitude-2.5)/speed
    if across<=travel*t then lane=true end
   end
  end
  local margin=arrival-travel-settings.catchRelease
  return {distance=distance,passTime=travel,nearest=nearest,nearestDefender=nearestDefender,defenderTime=arrival,margin=margin,laneBlocked=lane,reliable=#defenders>0,position=predicted}
 end
 api.EvaluatePassTarget=evaluate
 api.GetPassTargets=function()local r={}for i,v in ipairs(results)do r[i]=table.clone(v)end return r end
 local board=Instance.new("ScreenGui");board.Name="PracticalPassTargets";board.ResetOnSpawn=false;board.Enabled=false;board.Parent=P.PlayerGui
 local panel=create("Frame",{AnchorPoint=Vector2.new(1,1),Position=UDim2.new(1,-24,1,-155),Size=UDim2.fromOffset(264,94),BackgroundColor3=Color3.fromRGB(15,20,29),BackgroundTransparency=.12,BorderSizePixel=0},board)
 if settings._hudRegister then settings._hudRegister(panel,10)end
 create("UICorner",{CornerRadius=UDim.new(0,10)},panel)
 create("UIStroke",{Color=Color3.fromRGB(54,72,98),Thickness=1},panel)
 local heading=create("TextLabel",{BackgroundTransparency=1,Position=UDim2.fromOffset(14,7),Size=UDim2.fromOffset(236,18),Font=Enum.Font.GothamBold,TextSize=12,TextXAlignment=Enum.TextXAlignment.Left,TextColor3=Color3.fromRGB(160,170,192),Text="PASS TARGETS"},panel)
 local labels={}
 for i=1,3 do labels[i]=create("TextLabel",{BackgroundTransparency=1,Position=UDim2.fromOffset(14,30+(i-1)*20),Size=UDim2.fromOffset(236,18),TextTruncate=Enum.TextTruncate.AtEnd,Font=Enum.Font.GothamMedium,TextSize=13,TextXAlignment=Enum.TextXAlignment.Left,TextColor3=Color3.fromRGB(175,184,200),Text=""},panel)end
 local outline=create("Highlight",{Name="SuggestedPassTarget",Enabled=false,FillTransparency=.9,OutlineTransparency=.05,DepthMode=Enum.HighlightDepthMode.Occluded},workspace)
 local targetVisuals={}
 local function tint(v)
  return not v.reliable and Color3.fromRGB(178,186,206)or(v.laneBlocked or v.obstructed)and Color3.fromRGB(255,125,138)or v.open and Color3.fromRGB(104,228,176)or Color3.fromRGB(255,200,102)
 end
 for i=1,3 do
  local item={ring={},halo={}}
  item.closeout=visualPart("Receiver defender closeout "..i,Color3.fromRGB(242,178,87));item.closeout.Transparency=1
  item.highlight=create("Highlight",{Name="PassOption"..i,Enabled=false,FillTransparency=1,OutlineTransparency=.25,DepthMode=Enum.HighlightDepthMode.Occluded},workspace)
  item.badge=create("BillboardGui",{Name="PassTargetBadge"..i,Enabled=false,AlwaysOnTop=true,Size=UDim2.fromOffset(176,48),StudsOffsetWorldSpace=Vector3.new(0,4.2,0),MaxDistance=110},board)
  item.cardHalo={}
  for layer=1,1 do
   local spread=layer*3
   local glow=create("Frame",{Name="PassCardHalo",Position=UDim2.fromOffset(-spread,-spread),Size=UDim2.new(1,spread*2,1,spread*2),BackgroundTransparency=1},item.badge)
   create("UICorner",{CornerRadius=UDim.new(0,11+spread)},glow)
   item.cardHalo[layer]=create("UIStroke",{Color=Color3.fromRGB(86,215,164),Transparency=.7+layer*.07,Thickness=2},glow)
  end
  item.card=create("Frame",{Size=UDim2.fromScale(1,1),BackgroundColor3=Color3.fromRGB(12,19,27),BackgroundTransparency=.12,BorderSizePixel=0},item.badge)
  create("UICorner",{CornerRadius=UDim.new(0,11)},item.card)
  item.stroke=create("UIStroke",{Color=Color3.fromRGB(86,215,164),Transparency=.35,Thickness=1.25},item.card)
  item.label=create("TextLabel",{Position=UDim2.fromOffset(10,5),Size=UDim2.new(1,-20,0,20),BackgroundTransparency=1,BorderSizePixel=0,Font=Enum.Font.GothamBold,TextSize=14,TextXAlignment=Enum.TextXAlignment.Left,TextTruncate=Enum.TextTruncate.AtEnd,TextColor3=Color3.fromRGB(220,230,245)},item.card)
  item.detail=create("TextLabel",{Position=UDim2.fromOffset(10,25),Size=UDim2.new(1,-20,0,14),BackgroundTransparency=1,Font=Enum.Font.Gotham,TextSize=11,TextXAlignment=Enum.TextXAlignment.Left,TextColor3=Color3.fromRGB(192,207,224)},item.card)
  item.track=create("Frame",{Position=UDim2.new(0,10,1,-7),Size=UDim2.new(1,-20,0,3),BackgroundColor3=Color3.fromRGB(48,62,78),BorderSizePixel=0},item.card)
  local track=item.track
  create("UICorner",{CornerRadius=UDim.new(1,0)},track)
  item.window=create("Frame",{Size=UDim2.fromScale(0,1),BackgroundColor3=Color3.fromRGB(86,215,164),BorderSizePixel=0},track)
  create("UICorner",{CornerRadius=UDim.new(1,0)},item.window)
  item.windowAmount=0
  for j=1,32 do
   item.ring[j]=visualPart("Pass target underline "..i.." "..j,Color3.fromRGB(86,215,164));item.ring[j].Transparency=1
   item.halo[j]=visualPart("Pass target halo "..i.." "..j,Color3.fromRGB(86,215,164));item.halo[j].Transparency=1
  end
  targetVisuals[i]=item
 end
 -- Screen-space chips replace the old world billboards: slim pills above each target that
 -- push each other apart on screen so they never sit on top of one another.
 local HS=settings._hudStyle
 local chipStroke=HS and HS.c.stroke or Color3.fromRGB(40,47,64)
 local chipGui=create("ScreenGui",{Name="PracticalPassChips",ResetOnSpawn=false,IgnoreGuiInset=true,Enabled=false,DisplayOrder=5},P.PlayerGui)
 local chips={}
 for i=1,3 do
  local frame=create("Frame",{Name="Chip"..i,AnchorPoint=Vector2.new(.5,1),Size=UDim2.fromOffset(122,24),BackgroundColor3=Color3.fromRGB(11,16,24),BackgroundTransparency=.08,BorderSizePixel=0,Visible=false},chipGui)
  create("UICorner",{CornerRadius=UDim.new(0,12)},frame)
  local stroke=create("UIStroke",{Color=Color3.fromRGB(86,215,164),Thickness=1.5,Transparency=.1},frame)
  local dot=create("Frame",{AnchorPoint=Vector2.new(0,.5),Position=UDim2.new(0,9,.5,0),Size=UDim2.fromOffset(8,8),BackgroundColor3=Color3.fromRGB(86,215,164),BorderSizePixel=0},frame)
  create("UICorner",{CornerRadius=UDim.new(1,0)},dot)
  local name=create("TextLabel",{BackgroundTransparency=1,Position=UDim2.fromOffset(24,0),Size=UDim2.new(1,-70,1,0),Font=Enum.Font.GothamBold,TextSize=12,TextXAlignment=Enum.TextXAlignment.Left,TextTruncate=Enum.TextTruncate.AtEnd,TextColor3=Color3.fromRGB(246,248,252)},frame)
  local value=create("TextLabel",{BackgroundTransparency=1,AnchorPoint=Vector2.new(1,0),Position=UDim2.new(1,-9,0,0),Size=UDim2.new(0,44,1,0),Font=Enum.Font.GothamBold,TextSize=12,TextXAlignment=Enum.TextXAlignment.Right,TextColor3=Color3.fromRGB(86,215,164)},frame)
  local sub=create("TextLabel",{BackgroundTransparency=1,Position=UDim2.fromOffset(24,21),Size=UDim2.new(1,-34,0,15),Font=Enum.Font.GothamMedium,TextSize=11,TextXAlignment=Enum.TextXAlignment.Left,TextColor3=Color3.fromRGB(190,198,216),Visible=false},frame)
  chips[i]={frame=frame,stroke=stroke,dot=dot,name=name,value=value,sub=sub}
  if HS then HS.skin(frame,12,false)end
 end
 local function hideChips()
  chipGui.Enabled=false
  for _,chip in ipairs(chips)do chip.frame.Visible=false;chip.x=nil end
 end
 local function layoutChips(data,dt)
  local camera=workspace.CurrentCamera
  local order={}
  if camera then
   for i=1,3 do
    local d=data[i]
    if d then
     local point,onScreen=camera:WorldToViewportPoint(d.position+Vector3.new(0,3.5,0))
     if onScreen and point.Z>0 then order[#order+1]={i=i,x=point.X,y=point.Y,d=d}end
    end
   end
  end
  if #order==0 then hideChips();return end
  table.sort(order,function(a,b)return a.y>b.y end)
  local viewport=camera.ViewportSize
  local blend=1-math.exp(-math.clamp(dt or 1/60,0,.1)*16)
  local placed,shown={},{}
  for _,o in ipairs(order)do
   local chip=chips[o.i];local d=o.d;local primary=o.i==1
   local w,h=primary and 156 or 122,primary and 38 or 24
   local x=math.clamp(o.x,w/2+8,math.max(w/2+8,viewport.X-w/2-8))
   local y=math.clamp(o.y,h+8,math.max(h+8,viewport.Y-8))
   -- Push up until clear of every chip already placed (at most three chips).
   for _=1,3 do
    for _,p in ipairs(placed)do
     if math.abs(x-p.x)<(w+p.w)/2+4 and y>p.top-5 and y-h<p.y+5 then y=p.top-6 end
    end
   end
   placed[#placed+1]={x=x,y=y,top=y-h,w=w}
   shown[o.i]=true
   if not chip.x or(Vector2.new(chip.x,chip.y)-Vector2.new(x,y)).Magnitude>320 then chip.x=x;chip.y=y
   else chip.x=chip.x+(x-chip.x)*blend;chip.y=chip.y+(y-chip.y)*blend end
   chip.frame.Size=UDim2.fromOffset(w,h)
   chip.frame.Position=UDim2.fromOffset(chip.x,chip.y)
   chip.frame.Visible=true
   -- Neutral menu-style outline; the best target's outline picks up its status colour.
   chip.stroke.Color=primary and chipStroke:Lerp(d.color,.55)or chipStroke;chip.stroke.Thickness=d.urgent and 1.6 or 1
   chip.dot.BackgroundColor3=d.color;chip.value.TextColor3=d.color
   chip.name.Text=d.name;chip.name.TextSize=primary and 14 or 12
   chip.name.Position=primary and UDim2.fromOffset(24,2)or UDim2.fromOffset(24,0)
   chip.name.Size=primary and UDim2.new(1,-74,0,19)or UDim2.new(1,-70,1,0)
   chip.value.Size=primary and UDim2.new(0,48,0,19)or UDim2.new(0,44,1,0)
   chip.value.Position=primary and UDim2.new(1,-9,0,2)or UDim2.new(1,-9,0,0)
   chip.value.Text=d.reliable and string.format("%+.2f",d.margin)or "--"
   chip.sub.Visible=primary;chip.sub.Text=d.short
  end
  for i,chip in ipairs(chips)do if not shown[i]then chip.frame.Visible=false;chip.x=nil end end
  chipGui.Enabled=true
 end
 local lane={}
 for i=1,9 do lane[i]=visualPart("Recommended pass lane "..i,Color3.fromRGB(86,215,164));lane[i].Transparency=1 end
 local ballOutline=create("Highlight",{Name="PassPossessionBall",Enabled=false,FillTransparency=1,OutlineColor=Color3.fromRGB(94,182,255),OutlineTransparency=.05,DepthMode=Enum.HighlightDepthMode.Occluded},workspace)
 local function hideCourt()
  hideChips()
  ballOutline.Enabled=false
  for _,item in ipairs(targetVisuals)do item.closeout.Transparency=1 end
  for _,item in ipairs(targetVisuals)do item.badge.Enabled=false;item.highlight.Enabled=false;for _,part in ipairs(item.ring)do part.Transparency=1 end;for _,part in ipairs(item.halo)do part.Transparency=1 end end
  for _,part in ipairs(lane)do part.Transparency=1 end
 end
 local function updateCourt(dt)
  local own=model();local root=rootOf(own)
  if not settings.passVision or not root or not holdsBall(own)or settings._visualV2 and not settings.visualShowPass then hideCourt();return end
  local ball=attr(own,"Basketball")
  local hasBall=typeof(ball)=="Instance"
  if hasBall then ballOutline.Adornee=ball end
  if ballOutline.Enabled~=hasBall then ballOutline.Enabled=hasBall end
  local function ground(point)return settings._visualV2 and api.GetVisualGroundPoint and api.GetVisualGroundPoint(point)or floorPoint(point,own)end
  local origin=ground(root.Position)
  local drawn={};local laneDrawn=false;local chipData={}
  for i=1,math.min(3,#results)do
   local v=results[i];local r=rootOf(v.character);local item=targetVisuals[i]
   if r then
    drawn[i]=true
    local proxy=attr(v.character,"ProxyCharacter")
    local renderModel=typeof(proxy)=="Instance"and proxy or v.character
    local renderRoot=typeof(proxy)=="Instance"and(proxy:IsA("BasePart")and proxy or rootOf(proxy))or nil
    renderRoot=renderRoot or r
    local color=tint(v);local center=ground(renderRoot.Position)
    local urgent=i==1 and v.wideOpen
    item.closeout.Transparency=1
    if i==1 and v.nearestDefender then
     local defender=ground(v.nearestDefender);local delta=flat(defender-center)
     local edge=center+(delta.Magnitude>.01 and delta.Unit or Vector3.new(1,0,0))*math.min(5,delta.Magnitude)
     ribbon(item.closeout,defender,edge,.09);item.closeout.Color=tint(v);item.closeout.Transparency=.35
    end
    if urgent then color=Color3.fromRGB(67,236,181)end
    item.highlight.Adornee=renderModel;item.highlight.OutlineColor=color;item.highlight.FillColor=color;item.highlight.FillTransparency=urgent and .83 or 1;item.highlight.OutlineTransparency=urgent and 0 or .3;item.highlight.Enabled=true
    item.badge.Adornee=renderRoot;item.badge.Enabled=false;item.label.TextColor3=color
    -- Compact cards: only the best target gets a detail line and timing bar.
    local primary=i==1
    local short=urgent and "PASS NOW"or not v.reliable and "NO DATA"or(v.laneBlocked or v.obstructed)and "BLOCKED"or v.open and "OPEN"or "CLOSING"
    item.label.Text=v.name.."  ·  "..short
    item.label.TextSize=primary and 14 or 12
    item.label.Position=primary and UDim2.fromOffset(10,5)or UDim2.fromOffset(10,4)
    item.detail.Visible=primary;item.track.Visible=primary
    item.detail.Text=not v.reliable and "no defender data"or string.format("%+.2fs shot window",v.margin)
    item.badge.Size=primary and(urgent and UDim2.fromOffset(196,52)or UDim2.fromOffset(176,48))or UDim2.fromOffset(150,28)
    chipData[i]={position=renderRoot.Position,color=color,name=v.name,margin=v.margin,reliable=v.reliable,urgent=urgent,
     short=urgent and "PASS NOW"or not v.reliable and "NO DATA"or(v.laneBlocked or v.obstructed)and "BLOCKED"or v.open and "OPEN LANE"or "DEFENDER CLOSING"}
    item.stroke.Color=color;item.window.BackgroundColor3=color
    for layer,stroke in ipairs(item.cardHalo)do stroke.Color=color;stroke.Transparency=urgent and(.58+layer*.09)or(.78+layer*.045)end
    local amount=v.reliable and math.clamp(v.margin/1.25,0,1)or 0
    item.windowAmount=item.windowAmount+(amount-item.windowAmount)*(1-math.exp(-math.clamp(dt or 1/60,0,.1)*12))
    item.window.Size=UDim2.fromScale(item.windowAmount,1)
    for j,part in ipairs(item.ring)do repeat
     -- Only the best target keeps a ground ring; the others are just their chip.
     if i~=1 then part.Transparency=1;item.halo[j].Transparency=1;break end
     local radius=(urgent and 2.2 or 1.25)*(settings.visualScale or 1)
     local a=math.pi*.12+(j-1)*math.pi*1.75/32;local b=math.pi*.12+j*math.pi*1.75/32
     local weight=settings._visualV2 and (settings.visualWeight or 1)or 1
     ribbon(part,center+Vector3.new(math.cos(a)*radius,0,math.sin(a)*radius),center+Vector3.new(math.cos(b)*radius,0,math.sin(b)*radius),(i==1 and .075 or .04)*weight)
     part.Color=color;part.Transparency=(i==1 and .18 or .5)+(settings._visualV2 and (1-(settings.visualIntensity or 80)/100)*.3 or 0)
     local halo=item.halo[j]
     if settings._visualV2 and i==1 then
      ribbon(halo,center+Vector3.new(math.cos(a)*(radius+.2),.025,math.sin(a)*(radius+.2)),center+Vector3.new(math.cos(b)*(radius+.2),.025,math.sin(b)*(radius+.2)),.18*weight)
      halo.Color=color;halo.Transparency=urgent and .65 or .88
     elseif halo.Transparency~=1 then halo.Transparency=1 end
    until true end
    if i==1 and v.open then
     laneDrawn=true
     local delta=center-origin
     if settings._visualV2 and api.DrawVisualRoute then
      api.DrawVisualRoute(lane,origin,center,.055*(settings.visualWeight or 1),color,.4)
     else for j,part in ipairs(lane)do
      ribbon(part,origin+delta*((j-.8)/9),origin+delta*((j-.35)/9),.085);part.Color=color;part.Transparency=.4
     end end
    end
   end
  end
  for i,item in ipairs(targetVisuals)do if not drawn[i]then
   item.closeout.Transparency=1
   if item.badge.Enabled then item.badge.Enabled=false end
   if item.highlight.Enabled then item.highlight.Enabled=false end
   for _,part in ipairs(item.ring)do if part.Transparency~=1 then part.Transparency=1 end end
   for _,part in ipairs(item.halo)do if part.Transparency~=1 then part.Transparency=1 end end
  end end
  if not laneDrawn then for _,part in ipairs(lane)do if part.Transparency~=1 then part.Transparency=1 end end end
  layoutChips(chipData,dt)
 end
 local courtConnection=(Run.RenderStepped or Run.Heartbeat):Connect(updateCourt)
 local function velocity(m,position,now)
  local previous=snapshots[m];local result=Vector3.zero
  if previous then
   local dt=now-previous.time
   if dt>.02 and dt<.6 then result=flat(position-previous.position)/dt;if result.Magnitude>28 then result=Vector3.zero end end
  end
  snapshots[m]={position=position,time=now};return result
 end
 local last=0
 local connection=Run.Heartbeat:Connect(function()
  local now=os.clock();if now-last<.12 then return end;last=now
  if not settings.passVision then board.Enabled=false;outline.Enabled=false;results={};return end
  local own=model();local root=rootOf(own)
  if not root or not holdsBall(own)then board.Enabled=false;outline.Enabled=false;results={};return end
  results={}
  board.Enabled=true;heading.Text="PASS TARGETS"
  panel.Visible=not settings._visualV2 or settings.visualPassPanel and settings.visualShowPass
  local team=attr(own,"Team")
  if team==nil or team==0 then labels[1].Text="Waiting for reliable team information";labels[2].Text="";labels[3].Text="";outline.Enabled=false;return end
  local mates,defenders={},{}
  for _,m in ipairs(defenseCharacters())do
   local r=rootOf(m)
   local ownCourt,otherCourt=attr(own,"CourtNumber")or attr(own,"Court"),attr(m,"CourtNumber")or attr(m,"Court")
   local sameCourt=not(ownCourt~=nil and otherCourt~=nil and ownCourt~=0 and otherCourt~=0 and ownCourt~=otherCourt)
   if r and m~=own and sameCourt and math.abs(r.Position.Y-root.Position.Y)<10 then
    local v=velocity(m,r.Position,now);local t=attr(m,"Team")
    if t==team then table.insert(mates,{model=m,position=r.Position,velocity=v})
    elseif t~=nil and t~=0 then table.insert(defenders,{position=r.Position,speed=v.Magnitude})end
   end
  end
  for _,mate in ipairs(mates)do
   if flat(mate.position-root.Position).Magnitude<=100 then
    local value=evaluate(root.Position,mate.position,mate.velocity,defenders,ping)
    value.name=mate.model.Name;value.character=mate.model
    local params=RaycastParams.new();params.FilterType=Enum.RaycastFilterType.Exclude
    local excluded={own,mate.model,visualFolder}
    local chars=workspace:FindFirstChild("Characters");if chars then table.insert(excluded,chars)end
    local ball=attr(own,"Basketball");if typeof(ball)=="Instance"then table.insert(excluded,ball)end
    params.FilterDescendantsInstances=excluded
    value.obstructed=workspace:Raycast(root.Position+Vector3.new(0,2,0),value.position-root.Position,params)~=nil
    value.open=value.reliable and not value.laneBlocked and not value.obstructed and value.margin>.15
    local goal=goalPosition(attr(mate.model,"Goal"))or goalPosition(attr(own,"Goal"))
    value.wideOpen=value.open and value.nearest>=10 and value.margin>.3 and goal~=nil and flat(goal-mate.position).Magnitude<=65
    value.score=(value.wideOpen and 150 or value.open and 100 or 0)+math.clamp(value.margin,-5,5)-(value.laneBlocked and 10 or 0)-(value.obstructed and 20 or 0)
    table.insert(results,value)
   end
  end
  table.sort(results,function(a,b)return a.score>b.score end)
  -- Keep similarly ranked open targets stable instead of flashing between teammates.
  if preferred and results[1]and results[1].open then
   for i=2,#results do
    if results[i].character==preferred and results[i].open and results[1].score-results[i].score<.25 then
     local retained=table.remove(results,i);table.insert(results,1,retained);break
    end
   end
  end
  preferred=results[1]and results[1].open and results[1].character or nil
  if #results==0 then labels[1].Text="No nearby teammates detected"end
  for i=1,math.min(3,#results)do
   local v=results[i]
   local verdict=not v.reliable and "NO DATA"or v.obstructed and "BLOCKED"or v.laneBlocked and "AT RISK"or v.open and "OPEN"or "CLOSING"
   labels[i].Text=v.reliable and string.format("%d  %s  ·  %s  ·  %+.2fs",i,v.name,verdict,v.margin)or string.format("%d  %s  ·  %s",i,v.name,verdict)
   labels[i].TextColor3=v.open and Color3.fromRGB(104,228,176)or Color3.fromRGB(255,200,102)
  end
  for i=math.max(2,#results+1),3 do if labels[i].Text~=""then labels[i].Text=""end end
  local showBest=results[1]~=nil and results[1].open and not settings._visualV2
  if showBest then outline.Adornee=results[1].character;outline.FillColor=Color3.fromRGB(53,186,135);outline.OutlineColor=Color3.fromRGB(86,215,164)end
  if outline.Enabled~=showBest then outline.Enabled=showBest end
 end)
 -- Persistent geometry: no per-frame instances or blinking visibility timers.
 local shotBoard=create("ScreenGui",{Name="CatchShootLiveReadout",Enabled=false,ResetOnSpawn=false,DisplayOrder=4},P.PlayerGui)
 local shotCard=create("Frame",{Size=UDim2.fromOffset(212,46),BackgroundColor3=Color3.fromRGB(12,19,27),BackgroundTransparency=.1,BorderSizePixel=0},shotBoard)
 if settings._hudRegister then settings._hudRegister(shotCard,5)end
 create("UICorner",{CornerRadius=UDim.new(0,12)},shotCard)
 local shotStroke=create("UIStroke",{Thickness=1.5,Color=Color3.fromRGB(94,182,255)},shotCard)
 local shotTitle=create("TextLabel",{BackgroundTransparency=1,Position=UDim2.fromOffset(10,5),Size=UDim2.new(1,-20,0,20),Font=Enum.Font.GothamBold,TextSize=14,TextXAlignment=Enum.TextXAlignment.Left,TextTruncate=Enum.TextTruncate.AtEnd,TextColor3=Color3.fromRGB(94,182,255),Text=""},shotCard)
 local shotDetail=create("TextLabel",{BackgroundTransparency=1,Position=UDim2.fromOffset(10,25),Size=UDim2.new(1,-20,0,16),Font=Enum.Font.Gotham,TextSize=12,TextXAlignment=Enum.TextXAlignment.Left,TextTruncate=Enum.TextTruncate.AtEnd,TextColor3=Color3.fromRGB(226,230,240),Text=""},shotCard)
 local shotRing={}
 for i=1,40 do shotRing[i]=visualPart("Estimated contest boundary "..i,Color3.fromRGB(94,182,255));shotRing[i].Transparency=1 end
 local closeoutLine=visualPart("Nearest defender to contest boundary",Color3.fromRGB(242,178,87));closeoutLine.Transparency=1
 local defenderMark=create("Highlight",{Name="CatchShootNearestDefender",Enabled=false,FillTransparency=1,OutlineTransparency=.15,OutlineColor=Color3.fromRGB(242,178,87),DepthMode=Enum.HighlightDepthMode.Occluded},workspace)
 local function hideShot()
  shotBoard.Enabled=false;defenderMark.Enabled=false;closeoutLine.Transparency=1
  for _,part in ipairs(shotRing)do part.Transparency=1 end
 end
 local shotConnection=(Run.RenderStepped or Run.Heartbeat):Connect(function()
  local own=model();local root=rootOf(own)
  if not settings.catchShoot or not root or settings._visualV2 and not settings.visualShowPass then hideShot();return end
  local state=api.GetCatchShootState and api.GetCatchShootState()or {}
  if not holdsBall(own)and not state.preholding and not state.pending then hideShot();return end
  local center=floorPoint(root.Position,own)
  local nearest,other=math.huge,nil
  local ownTeam=attr(own,"Team")
  for _,character in ipairs(defenseCharacters())do
   local r=rootOf(character);local team=attr(character,"Team")
   local court=attr(character,"CourtNumber")or attr(character,"Court")
   local ownCourt=attr(own,"CourtNumber")or attr(own,"Court")
   local function normalized(v)return tostring(v or ""):lower():gsub("^court[%s_%-]*","")end
   local sameCourt=not court or not ownCourt or normalized(court)==normalized(ownCourt)
   if r and character~=own and ownTeam and ownTeam~=0 and team and team~=0 and team~=ownTeam and sameCourt and math.abs(r.Position.Y-root.Position.Y)<10 then
    local distance=flat(r.Position-root.Position).Magnitude
    if distance<nearest then nearest=distance;other=character end
   end
  end
  local radius=10*(1-math.clamp(settings.catchShootContest or 10,0,100)/100)
  local arrival=other and math.max(0,nearest-radius)/16 or nil
  local release=(settings.catchRelease or .5)+math.min(ping or 0,.2)*.5+.06
  local margin=arrival and arrival-release or nil
  local safe=margin and margin>.15
  local color=not other and Color3.fromRGB(178,186,206)or safe and Color3.fromRGB(104,228,176)or Color3.fromRGB(255,200,102)
  shotBoard.Enabled=true;shotTitle.TextColor3=color
  shotTitle.Text=attr(own,"Action")=="Shooting"and "SHOOTING"or state.preholding and "SHOT QUEUED"or state.shooting and "SHOT SENT"or safe and "OPEN SHOT"or other and "CLOSEOUT RISK"or "NO DEFENDER DATA"
  shotDetail.Text=other and string.format("%s · %.1f studs · %+.2fs",other.Name,math.max(0,nearest-radius),margin or 0)or "no same-court defender seen"
  if state.preholding then shotDetail.Text=tostring(state.bufferReason or state.status or "waiting for catch")end
  for i,part in ipairs(shotRing)do
   local a=(i-1)*math.pi*2/40;local b=i*math.pi*2/40
   ribbon(part,center+Vector3.new(math.cos(a)*radius,0,math.sin(a)*radius),center+Vector3.new(math.cos(b)*radius,0,math.sin(b)*radius),.055)
   part.Color=color;part.Transparency=.42
  end
  defenderMark.Enabled=other~=nil;closeoutLine.Transparency=1
  if other then
   local proxy=attr(other,"ProxyCharacter");defenderMark.Adornee=typeof(proxy)=="Instance"and proxy or other
   local point=floorPoint(rootOf(other).Position,own);local delta=flat(point-center)
   local edge=center+(delta.Magnitude>.01 and delta.Unit or Vector3.new(1,0,0))*math.min(radius,delta.Magnitude)
   ribbon(closeoutLine,point,edge,.11);closeoutLine.Color=color;closeoutLine.Transparency=.22
  end
 end)
 local priorShotCleanup=featureCleanup
 featureCleanup=function()shotConnection:Disconnect();hideShot();shotBoard:Destroy();defenderMark:Destroy();closeoutLine:Destroy();for _,part in ipairs(shotRing)do part:Destroy()end;priorShotCleanup()end
 local previous=featureCleanup
 featureCleanup=function()connection:Disconnect();courtConnection:Disconnect();hideCourt();for _,item in ipairs(targetVisuals)do item.closeout:Destroy();item.highlight:Destroy();for _,part in ipairs(item.ring)do part:Destroy()end;for _,part in ipairs(item.halo)do part:Destroy()end end;for _,part in ipairs(lane)do part:Destroy()end;ballOutline:Destroy();chipGui:Destroy();board:Destroy();outline:Destroy();settings.passVision=false;previous()end
end)()end

-- Reachable enemy lob interception; native movement/jump/steal only.
if api then (function()
 settings.antiOop=false;settings.oopMove=true;settings.oopRange=18;settings.oopGuide=true
 numericLimits.oopRange={8,26}
 local samples=setmetatable({},{__mode="k"})
 local boards={};local boardsAt=-math.huge
 local function nearBackboard(position,now,radius)
  if now-boardsAt>2 then
   boardsAt=now;boards={}
   pcall(function()for _,part in ipairs(workspace:GetDescendants())do
    if part:IsA("BasePart")and part.Name:lower():find("backboard",1,true)then boards[#boards+1]=part end
   end end)
  end
  for _,part in ipairs(boards)do
   local ok,distance=pcall(function()
    local q=part.CFrame:PointToObjectSpace(position);local h=part.Size*.5
    return (q-Vector3.new(math.clamp(q.X,-h.X,h.X),math.clamp(q.Y,-h.Y,h.Y),math.clamp(q.Z,-h.Z,h.Z))).Magnitude
   end)
   if ok and distance<=(radius or .8)+.08 then return true end
  end
  return false
 end
 local requestBoardJump
 local candidate,releaseAt,releaseService
 local status="Waiting for a lob"
 local traceAt=-math.huge
 local function traceJump(now,phase,root,sample,point,plan)
  if phase=="tracking"and now-traceAt<.25 then return end
  traceAt=now
  local position=sample and sample.position
  local fit=sample and sample.fit
  local detail=string.format("[Practical Lob] %s t=%.3f status=%s gap=%.2f ballY=%.2f vy=%.2f contactETA=%.3f takeoffETA=%.3f board=%s lastJump=%s",
   phase,now,status,point and flat(point-root.Position).Magnitude or -1,
   position and position.Y-root.Position.Y or -999,fit and fit.velocity.Y or -999,
   plan and plan.meetAt-now or -999,plan and plan.jumpAt-now or -999,
   tostring(sample and sample.boardHitAt~=nil),tostring(settings._lastDefenseJump and settings._lastDefenseJump.source))
  print(detail)
 end
 api.GetAntiOopState=function()
  local root=rootOf(model())
  local fit=candidate and candidate.sample.fit
  return {status=status,target=candidate and candidate.point,time=candidate and candidate.time,
   ballPosition=candidate and candidate.position,flight=fit and {position=fit.position,velocity=fit.velocity,gravity=fit.gravity,time=candidate.chaseOnly and .9 or math.min(1.2,candidate.time or .9)},
   gap=root and candidate and flat(candidate.point-root.Position).Magnitude,
   lastJump=settings._lastDefenseJump and settings._lastDefenseJump.source,jumpDetails=settings._lastDefenseJump,backboardHit=candidate and candidate.sample.boardHitAt,fitError=candidate and candidate.sample.fit and candidate.sample.fit.error,jumpModel=settings._oopJumpProfile and table.clone(settings._oopJumpProfile)}
 end
 local marker=visualPart("Lob interception zone",Color3.fromRGB(73,168,255))
 marker.Shape=Enum.PartType.Cylinder;marker.Size=Vector3.new(.05,1.8,1.8);marker.Transparency=1
 local guide=visualPart("Lob interception route",Color3.fromRGB(73,168,255));guide.Transparency=1
 local stopped=false
 local function release()
  if releaseService then pcall(function()releaseService.Shoot:Fire({Shoot=false,UseSpace=true})end)end
  releaseAt=nil;releaseService=nil
 end
 local function teamEnemy(own,other)
  if other==nil or other==own then return false end
  local a,b=attr(own,"Team"),attr(other,"Team")
  if a~=nil and a~=0 and a~="" and b~=nil and b~=0 and b~="" then return a~=b end
  -- No team attributes (1v1 modes such as Blacktop): use Roblox teams if they exist, otherwise
  -- anyone else on your court counts as the opponent.
  local players=game:GetService("Players")
  local ownPlayer=players:GetPlayerFromCharacter(own)or P
  local otherPlayer=players:GetPlayerFromCharacter(other)or players:FindFirstChild(other.Name)
  if ownPlayer and otherPlayer and ownPlayer.Team~=nil and otherPlayer.Team~=nil then return ownPlayer.Team~=otherPlayer.Team end
  local courtA,courtB=attr(own,"CourtNumber"),attr(other,"CourtNumber")
  if courtA~=nil and courtB~=nil and courtA~=0 and courtB~=0 and courtA~=courtB then return false end
  return true
 end
 settings._oopOwnsBall=function(ball)
  if not settings.antiOop or not ball then return false end
  local sample=samples[ball];local own=model()
  if not own then return false end
  local owner=sample and sample.owner
  -- Reserve an observed lob even before this frame's tracking callback runs.
  for _,character in ipairs(defenseCharacters())do
   if attr(character,"Basketball")==ball then owner=character;break end
  end
  if not owner or not teamEnemy(own,owner)then return false end
  local recent=sample and (os.clock()-(sample.heldAt or 0)<1.8 or sample.accepted and os.clock()-(sample.freeSince or 0)<4)
  local held=attr(owner,"Basketball")==ball
  if not held and not recent then return false end
  local kind=tostring(attr(ball,"LastShotType")or ""):lower()
  local marked=kind:find("lob",1,true)~=nil or attr(ball,"Lob")==true or attr(ball,"SelfLob")==true
  local position=goalPosition(ball);local goal=goalPosition(attr(owner,"Goal"));local root=rootOf(own)
  return marked or (attr(ball,"MidShot")~=true and position and root and goal and position.Y-root.Position.Y>3 and flat(position-goal).Magnitude<13)or false
 end
 settings._oopDirection=function()
  settings.oopDriving=false
  if not settings.antiOop or not settings.oopMove or not candidate or os.clock()-candidate.at>.15 or Input:GetFocusedTextBox()then return nil end
  local own=model();local root=rootOf(own)
  if not root or holdsBall(own)or attr(own,"CanMove")==false then return nil end
  local action=attr(own,"Action")
  if action=="Shooting"or action=="Dunking"or action=="Stealing"then return nil end
  local delta=flat(candidate.point-root.Position)
  settings.oopDriving=true;guardError=delta.Magnitude;guardReason="Intercepting lob"
  return delta.Magnitude>.8 and delta.Unit or Vector3.zero
 end
-- Fit the observed ball path in seconds relative to the current frame.
local function fitFlight(history,now)
 if not history or #history<5 then return nil end
 local pts={}
 for _,p in ipairs(history)do if now-p.time<=.38 then pts[#pts+1]=p end end
 if #pts<5 or pts[#pts].time-pts[1].time<.12 or now-pts[#pts].time>.12 then return nil end
 local n,s1,s2,s3,s4=0,0,0,0,0
 local x,z,tx,tz,y,ty,t2y=0,0,0,0,0,0,0
 for _,p in ipairs(pts)do
  local t=p.time-now;local t2=t*t
  n=n+1;s1=s1+t;s2=s2+t2;s3=s3+t2*t;s4=s4+t2*t2
  x=x+p.position.X;z=z+p.position.Z;tx=tx+t*p.position.X;tz=tz+t*p.position.Z
  y=y+p.position.Y;ty=ty+t*p.position.Y;t2y=t2y+t2*p.position.Y
 end
 local denom=n*s2-s1*s1;if math.abs(denom)<1e-7 then return nil end
 local vx=(n*tx-s1*x)/denom;local vz=(n*tz-s1*z)/denom
 local px=(x-vx*s1)/n;local pz=(z-vz*s1)/n
 local matrix={{n,s1,s2,y},{s1,s2,s3,ty},{s2,s3,s4,t2y}}
 for col=1,3 do
  local pivot=col
  for row=col+1,3 do if math.abs(matrix[row][col])>math.abs(matrix[pivot][col])then pivot=row end end
  matrix[col],matrix[pivot]=matrix[pivot],matrix[col]
  local divisor=matrix[col][col];if math.abs(divisor)<1e-9 then return nil end
  for k=col,4 do matrix[col][k]=matrix[col][k]/divisor end
  for row=1,3 do if row~=col then
   local factor=matrix[row][col]
   for k=col,4 do matrix[row][k]=matrix[row][k]-factor*matrix[col][k]end
  end end
 end
 local py,vy,curve=matrix[1][4],matrix[2][4],matrix[3][4]
 local gravity=-2*curve
 if gravity<8 or gravity>240 or flat(Vector3.new(vx,0,vz)).Magnitude>110 then return nil end
 local error=0
 for _,p in ipairs(pts)do
  local t=p.time-now
  local predicted=Vector3.new(px+vx*t,py+vy*t+curve*t*t,pz+vz*t)
  error=error+(predicted-p.position).Magnitude^2
 end
 error=math.sqrt(error/n)
 if error>.6 then return nil end
 return {position=Vector3.new(px,py,pz),velocity=Vector3.new(vx,vy,vz),gravity=gravity,error=error,count=n,time=now}
end
local function flightPoint(fit,t)
 return fit.position+fit.velocity*t+Vector3.new(0,-.5*fit.gravity*t*t,0)
end
-- Find a meeting time that permits both ground travel and the rising phase of
-- a normal jump. Player jump reach is an estimate; ball acceleration is fitted.
local function contactPlan(fit,rootPosition,speed,profile,latency,freeSince,now,previous,readyAt)
 if not fit then return nil end
 local gravity,launch,hands=profile.gravity,profile.launch,profile.hands
 local function evaluate(t)
  if t<.025 or t>1.25 then return nil end
  local point=flightPoint(fit,t);local height=point.Y-rootPosition.Y
  local rise=height-hands
  local discriminant=launch*launch-2*gravity*rise
  if rise<.5 or discriminant<0 or fit.velocity.Y-fit.gravity*t> -1 then return nil end
  local riseTime=math.max(.035,(launch-math.sqrt(discriminant))/gravity)
  local delay=t-riseTime-(profile.delay or .05)-math.clamp(latency or 0,0,.15)
  if delay<-.09 or now+delay<(freeSince or now)+.16 then return nil end
  if readyAt and now+delay<readyAt then return nil end
  local gap=flat(point-rootPosition).Magnitude
  local run=math.max(0,gap-.85)/math.max(speed,6)
  if gap>.9 and run+.025>delay then return nil end
  return {point=point,meetAt=now+t,jumpAt=now+delay,riseTime=riseTime,height=height,runTime=run,time=t,score=t+gap*.025}
 end
 if previous then
  local value=evaluate(previous.meetAt-now)
  if value and flat(value.point-previous.point).Magnitude<1.25 then return value end
 end
 local best
 for i=4,62 do
  local value=evaluate(i*.02)
  if value and(not best or value.score<best.score)then best=value end
 end
 return best
end

 local mover={speed=14}
 local playerGravity=math.max(30,workspace.Gravity)
 local jumpProfile={gravity=playerGravity,launch=math.sqrt(2*playerGravity*4.5),hands=2.5,delay=.05}
 settings._oopJumpProfile=jumpProfile
 local jumpObservation
 requestBoardJump=function(ball,sample,now)
  if stopped or not settings.antiOop or sample.jumped or not sample.boardHitAt or now-sample.boardHitAt>.2 or not sample.freeSince then return end
  local own=model();local root=rootOf(own)
  local service=keyboard and keyboard.Services and keyboard.Services.InputService
  if not root or not service or not teamEnemy(own,sample.owner)or holdsBall(own)or Input:GetFocusedTextBox()then return end
  local position=goalPosition(ball)
  if not position or position.Y-root.Position.Y<3 or flat(position-root.Position).Magnitude>(settings.oopRange or 18)then return end
  if attr(own,"CanJump")==false or now-lastJump<(settings.jumpCooldown or 1.2)then return end
  settings._oopActiveAt=now
  pcall(function()service.HoldG:Fire({HoldingG=false})end)
  if pcall(function()service.Shoot:Fire({Shoot=true,Input="E",UseSpace=true})end)then releaseService=service;releaseAt=now+.08 end
  local ok,err=pcall(function()service.Jump:Fire()end)
  if ok then
   sample.jumped=true;lastJump=now;jumpObservation={start=now,baseY=root.Position.Y,history={}}
   status="Jump requested: backboard contact"
   settings._lastDefenseJump={source="Anti alley-oop backboard",time=now,detector=sample.boardSource or "sound"}
   recordDefense("attempt_sent",{action="Backboard lob jump"})
  else status="Jump input failed: "..tostring(err)end
 end
 local arc={}
 for i=1,12 do arc[i]=visualPart("Predicted lob path "..i,Color3.fromRGB(80,174,255));arc[i].Transparency=1 end
 local connection=Run.Heartbeat:Connect(function()
  if stopped then return end
  local now=os.clock()
  if releaseAt and now>=releaseAt then release()end
  candidate=nil;for _,part in ipairs(arc)do part.Transparency=1 end;settings._oopFacePosition=nil;settings._oopFaceAt=nil;marker.Transparency=1;guide.Transparency=1
  if not settings.antiOop or Input:GetFocusedTextBox()then settings.oopDriving=false;release();return end
  local own=model();local root=rootOf(own)
  local service=keyboard and keyboard.Services and keyboard.Services.InputService
  if not root or not service then return end
  if mover.position and now-mover.time>.02 and now-mover.time<.2 and settings.oopDriving then
   local measured=flat(root.Position-mover.position).Magnitude/(now-mover.time)
   if measured>3 and measured<28 then mover.speed=math.clamp(mover.speed*.75+measured*.25,6,22)end
  end
  mover.position=root.Position;mover.time=now
  if jumpObservation then
   local observe=jumpObservation
   if not observe.confirmed and root.Position.Y-observe.baseY>.3 then
    observe.confirmed=true
    print(string.format("[Practical Lob] actual takeoff observed after %.3f seconds",now-observe.start))
   end
   if now-observe.start>1.2 then
    if not observe.confirmed then print("[Practical Lob] jump request produced no observed takeoff")end
    jumpObservation=nil
   elseif root.Position.Y-observe.baseY>.12 and (not observe.last or now-observe.last>=.025)then
    observe.last=now
    observe.history[#observe.history+1]={position=root.Position,time=now}
    if #observe.history>14 then table.remove(observe.history,1)end
    local fit=fitFlight(observe.history,now)
    if fit and fit.gravity>=30 then
     local height=fit.position.Y-observe.baseY
     local discriminant=fit.velocity.Y^2+2*fit.gravity*height
     if discriminant>0 then
      local elapsed=(-fit.velocity.Y+math.sqrt(discriminant))/fit.gravity
      local launch=fit.velocity.Y+fit.gravity*elapsed
      local delay=now-elapsed-observe.start
      local peak=launch^2/(2*fit.gravity)
      if delay>=-.02 and delay<.25 and launch>=15 and launch<=80 and peak>=1.5 and peak<=12 then
       jumpProfile.gravity=fit.gravity;jumpProfile.launch=launch;jumpProfile.delay=math.clamp(delay,.01,.2);jumpProfile.learned=true
      end
     end
    end
   end
  end


  if holdsBall(own)then local held=attr(own,"Basketball");if held then samples[held]=nil end;return end
  local occupied={}
  for _,m in ipairs(defenseCharacters())do
   local ball=attr(m,"Basketball")
   if typeof(ball)=="Instance"then occupied[ball]=m end
  end
  local folder=workspace:FindFirstChild("Basketballs");if not folder then return end
  for _,ball in ipairs(folder:GetChildren())do
   local position=goalPosition(ball)
   if position then
    local sample=samples[ball]
    if not sample then
     sample={position=position,time=now};samples[ball]=sample
     pcall(function()
      local sounds=ball:FindFirstChild("Sounds",true)
      local sound=sounds and sounds:FindFirstChild("Backboard")
      if sound and sound:IsA("Sound")then
       sample.boardConnection=sound.Played:Connect(function()
        if sample.freeSince then sample.boardHitAt=os.clock();if requestBoardJump then requestBoardJump(ball,sample,sample.boardHitAt)end end
       end)
      end
     end)
    end
    local holder=occupied[ball]
    if not sample.touchConnection then pcall(function()
     sample.touchConnection=ball.Touched:Connect(function(part)
      local isBoard=false
      local node=part
      for _=1,4 do
       if not node then break end
       if node.Name:lower():find("backboard",1,true)then isBoard=true;break end
       node=node.Parent
      end
      if isBoard and sample.freeSince and not sample.boardHitAt then
       local hitAt=os.clock();sample.boardHitAt=hitAt;sample.boardSource="physical touch"
       requestBoardJump(ball,sample,hitAt)
      end
     end)
    end)end
    if holder then sample.owner=holder;sample.heldAt=now;sample.jumped=false;sample.stolen=false;sample.velocity=nil;sample.rawVelocity=nil;sample.gravity=nil;sample.airSamples=0;sample.freeSince=nil;sample.fallSince=nil;sample.peak=nil;sample.plan=nil;sample.boardHitAt=nil;sample.boardSource=nil;sample.accepted=false;sample.history={};sample.fit=nil end
    if not holder then
     sample.freeSince=sample.freeSince or now
     sample.peak=math.max(sample.peak or position.Y,position.Y)
    end
    -- Check actual ball-surface contact each heartbeat, before the 25 ms velocity sampler.
    if not holder and not sample.boardHitAt and sample.rawVelocity and sample.rawVelocity.Magnitude>2 then
     local radius=.8
     pcall(function()radius=math.clamp(math.max(ball.Size.X,ball.Size.Y,ball.Size.Z)*.5,.25,2)end)
     if nearBackboard(position,now,radius)then
      sample.boardHitAt=now;sample.boardSource="surface contact";requestBoardJump(ball,sample,now)
     end
    end
    local dt=now-sample.time
    if dt>=.025 then
     local velocity=(position-sample.position)/dt
     if not holder and sample.rawVelocity and not sample.boardHitAt and dt<.15 then
      local before=flat(sample.rawVelocity);local after=flat(velocity)
      if before.Magnitude>4 and after.Magnitude>2 and before:Dot(after)<0 and nearBackboard(position,now)then
       sample.boardHitAt=now;sample.history={};sample.fit=nil;sample.plan=nil;requestBoardJump(ball,sample,now)
      end
     end
     if not holder then sample.airSamples=(sample.airSamples or 0)+1 end
     if not holder then
      sample.history=sample.history or {}
      sample.history[#sample.history+1]={position=position,time=now}
      if #sample.history>14 then table.remove(sample.history,1)end
      local fitted=fitFlight(sample.history,now)
      if fitted then sample.fit=fitted
      elseif sample.fit and now-sample.fit.time>.12 then sample.fit=nil end
     end

     sample.position=position;sample.time=now
     if velocity.Magnitude<150 and dt<.3 then
      if sample.rawVelocity then
       local measured=(sample.rawVelocity.Y-velocity.Y)/dt
       if measured>5 and measured<220 then sample.gravity=sample.gravity and sample.gravity*.7+measured*.3 or measured end
      end
      sample.rawVelocity=velocity
      sample.velocity=sample.velocity and sample.velocity:Lerp(velocity,.6)or velocity
     else sample.velocity=nil;sample.rawVelocity=nil;sample.gravity=nil end
    end
    if not holder and sample.rawVelocity and sample.rawVelocity.Y< -2 and (sample.peak or position.Y)-position.Y>.2 then
     sample.fallSince=sample.fallSince or now
    else sample.fallSince=nil end
    local owner=sample.owner
    local v=sample.velocity
    local recent=owner and (now-(sample.heldAt or 0)<1.8 or sample.accepted and now-(sample.freeSince or now)<4)
    local shotType=tostring(attr(ball,"LastShotType")or ""):lower()
    local marked=shotType:find("lob",1,true)~=nil or attr(ball,"Lob")==true or attr(ball,"SelfLob")==true
    local goal=owner and goalPosition(attr(owner,"Goal"))
    local high=position.Y-root.Position.Y>3 or v and v.Y>4
    local normalShot=attr(ball,"MidShot")==true and not marked
    if not holder and recent and teamEnemy(own,owner)and v and now-sample.time<=.15 and not normalShot and (marked or high and goal and flat(position-goal).Magnitude<13)and (sample.accepted or flat(position-root.Position).Magnitude<=settings.oopRange)then
     sample.accepted=true
     -- Plan a takeoff AFTER the same observed-descent gate used by the executor.
     -- Previously the planner chose an earlier takeoff, which expired while waiting.
     local readyAt=sample.fallSince and sample.fallSince+.06
     if not readyAt and sample.fit then
      local fit=sample.fit
      readyAt=now+math.max(0,(fit.velocity.Y+math.max(2,math.sqrt(.4*fit.gravity)))/fit.gravity)+.06
     end
     local contact=contactPlan(sample.fit,root.Position,mover.speed,jumpProfile,jumpProfile.learned and 0 or ping,sample.freeSince,now,sample.plan,readyAt)
     -- Preserve the chosen meeting point through a brief fit/reachability fluctuation.
     local prior=sample.plan
     if prior and (not readyAt or prior.jumpAt>=readyAt)and sample.fit and prior.meetAt>now+.025 and now<=prior.jumpAt+.09 and flat(flightPoint(sample.fit,prior.meetAt-now)-prior.point).Magnitude<.75 then
      contact=table.clone(prior);contact.time=prior.meetAt-now
     end
     sample.plan=contact
     local point,time=contact and contact.point,contact and contact.time
     if point then
      local score=flat(point-root.Position).Magnitude*.05+time
      if not candidate or candidate.chaseOnly or score<candidate.score then candidate={ball=ball,point=point,position=position,time=time,at=now,sample=sample,score=score,owner=owner,plan=contact}end
     end
     if not candidate then
      -- Keep pursuing an observed enemy lob while its catch window is forming.
      -- This fallback can move under the flight, but can never trigger a jump.
      candidate={ball=ball,point=position+flat(v)*.12,position=position,time=.12,at=now,sample=sample,score=100,chaseOnly=true,owner=owner}
     end
     if sample.jumped and flat(position-root.Position).Magnitude<=4 and position.Y-root.Position.Y>-.5 and position.Y-root.Position.Y<4.5 then
      candidate={ball=ball,point=position,position=position,time=0,at=now,sample=sample,score=-1,owner=owner}
     end
    end
   end
  end
  if not candidate then status="No reachable enemy lob";return end
  settings._oopFacePosition=candidate.position;settings._oopFaceAt=now
  local gap=flat(candidate.point-root.Position).Magnitude
  settings._oopActiveAt=now
  status=candidate.chaseOnly and "Approach: no reachable catch window yet"or gap>1.1 and "Approach: move beneath catch point"or "Aligned: waiting for descending catch window"
  if settings.oopGuide then
   local destination=floorPoint(candidate.point,own)
   marker.CFrame=CFrame.new(destination)*CFrame.Angles(0,0,math.pi/2);marker.Transparency=settings._visualV2 and 1 or .5;marker.Color=candidate.plan and Color3.fromRGB(65,191,221)or Color3.fromRGB(232,166,78)
   ribbon(guide,floorPoint(root.Position,own),destination,.11);guide.Transparency=settings._visualV2 and 1 or .2
   if candidate.sample.fit and not settings._visualV2 then
    local fit=candidate.sample.fit;local duration=candidate.chaseOnly and .9 or math.min(1.2,candidate.time)
    for i,part in ipairs(arc)do
     ribbon(part,flightPoint(fit,duration*(i-1)/12),flightPoint(fit,duration*i/12),.045);part.Transparency=.4
    end
   end

  end
  if attr(own,"CanMove")==false then status="Game movement locked";return end
  local action=attr(own,"Action")
  if action=="Shooting"or action=="Dunking"or action=="Stealing"then return end
  local height=candidate.position.Y-root.Position.Y
  local distance=flat(candidate.position-root.Position).Magnitude
  local sample=candidate.sample

  local stable=sample.fit~=nil and now-(sample.freeSince or now)>=.16 and now-sample.time<=.12
  local descending=sample.fallSince and now-sample.fallSince>=.06
  local planned=candidate.plan
  if sample.boardHitAt then requestBoardJump(candidate.ball,sample,now)end
  if sample.jumped then status="Jump requested: backboard contact"
  elseif not sample.boardHitAt then status="Tracking lob: waiting for backboard contact"
  elseif attr(own,"CanJump")==false then status="Backboard hit: game disallows jumping"
  else status="Backboard hit: native jump unavailable"end
  traceJump(now,"tracking",root,sample,candidate.point,planned)
  if (settings.steal or Input:IsKeyDown(Enum.KeyCode.R))and not sample.stolen and (sample.jumped or stable and descending and height<3) and distance<=3.5 and height>-.5 and height<4.5 and now-lastSteal>=(settings.stealCooldown or 1.4)then
   sample.stolen=true;lastSteal=now;status="Native catch attempt requested"
   pcall(function()if settings._requestLooseCatch then settings._requestLooseCatch()else service.Steal:Fire()end end)
   recordDefense("attempt_sent",{action="Lob intercept"})
  end
 end)
 local previous=featureCleanup
 featureCleanup=function()stopped=true;release();for _,sample in pairs(samples)do if sample.boardConnection then sample.boardConnection:Disconnect()end;if sample.touchConnection then sample.touchConnection:Disconnect()end end;candidate=nil;settings.oopDriving=false;settings._oopDirection=nil;settings._oopOwnsBall=nil;settings._oopFacePosition=nil;settings._oopFaceAt=nil;settings._oopActiveAt=nil;settings._oopJumpProfile=nil;marker:Destroy();guide:Destroy();for _,part in ipairs(arc)do part:Destroy()end;connection:Disconnect();previous()end
end)()end

-- Live visual rendering V2: presentation only.
do (function()
local visualModules={}
visualModules.Theme=(function()
return {
 background=Color3.fromRGB(12,15,21), panel=Color3.fromRGB(17,21,29),
 elevated=Color3.fromRGB(24,30,41), hover=Color3.fromRGB(31,39,53),
 border=Color3.fromRGB(42,51,67), text=Color3.fromRGB(228,235,246),
 muted=Color3.fromRGB(137,151,175), dim=Color3.fromRGB(88,103,127),
 accent=Color3.fromRGB(64,132,255), cyan=Color3.fromRGB(94,211,241),
 green=Color3.fromRGB(103,221,175), amber=Color3.fromRGB(244,187,106),
 red=Color3.fromRGB(241,115,133),
 palettes={
  {name="Cobalt",color=Color3.fromRGB(64,132,255)},
  {name="Glacier",color=Color3.fromRGB(94,211,241)},
  {name="Mint",color=Color3.fromRGB(103,221,175)},
  {name="Amber",color=Color3.fromRGB(244,187,106)},
 },
}

end)()
visualModules.Geometry=(function()
-- Rendering primitives for the locally generated WorldModel only.
local G={}
function G.make(class,properties,parent)
 local object=Instance.new(class)
 for key,value in pairs(properties or {})do object[key]=value end
 object.Parent=parent
 return object
end
function G.part(parent,name,size,color,alpha)
 return G.make("Part",{Name=name,Size=size,Color=color,Transparency=alpha or 0,
  Anchored=true,CanCollide=false,CanTouch=false,CanQuery=false,CastShadow=false,
  Material=Enum.Material.Neon},parent)
end
function G.line(parent,name,color)
 return G.part(parent,name,Vector3.new(.06,.06,.1),color,1)
end
function G.placeLine(part,a,b,width,color,alpha)
 local length=(b-a).Magnitude
 part.Transparency=length<.001 and 1 or alpha or 0
 if length<.001 then return end
 part.Size=Vector3.new(width,width,length)
 part.CFrame=CFrame.lookAt((a+b)*.5,b)
 if color then part.Color=color end
end
function G.ring(parent,name,count,color)
 local ring={parts={},count=count}
 for i=1,count do ring.parts[i]=G.line(parent,name.." "..i,color)end
 return ring
end
function G.drawRing(ring,center,radius,width,color,alpha,startAngle,sweep)
 local start=startAngle or 0;local span=sweep or math.pi*2
 for i,part in ipairs(ring.parts)do
  local a=start+span*(i-1)/ring.count;local b=start+span*i/ring.count
  G.placeLine(part,center+Vector3.new(math.cos(a)*radius,0,math.sin(a)*radius),
   center+Vector3.new(math.cos(b)*radius,0,math.sin(b)*radius),width,color,alpha)
 end
end
function G.disk(parent,name,color)
 local disk=G.part(parent,name,Vector3.new(.025,1,1),color,.9)
 disk.Shape=Enum.PartType.Cylinder
 return disk
end
function G.placeDisk(disk,point,radius,color,alpha)
 disk.Size=Vector3.new(.025,radius*2,radius*2)
 disk.CFrame=CFrame.new(point)*CFrame.Angles(0,0,math.pi/2)
 disk.Color=color;disk.Transparency=alpha
end
function G.sphere(parent,name,radius,color,alpha)
 local p=G.part(parent,name,Vector3.new(radius*2,radius*2,radius*2),color,alpha)
 p.Shape=Enum.PartType.Ball;return p
end
-- Upright pillar standing on `base` (used for the landing beacon and the apex drop line).
function G.vertical(part,base,height,width,color,alpha)
 part.Size=Vector3.new(width,math.max(height,.05),width)
 part.CFrame=CFrame.new(base+Vector3.new(0,math.max(height,.05)*.5,0))
 part.Color=color;part.Transparency=alpha
end
function G.dashes(parent,name,count,color)
 local result={}
 for i=1,count do result[i]=G.line(parent,name.." "..i,color)end
 return result
end
function G.drawDashes(parts,a,b,width,color,alpha)
 local delta=b-a
 if G.routeStyle==2 then
  G.placeLine(parts[1],a,b,width,color,alpha)
  for i=2,#parts do parts[i].Transparency=1 end
  return
 elseif G.routeStyle==3 and delta.Magnitude>.01 then
  local unit=delta/delta.Magnitude;local side=Vector3.new(-unit.Z,0,unit.X)
  local count=math.floor(#parts/2);local length=math.clamp(delta.Magnitude/math.max(count,1)*.25,.25,.6)
  for i=1,count do
   local point=a+delta*i/(count+1)
   G.placeLine(parts[i*2-1],point-unit*length+side*length*.6,point,width,color,alpha)
   G.placeLine(parts[i*2],point-unit*length-side*length*.6,point,width,color,alpha)
  end
  if #parts%2==1 then parts[#parts].Transparency=1 end
  return
 end
 for i,p in ipairs(parts)do
  G.placeLine(p,a+delta*((i-.82)/#parts),a+delta*((i-.18)/#parts),width,color,alpha)
 end
end
function G.flat(point,y)return Vector3.new(point.X,y or .07,point.Z)end
function G.quadratic(a,b,c,t)return a*(1-t)^2+b*(2*(1-t)*t)+c*t*t end
function G.drawCurve(parts,a,b,c,width,color,alpha)
 if G.routeStyle==3 then
  local count=math.floor(#parts/2)
  for i=1,count do
   local t=i/(count+1);local point=G.quadratic(a,b,c,t)
   local tangent=G.quadratic(a,b,c,math.min(1,t+.01))-G.quadratic(a,b,c,math.max(0,t-.01))
   local unit=tangent/math.max(.001,tangent.Magnitude);local side=Vector3.new(-unit.Z,0,unit.X)
   G.placeLine(parts[i*2-1],point-unit*.42+side*.23,point,width,color,alpha)
   G.placeLine(parts[i*2],point-unit*.42-side*.23,point,width,color,alpha)
  end
  if #parts%2==1 then parts[#parts].Transparency=1 end
  return
 end
 for i,p in ipairs(parts)do
  local start=G.routeStyle==2 and (i-1)/#parts or (i-.86)/#parts
  local finish=G.routeStyle==2 and i/#parts or (i-.22)/#parts
  G.placeLine(p,G.quadratic(a,b,c,start),G.quadratic(a,b,c,finish),width,color,alpha)
 end
end
function G.outline(parent,name,color)
 return G.dashes(parent,name,12,color)
end
function G.drawOutline(parts,center,size,color,alpha)
 local h=size*.5
 local p={}
 for _,x in ipairs({-1,1})do for _,y in ipairs({-1,1})do for _,z in ipairs({-1,1})do
  p[#p+1]=center+Vector3.new(x*h.X,y*h.Y,z*h.Z)
 end end end
 local index=0
 for i=1,8 do for j=i+1,8 do
  local delta=p[i]-p[j]
  local changes=(math.abs(delta.X)>.001 and 1 or 0)+(math.abs(delta.Y)>.001 and 1 or 0)+(math.abs(delta.Z)>.001 and 1 or 0)
  if changes==1 then index=index+1;G.placeLine(parts[index],p[i],p[j],.035,color,alpha)end
 end end
end
return G

end)()
visualModules.Effect=(function()
local G=visualModules.Geometry
local Effect={}
function Effect.new(context,name)
 local self={folder=G.make("Folder",{Name=name},context.world),labels={},visible=true,context=context,alphas={}}
 function self:Label(value)
  local label={text=value,position=Vector3.zero,color=context.theme.text,visible=true}
  self.labels[#self.labels+1]=label;return label
 end
 function self:SetVisible(value)
  if self.visible==value then return end
  self.visible=value
  -- Keep render objects mounted. Reparenting every frame tears down render proxies.
  for _,part in ipairs(self.folder:GetDescendants())do
   if part:IsA("BasePart")then
    if value then part.Transparency=self.alphas[part]or 1
    else self.alphas[part]=part.Transparency;part.Transparency=1 end
   end
  end
 end
 function self:Destroy()self.folder:Destroy();self.labels={}end
 return self
end
return Effect

end)()
visualModules.LandingZone=(function()
local G=visualModules.Geometry
local Effect=visualModules.Effect
local F={id="LandingZone",title="Landing zone",section="COURT EFFECTS",subtitle="A precise center with a soft, layered perimeter.",color="cyan"}
function F.new(ctx)
 local self=Effect.new(ctx,F.id)
 -- Soft filled disc (three stacked layers), crisp edge, halo, closing timer ring, beacon pillar.
 local fills={}
 for i=1,3 do fills[i]=G.disk(self.folder,"Landing fill "..i,ctx.theme.cyan)end
 local edge=G.ring(self.folder,"Crisp perimeter",48,ctx.theme.cyan)
 local glow=G.ring(self.folder,"Soft outer halo",48,ctx.theme.cyan)
 local pulse=G.ring(self.folder,"Expanding arrival pulse",48,ctx.theme.cyan)
 local closing=G.ring(self.folder,"Closing timer ring",48,ctx.theme.cyan)
 local ticks=G.dashes(self.folder,"Cardinal tick",4,ctx.theme.cyan)
 local beaconCore=G.part(self.folder,"Landing beacon core",Vector3.new(.05,1,.05),ctx.theme.cyan,1)
 local beaconHalo=G.part(self.folder,"Landing beacon halo",Vector3.new(.2,1,.2),ctx.theme.cyan,1)
 local label=self:Label("LANDING ZONE")
 local flightLongest,lastRemaining=0,0
 function self:Update(s,t,o)
  local remaining=math.max(0,(1-s.phase)*ctx.math.duration)
  -- A jump upward in remaining time means a new flight; restart the timer ring.
  if flightLongest<=0 or remaining>lastRemaining+.25 then flightLongest=remaining else flightLongest=math.max(flightLongest,remaining)end
  lastRemaining=remaining
  local progress=flightLongest>.05 and math.clamp(1-remaining/flightLongest,0,1) or 1
  local urgency=o.urgency and math.clamp(1-remaining/1.1,0,1) or 0
  local color=o.urgency and o.color:Lerp(ctx.theme.amber,urgency) or o.color
  local center=G.flat(s.landing,.06);local r=1.55*o.scale;local a=1-o.intensity
  local radii={.35,.65,.92}
  for i,disk in ipairs(fills)do
   G.placeDisk(disk,center+Vector3.new(0,(i-1)*.004,0),r*radii[i],color,math.clamp(.9-urgency*.1+(i-1)*.025+a*.05,0,1))
  end
  G.drawRing(edge,center+Vector3.new(0,.025,0),r,.075*o.weight,color,.16+a*.65)
  G.drawRing(glow,center+Vector3.new(0,.012,0),r+.2,.21*o.weight,color,.88+a*.1)
  local cycle=(t*.5*o.motion)%1
  G.drawRing(pulse,center+Vector3.new(0,.035,0),r+.35+cycle*.6,.055*o.weight,color,o.motion<=0 and 1 or math.clamp(.8+cycle*.2+a*.15,0,1))
  -- Timer ring: starts wide and tightens onto the zone as the ball arrives.
  if o.urgency and progress<.985 then
   G.drawRing(closing,center+Vector3.new(0,.045,0),r+(1-progress)*r*1.6,.05*o.weight,color,.3+a*.4)
  else G.drawRing(closing,center,r,.05,color,1)end
  for i,part in ipairs(ticks)do
   local angle=(i-1)*math.pi*.5;local unit=Vector3.new(math.cos(angle),0,math.sin(angle))
   G.placeLine(part,center+unit*(r+.32),center+unit*(r+.61),.065*o.weight,color,.12+a*.6)
  end
  if o.beacon then
   G.vertical(beaconCore,center,6*o.scale,.05*o.weight,color,.35+a*.4)
   G.vertical(beaconHalo,center,6*o.scale,.2*o.weight,color,.86+a*.1)
  else beaconCore.Transparency=1;beaconHalo.Transparency=1 end
  label.position=center+Vector3.new(0,.15,2.4);label.color=color
  label.text=string.format("LANDING  /  %.1fs",remaining)
 end
 return self
end
return F

end)()
visualModules.BallFlight=(function()
local G=visualModules.Geometry
local Effect=visualModules.Effect
local F={id="BallFlight",title="Flight trail",section="COURT EFFECTS",subtitle="A tapered arc, fading tail and restrained ball halo.",color="cyan"}
function F.new(ctx)
 local self=Effect.new(ctx,F.id)
 local arc=G.dashes(self.folder,"Future trajectory",48,ctx.theme.cyan)
 local shadow=G.dashes(self.folder,"Floor shadow",24,ctx.theme.cyan)
 local drop=G.part(self.folder,"Apex drop line",Vector3.new(.03,1,.03),ctx.theme.cyan,1)
 local apex=G.sphere(self.folder,"Apex marker",.17,ctx.theme.cyan,1)
 local tail=G.dashes(self.folder,"Motion tail",16,ctx.theme.cyan)
 local halo=G.sphere(self.folder,"Ball halo",.59,ctx.theme.cyan,.82)
 function self:Update(s,t,o)
  -- Predicted path: tapers and strengthens toward the landing point.
  for i,part in ipairs(arc)do
   local a=(i-1)/#arc;local b=i/#arc
   G.placeLine(part,ctx.math.position(a),ctx.math.position(b),.035*(1.3-a*.6)*o.weight,o.color,math.clamp(.62-a*.3+(1-o.intensity)*.2,0,1))
  end
  -- Floor shadow of the path and the highest point, for depth perception.
  local hideApex=true
  if o.flightShadow then
   for i,part in ipairs(shadow)do
    local a=(i-1)/#shadow;local b=i/#shadow
    G.placeLine(part,G.flat(ctx.math.position(a),.02),G.flat(ctx.math.position(b),.02),.04*o.weight,o.color,.72+(1-o.intensity)*.2)
   end
   local bestY,bestIndex,bestPoint=-math.huge,0,nil
   for i=0,24 do
    local p=ctx.math.position(i/24)
    if p.Y>bestY then bestY=p.Y;bestIndex=i;bestPoint=p end
   end
   if bestPoint and bestIndex>0 and bestIndex<24 then
    local ground=G.flat(bestPoint,.02)
    local height=bestPoint.Y-ground.Y
    if height>.6 then
     hideApex=false
     G.vertical(drop,ground,height,.025*o.weight,o.color,.8)
     apex.Position=bestPoint;apex.Size=Vector3.one*(.34*o.scale);apex.Color=o.color;apex.Transparency=.45
    end
   end
  else for _,part in ipairs(shadow)do part.Transparency=1 end end
  if hideApex then drop.Transparency=1;apex.Transparency=1 end
  -- Motion trail from the ball's recent real positions (oldest fades out).
  local trail=s.trail
  for i,part in ipairs(tail)do
   local newer=o.comet and trail and trail[#trail-(i-1)]
   local older=o.comet and trail and trail[#trail-i]
   if newer and older then
    local fade=i/#tail
    G.placeLine(part,older,newer,.11*(1-fade*.75)*o.weight,o.color,math.clamp(.25+fade*.65+(1-o.intensity)*.3,0,1))
   else part.Transparency=1 end
  end
  halo.Position=s.ball;halo.Size=Vector3.one*(1.18*o.scale);halo.Color=o.color;halo.Transparency=.86+(1-o.intensity)*.12
 end
 return self
end
return F

end)()
visualModules.GuardRoute=(function()
local G=visualModules.Geometry
local Effect=visualModules.Effect
local F={id="GuardRoute",title="Positioning route",section="ROUTES",subtitle="A curved ground route with a clean destination reticle.",color="accent"}
function F.new(ctx)
 local self=Effect.new(ctx,F.id)
 local route=G.dashes(self.folder,"Curved route",22,ctx.theme.accent)
 local endRing=G.ring(self.folder,"Destination reticle",40,ctx.theme.accent)
 local assignment=G.dashes(self.folder,"Assignment guide",7,ctx.theme.accent)
 local arrow=G.dashes(self.folder,"Route arrowhead",2,ctx.theme.accent)
 local label=self:Label("DESTINATION")
 function self:Update(s,t,o)
  local a=G.flat(s.passer);local b=G.flat(s.guardPoint);local control=(a+b)*.5+Vector3.new(-3,0,0)
  G.drawCurve(route,a,control,b,.065*o.weight,o.color,.25+(1-o.intensity)*.55)
  -- Arrowhead on the reticle edge, pointing along the end of the route.
  local heading=Vector3.new(b.X-control.X,0,b.Z-control.Z)
  if heading.Magnitude>.2 then
   local u=heading.Unit;local side=Vector3.new(-u.Z,0,u.X)
   local tip=b-u*(1.05*o.scale);local wing=.55*o.scale
   local arrowAlpha=.2+(1-o.intensity)*.5
   G.placeLine(arrow[1],tip-u*wing+side*wing*.7,tip,.06*o.weight,o.color,arrowAlpha)
   G.placeLine(arrow[2],tip-u*wing-side*wing*.7,tip,.06*o.weight,o.color,arrowAlpha)
  else arrow[1].Transparency=1;arrow[2].Transparency=1 end
  G.drawRing(endRing,b,1*o.scale,.06*o.weight,o.color,.1+(1-o.intensity)*.65,t*.12*o.motion,math.pi*1.7)
  G.drawDashes(assignment,b,G.flat(s.handler),.032*o.weight,o.color,.62)
  label.position=b+Vector3.new(0,.2,1.7);label.color=o.color
 end
 return self
end
return F

end)()
visualModules.Contest=(function()
local G=visualModules.Geometry
local Effect=visualModules.Effect
local F={id="Contest",title="Coverage display",section="PLAYER MARKERS",subtitle="A floor arc and compact percentage, without overhead clutter.",color="accent"}
function F.new(ctx)
 local self=Effect.new(ctx,F.id)
 local base=G.ring(self.folder,"Coverage track",48,ctx.theme.dim)
 local value=G.ring(self.folder,"Coverage amount",32,ctx.theme.accent)
 local outline=G.outline(self.folder,"Player contour",ctx.theme.accent)
 local label=self:Label("32%  COVERAGE")
 function self:Update(s,t,o)
  local point=G.flat(s.handler);local ratio=s.contest/100;local r=1.55*o.scale
  G.drawRing(base,point,r,.035*o.weight,ctx.theme.dim,.65)
  G.drawRing(value,point+Vector3.new(0,.015,0),r,.085*o.weight,o.color,.13+(1-o.intensity)*.65,math.pi*.25,math.max(.03,ratio*math.pi*2))
  G.drawOutline(outline,s.handler+Vector3.new(0,2.5,0),Vector3.new(2.7,5.1,1.5),o.color,.65+(1-o.intensity)*.3)
  label.text=string.format("%d%%  COVERAGE",s.contest);label.position=point+Vector3.new(0,.15,2.4);label.color=o.color
 end
 return self
end
return F

end)()
visualModules.BallRecovery=(function()
local G=visualModules.Geometry
local Effect=visualModules.Effect
local F={id="BallRecovery",title="Recovery marker",section="COURT EFFECTS",subtitle="A loose-ball bracket, soft radius and ground-level approach line.",color="amber"}
function F.new(ctx)
 local self=Effect.new(ctx,F.id)
 local inner=G.ring(self.folder,"Ball bracket",32,ctx.theme.amber)
 local outer=G.ring(self.folder,"Recovery perimeter",48,ctx.theme.amber)
 local route=G.dashes(self.folder,"Recovery route",16,ctx.theme.amber)
 local label=self:Label("LOOSE BALL")
 function self:Update(s,t,o)
  local center=G.flat(s.loose)
  G.drawRing(inner,center,.75*o.scale,.065*o.weight,o.color,.15+(1-o.intensity)*.65,t*.25*o.motion,math.pi*1.6)
  G.drawRing(outer,center,2.1*o.scale,.035*o.weight,o.color,.72+(1-o.intensity)*.22)
  G.drawDashes(route,G.flat(s.passer),center,.04*o.weight,o.color,.55+(1-o.intensity)*.35)
  label.position=center+Vector3.new(0,.15,2.7);label.color=o.color
 end
 return self
end
return F

end)()
visualModules.ScreenAlert=(function()
local G=visualModules.Geometry
local Effect=visualModules.Effect
local F={id="ScreenAlert",title="Screen warning",section="PLAYER MARKERS",subtitle="An amber danger sector and a quieter bypass guide.",color="amber"}
function F.new(ctx)
 local self=Effect.new(ctx,F.id)
 local zone=G.ring(self.folder,"Warning sector",40,ctx.theme.amber)
 local outline=G.outline(self.folder,"Screener contour",ctx.theme.amber)
 local path=G.dashes(self.folder,"Bypass path",20,ctx.theme.amber)
 local label=self:Label("SCREEN  /  LEFT SIDE")
 function self:Update(s,t,o)
  local center=G.flat(s.screen)
  G.drawRing(zone,center,2.5*o.scale,.07*o.weight,o.color,.3+(1-o.intensity)*.5,0,math.pi*1.25)
  G.drawOutline(outline,s.screen+Vector3.new(0,2.5,0),Vector3.new(2.7,5.1,1.5),o.color,.65+(1-o.intensity)*.3)
  local a=G.flat(s.passer);local b=center+Vector3.new(-4,0,0);local c=G.flat(s.guardPoint)
  G.drawCurve(path,a,b,c,.035*o.weight,o.color,.6)
  label.position=center+Vector3.new(0,.2,3.2);label.color=o.color
 end
 return self
end
return F

end)()
visualModules.LobIntercept=(function()
local G=visualModules.Geometry
local Effect=visualModules.Effect
local F={id="LobIntercept",title="Meeting-point marker",section="ROUTES",subtitle="A split reticle and a precise crosshair at the fictional meeting point.",color="cyan"}
function F.new(ctx)
 local self=Effect.new(ctx,F.id)
 local arcs={};for i=1,4 do arcs[i]=G.ring(self.folder,"Meeting reticle "..i,12,ctx.theme.cyan)end
 local cross=G.dashes(self.folder,"Crosshair",2,ctx.theme.cyan)
 local route=G.dashes(self.folder,"Meeting route",19,ctx.theme.cyan)
 local label=self:Label("MEETING POINT")
 function self:Update(s,t,o)
  local p=G.flat(s.landing,.13);local r=.95*o.scale
  for i,arc in ipairs(arcs)do G.drawRing(arc,p,r,.065*o.weight,o.color,.15+(1-o.intensity)*.6,(i-1)*math.pi*.5+t*.08*o.motion,math.pi*.34)end
  G.placeLine(cross[1],p+Vector3.new(-.22,0,0),p+Vector3.new(.22,0,0),.035*o.weight,o.color,.25)
  G.placeLine(cross[2],p+Vector3.new(0,0,-.22),p+Vector3.new(0,0,.22),.035*o.weight,o.color,.25)
  G.drawDashes(route,G.flat(s.passer),p,.035*o.weight,o.color,.55+(1-o.intensity)*.35)
  label.position=p+Vector3.new(0,.15,-2);label.color=o.color
 end
 return self
end
return F

end)()
-- Presentation adapter: consumes existing state; sends no gameplay input.
 local function installLiveVisuals()
  local folder=visualModules.Geometry.make("Folder",{Name="PracticalVisualStyleV2"},visualFolder)
  local G=visualModules.Geometry
  local originalFlat=G.flat
  local originalLine=G.placeLine
  G.placeLine=function(part,a,b,width,color,alpha)
   local camera=workspace.CurrentCamera
   if camera and camera.CFrame then
    local distance=(camera.CFrame.Position-(a+b)*.5).Magnitude
    width=math.max(width,math.clamp(distance*.0018,.045,.14)*(settings.visualWeight or 1))
   end
   originalLine(part,a,b,width,color,alpha)
  end
  local floorY;local floorAt=-math.huge
  G.flat=function(point,y)
   local height=floorY or floorPoint(point,model()).Y
   return Vector3.new(point.X,height+(settings.visualClearance or .12)+(y or .02),point.Z)
  end
  -- Live avatar Highlights already provide exact silhouettes; preview boxes stay hidden.
  G.drawOutline=function()end
  local theme=visualModules.Theme
  local flight
  local context={world=folder,theme=theme,math={duration=0}}
  context.math.position=function(phase)
   if not flight then return Vector3.zero end
   local t=math.clamp(phase,0,1)*flight.time
   return flight.position+flight.velocity*t-Vector3.new(0,.5*(flight.gravity or workspace.Gravity)*t*t,0)
  end
  local names={"LandingZone","BallFlight","GuardRoute","Contest","BallRecovery","ScreenAlert","LobIntercept"}
  local effects={}
  for _,name in ipairs(names)do effects[name]=visualModules[name].new(context);effects[name]:SetVisible(false)end
  local labelsGui=create("ScreenGui",{Name="PracticalVisualLabelsV2",ResetOnSpawn=false},P.PlayerGui)
  local labelObjects={}
  for name,effect in pairs(effects)do
   labelObjects[name]={}
   for i in ipairs(effect.labels)do
    local board=create("BillboardGui",{Name=name.."FloorLabel",Size=UDim2.fromOffset(170,25),AlwaysOnTop=true,Enabled=false,MaxDistance=70},labelsGui)
    local text=create("TextLabel",{Size=UDim2.fromScale(1,1),BackgroundColor3=theme.background,BackgroundTransparency=.16,BorderSizePixel=0,Font=Enum.Font.GothamMedium,TextSize=10,Text=""},board)
    create("UICorner",{CornerRadius=UDim.new(0,5)},text)
    local stroke=create("UIStroke",{Color=theme.border,Transparency=.4},text)
    labelObjects[name][i]={board=board,text=text,stroke=stroke}
   end
  end
  local lanes={}
  for i=1,10 do lanes[i]=G.dashes(folder,"Passing lane "..i,12,theme.green)end
  local stopped=false;local failure;local last=0;local cache={};local dt=1/60
  settings.visualPolish=true;settings.visualLabels=true;settings.visualMotion=false
  settings.visualScale=1;settings.visualIntensity=65;settings.visualWeight=1.15
  settings.visualSmoothing=55;settings.visualClearance=.12;settings.visualPalette=1
  settings.visualRouteStyle=1;settings.visualCustomColor=false
  settings.visualRed=64;settings.visualGreen=132;settings.visualBlue=255
  settings.visualLabelSize=11;settings.visualLabelOpacity=80;settings.visualPassPanel=true
  settings.visualUrgency=true;settings.visualBeacon=true;settings.visualFlightShadow=true;settings.visualComet=true
  local trail,trailAt={},-math.huge
  -- Smooth blue -> amber -> red ramp instead of two hard colour steps.
  local function contestColor(p)
   p=math.clamp(p,0,100)
   if p<=35 then return theme.accent:Lerp(theme.amber,p/35)end
   return theme.amber:Lerp(theme.red,math.clamp((p-35)/30,0,1))
  end
  for _,key in ipairs({"Landing","Flight","Route","Coverage","Pass","Recovery","Screen","Intercept"})do settings["visualShow"..key]=true end
  numericLimits.visualScale={.6,1.6};numericLimits.visualIntensity={10,100};numericLimits.visualWeight={.5,2}
  numericLimits.visualSmoothing={0,100};numericLimits.visualClearance={.04,.35};numericLimits.visualPalette={1,4}
  numericLimits.visualRouteStyle={1,3};numericLimits.visualRed={0,255};numericLimits.visualGreen={0,255};numericLimits.visualBlue={0,255}
  numericLimits.visualLabelSize={9,16};numericLimits.visualLabelOpacity={0,95}
  local function smooth(key,value)
   local previous=cache[key]
   local amount=settings.visualSmoothing or 55
   local alpha=amount<=0 and 1 or 1-math.exp(-60/(1+amount*.04)*dt)
   local result=previous and (value-previous).Magnitude<15 and previous:Lerp(value,alpha)or value
   cache[key]=result;return result
  end
  local function hide()
   for name,effect in pairs(effects)do
    effect:SetVisible(false)
    for _,object in ipairs(labelObjects[name])do object.board.Enabled=false end
   end
   for _,path in ipairs(lanes)do for _,part in ipairs(path)do part.Transparency=1 end end
  end
  local function show(name,visible,state,now,options,root)
   local effect=effects[name];effect:SetVisible(visible==true)
   if not visible then for _,object in ipairs(labelObjects[name])do if object.board.Enabled then object.board.Enabled=false end end;return end
   effect:Update(state,now,options)
   if name=="GuardRoute"and effect.labels[1]then
    effect.labels[1].text=settings.denyReceiver and settings._denyReceiverTarget and ("DENY / "..settings._denyReceiverTarget.Name)or "DESTINATION"
   end
   for i,label in ipairs(effect.labels)do
    local object=labelObjects[name][i]
    -- Keep the contest visual at the feet; no text over the local player's head.
    -- Floor label cards were removed (landing time, DESTINATION, LOOSE BALL, ...).
    local enabled=false
    if object.board.Enabled~=(enabled==true)then object.board.Enabled=enabled==true end
    if enabled then
     object.board.Adornee=root
     object.board.StudsOffsetWorldSpace=label.position-root.Position
     object.text.Text=label.text;object.text.TextColor3=label.color
     object.text.TextSize=settings.visualLabelSize or 11
     -- Pill hugs its text and its outline follows the effect colour.
     local size=settings.visualLabelSize or 11
     object.board.Size=UDim2.fromOffset(math.clamp(#label.text*size*.62+24,80,280),size+14)
     object.stroke.Color=label.color;object.stroke.Transparency=.5
     object.text.BackgroundTransparency=1-(settings.visualLabelOpacity or 80)/100
    end
   end
  end
  local function draw(now)
   settings._visualV2=settings.visualPolish and not failure
   if not settings._visualV2 then hide();return end
   local own=model();local root=rootOf(own)
   if not root then hide();cache={};floorY=nil;return end
   if now-floorAt>.12 then
    local ground=floorPoint(root.Position,own)
    -- Exclude balls and avatars so a held/flying ball cannot become the floor.
    pcall(function()
     local params=RaycastParams.new();params.FilterType=Enum.RaycastFilterType.Exclude
     local excluded={visualFolder,own}
     for _,name in ipairs({"Characters","ProxyCharacter","Basketballs","PracticalCourtTools"})do local object=workspace:FindFirstChild(name);if object then excluded[#excluded+1]=object end end
     params.FilterDescendantsInstances=excluded
     params.RespectCanCollide=true
     local lowest
     for _,offset in ipairs({Vector3.zero,Vector3.new(2,0,0),Vector3.new(-2,0,0),Vector3.new(0,0,2),Vector3.new(0,0,-2)})do
      local hit=workspace:Raycast(root.Position+offset+Vector3.new(0,2,0),Vector3.new(0,-32,0),params)
      if hit and (not lowest or hit.Position.Y<lowest.Y)then lowest=hit.Position end
     end
     if lowest then ground=lowest+Vector3.new(0,.06,0)end
    end)
    if not floorY or math.abs(ground.Y-floorY)>.5 then floorY=ground.Y end
    floorAt=now
   end
   local predicted=landingPrediction
   if predicted then cache.prediction=table.clone(predicted);cache.predictedAt=now
   elseif settings.landing and cache.predictedAt and now-cache.predictedAt<.12 then predicted=cache.prediction end
   local actualBall=goalPosition(trackedBall)or ballPosition
   local palette=theme.palettes[math.clamp(math.floor(settings.visualPalette or 1),1,4)].color
   if settings.visualCustomColor then palette=Color3.fromRGB(settings.visualRed,settings.visualGreen,settings.visualBlue)end
   G.routeStyle=math.clamp(math.floor(settings.visualRouteStyle or 1),1,3)
   local state={phase=0,ball=actualBall or root.Position,landing=predicted and smooth("landing",predicted.position)or root.Position,
    passer=G.flat(root.Position),handler=root.Position,guardPoint=guardAim and smooth("guardPoint",guardAim)or root.Position,
    screen=root.Position,loose=root.Position,receivers={},contest=settings.hudPressure or 0}
   local options={scale=settings.visualScale,intensity=settings.visualIntensity/100,weight=settings.visualWeight,
    motion=settings.visualMotion and 1 or 0,color=palette,urgency=settings.visualUrgency~=false,beacon=settings.visualBeacon~=false,
    flightShadow=settings.visualFlightShadow~=false,comet=settings.visualComet~=false}
   flight=predicted and actualBall and ballVelocity and {position=actualBall,velocity=smooth("velocity",ballVelocity),time=predicted.time}or nil
   context.math.duration=flight and flight.time or 0
   -- Recent real ball positions for the motion trail; cleared when the ball is gone or jumps.
   if flight and actualBall then
    local newest=trail[#trail]
    if newest and (actualBall-newest).Magnitude>12 then trail={};newest=nil end
    if not newest or (actualBall-newest).Magnitude>.12 then
     trail[#trail+1]=actualBall
     if #trail>17 then table.remove(trail,1)end
    end
    trailAt=now
   elseif now-trailAt>.25 and #trail>0 then trail={} end
   state.trail=trail
   local lob=api.GetAntiOopState and api.GetAntiOopState()
   if lob and lob.target then cache.lob=table.clone(lob);cache.lobAt=now
   elseif settings.antiOop and cache.lobAt and now-cache.lobAt<.1 then lob=cache.lob end
   local lobActive=settings.antiOop and lob and lob.target~=nil
   if lobActive and lob.flight then
    flight=lob.flight;state.ball=lob.ballPosition or flight.position;context.math.duration=flight.time
   end
   options.scale=(settings.landingZoneScale or 1)*settings.visualScale
   show("LandingZone",settings.visualShowLanding and settings.landing and settings.landingZone and predicted~=nil and not lobActive,state,now,options,root)
   options.scale=settings.visualScale;options.weight=settings.visualWeight*(settings.landingBeamWidth or .065)/.065
   local flightVisible=not lobActive and settings.landing and settings.landingBeam or lobActive and settings.oopGuide
   show("BallFlight",settings.visualShowFlight and flightVisible and flight~=nil,state,now,options,root)
   options.weight=settings.visualWeight;options.color=guardClosing and theme.amber or palette
   local otherRoot=rootOf(settings.denyReceiver and settings._denyReceiverTarget or target)
   if otherRoot then state.handler=floorPoint(otherRoot.Position,own)end
   local routeVisible=settings.visuals and guardEngaged and guardAim~=nil and not lobActive
   show("GuardRoute",settings.visualShowRoute and routeVisible,state,now,options,root)
   local pressure=settings.hudPressure
   state.handler=floorPoint(root.Position,own);state.contest=pressure or 0
   options.color=pressure and contestColor(pressure) or theme.accent
   show("Contest",settings.visualShowCoverage and settings.liveContest and pressure~=nil and now-(settings.hudPressureAt or 0)<.3,state,now,options,root)
   local characters=defenseCharacters();local byName={};local occupied={}
   for _,character in ipairs(characters)do
    byName[character.Name]=character
    local held=attr(character,"Basketball");if held then occupied[held]=true end
   end
   local tools=api.GetCourtToolsState and api.GetCourtToolsState()
   local screen=tools and tools.screen
   local screenRoot=screen and rootOf(byName[screen.player])
   options.color=theme.amber
   if screenRoot then state.screen=floorPoint(screenRoot.Position,own)end
   show("ScreenAlert",settings.visualShowScreen and settings.screenAwareness and screenRoot~=nil,state,now,options,root)
   local recoveryBall;local nearest=settings.recoveryRange or 12
   if settings.ballRecovery and not holdsBall(own)then
    local balls=workspace:FindFirstChild("Basketballs")
    if balls then for _,ball in ipairs(balls:GetChildren())do
     local position=goalPosition(ball)
     if position and not occupied[ball]and math.abs(position.Y-root.Position.Y)<6 then
      local distance=flat(position-root.Position).Magnitude
      if distance<nearest then recoveryBall=position;nearest=distance end
     end
    end end
   end
   if recoveryBall then state.loose=smooth("loose",recoveryBall)end
   show("BallRecovery",settings.visualShowRecovery and recoveryBall~=nil,state,now,options,root)
   if lobActive then state.landing=smooth("interception",lob.target)end
   options.color=palette
   show("LobIntercept",settings.visualShowIntercept and lobActive and settings.oopGuide,state,now,options,root)
   local activeLanes={}
   if settings.visualShowPass and settings.passingLanes and holdsBall(own)and tools then
    for i,lane in ipairs(tools.lanes or {})do
     local receiver=rootOf(byName[lane.player])
     if lanes[i]and receiver then
      activeLanes[i]=true
      G.drawDashes(lanes[i],G.flat(root.Position),G.flat(receiver.Position),.045*settings.visualWeight,lane.blocked and theme.amber or theme.green,.45+(1-options.intensity)*.4)
     end
    end
   end
   for i,path in ipairs(lanes)do if not activeLanes[i]then for _,part in ipairs(path)do if part.Transparency~=1 then part.Transparency=1 end end end end
  end
  local connection=(Run.RenderStepped or Run.Heartbeat):Connect(function()
   if stopped or failure then return end
   local now=os.clock();dt=math.clamp(now-last,.001,.05);last=now
   local ok,message=pcall(draw,now)
   if not ok then failure=tostring(message);settings._visualV2=false;hide();warn("Practical visual V2 fallback: "..failure)end
  end)
  settings._visualV2=true
  api.GetVisualGroundPoint=function(point)return G.flat(point)end
  api.DrawVisualRoute=function(parts,a,b,width,color,alpha)G.drawDashes(parts,a,b,width,color,alpha)end
  api.GetVisualStyleState=function()
   local active={};for name,effect in pairs(effects)do active[name]=effect.visible end
   return {enabled=settings._visualV2==true,error=failure,version=3,features=active}
  end
  local previous=featureCleanup
  featureCleanup=function()
   stopped=true;connection:Disconnect();settings._visualV2=false
   for _,effect in pairs(effects)do effect:Destroy()end
   labelsGui:Destroy();folder:Destroy();api.GetVisualGroundPoint=nil;api.DrawVisualRoute=nil;G.flat=originalFlat;G.placeLine=originalLine;previous()
  end
 end
 local visualOK,visualError=pcall(installLiveVisuals)
 if not visualOK then settings._visualV2=false;warn("Practical visual V2 setup failed: "..tostring(visualError))end

end)() end

-- Named dribble sequences and independent hotkeys, persisted separately from UI profiles.
if api then (function()
 local entries={}
 local path="PracticalSavedCombos-v1.json"
 local stopped=false
 local function validSequence(value)
  if type(value)~="string"or #value==0 or #value>300 then return false end
  local count=0
  for piece in (value..">"):gmatch("(.-)>")do
   local keys,delay=piece:match("^%s*([ZXCVWAIT]+)@([%d%.]+)%s*$")
   delay=tonumber(delay)
   if not keys or not delay or delay<.08 or delay>1.5 or not(keys=="WAIT"or #keys<=4 and keys:match("^[ZXCV]+$"))then return false end
   count=count+1
  end
  return count>0 and count<=12
 end
 local function usable(key)
  if key==(settings.smartDribbleKey or Enum.KeyCode.H)and(settings.smartDribble or settings.ballProtection)then return false end
  if not bindingValid(key) then return false end
  for _,reserved in ipairs({Enum.KeyCode.W,Enum.KeyCode.A,Enum.KeyCode.S,Enum.KeyCode.D,Enum.KeyCode.Z,Enum.KeyCode.X,Enum.KeyCode.C,Enum.KeyCode.V,Enum.KeyCode.Space,Enum.KeyCode.RightShift,Enum.KeyCode.LeftShift,Enum.KeyCode.R})do if key==reserved then return false end end
  return key~=guardKey and key~=(settings.comboPlayKey or Enum.KeyCode.J)and not(api.IsUIReservedKey and api.IsUIReservedKey(key))
 end
 local function persist()
  if type(writefile)~="function"then return false,"Saved for this session only; file saving unavailable"end
  local serialized={}
  for name,e in pairs(entries)do serialized[name]={sequence=e.sequence,repeats=e.repeats,sprint=e.sprint,key=e.key.Name}end
  local ok=pcall(function()writefile(path,Http:JSONEncode({version=1,combos=serialized}))end)
  return ok,ok and "Combo saved"or "Saved in session; could not write combo file"
 end
 api.GetNamedCombos=function()
  local list={}for name,e in pairs(entries)do local item=table.clone(e);item.name=name;list[#list+1]=item end
  table.sort(list,function(a,b)return a.name:lower()<b.name:lower()end);return list
 end
 api.SaveNamedCombo=function(name,key)
  if type(name)~="string"then return false,"Enter a combo name"end
  name=name:match("^%s*(.-)%s*$")
  if #name<1 or #name>48 then return false,"Use a name between 1 and 48 characters"end
  if not usable(key)then return false,"Choose a key other than movement, dribble, guard, R or menu keys"end
  for other,e in pairs(entries)do if other~=name and e.key==key then return false,"That key belongs to "..other end end
  local count=0;for _ in pairs(entries)do count=count+1 end
  if not entries[name]and count>=40 then return false,"Maximum 40 saved combos"end
  local value=api.GetComboSequence()
  if not validSequence(value)then return false,"Add valid moves before saving"end
  entries[name]={sequence=value,repeats=settings.comboRepeats,sprint=settings.comboSprint,key=key}
  local _,message=persist();return true,message.." · "..name.." ["..key.Name.."]"
 end
 api.DeleteNamedCombo=function(name)
  if not entries[name]then return false,"Choose a saved combo first"end
  entries[name]=nil;persist();return true,"Saved combo removed"
 end
 api.LoadNamedCombo=function(name)
  local e=entries[name];if not e then return false,"Choose a saved combo first"end
  local ok,message=api.SetComboSequence(e.sequence);if not ok then return false,message end
  settings.comboRepeats=e.repeats;settings.comboSprint=e.sprint
  return true,table.clone(e)
 end
 if type(readfile)=="function"then
  local ok,data=pcall(function()return Http:JSONDecode(readfile(path))end)
  if ok and type(data)=="table"and data.version==1 and type(data.combos)=="table"then
   local count=0
   for name,e in pairs(data.combos)do
    if count>=40 then break end
    if type(name)=="string"and #name>0 and #name<=48 and type(e)=="table"and validSequence(e.sequence)and type(e.key)=="string"then
     local keyOk,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()return Enum.KeyCode[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end)
     if keyOk and usable(key)then
      local repeats=type(e.repeats)=="number"and e.repeats==e.repeats and math.clamp(math.floor(e.repeats),1,4)or 1
      entries[name]={sequence=e.sequence,repeats=repeats,sprint=e.sprint==true,key=key};count=count+1
     end
    end
   end
  end
 end
 local connection=Input.InputBegan:Connect(function(input,processed)
  if stopped or processed or Input:GetFocusedTextBox()or not settings.comboBuilder or api.IsUICapturingInput and api.IsUICapturingInput()then return end
  for name,e in pairs(entries)do
   if bindingMatches(input,e.key) and usable(e.key)then
    local ok,message=api.PlayCombo(e)
    if not ok then assistState=message else assistState="Playing "..name end
    return
   end
  end
 end)
 local previous=featureCleanup
 featureCleanup=function()stopped=true;connection:Disconnect();previous()end
end)()end



-- Smart pass: choose a native gesture for the user's number-key receiver.
do (function()
 settings.smartPass=false;settings.smartPassSpin=false
 local styles={"Overhead","Chest","Bounce","Spin"}
 local state={status="Waiting for native pass controller",passes=0,slots={}}
 local slots={};local hook;local stopped=false
 local recentKey,recentAt=nil,0
 local emitting=false;local generation=0;local transaction=nil
 local keys={Enum.KeyCode.One,Enum.KeyCode.Two,Enum.KeyCode.Three,Enum.KeyCode.Four}
 local actionName="VantaSmartPassGesture"
 local CAS=game:GetService("ContextActionService")
 local VIM=game:GetService("VirtualInputManager")
 local function team(a,b)
  if not a or not b or a==b then return false end
  local x,y=attr(a,"Team"),attr(b,"Team")
  return x~=nil and y~=nil and x~=0 and x~=""and x==y
 end
 local function receiver(args,own)
  local found={};local seen={}
  local function visit(value,depth)
   if depth>5 then return end
   if typeof(value)=="Instance"then
    local character=value:IsA("Player")and value.Character or value
    if rootOf(character)and team(own,character)then found[character]=true end
   elseif type(value)=="string"then
    for _,character in ipairs(defenseCharacters())do if value==character.Name and team(own,character)then found[character]=true end end
   elseif type(value)=="table"and not seen[value]then
    seen[value]=true;for _,v in pairs(value)do visit(v,depth+1)end
   end
  end
  visit(args,0)
  local only
  for character in pairs(found)do if only then return nil end;only=character end
  return only
 end
 local function choose(own,target)
  local r=rootOf(own);local tr=rootOf(target)
  if not r or not tr then return nil end
  local origin=r.Position+Vector3.new(0,1,0)
  local distance=flat(tr.Position-r.Position).Magnitude
  local ground=r.Position.Y-2.5
  if api.GetVisualGroundPoint then local ok,p=pcall(api.GetVisualGroundPoint,r.Position);if ok and p then ground=p.Y end end
  local defenders={}
  local ownTeam=attr(own,"Team")
  for _,other in ipairs(defenseCharacters())do
   local dr=rootOf(other);local t=attr(other,"Team")
   local court,otherCourt=attr(own,"CourtNumber"),attr(other,"CourtNumber")
   if other~=own and dr and ownTeam~=nil and ownTeam~=0 and t~=nil and t~=0 and ownTeam~=t
    and(not court or not otherCourt or court==0 or otherCourt==0 or court==otherCourt)then
    defenders[#defenders+1]={position=dr.Position,velocity=dr.AssemblyLinearVelocity or Vector3.zero,
     high=attr(other,"HoldingG")==true or attr(other,"Action")=="Blocking",low=attr(other,"Action")=="Stealing"}
   end
  end
  local best,bestScore
  for _,name in ipairs(styles)do
   if name~="Spin"or settings.smartPassSpin then
    local speed=(settings.passSpeed or 80)*(name=="Chest"and 1.15 or name=="Bounce"and .85 or name=="Spin"and .95 or 1)
    local travel=distance/speed
    local destination=tr.Position+Vector3.new(0,1,0)+flat(tr.AssemblyLinearVelocity or Vector3.zero)*math.min(travel,.3)
    local risk=0
    for i=1,15 do
     local t=i/16;local point=origin+(destination-origin)*t
     if name=="Overhead"then point=point+Vector3.new(0,math.sin(math.pi*t)*4,0)
     elseif name=="Bounce"then
      local y=t<.5 and origin.Y+(ground+.45-origin.Y)*t*2 or ground+.45+(destination.Y-ground-.45)*(t-.5)*2
      point=Vector3.new(point.X,y,point.Z)
     end
     for _,d in ipairs(defenders)do
      local v=flat(d.velocity);if v.Magnitude>28 then v=v.Unit*28 end
      local pos=d.position+v*math.min(travel*t,.3)
      local height=point.Y-pos.Y
      if height>=(d.low and -3 or -1.8)and height<=(d.high and 6 or 3.8)then
       risk=math.max(risk,math.clamp(1-flat(point-pos).Magnitude/3,0,1))
      end
     end
    end
    local score=risk*10+travel*.8+(name=="Chest"and 0 or name=="Overhead"and .08 or name=="Spin"and .2 or .14)
    if not bestScore or score<bestScore then best=name;bestScore=score end
   end
  end
  return best,bestScore
 end

 local function passGesture(style)
  return {shift=style=="Chest"or style=="Spin",taps=(style=="Bounce"or style=="Spin")and 2 or 1}
 end
 -- Run only the native input sequence; no pass payload or receiver is fabricated.
 local function emitGesture(key,style,send,wait,cancelled)
  local gesture=passGesture(style)
  local modifier=false;local down=false
  local ok,err=pcall(function()
   if cancelled()then return end
   if gesture.shift then send(true,Enum.KeyCode.LeftShift);modifier=true end
   for i=1,gesture.taps do
    if cancelled()then break end
    send(true,key);down=true;wait(.025)
    send(false,key);down=false
    if i<gesture.taps then wait(.055)end
   end
  end)
  -- Always release our synthetic keys, including cancellation and input failures.
  if down then pcall(send,false,key)end
  if modifier then pcall(send,false,Enum.KeyCode.LeftShift)end
  return ok,err
 end
 local function numberHeld()
  if transaction and os.clock()<transaction.untilAt then return transaction.key end
  for _,key in ipairs(keys)do if Input:IsKeyDown(key)then return key end end
  if recentKey and os.clock()-recentAt<.4 then return recentKey end
 end
 local function bind()
  local service=keyboard and keyboard.Services and keyboard.Services.InputService
  local signal=service and service.Pass
  if not signal or type(signal.Fire)~="function"then state.status="Native pass controller unavailable";return end
  if hook and hook.signal==signal then return end
  if hook and hook.signal.Fire==hook.wrapper then hook.signal.Fire=hook.original end
  slots={};state.slots={}
  local original=signal.Fire
  local wrapper=function(self,...)
   if not stopped then
    local args=table.pack(...)
    pcall(function()
     local own=model();local target=own and receiver(args,own);local key=numberHeld()
     if target and key then
      slots[key]=target;state.slots[key.Name]=target.Name
      if transaction and os.clock()<transaction.untilAt and key==transaction.key then
       if not transaction.recorded then state.passes+=1;transaction.recorded=true end
       state.choice=transaction.style;state.target=target.Name
       state.status=transaction.style.." input sent to "..target.Name.." (native receiver)"
       if target~=transaction.target then state.status="Receiver slot changed: native receiver preserved; mapping refreshed"end
      else state.status="Receiver "..key.Name..": "..target.Name.." ready for Smart pass"end
     elseif settings.smartPass then state.status="Receiver unresolved: native pass preserved"end
    end)
   end
   return original(self,...)
  end
  local ok,err=pcall(function()signal.Fire=wrapper end)
  if ok then hook={signal=signal,original=original,wrapper=wrapper};state.status="Native gestures ready; receiver slots resolve from normal passes"
  else state.status="Pass observer unavailable: "..tostring(err)end
 end
 local held={}
 local function onNumber(_,inputState,event)
  if emitting then return Enum.ContextActionResult.Pass end
  if inputState~=Enum.UserInputState.Begin then
   if held[event.KeyCode]then
    if inputState==Enum.UserInputState.End or inputState==Enum.UserInputState.Cancel then held[event.KeyCode]=nil end
    return Enum.ContextActionResult.Sink
   end
   return Enum.ContextActionResult.Pass
  end
  recentKey=event.KeyCode;recentAt=os.clock()
  if stopped or not settings.smartPass or Input:GetFocusedTextBox()then return Enum.ContextActionResult.Pass end
  local own=model();if not own or not holdsBall(own)or not hook then return Enum.ContextActionResult.Pass end
  -- Explicit modified passes remain manual, including sprint-held Shift.
  if Input:IsKeyDown(Enum.KeyCode.LeftShift)or Input:IsKeyDown(Enum.KeyCode.RightShift)then
   state.status="Manual Shift pass preserved";return Enum.ContextActionResult.Pass
  end
  local target=slots[event.KeyCode]
  if not target or not target.Parent or not rootOf(target)or not team(own,target)then
   state.status="Receiver slot not identified: normal pass preserved"
   return Enum.ContextActionResult.Pass
  end
  if transaction and os.clock()<transaction.untilAt then return Enum.ContextActionResult.Sink end
  local style=choose(own,target)
  if not style then return Enum.ContextActionResult.Pass end
  held[event.KeyCode]=true;generation+=1
  local token=generation
  transaction={key=event.KeyCode,style=style,target=target,untilAt=os.clock()+.7}
  state.status="Selecting "..style.." for "..target.Name
  task.defer(function()
   if stopped or token~=generation or not settings.smartPass then transaction=nil;return end
   emitting=true
   local ok,err=emitGesture(event.KeyCode,style,function(down,key)
    VIM:SendKeyEvent(down,key,false,game)
   end,task.wait,function()return stopped or token~=generation or not settings.smartPass end)
   emitting=false
   if not ok then transaction=nil;state.status="Native key input failed: "..tostring(err)end
  end)
  return Enum.ContextActionResult.Sink
 end
 CAS:BindActionAtPriority(actionName,onNumber,false,10000,table.unpack(keys))
 local input=Input.InputBegan:Connect(function(event,processed)
  if emitting or Input:GetFocusedTextBox()then return end
  if table.find(keys,event.KeyCode)then recentKey=event.KeyCode;recentAt=os.clock()end
 end)
 api.GetSmartPassState=function()
  local out=table.clone(state);out.slots=table.clone(state.slots);return out
 end
 local lastBind=-math.huge;local rosterStamp
 local connection=Run.Heartbeat:Connect(function()
  if stopped or os.clock()-lastBind<.5 then return end;lastBind=os.clock()
  local own=model();local names={}
  if own then
   for _,character in ipairs(defenseCharacters())do if team(own,character)then names[#names+1]=character.Name end end
  end
  table.sort(names)
  local stamp=tostring(own)..":"..tostring(own and attr(own,"CourtNumber"))..":"..tostring(own and attr(own,"InGame"))..":"..table.concat(names,",")
  if rosterStamp~=stamp then slots={};state.slots={};rosterStamp=stamp end
  local ok,err=pcall(bind);if not ok then state.status="Pass binding unavailable: "..tostring(err)end
 end)
 local previous=featureCleanup
 featureCleanup=function()
  stopped=true;generation+=1;CAS:UnbindAction(actionName);input:Disconnect();connection:Disconnect()
  if hook then pcall(function()if hook.signal.Fire==hook.wrapper then hook.signal.Fire=hook.original end end)end
  previous()
 end
end)()end
-- Catch-and-shoot: one native shot after a confirmed teammate possession transfer.
do (function()
 settings.catchShoot=false;settings.catchShootContest=10
 numericLimits.catchShootContest={0,25}
 local owner,lastBall,initialized,pending,shot,openSince,early
 local seen,flight,buffered={},{},{}
 local status="Disabled"
 local earlyRequests,lastEarlyAt,lastLeadMs,bufferReason=0,nil,nil,"Waiting for a teammate pass"
 local function heldBall(character)
  local value=character and attr(character,"Basketball")
  return value~=nil and value~=false and value~=0 and value~=""and value or nil
 end
 local moving,hasMoved,stillSince,lastPosition,lastPositionAt,positionOwner=false,false
 local function updateMotion(own,root,now)
  if positionOwner~=own then positionOwner=own;lastPosition=nil;lastPositionAt=nil;hasMoved=false;stillSince=nil end
  local speed=flat(root.AssemblyLinearVelocity or Vector3.zero).Magnitude
  local proxy=attr(own,"ProxyCharacter")
  if typeof(proxy)=="Instance"then
   local proxyRoot=proxy:IsA("BasePart")and proxy or rootOf(proxy)
   if proxyRoot then speed=math.max(speed,flat(proxyRoot.AssemblyLinearVelocity or Vector3.zero).Magnitude)end
  end
  if lastPosition and now>(lastPositionAt or now)and now-lastPositionAt<.25 then speed=math.max(speed,flat(root.Position-lastPosition).Magnitude/(now-lastPositionAt))end
  lastPosition=root.Position;lastPositionAt=now
  moving=speed>1.25 or math.abs((root.AssemblyLinearVelocity or Vector3.zero).Y)>2
  for _,key in ipairs({Enum.KeyCode.W,Enum.KeyCode.A,Enum.KeyCode.S,Enum.KeyCode.D})do if Input:IsKeyDown(key)then moving=true end end
  if moving then hasMoved=true;stillSince=nil else stillSince=stillSince or now end
 end
 local function settled(now)return not moving and(not hasMoved or stillSince and now-stillSince>=.06)end
 local function release()
  if shot then pcall(function()shot.service.Shoot:Fire({Shoot=false})end)end
  if early then pcall(function()early.service.Shoot:Fire({Shoot=false})end)end
  shot=nil;early=nil;pending=nil;openSince=nil;settings._catchShootOwns=false
 end
 api.GetCatchShootState=function()return {status=status,pending=pending~=nil,shooting=shot~=nil,preholding=early~=nil,earlyRequests=earlyRequests,lastLeadMs=lastLeadMs,bufferReason=bufferReason}end
 local function releaseBudget(root,defenders,eta)
  if #defenders==0 then return -1 end
  local closeout=math.huge
  local reach=10*(1-settings.catchShootContest/100)
  for _,r in ipairs(defenders)do
   local delta=flat(root.Position-r.Position);local distance=delta.Magnitude
   local velocity=flat(r.AssemblyLinearVelocity or Vector3.zero)
   local toward=distance>.01 and velocity:Dot(delta.Unit)or 0
   local speed=math.clamp(math.max(16,toward),16,30)
   closeout=math.min(closeout,math.max(0,distance-reach)/speed+.12)
  end
  local releaseTime=math.clamp(settings.catchRelease or .5,.2,1)+math.clamp(ping or 0,0,.2)*.5+.06
  return closeout-releaseTime-(eta or 0)
 end
 local function opportunity(own,root,defenders,eta)
  if not settled(os.clock())then return false,"Waiting until standing still"end
  if not(api.GetAutoTime and api.GetAutoTime())then return false,"Enable Auto time"end
  if #defenders==0 then return false,"No reliable defender data"end
  local goal=goalPosition(attr(own,"Goal"))
  if not goal or flat(goal-root.Position).Magnitude>65 then return false,"Outside catch-and-shoot range"end
  if attr(own,"CanMove")==false then return false,"Movement locked"end
  local action=attr(own,"Action")
  if action=="Diving"or action=="Stealing"or action=="Blocking"or action=="Rebounding"or action=="Dunking"or action=="Stunned"then return false,"Busy: "..action end
  local contest=0
  for _,r in ipairs(defenders)do
   local velocity=flat(r.AssemblyLinearVelocity or Vector3.zero)
   if velocity.Magnitude>30 then velocity=velocity.Unit*30 end
   local gap=math.min(flat(r.Position-root.Position).Magnitude,flat(r.Position+velocity*((eta or 0)+.35)-root.Position).Magnitude)
   contest=math.max(contest,math.clamp(1-gap/10,0,1)*100)
  end
  if contest>settings.catchShootContest then return false,"Defender closing; holding shot"end
  if releaseBudget(root,defenders,eta)<=0 then return false,"Closeout would beat shot release"end
  for _,key in ipairs({Enum.KeyCode.W,Enum.KeyCode.A,Enum.KeyCode.S,Enum.KeyCode.D})do if Input:IsKeyDown(key)then return false,"Manual movement takes priority"end end
  return true
 end
 local function arrival(candidate,root,now)
  local position=goalPosition(candidate);if not position then return nil,"Ball position unavailable"end
  local previous=flight[candidate];local velocity
  if previous and now>previous.at and now-previous.at<.2 then velocity=(position-previous.position)/(now-previous.at)end
  flight[candidate]={position=position,at=now}
  if not velocity or velocity.Magnitude>160 then return nil,"Waiting for flight samples"end
  local horizontal=flat(velocity);local delta=flat(root.Position-position)
  if horizontal.Magnitude<5 or horizontal:Dot(delta)<=0 then return nil,"Pass not approaching you"end
  local eta=delta:Dot(horizontal)/math.max(horizontal:Dot(horizontal),.01)
  local window=math.clamp(.18+(ping or 0)*.5,.18,.3)
  if eta<0 or eta>window then return nil,"Incoming pass not in buffer range yet"end
  local point=position+velocity*eta
  if flat(point-root.Position).Magnitude>2.4 or point.Y-root.Position.Y<-.5 or point.Y-root.Position.Y>4 then return nil,"Pass misses reachable catch area"end
  if attr(candidate,"MidShot")==true or attr(candidate,"Lob")==true or attr(candidate,"SelfLob")==true then return nil,"Active shot or lob; not a pass buffer"end
  -- LastShotType can describe an earlier possession. Active flight flags,
  -- teammate ownership and the current trajectory determine eligibility.
  return eta
 end
 local connection=Run.Heartbeat:Connect(function()
  local now=os.clock();local own=model();local root=rootOf(own)
  local ball=heldBall(own)
  if not settings.catchShoot or not own or not root or Input:GetFocusedTextBox()or api.IsUICapturingInput and api.IsUICapturingInput()then release();initialized=false;seen={};flight={};buffered={};status="Disabled or unavailable";return end
  if owner~=own then release();owner=own;initialized=false;seen={};flight={};buffered={}end
  updateMotion(own,root,now)
  if early and not settled(now)then release();status="Movement cancelled early hold"end
  local team=attr(own,"Team");local defenders={};local occupied={}
  for _,other in ipairs(defenseCharacters())do
   local otherTeam=attr(other,"Team");local r=rootOf(other)
   local held=heldBall(other);if held then occupied[held]=other end
   local ownCourt,otherCourt=attr(own,"CourtNumber")or attr(own,"Court"),attr(other,"CourtNumber")or attr(other,"Court")
   local sameCourt=not(ownCourt~=nil and otherCourt~=nil and ownCourt~=0 and otherCourt~=0 and ownCourt~=otherCourt)
   if other~=own and r and sameCourt and math.abs(r.Position.Y-root.Position.Y)<10 and flat(r.Position-root.Position).Magnitude<100 and team~=nil and team~=0 and otherTeam~=nil and otherTeam~=0 then
    if otherTeam==team then
     if held then seen[held]=now;buffered[held]=nil;local p=goalPosition(held);if p then flight[held]={position=p,at=now}end end
    else defenders[#defenders+1]=r end
   end
  end
  for held,time in pairs(seen)do if now-time>2 then seen[held]=nil;flight[held]=nil;buffered[held]=nil end end
  if early and(early.ball~=ball and(occupied[early.ball]or now>early.untilTime))then release();status="Early hold cancelled"end
  if not ball and not early and not active and not pending then
   for candidate in pairs(seen)do
    if not occupied[candidate]and not buffered[candidate]then
     local eta,reason=arrival(candidate,root,now)
     bufferReason=reason or "Incoming pass detected"
     if eta then
      local ready,why=opportunity(own,root,defenders,eta)
      if not ready then bufferReason=why end
      local service=keyboard and keyboard.Services and keyboard.Services.InputService
      if ready and service and service.Shoot then
       local ok=pcall(function()service.Shoot:Fire({Shoot=true,Input="E"})end)
       if ok then early={ball=candidate,service=service,untilTime=now+eta+.25};buffered[candidate]=true;status="Early E input requested";bufferReason="E input sent before possession";earlyRequests=earlyRequests+1;lastEarlyAt=now end
       break
      end
     end
    end
   end
  end
  if early and not ball then
   local ready=opportunity(own,root,defenders,math.max(0,early.untilTime-now-.25))
   if not ready then release();status="Closeout cancelled early hold"end
  end
  if initialized and ball and ball~=lastBall and seen[ball]and now-seen[ball]<=2 then
   if early and early.ball==ball then
    lastLeadMs=lastEarlyAt and(now-lastEarlyAt)*1000 or nil
    shot={ball=ball,service=early.service,at=now,buffered=true,retryAt=now+.025,deadline=now+releaseBudget(root,defenders,0)};early=nil;pending=nil;status="Buffered catch-and-shoot"
   else pending={ball=ball,at=now,deadline=now+releaseBudget(root,defenders,0)};status="Checking catch window"end
   openSince=nil;seen[ball]=nil
  end
  initialized=true;lastBall=ball
  if shot then
   if shot.buffered and not settled(now)and not active and attr(own,"Action")~="Shooting"then
    local caught,deadline=shot.ball,shot.deadline;release();pending={ball=caught,at=now,deadline=deadline};status="Waiting until standing still"
   end
  end
  if shot then
   if not ball or ball~=shot.ball then release()
   elseif active and active.sent then shot=nil;settings._catchShootOwns=false
   elseif shot.buffered and not active and attr(own,"Action")~="Shooting"and(now>(shot.deadline or now)or releaseBudget(root,defenders,0)<=0)then
    release();status="Skipped: catch-and-shoot window closed"
   elseif shot.buffered and not shot.retried and now>=shot.retryAt and not active and attr(own,"Action")~="Shooting"and attr(own,"CanShoot")~=false then
    local ready=opportunity(own,root,defenders,0)
    if ready then pcall(function()shot.service.Shoot:Fire({Shoot=true,Input="E"})end);shot.retried=true;status="Catch confirmed; early input fallback"
    else release();status="Catch closed; cancelled buffered input"end
   elseif now-shot.at>2.2 then release();status="Shot timed out"end
   return
  end
  if not pending then settings._catchShootOwns=early~=nil;return end
  if active then release();status="Manual shot already active";return end
  if ball~=pending.ball or now-pending.at>3 then release();status="Catch window ended";return end
  local budget=releaseBudget(root,defenders,0)
  pending.deadline=math.min(pending.deadline or(now+budget),now+budget)
  if budget<=0 or now>pending.deadline then release();status="Skipped: closeout would beat release";return end
  settings._catchShootOwns=true
  if not settled(now)then status="Waiting until standing still";return end
  if not(api.GetAutoTime and api.GetAutoTime())then release();status="Enable Auto time";return end
  if #defenders==0 then openSince=nil;status="No reliable defender data";return end
  local goal=goalPosition(attr(own,"Goal"))
  if not goal or flat(goal-root.Position).Magnitude>65 then release();status="Outside catch-and-shoot range";return end
  local action=attr(own,"Action")
  if attr(own,"CanShoot")==false or attr(own,"CanMove")==false or(action and action~=""and action~="Idle"and action~="Catching"and action~="Receiving")then openSince=nil;return end
  local contest=0
  for _,r in ipairs(defenders)do
   local velocity=flat(r.AssemblyLinearVelocity or Vector3.zero)
   if velocity.Magnitude>30 then velocity=velocity.Unit*30 end
   local gap=math.min(flat(r.Position-root.Position).Magnitude,flat(r.Position+velocity*.35-root.Position).Magnitude)
   contest=math.max(contest,math.clamp(1-gap/10,0,1)*100)
  end
  if contest>settings.catchShootContest then release();status="Skipped: defender closed the catch window";return end
  for _,key in ipairs({Enum.KeyCode.W,Enum.KeyCode.A,Enum.KeyCode.S,Enum.KeyCode.D})do if Input:IsKeyDown(key)then openSince=nil;status="Manual movement takes priority";return end end
  -- The incoming-flight check prepares the decision before possession. No
  -- additional 100 ms confirmation pause is added after a confirmed catch.
  local service=keyboard and keyboard.Services and keyboard.Services.InputService
  if not service or not service.Shoot then release();status="Shot controller unavailable";return end
  local ok=pcall(function()service.Shoot:Fire({Shoot=true,Input="E"})end)
  if ok then shot={service=service,ball=ball,at=now};pending=nil;status="Catch-and-shoot requested"
  else release();status="Shot request failed"end
 end)
 local blurred=Input.WindowFocusReleased:Connect(release)
 local manual=Input.InputBegan:Connect(function(input,processed)
  if not processed and(pending or early)and(input.KeyCode==Enum.KeyCode.E or input.KeyCode==Enum.KeyCode.Space or input.KeyCode==Enum.KeyCode.Z or input.KeyCode==Enum.KeyCode.X or input.KeyCode==Enum.KeyCode.C or input.KeyCode==Enum.KeyCode.V)then
   if input.KeyCode==Enum.KeyCode.E then early=nil;pending=nil;settings._catchShootOwns=false else release()end
   openSince=nil;status="Manual input takes priority"
  end
 end)
 local previous=featureCleanup
 featureCleanup=function()release();connection:Disconnect();blurred:Disconnect();manual:Disconnect();previous()end
end)()end
-- END CATCH SHOOT

-- Contextual dribble selection using measured idle-input profiles.
do (function()
-- Measured idle-input profiles from the supplied capture; no mirrored or invented moves.
local measuredDribbleProfiles={
 {keys="Z",sprint=false,hand="R",handAfter="L",forward=0.0,right=-1.38,duration=0.85,package="Crossover 1",count=1},
 {keys="C",sprint=false,hand="L",handAfter="R",forward=-0.003,right=1.38,duration=0.85,package="Crossover 1",count=1},
 {keys="X",sprint=false,hand="R",handAfter="R",forward=-5.619,right=0.015,duration=0.866,package="StepBack 1",count=1},
 {keys="CX",sprint=false,hand="R",handAfter="L",forward=-0.003,right=-1.259,duration=0.851,package="BehindBack 1",count=1},
 {keys="Z",sprint=true,hand="R",handAfter="L",forward=0.798,right=-6.968,duration=0.865,package="Hang Cross",count=1},
 {keys="XC",sprint=true,hand="L",handAfter="L",forward=2.316,right=-4.853,duration=0.866,package="DoubleBehindBack 1",count=1},
 {keys="ZX",sprint=true,hand="L",handAfter="R",forward=1.883,right=9.112,duration=0.85,package="BehindBack 1",count=1},
 {keys="XZ",sprint=true,hand="R",handAfter="R",forward=2.028,right=4.249,duration=0.866,package="DoubleBehindBack 1",count=1},
 {keys="CXZ",sprint=true,hand="R",handAfter="L",forward=2.752,right=-5.017,duration=0.515,package="Spin 1",count=1},
 {keys="C",sprint=true,hand="L",handAfter="R",forward=1.478,right=8.405,duration=0.868,package="Hang Cross",count=1},
 {keys="ZXC",sprint=true,hand="L",handAfter="R",forward=2.974,right=2.336,duration=0.515,package="Spin 1",count=1},
}

-- BEGIN FARM DRIBBLE SELECTOR
-- Farm owns execution and movement. Share the measured moves without enabling
-- the independent hold-key controller. Profiles were captured with idle input.
local function farmDribble(context)
 local origin=context.origin;local route=flat(context.target-origin)
 if route.Magnitude<.1 then return nil end
 local velocity=flat(context.velocity or Vector3.zero)
 if velocity.Magnitude>28 then velocity=velocity.Unit*28 end
 local nearest=30
 for _,d in ipairs(context.defenders)do nearest=math.min(nearest,flat(d.position-origin).Magnitude)end
 local best,bestScore
 for _,p in ipairs(measuredDribbleProfiles)do
  if p.hand==context.hand and context.now>=(context.failed[p.keys]or 0)then
   local motion=flat(context.facing)*p.forward+flat(context.right)*p.right
   local destination=origin+motion
   local pathGap,ending=30,30;local safe=true
   for i=1,8 do
    local fraction=i/8;local t=p.duration*fraction
    local point=origin+motion*fraction
    -- Check a second envelope for residual momentum as the move takes over.
    local drift=velocity*(.10*math.min(1,t/.15))
    if not context.allowed(point)or not context.allowed(point+drift)then safe=false;break end
    for _,d in ipairs(context.defenders)do
     local v=flat(d.velocity or Vector3.zero)
     if v.Magnitude>28 then v=v.Unit*28 end
     local predicted=d.position+v*math.min(t+(context.ping or 0),.4)
     local gap=math.min(flat(point-predicted).Magnitude,flat(point+drift-predicted).Magnitude)
     -- Uncertainty grows after the short reliable velocity horizon.
     gap=gap-math.max(0,t-.25)*1.5
     pathGap=math.min(pathGap,gap)
     if i==8 then ending=math.min(ending,gap)end
    end
   end
   local progress=route.Magnitude-flat(context.target-destination).Magnitude
   local alignment=motion.Magnitude>.1 and route.Unit:Dot(motion.Unit)or 0
   local gain=ending-nearest
   local useful=progress>.6 or gain>1 and progress> -1
   local score=progress*1.8+math.min(pathGap,10)+math.min(ending,12)*.65+alignment*2-p.duration
   -- Prefer faster separation; diversify only among moves that pass the same safety gate.
   score=score+math.clamp(gain,-4,4)*.6+(p.sprint and .5 or 0)
   for _,recent in ipairs(context.history or {})do
    local weight=math.clamp(1-(context.now-recent.at)/4,0,1)
    if recent.keys==p.keys then score=score-6*weight
    elseif recent.package==p.package then score=score-3*weight end
   end
   if context.lastKeys==p.keys and context.now-context.lastAt<1 then score=score-3 end
   if safe and pathGap>=math.max(1.5,math.min(2.4,nearest-.25))and useful and(not bestScore or score>bestScore)then
    best={keys=p.keys,sprint=p.sprint,duration=p.duration,handAfter=p.handAfter,
     destination=destination,package=p.package,gap=pathGap};bestScore=score
   end
  end
 end
 return best
end
-- END FARM DRIBBLE SELECTOR
settings._farmSelectDribble=farmDribble

local positional=(function()
-- Positional geometry shared by the live planner and scenario tests.
local function flat(v)return Vector3.new(v.X,0,v.Z)end
local function unit(v,fallback)return v.Magnitude>.01 and v.Unit or fallback end
local function predict(d,t)
 local v=flat(d.velocity or Vector3.zero)
 if v.Magnitude>28 then v=v.Unit*28 end
 return d.position+v*math.min(t,.35)
end
local function gap(point,list,t)
 local distance=math.huge
 for _,d in ipairs(list)do distance=math.min(distance,flat(point-predict(d,t)).Magnitude)end
 return distance
end
local function lane(a,b,list,duration)
 local minimum=math.huge
 for i=1,8 do minimum=math.min(minimum,gap(a+(b-a)*(i/8),list,duration*i/8))end
 return minimum
end
local function driveRace(origin,goal,list,options,allowed)
 local delta=flat(goal-origin);local distance=delta.Magnitude
 if distance<=9 or #list==0 then return nil end
 local forward=delta.Unit;local right=Vector3.new(-forward.Z,0,forward.X)
 local primary,nearest
 for _,d in ipairs(list)do
  local gap=flat(d.position-origin).Magnitude
  if not nearest or gap<nearest then primary=d;nearest=gap end
 end
 local relative=flat(primary.position-origin)
 local ahead=relative:Dot(forward);local lateral=math.abs(relative:Dot(right))
 if ahead>1 or ahead>=-.75 and lateral<1.5 then return nil end
 local ownSpeed=math.clamp(options.playerSpeed or 13.5,6,28)
 local target=origin+forward*(distance-7)
 local margin=math.huge
 -- Compare arrival at points along the entire run, not just at the hoop.
 -- Every defender may cut it off, including a helper nearer the basket.
 for i=1,16 do
  local point=origin+(target-origin)*(i/16)
  if not allowed(point)then return nil end
  local playerTime=flat(point-origin).Magnitude/ownSpeed
  for _,d in ipairs(list)do
   local speed=math.clamp(math.max(d.speed or ownSpeed,flat(d.velocity or Vector3.zero).Magnitude),6,30)
   local defenderTime=math.max(0,flat(point-d.position).Magnitude-2.1)/speed
   margin=math.min(margin,defenderTime-playerTime)
   if defenderTime<=playerTime+.025 then return nil end
  end
 end
 return {target=target,margin=margin,primaryAhead=ahead,primaryLateral=lateral}
end
local function window(point,goal,list,options)
 local pressure=0;local toGoal=flat(goal-point)
 local distance=gap(point,list,0)
 for _,d in ipairs(list)do
  for _,t in ipairs({0,.30})do
   local delta=flat(predict(d,t)-point);local n=delta.Magnitude
   local front=n>.01 and toGoal.Magnitude>.01 and math.clamp((toGoal.Unit:Dot(delta.Unit)+1)/2,0,1)or 1
   pressure=math.max(pressure,math.clamp(1-n/10,0,1)*(.35+.65*front)*100)
  end
 end
 local range=toGoal.Magnitude
 local outside=not options.threeOnly or options.threeDistance>0 and range>=options.threeDistance+.75
 local inPlan=options.mode~=2 or range<=16
 return {open=distance>=math.max(4,options.gap*.65)and pressure<=options.contest and range<=options.range and outside and inPlan,
  contest=pressure,gap=distance,range=range,outside=outside}
end
local function analyze(origin,facing,goal,list,options,allowed)
 local forward=unit(flat(goal-origin),facing)
 local right=Vector3.new(-forward.Z,0,forward.X)
 local nearest,nearestDistance
 for _,d in ipairs(list)do
  local distance=flat(d.position-origin).Magnitude
  if not nearestDistance or distance<nearestDistance then nearest=d;nearestDistance=distance end
 end
 local shade=nearest and flat(predict(nearest,.18)-origin):Dot(right)or 0
 local preferred=shade>.35 and -1 or shade<-.35 and 1 or 0
 local driveDistance=math.min(8,math.max(0,flat(goal-origin).Magnitude-7))
 local candidates={}
 for _,side in ipairs({-1,0,1})do
  local target=origin+forward*driveDistance+right*side*5.5
  if allowed(target)then
   local pathGap=lane(origin,target,list,.45)
   local progress=flat(goal-origin).Magnitude-flat(goal-target).Magnitude
   local score=progress*1.1+math.min(pathGap,10)*1.8+(side==preferred and preferred~=0 and 2.5 or 0)
   candidates[#candidates+1]={target=target,side=side,gap=pathGap,score=score,progress=progress}
  end
 end
 local chosen
 for _,candidate in ipairs(candidates)do
  if candidate.gap>=2.1 and(not chosen or candidate.score>chosen.score)then chosen=candidate end
 end
 local shot=window(origin,goal,list,options)
 local plan={intent="Find a driving lane",shade=shade,preferredSide=preferred,forward=forward,right=right,
  lanes=candidates,shot=shot,primary=nearest and nearest.character,blocked=not chosen}
 local race=driveRace(origin,goal,list,options,allowed)
 if race then
  plan.intent="Defender beaten; run straight to basket";plan.target=race.target
  plan.driveAdvantage=true;plan.raceMargin=race.margin;plan.blocked=false;return plan
 end
 if shot.open then plan.intent="Take open shot";plan.target=origin;return plan end
 if options.threeOnly and options.threeDistance<=0 then plan.intent="Waiting for detected three-point arc";return plan end
 local huntThree=options.threeOnly or options.threeDistance>0 and options.mode==1 and shot.range>=options.threeDistance-5
 if huntThree then
  local radial=unit(flat(origin-goal),forward*-1)
  local perpendicular=Vector3.new(-radial.Z,0,radial.X)
  local best,bestScore
  for _,angle in ipairs({-.42,-.22,0,.22,.42})do
   local target=goal+(radial*math.cos(angle)+perpendicular*math.sin(angle))*(options.threeDistance+1.5)
   target=Vector3.new(target.X,origin.Y,target.Z)
   local delta=flat(target-origin)
   local nextPoint=delta.Magnitude>8 and origin+delta.Unit*8 or target
   if allowed(target)and allowed(nextPoint)then
    local pathGap=lane(origin,nextPoint,list,.45)
    local w=window(target,goal,list,options)
    local score=math.min(w.gap,15)*.7+math.min(pathGap,10)*1.2-delta.Magnitude*.25-w.contest*.08
    if pathGap>=2.1 and(not bestScore or score>bestScore)then best=target;bestScore=score end
   end
  end
  if best then plan.intent="Create open three";plan.target=best;plan.blocked=false;return plan end
 end
 if chosen then
  plan.intent=chosen.side<0 and "Attack left lane"or chosen.side>0 and "Attack right lane"or "Attack middle lane"
  plan.target=chosen.target;plan.side=chosen.side
 else plan.intent="Defender contains drive; create a shot"end
 return plan
end
local function threeSetup(origin,goal,list,options,stage,now,allowed,retreats)
 if options.mode~=3 or options.threeDistance<=0 or #list==0 then return nil,nil end
 local actual=window(origin,goal,list,options)
 if actual.open then return nil,nil end
 local forward=unit(flat(goal-origin),Vector3.new(1,0,0))
 local right=Vector3.new(-forward.Z,0,forward.X)
 local primary,nearest
 for _,d in ipairs(list)do
  local distance=flat(d.position-origin).Magnitude
  if not nearest or distance<nearest then primary=d;nearest=distance end
 end
 if stage and(flat(stage.goal-goal).Magnitude>1 or now-stage.started>2)then stage=nil end
 if stage and stage.phase=="Retreat"then
  local selected
  for _,r in ipairs(retreats)do
   if r.keys==stage.keys and allowed(r.target)and window(r.target,goal,list,options).open and lane(origin,r.target,list,.35)>=1.6 then selected=r;break end
  end
  if selected then
   return {intent="Stepback outside arc for the shot",target=selected.target,keys=selected.keys,phase="Retreat"},stage
  end
  stage=nil
 end
 if not stage then
  local shade=flat(primary.position-origin):Dot(right)
  local preferred=shade>.3 and -1 or shade<-.3 and 1 or options.hand=="L"and 1 or -1
  local target
  for _,side in ipairs({preferred,-preferred})do
   local candidate=origin+forward*2.8+right*side*2.5
   if allowed(candidate)and lane(origin,candidate,list,.30)>=1.6 then target=candidate;break end
  end
  if not target then return nil,nil end
  stage={phase="Attack",origin=origin,goal=goal,target=target,forward=forward,started=now,
   defender=primary.character,defenderPosition=primary.position}
 end
 local progress=flat(origin-stage.origin):Dot(stage.forward)
 local reacted=false
 for _,d in ipairs(list)do
  if d.character==stage.defender then
   local change=flat(d.position-stage.defenderPosition)
   local toward=unit(flat(stage.target-stage.defenderPosition),stage.forward)
   reacted=change:Dot(stage.forward)>.65 or change:Dot(toward)>.65 or flat(d.velocity or Vector3.zero):Dot(toward)>2.5
  end
 end
 if progress>=1 and(reacted or now-stage.started>.55)then
  for _,r in ipairs(retreats)do
   if allowed(r.target)and window(r.target,goal,list,options).open and lane(origin,r.target,list,.35)>=1.6 then
    stage.phase="Retreat";stage.keys=r.keys
    return {intent="Defender reacted; stepback into three",target=r.target,keys=r.keys,phase="Retreat"},stage
   end
  end
 end
 if now-stage.started>1.25 or flat(stage.target-origin).Magnitude<.65 then return nil,stage end
 if not allowed(stage.target)or lane(origin,stage.target,list,.30)<1.6 then return nil,nil end
 return {intent="Show basket attack to set up the three",target=stage.target,phase="Attack"},stage
end
return {Analyze=analyze,Window=window,Lane=lane,Gap=gap,DriveRace=driveRace,ThreeSetup=threeSetup}

end)()
 settings.smartDribble=false;settings.ballProtection=false
 settings.smartDribbleKey=Enum.KeyCode.H
 settings.smartDribbleGap=7;settings.smartDribbleStamina=30;settings.smartDribbleCooldown=.12
 settings.smartDribbleShoot=true;settings.smartDribbleShotContest=15;settings.smartDribbleShotRange=65
 settings.smartDribblePlan=1;settings.smartDribbleThreeDistance=0;settings.smartDribbleSteer=true
 numericLimits.smartDribblePlan={1,3};numericLimits.smartDribbleThreeDistance={0,90}
 numericLimits.smartDribbleShotContest={0,40};numericLimits.smartDribbleShotRange={15,90}
 settings.ballProtectionRange=4.5
 numericLimits.smartDribbleGap={4,12};numericLimits.smartDribbleStamina={15,70}
 numericLimits.smartDribbleCooldown={.07,.5};numericLimits.ballProtectionRange={2,6}
 local function keyName()return tostring(settings.smartDribbleKey):match("[^.]+$")end
 local held=false;local stopped=false;local last=-math.huge;local manualUntil=0
 local status={status="Disabled",attempts=0,confirmed=0,rejected=0,shots=0}
 local outstanding
 local chainRemaining,chainAt=0,-math.huge
 local function cadence(now)
  return chainRemaining>0 and now-chainAt<.9 and math.min(settings.smartDribbleCooldown,.09)or settings.smartDribbleCooldown
 end
 local function chainSent(now)
  chainRemaining=chainRemaining>0 and now-chainAt<.9 and chainRemaining-1 or 2
  chainAt=now
 end
 local history={};local failed={};local openSince;local shotRequest
 local possessionPlan,planAt;local settling=false;local tacticalWaitUntil=0
 local driveCommittedUntil=0
 local threeSetupState
 local pickedBasket,pickingBasket,pickedBasketOwner,pickedBasketCourt,routeProbe,routeChosenAt
 local movementSample,movementAt,observedSpeed
 local busyActions={Shooting=true,Dunking=true,Blocking=true,Rebounding=true,Stealing=true,Diving=true,Stunned=true,Dead=true}
 local function staminaPercent(own)
  local value=tonumber(attr(own,"Stamina"));local maximum=tonumber(attr(own,"MaxStamina"))
  if value and maximum and maximum>0 then return value/maximum*100 end
  return value
 end
 local function stopDirection(reason)
  settings._smartOffenseReason=reason;return nil
 end
 local function clearOffenseMovement()
  if not settings._smartOffenseMoving then return end
  settings._smartOffenseMoving=false
  for _,key in ipairs({Enum.KeyCode.W,Enum.KeyCode.A,Enum.KeyCode.S,Enum.KeyCode.D})do if Input:IsKeyDown(key)then return end end
  local controller=keyboard and keyboard.Controllers and keyboard.Controllers.Player
  local movement=controller and controller.Character and controller.Character.Movement
  if movement then
   movement.MoveDirection=Vector3.zero;movement.lastMoveDirection=Vector3.zero
   movement.forwardValue=0;movement.rightValue=0
   pcall(function()movement:SendMoveDirection(Vector3.zero)end)
  end
 end
 local function options()
  local own=model();local r=rootOf(own)
  local speed=tonumber(own and attr(own,"WalkSpeed"))or 13.5
  if r then speed=math.max(speed,flat(r.AssemblyLinearVelocity or Vector3.zero).Magnitude)end
  local controller=keyboard and keyboard.Controllers and keyboard.Controllers.Player
  local proxy=controller and controller.Character and controller.Character.ProxyCharacter
  if proxy then speed=math.max(speed,flat(proxy.AssemblyLinearVelocity).Magnitude)end
  local committed=os.clock()<driveCommittedUntil
  return {gap=settings.smartDribbleGap,contest=settings.smartDribbleShotContest,range=settings.smartDribbleShotRange,
   mode=committed and 1 or(settings.smartDribblePlan==3 and settings.smartDribbleThreeDistance<=0 and 1 or settings.smartDribblePlan),threeDistance=settings.smartDribbleThreeDistance,threeOnly=settings.smartDribblePlan==3 and settings.smartDribbleThreeDistance>0 and not committed,playerSpeed=speed,hand=own and attr(own,"Hand")}
 end
 local practice,practiceRoot,practiceOwner,practiceAxis,practiceBasket
 local practiceHistory={};local practiceVelocity=Vector3.zero
 settings.dribbleTestFollow=true;settings.dribbleTestSpeed=12
 numericLimits.dribbleTestSpeed={3,20}
 local function removePractice()
  if practice then practice:Destroy()end
  practice=nil;practiceRoot=nil;practiceOwner=nil;practiceBasket=nil
  practiceHistory={};practiceVelocity=Vector3.zero
 end
 api.StopDribblePractice=function()removePractice();return true,"Practice defender removed"end
 api.StartDribblePractice=function()
  local own=model();local r=rootOf(own)
  if not r then return false,"Character unavailable"end
  removePractice()
  local dummy=Instance.new("Model");dummy.Name="PracticalDribblePractice"
  local origin=r.CFrame*CFrame.new(0,0,-3.5)
  local function part(name,size,offset,color)
   local p=Instance.new("Part");p.Name=name;p.Size=size;p.Anchored=true
   p.CanCollide=false;p.CanTouch=false;p.CanQuery=false;p.CastShadow=false
   p.Color=color;p.CFrame=origin*CFrame.new(offset);p.Parent=dummy
   return p
  end
  local blue=Color3.fromRGB(57,137,232);local dark=Color3.fromRGB(29,40,60)
  local root=part("HumanoidRootPart",Vector3.new(2,2,1),Vector3.zero,dark);root.Transparency=1
  part("Torso",Vector3.new(2,2,1),Vector3.zero,blue)
  local head=part("Head",Vector3.new(1.1,1.1,1.1),Vector3.new(0,1.65,0),Color3.fromRGB(190,204,223))
  head.Shape=Enum.PartType.Ball
  part("LeftArm",Vector3.new(.65,2,.65),Vector3.new(-1.4,0,0),blue)
  part("RightArm",Vector3.new(.65,2,.65),Vector3.new(1.4,0,0),blue)
  part("LeftLeg",Vector3.new(.8,2,.8),Vector3.new(-.55,-2,0),dark)
  part("RightLeg",Vector3.new(.8,2,.8),Vector3.new(.55,-2,0),dark)
  local marker=Instance.new("Highlight");marker.Name="PracticeOutline";marker.Adornee=dummy
  marker.FillTransparency=1;marker.OutlineColor=blue;marker.OutlineTransparency=.15;marker.Parent=dummy
  local badge=Instance.new("BillboardGui");badge.Size=UDim2.fromOffset(200,24)
  badge.StudsOffsetWorldSpace=Vector3.new(0,1.3,0);badge.AlwaysOnTop=true;badge.Adornee=head;badge.Parent=head
  local text=Instance.new("TextLabel");text.Size=UDim2.fromScale(1,1);text.BackgroundTransparency=1
  text.Text="LOCAL PRACTICE DEFENDER";text.TextColor3=blue;text.Font=Enum.Font.GothamBold;text.TextSize=11;text.Parent=badge
  dummy.PrimaryPart=root;dummy.Parent=workspace
  practice=dummy;practiceRoot=root;practiceOwner=own
  practiceAxis=flat(r.CFrame.LookVector).Unit
  -- In solo/lobby practice the game may not assign Goal. Use an actual nearby
  -- rim part if present, never a fictional shot target.
  if workspace.GetDescendants then
   local distance=math.huge
   for _,object in ipairs(workspace:GetDescendants())do
    if object:IsA("BasePart")and(object.Name:lower()=="rim"or object.Name:lower()=="hoop")then
     local delta=object.Position-r.Position
     local d=flat(delta).Magnitude
     if delta.Y>1 and delta.Y<18 and d<90 and d<distance then practiceBasket=object;distance=d end
    end
   end
  end
  status.status="Practice defender ready; gain possession and hold "..keyName()..""
  return true,"Practice defender spawned nearby; use your real ball and hold "..keyName()..""
 end
 local courtScanAt,autoOwner,autoCourt,autoBasket,autoGoalPart,arcScanAt=0
 local function automaticCourt(own,assigned)
  local root=rootOf(own);if not root then return assigned end
  local court=attr(own,"CourtNumber")or attr(own,"Court")
  if autoOwner~=own or autoCourt~=court then
   autoOwner=own;autoCourt=court;autoBasket=nil;autoGoalPart=nil;courtScanAt=0;arcScanAt=0
   settings.smartDribbleThreeDistance=0
  end
  local now=os.clock()
  if now-courtScanAt>=1 then
   courtScanAt=now
   local best,bestDistance
   if workspace.GetDescendants then
    for _,object in ipairs(workspace:GetDescendants())do
     if object:IsA("BasePart")then
      local name=object.Name:lower()
      if name=="rim"or name=="hoop"or name=="goal"then
       local point=object.Position;local height=point.Y-root.Position.Y
       local distance=flat(point-(assigned or root.Position)).Magnitude
       if height>1 and height<18 and distance<(assigned and 8 or 90)and(not bestDistance or distance<bestDistance)then best=object;bestDistance=distance end
      end
     end
    end
   end
   autoGoalPart=best;autoBasket=best and best.Position
  end
  local goal=assigned or autoBasket
  status.basketFound=goal~=nil;status.basketSource=assigned and "Assigned Goal"or autoBasket and "Court rim"or "Unavailable"
  if not goal then status.court="Basket unavailable; defender-relative dribbling"end
  if goal and now-(arcScanAt or 0)>=.5 then
   arcScanAt=now
   -- Read the same three-point zone geometry used by GameUtil.CheckInsideArc.
   -- No fixed court radius and no manual arc calibration.
   local ok,radius=pcall(function()
    local assets=workspace:FindFirstChild("Assets")
    local functionals=assets and assets:FindFirstChild("Functionals")
    local zones=functionals and functionals:FindFirstChild("Zones")
    local arc=zones and zones:FindFirstChild("ThreePoint")
    local radial=flat(root.Position-goal)
    if not arc or radial.Magnitude<1 then return nil end
    local params=RaycastParams.new();params.FilterType=Enum.RaycastFilterType.Include;params.FilterDescendantsInstances={arc}
    local origin=assigned or(autoGoalPart and autoGoalPart.Position)or goal
    local hit=workspace:Raycast(origin,radial.Unit*100,params)
    local distance=hit and flat(hit.Position-origin).Magnitude
    if distance and distance>=10 and distance<=90 then return distance end
   end)
   if ok and radius then settings.smartDribbleThreeDistance=radius;status.court="Basket and arc detected"
   else settings.smartDribbleThreeDistance=0;status.court=goal and "Basket detected; arc geometry unavailable"or "Waiting for basket"end
  end
  return goal
 end
 local function basket(own)
  local value=attr(own,"Goal")
  if typeof(value)=="Instance"and value:IsA("ObjectValue")then value=value.Value end
  local assigned=goalPosition and goalPosition(value)
  if not assigned and type(value)=="table"then assigned=goalPosition(value.Base or value.Model or value.Root)end
  local automatic=automaticCourt(own,assigned)
  if automatic then return automatic end
  if pickedBasket then
   local court=own and(attr(own,"CourtNumber")or attr(own,"Court"))
   if own==pickedBasketOwner and court==pickedBasketCourt then status.basketFound=true;status.basketSource="Selected rim";return pickedBasket end
   pickedBasket=nil;pickedBasketOwner=nil;pickedBasketCourt=nil
  end
  local goal=goalPosition and goalPosition(attr(own,"Goal"))
  return goal or(practice and practiceBasket and practiceBasket.Parent and practiceBasket.Position)
 end
 api.PickDribbleBasket=function()
  pickingBasket=true;status.status="Click the rim in the game to select the basket"
  return true,status.status
 end
 api.MarkDribbleThreePointLine=function()
  local own=model();local r=rootOf(own);local goal=basket(own)
  if not r or not goal then return false,"Assigned basket not found; use a court with a detected basket"end
  local distance=flat(goal-r.Position).Magnitude
  if distance<10 or distance>90 then return false,"Stand on the three-point arc, between 10 and 90 studs from the basket"end
  settings.smartDribbleThreeDistance=distance
  return true,string.format("Three-point target distance marked: %.1f studs (radial approximation)",distance)
 end
 local function updatePractice(dt)
  if not practice then return end
  local own=model();local r=rootOf(own)
  if not practice.Parent or own~=practiceOwner or not r or(practiceRoot.Position-r.Position).Magnitude>60 then removePractice();return end
  local position=practiceRoot.Position;local now=os.clock();dt=math.min(dt,.1)
  practiceHistory[#practiceHistory+1]={time=now,position=r.Position,velocity=flat(r.AssemblyLinearVelocity or Vector3.zero)}
  while #practiceHistory>1 and practiceHistory[2].time<now-.18 do table.remove(practiceHistory,1)end
  if settings.dribbleTestFollow and holdsBall(own)and held and(settings.smartDribble or settings.ballProtection)then
   local sample=practiceHistory[1]
   local goal=basket(own)
   local axis=goal and flat(goal-sample.position)or practiceAxis
   if axis.Magnitude>.1 then axis=axis.Unit else axis=practiceAxis end
   local lateral=Vector3.new(-axis.Z,0,axis.X)
   local shade=(attr(own,"Hand")=="R"and 1 or -1)*.65
   local desired=sample.position+axis*3.5+lateral*shade+sample.velocity*.10
   local delta=flat(clampCourt(desired)-position)
   local desiredVelocity=delta.Magnitude>.3 and delta.Unit*math.min(settings.dribbleTestSpeed,delta.Magnitude*5)or Vector3.zero
   local change=desiredVelocity-practiceVelocity
   if change.Magnitude>36*dt then change=change.Unit*(36*dt)end
   practiceVelocity=practiceVelocity+change
   if practiceVelocity.Magnitude*dt>delta.Magnitude then position=position+delta;practiceVelocity=Vector3.zero
   else position=position+practiceVelocity*dt end
  else practiceVelocity=Vector3.zero end
  position=Vector3.new(position.X,r.Position.Y,position.Z)
  local look=Vector3.new(r.Position.X,position.Y,r.Position.Z)
  if flat(look-position).Magnitude>.05 then practice:PivotTo(CFrame.lookAt(position,look))end
 end
 local function sameCourt(a,b)
  for _,field in ipairs({"Court","CourtNumber"})do
   local x,y=attr(a,field),attr(b,field)
   local function normalize(v)
    if typeof(v)=="Instance"then v=v.Name end
    if type(v)=="number"or type(v)=="string"then return tostring(v):lower():gsub("^court[%s_%-]*","")end
    return v
   end
   x,y=normalize(x),normalize(y)
   if x~=nil and y~=nil and x~="0"and y~="0"and x~=""and y~=""and x~=y then return false end
  end
  return true
 end
 local function enemy(a,b)
  if a==b or not rootOf(b)or not sameCourt(a,b)then return false end
  local x,y=attr(a,"Team"),attr(b,"Team")
  if x~=nil and y~=nil and x~=0 and y~=0 and x~=""and y~=""then return x~=y end
  local players=game:GetService("Players")
  local pa=players:GetPlayerFromCharacter(a)or P
  local pb=players:GetPlayerFromCharacter(b)or players:FindFirstChild(b.Name)
  return pa and pb and pa.Team~=nil and pb.Team~=nil and pa.Team~=pb.Team or false
 end
 local function defenders(own,origin)
  local list={}
  for _,other in ipairs(defenseCharacters())do
   local r=rootOf(other)
   if r and enemy(own,other)and math.abs(r.Position.Y-origin.Y)<9 and flat(r.Position-origin).Magnitude<80 then
    list[#list+1]={character=other,position=r.Position,velocity=r.AssemblyLinearVelocity or Vector3.zero,speed=tonumber(attr(other,"WalkSpeed"))}
   end
  end
  -- Explicit local target only; real-player team checks stay unchanged.
  if practice and practice.Parent and practiceRoot and practiceOwner==own then
   list[#list+1]={character=practice,position=practiceRoot.Position,velocity=practiceVelocity,speed=settings.dribbleTestSpeed,localPractice=true}
  end
  return list
 end
 local function clearance(point,list,time)
  local minimum=30
  for _,d in ipairs(list)do
   local velocity=flat(d.velocity)
   if velocity.Magnitude>30 then velocity=velocity.Unit*30 end
   minimum=math.min(minimum,flat(point-(d.position+velocity*math.min(time,.25))).Magnitude)
  end
  return minimum
 end
 local function moveKey(p)return p.keys..":"..tostring(p.sprint)..":"..p.hand end
 local function selectMove(own,r,list,protect,now,plan)
  local hand=attr(own,"Hand")
  local nearest=clearance(r.Position,list,0)
  local best,bestScore
  for _,p in ipairs(measuredDribbleProfiles)do
   local key=moveKey(p);local repeated=false
   for i=math.max(1,#history-2),#history do
    local h=history[i]
    if h.keys==p.keys and(now-h.at<.55 or i==#history)then repeated=true end
   end
   local tacticalRetreat=plan and plan.threePhase=="Retreat"and p.keys==plan.retreatKeys
   if tacticalRetreat then repeated=false end
   if not p.keys:find("V",1,true)and p.hand==hand and not repeated and now>=(failed[key]or 0)
    and(not plan or plan.threePhase~="Retreat"or tacticalRetreat)then
    local destination=r.Position+flat(r.CFrame.LookVector)*p.forward+flat(r.CFrame.RightVector)*p.right
    local bounded=clampCourt(destination)
    if flat(bounded-destination).Magnitude<.15 then
     -- A heuristic path test; the actual animation is curved and opponents can react.
     local pathGap=30
     for i=1,4 do pathGap=math.min(pathGap,clearance(r.Position+(destination-r.Position)*(i/4),list,p.duration*i/4))end
     local ending=clearance(destination,list,p.duration)
     local gain=ending-nearest
     local goal=nil -- Dribble selection never depends on court hoops.
     local progress=goal and(flat(goal-r.Position).Magnitude-flat(goal-destination).Magnitude)or p.forward
     local retreatCount=0
     for i=math.max(1,#history-1),#history do if history[i].forward< -2 then retreatCount=retreatCount+1 end end
     local score=ending*.7+pathGap*.55+math.clamp(progress,-8,8)*.22-(p.sprint and .25 or 0)
     if p.forward< -2 then score=score-retreatCount*5 end
     -- A hand-changing setup can precede the escape, but cannot replace a
     -- failed escape with an endless alternating crossover loop.
     local setup=p.handAfter and p.handAfter~=hand and #history<2
     if goal and plan then
      local shot=positional.Window(destination,goal,list,options())
      local route=plan.target and flat(plan.target-r.Position)
      local motion=flat(destination-r.Position)
      local alignment=route and route.Magnitude>.1 and motion.Magnitude>.1 and route.Unit:Dot(motion.Unit)or 0
      local side=motion:Dot(plan.right)
      local awayFromShade=plan.preferredSide~=0 and side*plan.preferredSide or 0
      score=progress*1.6+math.min(pathGap,10)*1.25+math.min(ending,15)*.3+alignment*3+math.clamp(awayFromShade,-5,5)*.55
      -- Retreat only serves a specific shot, rather than farming raw separation.
      if p.forward< -2 and not protect then
       local arc=settings.smartDribbleThreeDistance
       local reachesArc=arc>0 and flat(goal-r.Position).Magnitude<arc+.75 and shot.range>=arc+.75
       local purpose=plan.blocked or settings.smartDribblePlan==3 or math.abs(plan.shade)<1.4 and nearest<6 or reachesArc
       if shot.open and purpose then score=score+23 else score=score-25 end
      elseif shot.open then score=score+12 end
      if plan.blocked and shot.open then score=score+8 end
      if settings.smartDribblePlan==3 and not shot.outside then score=score-10 end
      if retreatCount>0 and p.forward< -2 then score=score-12 end
      if p.package:sub(1,4)=="Spin"and nearest<=5.5 and nearest>=2 and alignment>.25 and pathGap>=1.6 and progress>.5 then score=score+6 end
      if plan.threePhase=="Attack"and p.forward< -2 then score=score-100 end
      if plan.threePhase=="Retreat"then score=score+(p.keys==plan.retreatKeys and 100 or -100)end
     end
     if protect then score=score+(p.hand=="R"and -p.right or p.right)*.12 end
     if chainRemaining>0 and pathGap>=1.6 then
      score=score+#p.keys*1.4
      if p.package:sub(1,4)=="Spin"and nearest<=7 and progress>0 then score=score+3 end
      for _,h in ipairs(history)do
       if h.package==p.package and now-h.at<1.2 then score=score-3 end
       if h.keys==p.keys and now-h.at<1.2 then score=score-2 end
      end
     end
     local useful=gain>.35 or setup and gain>-.25 or plan and progress>1 and pathGap>=2.1
     if pathGap>=math.min(1.6,nearest-.1)and useful and(not bestScore or score>bestScore)then
      best=p;bestScore=score
     end
    end
   end
  end
  return best
 end
 local function stamp()
  local movement=keyboard and keyboard.Controllers and keyboard.Controllers.Player
  movement=movement and movement.Character and movement.Character.Movement
  return movement and movement.lastDribble
 end
 local function openSpaceMove(own,r,now)
  local best,bestScore
  for _,p in ipairs(measuredDribbleProfiles)do
   local lastUsed=now-12
   for _,h in ipairs(history)do if h.keys==p.keys then lastUsed=math.max(lastUsed,h.at)end end
   local destination=r.Position+flat(r.CFrame.LookVector)*p.forward+flat(r.CFrame.RightVector)*p.right
   local previous=history[#history]
   if p.hand==attr(own,"Hand")and not p.keys:find("V",1,true)and now-lastUsed>=.45 and(not previous or previous.keys~=p.keys)and now>=(failed[moveKey(p)]or 0)
    and flat(clampCourt(destination)-destination).Magnitude<.15 then
    local score=math.min(now-lastUsed,12)-math.abs(p.forward)*.12+#p.keys*.4
    for _,h in ipairs(history)do if h.package==p.package and now-h.at<1.2 then score=score-2 end end
    if not bestScore or score>bestScore then best=p;bestScore=score end
   end
  end
  if not best then status.status="Waiting for another open-space move";return end
  local service=keyboard and keyboard.Services and keyboard.Services.InputService
  if not service or not service.Dribble then return end
  if api.CancelCombo then api.CancelCombo()end
  local before=stamp()
  local ok=pcall(function()service.Dribble:Fire(best.keys,best.sprint,Vector3.zero)end)
  last=now
  if not ok then failed[moveKey(best)]=now+2;status.status="Open-space move unavailable";return end
  outstanding={character=own,stamp=before,at=now,key=moveKey(best),hand=attr(own,"Hand"),handAfter=best.handAfter,action=attr(own,"Action")}
  chainSent(now)
  history[#history+1]={keys=best.keys,package=best.package,forward=best.forward,at=now};if #history>6 then table.remove(history,1)end
  status.attempts=status.attempts+1;status.move=best.keys;status.package=best.package
  status.intent="Open-space dribbling";status.status="Open space: "..best.package
 end
 local function update()
  if stopped then return end
  local now=os.clock();local own=model();local r=rootOf(own)
  settling=false
  if r then
   if movementSample and movementAt and now>movementAt then observedSpeed=flat(r.Position-movementSample).Magnitude/(now-movementAt)end
   movementSample=r.Position;movementAt=now
  end
  if shotRequest then
   local shot=shotRequest
   local released=own and attr(own,"ReleasedShot")==true
   local meter=active and active.started and active.started>=shot.at-.05
   if own~=shot.character or not r or not holdsBall(own)or released or meter and active.sent then shotRequest=nil
   elseif now-shot.at>2.2 or not settings.smartDribble or not held then
    pcall(function()shot.service.Shoot:Fire({Shoot=false})end);shotRequest=nil
   else status.status="Open shot requested; waiting for Auto time";return end
  end
  if outstanding then
   if own~=outstanding.character or not r or not holdsBall(own)then outstanding=nil
   elseif stamp()~=outstanding.stamp
    or outstanding.handAfter and outstanding.handAfter~=outstanding.hand and attr(own,"Hand")==outstanding.handAfter
    or outstanding.action~="Dribbling"and attr(own,"Action")=="Dribbling"then
    status.confirmed=status.confirmed+1;outstanding=nil
   elseif now-outstanding.at>.4 then
    status.rejected=status.rejected+1;status.status="Move not acknowledged; backing off"
    failed[outstanding.key]=now+2;last=now+.15;outstanding=nil
   end
  end
  if settings.farm then clearOffenseMovement();status.status="Auto farm owns dribble decisions";return end
  if not settings.smartDribble then clearOffenseMovement()end
  if not settings.smartDribble and not settings.ballProtection then status.status="Disabled";return end
  if settings._catchShootOwns then clearOffenseMovement();status.status="Catch-and-shoot owns this possession";return end
  if (settings.smartDribble or settings.ballProtection)and(guardKey==settings.smartDribbleKey or settings.comboPlayKey==settings.smartDribbleKey or api.IsUIReservedKey and api.IsUIReservedKey(settings.smartDribbleKey))then status.status="Dribble key conflicts with another bound action";openSince=nil;return end
  if not held then status.status="Hold "..keyName().." for dribbling or ball protection";openSince=nil;return end
  if Input:GetFocusedTextBox()or api.IsUICapturingInput and api.IsUICapturingInput()then status.status="Paused for typing or key binding";openSince=nil;return end
  if not r or not holdsBall(own)then status.status="Waiting for possession";openSince=nil;history={};possessionPlan=nil;driveCommittedUntil=0;threeSetupState=nil;return end
  local action=attr(own,"Action")
  if busyActions[action]then status.status="Busy: "..tostring(action);openSince=nil;return end
  if now<tacticalWaitUntil and action=="Dribbling"then status.status="Let stepback create the shot window";return end
  if attr(own,"CanMove")==false and action~="Dribbling"or attr(own,"CanDribble")==false or attr(own,"InPost")==true then status.status="Movement or dribble unavailable";return end
  local stamina=staminaPercent(own)
  if stamina and stamina<settings.smartDribbleStamina then status.status="Saving stamina";openSince=nil;return end
  if now<manualUntil or outstanding then openSince=nil;status.status=outstanding and "Waiting for move confirmation"or "Manual dribble takes priority";return end
  local list=defenders(own,r.Position)
  status.defenders=#list
  local team=attr(own,"Team")
  local nearest=clearance(r.Position,list,0)
  status.basketFound=false;status.basketSource="Not used";status.arcFallback=false
  if settings.smartDribble and held and nearest>=12 then
   clearOffenseMovement();possessionPlan=nil;openSince=nil;settling=false
   if now-last>=cadence(now)then openSpaceMove(own,r,now)end
   return
  end
  if #list==0 and(team==nil or team==0 or team=="")and P.Team==nil then status.status="No identified opponent nearby";openSince=nil;possessionPlan=nil;return end
  local protect=false
  if settings.ballProtection and held and nearest<=settings.ballProtectionRange then
   local side=attr(own,"Hand")=="R"and 1 or -1
   for _,d in ipairs(list)do
    local delta=flat(d.position-r.Position)
    if delta.Magnitude<=settings.ballProtectionRange and delta:Dot(flat(r.CFrame.RightVector))*side>.25 then protect=true;break end
   end
  end
  if not protect and not(settings.smartDribble and held)then status.status="Hold "..keyName().." for smart dribbling";openSince=nil;return end
  -- Dribble-only mode: no basket/arc scan, shot queue, or hoop-directed steering.
  local goal,plan=nil,nil
  possessionPlan=nil;threeSetupState=nil;routeProbe=nil
  status.destination=nil;status.intent="Defender-relative dribbling"
  clearOffenseMovement();openSince=nil;settling=false
  if now-last<cadence(now)then return end
  local p=selectMove(own,r,list,protect,now,plan)
  if not p then status.status="No measured move improves the gap safely";return end
  local service=keyboard and keyboard.Services and keyboard.Services.InputService
  if not service or not service.Dribble then status.status="Dribble controller unavailable";return end
  if api.CancelCombo then api.CancelCombo()end
  local prior=stamp()
  local ok,err=pcall(function()service.Dribble:Fire(p.keys,p.sprint,Vector3.zero)end)
  last=now
  if not ok then status.status="Dribble call failed: "..tostring(err);return end
  status.attempts=status.attempts+1;status.move=p.keys;status.package=p.package
  chainSent(now)
  if plan and goal and p.forward< -2 then
   local destination=r.Position+flat(r.CFrame.LookVector)*p.forward+flat(r.CFrame.RightVector)*p.right
   if positional.Window(destination,goal,list,options()).open then
    status.intent="Stepback into open shot";plan.intent=status.intent;plan.target=destination
    tacticalWaitUntil=now+math.min(p.duration,.65)
   end
  end
  status.status=(protect and "Protect ball: "or plan and plan.intent..": "or "Create space: ")..p.package.." ["..p.keys.."]"
  history[#history+1]={keys=p.keys,package=p.package,forward=p.forward,at=now}
  if #history>6 then table.remove(history,1)end
  outstanding={character=own,stamp=prior,at=now,key=moveKey(p),hand=attr(own,"Hand"),handAfter=p.handAfter,action=attr(own,"Action")}
 end
 api.GetSmartDribbleState=function()
  local result=table.clone(status);result.localPractice=practice~=nil;result.threeDistance=settings.smartDribbleThreeDistance
  local own=model()
  result.held=held;result.key=keyName();result.action=own and attr(own,"Action")
  result.stamina=own and staminaPercent(own);result.canDribble=own and attr(own,"CanDribble")
  result.movementReason=settings._smartOffenseReason or "Movement callback not observed"
  result.observedSpeed=observedSpeed or 0
  result.movementAppliedAt=settings._smartOffenseAppliedAt
  return result
 end
 settings._smartOffenseDirection=function()
  if stopped or not settings.smartDribble or not held then return stopDirection("Smart movement inactive")end
  if not settings.smartDribbleSteer then return stopDirection("Move through chosen lane is disabled")end
  if Input:GetFocusedTextBox()or api.IsUICapturingInput and api.IsUICapturingInput()then return stopDirection("Typing or binding a key")end
  for _,key in ipairs({Enum.KeyCode.W,Enum.KeyCode.A,Enum.KeyCode.S,Enum.KeyCode.D})do if Input:IsKeyDown(key)then return stopDirection("WASD has control")end end
  local own=model();local r=rootOf(own)
  if not r or not holdsBall(own)then return stopDirection("No character or possession")end
  if attr(own,"CanMove")==false then return stopDirection("Game has locked movement")end
  local action=attr(own,"Action")
  if busyActions[action]then return stopDirection("Busy: "..tostring(action))end
  if shotRequest or settling or settings._catchShootOwns then settings._smartOffenseReason="Settling for shot";return Vector3.zero end
  if os.clock()<manualUntil then return stopDirection("Manual dribble has control")end
  if not possessionPlan or os.clock()-(planAt or 0)>.5 or not possessionPlan.target or possessionPlan.blocked then return stopDirection("No fresh usable route")end
  local stamina=staminaPercent(own)
  if stamina and stamina<settings.smartDribbleStamina then return stopDirection("Stamina reserve")end
  local delta=flat(possessionPlan.target-r.Position)
  settings._smartOffenseReason=delta.Magnitude>.8 and "Sending lane movement"or "Reached lane destination"
  return delta.Magnitude>.8 and delta.Unit or Vector3.zero
 end
 local began=Input.InputBegan:Connect(function(input,processed)
  if Input:GetFocusedTextBox()or api.IsUICapturingInput and api.IsUICapturingInput()then return end
  -- Some courts consume the keyboard activation input before our listener.
  -- Accept the configured keyboard hold, but never processed mouse/UI clicks.
  if processed and(input.UserInputType~=Enum.UserInputType.Keyboard or input.KeyCode~=settings.smartDribbleKey)then return end
  local ok,menuOpen=pcall(function()return game:GetService("GuiService").MenuIsOpen end)
  if ok and menuOpen==true then return end
  if pickingBasket and input.UserInputType==Enum.UserInputType.MouseButton1 then
   local mouse=P:GetMouse();local hit=mouse.Target
   local own=model();local r=rootOf(own)
   local position=mouse.Hit.Position
   local avatarHit=hit and own and hit:IsDescendantOf(own)or hit and practice and hit:IsDescendantOf(practice)
   if hit and not avatarHit and r and(position-r.Position).Magnitude<140 and position.Y>r.Position.Y+1 and position.Y<r.Position.Y+25 then
    pickedBasket=position;pickedBasketOwner=own;pickedBasketCourt=attr(own,"CourtNumber")or attr(own,"Court")
    pickingBasket=false;possessionPlan=nil;routeProbe=nil;routeChosenAt=nil
    status.status="Basket selected; hold "..keyName().." to attack"
   else status.status="Click the rim or backboard above the court, within 140 studs"end
   return
  end
  if bindingMatches(input,settings.smartDribbleKey) then held=true
  elseif input.KeyCode==Enum.KeyCode.Z or input.KeyCode==Enum.KeyCode.X or input.KeyCode==Enum.KeyCode.C or input.KeyCode==Enum.KeyCode.V then manualUntil=os.clock()+1.2 end
 end)
 local function releaseHeld()
  held=false;openSince=nil;history={};possessionPlan=nil;settling=false;tacticalWaitUntil=0;driveCommittedUntil=0;routeProbe=nil;routeChosenAt=nil;threeSetupState=nil
  chainRemaining=0;chainAt=-math.huge
  outstanding=nil
  clearOffenseMovement()
  if shotRequest then pcall(function()shotRequest.service.Shoot:Fire({Shoot=false})end);shotRequest=nil end
 end
 api.SetSmartDribbleKey=function(key)
  if not bindingValid(key) then return false,"Choose a keyboard key or mouse button or mouse button"end
  if key~=settings.smartDribbleKey then releaseHeld();settings.smartDribbleKey=key end
  return true
 end
 local ended=Input.InputEnded:Connect(function(input)if bindingMatches(input,settings.smartDribbleKey) then releaseHeld()end end)
 local blurred=Input.WindowFocusReleased:Connect(releaseHeld)
 local tickAt=0
 local connection=Run.Heartbeat:Connect(function()
  local elapsed=os.clock()-tickAt
  if elapsed<.025 then return end;tickAt=os.clock()
  local practiceOk,practiceError=pcall(updatePractice,elapsed)
  if not practiceOk then removePractice();status.status="Practice defender error: "..tostring(practiceError)end
  local ok,err=pcall(update)
  if not ok then status.status="Planner unavailable: "..tostring(err)end
  if held then status.lastHeldStatus=status.status end
 end)
 local previous=featureCleanup
 featureCleanup=function()
  stopped=true;releaseHeld();outstanding=nil;removePractice();settings._smartOffenseDirection=nil;settings._farmSelectDribble=nil
  began:Disconnect();ended:Disconnect();blurred:Disconnect();connection:Disconnect()
  previous()
 end
end)()end

-- END MEASURED SMART DRIBBLE


-- Defender alerts: edge-of-screen badges pointing at nearby opponents you cannot see.
-- Information only: reads positions, sends no input and moves nothing.
do (function()
 settings.defenderAlerts=false;settings.alertBallOnly=false;settings.alertRange=16
 numericLimits.alertRange={6,40}
 local style=settings._hudStyle
 local palette=style and style.c or {surface=Color3.fromRGB(17,20,29),stroke=Color3.fromRGB(40,47,64),
  text=Color3.fromRGB(246,248,252),sub=Color3.fromRGB(178,186,206),amber=Color3.fromRGB(255,200,102),danger=Color3.fromRGB(255,125,138)}
 local gui=create("ScreenGui",{Name="PracticalDefenderAlerts",ResetOnSpawn=false,IgnoreGuiInset=true,Enabled=false,DisplayOrder=6},P.PlayerGui)
 local badges={}
 for i=1,4 do
  local frame=create("Frame",{Name="Alert"..i,AnchorPoint=Vector2.new(.5,.5),Size=UDim2.fromOffset(38,38),BackgroundColor3=palette.surface,BackgroundTransparency=.05,BorderSizePixel=0,Visible=false},gui)
  create("UICorner",{CornerRadius=UDim.new(1,0)},frame)
  local stroke=create("UIStroke",{Color=palette.stroke,Thickness=1.5},frame)
  -- A small diamond sits on the outer side of the badge, on the side the opponent is.
  local pointer=create("Frame",{AnchorPoint=Vector2.new(.5,.5),Size=UDim2.fromOffset(11,11),Rotation=45,BackgroundColor3=palette.text,BorderSizePixel=0},frame)
  local number=create("TextLabel",{BackgroundTransparency=1,Size=UDim2.fromScale(1,1),Font=Enum.Font.GothamBold,TextSize=14,TextColor3=palette.text,Text=""},frame)
  if style then style.skin(frame,19,false);stroke.Thickness=1.5 end
  badges[i]={frame=frame,stroke=stroke,pointer=pointer,number=number}
 end
 local snapshots=setmetatable({},{__mode="k"})
 local function enemy(own,other)
  local a,b=attr(own,"Team"),attr(other,"Team")
  if a~=nil and b~=nil and a~=0 and b~=0 and a~=""and b~=""then return a~=b end
  local players=game:GetService("Players")
  local otherPlayer=players:GetPlayerFromCharacter(other)or players:FindFirstChild(other.Name)
  return otherPlayer~=nil and P.Team~=nil and otherPlayer.Team~=nil and otherPlayer.Team~=P.Team
 end
 local function sameCourt(a,b)
  local x,y=attr(a,"CourtNumber"),attr(b,"CourtNumber")
  return not(x~=nil and y~=nil and x~=0 and y~=0 and x~=y)
 end
 local function hideAll()
  gui.Enabled=false
  for _,badge in ipairs(badges)do badge.frame.Visible=false;badge.owner=nil;badge.x=nil end
 end
 local connection=(Run.RenderStepped or Run.Heartbeat):Connect(function(dt)
  if not settings.defenderAlerts then if gui.Enabled then hideAll()end;return end
  local camera=workspace.CurrentCamera
  local own=model();local root=rootOf(own)
  if not camera or not root or(settings.alertBallOnly and not holdsBall(own))then hideAll();return end
  local now=os.clock();local viewport=camera.ViewportSize
  local found={}
  for _,other in ipairs(defenseCharacters())do
   local r=rootOf(other)
   if r and other~=own and other.Name~="InvisCharacter" and r.Position.Y>-100 and enemy(own,other)and sameCourt(own,other)then
    local delta=flat(r.Position-root.Position);local distance=delta.Magnitude
    -- Opponent speed from position samples (replicated avatars; measured rather than assumed).
    local previous=snapshots[other]
    local velocity=previous and previous.velocity or Vector3.zero
    if not previous then snapshots[other]={position=r.Position,time=now,velocity=Vector3.zero}
    elseif now-previous.time>=.06 then
     if now-previous.time<.5 then
      velocity=flat(r.Position-previous.position)/(now-previous.time)
      if velocity.Magnitude>40 then velocity=Vector3.zero end
     else velocity=Vector3.zero end
     previous.position=r.Position;previous.time=now;previous.velocity=velocity
    end
    local closing=distance>.1 and -velocity:Dot(delta.Unit)or 0
    if distance<=settings.alertRange and(closing>2 or distance<=7)then
     found[#found+1]={character=other,position=r.Position,distance=distance,closing=closing}
    end
   end
  end
  table.sort(found,function(a,b)return a.distance<b.distance end)
  -- Keep each opponent on the same badge so it does not jump between them.
  local assigned={}
  for _,badge in ipairs(badges)do
   local keep=false
   for i=1,math.min(4,#found)do if found[i].character==badge.owner then keep=true;assigned[i]=badge end end
   if not keep then badge.owner=nil end
  end
  local blend=1-math.exp(-math.clamp(dt or 1/60,0,.1)*16)
  local anyShown=false;local used={}
  for i=1,math.min(4,#found)do
   local item=found[i];local badge=assigned[i]
   if not badge then for _,candidate in ipairs(badges)do if not candidate.owner then badge=candidate;badge.owner=item.character;break end end end
   if badge then
    used[badge]=true
    local screen,onScreen=camera:WorldToViewportPoint(item.position)
    if onScreen and screen.Z>0 then badge.frame.Visible=false;badge.x=nil -- visible on screen already
    else
     -- Direction toward the opponent in camera space (behind you points down the screen).
     local relative=camera.CFrame:PointToObjectSpace(item.position)
     local dx,dy=relative.X,-relative.Y
     if math.abs(dx)+math.abs(dy)<.01 then dx,dy=0,1 end
     local length=math.sqrt(dx*dx+dy*dy);dx,dy=dx/length,dy/length
     local halfX,halfY=viewport.X/2-50,viewport.Y/2-50
     local scale=math.min(halfX/math.max(math.abs(dx),1e-3),halfY/math.max(math.abs(dy),1e-3))
     local x,y=viewport.X/2+dx*scale,viewport.Y/2+dy*scale
     if not badge.x or math.abs(badge.x-x)+math.abs(badge.y-y)>400 then badge.x=x;badge.y=y
     else badge.x=badge.x+(x-badge.x)*blend;badge.y=badge.y+(y-badge.y)*blend end
     local color=(item.distance<=6 or item.closing>8)and palette.danger or(item.distance<=11 or item.closing>4)and palette.amber or palette.sub
     badge.frame.Position=UDim2.fromOffset(badge.x,badge.y);badge.frame.Visible=true
     badge.stroke.Color=color;badge.pointer.BackgroundColor3=color
     badge.pointer.Position=UDim2.new(.5,dx*25,.5,dy*25)
     badge.number.Text=tostring(math.floor(item.distance+.5))
     anyShown=true
    end
   end
  end
  for _,badge in ipairs(badges)do if not used[badge]then badge.frame.Visible=false;badge.owner=nil;badge.x=nil end end
  gui.Enabled=anyShown
 end)
 local previous=featureCleanup
 featureCleanup=function()connection:Disconnect();gui:Destroy();previous()end
end)() end
-- Early rebound positioning: while a shot is in the air, head for a spot on the line from the
-- hoop to you, `reboundSpot` studs from the rim. The existing landing-spot chase takes over as
-- soon as the ball is falling. Used only when Auto rebound + Pursue are on; WASD still overrides.
do (function()
 settings.reboundEarly=false;settings.reboundSpot=5.5
 numericLimits.reboundSpot={3,9}
 settings._reboundEarlySpot=function(root,ballPosition,ball)
  if not ball or not ball.Parent or attr(ball,"MidShot")~=true then return nil end
  -- The hoop this shot is heading for: the nearest assigned Goal to the ball.
  local hoop,best
  for _,character in ipairs(defenseCharacters())do
   local goal=goalPosition(attr(character,"Goal"))
   if goal then
    local distance=flat(goal-ballPosition).Magnitude
    if distance<45 and(not best or distance<best)then hoop=goal;best=distance end
   end
  end
  if not hoop then return nil end
  local fromHoop=flat(root.Position-hoop)
  if fromHoop.Magnitude<.5 or fromHoop.Magnitude>30 then return nil end
  -- The ball must still be up in the air near the rim, not already on its way down to the floor.
  if ballPosition.Y<root.Position.Y+2 then return nil end
  local spot=hoop+fromHoop.Unit*(settings.reboundSpot or 5.5)
  return clampCourt(Vector3.new(spot.X,root.Position.Y,spot.Z))
 end
 local previous=featureCleanup
 featureCleanup=function()settings._reboundEarlySpot=nil;previous()end
end)() end
-- Court volumes observed in the user's live geometry report; no map coordinates hard-coded.
do (function()
 local cacheAt=-math.huge
 local volume,divider,halfSign=nil,nil,1
 local function refresh()
  if os.clock()-cacheAt<.5 then return end
  cacheAt=os.clock();volume=nil;divider=nil
  local own=model();local root=rootOf(own);if not root then return end
  local assets=workspace:FindFirstChild("Assets")
  local bounds=assets and assets:FindFirstChild("CourtBounds")
  local nearest=math.huge
  if bounds then
   for _,part in ipairs(bounds:GetChildren())do
    if part:IsA("BasePart")then
     local localPoint=part.CFrame:PointToObjectSpace(root.Position)
     local excess=math.max(0,math.abs(localPoint.X)-part.Size.X*.5)^2+math.max(0,math.abs(localPoint.Z)-part.Size.Z*.5)^2
     if excess<nearest then nearest=excess;volume=part end
    end
   end
  end
  local map=workspace:FindFirstChild("Map");local courts=map and map:FindFirstChild("Courts")
  if not courts or not volume then return end
  local number=attr(own,"CourtNumber")
  for _,half in ipairs(courts:GetDescendants())do
   if half:IsA("Model")and half:GetAttribute("StartedGame")==true and half:GetAttribute("FullCourt")==false
    and tostring(half:GetAttribute("CourtNumber"))==tostring(number)then
    local out=half:FindFirstChild("OutOfBounds")
    local edge=out and out:FindFirstChild("HalfCourtBound")
    local violations=half:FindFirstChild("Violations")
    local inside=violations and violations:FindFirstChild("Backcourt")
    if edge and edge:IsA("BasePart")and inside and inside:IsA("BasePart")
     and (edge.Position-volume.Position).Magnitude<math.max(volume.Size.X,volume.Size.Z)then
     divider=edge
     halfSign=edge.CFrame:PointToObjectSpace(inside.Position).Z>=0 and 1 or -1
     break
    end
   end
  end
 end
 settings._courtClamp=function(point,margin)
  refresh();margin=margin or 2
  if not volume then return point end
  local q=volume.CFrame:PointToObjectSpace(point)
  local hx=math.max(.1,volume.Size.X*.5-margin);local hz=math.max(.1,volume.Size.Z*.5-margin)
  local result=volume.CFrame:PointToWorldSpace(Vector3.new(math.clamp(q.X,-hx,hx),q.Y,math.clamp(q.Z,-hz,hz)))
  if divider then
   local d=divider.CFrame:PointToObjectSpace(result)
   local limit=divider.Size.Z*.5+margin
   if d.Z*halfSign<limit then result=divider.CFrame:PointToWorldSpace(Vector3.new(d.X,d.Y,limit*halfSign))end
  end
  return Vector3.new(result.X,point.Y,result.Z)
 end
 settings._courtInside=function(point,margin)
  local limited=settings._courtClamp(point,margin)
  return flat(limited-point).Magnitude<.05,volume~=nil
 end
 settings._courtSteer=function(root,direction)
  if not root or not direction or direction.Magnitude<.01 then return direction end
  local origin=root.Position
  local inset=settings._courtClamp(origin,2.5)
  if flat(inset-origin).Magnitude>.2 then return flat(inset-origin).Unit end
  local lead=math.clamp(flat(root.AssemblyLinearVelocity).Magnitude*.25,3,8)
  local requested=origin+direction*lead
  local endpoint=settings._courtClamp(requested,2.5)
  local delta=flat(endpoint-origin)
  return delta.Magnitude>.2 and delta.Unit or Vector3.zero
 end
 local previous=featureCleanup
 featureCleanup=function()
  settings._courtClamp=nil;settings._courtInside=nil;settings._courtSteer=nil
  previous()
 end
end)() end

-- Blacktop autofarm (1v1): auto guard, loose-ball pursuit from anywhere, drives past the defender for dunks.
-- The toggle is saved to VantaAutofarm-v1.json. Each time the script runs with it ON, a 5 second prompt
-- appears; "No" turns it off, otherwise it readies up, queues itself for the next teleport and farms.
do (function()
local farmLoaded,farmError=pcall(function()
 ----------------------------------------------------------------------------------------------
 -- YOUR HOOKS. Fill these in; everything else is wired around them.
 ----------------------------------------------------------------------------------------------
 local state={status="Off",phase="off",ready=0,dunks=0,loose=0,crosses=0,queue="not queued"}
 -- Lobby queue: pick the 1v1 mode, then ready up. Both are InvokeServer calls, which wait for the
 -- server, so they run on their own thread and report back through the status line.
 -- (You can override this with getgenv().VantaFarmReady = function() ... end.)
 local function fireReady()
  task.spawn(function()
   local replies={}
   local ok,err=pcall(function()
    local service=game:GetService("ReplicatedStorage"):WaitForChild("Aero",5):WaitForChild("AeroRemoteServices",5):WaitForChild("QueueService",5)
    replies[1]=table.pack(service:WaitForChild("SelectMode",5):InvokeServer("1v1"))
    replies[2]=table.pack(service:WaitForChild("Ready",5):InvokeServer(true))
   end)
   local function show(pack)
    if not pack then return "no reply" end
    local parts={}
    for i=1,pack.n do parts[i]=tostring(pack[i])end
    return #parts>0 and table.concat(parts,",") or "nil"
   end
   state.status=ok and ("Ready sent · SelectMode replied "..show(replies[1]).." · Ready replied "..show(replies[2])) or "Ready failed: "..tostring(err)
  end)
  return true
 end
 -- Script that re-runs this suite after a teleport. Example:
 --   loadstring(game:HttpGet("https://your-host/PracticalComplete.luau"))()
 -- Left empty, the suite looks for PracticalComplete.luau in the executor's workspace folder.
 local AUTOEXEC_SOURCE=""
 -- Dunk input is exclusively a held Space key; E is reserved for standing jump shots.
 local spaceHeld,spaceMode,spaceStart,spaceSawDunk=false,nil,0,false
 local function releaseSpace(service)
  if not spaceHeld then return end
  spaceHeld=false
  pcall(function()game:GetService("VirtualInputManager"):SendKeyEvent(false,Enum.KeyCode.Space,false,game)end)
 end
 local function dunkAction(service,attempt)
  releaseSpace(service)
  -- Use the observed native Space hold only. A guessed Shoot/E fallback can become a fadeaway.
  game:GetService("VirtualInputManager"):SendKeyEvent(true,Enum.KeyCode.Space,false,game)
  spaceStart=os.clock();spaceSawDunk=false;spaceMode="key";spaceHeld=true
 end

 ----------------------------------------------------------------------------------------------

 settings.farmPostClamp=true
 settings.farmChatRequeue=true
 settings.farmRimStop=true;settings.farmVisuals=true;settings.farm=false;settings.farmCrossover=true;settings.farmDunkRange=12
 settings.farmStepbackAggression=70;settings.farmShoot=true;settings.farmShotRange=32;settings.farmRandomThrees=true
 numericLimits.farmStepbackAggression={0,100};numericLimits.farmDunkRange={6,24};numericLimits.farmShotRange={12,45}
 local CONFIG="VantaAutofarm-v1.json"
 local COUNTDOWN=5
 local plan={at=-math.huge}
 -- BEGIN FARM SPRINT
 local function syncSprint(service,wanted,now)
  if not service or not service.Sprint then return end
  if wanted then
   if plan.sprintOwned and now-(plan.sprintAt or 0)<.3 then return end
  elseif not plan.sprintOwned then return end
  local ok=pcall(function()service.Sprint:Fire(wanted==true)end)
  if ok then plan.sprintOwned=wanted==true;plan.sprintAt=now end
 end
 -- END FARM SPRINT
 -- BEGIN FARM PLANT
 local function plantFeet(service,now)
  plan.dir=Vector3.zero;plan.reason="Jump shot: feet planted";guardError=0
  settings._farmAttack=true;settings._farmSprintWanted=false;settings._farmPlantUntil=now+.2
  syncSprint(service,false,now)
  if settings._farmPlantFeet then settings._farmPlantFeet()end
 end
 -- END FARM PLANT
 local busy={Shooting=true,Dunking=true,Layup=true,Stunned=true,Dead=true,Diving=true,Stealing=true,Blocking=true,Rebounding=true}
 local running,stopped=false,false
 local testOpponent=nil
 local testPlayer=nil
 local perimeterNeeded=false
 local snapshot
 local lastReady,lastDunk,lastCross,lastCatch,lastThink=-math.huge,-math.huge,-math.huge,-math.huge,0
 local side,sideAt=1,-math.huge
 local rimCache,rimAt=nil,-math.huge
 local lastPrinted,noCharacterSince
 local lastCrossKeys,guarding
 local retreat,driveUntil,lastStepback=nil,0,-math.huge
 local stepbackShotUntil=0
 local confirmedStepback=false
 local possessionOwner=nil
 local openingUntil=0
 local farmMove=nil
 local dribbleFailures={}
 local dribbleHistory={}
 local function resetOffense(service)
  retreat=nil;driveUntil=0;stepbackShotUntil=0;confirmedStepback=false;farmMove=nil
  releaseSpace(service)
 end
 local peakSpeed=0
 local forcedAutoTime,autoTimeTry=false,-math.huge
 local madeUntil,lastBall,box=-math.huge,setmetatable({},{__mode="k"}),{}
 local sideLock,progressAt,progressDistance,crossCount=-math.huge,0,nil,0

 -- A held guard key is simulated while farming (and only while you do not have the ball).
 local baseBindingDown=bindingDown
 bindingDown=function(key)
  if settings._farmGuard and key==guardKey then return true end
  return baseBindingDown(key)
 end

 local function loadConfig()
  if type(readfile)~="function" then return false end
  local ok,value=pcall(function()return Http:JSONDecode(readfile(CONFIG))end)
  return ok and type(value)=="table" and value.enabled==true
 end
 local function saveConfig(on)
  if type(writefile)~="function" then state.queue="cannot save config (no writefile)";return end
  pcall(function()writefile(CONFIG,Http:JSONEncode({version=1,enabled=on==true}))end)
 end

 -- Guard, rim pressure and rebounds are enabled while farming; auto-steal stays off.
 local function forceDefense(on)
  if on and not snapshot then
   snapshot={rimPress=settings.rimPress,catchShoot=settings.catchShoot,smartDribble=settings.smartDribble,guard=settings.guard,steal=settings.steal,rebound=settings.rebound,reboundChase=settings.reboundChase,guardRange=settings.guardRange,antiOop=settings.antiOop,oopMove=settings.oopMove}
   settings.catchShoot=false;settings.smartDribble=false;settings.guard=true;settings.rimPress=true;settings.steal=false;settings.rebound=true;settings.reboundChase=true;settings.antiOop=true;settings.oopMove=true
   settings.guardRange=math.max(settings.guardRange,70)
  elseif not on and snapshot then
   for key,value in pairs(snapshot)do settings[key]=value end
   snapshot=nil
  end
  if not on and forcedAutoTime then
   forcedAutoTime=false;pcall(api.SetAutoTime,false)
  end
 end

 local function queueReexec()
  if env.VantaFarmQueued then return end
  local queue=queue_on_teleport or(syn and syn.queue_on_teleport)or(fluxus and fluxus.queue_on_teleport)
  if type(queue)~="function"then state.queue="queue_on_teleport unavailable";return end
  local source=env.VantaFarmSource
  if type(source)~="string"or source==""then source=AUTOEXEC_SOURCE end
  if source==""and type(isfile)=="function"and type(readfile)=="function"then
   local ok,exists=pcall(isfile,"PracticalComplete.luau")
   if ok and exists then source='loadstring(readfile("PracticalComplete.luau"))()'end
  end
  if source==""then state.queue="no auto-exec source (set AUTOEXEC_SOURCE or use your executor's autoexec folder)";return end
  if pcall(queue,source)then env.VantaFarmQueued=true;state.queue="queued for next teleport"else state.queue="queue failed"end
 end

 local function ballPart(value)
  if typeof(value)~="Instance"then return nil end
  if value:IsA("BasePart")then return value end
  if value:IsA("Model")then return value.PrimaryPart or value:FindFirstChildWhichIsA("BasePart",true)end
 end
 local function opponents(own)
  if testOpponent then return testOpponent.Parent and rootOf(testOpponent) and {testOpponent} or {} end
  local list={}
  local candidates,seen={},{}
  for _,character in ipairs(defenseCharacters())do candidates[#candidates+1]=character end
  for _,player in ipairs(game:GetService("Players"):GetPlayers())do
   if player.Character then candidates[#candidates+1]=player.Character end
  end
  local ownRoot=rootOf(own)
  for _,character in ipairs(candidates)do
   local r=rootOf(character)
   local humanoid=character:FindFirstChildOfClass("Humanoid")
   local valid=not seen[character] and(not humanoid or humanoid.Health>0)
    and ownRoot and r and math.abs(r.Position.Y-ownRoot.Position.Y)<15
   seen[character]=true
   if valid and character~=own and character.Name~="InvisCharacter"and r and r.Position.Y>-100 then
    local a,b=attr(own,"Team"),attr(character,"Team")
    local courtA,courtB=attr(own,"CourtNumber"),attr(character,"CourtNumber")
    local otherCourt=courtA~=nil and courtB~=nil and courtA~=0 and courtB~=0 and tostring(courtA)~=tostring(courtB)
    if not otherCourt and not(a~=nil and b~=nil and a~=0 and a~=""and a==b)then list[#list+1]=character end
   end
  end
  return list
 end
 local ballCache,ballCacheAt,ballFallbackAt={},-math.huge,-math.huge
 local function ballCandidates()
  local now=os.clock()
  if now-ballCacheAt<.3 then return ballCache end
  ballCacheAt=now
  local list={}
  local folder=workspace:FindFirstChild("Basketballs")
  if folder then for _,object in ipairs(folder:GetDescendants())do if object:IsA("BasePart")then list[#list+1]=object end end end
  -- Some courts keep the ball elsewhere: fall back to a slow scan for parts named like a basketball.
  if #list==0 and now-ballFallbackAt>2 then
   ballFallbackAt=now
   local characters=workspace:FindFirstChild("Characters")
   for _,object in ipairs(workspace:GetDescendants())do
    if object:IsA("BasePart")and object.Name:lower():find("basketball",1,true)and not(characters and object:IsDescendantOf(characters))then list[#list+1]=object end
   end
   ballCache=list;return list
  end
  if #list>0 or now-ballFallbackAt>2 then ballCache=list end
  return ballCache
 end
 local function looseBalls()
  local held={}
  for _,character in ipairs(defenseCharacters())do
   local part=ballPart(attr(character,"Basketball"))
   if part then held[part]=true;if part.Parent and part.Parent:IsA("Model")then held[part.Parent]=true end end
  end
  local list={}
  for _,object in ipairs(ballCandidates())do
   if object.Parent and object.Position.Y>-50 and((not held[object]and not held[object.Parent])or attr(object,"MidShot")==true)then list[#list+1]=object end
  end
  return list
 end
 -- Where the ball will be reachable: its landing spot in the air, a small lead when it rolls.
 local function ballTarget(ball,root)
  local position,velocity=ball.Position,ball.AssemblyLinearVelocity
  local gravity=workspace.Gravity
  local speed=math.clamp(math.max(flat(root.AssemblyLinearVelocity).Magnitude,16),16,26)
  local airborne=position.Y>root.Position.Y+1.5 or math.abs(velocity.Y)>4
  local best,bestError,bestTime=nil,math.huge,0
  -- Search simultaneous player/ball arrival times, not a fixed lead offset.
  for i=0,60 do
   local t=i*.05
   local y=position.Y+velocity.Y*t-(airborne and .5*gravity*t*t or 0)
   -- Stop at first predicted ground contact; a bounce needs a fresh observation.
   if airborne and y<root.Position.Y-2.5 then break end
   local point=Vector3.new(position.X+velocity.X*t,root.Position.Y,position.Z+velocity.Z*t)
   local heightOK=math.abs(y-root.Position.Y)<=4
   local travel=math.max(0,flat(point-root.Position).Magnitude-2)/speed+.12
   local error=math.max(0,travel-t)+(heightOK and 0 or 10)
   if error<bestError then best=point;bestError=error;bestTime=t end
   if heightOK and travel<=t then
    state.ballInterceptTime=t;state.ballInterceptReachable=true;return point
   end
  end
  state.ballInterceptTime=bestTime;state.ballInterceptReachable=false
  return best or Vector3.new(position.X,root.Position.Y,position.Z)
 end
 local rimCandidates,rimScanAt={},-math.huge
 local ballOwners=setmetatable({},{__mode="k"})
 local rimHits=setmetatable({},{__mode="k"})
 local rimWatches=setmetatable({},{__mode="k"})
 local clearBall,clearRim,clearTarget
 local rimSounds=setmetatable({},{__mode="k"})
 local function watchRim(part)
  if rimWatches[part]then return end
  rimWatches[part]=true
  on(part.Touched,function(hit)
   if not running then return end
   for _,ball in ipairs(ballCandidates())do
    if hit==ball or (ball.Parent and ball.Parent:IsA("Model")and hit:IsDescendantOf(ball.Parent))then
     local owner=ballOwners[ball]
     if owner~=model()then rimHits[ball]={rim=part.Position,at=os.clock()};state.clearReason="Rim touch observed"end
    end
   end
  end)
 end
 local function clearDirection(own,root)
  local ball=ballPart(attr(own,"Basketball"))
  if ball and not rimHits[ball]then
   for candidate,hit in pairs(rimHits)do
    if candidate.Parent and os.clock()-hit.at<20 and (candidate.Position-ball.Position).Magnitude<5 then
     rimHits[ball]=hit;rimHits[candidate]=nil;break
    end
   end
  end
  if not clearBall and ball and rimHits[ball]then
   clearBall=ball;clearRim=rimHits[ball].rim;clearTarget=nil
  end
  if not clearBall then return nil end
  if ball~=clearBall then clearBall=nil;clearRim=nil;clearTarget=nil;return nil end
  releaseSpace(nil)
  if not clearTarget then
   local radial=flat(root.Position-clearRim)
   if radial.Magnitude<1 then radial=flat(root.CFrame.LookVector)*-1 end
   local assets=workspace:FindFirstChild("Assets")
   local functionals=assets and assets:FindFirstChild("Functionals")
   local zones=functionals and functionals:FindFirstChild("Zones")
   local arc=zones and zones:FindFirstChild("ThreePoint")
   if arc then
    local params=RaycastParams.new();params.FilterType=Enum.RaycastFilterType.Include;params.FilterDescendantsInstances={arc}
    for _,height in ipairs({clearRim.Y,root.Position.Y,root.Position.Y-2.5})do
     local origin=Vector3.new(clearRim.X,height,clearRim.Z)
     local hit=workspace:Raycast(origin,radial.Unit*100,params)
     if hit then
      local radius=flat(hit.Position-clearRim).Magnitude
      if radius>=10 and radius<=90 then clearTarget={direction=radial.Unit,radius=radius+2};break end
     end
    end
   end
   if not clearTarget then state.status="Must clear ball: three-point arc not detected";return Vector3.zero end
  end
  local fromRim=flat(root.Position-clearRim)
  if fromRim:Dot(clearTarget.direction)>=clearTarget.radius then
   rimHits[clearBall]=nil;clearBall=nil;clearRim=nil;clearTarget=nil;state.status="Ball cleared beyond arc";return nil
  end
  local target=clearRim+clearTarget.direction*clearTarget.radius
  local delta=flat(target-root.Position)
  state.status=string.format("Clear ball before offense: %.1f studs to arc",delta.Magnitude)
  return delta.Magnitude>.1 and delta.Unit or Vector3.zero
 end
 local function rimFor(own,root,now)
  if now-rimScanAt>=2 then
   rimScanAt=now;rimCandidates={}
   for _,object in ipairs(workspace:GetDescendants())do
    local name=object.Name:lower():gsub("[%s_%-]","")
    if name=="rim"or name=="hoop"or name=="basketballrim"then
     local part=object:IsA("BasePart")and object or object:IsA("Model")and(object.PrimaryPart or object:FindFirstChildWhichIsA("BasePart",true))
     if part then rimCandidates[#rimCandidates+1]=part;watchRim(part)end
    end
   end
  end
  -- Select again each tick: cache the inventory, never a previous court's target.
  local nearest,best
  for _,part in ipairs(rimCandidates)do
   if part.Parent then
    local height=part.Position.Y-root.Position.Y
    local distance=flat(part.Position-root.Position).Magnitude
    if height>1 and height<18 and(not best or distance<best)then nearest=part;best=distance end
   end
  end
  if nearest then
   state.rim=nearest:GetFullName();state.rimDistance=best;state.rimsFound=#rimCandidates
   return nearest.Position
  end
  local fallback=goalPosition(attr(own,"Goal"))
  state.rim=fallback and "Goal fallback: no named rim found"or "No rim found";state.rimsFound=#rimCandidates
  state.rimDistance=fallback and flat(fallback-root.Position).Magnitude or nil
  return fallback
 end

 local rng=Random.new()
 local boundaryAt=-math.huge
 local outZones={}
 local function withinCourt(point)
  if settings._courtInside then
   local inside,known=settings._courtInside(point,2.5)
   if known then state.boundariesKnown=true;return inside end
  end
  if courtBounds and(point.X<courtBounds.minX+1.5 or point.X>courtBounds.maxX-1.5 or point.Z<courtBounds.minZ+1.5 or point.Z>courtBounds.maxZ-1.5)then return false end
  if os.clock()-boundaryAt>3 then
   boundaryAt=os.clock();outZones={}
   local assets=workspace:FindFirstChild("Assets")
   local functionals=assets and assets:FindFirstChild("Functionals")
   local zones=functionals and functionals:FindFirstChild("Zones")
   if zones then
    for _,obj in ipairs(zones:GetDescendants())do
     if obj:IsA("BasePart")then
      local path=obj:GetFullName():lower():gsub("[^%a]","")
      if path:find("outofbounds",1,true)then outZones[#outZones+1]=obj end
     end
    end
   end
  end
  for _,zone in ipairs(outZones)do
   if zone.Parent then
    local q=zone.CFrame:PointToObjectSpace(point);local half=zone.Size*.5
    if math.abs(q.X)<=half.X+1.5 and math.abs(q.Z)<=half.Z+1.5 and math.abs(q.Y)<=half.Y+5 then return false end
   end
  end
  state.boundariesKnown=courtBounds~=nil or #outZones>0
  return true
 end
 local function safeMove(a,b)
  for i=0,12 do if not withinCourt(a:Lerp(b,i/12))then return false end end
  return true
 end
 -- Request one measured move, then wait for native acknowledgement and landing.
 local function smartEscape(own,root,list,target,now,service,ping)
  if not settings.farmCrossover or not settings._farmSelectDribble or farmMove
   or not service or not service.Dribble or now-lastCross<.12
   or attr(own,"Action")=="Dribbling"or busy[attr(own,"Action")]
   or attr(own,"CanDribble")==false or attr(own,"CanMove")==false or attr(own,"InPost")==true then return false end
  local stamina,maximum=tonumber(attr(own,"Stamina")),tonumber(attr(own,"MaxStamina"))
  if stamina and maximum and maximum>0 then stamina=stamina/maximum*100 end
  if stamina and stamina<(settings.smartDribbleStamina or 30)then return false end
  local chosen=settings._farmSelectDribble({origin=root.Position,target=target,
   facing=root.CFrame.LookVector,right=root.CFrame.RightVector,velocity=root.AssemblyLinearVelocity,
   hand=attr(own,"Hand"),defenders=list,ping=ping,allowed=withinCourt,now=now,
   failed=dribbleFailures,lastKeys=lastCrossKeys,lastAt=lastCross,history=dribbleHistory})
  if not chosen then return false end
  if api.CancelCombo then api.CancelCombo()end
  lastCross=now
  local ok=pcall(function()service.Dribble:Fire(chosen.keys,chosen.sprint,Vector3.zero)end)
  if not ok then dribbleFailures[chosen.keys]=now+2;return false end
  lastCrossKeys=chosen.keys
  dribbleHistory[#dribbleHistory+1]={keys=chosen.keys,package=chosen.package,at=now}
  if #dribbleHistory>4 then table.remove(dribbleHistory,1)end
  farmMove={at=now,origin=root.Position,hand=attr(own,"Hand"),move=chosen,seen=false}
  state.crosses=state.crosses+1;state.dribblePlan=chosen.package.." ["..chosen.keys.."]"
  state.dribbleSide=flat(chosen.destination-root.Position):Dot(flat(root.CFrame.RightVector))<0 and "left"or "right"
  state.visualTarget=chosen.destination;state.status="Create space: "..state.dribblePlan
  plan.dir=Vector3.zero;plan.reason="Measured dribble: let animation move";guardError=0
  return true
 end
 -- Letting go of the stance is not enough in this game: the guard button has to be clicked once more
 -- to reset it, the same tap the key-up handler sends after you let go of G.
 local function releaseGuard()
  pcall(defensiveInputs,false,false,true)
  local service=keyboard and keyboard.Services and keyboard.Services.InputService
  if not service or not service.HoldG then return end
  pcall(function()service.HoldG:Fire({HoldingG=true})end)
  task.delay(.05,function()pcall(function()service.HoldG:Fire({HoldingG=false})end)end)
 end

 -- Out-of-play detection for loose balls: rim distance, recorded court bounds and futility.
 local giveUp=setmetatable({},{__mode="k"})
 local chaseBall,chaseSince,chaseGap,chaseGapAt
 local function rimPositions(own,root,now)
  local list={}
  for _,character in ipairs(defenseCharacters())do
   local goal=goalPosition(attr(character,"Goal"))
   if goal then list[#list+1]=goal end
  end
  local mine=rimFor(own,root,now)
  if mine then list[#list+1]=mine end
  return list
 end
 local function inPlay(ball,root,rims)
  local p=ball.Position
  if p.Y<root.Position.Y-25 or p.Y>root.Position.Y+60 then return false end
  if courtBounds and(p.X<courtBounds.minX-3 or p.X>courtBounds.maxX+3 or p.Z<courtBounds.minZ-3 or p.Z>courtBounds.maxZ+3)then return false end
  -- The court area as seen from where players actually stand; a ball well outside it is out of play.
  if box.minX and box.maxX-box.minX>35 and box.maxZ-box.minZ>35 then
   if p.X<box.minX-14 or p.X>box.maxX+14 or p.Z<box.minZ-14 or p.Z>box.maxZ+14 then return false end
  end
  if #rims>0 then
   local nearest=math.huge
   for _,rim in ipairs(rims)do nearest=math.min(nearest,flat(p-rim).Magnitude)end
   if nearest>70 then return false end
  end
  return true
 end

 -- ---------------------------------------------------------------------------------------------
 -- Offense. Read the defender, then pick one of: drive (lane open), steer around, back off to
 -- create space, or take an open shot. Dunks only come from a full-speed run at the rim.
 -- ---------------------------------------------------------------------------------------------
 local lastHeading,pressureSince,lastShot,shotStart=nil,nil,-math.huge,nil
 local shotSawAction=false
 local openSince,lastTight=nil,-math.huge
 local approachDir,resetting,resetSpot,resetUntil,settleSince=nil,false,nil,-math.huge,nil
 local dunkAttempts,slowSince=0,nil
 local recentPeak,recentAt=0,0
 local function defenders(own)
  local list={}
  for _,enemy in ipairs(opponents(own))do
   local er=rootOf(enemy)
   local velocity=flat(er.AssemblyLinearVelocity or Vector3.zero)
   if velocity.Magnitude>28 then velocity=velocity.Unit*28 end
   list[#list+1]={character=enemy,position=er.Position,velocity=velocity,facing=flat(er.CFrame.LookVector),predicted=er.Position+velocity*.4}
  end
  return list
 end
 local function segmentGap(point,a,b)
  local ab=flat(b-a);local length2=ab:Dot(ab)
  if length2<.01 then return flat(point-a).Magnitude end
  local t=math.clamp(flat(point-a):Dot(ab)/length2,0,1)
  return flat(point-(a+ab*t)).Magnitude
 end
 -- Score 16 headings: closeness to the rim (weighted by `wantRim`, negative backs away), how far the
 -- path and the end point stay from every defender, no running off the court, and no flip-flopping.
 local function chooseHeading(root,hoop,list,wantRim,speed)
  local origin=root.Position
  local toHoop=flat(hoop-origin)
  local hoopDir=toHoop.Magnitude>.1 and toHoop.Unit or Vector3.new(0,0,-1)
  local best,bestScore
  for i=0,15 do
   local angle=i*math.pi/8
   local dir=Vector3.new(math.cos(angle),0,math.sin(angle))
   local spot=origin+dir*math.min(speed*.45,9)
   local progress=toHoop.Magnitude-flat(hoop-spot).Magnitude
   local score=progress*wantRim
   local minGap=10
   for _,d in ipairs(list)do
    minGap=math.min(minGap,flat(spot-d.predicted).Magnitude)
    -- Compare simultaneous positions along the route; a defender behind us should not
    -- make every forward path look blocked just because they are close to the origin.
    for sample=1,4 do
     local t=sample*.1
     local ours=origin+dir*speed*t
     local theirs=d.position+d.velocity*t
     local gap=flat(ours-theirs).Magnitude
     if gap<2.6 then score=score-(2.6-gap)*5 end
    end
   end
   score=score+math.min(minGap,7)*.45
   if flat(spot-hoop).Magnitude>settings.farmShotRange+6 then score=score-6 end
   if courtBounds and(spot.X<courtBounds.minX or spot.X>courtBounds.maxX or spot.Z<courtBounds.minZ or spot.Z>courtBounds.maxZ)then score=score-8 end
   if lastHeading then score=score+dir:Dot(lastHeading)*1.2 end
   if not bestScore or score>bestScore then best=dir;bestScore=score end
  end
  lastHeading=best
  return best
 end
 -- Race along the path to the rim: could any defender be standing on it before we get there?
 -- If we win every point of the run, the dunk is on, whatever the lane looks like.
 local function canBeat(root,hoop,list,runSpeed,walk)
  local origin=root.Position
  local toHoop=flat(hoop-origin);local distance=toHoop.Magnitude
  if distance<2 or #list==0 then return true end
  local forward=toHoop.Unit
  local target=hoop-forward*2
  for i=1,12 do
   local point=origin+(target-origin)*(i/12)
   local playerTime=flat(point-origin).Magnitude/runSpeed
   for _,d in ipairs(list)do
    local speed=math.clamp(math.max(d.velocity.Magnitude,walk),10,26)
    local defenderTime=math.max(0,flat(point-d.position).Magnitude-2.1)/speed
    if defenderTime<=playerTime+.15 then return false end
   end
  end
  return true
 end
 local function dunkLaneOpen(root,hoop,list)
  return canBeat(root,hoop,list,math.max(16,flat(root.AssemblyLinearVelocity).Magnitude),16)
 end
 local function stepbackRequirements(ping)
  local aggression=math.clamp(settings.farmStepbackAggression or 70,0,100)/100
  return 8-3*aggression,.6-.45*aggression+ping
 end
 local function shotWindow(position,list,ping,stepbackShot)
  if #list==0 then return true,math.huge,math.huge end
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
  local requiredGap,requiredWindow=7,.58+ping
  if stepbackShot then requiredGap,requiredWindow=stepbackRequirements(ping)end
  return gap>=requiredGap and arrival>requiredWindow,gap,arrival
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
  local requiredGap,requiredWindow=stepbackRequirements(ping)
  return minimumGap>=requiredGap and minimumArrival>requiredWindow,minimumGap,minimumArrival
 end
 -- BEGIN FARM SHOT RANGE
 -- Detect the arc in this direction; corner and wing distances can differ.
 local function shotRange(root,hoop,now)
  local radial=flat(root.Position-hoop)
  local cached=plan.arc
  if not cached or now-cached.at>.5 or flat(cached.hoop-hoop).Magnitude>1
   or radial.Magnitude>1 and cached.direction:Dot(radial.Unit)<.995 then
   local radius=nil
   local assets=workspace:FindFirstChild("Assets")
   local functionals=assets and assets:FindFirstChild("Functionals")
   local zones=functionals and functionals:FindFirstChild("Zones")
   local arc=zones and zones:FindFirstChild("ThreePoint")
   if arc and radial.Magnitude>1 then
    local params=RaycastParams.new();params.FilterType=Enum.RaycastFilterType.Include;params.FilterDescendantsInstances={arc}
    for _,height in ipairs({hoop.Y,root.Position.Y,root.Position.Y-2.5})do
     local hit=workspace:Raycast(Vector3.new(hoop.X,height,hoop.Z),radial.Unit*100,params)
     local distance=hit and flat(hit.Position-hoop).Magnitude
     if distance and distance>=10 and distance<=90 then radius=distance;break end
    end
   end
   cached={at=now,hoop=hoop,direction=radial.Magnitude>1 and radial.Unit or Vector3.zero,radius=radius};plan.arc=cached
  end
  local limit=math.max(settings.farmShotRange,cached.radius and cached.radius+3 or 0)
  state.shotRange=limit;state.threePointRadius=cached.radius
  return limit,cached.radius
 end
 -- END FARM SHOT RANGE
 local function requestStandingShot(own,root,service,now,stepbackShot)
  local hoop=rimFor(own,root,now)
  if not hoop or not holdsBall(own)then return false,"No rim or possession"end
  local distance=flat(root.Position-hoop).Magnitude
  local assigned=goalPosition(attr(own,"Goal"))
  if assigned then distance=math.min(distance,flat(root.Position-assigned).Magnitude)end
  local finishRange=math.max(18,settings.farmDunkRange+6)
  if distance<=finishRange then return false,"Finishing zone: keep driving, no E shot"end
  if distance>shotRange(root,hoop,now)then return false,"Outside shot range"end
  if spaceHeld or attr(own,"CanShoot")==false or attr(own,"CanMove")==false
   or attr(own,"Action")=="Dribbling"or busy[attr(own,"Action")]then
   return false,"Wait for planted feet and completed dribble"
  end
  local ping=0;pcall(function()ping=math.clamp(P:GetNetworkPing(),0,.35)end)
  if not shotWindow(root.Position,defenders(own),ping,stepbackShot or(confirmedStepback and now<stepbackShotUntil))then return false,"Defender can contest actual release position"end
  plantFeet(service,now)
  local motion=root.AssemblyLinearVelocity
  if flat(motion).Magnitude>.35 or math.abs(motion.Y)>1 then return false,"Planting feet"end
  return pcall(function()service.Shoot:Fire({Shoot=true,Input="E"})end)
 end
 local lastShotVisual=nil
 local function captureShot(root,list,ping,reason)
  local allowed,gap,arrival=shotWindow(root.Position,list,ping)
  local nearest=nil;local best=math.huge
  for _,d in ipairs(list)do local n=flat(d.position-root.Position).Magnitude;if n<best then best=n;nearest=d.position end end
  lastShotVisual={at=os.clock(),origin=root.Position,defender=nearest,
   text=string.format("SHOT: %s\nGap %.1f studs | closeout %.2fs | required %.2fs",reason,gap,arrival,.58+ping)}
 end
 local function reposition(root,hoop,list)
  local origin=root.Position;local current=math.huge
  for _,d in ipairs(list)do current=math.min(current,flat(origin-d.position).Magnitude)end
  local best,bestScore=nil,-math.huge
  for i=0,15 do
   local angle=i*math.pi/8;local direction=Vector3.new(math.cos(angle),0,math.sin(angle))
   local endpoint=origin+direction*3
   if safeMove(origin,endpoint)and flat(endpoint-hoop).Magnitude<=flat(origin-hoop).Magnitude+1 then
    local gap=math.huge;local collision=false
    for _,d in ipairs(list)do
     local initial=flat(origin-d.position).Magnitude
     for sample=1,6 do
      local fraction=sample/6
      local distance=flat(origin+direction*(3*fraction)-(d.position+d.velocity*(.2*fraction))).Magnitude
      if distance<math.min(2.5,initial-.1)then collision=true end
     end
     gap=math.min(gap,flat(endpoint-(d.position+d.velocity*.2)).Magnitude)
    end
    local gain=gap-current
    local score=gain-flat(endpoint-hoop).Magnitude*.015
    if not collision and gain>.25 and score>bestScore then best={direction=direction,point=endpoint};bestScore=score end
   end
  end
  return best
 end
 -- BEGIN FARM ARRIVAL
 -- Earliest contest arrival: measured momentum, bounded acceleration, latency reserve.
 local function interceptTime(position,velocity,point,ping,facing)
  local delta=flat(point-position);local distance=math.max(0,delta.Magnitude-2.1)
  if distance==0 then return 0 end
  local top=math.clamp(math.max(16,velocity.Magnitude),16,26)
  local toward=delta.Magnitude>.01 and math.clamp(velocity:Dot(delta.Unit),-top,top)or 0
  local acceleration=60;local ramp=(top-toward)/acceleration
  local rampDistance=toward*ramp+.5*acceleration*ramp*ramp
  local travel=distance<=rampDistance and(-toward+math.sqrt(toward*toward+2*acceleration*distance))/acceleration
   or ramp+(distance-rampDistance)/top
  -- Facing is a small reaction cue, never permission to ignore a nearby defender.
  local turn=facing and facing.Magnitude>.1 and delta.Magnitude>.1
   and(1-math.clamp(facing.Unit:Dot(delta.Unit),-1,1))*.035 or 0
  return math.max(0,travel+turn-math.clamp(ping or 0,0,.35))
 end
 -- END FARM ARRIVAL
 local tactics={failures={},choice=nil,choiceAt=0,progress=nil}
 local function candidatePlans(root,hoop,list,ping)
  local origin=root.Position;local delta=flat(hoop-origin)
  local rangeLimit=shotRange(root,hoop,os.clock())
  if delta.Magnitude<.1 then return {}end
  local forward=delta.Unit;local right=Vector3.new(-forward.Z,0,forward.X)
  local speed=math.clamp(math.max(16,flat(root.AssemblyLinearVelocity).Magnitude),16,26)
  local velocity=flat(root.AssemblyLinearVelocity)
  local entries={}
  local function drive(name,point,bend)
   point=clampCourt(point);bend=bend or Vector3.zero
   local control=(origin+point)*.5+bend
   local heading=flat(control-origin)
   local turn=heading.Magnitude>.1 and velocity.Magnitude>1
    and(1-math.clamp(velocity.Unit:Dot(heading.Unit),-1,1))*math.min(velocity.Magnitude/80,.25)or 0
   local facing=flat(root.CFrame.LookVector)
   if heading.Magnitude>.1 then turn=turn+(1-math.clamp(facing:Dot(heading.Unit),-1,1))*.035 end
   local previous=origin;local length=0;local margin=math.huge;local bounded=true
   local futureMargin=math.huge;local nearProgress=0
   local limiting,intercept,ourTime,theirTime=nil,nil,0,0
   for i=1,20 do
    local t=i/20
    local pos=origin*((1-t)^2)+control*(2*(1-t)*t)+point*(t*t)
    if not withinCourt(pos)then bounded=false end
    length=length+flat(pos-previous).Magnitude;previous=pos
    local ours=length/speed+turn*math.min(1,length/3)
    if ours<=.65 then nearProgress=delta.Magnitude-flat(pos-hoop).Magnitude end
    for _,d in ipairs(list)do
     local theirs=interceptTime(d.position,d.velocity,pos,ping,d.facing)
     futureMargin=math.min(futureMargin,theirs-ours)
     if ours<=.65 and theirs-ours<margin then
      margin=theirs-ours;limiting=d.position;intercept=pos;ourTime=ours;theirTime=theirs
     end
    end
   end
   local travel=length/speed
   -- Seconds of minimum interception clearance dominate; progress breaks close ties.
   local score=math.clamp(margin,-2,.6)*6+nearProgress*1.5-travel*.4
   -- Do not let a tempting first step hide a covered finish farther along the route.
   score=score+math.clamp(futureMargin,-1,0)*3
   local stalled=tactics.failures[name] and os.clock()<tactics.failures[name]
   if stalled then score=score-8 end
   if not bounded then margin=-math.huge;score=-math.huge end
   local quality=margin>.15 and not stalled and "good"or(margin>0 and "uncertain"or "bad")
   entries[#entries+1]={kind="drive",name=name,point=point,bend=bend,origin=origin,score=score,
    margin=margin,futureMargin=futureMargin,defender=limiting,intercept=intercept,ourTime=ourTime,theirTime=theirTime,
    quality=quality,text=string.format("%s | score %.1f\nNext 0.65s: you %.2fs / defender %.2fs | margin %+.2fs",name,score,ourTime,theirTime,margin)}
  end
  drive("DRIVE LEFT",origin+forward*8-right*6,-right*1.5)
  drive("DRIVE RIM",hoop-forward*2,Vector3.zero)
  drive("DRIVE RIGHT",origin+forward*8+right*6,right*1.5)
  drive("WIDE LEFT",origin+forward*4-right*9,-right*2)
  drive("WIDE RIGHT",origin+forward*4+right*9,right*2)
  drive("CUT LEFT",origin+forward*11-right*3,-right)
  drive("CUT RIGHT",origin+forward*11+right*3,right)
  local points={{"STEPBACK",clampCourt(origin-flat(root.CFrame.LookVector)*5.6)},{"CROSS LEFT",clampCourt(origin-right*5)},{"CROSS RIGHT",clampCourt(origin+right*5)},{"SHOOT HERE",origin}}
  for _,item in ipairs(points)do
   local name,point=item[1],item[2]
   local allowed,gap,arrival
   if name=="STEPBACK"then allowed,gap,arrival=stepbackWindow(point,list,ping)
   else allowed,gap,arrival=shotWindow(point,list,ping,name=="SHOOT HERE"and confirmedStepback)end
   local range=flat(point-hoop).Magnitude
   local reason=string.format("Gap %.1f | closeout %.2fs",gap,arrival)
   if name=="STEPBACK"then
    local requiredGap,requiredWindow=stepbackRequirements(ping)
    reason=string.format("Setup: gap %.1f / need %.1f | window %.2fs / need %.2fs",gap,requiredGap,arrival,requiredWindow)
    if not allowed then reason=(gap<requiredGap and "Landing covered: "or "Closeout too soon: ")..reason end
   end
   if not safeMove(origin,point)then allowed=false;reason="Move crosses court boundary"
   elseif range<=settings.farmDunkRange+6 then allowed=false;reason="Finishing zone: keep attacking"
   elseif range>rangeLimit then allowed=false;reason=string.format("Landing %.1f studs / range limit %.1f",range,rangeLimit)
   elseif not settings.farmShoot then allowed=false;reason="Shots disabled"
   elseif not(api.GetAutoTime and api.GetAutoTime())then allowed=false;reason="Auto time is off"end
   entries[#entries+1]={name=name,point=point,quality=allowed and "good"or "bad",text=name.."\n"..reason}
  end
  return entries
 end
 -- BEGIN FARM POLICY
 local function farmChoice(c)
  if c.finish then return "finish" end
  if c.easyDunk then return "drive" end
  if c.wideOpen and c.shotEnabled and c.inRange and c.ready and not c.dribbling then return "shoot" end
  if c.confirmedStepback and c.stepbackWindow and c.shotEnabled and c.inRange and c.open and c.ready and not c.dribbling then return "shoot" end
  if c.stepback then return "stepback" end
  if c.beaten or c.laneClear then return "drive" end
  -- Covered possessions keep creating a drive or a safe stepback.
  if c.stepback then return "stepback" end
  return "create"
 end
 -- END FARM POLICY
 local function retreatingDefender(gap,ahead,lateral,rimSpeed,awaySpeed)
  return gap>=5 and gap<=14 and ahead>2 and lateral<=6
   and rimSpeed>=3 and awaySpeed>=2
 end
 local function stationaryShot(speed,vertical,action,canMove,now,lastMotion)
  local moving=speed>.35 or math.abs(vertical)>1 or action=="Dribbling" or canMove==false
  if moving then return false,now end
  return true,lastMotion -- No extra timer after native movement has actually stopped.
 end
 local lastShotMotion=-math.huge
 local function releaseShot(service)
  if not shotStart then return end
  shotStart=nil;state.shotActionAt=nil;retreat=nil;confirmedStepback=false;settings._farmPlantUntil=nil
  if service and service.Shoot then pcall(function()service.Shoot:Fire({Shoot=false})end)end
 end
 -- BEGIN FARM QUICK STEPBACK
 -- Preserve native XX motion, then request E on the first completed-move frame.
 local function stepbackAndShoot(service,now)
  local own=model();local root=rootOf(own)
  if not root or not service or not service.Dribble or not service.Shoot then return false,"Shot controller unavailable"end
  if not holdsBall(own)or spaceHeld or shotStart or not settings.farmShoot
   or not(api.GetAutoTime and api.GetAutoTime())or attr(own,"CanDribble")==false
   or attr(own,"Action")=="Dribbling"or busy[attr(own,"Action")]then return false,"Stepback unavailable"end
  local hoop=rimFor(own,root,now)
  if not hoop then return false,"No rim"end
  local landing=root.Position-flat(root.CFrame.LookVector)*5.6
  local range=flat(landing-hoop).Magnitude
  local ping=0;pcall(function()ping=math.clamp(P:GetNetworkPing(),0,.35)end)
  if range<=math.max(18,settings.farmDunkRange+6)or range>shotRange(root,hoop,now)
   or not safeMove(root.Position,landing)or not stepbackWindow(landing,defenders(own),ping)then return false,"No safe backstep shot"end
  plantFeet(service,now)
  local ok,err=pcall(function()service.Dribble:Fire("XX",true,Vector3.zero)end)
  if not ok then return false,err end
  lastStepback=now;retreat={at=now,origin=root.Position,seen=false}
  confirmedStepback=false;stepbackShotUntil=0
  plan.reason="Stepback: hold position for immediate landing shot";state.status="Stepback: shoot as native move ends"
  return true
 end
 -- END FARM QUICK STEPBACK
 local preferredDrive="DRIVE RIGHT"
 local function bestDrive(options)
  local best=nil
  for _,candidate in ipairs(options)do
   if candidate.kind=="drive"and candidate.quality~="bad"then
    local held=tactics.choice==candidate.name and os.clock()-tactics.choiceAt<.45
    local candidateScore=candidate.score+(held and .8 or 0)
    local bestScore=best and(best.score+((tactics.choice==best.name and os.clock()-tactics.choiceAt<.45)and .8 or 0))or -math.huge
    if not best or candidateScore>bestScore+.75
     or(math.abs(candidateScore-bestScore)<=.75 and candidate.name==preferredDrive)then best=candidate end
   end
  end
  return best
 end
 local escapePlan,allBlockedAt=nil,nil
 local coveredSince=nil
 local resetScanAt=-math.huge
 -- BEGIN FARM OPEN SHOT
 -- Real scoring chances precede the possession sprint and optional combo budget.
 local function takeOpenShot(own,root,now,service)
  if not settings.farmShoot or not(api.GetAutoTime and api.GetAutoTime())or shotStart or spaceHeld
   or retreat or confirmedStepback or farmMove or not service or not service.Shoot
   or attr(own,"Action")=="Dribbling"or busy[attr(own,"Action")]
   or attr(own,"CanShoot")==false or attr(own,"CanMove")==false or now-lastShot<=.75 then return false end
  local hoop=rimFor(own,root,now)
  if not hoop then return false end
  local distance=flat(root.Position-hoop).Magnitude
  local limit,arc=shotRange(root,hoop,now)
  if distance<=math.max(18,settings.farmDunkRange+6)or distance>limit then return false end
  local ping=0;pcall(function()ping=math.clamp(P:GetNetworkPing(),0,.35)end)
  local list=defenders(own)
  local open,gap=shotWindow(root.Position,list,ping)
  if not open or gap<10 then return false end
  escapePlan=nil;coveredSince=nil;perimeterNeeded=false;resetting=false;openingUntil=0
  state.decision="shoot";state.visualTarget=root.Position
  captureShot(root,list,ping,arc and distance>=arc+1 and "open three"or "wide-open shot")
  local ok,reason=requestStandingShot(own,root,service,now)
  if ok then
   lastShot=now;shotStart=now;shotSawAction=false;state.shotActionAt=nil
   state.shots=(state.shots or 0)+1;state.status=arc and distance>=arc+1 and "Open three: shot requested"or "Wide open: shot requested"
  else state.status="Open shot: "..tostring(reason)end
  return true
 end
 -- END FARM OPEN SHOT
 -- BEGIN FARM RESET POLICY
 local function shouldReset(now,since,drive,scoring,forced)
  local strong=drive and drive.quality=="good"and(drive.futureMargin or 0)>-.15
  if scoring or strong and not forced then return nil,false end
  since=since or now
  return since,forced or now-since>=1.2
 end
 -- END FARM RESET POLICY
 local function offense(own,root,now,service)
  local hoop=rimFor(own,root,now)
  if not hoop then state.status="Have ball, no rim found";return end
  local toHoop=flat(hoop-root.Position);local distance=toHoop.Magnitude
  -- Remember which way the court is, so a reset knows where "back out" is even from under the rim.
  if distance>8 then approachDir=(-toHoop).Unit end
  local forward=distance>.1 and toHoop.Unit or(approachDir and -approachDir)or Vector3.new(0,0,-1)
  state.visualOffense=true;state.visualRim=hoop;state.distance=distance
  -- Measure progress toward the chosen waypoint; sideways routes need not approach the rim.
  if tactics.progress and not escapePlan and now-tactics.progress.at>=.85 then
   local p=tactics.progress
   local gain=p.gap-flat(p.point-root.Position).Magnitude
   if gain<.75 and attr(own,"Action")~="Dribbling"then
    tactics.failures[p.name]=now+2.5;tactics.choice=nil
    state.routeRead="Stalled "..p.name..": try another route"
   end
   tactics.progress=nil
  elseif escapePlan then tactics.progress=nil end
  local right=Vector3.new(-forward.Z,0,forward.X)
  local list=defenders(own)
  if retreat or confirmedStepback then escapePlan=nil;resetting=false;allBlockedAt=nil end
  if escapePlan then
   local resetPing=0;pcall(function()resetPing=math.clamp(P:GetNetworkPing(),0,.35)end)
   local fresh=candidatePlans(root,hoop,list,resetPing)
   local drive=bestDrive(fresh)
   local scoreAvailable=distance<=settings.farmDunkRange or drive and drive.quality=="good"and drive.futureMargin>-.15
   for _,option in ipairs(fresh)do
    if option.quality=="good"and attr(own,"CanShoot")~=false and now-lastShot>.75 then
     if option.name=="SHOOT HERE"then
      local open,gap=shotWindow(root.Position,list,resetPing,false)
      if open and gap>=10 then scoreAvailable=true end
     elseif option.name=="STEPBACK"and now-lastStepback>1.6 and attr(own,"CanDribble")~=false then scoreAvailable=true end
    end
   end
   if scoreAvailable then escapePlan=nil;allBlockedAt=nil;coveredSince=nil;perimeterNeeded=false;state.status="Space recovered: attack scoring opportunity"end
  end
  if escapePlan then
   if escapePlan.waypoint and flat(escapePlan.waypoint-root.Position).Magnitude<2 then
    escapePlan.waypoint=nil;escapePlan.progressAt=now;escapePlan.gap=flat(escapePlan.point-root.Position).Magnitude
   end
   local delta=flat((escapePlan.waypoint or escapePlan.point)-root.Position)
   local unsafe=not safeMove(root.Position,escapePlan.waypoint or escapePlan.point)
   local pressure=math.huge
   for _,d in ipairs(list)do
    pressure=math.min(pressure,flat(d.position-root.Position).Magnitude)
    for i=1,4 do
     local t=i*.1
     local probe=root.Position+(delta.Magnitude>.1 and delta.Unit or Vector3.zero)*math.min(delta.Magnitude,16*t)
     if flat(probe-(d.position+d.velocity*t)).Magnitude<2.4 then unsafe=true end
    end
   end
   if delta.Magnitude<1.5 or now-escapePlan.at>5 then escapePlan=nil;allBlockedAt=nil;coveredSince=nil;perimeterNeeded=false
   elseif unsafe then
    local target=escapePlan.waypoint or escapePlan.point
    local resetPing=0;pcall(function()resetPing=math.clamp(P:GetNetworkPing(),0,.35)end)
    if smartEscape(own,root,list,target,now,service,resetPing)then return end
    escapePlan=nil;allBlockedAt=now-.5;state.status="Reset path covered: choose another route"
   elseif now-(escapePlan.progressAt or escapePlan.at)>1.2 and (escapePlan.gap or delta.Magnitude)-delta.Magnitude<.5 then
    escapePlan=nil;allBlockedAt=nil;state.status="Reset route stalled: reassess"
   elseif now-(escapePlan.progressAt or escapePlan.at)>1.2 then
    escapePlan.progressAt=now;escapePlan.gap=delta.Magnitude
    plan.dir=delta.Unit;plan.reason="Reset to perimeter";state.visualTarget=escapePlan.point;guardError=60;return
   else
    local resetPing=0;pcall(function()resetPing=math.clamp(P:GetNetworkPing(),0,.35)end)
    if pressure<8 and smartEscape(own,root,list,escapePlan.waypoint or escapePlan.point,now,service,resetPing)then return end
    plan.dir=delta.Unit;plan.reason="Reset to perimeter";state.visualTarget=escapePlan.point
    state.status="No good opening: reset to the arc";guardError=60;return
   end
  end
  -- How close the nearest defender will be, and whether any defender that is still ahead of us
  -- stands in the path to the rim.
  local gap,laneGap=math.huge,math.huge
  local currentGap=math.huge
  for _,d in ipairs(list)do
   currentGap=math.min(currentGap,flat(d.position-root.Position).Magnitude)
   local offset=flat(d.predicted-root.Position)
   gap=math.min(gap,offset.Magnitude)
   if offset:Dot(forward)>-1.5 then laneGap=math.min(laneGap,segmentGap(d.predicted,root.Position,hoop))end
  end
  local laneClear=dunkLaneOpen(root,hoop,list)
  local winsRace=canBeat(root,hoop,list,math.max(peakSpeed,16),tonumber(attr(own,"WalkSpeed"))or 13.5)
  if winsRace then laneClear=true end
  -- Every defender is behind us (and we are past them): nobody can stop the run, so just go and dunk.
  local beaten=#list>0
  for _,d in ipairs(list)do
   if flat(d.predicted-root.Position):Dot(forward)>-1 then beaten=false end
  end
  if beaten then laneClear=true end
  local velocity=flat(root.AssemblyLinearVelocity or Vector3.zero);local speed=velocity.Magnitude
  if speed>peakSpeed then peakSpeed=math.min(speed,45)end
  local runSpeed=math.clamp(math.max(speed,tonumber(attr(own,"WalkSpeed"))or 16),12,26)
  local walk=tonumber(attr(own,"WalkSpeed"))or 13.5
  -- A good run is most of your best speed this match, but never above a sane cap (one boosted frame
  -- must not make the bar unreachable).
  -- "Running" means not slowing down: close to the best speed of the last few seconds.
  recentPeak=math.max(speed,recentPeak-(now-recentAt)*4);recentAt=now
  local needed=math.clamp(walk*.55,5,9) -- A prior boosted frame must not block a normal-speed dunk.
  local facing=speed>1 and velocity.Unit:Dot(forward)or 0
  local runningAtRim=speed>=needed and facing>.75
  if distance>14 then dunkAttempts=0 end
  if distance<=5 and speed<5 then slowSince=slowSince or now else slowSince=nil end
  if gap<5 and not laneClear then pressureSince=pressureSince or now else pressureSince=nil end
  local pressure=pressureSince and now-pressureSince or 0
  state.seen=string.format("gap %.1f · lane %s%s · run-up %.0f/%.0f · aim %.0f%%",gap==math.huge and 99 or gap,laneClear and "open" or "blocked",winsRace and " (race won)" or "",speed,needed,math.max(facing,0)*100)

  if spaceHeld then
   plan.dir=forward;plan.reason="Autofarm: dunk approach";guardError=60
   state.status="Dunk input held: continuing approach";return
  end
  -- Native finish request is distance-based. Do not veto it with predicted defenders,
  -- run-up speed, facing estimates, or a previously planned retreat.
  if distance<=settings.farmDunkRange and now-lastDunk>.8 and not shotStart
   and attr(own,"Action")~="Dribbling" and service and service.Shoot then
   resetting=false;retreat=nil;stepbackShotUntil=0
   plan.dir=forward;plan.reason="Autofarm: dunk in range";guardError=60
   dunkAttempts=dunkAttempts+1;lastDunk=now
   local ok,err=pcall(dunkAction,service,dunkAttempts)
   if ok then state.dunks=state.dunks+1;state.status=string.format("Dunk requested: nearest rim %.1f / %.1f studs",distance,settings.farmDunkRange)
   else state.status="Dunk input failed: "..tostring(err)end
   return
  end
  -- Reset: a dunk needs a run at the rim. If we are slow near the rim (under it, stopped by a defender,
  -- or a finish that did not go), run back out to the arc, then attack again from there.
  if resetting and laneClear and distance>=math.max(5,settings.farmDunkRange-3) then resetting=false;lastHeading=nil end
  if resetting then
   local toSpot=flat(resetSpot-root.Position)
   if toSpot.Magnitude<2.5 or now>resetUntil then resetting=false;lastHeading=nil
   else
    plan.dir=toSpot.Unit;plan.reason="Autofarm: resetting";guardError=60
    state.status=string.format("Resetting: running back out for a run-up (%.0f studs to go)",toSpot.Magnitude)
    return
   end
  end
  -- Only back out after two finishes that did not work, or when stopped near the rim for almost a second.
  if not shotStart and distance<=3 and now-lastDunk>1.5 and dunkAttempts>=2 and(slowSince and now-slowSince>1.2)then
   dunkAttempts=0;slowSince=nil
   local base=approachDir or Vector3.new(1,0,0)
   local length=math.clamp(settings.farmDunkRange-2,6,10) -- Short run-up, never retreat to the arc.
   local bestSpot,bestScore
   -- Several spots around the way back out; take the one farthest from the defender.
   for _,degrees in ipairs({0,25,-25,50,-50})do
    local radians=math.rad(degrees);local c,s=math.cos(radians),math.sin(radians)
    local spotDir=Vector3.new(base.X*c-base.Z*s,0,base.X*s+base.Z*c)
    local spot=hoop+spotDir*length
    local clamped=clampCourt(Vector3.new(spot.X,root.Position.Y,spot.Z))
    local score=-math.abs(degrees)*.05
    for _,d in ipairs(list)do score=score+math.min(flat(clamped-d.predicted).Magnitude,12)*.3 end
    if not bestScore or score>bestScore then bestSpot=clamped;bestScore=score end
   end
   resetting=true;resetSpot=bestSpot;resetUntil=now+3.5;lastHeading=nil;state.resets=(state.resets or 0)+1
   local toSpot=flat(resetSpot-root.Position)
   plan.dir=toSpot.Magnitude>.1 and toSpot.Unit or Vector3.zero;plan.reason="Autofarm: resetting";guardError=60
   state.status="Resetting: running back out for a run-up"
   return
  end

  -- A shot already going up: keep still and let Auto time release it; release it ourselves if it hangs.
  if shotStart then
   if now-shotStart>1.8 then releaseShot(service)else plan.dir=Vector3.zero;plan.reason="Shooting";state.status="Shot up, waiting for release";return end
  end

  -- A window is evaluated at the current position, not just a single .4-second prediction.
  -- Estimate when the closest defender can enter contest distance, including network delay.
  local ping=0
  pcall(function()ping=math.clamp(P:GetNetworkPing(),0,.35)end)
  local earliest=math.huge
  for _,d in ipairs(list)do
   local delta=flat(root.Position-d.position)
   local closing=delta.Magnitude>.1 and math.max(0,d.velocity:Dot(delta.Unit))or 0
   local time=math.max(0,delta.Magnitude-3.5)/math.max(10,closing)
   earliest=math.min(earliest,time)
  end
  local autoTimeOn=api.GetAutoTime and api.GetAutoTime()==true
  local rangeLimit=shotRange(root,hoop,now)
  local inRange=distance>settings.farmDunkRange+6 and distance<=rangeLimit
  local open,shotGap,shotArrival=shotWindow(root.Position,list,ping,(confirmedStepback and now<stepbackShotUntil)or(retreat and not retreat.lateral))
  local wideOpen=open and shotGap>=10
  local action=attr(own,"Action")
  local ready=attr(own,"CanShoot")~=false and now-lastShot>.75
  state.shotWhy=not settings.farmShoot and "shots disabled" or not autoTimeOn and "Auto time is off"
   or not inRange and "outside configured shot range" or not open and "defender can close before release"
   or not ready and "shot not ready" or "open window"
  state.closeout=shotArrival;state.distance=distance;state.gap=currentGap

  local easyDunk=distance<=math.max(18,settings.farmDunkRange+6)
  if easyDunk and retreat then
   retreat=nil;confirmedStepback=false;stepbackShotUntil=0
  end
  -- Let a measured retreat finish; do not override its glide with forward steering.
  if retreat then
   plantFeet(service,now)
   local elapsed=now-retreat.at
   if action=="Dribbling" then retreat.seen=true end
   local moved=flat(root.Position-retreat.origin).Magnitude
   local completed=action~="Dribbling"and(retreat.seen or moved>2 and attr(own,"CanShoot")==true)
   if completed or elapsed>1.15 then
    local finished=retreat
    if completed and moved>2 then
     if not finished.lateral then
      local measured=math.clamp(elapsed,.12,.9)
      state.observedStepbackDuration=state.observedStepbackDuration and(state.observedStepbackDuration*.7+measured*.3)or measured
     end
     stepbackShotUntil=now+2.5;confirmedStepback=true
    else confirmedStepback=false end
    retreat=nil
    state.stepbackClosedAt=nil
    state.status=confirmedStepback and "Stepback complete: committing to release"or "Stepback cancelled: move not confirmed"
   else
    plan.dir=retreat.direction or Vector3.zero;plan.reason="Autofarm: stepback";state.status="Stepback: waiting for separation";return
   end
  end
  -- Own the movement plan through landing and release; do not restart the drive while planting.
  if confirmedStepback then
   plantFeet(service,now)
   plan.dir=Vector3.zero;plan.reason="Stepback: plant and release";state.visualTarget=root.Position
   local validRange=distance>math.max(18,settings.farmDunkRange+6)and distance<=rangeLimit
   local cancelReason=not validRange and "landing outside shot range"
    or not settings.farmShoot and "shots disabled"
    or not autoTimeOn and "Auto time disabled"
    or not open and "landing covered: relocate for three"
    or now>=stepbackShotUntil and "release window expired"
   if cancelReason then
    if not open then perimeterNeeded=true end
    confirmedStepback=false;stepbackShotUntil=0;state.stepbackClosedAt=nil
    state.status="Stepback cancelled: "..cancelReason
    return
   end
   if not ready then state.status="Stepback committed: shot not ready";return end
   local ok,reason=requestStandingShot(own,root,service,now,true)
   if ok then
    confirmedStepback=false;stepbackShotUntil=0;lastShot=now;shotStart=now;shotSawAction=false
    state.shots=(state.shots or 0)+1;state.status="Stepback committed: shot requested"
   else state.status="Stepback committed: "..tostring(reason)end
   return
  end
  local finish=false -- Distance-based dunk request is handled before tactical decisions.
  local facingRim=flat(root.CFrame.LookVector):Dot(forward)>.7
  local retreatEnd=clampCourt(root.Position-flat(root.CFrame.LookVector)*5.6)
  state.visualStepback=retreatEnd
  local retreatAllowed=settings.farmShoot and autoTimeOn and inRange and ready
   and now-lastStepback>(1.6-.8*math.clamp(settings.farmStepbackAggression or 70,0,100)/100) and action~="Dribbling" and attr(own,"CanDribble")~=false
   and flat(retreatEnd-hoop).Magnitude<=rangeLimit and #list>0 and safeMove(root.Position,retreatEnd)
  if retreatAllowed then
   for _,d in ipairs(list)do
    if segmentGap(d.position,root.Position,retreatEnd)<2 then retreatAllowed=false end
   end
  end
  -- A spaced defender can concede a pull-up even when the drive lane is technically clear.
  -- Compare their travel time to the measured stepback duration plus shot release and latency.
  local stepbackCloseout=math.huge
  for _,d in ipairs(list)do
   local delta=flat(retreatEnd-(d.position+d.velocity*.4))
   local toward=delta.Magnitude>.1 and math.max(0,d.velocity:Dot(delta.Unit))or 0
   local arrival=math.max(0,delta.Magnitude-3.5)/math.clamp(math.max(16,d.velocity.Magnitude),16,26)
   stepbackCloseout=math.min(stepbackCloseout,arrival)
  end
  -- Read the nearest defender's movement, not their facing animation.
  local nearest,nearestGap=nil,math.huge
  for _,d in ipairs(list)do
   local n=flat(d.position-root.Position).Magnitude
   if n<nearestGap then nearest=d;nearestGap=n end
  end
  state.visualDefender=nearest and nearest.position or nil
  local backingOff=false
  if nearest and nearestGap>.01 then
   local offset=flat(nearest.position-root.Position)
   local defenderVelocity=flat(nearest.velocity)
   backingOff=retreatingDefender(nearestGap,offset:Dot(forward),math.abs(offset:Dot(right)),
    defenderVelocity:Dot(forward),defenderVelocity:Dot(offset.Unit))
  end
  local safePullup,projectedGap,projectedArrival=stepbackWindow(retreatEnd,list,ping)
  stepbackCloseout=projectedArrival
  local smartRetreat=retreatAllowed and not easyDunk and safePullup
  state.stepbackCloseout=stepbackCloseout;state.defenderRetreating=backingOff
  state.pullupRead=smartRetreat and(backingOff and "Defender retreating to protect dunk: step back"
   or "Defender too far to close predicted stepback")or "Keep attacking / no safe pull-up window"
  local evaluated=candidatePlans(root,hoop,list,ping)
  state.planCandidates=evaluated;state.planEvaluatedAt=now;state.selectedCandidate=nil
  local choice=farmChoice({finish=finish,easyDunk=easyDunk,wideOpen=wideOpen,shotEnabled=settings.farmShoot and autoTimeOn,inRange=inRange,
   open=open,ready=ready,confirmedStepback=confirmedStepback,stepbackWindow=now<stepbackShotUntil,dribbling=action=="Dribbling",beaten=beaten,laneClear=laneClear,stepback=retreatAllowed and safePullup,smartRetreat=smartRetreat})
  state.decision=choice
  for _,candidate in ipairs(evaluated)do
   if(choice=="stepback"and candidate.name=="STEPBACK")or(choice=="shoot"and candidate.name=="SHOOT HERE")then
    state.selectedCandidate=candidate
   end
  end
  state.visualTarget=choice=="stepback"and retreatEnd or(choice=="shoot"and root.Position or hoop)
  if choice=="finish" and service and service.Shoot then
   plan.dir=forward;plan.reason="Autofarm: finishing";guardError=60
   dunkAttempts=dunkAttempts+1;lastDunk=now
   local ok,err=pcall(dunkAction,service,dunkAttempts)
   if ok then state.dunks=state.dunks+1;state.status="Attacking rim: dunk requested"
   else state.status="Dunk input failed: "..tostring(err)end
   return
  end
  if choice=="shoot" and (wideOpen or(confirmedStepback and now<stepbackShotUntil)) and open and inRange and service and service.Shoot then
   coveredSince=nil
   plantFeet(service,now)
   captureShot(root,list,ping,wideOpen and "wide-open shot"or "confirmed stepback")
   local ok,err=requestStandingShot(own,root,service,now)
   if ok then confirmedStepback=false;stepbackShotUntil=0;lastShot=now;shotStart=now;shotSawAction=false;state.shots=(state.shots or 0)+1;state.status="Open window: shot requested"
   else state.status="Shot input failed: "..tostring(err)end
   return
  end

  if choice=="stepback" and not perimeterNeeded then
   coveredSince=nil
   local ok,err=stepbackAndShoot(service,now)
   if not ok then state.status="Stepback / E input failed: "..tostring(err)end
   return
  end
  local options=evaluated
  local candidateDrive=bestDrive(options)
  local allRed=candidateDrive==nil
  local resetWanted
  coveredSince,resetWanted=shouldReset(now,coveredSince,candidateDrive,choice=="shoot",perimeterNeeded)
  if allRed or resetWanted then allBlockedAt=allBlockedAt or now else allBlockedAt=nil end
  if allBlockedAt and now-allBlockedAt>.4 and now-resetScanAt>.75 and not spaceHeld and not shotStart then
   resetScanAt=now
   local outward=-forward
   local detectedArc=settings.smartDribbleThreeDistance or 0
   local radius=detectedArc>0 and detectedArc+1.5 or settings.farmShotRange
   local assets=workspace:FindFirstChild("Assets")
   local functionals=assets and assets:FindFirstChild("Functionals")
   local zones=functionals and functionals:FindFirstChild("Zones")
   local arc=zones and zones:FindFirstChild("ThreePoint")
   local ray=RaycastParams.new();ray.FilterType=Enum.RaycastFilterType.Include
   if arc then ray.FilterDescendantsInstances={arc}end
   local point,bestReset,resetWaypoint=nil,-math.huge,nil
   local speed=math.max(16,flat(root.AssemblyLinearVelocity).Magnitude)
   local resetCandidates=0
   -- Sweep every 7.5 degrees around the hoop. Each direction gets its own
   -- measured arc radius, rather than reusing the radius behind the player.
   for index=0,47 do
    local angle=index*math.pi*2/48
    local heading=Vector3.new(math.cos(angle),0,math.sin(angle))
    local directionRadius=not arc and radius or nil
    if arc then
     for _,height in ipairs({hoop.Y,root.Position.Y,root.Position.Y-2.5})do
      local hit=workspace:Raycast(Vector3.new(hoop.X,height,hoop.Z),heading*100,ray)
      if hit then
       local found=flat(hit.Position-hoop).Magnitude
       if found>=10 and found<=90 then directionRadius=found+1;break end
      end
     end
    end
    local raw=hoop+heading*(directionRadius or radius)
    local endpoint=clampCourt(raw)
    if directionRadius and flat(endpoint-raw).Magnitude<1.5 and flat(endpoint-root.Position).Magnitude>2 and safeMove(root.Position,endpoint)then
     resetCandidates+=1
     local route=flat(endpoint-root.Position)
     local sideways=route.Magnitude>.1 and Vector3.new(-route.Z,0,route.X).Unit or right
     for _,bend in ipairs({0,-10,10,-18,18})do
      local waypoint=clampCourt((root.Position+endpoint)*.5+sideways*bend)
      if safeMove(root.Position,waypoint)and safeMove(waypoint,endpoint)then
       local first=flat(waypoint-root.Position).Magnitude
       local second=flat(endpoint-waypoint).Magnitude
       local travel=(first+second)/speed;local margin=math.huge;local collision=false
       for i=1,16 do
        local t=i/16
        local pos=t<=.5 and root.Position:Lerp(waypoint,t*2)or waypoint:Lerp(endpoint,(t-.5)*2)
        local elapsed=(t<=.5 and first*t*2 or first+second*(t-.5)*2)/speed
        for _,d in ipairs(list)do
         local gap=flat(pos-(d.position+d.velocity*math.min(elapsed+ping,.4))).Magnitude
         if gap<2.8 then collision=true end
         local arrival=interceptTime(d.position,d.velocity,pos,ping,d.facing)
         margin=math.min(margin,arrival-elapsed)
        end
       end
       local endpointOpen,endpointGap=shotWindow(endpoint,list,ping,false)
       local score=math.clamp(margin,-2,2)*10-travel+math.min(endpointGap,20)*.7+(endpointOpen and 8 or 0)
       if not collision and score>bestReset then point=endpoint;bestReset=score;resetWaypoint=waypoint end
      end
     end
    end
   end
   state.resetCandidates=resetCandidates;state.resetUsesArc=arc~=nil
   if point and flat(point-root.Position).Magnitude>2 then
    escapePlan={point=point,waypoint=resetWaypoint,at=now,gap=flat((resetWaypoint or point)-root.Position).Magnitude,progressAt=now};retreat=nil;confirmedStepback=false;stepbackShotUntil=0
    state.resets=(state.resets or 0)+1
    plan.dir=flat((resetWaypoint or point)-root.Position).Unit;plan.reason="Reset to perimeter";state.visualTarget=point
    state.status=string.format("Reset: best of %d perimeter destinations",resetCandidates);guardError=60;return
   end
  end
  local best=bestDrive(evaluated)
  local dir
  if best then
   state.selectedCandidate=best;state.visualTarget=best.point
   if tactics.choice~=best.name then tactics.choice=best.name;tactics.choiceAt=now;tactics.progress=nil end
   if not tactics.progress then tactics.progress={name=best.name,point=best.point,gap=flat(best.point-root.Position).Magnitude,at=now}end
   -- Follow the tangent of the same quadratic route evaluated above.
   local control=(root.Position+best.point)*.5+best.bend
   local toward=flat(control-root.Position)
   dir=toward.Magnitude>.1 and toward.Unit or forward
   lastHeading=dir
   state.status=string.format("%s: score %.1f, minimum defender margin %+.2fs",best.name,best.score,best.margin)
  else
   local space=reposition(root,hoop,list)
   if space and currentGap<10 and smartEscape(own,root,list,space.point,now,service,ping)then return end
   plan.dir=space and space.direction or Vector3.zero
   state.visualTarget=space and space.point or root.Position
   plan.reason=space and "Create reset space"or "No collision-free escape"
   state.status=space and "Drive lanes blocked: increase separation, then reassess"or "No safe movement found"
   guardError=space and 60 or 0;return
  end
  -- Save crossovers for actual pressure; do not interrupt an uncontested run.
  if service and service.Dribble and settings.farmCrossover and distance>settings.farmDunkRange+2
   and currentGap<12 and not beaten and (not laneClear or currentGap<6)
   and now-lastCross>.12 and action~="Dribbling" and attr(own,"CanDribble")~=false then
   if smartEscape(own,root,list,root.Position+dir*10,now,service,ping)then return end
  end
  plan.dir=dir;plan.reason="Autofarm: driving"
  guardError=60


 end

 -- Optional measured escape into a freshly validated backstep shot.
 local threeRun=nil
 local threeMatch=false
 local threeBudget,threeUsed,threeNext=0,0,0
 local function cancelThree(service)
  if threeRun and threeRun.stage=="shot"then releaseShot(service)end
  threeRun=nil
 end
 local function randomThree(own,root,now,service)
  if perimeterNeeded or escapePlan or not settings.farmShoot or not settings.farmRandomThrees or not holdsBall(own)then cancelThree(service);return false end
  if clearBall or rimHits[ballPart(attr(own,"Basketball"))]then cancelThree(service);return false end
  if not service or not service.Dribble or not service.Shoot then return false end
  local priorityHoop=rimFor(own,root,now)
  if priorityHoop and(not threeRun or threeRun.stage~="shot")
   and dunkLaneOpen(root,priorityHoop,defenders(own))then
   cancelThree(service);state.pullupRead="Open dunk lane: skip random three";return false
  end
  if threeRun then
   plan.dir=Vector3.zero;plan.reason="Autofarm: optional three combo"
   if threeRun.stage=="shot" and not shotStart then threeRun=nil;return true end
   if now<threeRun.next then return true end
   if threeRun.stage=="doubleX"then
    if attr(own,"Action")=="Dribbling"then
     if now-threeRun.at>1.4 then cancelThree(service)end
     return true
    end
    threeRun=nil
    local ok,err=stepbackAndShoot(service,now)
    if not ok then cancelThree(service);state.status="Three combo input failed: "..tostring(err);return true end
    -- The native E queue now owns this attempt.
    return true
   end
   if shotStart then releaseShot(service)end;threeRun=nil;return true
  end
  if threeUsed>=threeBudget or now<threeNext then return false end
  state.threeWhy="Looking for an eligible three combo"
  local hoop=rimFor(own,root,now)
  if not hoop then return false end
  local distance=flat(root.Position-hoop).Magnitude
  local limit,detectedArc=shotRange(root,hoop,now)
  local arc=detectedArc or settings.smartDribbleThreeDistance or 0
  -- Do not interrupt an immediate finish. Use a detected arc when available.
  if distance<=settings.farmDunkRange+6 or distance>limit or(arc>0 and distance<arc-5)then state.threeWhy=string.format("Range blocked: %.1f studs, limit %.1f",distance,limit);return false end
  local action=attr(own,"Action")
  if busy[action]or action=="Dribbling"or attr(own,"CanDribble")==false or spaceHeld or shotStart then return false end
  local retreatPoint=clampCourt(root.Position-flat(root.CFrame.LookVector)*5.6)
  local ping=0;pcall(function()ping=math.clamp(P:GetNetworkPing(),0,.35)end)
  if not safeMove(root.Position,retreatPoint)or flat(retreatPoint-hoop).Magnitude>limit or not stepbackWindow(retreatPoint,defenders(own),ping)then
   state.threeWhy="No safe stepback landing window";return false
  end
  local hand=attr(own,"Hand")
  if hand~="R"and hand~="L"then return false end
  if not(api.GetAutoTime and api.GetAutoTime())then return false end
  threeNext=now+rng:NextNumber(4,8)
  -- Use the same measured move selector and native-completion tracking as drives.
  if not smartEscape(own,root,defenders(own),hoop,now,service,ping)then return false end
  threeUsed=threeUsed+1;threeRun={stage="doubleX",next=now,at=now}
  state.threeAttempts=threeUsed;state.threeBudget=threeBudget
  state.status=string.format("Three attempt %d/%d: %s",threeUsed,threeBudget,state.dribblePlan)
  return true
 end
 local leaveScanAt,leaveClickAt=-math.huge,-math.huge
 local leaveButton=nil
 local leavePending=false
 local function visibleLeaveButton(button)
  if not button or not button.Parent or not button:IsA("GuiObject")or not button.Active then return false end
  if button.AbsoluteSize.X<2 or button.AbsoluteSize.Y<2 then return false end
  local node=button
  while node do
   if node:IsA("GuiObject")and not node.Visible then return false end
   if node:IsA("ScreenGui")and not node.Enabled then return false end
   node=node.Parent
  end
  return true
 end
 local function leaveCourt(now,force)
  if leavePending or now-leaveClickAt<5 then state.status="LeaveRec requested: waiting for lobby";return end
  if not force then
  local gui=P:FindFirstChildOfClass("PlayerGui")
  if not gui then return end
  if now-leaveScanAt>=.5 then
   leaveScanAt=now;leaveButton=nil
   for _,item in ipairs(gui:GetDescendants())do
    if(item:IsA("TextButton")or item:IsA("TextLabel"))and item.Text:lower():gsub("<[^>]*>",""):gsub("[^%a]","")=="leavecourt"then
     local candidate=item
     while candidate and candidate~=gui and not candidate:IsA("GuiButton")do candidate=candidate.Parent end
     if not visibleLeaveButton(candidate)then candidate=item end
     if visibleLeaveButton(candidate)then leaveButton=candidate;break end
    end
   end
  end
  if not visibleLeaveButton(leaveButton)then state.status="Match finished: waiting for visible Leave Court button";return end
  end
  local storage=game:GetService("ReplicatedStorage")
  local aero=storage:FindFirstChild("Aero")
  local services=aero and aero:FindFirstChild("AeroRemoteServices")
  local interface=services and services:FindFirstChild("InterfaceService")
  local remote=interface and interface:FindFirstChild("LeaveRec")
  if not remote or not remote:IsA("RemoteFunction")then state.status="LeaveRec remote unavailable";return end
  leaveClickAt=now;leavePending=true
  state.status="LeaveRec requested: waiting for lobby"
  task.spawn(function()
   if not running then leavePending=false;return end
   local ok,result=pcall(function()return remote:InvokeServer()end)
   leavePending=false
   if not running then return end
   state.status=ok and "LeaveRec returned: waiting for lobby"or("LeaveRec failed: "..tostring(result))
  end)
 end
 local function chatTriggerWord(message)
  local words={cheat=true,cheats=true,cheater=true,cheaters=true,cheating=true,hack=true,hacks=true,hacker=true,hackers=true,hacking=true,exploit=true,exploits=true,exploiter=true,exploiters=true,exploiting=true,aimbot=true,aimbotting=true,aimlock=true}
  for word in string.lower(tostring(message)):gmatch("%a+")do
   if words[word]then return word end
  end
 end
 local function chatRequeue(message)
  if testOpponent or stopped or not running or not settings.farmChatRequeue or state.chatExit then return end
  local now=os.clock()
  if now-(state.chatExitAt or -math.huge)<60 then return end
  local own=model()
  if not own or attr(own,"InGame")~=true then return end
  local word=chatTriggerWord(message);if not word then return end
  state.chatExitAt=now;state.chatExit={deadline=now+20,word=word}
  state.status="Chat trigger ("..word.."): returning to lobby"
  local service=keyboard and keyboard.Services and keyboard.Services.InputService
  releaseShot(service);releaseSpace(service);resetOffense(service);cancelThree(service)
  escapePlan=nil;plan.dir=Vector3.zero;plan.at=now
  settings._farmAttack=true;settings._farmGuard=false
  if guarding then guarding=false;releaseGuard()end
  leaveCourt(now,true)
 end
 -- Read delivered messages only; do not replace the game's chat callbacks.
 pcall(function()
  on(game:GetService("TextChatService").MessageReceived,function(message)
   if message.TextSource then chatRequeue(message.Text)end
  end)
 end)
 local function watchChat(player)on(player.Chatted,chatRequeue)end
 for _,player in ipairs(game:GetService("Players"):GetPlayers())do watchChat(player)end
 on(game:GetService("Players").PlayerAdded,watchChat)
 local dunkBlocks=setmetatable({},{__mode="k"})
 local lastRimBlock=-math.huge
 local function attemptRimBlock(own,root,enemy,now,service)
  if holdsBall(own)or attr(enemy,"Action")~="Dunking"then return false end
  if not service or not service.Jump or not service.Shoot then state.rimBlockWhy="Input controller unavailable";return false end
  local er=rootOf(enemy);local ball=ballPart(attr(enemy,"Basketball"))
  if not er or not ball then state.rimBlockWhy="Dunk ball not tracked";return false end
  local hoop=rimFor(enemy,er,now)
  if not hoop or flat(er.Position-hoop).Magnitude>12 then state.rimBlockWhy="Outside rim area";return false end
  local recent=settings._lastDefenseJump
  if dunkBlocks[enemy]or now-lastRimBlock<.8 or(recent and now-recent.time<.65)then return false end
  local grounded=false;local hum=own:FindFirstChildOfClass("Humanoid")
  if hum then grounded=hum.FloorMaterial~=Enum.Material.Air end
  if not grounded and math.abs(root.AssemblyLinearVelocity.Y)<3 then
   local ray=RaycastParams.new();ray.FilterType=Enum.RaycastFilterType.Exclude
   local excluded={own,enemy};local chars=workspace:FindFirstChild("Characters")
   if chars then excluded[#excluded+1]=chars end
   local visuals=workspace:FindFirstChild("VantaFarmPlan");if visuals then excluded[#excluded+1]=visuals end
   ray.FilterDescendantsInstances=excluded
   local hit=workspace:Raycast(root.Position,Vector3.new(0,-4.5,0),ray)
   grounded=hit~=nil and hit.Normal.Y>.7
  end
  if not grounded then state.rimBlockWhy="Waiting for grounded feet";return false end
  if attr(own,"CanJump")==false then state.rimBlockWhy="Native CanJump is false";return false end
  local ballGap=flat(ball.Position-root.Position).Magnitude
  if ballGap>5.5 then state.rimBlockWhy=string.format("Ball %.1f studs away",ballGap);return false end
  if ball.Position.Y-root.Position.Y<2 then state.rimBlockWhy="Waiting for ball to rise";return false end
  local ok,err=pcall(function()
   service.Shoot:Fire({Shoot=true,Input="E",UseSpace=true});service.Jump:Fire()
  end)
  if ok then
   dunkBlocks[enemy]=true;lastRimBlock=now
   settings._lastDefenseJump={source="Rim stopper",time=now}
   state.rimBlockWhy="Native jump requested";state.rimBlockUntil=now+.7
  else state.rimBlockWhy="Jump input failed: "..tostring(err)end
  task.delay(.08,function()if not holdsBall(model())then pcall(function()service.Shoot:Fire({Shoot=false})end)end end)
  return ok
 end
 settings._rimStopAttempt=attemptRimBlock
 on(Run.Heartbeat,function()
  for enemy in pairs(dunkBlocks)do if not enemy.Parent or attr(enemy,"Action")~="Dunking"then dunkBlocks[enemy]=nil end end
 end)
 local function stopDunk(own,root,now,service)
  if not settings.farmRimStop or holdsBall(own)or not service or not service.Jump or not service.Shoot then return false end
  for _,enemy in ipairs(opponents(own))do
   if attr(enemy,"Action")~="Dunking"then dunkBlocks[enemy]=nil
   else
    local er=rootOf(enemy);local ball=ballPart(attr(enemy,"Basketball"))
    local hoop=er and rimFor(enemy,er,now)
    if er and hoop and flat(er.Position-hoop).Magnitude<12 and flat(er.Position-root.Position).Magnitude<7 then
     local radial=flat(er.Position-hoop)
     local aim=er.Position+(radial.Magnitude>.1 and -radial.Unit*3.2 or Vector3.zero)
     local delta=flat(aim-root.Position)
     state.visualTarget=aim;state.visualDefender=er.Position;state.rimBlockUntil=now+.25
     settings._farmAttack=true
     plan.dir=delta.Magnitude>.6 and delta.Unit or Vector3.zero
     plan.reason="Dunk block: close gap";guardError=60
     if guarding then guarding=false;releaseGuard()end
     local away=flat(root.Position-er.Position)
     if settings.antiPush and away.Magnitude>.1 and away.Magnitude<3.2 then plan.dir=away.Unit end
     attemptRimBlock(own,root,enemy,now,service)
     state.status="Dunk block: "..tostring(state.rimBlockWhy or "Closing safely")
     return true
    end
   end
  end
  return false
 end
 -- BEGIN FARM MOVE LIFECYCLE
 local function movePhase(pending,now,position,hand,action)
  local elapsed=now-pending.at
  local moved=flat(position-pending.origin)
  local expected=flat(pending.move.destination-pending.origin)
  local confirmedMotion=moved.Magnitude>.75 and expected.Magnitude>.1 and moved.Unit:Dot(expected.Unit)>.6
  local switched=hand==pending.move.handAfter and hand~=pending.hand
  if action=="Dribbling"then pending.nativeSeen=true end
  if action=="Dribbling"or switched or confirmedMotion then pending.seen=true end
  if action~="Dribbling"and(pending.nativeSeen or pending.seen and elapsed>=pending.move.duration*.65)then return "complete"end
  if not pending.seen and elapsed>.4 then return "timeout"end
  if elapsed>pending.move.duration+.5 then return "timeout"end
  return "wait"
 end
 -- END FARM MOVE LIFECYCLE
 local function planDuringDribble(own,root,now,service)
  local hoop=rimFor(own,root,now);if not hoop then return false end
  local list=defenders(own);local ping=0
  pcall(function()ping=math.clamp(P:GetNetworkPing(),0,.35)end)
  local options=candidatePlans(root,hoop,list,ping)
  state.planCandidates=options;state.planEvaluatedAt=now
  local best,step=bestDrive(options),nil
  for _,candidate in ipairs(options)do
   if candidate.name=="STEPBACK"then step=candidate end
  end
  local distance=flat(hoop-root.Position).Magnitude
  local dunkOpen=distance<=math.max(18,settings.farmDunkRange+6)
  local wantsStep=not dunkOpen and distance>math.max(18,settings.farmDunkRange+6)
   and step and step.quality=="good"and settings.farmShoot and safeMove(root.Position,step.point)
  local selected=wantsStep and step or best
  state.selectedCandidate=selected;state.visualTarget=selected and selected.point or hoop
  settings._farmAttack=true;guardError=60
  local delta=flat((best and((root.Position+best.point)*.5+(best.bend or Vector3.zero))or hoop)-root.Position)
  local space=not best and reposition(root,hoop,list)or nil
  plan.dir=best and delta.Magnitude>.1 and delta.Unit or(space and space.direction or Vector3.zero)
  if space then state.visualTarget=space.point end
  plan.reason=wantsStep and "Live read: prepare stepback"or "Live read: attack best route"
  state.status="Dribbling: reevaluating drive and shot opportunities"
  -- Observe during native dribbles; select the next measured input after landing.
  return true
 end
 local function think(now)
  state.visualTarget=nil;state.visualStepback=nil;state.visualDefender=nil;state.visualOffense=false;state.selectedCandidate=nil;state.planEvaluatedAt=nil;state.shotWhy=nil;state.gap=nil;state.closeout=nil
  plan.dir=nil;plan.reason=nil;settings._farmGuard=false;settings._farmAttack=false;settings._farmPlantUntil=nil
  local own=model();local root=rootOf(own)
  -- No character is normal in a queue lobby; only treat it as the lobby once it has lasted a few seconds.
  if root then noCharacterSince=nil elseif not noCharacterSince then noCharacterSince=now end
  local noCharacter=not root and now-(noCharacterSince or now)>=3
  local matchFlag=own and attr(own,"InGame")
  local onCourt=own and attr(own,"OnCourt")
  if not testOpponent and state.chatExit then
   farmMove=nil
   if matchFlag~=true and onCourt~=true and not visibleLeaveButton(leaveButton)and not leavePending then
    state.chatExit=nil;lastReady=-math.huge
   elseif now>=state.chatExit.deadline then
    state.chatExit=nil;state.status="Chat exit timed out; lobby not confirmed"
   else
    settings._farmAttack=true;plan.dir=Vector3.zero;plan.reason="Returning to lobby after chat trigger"
    state.phase="leaving";leaveCourt(now,true);return
   end
  end
  -- A visible native exit must work when farming starts on the results screen too.
  -- An explicit active-match flag still blocks leaving during play.
  if not testOpponent and matchFlag~=true then
   leaveCourt(now)
  end
  if not testOpponent and matchFlag~=true and(visibleLeaveButton(leaveButton)or leavePending or now-leaveClickAt<5)then
   state.phase="postmatch"
   settings._farmAttack=false;settings._farmGuard=false
   cancelThree(keyboard and keyboard.Services and keyboard.Services.InputService)
   releaseSpace(nil)
   if guarding then guarding=false;releaseGuard()end
   return
  end
  if not root and not noCharacter then state.phase="wait";state.status="Waiting for character";return end
  local inGame=root~=nil and(testOpponent~=nil or matchFlag==true or(matchFlag==nil and(onCourt==true or goalPosition(attr(own,"Goal"))~=nil)))or env.VantaFarmForce==true
  if testPlayer then testOpponent=testPlayer.Character or testOpponent end
  if testOpponent and(not root or not testOpponent.Parent or not rootOf(testOpponent))then
   plan.dir=Vector3.zero;state.status="Gym test: waiting for selected friend";return
  end
  if not inGame then
   threeMatch=false;cancelThree(keyboard and keyboard.Services and keyboard.Services.InputService)
   box={};madeUntil=-math.huge;clearBall=nil;clearRim=nil;clearTarget=nil;table.clear(rimHits);table.clear(ballOwners)
   state.phase="lobby";state.seen=root and string.format("InGame=%s OnCourt=%s Goal=%s",tostring(attr(own,"InGame")),tostring(attr(own,"OnCourt")),tostring(attr(own,"Goal")))or "no character";if now-lastReady>2 then state.status="In lobby: waiting to ready up"end
   if now-lastReady>=15 then
    lastReady=now
    local custom=env.VantaFarmReady
    local ok,sent,why
    if type(custom)=="function"then ok,sent=pcall(custom)else ok,sent,why=pcall(fireReady)end
    if ok and sent~=false then state.ready=state.ready+1;state.status="Ready sent"
    else state.status="Ready not sent: "..tostring(type(custom)=="function"and sent or why or sent)end
   end
   return
  end
  if not threeMatch then
   threeMatch=true;threeBudget=rng:NextInteger(2,3);threeUsed=0;threeNext=now+rng:NextNumber(4,9)
   state.threeAttempts=0;state.threeBudget=threeBudget
  end
  state.phase="match"
  -- Learn the court area, and watch for a make: a ball dropping through a rim from above.
  for _,character in ipairs(defenseCharacters())do
   local characterRoot=rootOf(character)
   if characterRoot then
    local p=characterRoot.Position
    box.minX=math.min(box.minX or p.X,p.X);box.maxX=math.max(box.maxX or p.X,p.X)
    box.minZ=math.min(box.minZ or p.Z,p.Z);box.maxZ=math.max(box.maxZ or p.Z,p.Z)
   end
  end
  for _,character in ipairs(defenseCharacters())do
   local held=ballPart(attr(character,"Basketball"))
   if held then ballOwners[held]=character end
  end
  local allRims=rimPositions(own,root,now)
  for _,ball in ipairs(ballCandidates())do
   local p=ball.Position;local before=lastBall[ball]
   if not rimSounds[ball]then
    local sounds=ball:FindFirstChild("Sounds")
    local sound=sounds and sounds:FindFirstChild("Rim")
    if sound and sound:IsA("Sound")then
     rimSounds[ball]=true
     on(sound.Played,function()
      if not running or ballOwners[ball]==model()then return end
      local closest,dist
      for _,rim in ipairs(rimCandidates)do
       if rim.Parent then local d=(ball.Position-rim.Position).Magnitude;if not dist or d<dist then closest=rim;dist=d end end
      end
      if closest and dist<12 then rimHits[ball]={rim=closest.Position,at=os.clock()};state.clearReason="Replicated rim sound"end
     end)
    end
   end
   -- Swept proximity plus a trajectory change supplements Touched; do not treat
   -- an ordinary flight through the hoop as contact by proximity alone.
   if before and ballOwners[ball]~=own then
    local previousVelocity=state._ballVelocity and state._ballVelocity[ball]
    local velocity=ball.AssemblyLinearVelocity
    if previousVelocity and(velocity-previousVelocity).Magnitude>4 then
     for _,rim in ipairs(rimCandidates)do
      if rim.Parent then
       local delta=p-before;local n=delta:Dot(delta)
       local t=n>.001 and math.clamp((rim.Position-before):Dot(delta)/n,0,1)or 0
       local gap=(before+delta*t-rim.Position).Magnitude
       local radius=math.clamp(math.max(rim.Size.X,rim.Size.Z)*.5+math.max(ball.Size.X,ball.Size.Z)*.5,.8,2.8)
       if gap<=radius then rimHits[ball]={rim=rim.Position,at=now};state.clearReason="Rim collision trajectory";break end
      end
     end
    end
   end
   state._ballVelocity=state._ballVelocity or setmetatable({},{__mode="k"})
   state._ballVelocity[ball]=ball.AssemblyLinearVelocity
   if before then
    for _,rim in ipairs(allRims)do
     if before.Y>rim.Y and p.Y<=rim.Y and flat(p-rim).Magnitude<1.3 then madeUntil=now+8;state.makes=(state.makes or 0)+1 end
    end
   end
   lastBall[ball]=p
  end
  settings._guardReleased=false
  -- A profile switch in the menu must not turn the forced defence back off mid-farm.
  settings.catchShoot=false;settings.smartDribble=false;settings.guard=true;settings.rimPress=true;settings.steal=false;settings.rebound=true;settings.reboundChase=true;settings.antiOop=true
  local service=keyboard and keyboard.Services and keyboard.Services.InputService
  local action=attr(own,"Action")
  if retreat and action=="Dribbling"then retreat.seen=true end
  local motion= root.AssemblyLinearVelocity
  local _,motionAt=stationaryShot(flat(motion).Magnitude,motion.Y,action,attr(own,"CanMove"),now,lastShotMotion)
  lastShotMotion=motionAt
  if shotStart then plan.dir=Vector3.zero;plan.reason="Autofarm: shot movement locked" end
  local possession=holdsBall(own)
  local gainedPossession=possession and possessionOwner~=own
  if (possessionOwner~=own or not possession)and action~="Dunking" then
   resetOffense(service);lastHeading=nil;resetting=false;openSince=nil;settleSince=nil
   if shotStart and not gainedPossession and action~="Shooting"then releaseShot(service)end
  end
  possessionOwner=possession and own or nil
  if gainedPossession then
   tactics.failures={};tactics.choice=nil;tactics.progress=nil;state.routeRead=nil
   preferredDrive=preferredDrive=="DRIVE RIGHT"and "DRIVE LEFT"or "DRIVE RIGHT"
   escapePlan=nil;allBlockedAt=nil;perimeterNeeded=false
   openingUntil=now+.45
   cancelThree(nil);farmMove=nil;dribbleFailures={};dribbleHistory={};coveredSince=nil
   -- Discard stale shot bookkeeping; never send a release into a new possession.
   shotStart=nil;shotSawAction=false
   settings._farmAttack=true
   if guarding then guarding=false;releaseGuard()end
  elseif not possession then openingUntil=0;farmMove=nil;escapePlan=nil;coveredSince=nil;allBlockedAt=nil;perimeterNeeded=false end
  if shotStart and action=="Shooting"then
   if not shotSawAction then state.shotActionAt=now end
   shotSawAction=true
  end
  if shotStart and shotSawAction and action~="Shooting"then releaseShot(service)end
  if shotStart and ((shotSawAction and now-(state.shotActionAt or shotStart)>1.8)
   or(not shotSawAction and now-shotStart>1.8))then
   releaseShot(service);state.shotWhy="shot input not acknowledged or timed out"
  end
  -- Jump shots own movement until the shot action ends. Native finishes keep their approach.
  if (shotStart or action=="Shooting")and not spaceHeld and action~="Dunking"and action~="Layup"then
   plantFeet(service,now)
   settings._farmAttack=true;settings._farmGuard=false
   plan.dir=Vector3.zero;plan.reason="Jump shot: movement locked";guardError=0
   state.status="Jump shot: holding position through release"
   return
  end
  if spaceHeld then
   if action=="Dunking" then
    if not spaceSawDunk then state.dunkAcknowledged=(state.dunkAcknowledged or 0)+1 end
    spaceSawDunk=true
   end
   if not spaceSawDunk and now-spaceStart>1.0 then state.finishWhy="Space hold not acknowledged as a dunk" end
   if(spaceSawDunk and action~="Dunking")or now-spaceStart>1.1 or(not holdsBall(own)and action~="Dunking"and now-spaceStart>.6)then releaseSpace(service)end
  end
  -- Open shots are timed by Auto time, so make sure it is on while the farm runs.
  if (settings.farmShoot or settings.farmRandomThrees) and api.GetAutoTime and not api.GetAutoTime()and now-autoTimeTry>2 then
   autoTimeTry=now
   if api.SetAutoTime(true)then forcedAutoTime=true end
  end
  if possession and not clearDirection(own,root)and takeOpenShot(own,root,now,service)then threeRun=nil;return end
  if possession and now<openingUntil and not busy[action] then
   settings._farmAttack=true
   local clearing=clearDirection(own,root)
   local hoop=rimFor(own,root,now)
   local delta=hoop and flat(hoop-root.Position)
   -- Let an immediate dunk go through the normal finishing path.
   if clearing or not delta or delta.Magnitude>settings.farmDunkRange then
    local direction=clearing
    if not direction and delta and delta.Magnitude>.1 then
     direction=chooseHeading(root,hoop,defenders(own),1.5,math.max(16,flat(root.AssemblyLinearVelocity).Magnitude))
     if direction and not safeMove(root.Position,root.Position+direction*4)then direction=Vector3.zero end
    end
    plan.dir=direction or Vector3.zero
    plan.reason="Autofarm: sprint-first possession";guardError=60
    state.status="Start possession: sprint forward";return
   end
  end
  if farmMove and possession then
   settings._farmAttack=true
   local clearing=clearDirection(own,root)
   if clearing or busy[action]then
    farmMove=nil
    if clearing then plan.dir=clearing;plan.reason="Autofarm: clearing ball";guardError=60;return end
   else
    local pending=farmMove
    local phase=movePhase(pending,now,root.Position,attr(own,"Hand"),action)
    if phase~="wait"then
     if phase=="timeout"then
      dribbleFailures[pending.move.keys]=now+3;cancelThree(nil);state.dribblePlan="Move not acknowledged: choose another escape"
     else state.dribblePlan="Move finished: read the new lane"end
     farmMove=nil;tactics.progress=nil;tactics.choice=nil
     if phase=="timeout"then lastCross=now end
     if escapePlan then escapePlan.progressAt=now;escapePlan.gap=flat((escapePlan.waypoint or escapePlan.point)-root.Position).Magnitude end
    else
     plan.dir=Vector3.zero;plan.reason="Measured dribble: wait for landing";guardError=0
     state.visualTarget=pending.move.destination;state.status="Creating space with "..pending.move.keys..": reading landing";return
    end
   end
  end
  -- Recheck on the same frame a measured move completes, before another combo.
  if possession and not clearDirection(own,root)and takeOpenShot(own,root,now,service)then threeRun=nil;return end
  if (settings.farmRandomThrees or threeRun)and not escapePlan and not coveredSince and not retreat and not confirmedStepback and not shotStart then
   settings._farmAttack=possession
   if randomThree(own,root,now,service)then return end
  end
  if possession and action=="Dribbling"and not retreat and not confirmedStepback then
   settings._farmAttack=true
   local clearing=clearDirection(own,root)
   if clearing then farmMove=nil;plan.dir=clearing;plan.reason="Autofarm: clearing ball";guardError=60;return end
   if planDuringDribble(own,root,now,service)then return end
  end
  if possession and (retreat or confirmedStepback) and not busy[action] then
   settings._farmAttack=true
   local clearing=clearDirection(own,root)
   if clearing then
    retreat=nil;confirmedStepback=false;stepbackShotUntil=0
    plan.dir=clearing;plan.reason="Autofarm: clearing ball";guardError=60;return
   end
   offense(own,root,now,service);return
  end
  if busy[action]or attr(own,"CanMove")==false then
   state.status="Busy: "..tostring(action or "locked")
   return
  end
  if holdsBall(own)then
   settings._farmAttack=true
   if guarding then guarding=false;releaseGuard()end
   local clearing=clearDirection(own,root)
   if clearing then
    retreat=nil;resetting=false;stepbackShotUntil=0
    if shotStart then releaseShot(service)end
    plan.dir=clearing;plan.reason="Autofarm: clearing ball";guardError=60;return
   end
   offense(own,root,now,service)
   return
  end
  if stopDunk(own,root,now,service)then return end
  -- No ball: go and get any loose one, otherwise guard whoever has it.
  local best,bestDistance
  local balls=looseBalls()
  local rims=rimPositions(own,root,now)
  local usable=0
  local scored=now<madeUntil
  for _,ball in ipairs(balls)do
   if not scored and(not giveUp[ball]or giveUp[ball]<now)and inPlay(ball,root,rims)then
    usable=usable+1
    local distance=flat(ballTarget(ball,root)-root.Position).Magnitude
    if not bestDistance or distance<bestDistance then best=ball;bestDistance=distance end
   end
  end
  state.seen=string.format("loose balls seen: %d, in play: %d",#balls,usable)
  if best then
   settings._farmAttack=true
   -- Give up on a ball that is not getting closer (fence, out of bounds) or that takes too long.
   if best~=chaseBall then chaseBall=best;chaseSince=now;chaseGap=bestDistance;chaseGapAt=now end
   if now-chaseGapAt>2.5 then
    if chaseGap-bestDistance<.5 and bestDistance>6 and flat(best.AssemblyLinearVelocity).Magnitude<2 then giveUp[best]=now+3 end
    chaseGap=bestDistance;chaseGapAt=now
   end
   if now-chaseSince>12 and bestDistance>8 then giveUp[best]=now+3 end
   local aim=ballTarget(best,root);state.visualTarget=aim;local delta=flat(aim-root.Position)
   plan.dir=delta.Magnitude>.35 and delta.Unit or Vector3.zero;plan.reason="Autofarm: loose ball"
   -- Drop the defensive stance right away so the player runs instead of shuffling in guard.
   if guarding then guarding=false;releaseGuard()end
   guardError=60
   state.status=string.format("Chasing loose ball · %.0f studs",delta.Magnitude)
   -- Chase only: autofarm never requests a steal or loose-ball catch input.
   return
  end
  chaseBall=nil
  -- Only guard while an opponent actually has the ball. A dead or out-of-bounds ball means stand down.
  local handlerHasBall=false
  for _,character in ipairs(opponents(own))do if holdsBall(character)then handlerHasBall=true;break end end
  if handlerHasBall then
   settings._farmGuard=true;guarding=true;state.status="Guarding the ball handler"
   if testOpponent then
    local er=rootOf(testOpponent);local hoop=er and rimFor(testOpponent,er,now)
    local toward=hoop and flat(hoop-er.Position)
    if toward and toward.Magnitude>.1 then
     local aim=clampCourt(er.Position+toward.Unit*3.5+flat(er.AssemblyLinearVelocity)*.12)
     local correction=flat(aim-root.Position)
     plan.dir=correction.Magnitude>.5 and correction.Unit or Vector3.zero
     plan.reason="Gym test: contain selected friend";guardAim=er.Position;guardError=correction.Magnitude
    end
   end
   if settings.farmPostClamp then
    for _,enemy in ipairs(opponents(own))do
     local er=rootOf(enemy)
     if er and holdsBall(enemy)then
      local hoop=rimFor(enemy,er,now)
      local delta=hoop and flat(hoop-er.Position)
      local gap=flat(root.Position-er.Position).Magnitude
      local action=tostring(attr(enemy,"Action")or ""):lower()
      local posting=action:find("post",1,true)or attr(enemy,"PostingUp")==true or attr(enemy,"PostUp")==true
      if delta and delta.Magnitude>.1 and gap<8 and(posting or(gap<4.5 and flat(er.CFrame.LookVector):Dot(delta.Unit)<-.35))then
       local aim=er.Position+delta.Unit*2.8+flat(er.AssemblyLinearVelocity)*.12
       local correction=flat(clampCourt(aim)-root.Position)
       plan.dir=correction.Magnitude>.25 and correction.Unit or Vector3.zero
       plan.reason="Post clamp: hold goal-side ground";guardError=0
       guardAim=er.Position;state.visualTarget=aim;state.visualDefender=er.Position
       state.status="Post clamp: counter backdown and protect the rim"
       break
      end
     end
    end
   end
  else
   if guarding then guarding=false;releaseGuard()end
   state.status=scored and "Basket made: standing by" or #balls>0 and "Ball out of play: standing by" or "Nobody has the ball: standing by"
  end
 end

 local function disarm(persist)
  syncSprint(keyboard and keyboard.Services and keyboard.Services.InputService,false,os.clock())
  settings._farmSprintWanted=nil;settings._farmPlantUntil=nil
  cancelThree(keyboard and keyboard.Services and keyboard.Services.InputService)
  running=false;testOpponent=nil;testPlayer=nil;state.testOpponent=nil;perimeterNeeded=false;escapePlan=nil;farmMove=nil;coveredSince=nil;allBlockedAt=nil;releaseSpace(nil);releaseShot(keyboard and keyboard.Services and keyboard.Services.InputService);retreat=nil;resetting=false;possessionOwner=nil;lastShotMotion=os.clock();settings.farm=false;settings._farmGuard=false;settings._farmAttack=false;plan.dir=nil
  if guarding then guarding=false;releaseGuard()end
  forceDefense(false)
  clearBall=nil;clearRim=nil;clearTarget=nil
  state.phase="off";state.status="Off"
  if persist then saveConfig(false)end
 end
 local function arm()
  if running then return end
  running=true;settings.farm=true
  forceDefense(true);if not testOpponent then saveConfig(true);queueReexec()end
  state.status="Armed"
 end

 -- The 5 second prompt shown whenever the script runs with the toggle saved ON.
 local gui,countdownToken
 countdownToken=0
 local function closePrompt()if gui then gui:Destroy();gui=nil end end
 local function prompt(seconds)
  countdownToken=countdownToken+1;local mine=countdownToken
  closePrompt()
  local style=settings._hudStyle
  local c=style and style.c or{surface=Color3.fromRGB(17,20,29),stroke=Color3.fromRGB(40,47,64),raised=Color3.fromRGB(23,27,38),text=Color3.fromRGB(240,242,248),sub=Color3.fromRGB(178,186,206),danger=Color3.fromRGB(255,125,138)}
  local accent=style and style.accent or Color3.fromRGB(72,134,255)
  gui=create("ScreenGui",{Name="VantaAutofarmPrompt",ResetOnSpawn=false,IgnoreGuiInset=true,DisplayOrder=60},P.PlayerGui)
  local card=create("Frame",{AnchorPoint=Vector2.new(.5,0),Position=UDim2.new(.5,0,0,26),Size=UDim2.fromOffset(380,134),BackgroundColor3=c.surface,BackgroundTransparency=.03,BorderSizePixel=0},gui)
  create("UICorner",{CornerRadius=UDim.new(0,14)},card)
  create("UIStroke",{Color=c.stroke,Thickness=1},card)
  create("TextLabel",{BackgroundTransparency=1,Position=UDim2.fromOffset(18,14),Size=UDim2.new(1,-36,0,22),Text="Blacktop autofarm",Font=Enum.Font.GothamBold,TextSize=16,TextXAlignment=Enum.TextXAlignment.Left,TextColor3=c.text},card)
  local sub=create("TextLabel",{BackgroundTransparency=1,Position=UDim2.fromOffset(18,38),Size=UDim2.new(1,-36,0,20),Text="",Font=Enum.Font.GothamMedium,TextSize=13,TextXAlignment=Enum.TextXAlignment.Left,TextColor3=c.sub},card)
  local track=create("Frame",{Position=UDim2.fromOffset(18,66),Size=UDim2.new(1,-36,0,4),BackgroundColor3=c.raised,BorderSizePixel=0},card)
  create("UICorner",{CornerRadius=UDim.new(1,0)},track)
  local fill=create("Frame",{Size=UDim2.fromScale(1,1),BackgroundColor3=accent,BorderSizePixel=0},track)
  create("UICorner",{CornerRadius=UDim.new(1,0)},fill)
  local function button(text,x,color,textColor)
   local b=create("TextButton",{AutoButtonColor=true,Position=UDim2.new(x,x==0 and 18 or 4,0,84),Size=UDim2.new(.5,-22,0,32),BackgroundColor3=color,BorderSizePixel=0,Text=text,Font=Enum.Font.GothamBold,TextSize=14,TextColor3=textColor},card)
   create("UICorner",{CornerRadius=UDim.new(0,9)},b)
   return b
  end
  local no=button("No, stop it",0,c.raised,c.danger)
  local go=button("Start now",.5,accent,Color3.new(1,1,1))
  local started=os.clock()
  local done=false
  local function finish(start)
   if done or mine~=countdownToken then return end
   done=true;closePrompt()
   if start then arm()else disarm(true);state.status="Cancelled at the prompt"end
  end
  no.Activated:Connect(function()finish(false)end)
  go.Activated:Connect(function()finish(true)end)
  task.spawn(function()
   while not done and mine==countdownToken and not stopped do
    local left=seconds-(os.clock()-started)
    if left<=0 then finish(true);break end
    sub.Text=string.format("Starting in %d. Press No to cancel.",math.ceil(left))
    fill.Size=UDim2.fromScale(math.clamp(left/seconds,0,1),1)
    task.wait(.05)
   end
  end)
  state.status="Waiting at the prompt"
 end

 api.SetFarm=function(value)
  if value then
   if testOpponent then disarm(false)end
   countdownToken=countdownToken+1;closePrompt()
   arm();return true,"Autofarm on (saved)"
  end
  countdownToken=countdownToken+1;closePrompt()
  disarm(true);return true,"Autofarm off (saved)"
 end
 api.StartFarmGymTest=function()
  local own=model();local root=rootOf(own)
  if not root then return false,"Spawn in the private gym first"end
  local selected,gap=nil,math.huge
  for _,player in ipairs(game:GetService("Players"):GetPlayers())do
   local character=player.Character;local r=rootOf(character)
   if character~=own and r then
    local distance=(r.Position-root.Position).Magnitude
    if distance<gap and distance<100 then selected=character;gap=distance end
   end
  end
  if not selected then return false,"Stand near your friend in the private gym first"end
  countdownToken=countdownToken+1;closePrompt();disarm(false)
  testOpponent=selected;testPlayer=game:GetService("Players"):GetPlayerFromCharacter(selected);state.chatExit=nil;lastReady=os.clock();arm()
  state.testOpponent=selected.Name;state.status="Gym test against "..selected.Name
  return true,state.status
 end
 api.StopFarmGymTest=function()
  countdownToken=countdownToken+1;closePrompt();disarm(false);state.testOpponent=nil
  return true,"Gym test stopped"
 end
 api.FarmReadyNow=function()
  if testOpponent then return false,"Ready disabled during gym testing"end
  lastReady=-math.huge
  if not running then return false,"Turn Autofarm on first"end
  return true,"Ready will be sent on the next tick if you are in the lobby"
 end
 api.GetFarmState=function()
  local result=table.clone(state);result._ballVelocity=nil;result.mustClear=clearBall~=nil;result.running=running;result.enabled=settings.farm;result.testMode=testOpponent~=nil
  return result
 end

 -- Persistent ground-only planning overlay. Geometry is reused, never rebuilt per frame.
 local visualFolder=Instance.new("Folder");visualFolder.Name="VantaFarmPlan";visualFolder.Parent=workspace
 local function visualPart(name)
  local part=Instance.new("Part");part.Name=name;part.Anchored=true
  part.CanCollide=false;part.CanTouch=false;part.CanQuery=false;part.CastShadow=false
  part.Material=Enum.Material.Neon;part.Transparency=1;part.Size=Vector3.new(.1,.04,.1);part.Parent=visualFolder
  return part
 end
 local route,preview,defenderLine,arrows,marker={},{},{},{},{}
 for i=1,16 do route[i]=visualPart("Movement path");preview[i]=visualPart("Destination guide")end
 for i=1,8 do defenderLine[i]=visualPart("Defender link")end
 for i=1,6 do arrows[i]=visualPart("Direction chevron")end
 for i=1,4 do marker[i]=visualPart("Target corners")end
 local params=RaycastParams.new();params.FilterType=Enum.RaycastFilterType.Exclude
 local function ground(point,root)
  local excluded={visualFolder,root.Parent}
  for _,name in ipairs({"Characters","ProxyCharacter","Basketballs"})do
   local object=workspace:FindFirstChild(name);if object then excluded[#excluded+1]=object end
  end
  -- Start near foot height so overhead props cannot become the apparent floor.
  for _=1,5 do
   params.FilterDescendantsInstances=excluded
   local hit=workspace:Raycast(Vector3.new(point.X,root.Position.Y+.5,point.Z),Vector3.new(0,-30,0),params)
   if not hit then break end
   if hit.Normal.Y>.85 and hit.Position.Y<=root.Position.Y-1 then
    return Vector3.new(point.X,hit.Position.Y+.045,point.Z)
   end
   excluded[#excluded+1]=hit.Instance
  end
  return Vector3.new(point.X,root.Position.Y-2.95,point.Z)
 end
 local function segment(part,a,b,color,width)
  local length=(b-a).Magnitude
  if length<.01 then part.Transparency=1;return end
  part.Size=Vector3.new(width,.035,length);part.CFrame=CFrame.lookAt((a+b)*.5,b)
  part.Color=color;part.Transparency=.15
 end
 local function ring(parts,center,radius,color)
  for i,part in ipairs(parts)do
   local a=(i-1)/#parts*math.pi*2;local b=i/#parts*math.pi*2
   segment(part,center+Vector3.new(math.cos(a)*radius,0,math.sin(a)*radius),center+Vector3.new(math.cos(b)*radius,0,math.sin(b)*radius),color,.09)
  end
 end
 local function dashed(parts,a,b,color)
  for i,part in ipairs(parts)do
   segment(part,a:Lerp(b,(i-1)/#parts),a:Lerp(b,(i-.35)/#parts),color,.11)
  end
 end
 local palette={good=Color3.fromRGB(75,220,150),uncertain=Color3.fromRGB(255,190,75),bad=Color3.fromRGB(255,100,110)}
 local function labelAnchor(name)
  local anchor=visualPart(name)
  local gui=Instance.new("BillboardGui");gui.Size=UDim2.fromOffset(205,48);gui.StudsOffset=Vector3.new(0,.65,0)
  gui.AlwaysOnTop=false;gui.MaxDistance=100;gui.Adornee=anchor;gui.Parent=anchor
  local text=Instance.new("TextLabel");text.Size=UDim2.fromScale(1,1);text.BackgroundTransparency=.3
  text.BackgroundColor3=Color3.fromRGB(12,17,26);text.TextColor3=Color3.fromRGB(235,240,255)
  text.Font=Enum.Font.GothamMedium;text.TextSize=12;text.TextWrapped=true;text.Parent=gui
  local corner=Instance.new("UICorner");corner.CornerRadius=UDim.new(0,5);corner.Parent=text
  return {anchor=anchor,gui=gui,text=text}
 end
 local candidates={}
 for i=1,7 do
  local entry=labelAnchor("Candidate "..i);entry.parts={}
  for j=1,24 do entry.parts[j]=visualPart("Candidate route "..i)end
  candidates[i]=entry
 end
 local defenderInfo=labelAnchor("Defender distance")
 local shotInfo=labelAnchor("Last shot reason");local shotLines={}
 for i=1,12 do shotLines[i]=visualPart("Frozen shot decision")end
 local candidateAt=0;local candidateCache={}
 local function hideLabels()
  for _,entry in ipairs(candidates)do entry.gui.Enabled=false end
  defenderInfo.gui.Enabled=false;shotInfo.gui.Enabled=false
 end
 local function showCandidates(own,root,origin)
  local now=os.clock();local hoop=rimFor(own,root,now);local list=defenders(own)
  local active=holdsBall(own)and hoop~=nil
  if active and now-candidateAt>.15 then
   local ping=0;pcall(function()ping=math.clamp(P:GetNetworkPing(),0,.35)end)
   candidateCache=candidatePlans(root,hoop,list,ping);candidateAt=now
  end
  for i,entry in ipairs(candidates)do
   local data=active and candidateCache[i]or nil
   entry.gui.Enabled=data~=nil
   if data then
    local point=ground(data.point,root);local color=palette[data.quality]
    entry.anchor.Position=point;entry.text.Text=data.text;entry.text.TextColor3=color
    dashed(entry.parts,origin,point,color)
    for _,part in ipairs(entry.parts)do part.Transparency=.65 end
   else for _,part in ipairs(entry.parts)do part.Transparency=1 end end
  end
  local nearest=nil;local gap=math.huge
  for _,d in ipairs(list)do local n=flat(d.position-root.Position).Magnitude;if n<gap then gap=n;nearest=d end end
  defenderInfo.gui.Enabled=nearest~=nil
  if nearest then
   local point=ground(nearest.position,root);defenderInfo.anchor.Position=point
   defenderInfo.text.Text=string.format("DEFENDER: %.1f studs\nMoving %.1f studs/s",gap,nearest.velocity.Magnitude)
   dashed(defenderLine,origin,point,Color3.fromRGB(255,100,110))
  end
  local frozen=lastShotVisual and now-lastShotVisual.at<3
  shotInfo.gui.Enabled=frozen==true
  if frozen then
   local point=ground(lastShotVisual.origin,root);shotInfo.anchor.Position=point
   shotInfo.text.Text=lastShotVisual.text;shotInfo.text.TextColor3=Color3.fromRGB(190,140,255)
   if lastShotVisual.defender then dashed(shotLines,point,ground(lastShotVisual.defender,root),Color3.fromRGB(190,140,255))end
  else for _,part in ipairs(shotLines)do part.Transparency=1 end end
 end
 local function curve(parts,start,finish,bend,root,color,alpha,width)
  local middle=(start+finish)*.5+bend
  local function sample(t)
   return ground(start*((1-t)*(1-t))+middle*(2*(1-t)*t)+finish*(t*t),root)
  end
  for i,part in ipairs(parts)do
   segment(part,sample((i-1)/#parts),sample(i/#parts),color,width)
   part.Transparency=alpha
  end
 end
 local visualAt=0
 on(Run.Heartbeat,function()
  if stopped or os.clock()-visualAt<1/30 then return end;visualAt=os.clock()
  local own=model();local root=rootOf(own)
  local shown=running and settings.farmVisuals and root and state.phase=="match" and plan.at and os.clock()-plan.at<.25
  hideLabels()
  for _,parts in ipairs({preview,defenderLine,arrows,marker,shotLines})do
   for _,part in ipairs(parts)do part.Transparency=1 end
  end
  for _,entry in ipairs(candidates)do for _,part in ipairs(entry.parts)do part.Transparency=1 end end
  if not shown then for _,part in ipairs(route)do part.Transparency=1 end;return end
  local origin=root.Position
  local direction=plan.dir or Vector3.zero
  local target=state.visualTarget or(retreat and(retreat.origin+(retreat.direction or -flat(root.CFrame.LookVector))*5.6))or(origin+direction*7)
  local delta=flat(target-origin)
  local finish=target
  local selected=state.selectedCandidate
  local fresh=state.planEvaluatedAt and os.clock()-state.planEvaluatedAt<.3
  local bend=fresh and selected and selected.bend or Vector3.zero
  curve(route,origin,finish,bend,root,Color3.fromRGB(70,195,255),.12,.12)
  -- One small selected-action label; no diagnostic paragraphs or player rings.
  if delta.Magnitude>1 then
   defenderInfo.gui.Enabled=true;defenderInfo.anchor.Position=ground(finish,root)
   defenderInfo.gui.Size=UDim2.fromOffset(240,52)
   defenderInfo.text.Text=escapePlan and "RESET"or shotStart and "SHOT"or spaceHeld and "FINISH"or retreat and "STEPBACK"or "MOVE"
   if fresh and selected and selected.kind=="drive"then
    defenderInfo.text.Text=string.format("BEST DRIVE: %s\nYou %.2fs / defender %.2fs | %+.2fs",selected.name,selected.ourTime,selected.theirTime,selected.margin)
    if selected.defender and selected.intercept then
     dashed(defenderLine,ground(selected.defender,root),ground(selected.intercept,root),Color3.fromRGB(255,110,115))
     dashed(preview,ground(selected.origin,root),ground(selected.intercept,root),Color3.fromRGB(85,200,255))
    end
   elseif fresh and selected then defenderInfo.text.Text=selected.text end
   defenderInfo.text.TextColor3=Color3.fromRGB(100,210,255)
  end
  if escapePlan or not holdsBall(own)then return end
  local hoop=rimFor(own,root,os.clock());if not hoop then return end
  candidateCache=fresh and state.planCandidates or {}
  local count=0
  for _,data in ipairs(candidateCache)do
   local offset=flat(data.point-origin)
   if offset.Magnitude>2 and flat(data.point-target).Magnitude>3 then
    count+=1;if count>5 then break end
    local endpoint=data.point
    curve(candidates[count].parts,origin,endpoint,data.bend or Vector3.zero,root,palette[data.quality],.72,.07)
    if data.kind=="drive"or data.name=="STEPBACK"then
     local entry=candidates[count];entry.gui.Enabled=true;entry.anchor.Position=ground(endpoint,root)
     entry.gui.Size=UDim2.fromOffset(data.name=="STEPBACK"and 265 or 175,data.name=="STEPBACK"and 48 or 32)
     entry.text.Text=data.name=="STEPBACK"and data.text or string.format("%s | score %.1f",data.name,data.score)
     entry.text.TextColor3=palette[data.quality]
    end
   end
  end
 end)

 on(Run.Heartbeat,function()
  if stopped or not running then return end
  local now=os.clock()
  local interval=(retreat or confirmedStepback or farmMove or shotStart)and 0 or 1/30
  if now-lastThink<interval then return end
  lastThink=now
  local ok,err=pcall(think,now)
  local sprint=ok and settings._farmAttack and not shotStart and not retreat and not confirmedStepback
   and((farmMove and farmMove.move.sprint)or(not farmMove and plan.dir and plan.dir.Magnitude>.1))or false
  if settings._farmAttack then settings._farmSprintWanted=sprint else settings._farmSprintWanted=nil end
  syncSprint(keyboard and keyboard.Services and keyboard.Services.InputService,sprint,now)
  if state.status~=lastPrinted and (not state.printAt or now-state.printAt>.5)then state.printAt=now; lastPrinted=state.status;print("[Autofarm] "..tostring(state.status)..(state.seen and " | "..state.seen or ""))end
  if ok then plan.at=now else plan.dir=nil;settings._farmPlantUntil=nil;settings._farmGuard=false;settings._farmAttack=false;state.status="Autofarm error: "..tostring(err)end
 end)
 settings._farmDirection=function()
  if not running or os.clock()-plan.at>.2 or not plan.dir then return nil end
  local direction=plan.dir
  if direction.Magnitude>.01 and settings._courtSteer then direction=settings._courtSteer(rootOf(model()),direction)end
  return direction,plan.reason
 end

 -- Saved ON from last time: confirm before doing anything.
 if loadConfig()then
  settings.farm=true
  task.spawn(function()
   task.wait(1)
   if not stopped and not running and settings.farm then prompt(COUNTDOWN)end
  end)
 end

 local previous=featureCleanup
 featureCleanup=function()
  settings._rimStopAttempt=nil
  stopped=true;visualFolder:Destroy();countdownToken=countdownToken+1;closePrompt()
  disarm(false)
  bindingDown=baseBindingDown;settings._farmDirection=nil
  previous()
 end
end)
 if not farmLoaded then
  warn("[Autofarm] failed to load: "..tostring(farmError))
  api.SetFarm=function()return false,"Autofarm failed to load: "..tostring(farmError)end
  api.FarmReadyNow=api.SetFarm
  api.GetFarmState=function()return {running=false,status="Failed to load: "..tostring(farmError),ready=0,dunks=0,loose=0,crosses=0,queue="n/a"}end
 end
end)() end
-- Guard positioning with sampled reactions, bounded turning and occasional idle stance changes.
do (function()
 settings.guardHuman=true;settings.guardHumanLevel=60;settings.guardIdleMovement=true
 settings.rimPress=true;settings.rimPressRange=18;settings.rimPressGap=1.3
 numericLimits.guardHumanLevel={0,100};numericLimits.rimPressRange={8,30};numericLimits.rimPressGap={.75,3}
 local aim,observed,identity,lastAt,nextRead,holding=nil,nil,nil,0,0,false
 local rng=Random.new()
 local idle={offset=Vector3.zero,target=Vector3.zero,nextAt=0,quietSince=nil}
 local function resetIdle(now)
  idle.offset=Vector3.zero;idle.target=Vector3.zero;idle.quietSince=nil
  idle.nextAt=(now or os.clock())+rng:NextNumber(.6,1.2)
  settings._humanIdleActive=false
 end
 local function idleOffset(desired,handlerPosition,ownPosition,velocity,basketDirection,urgent,level,now,dt)
  if urgent or level==0 or not settings.guardIdleMovement or not handlerPosition or not ownPosition
   or not basketDirection or flat(velocity or Vector3.zero).Magnitude>2
   or flat(desired-ownPosition).Magnitude>4.5 or flat(ownPosition-handlerPosition).Magnitude<2.8 then
   resetIdle(now);return Vector3.zero
  end
  idle.quietSince=idle.quietSince or now
  if now-idle.quietSince>.4 and now>=idle.nextAt then
   local tangent=Vector3.new(-basketDirection.Z,0,basketDirection.X)
   local amplitude=math.max(1.25,(settings.guardDeadzone or .65)+.65)+.4*level
   local direction=rng:NextNumber()<.5 and -1 or 1
   local nextTarget=idle.target.Magnitude>.5 and rng:NextNumber()<.3 and Vector3.zero or tangent*direction*amplitude
   if (nextTarget-idle.target).Magnitude<.75 then nextTarget=idle.target.Magnitude>.5 and Vector3.zero or -tangent*direction*amplitude end
   idle.target=nextTarget
   idle.nextAt=now+rng:NextNumber(1.4,3)
   holding=false
  end
  idle.offset=idle.offset:Lerp(idle.target,1-math.exp(-dt*7))
  settings._humanIdleActive=idle.offset.Magnitude>.05
  return idle.offset
 end
 settings._humanReset=function()
  aim=nil;observed=nil;identity=nil;lastAt=0;nextRead=0;holding=false;resetIdle()
 end
 settings._humanize=function(desired,dt,handler,urgent,handlerPosition,ownPosition,velocity,basketDirection)
  local level=math.clamp(settings.guardHumanLevel or 0,0,100)/100
  local now=os.clock();dt=math.clamp(dt or 1/60,0,.1)
  if not aim or identity~=handler or now-lastAt>.4 or flat(aim-desired).Magnitude>12 then
   resetIdle(now);aim=desired;observed=desired;identity=handler;holding=false;nextRead=now+(urgent and .025 or .05+.12*level)
  end
  lastAt=now
  if level==0 then resetIdle(now);aim=desired;observed=desired;return desired end
  -- Snapshot tracking independently from the small lateral idle offset.
  if now>=nextRead then
   nextRead=now+(urgent and .025+.025*level or .05+.12*level)
   if flat(desired-observed).Magnitude>(urgent and .04 or .12+.12*level)then observed=desired end
  end
  aim=aim:Lerp(observed,1-math.exp(-dt*(urgent and 18-6*level or 12-7*level)))
  return aim+idleOffset(desired,handlerPosition,ownPosition,velocity,basketDirection,urgent,level,now,dt)
 end
 settings._humanHold=function(distance,deadzone,urgent)
  local level=math.clamp(settings.guardHumanLevel or 0,0,100)/100
  if settings._humanIdleActive then return distance<=.25 end
  if level==0 then return distance<=deadzone end
  local enter=deadzone+(urgent and .05 or .2)*level
  local leave=enter+(urgent and .12 or .45+.5*level)
  if distance<=enter then holding=true elseif distance>=leave then holding=false end
  return holding
 end
 settings._humanSteer=function(current,desired,dt,urgent)
  local level=math.clamp(settings.guardHumanLevel or 0,0,100)/100
  if desired.Magnitude<.001 then return Vector3.zero end
  desired=flat(desired).Unit
  if level==0 or current.Magnitude<.001 then return desired end
  current=flat(current).Unit
  local angle=math.atan2(current.X*desired.Z-current.Z*desired.X,current:Dot(desired))
  local maxTurn=(urgent and 15-5*level or 11-7*level)*math.clamp(dt or 1/60,0,.1)
  local turn=math.clamp(angle,-maxTurn,maxTurn)
  local c,s=math.cos(turn),math.sin(turn)
  return Vector3.new(current.X*c-current.Z*s,0,current.X*s+current.Z*c)
 end
 local previous=featureCleanup
 featureCleanup=function()
  settings._humanReset();settings._humanize=nil;settings._humanHold=nil;settings._humanSteer=nil;settings._humanReset=nil;settings._humanIdleActive=nil
  previous()
 end
end)() end
-- Block readiness follows the observed ball lift, not one fixed shot timer.
do (function()
 local sample
 settings._blockReady=function(shooter,started,root,now)
  if not sample or sample.shooter~=shooter or sample.started~=started then
   sample={shooter=shooter,started=started,ball=attr(shooter,"Basketball"),lastAt=now,initialRelease=attr(shooter,"ReleasedShot")==true}
  end
  local held=attr(shooter,"Basketball")
  if typeof(held)=="Instance"then
   sample.ball=held;sample.sawHeld=true
   settings._blockTrackShooter=shooter;settings._blockTrackUntil=now+.25
  end
  if attr(shooter,"ReleasedShot")~=true then sample.sawUnreleased=true end
  local point=goalPosition(sample.ball)
  if not point then settings._blockDecision=nil;return false end
  local delta=flat(point-root.Position);local distance=delta.Magnitude
  local facing=distance<.1 or root.CFrame.LookVector:Dot(delta.Unit)>=.1
  local rise=0
  if sample.position and now>sample.lastAt then rise=math.clamp((point.Y-sample.position.Y)/(now-sample.lastAt),-60,60)end
  sample.position=point;sample.lastAt=now
  local height=point.Y-root.Position.Y
  local delay=settings.blockDelay or .08
  local released=(sample.sawHeld and held~=sample.ball)
   or(attr(shooter,"ReleasedShot")==true and(sample.sawUnreleased or not sample.initialRelease))
  -- Ball lift is a wind-up, not a release. Never anticipate from vertical velocity.
  local ready=released and now-started>=delay and height>=3
   and distance<=(settings.blockRange or 5) and facing and height<=7.5
  settings._blockDecision={position=point,ready=ready,delay=delay,height=height,rise=rise,
   reason=not released and "Waiting for actual release"or "Release observed"}
  return ready
 end
 local previous=featureCleanup
 featureCleanup=function()
  settings._blockTrackShooter=nil;settings._blockTrackUntil=nil;settings._blockReady=nil;settings._blockDecision=nil;settings._blockFacingDirection=nil;settings._blockFacingUntil=nil
  previous()
 end
end)()end
do (function()
local run,err=loadstring([====[
--[[
	practical · UI 2.0
	Roblox Studio LocalScript. Put it in StarterPlayer > StarterPlayerScripts.

	Connected Practical Suite interface. Use PracticalComplete.luau to run the engine and UI together.

	  RightShift ............ show / hide (rebindable in Preferences)
	  Ctrl + K .............. search settings
	  Double-click title .... maximise / restore
	  Drag bottom-right ..... resize
]]

local runtime=(getgenv and getgenv()) or _G
local api=assert(runtime.PracticalSuiteAPI,"Run PracticalComplete.luau to load the feature engine")
local UI = {} -- Private helper namespace; avoids Luau's 200-local limit.
local Players = game:GetService("Players")
local RunService = game:GetService("RunService")
local TextService = game:GetService("TextService")
local TweenService = game:GetService("TweenService")
local UserInputService = game:GetService("UserInputService")

local player = assert(Players.LocalPlayer, "Run this UI as a LocalScript on the client")
local playerGui = player:WaitForChild("PlayerGui")

-------------------------------------------------------------------- lifecycle
local alive = true
local connections = {}

-- long-lived service connections; all dropped when the ScreenGui is destroyed
function UI.connect(signal, fn)
	local c = signal:Connect(function(...)
		if alive then
			fn(...)
		end
	end)
	table.insert(connections, c)
	return c
end

function UI.after(t, fn)
	return task.delay(t, function()
		if alive then
			fn()
		end
	end)
end

-------------------------------------------------------------------- config
local VERSION = "2.0"
local DEFAULT_SCALE = 1.0
local DEFAULT_KEY = Enum.KeyCode.RightShift
local FONT_FAMILY = "rbxasset://fonts/families/BuilderSans.json"
local FONT_ENUM = Enum.Font.BuilderSans -- same family, only used to measure text

local C = {
	bg = Color3.fromRGB(10, 12, 18),
	rail = Color3.fromRGB(13, 16, 23),
	surface = Color3.fromRGB(17, 20, 29),
	raised = Color3.fromRGB(23, 27, 38),
	field = Color3.fromRGB(27, 32, 45),
	hover = Color3.fromRGB(32, 38, 53),
	line = Color3.fromRGB(31, 37, 51),
	stroke = Color3.fromRGB(40, 47, 64),
	strokeHi = Color3.fromRGB(68, 80, 106),
	text = Color3.fromRGB(240, 242, 248),
	text2 = Color3.fromRGB(196, 202, 216),
	sub = Color3.fromRGB(132, 141, 162),
	muted = Color3.fromRGB(88, 96, 116),
	white = Color3.new(1, 1, 1),
	black = Color3.new(0, 0, 0),
	green = Color3.fromRGB(52, 211, 120),
	danger = Color3.fromRGB(255, 104, 112),
	dangerFill = Color3.fromRGB(190, 52, 64),
}

-- Accent pairs: `a` is the main accent, `b` the gradient partner.
local THEMES = {
	{ name = "Cobalt", a = Color3.fromRGB(72, 134, 255), b = Color3.fromRGB(134, 98, 255) },
	{ name = "Violet", a = Color3.fromRGB(152, 100, 255), b = Color3.fromRGB(236, 92, 210) },
	{ name = "Aqua", a = Color3.fromRGB(34, 196, 222), b = Color3.fromRGB(66, 122, 255) },
	{ name = "Emerald", a = Color3.fromRGB(38, 204, 140), b = Color3.fromRGB(46, 170, 230) },
	{ name = "Sunset", a = Color3.fromRGB(255, 128, 72), b = Color3.fromRGB(255, 72, 132) },
	{ name = "Gold", a = Color3.fromRGB(244, 184, 62), b = Color3.fromRGB(255, 118, 72) },
}
local THEME_BY_NAME, THEME_NAMES = {}, {}
for _, th in ipairs(THEMES) do
	THEME_BY_NAME[th.name] = th
	table.insert(THEME_NAMES, th.name)
end

-- live accent palette, re-derived whenever the theme changes
local A = {}
function UI.deriveTheme(th)
	A.name = th.name
	A.a = th.a
	A.b = th.b
	A.hi = th.a:Lerp(C.white, 0.25)
	A.soft = th.a:Lerp(C.bg, 0.45)
	A.dim = th.a:Lerp(C.bg, 0.78)
	-- Keep the on-screen HUD cards in the menu's accent.
	if api.SetUIAccent then
		pcall(api.SetUIAccent, th.a, th.b)
	end
end
UI.deriveTheme(THEMES[1])

-------------------------------------------------------------------- motion
local QUINT = Enum.EasingStyle.Quint
local BACK = Enum.EasingStyle.Back
local SINE = Enum.EasingStyle.Sine
local LINEAR = Enum.EasingStyle.Linear
local OUT = Enum.EasingDirection.Out
local IN = Enum.EasingDirection.In
local INOUT = Enum.EasingDirection.InOut

-- Driven by the Animation page: duration multiplier, default easing, reduce-motion.
local MOTION = { factor = 1, style = QUINT, reduce = false }

function UI.motionTime(t)
	if MOTION.reduce then
		return math.min(t, 0.06)
	end
	return t * MOTION.factor
end

-- Every transition goes through here. t <= 0 applies instantly and returns nil;
-- any t > 0 always returns a playing Tween (safe to chain .Completed).
function UI.tween(inst, t, goal, style, dir, delay)
	if t <= 0 then
		for k, v in pairs(goal) do
			inst[k] = v
		end
		return nil
	end
	local info = TweenInfo.new(
		math.max(UI.motionTime(t), 0.01),
		style or MOTION.style,
		dir or OUT,
		0,
		false,
		delay and UI.motionTime(delay) or 0
	)
	local tw = TweenService:Create(inst, info, goal)
	tw:Play()
	return tw
end

-------------------------------------------------------------------- theme registry
-- Static accent-coloured properties register once; a theme switch re-tints them all.
-- Anything whose colour depends on state (switches, checkboxes, nav) reads A.* when it paints.
-- (Strong table pruned on Destroying: Roblox can drop an Instance from a weak-keyed table
-- while it is still alive, which would silently break re-tinting.)
local themed = {}

function UI.themeValue(key)
	if key == "grad" then
		return ColorSequence.new(A.a, A.b)
	elseif key == "border" then
		return ColorSequence.new({
			ColorSequenceKeypoint.new(0, C.stroke),
			ColorSequenceKeypoint.new(0.34, C.stroke),
			ColorSequenceKeypoint.new(0.46, A.a),
			ColorSequenceKeypoint.new(0.54, A.b),
			ColorSequenceKeypoint.new(0.66, C.stroke),
			ColorSequenceKeypoint.new(1, C.stroke),
		})
	end
	return A[key]
end

function UI.bindTheme(inst, prop, key)
	local map = themed[inst]
	if not map then
		map = {}
		themed[inst] = map
		inst.Destroying:Connect(function()
			themed[inst] = nil
		end)
	end
	map[prop] = key
	inst[prop] = UI.themeValue(key)
	return inst
end

function UI.applyThemeRegistry(t)
	for inst, map in pairs(themed) do
		if inst.Parent then
			for prop, key in pairs(map) do
				local v = UI.themeValue(key)
				if typeof(v) == "Color3" and t > 0 then
					UI.tween(inst, t, { [prop] = v })
				else
					inst[prop] = v
				end
			end
		end
	end
end

-------------------------------------------------------------------- builders
local MEDIUM = Enum.FontWeight.Medium
local SEMIBOLD = Enum.FontWeight.SemiBold
local BOLD = Enum.FontWeight.Bold

function UI.make(class, props, children)
	local inst = Instance.new(class)
	local parent
	for k, v in pairs(props or {}) do
		if k == "Parent" then
			parent = v
		else
			inst[k] = v
		end
	end
	for _, child in ipairs(children or {}) do
		child.Parent = inst
	end
	inst.Parent = parent
	return inst
end

function UI.font(weight)
	return Font.new(FONT_FAMILY, weight or Enum.FontWeight.Regular)
end

function UI.textWidth(str, size)
	return TextService:GetTextSize(str, size, FONT_ENUM, Vector2.new(4000, 200)).X
end

function UI.trim(s)
	return (string.gsub(s, "^%s*(.-)%s*$", "%1"))
end

function UI.corner(r)
	return UI.make("UICorner", { CornerRadius = UDim.new(0, r) })
end

function UI.round()
	return UI.make("UICorner", { CornerRadius = UDim.new(1, 0) })
end

function UI.stroke(color, thickness, transparency)
	return UI.make("UIStroke", {
		Color = color,
		Thickness = thickness or 1,
		Transparency = transparency or 0,
		ApplyStrokeMode = Enum.ApplyStrokeMode.Border,
	})
end

function UI.padding(l, r, t, b)
	return UI.make("UIPadding", {
		PaddingLeft = UDim.new(0, l or 0),
		PaddingRight = UDim.new(0, r or 0),
		PaddingTop = UDim.new(0, t or 0),
		PaddingBottom = UDim.new(0, b or 0),
	})
end

function UI.accentGradient(rotation, transparency)
	local g = UI.make("UIGradient", { Rotation = rotation or 0 })
	if transparency then
		g.Transparency = transparency
	end
	return UI.bindTheme(g, "Color", "grad")
end

-- transparent at both ends, solid in the middle (hairline highlights)
local EDGE_FADE = NumberSequence.new({
	NumberSequenceKeypoint.new(0, 1),
	NumberSequenceKeypoint.new(0.5, 0),
	NumberSequenceKeypoint.new(1, 1),
})

-- solid strip positioned with UDim2s (lines that stretch or hug an edge)
function UI.strip(parent, pos, size, color)
	return UI.make("Frame", {
		BackgroundColor3 = color or C.line,
		BorderSizePixel = 0,
		Position = pos,
		Size = size,
		Parent = parent,
	})
end

-- Label at (x, y) = left edge (right with ax = 1, centre with ax = 0.5) / vertical centre.
-- `x` may be a UDim2, in which case `y` is ignored.
local function text(parent, str, x, y, size, opts)
	opts = opts or {}
	local ax = opts.ax or 0
	local label = UI.make(opts.button and "TextButton" or "TextLabel", {
		BackgroundTransparency = 1,
		Text = str,
		RichText = opts.rich == true,
		FontFace = UI.font(opts.weight),
		TextSize = size,
		TextColor3 = opts.color or C.text,
		TextXAlignment = ax == 1 and Enum.TextXAlignment.Right
			or ax == 0.5 and Enum.TextXAlignment.Center
			or Enum.TextXAlignment.Left,
		AutomaticSize = Enum.AutomaticSize.X,
		AnchorPoint = Vector2.new(ax, 0.5),
		Position = typeof(x) == "UDim2" and x or UDim2.fromOffset(x, y),
		Size = UDim2.fromOffset(0, size + 8),
		ZIndex = opts.z or 1,
		Parent = parent,
	})
	if opts.button then
		label.AutoButtonColor = false
	end
	return label
end

-- fixes a label's width (offset or UDim) and truncates with an ellipsis
function UI.clip(label, w)
	label.AutomaticSize = Enum.AutomaticSize.None
	local h = label.Size.Y.Offset
	if typeof(w) == "UDim" then
		label.Size = UDim2.new(w.Scale, w.Offset, 0, h)
	else
		label.Size = UDim2.fromOffset(w, h)
	end
	label.TextTruncate = Enum.TextTruncate.AtEnd
	return label
end

-- Letter-spaced caps ("MENU"). Roblox has no tracking, so one label per glyph.
function UI.tracked(parent, str, x, y, size, color, spacing)
	local holder = UI.make("Frame", {
		BackgroundTransparency = 1,
		AutomaticSize = Enum.AutomaticSize.X,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = typeof(x) == "UDim2" and x or UDim2.fromOffset(x, y),
		Size = UDim2.fromOffset(0, size + 8),
		Parent = parent,
	})
	UI.make("UIListLayout", {
		FillDirection = Enum.FillDirection.Horizontal,
		Padding = UDim.new(0, spacing or 1.5),
		SortOrder = Enum.SortOrder.LayoutOrder,
		VerticalAlignment = Enum.VerticalAlignment.Center,
		Parent = holder,
	})
	for i = 1, #str do
		UI.make("TextLabel", {
			BackgroundTransparency = 1,
			Text = str:sub(i, i),
			FontFace = UI.font(SEMIBOLD),
			TextSize = size,
			TextColor3 = color or C.sub,
			AutomaticSize = Enum.AutomaticSize.X,
			Size = UDim2.fromOffset(0, size + 8),
			LayoutOrder = i,
			Parent = holder,
		})
	end
	return holder
end

-------------------------------------------------------------------- icons (24 x 24 stroke icons)
function UI.push(p, x, y)
	local last = p[#p]
	if last and math.abs(last[1] - x) < 1e-3 and math.abs(last[2] - y) < 1e-3 then
		return
	end
	p[#p + 1] = { x, y }
end

-- degrees, screen space (0 = right, 90 = down)
function UI.arc(p, cx, cy, r, a0, a1, steps)
	steps = steps or 6
	for i = 0, steps do
		local a = math.rad(a0 + (a1 - a0) * i / steps)
		UI.push(p, cx + r * math.cos(a), cy + r * math.sin(a))
	end
end

function UI.cubic(p, x0, y0, x1, y1, x2, y2, x3, y3, steps)
	steps = steps or 8
	UI.push(p, x0, y0)
	for i = 1, steps do
		local t = i / steps
		local u = 1 - t
		local a, b, c, d = u * u * u, 3 * u * u * t, 3 * u * t * t, t * t * t
		UI.push(p, a * x0 + b * x1 + c * x2 + d * x3, a * y0 + b * y1 + c * y2 + d * y3)
	end
end

function UI.S(pts, closed)
	return { pts = pts, closed = closed or false }
end

function UI.circle(cx, cy, r, steps)
	local c = {}
	steps = steps or 20
	UI.arc(c, cx, cy, r, 0, 360 - 360 / steps, steps - 1)
	return UI.S(c, true)
end

function UI.rect(x0, y0, x1, y1)
	return UI.S({ { x0, y0 }, { x1, y0 }, { x1, y1 }, { x0, y1 } }, true)
end

local ICONS = {}
ICONS.minus = function()
	return { UI.S({ { 5, 12 }, { 19, 12 } }) }
end
ICONS.plus = function()
	return { UI.S({ { 5, 12 }, { 19, 12 } }), UI.S({ { 12, 5 }, { 12, 19 } }) }
end
ICONS.close = function()
	return { UI.S({ { 6, 6 }, { 18, 18 } }), UI.S({ { 18, 6 }, { 6, 18 } }) }
end
ICONS.restore = function()
	return { UI.rect(2.5, 8.5, 16.5, 20.5), UI.S({ { 7, 8.5 }, { 7, 4 }, { 21.5, 4 }, { 21.5, 16 }, { 16.5, 16 } }) }
end
ICONS.grip = function()
	return { UI.S({ { 21, 10 }, { 10, 21 } }), UI.S({ { 21, 16 }, { 16, 21 } }) }
end
ICONS.chevronRight = function()
	return { UI.S({ { 9, 6 }, { 15, 12 }, { 9, 18 } }) }
end
ICONS.check = function()
	return { UI.S({ { 20, 6 }, { 9, 17 }, { 4, 12 } }) }
end
ICONS.search = function()
	return { UI.circle(11, 11, 7, 24), UI.S({ { 21, 21 }, { 16.2, 16.2 } }) }
end
ICONS.logo = function()
	return { UI.rect(3, 9.5, 14.5, 21), UI.S({ { 9.5, 9.5 }, { 9.5, 3 }, { 21, 3 }, { 21, 14.5 }, { 14.5, 14.5 } }) }
end
ICONS.home = function()
	return {
		UI.S({ { 3.5, 11 }, { 12, 3.5 }, { 20.5, 11 } }),
		UI.S({ { 5.5, 9.5 }, { 5.5, 20 }, { 18.5, 20 }, { 18.5, 9.5 } }),
		UI.S({ { 10, 20 }, { 10, 14.5 }, { 14, 14.5 }, { 14, 20 } }),
	}
end
ICONS.grid = function()
	return { UI.rect(3.5, 3.5, 10.5, 10.5), UI.rect(13.5, 3.5, 20.5, 10.5), UI.rect(3.5, 13.5, 10.5, 20.5), UI.rect(13.5, 13.5, 20.5, 20.5) }
end
ICONS.activity = function()
	return { UI.S({ { 2.5, 12 }, { 6.5, 12 }, { 9, 5 }, { 14, 19 }, { 16.5, 12 }, { 21.5, 12 } }) }
end
ICONS.folder = function()
	return { UI.S({ { 3, 5 }, { 9.5, 5 }, { 11.5, 8 }, { 21, 8 }, { 21, 19 }, { 3, 19 } }, true) }
end
ICONS.bell = function()
	local p = {}
	UI.push(p, 4.5, 17)
	UI.push(p, 6, 15)
	UI.push(p, 6, 10)
	UI.arc(p, 12, 10, 6, 180, 360, 10)
	UI.push(p, 18, 15)
	UI.push(p, 19.5, 17)
	return { UI.S(p, true), UI.S({ { 10, 20.5 }, { 14, 20.5 } }) }
end
ICONS.sparkle = function()
	return {
		UI.S({ { 12, 3 }, { 13.8, 10.2 }, { 21, 12 }, { 13.8, 13.8 }, { 12, 21 }, { 10.2, 13.8 }, { 3, 12 }, { 10.2, 10.2 } }, true),
	}
end
ICONS.text = function()
	return { UI.S({ { 5, 7 }, { 5, 4 }, { 19, 4 }, { 19, 7 } }), UI.S({ { 12, 4 }, { 12, 20 } }), UI.S({ { 9, 20 }, { 15, 20 } }) }
end
ICONS.play = function()
	return { UI.S({ { 7, 4.5 }, { 19, 12 }, { 7, 19.5 } }, true) }
end
ICONS.info = function()
	return { UI.circle(12, 12, 9.5, 28), UI.S({ { 12, 11 }, { 12, 16.5 } }), UI.S({ { 12, 7.4 }, { 12, 7.6 } }) }
end
ICONS.pencil = function()
	return { UI.S({ { 4, 20 }, { 4.8, 16 }, { 16, 4.8 }, { 19.2, 8 }, { 8, 19.2 } }, true), UI.S({ { 13.5, 7.3 }, { 16.7, 10.5 } }) }
end
ICONS.save = function()
	return {
		UI.S({ { 4, 4 }, { 16.5, 4 }, { 20, 7.5 }, { 20, 20 }, { 4, 20 } }, true),
		UI.S({ { 8, 4 }, { 8, 8.5 }, { 15, 8.5 }, { 15, 4 } }),
		UI.rect(7.5, 13, 16.5, 20),
	}
end
ICONS.signal = function()
	return { UI.S({ { 5, 19 }, { 5, 16 } }), UI.S({ { 10, 19 }, { 10, 12 } }), UI.S({ { 15, 19 }, { 15, 8 } }), UI.S({ { 20, 19 }, { 20, 4 } }) }
end
ICONS.clock = function()
	return { UI.circle(12, 12, 9.5, 28), UI.S({ { 12, 7 }, { 12, 12 }, { 15.5, 14 } }) }
end
ICONS.bolt = function()
	return { UI.S({ { 13, 2.5 }, { 5, 13.5 }, { 11.5, 13.5 }, { 10.5, 21.5 }, { 19, 10 }, { 12.5, 10 } }, true) }
end
ICONS.run = function()
	return {
		UI.circle(15.5, 4, 2, 14),
		UI.S({ { 14, 8 }, { 11, 14 } }),
		UI.S({ { 7.5, 10 }, { 10, 7.5 }, { 14, 8 }, { 16, 11 }, { 19, 11.5 } }),
		UI.S({ { 11, 14 }, { 14.5, 16.5 }, { 13.5, 21.5 } }),
		UI.S({ { 11, 14 }, { 9, 18 }, { 4.5, 18.5 } }),
	}
end
ICONS.wrench = function()
	local p = {}
	UI.push(p, 14.7, 6.3)
	UI.push(p, 14.4, 7.0)
	UI.push(p, 14.7, 7.7)
	UI.push(p, 16.3, 9.3)
	UI.push(p, 17.0, 9.6)
	UI.push(p, 17.7, 9.3)
	UI.push(p, 21.47, 5.53)
	UI.arc(p, 16.01, 8.01, 6, -24.4, 114.4, 10)
	UI.push(p, 6.62, 20.38)
	UI.arc(p, 5.12, 18.88, 2.12, 45, 225, 8)
	UI.push(p, 10.53, 10.47)
	UI.arc(p, 16.01, 8.01, 6, 155.8, 294.2, 10)
	return { UI.S(p, true) }
end
ICONS.gear = function()
	local p = {}
	local function pol(r, deg)
		local t = math.rad(deg)
		UI.push(p, 12 + r * math.cos(t), 12 + r * math.sin(t))
	end
	for k = 0, 7 do
		local b = k * 45
		pol(7.6, b - 16)
		pol(10, b - 9)
		pol(10, b + 9)
		pol(7.6, b + 16)
		pol(7.6, b + 22.5)
	end
	return { UI.S(p, true), UI.circle(12, 12, 3, 14) }
end
ICONS.layers = function()
	return {
		UI.S({ { 12, 2.5 }, { 21.5, 7 }, { 12, 11.5 }, { 2.5, 7 } }, true),
		UI.S({ { 2.5, 12 }, { 12, 16.5 }, { 21.5, 12 } }),
		UI.S({ { 2.5, 17 }, { 12, 21.5 }, { 21.5, 17 } }),
	}
end
ICONS.bounds = function()
	return {
		UI.S({ { 4, 9 }, { 4, 4 }, { 9, 4 } }),
		UI.S({ { 15, 4 }, { 20, 4 }, { 20, 9 } }),
		UI.S({ { 20, 15 }, { 20, 20 }, { 15, 20 } }),
		UI.S({ { 9, 20 }, { 4, 20 }, { 4, 15 } }),
	}
end
ICONS.trash = function()
	return {
		UI.S({ { 3, 6 }, { 21, 6 } }),
		UI.S({ { 9, 6 }, { 9, 3.5 }, { 15, 3.5 }, { 15, 6 } }),
		UI.S({ { 5.5, 6 }, { 6.5, 21 }, { 17.5, 21 }, { 18.5, 6 } }),
		UI.S({ { 10, 10.5 }, { 10, 17 } }),
		UI.S({ { 14, 10.5 }, { 14, 17 } }),
	}
end
ICONS.refresh = function()
	local a, b = {}, {}
	UI.arc(a, 12, 12, 9, 180, 318, 12)
	UI.push(a, 21, 8)
	UI.arc(b, 12, 12, 9, 0, 138, 12)
	UI.push(b, 3, 16)
	return { UI.S(a), UI.S(b), UI.S({ { 21, 3 }, { 21, 8 }, { 16, 8 } }), UI.S({ { 8, 16 }, { 3, 16 }, { 3, 21 } }) }
end
ICONS.shield = function()
	local p = {}
	UI.cubic(p, 20, 13, 20, 18, 16.5, 20.5, 12.34, 21.95)
	UI.push(p, 11.67, 21.94)
	UI.cubic(p, 11.67, 21.94, 7.5, 20.5, 4, 18, 4, 13)
	UI.arc(p, 5, 6, 1, 180, 270, 3)
	UI.cubic(p, 5, 5, 7, 5, 9.5, 3.8, 11.24, 2.28, 6)
	UI.push(p, 12, 1.98)
	UI.push(p, 12.76, 2.28)
	UI.cubic(p, 12.76, 2.28, 14.51, 3.81, 17, 5, 19, 5, 6)
	UI.arc(p, 19, 6, 1, 270, 360, 3)
	return { UI.S(p, true) }
end

-- Draws (or redraws in place) an icon as capsule strokes, optionally rotated about its centre.
function UI.drawIcon(holder, name, color, thickness, angle)
	local build = ICONS[name]
	if not build then
		return
	end
	for _, child in ipairs(holder:GetChildren()) do
		if child.Name == "IconStroke" then
			child:Destroy()
		end
	end
	thickness = thickness or 2
	local a = math.rad(angle or 0)
	local ca, sa = math.cos(a), math.sin(a)
	local size = holder.Size.X.Offset > 0 and holder.Size.X.Offset or 24
	local k = size / 24
	local function point(p)
		local x, y = p[1] - 12, p[2] - 12
		return x * ca - y * sa + 12, x * sa + y * ca + 12
	end
	for _, shape in ipairs(build()) do
		local count = #shape.pts
		local segments = shape.closed and count or count - 1
		for i = 1, segments do
			local x1, y1 = point(shape.pts[i])
			local x2, y2 = point(shape.pts[i % count + 1])
			local dx, dy = x2 - x1, y2 - y1
			local length = math.sqrt(dx * dx + dy * dy) * k
			if length > 0.01 then
				UI.make("Frame", {
					Name = "IconStroke",
					AnchorPoint = Vector2.new(0.5, 0.5),
					Position = UDim2.fromScale((x1 + x2) / 48, (y1 + y2) / 48),
					-- extended by the stroke width so rounded caps overlap at the joints
					Size = UDim2.fromOffset(length + thickness * 0.9, thickness),
					BackgroundColor3 = color,
					BorderSizePixel = 0,
					Rotation = math.deg(math.atan2(dy, dx)),
					Parent = holder,
				}, { UI.round() })
			end
		end
	end
end

function UI.icon(parent, name, cx, cy, size, color, thickness)
	local holder = UI.make("Frame", {
		BackgroundTransparency = 1,
		AnchorPoint = Vector2.new(0.5, 0.5),
		Position = typeof(cx) == "UDim2" and cx or UDim2.fromOffset(cx, cy),
		Size = UDim2.fromOffset(size, size),
		Parent = parent,
	})
	UI.drawIcon(holder, name, color or C.text, thickness)
	return holder
end

-- recolours an icon without redrawing it
function UI.tintIcon(holder, color, t)
	for _, child in ipairs(holder:GetChildren()) do
		if child.Name == "IconStroke" then
			UI.tween(child, t or 0, { BackgroundColor3 = color })
		end
	end
end

-- Smoothly rotates an icon by redrawing its points (works inside clipped frames).
local spinning = {}
function UI.spinIcon(holder, name, color, thickness, toAngle, t)
	local st = spinning[holder]
	if not st then
		st = { angle = 0 }
		spinning[holder] = st
		holder.Destroying:Connect(function()
			spinning[holder] = nil
		end)
	end
	if st.tween then
		st.tween:Cancel()
	end
	if st.value then
		st.value:Destroy()
	end
	if t <= 0 then
		st.angle = toAngle
		UI.drawIcon(holder, name, color, thickness, toAngle)
		return
	end
	local nv = Instance.new("NumberValue")
	nv.Value = st.angle
	nv.Changed:Connect(function(v)
		st.angle = v
		UI.drawIcon(holder, name, color, thickness, v)
	end)
	st.value = nv
	st.tween = TweenService:Create(nv, TweenInfo.new(math.max(UI.motionTime(t), 0.01), QUINT, OUT), { Value = toAngle })
	st.tween:Play()
end

-------------------------------------------------------------------- effects
-- Squish on press, spring back on release.
function UI.pressable(btn, depth)
	local s = UI.make("UIScale", { Parent = btn })
	btn.MouseButton1Down:Connect(function()
		UI.tween(s, 0.1, { Scale = depth or 0.96 }, QUINT)
	end)
	local function release()
		UI.tween(s, 0.4, { Scale = 1 }, BACK)
	end
	btn.MouseButton1Up:Connect(release)
	btn.MouseLeave:Connect(release)
	return s
end

-- Material-style ink ripple from the pointer.
function UI.ripple(btn, color)
	btn.ClipsDescendants = true
	btn.MouseButton1Down:Connect(function()
		local size = btn.AbsoluteSize
		if size.X <= 0 or size.Y <= 0 then
			return
		end
		local rel = (UserInputService:GetMouseLocation() - btn.AbsolutePosition) / size
		local dot = UI.make("Frame", {
			Name = "Ripple",
			BackgroundColor3 = color or C.white,
			BackgroundTransparency = 0.8,
			BorderSizePixel = 0,
			AnchorPoint = Vector2.new(0.5, 0.5),
			Position = UDim2.fromScale(rel.X, rel.Y),
			Size = UDim2.fromScale(0, 0),
			ZIndex = 0,
			Parent = btn,
		}, { UI.round(), UI.make("UIAspectRatioConstraint", { AspectRatio = 1, DominantAxis = Enum.DominantAxis.Width }) })
		UI.tween(dot, 0.6, { Size = UDim2.fromScale(2.4, 2.4), BackgroundTransparency = 1 }, QUINT).Completed:Connect(function()
			dot:Destroy()
		end)
	end)
end

-- A soft highlight sweeps across the button on hover.
function UI.shimmer(btn)
	btn.ClipsDescendants = true
	local sheen = UI.make("Frame", {
		BackgroundColor3 = C.white,
		BorderSizePixel = 0,
		AnchorPoint = Vector2.new(0.5, 0),
		Position = UDim2.fromScale(-0.5, 0),
		Size = UDim2.fromScale(0.6, 1),
		ZIndex = 0,
		Parent = btn,
	}, {
		UI.make("UIGradient", {
			Rotation = 15,
			Transparency = NumberSequence.new({
				NumberSequenceKeypoint.new(0, 1),
				NumberSequenceKeypoint.new(0.5, 0.72),
				NumberSequenceKeypoint.new(1, 1),
			}),
		}),
	})
	btn.MouseEnter:Connect(function()
		sheen.Position = UDim2.fromScale(-0.5, 0)
		UI.tween(sheen, 0.75, { Position = UDim2.fromScale(1.5, 0) }, SINE, INOUT)
	end)
end

-- Slow expanding ring behind a status dot.
function UI.pulse(dot, color)
	local halo = UI.make("Frame", {
		BackgroundColor3 = color,
		BackgroundTransparency = 0.55,
		BorderSizePixel = 0,
		AnchorPoint = Vector2.new(0.5, 0.5),
		Position = UDim2.fromScale(0.5, 0.5),
		Size = UDim2.fromScale(1, 1),
		Parent = dot,
	}, { UI.round() })
	TweenService:Create(halo, TweenInfo.new(1.8, SINE, OUT, -1), {
		Size = UDim2.fromScale(2.8, 2.8),
		BackgroundTransparency = 1,
	}):Play()
	return halo
end

-- Fires `fn` once on press, then repeats while held.
function UI.repeatButton(btn, fn)
	local token = 0
	btn.MouseButton1Down:Connect(function()
		token += 1
		local mine = token
		fn()
		UI.after(0.38, function()
			while token == mine and btn.Parent do
				fn()
				task.wait(0.06)
			end
		end)
	end)
	local function stop()
		token += 1
	end
	btn.MouseButton1Up:Connect(stop)
	btn.MouseLeave:Connect(stop)
end

-- Quick side-to-side shake (invalid input).
function UI.shake(obj)
	local base = obj.Position
	task.spawn(function()
		for _, dx in ipairs({ -9, 8, -6, 4, -2, 0 }) do
			obj.Position = base + UDim2.fromOffset(dx, 0)
			task.wait(0.04)
		end
		obj.Position = base
	end)
end

-- Fades everything inside `host` as one layer. CanvasGroups render text into a texture, which
-- softens it, so the children only live inside one for the duration of a fade-in and are moved
-- back out once fully visible. A fade to 1 leaves them wrapped (hidden) until the next fade-in.
local fades, fadeWatched = {}, {}
function UI.groupFade(host, toT, t, delay, style, dir)
	local st = fades[host]
	if not st then
		if not fadeWatched[host] then
			fadeWatched[host] = true
			host.Destroying:Once(function()
				fades[host] = nil
				fadeWatched[host] = nil
			end)
		end
		local cg = UI.make("CanvasGroup", {
			Name = "Fade",
			BackgroundTransparency = 1,
			Size = UDim2.fromScale(1, 1),
			GroupTransparency = toT == 0 and 1 or 0,
			Parent = host,
		})
		for _, child in ipairs(host:GetChildren()) do
			if child ~= cg and child:IsA("GuiObject") then
				child.Parent = cg
			end
		end
		st = { cg = cg }
		fades[host] = st
	end
	if st.tween then
		st.tween:Cancel()
	end
	local info = TweenInfo.new(math.max(UI.motionTime(t), 0.01), style or QUINT, dir or OUT, 0, false, delay and UI.motionTime(delay) or 0)
	local tw = TweenService:Create(st.cg, info, { GroupTransparency = toT })
	st.tween = tw
	tw.Completed:Connect(function(state)
		if state ~= Enum.PlaybackState.Completed or st.tween ~= tw or toT ~= 0 then
			return
		end
		for _, child in ipairs(st.cg:GetChildren()) do
			child.Parent = host
		end
		st.cg:Destroy()
		fades[host] = nil
	end)
	tw:Play()
	return tw
end

-- Layered soft shadow. `host` must be a transparent frame sized like the thing it shades,
-- placed BEHIND it as a sibling (children always draw over their parent).
function UI.softShadow(host, radius, layers, spread, opacity, dy)
	for i = layers, 1, -1 do
		local g = i * spread
		UI.make("Frame", {
			Name = "Shade",
			BackgroundColor3 = C.black,
			BackgroundTransparency = 1 - opacity,
			BorderSizePixel = 0,
			Position = UDim2.fromOffset(-g, -g + (dy or 0)),
			Size = UDim2.new(1, g * 2, 1, g * 2),
			Parent = host,
		}, { UI.corner(radius + g) })
	end
end

-------------------------------------------------------------------- data
-- Sidebar pages. Each entry in `controls` is either
--   a feature card  { id, name, desc, icon, enabled, rows }   ON/OFF switch; ON opens its settings
--   a section       { id, kind = "section", name, rows }       always-visible settings
-- Pages with `tabs` show one tab's cards at a time (each card names its `tab`).
-- Row types: slider · segment · key · swatch · checks · actions · note · textPreview · motionPreview
local PREF_SCALE = {
	type = "slider", id = "uiScale", label = "Interface scale", desc = "Size of every panel on screen",
	min = 0.4, max = 1.5, step = 0.01, nudge = 0.05, value = DEFAULT_SCALE, format = "%.2f", stepper = true,
}
local PREF_KEY = { type = "key", id = "openKey", label = "Open menu key", desc = "Shows or hides this window", value = DEFAULT_KEY }
local PREF_NOTICE = { id = "hiddenNotice", label = "Remind me of the open key when hidden", value = true }

local PAGES = {
 {name="Shooting",icon="ball",subtitle="Timing and release limits",tabs={"Timing","Shot limits"},controls={
  {id="autoRelease",tab="Timing",name="Auto time",enabled=false,rows={
   {type="checks",items={{id="latencyCompensation",label="Adapt to ping and jitter (experimental)",value=false}}},
   {type="slider",id="offset",label="Release adjustment",min=-30,max=30,step=1,value=0,format="%.0f",suffix=" ms"},
   {type="slider",id="maxPing",label="Maximum ping",min=50,max=1000,step=25,value=1000,format="%.0f",suffix=" ms"},
   {type="slider",id="freshness",label="Meter freshness",min=60,max=200,step=10,value=120,format="%.0f",suffix=" ms"},
  }},
  {id="shotAssist",tab="Shot limits",name="Shot assist",enabled=false,rows={
   {type="slider",id="meterMin",label="Minimum release",min=1,max=100,step=1,value=80,format="%.0f",suffix="%"},
   {type="slider",id="meterMax",label="Maximum release",min=1,max=100,step=1,value=100,format="%.0f",suffix="%"},
   {type="checks",items={{id="holdEarly",label="Hold early release",value=true},{id="protectFall",label="Prevent overhold",value=true}}},
  }},
  {id="contestCeiling",tab="Shot limits",name="Contest ceiling",enabled=false,rows={
   {type="slider",id="limit",label="Maximum contest",min=5,max=100,step=5,value=30,format="%.0f",suffix="%"},
   {type="checks",items={{id="release",label="Release before a closeout",value=false}}},
   {type="note",label="Blocks new shots above the estimate. Optional release uses safe meter timing."},
  }}
 }},
 {name="Passing",icon="ball",subtitle="Pass selection and receiver information",tabs={"Smart pass","Receivers"},controls={
  {id="catchShoot",tab="Smart pass",name="Auto shoot off pass",enabled=false,rows={
   {type="actions",items={{type="button",id="status",label="Check early-shot buffer",icon="refresh"}}},
   {type="slider",id="contest",label="Maximum estimated contest",min=0,max=25,step=5,value=10,format="%.0f",suffix="%"},
   {type="note",label="Requires Auto time. Buffers an open catch, waits for standstill, and shoots as soon as ready. Skips if the defender could arrive before release; never queues a late shot."},
  }},
  {id="smartPass",tab="Smart pass",name="Smart pass",enabled=false,rows={
   {type="checks",items={{id="spin",label="Include Shift double-tap (spin: unverified)",value=false}}},
   {type="actions",items={{type="button",id="status",label="Pass status",icon="refresh"}}},
   {type="note",label="Tap a teammate number: picks overhead, chest or bounce from estimated defender positions and pass lanes. Uses native tap, Shift-tap and double-tap inputs; no pass-type training."},
   {type="note",label="An unidentified receiver slot passes normally and is remembered for this match. Holding Shift keeps manual control. Shift double-tap is optional until its spin-pass behavior is confirmed."},
  }},
  {id="passVision",tab="Receivers",name="Pass now · open teammates",enabled=false,rows={
   {type="slider",id="speed",label="Estimated pass speed",min=40,max=140,step=5,value=80,format="%.0f",suffix=" studs/s"},
   {type="slider",id="release",label="Catch-to-release time",min=.2,max=1,step=.05,value=.5,format="%.2f",suffix=" s"},
   {type="note",label="Ranks teammate spacing and time before a defender can contest. Estimates only; cannot guarantee a green."},
  }},
  {id="passingLanes",tab="Receivers",name="Passing lanes",enabled=false,rows={
   {type="slider",id="range",label="Teammate range",min=10,max=100,step=5,value=60,format="%.0f",suffix=" studs"},
   {type="slider",id="clearance",label="Defender clearance",min=1,max=6,step=.25,value=3,format="%.2f",suffix=" studs"},
  }}
 }},
 {name="Defense",icon="shield",subtitle="Choose the part of defense to configure",tabs={"On-ball","Off-ball","Steals","Blocks","Rebounds"},controls={
  {id="autoGuard",tab="On-ball",name="Auto guard",enabled=false,rows={
   {type="checks",items={{id="cutoff",label="Cut off drives",value=false}}},
   {type="checks",items={{id="coverage",label="Opponent coverage outline",value=true}}},
   {type="slider",id="standOff",label="Stand-off distance",min=2,max=8,step=.25,nudge=.5,value=5,format="%.2f",stepper=true},
   {type="segment",id="response",label="Response",options={"Legit","Balanced","Blatant"},value="Balanced"},
   {type="slider",id="delay",label="Reaction delay",min=0,max=.4,step=.02,value=.16,format="%.2f",suffix=" s"},
   {type="key",id="activation",label="Activation",value=Enum.KeyCode.G,hint="Hold"},
   {type="checks",items={{id="fakeFilter",label="Shot-fake filter (estimate)",value=true},{id="adaptive",label="Adaptive drive spacing",value=true},{id="antiPush",label="Avoid rim contact",value=true},{id="rimBlock",label="Block nearby dunks",value=true}}},
   {type="checks",items={{id="humanize",label="Humanize movement",value=true},{id="idleMovement",label="Idle stance adjustments",value=true},{id="rimPress",label="Press up near the rim",value=true}}},
   {type="slider",id="humanLevel",label="Movement softness",min=0,max=100,step=5,value=60,format="%.0f",suffix="%"},
   {type="note",label="Occasional small lateral shuffles while the handler is stationary, with varied pauses. Higher softness adds reaction time and smoother turns. Idle adjustments stop during drives, closeouts or close contact."},
   {type="slider",id="rimRange",label="Rim press range",min=8,max=30,step=1,value=18,format="%.0f",suffix=" studs"},
   {type="slider",id="rimGap",label="Rim press distance",min=.75,max=3,step=.25,value=1.3,format="%.2f",suffix=" studs"},
  }},
  {id="screens",tab="On-ball",name="Screen avoidance",enabled=false,rows={
   {type="checks",items={{id="visual",label="Show screen warnings",value=true}}},
   {type="note",label="Auto guard routes around players blocking its path. Visual warnings are optional."},
  }},
  {id="assignmentTools",tab="On-ball",kind="section",name="Target lock",rows={
   {type="checks",items={{id="lock",label="Keep selected matchup through passes",value=false}}},
   {type="actions",items={{type="button",id="next",label="Switch matchup (B)",icon="refresh"}}},
  }},
  {id="denyReceiver",tab="Off-ball",name="Off-ball guard",enabled=false,rows={
   {type="actions",items={{type="button",id="select",label="Change matchup",icon="crosshair"},{type="button",id="clear",label="Clear matchup",icon="close"}}},
   {type="slider",id="delay",label="Reaction delay",min=0,max=.6,step=.025,value=.2,format="%.2f",suffix=" sec"},
   {type="note",label="Uses the opponent you click at game start. Hold your guard key to cover them. Your mouse and camera stay unchanged."},
  }},
  {id="autoSteal",tab="Steals",name="Auto steal",enabled=false,rows={
   {type="slider",id="reach",label="Attempt distance",min=1,max=6,step=.25,value=3.5,format="%.2f",suffix=" studs"},
   {type="slider",id="delay",label="Opportunity delay",min=.05,max=.5,step=.025,value=.12,format="%.3f",suffix=" s"},
   {type="slider",id="cooldown",label="Attempt cooldown",min=.6,max=3,step=.1,value=1.6,format="%.1f",suffix=" s"},
  }},
  {id="passIntercept",tab="Steals",name="Pass interception · hold R",enabled=false,rows={
   {type="checks",items={{id="automatic",label="Automatic catches (Auto steal required)",value=false}}},
   {type="checks",items={{id="move",label="Move into flight path",value=true}}},
   {type="slider",id="reach",label="Attempt distance",min=2,max=6,step=.25,value=4,format="%.2f",suffix=" studs"},
   {type="checks",items={{id="shots",label="Include shots and rebounds",value=false}}},
   {type="actions",items={{type="button",id="status",label="Interception status",icon="refresh"}}},
   {type="note",label="Hold R before or during a pass, or enable automatic attempts. Tracks reachable loose flights; native catch success is unverified."},
  }},
  {id="recovery",tab="Steals",name="Loose-ball marker",enabled=false,rows={
   {type="slider",id="range",label="Marker range",min=4,max=24,step=1,value=12,format="%.0f",suffix=" studs"},
   {type="slider",id="window",label="After-steal window",min=1,max=5,step=.25,value=3,format="%.2f",suffix=" s"},
  }},
  {id="autoBlock",tab="Blocks",name="Auto block",enabled=false,rows={
   {type="slider",id="reactionDelay",label="Reaction delay",min=.05,max=.5,step=.01,value=.08,format="%.2f",suffix=" s"},
   {type="checks",items={{id="commit",label="Wait for committed shot",value=false}}},
   {type="checks",items={{id="oneAttempt",label="One attempt per shot",value=true}}},
  }},
  {id="antiOop",tab="Blocks",name="Anti alley-oop",enabled=false,rows={
   {type="checks",items={{id="move",label="Move toward reachable lobs",value=true}}},
   {type="checks",items={{id="guide",label="Show interception point and route",value=true}}},
   {type="slider",id="range",label="Detection distance",min=8,max=26,step=1,value=18,format="%.0f",suffix=" studs"},
   {type="actions",items={{type="button",id="status",label="Lob planner status",icon="refresh"}}},
   {type="note",label="Tracks observed lobs. Automatic catches require Auto steal. Manual R remains available; catches are server-decided."},
  }},
  {id="rebound",tab="Rebounds",name="Auto rebound",enabled=false,rows={
   {type="checks",items={{id="chase",label="Pursue nearby balls",value=true}}},
   {type="checks",items={{id="early",label="Head to the rim when a shot is released",value=false}}},
   {type="slider",id="spot",label="Rim spot distance",min=3,max=9,step=.5,value=5.5,format="%.1f",suffix=" studs"},
   {type="slider",id="range",label="Pursuit distance",min=4,max=20,step=1,value=12,format="%.0f",suffix=" studs"},
   {type="slider",id="reach",label="Jump distance",min=2,max=10,step=.5,value=5,format="%.1f",suffix=" studs"},
   {type="slider",id="height",label="Ball height",min=3,max=12,step=.5,value=7,format="%.1f",suffix=" studs"},
   {type="slider",id="lead",label="Flight lookahead",min=0,max=.35,step=.025,value=.15,format="%.3f",suffix=" s"},
  }}
 }},
 {name="Movement",icon="run",subtitle="Dribbling and saved sequences",tabs={"Dribbling","Combos"},controls={
  {id="smartDribble",tab="Dribbling",name="Smart dribbler",enabled=false,rows={
   {type="key",id="activation",label="Hold key",value=Enum.KeyCode.H,hint="Hold"},
   {type="slider",id="cooldown",label="Combo interval",min=.07,max=.5,step=.01,value=.12,format="%.2f",suffix=" s"},
   {type="actions",items={{type="button",id="status",label="Check dribbler status",icon="refresh"}}},
   {type="note",label="Hold your key to chain dribbles. Moves react to nearby defenders and your facing direction. No hoop detection, automatic shooting, or hoop-directed movement."},
  }},
  {id="ballProtection",tab="Dribbling",name="Ball protection",enabled=false,rows={
   {type="slider",id="range",label="Pressure range",min=2,max=6,step=.25,value=4.5,format="%.2f",suffix=" studs"},
   {type="note",label="Hold the dribble key to react to ball-side pressure. Release to stop new moves. Uses the dribbler stamina reserve and move interval."},
  }},
  {id="boost",tab="Dribbling",name="Dribble boost",enabled=false,rows={
   {type="slider",id="multiplier",label="Boost",min=1,max=1.5,step=.05,value=1.15,format="%.2f",stepper=true},
   {type="slider",id="tail",label="Duration",min=.1,max=.5,step=.05,value=.25,format="%.2f",suffix=" s"},
  }},
  {id="comboBuilder",tab="Combos",name="Combo builder",enabled=false,rows={
   {type="segment",id="preset",label="Preset",options={"Custom","Crossovers","Stepback","Mixed"},value="Custom"},
   {type="combo",id="sequence",label="Sequence",value="Z@0.20 > X@0.25 > C@0.20 > V@0.30"},
   {type="slider",id="repeats",label="Sequence repeats",min=1,max=4,step=1,value=1,format="%.0f"},
   {type="checks",items={{id="sprint",label="Sprint dribble inputs",value=false}}},
   {type="key",id="activation",label="Play sequence",value=Enum.KeyCode.J,hint="Press"},
   {type="actions",items={{type="button",id="play",label="Play combo",icon="run"},{type="button",id="cancel",label="Cancel",icon="close"}}},
  }},
  {id="comboLibrary",tab="Combos",kind="section",name="Saved combos",rows={
   {type="library",id="saved",value=""},
   {type="note",label="Build a combo above, name it here, choose a hotkey and Save. Saved keys play while Dribble builder is enabled."},
  }}
 }},
 {name="Visuals",icon="layers",subtitle="On-court indicators and appearance",tabs={"Ball","Players","Style","Details"},controls={
  {id="landingTools",tab="Ball",kind="section",name="Ball flight & landing",rows={
   {type="checks",items={{id="enabled",label="Predict flight and landing",value=true}}},
   {type="checks",items={{id="trail",label="Ball motion trail",value=false}}},
   {type="checks",items={{id="beam",label="Flight beam",value=true},{id="zone",label="Landing zone",value=true}}},
   {type="slider",id="width",label="Beam width",min=.02,max=.2,step=.01,value=.065,format="%.2f"},
   {type="slider",id="scale",label="Zone size",min=.5,max=2,step=.1,value=1,format="%.1f",suffix="x"},
   {type="slider",id="range",label="Track distance",min=8,max=120,step=4,value=60,format="%.0f",suffix=" studs"},
  }},
  {id="overlayTools",tab="Players",kind="section",name="Player indicators",rows={
   {type="checks",items={{id="contest",label="Contest outline",value=true},{id="guard",label="Guard position guide",value=true}}},
  }},
  {id="defenderAlerts",tab="Players",name="Defender alerts",enabled=false,rows={
   {type="slider",id="range",label="Alert distance",min=6,max=40,step=1,value=16,format="%.0f",suffix=" studs"},
   {type="checks",items={{id="ballOnly",label="Only while I have the ball",value=false}}},
   {type="note",label="Shows an arrow badge at the screen edge for a nearby opponent you can't see (behind or beside you), with their distance. Display only; it sends no input."},
  }},
  {id="visualStyle",tab="Style",kind="section",name="Appearance",rows={
   {type="checks",items={{id="enabled",label="Modern court visuals",value=true}}},
   {type="segment",id="preset",label="Style",options={"Minimal","Balanced","Vivid"},value="Balanced"},
   {type="segment",id="palette",label="Accent",options={"Cobalt","Glacier","Mint","Amber"},value="Cobalt"},
   {type="segment",id="route",label="Route pattern",options={"Dashed","Solid","Chevrons"},value="Dashed"},
   {type="checks",items={{id="motion",label="Animated rings and pulses",value=false}}},
   {type="checks",items={{id="urgency",label="Urgency colour + timer ring",value=true},{id="beacon",label="Landing beacon",value=true}}},
   {type="checks",items={{id="flightShadow",label="Floor shadow + apex",value=true},{id="comet",label="Ball motion trail",value=true}}},
   {type="slider",id="scale",label="Marker scale",min=.6,max=1.6,step=.05,value=1,format="%.2f",suffix="x"},
   {type="slider",id="intensity",label="Glow intensity",min=10,max=100,step=5,value=65,format="%.0f",suffix="%"},
   {type="slider",id="weight",label="Line weight",min=.5,max=2,step=.05,value=1.15,format="%.2f",suffix="x"},
   {type="slider",id="smoothing",label="Display smoothing",min=0,max=100,step=5,value=55,format="%.0f",suffix="%"},
   {type="slider",id="clearance",label="Height above court",min=.04,max=.35,step=.01,value=.12,format="%.2f",suffix=" studs"},
   {type="note",label="Styles the existing flight, landing, positioning and player indicators. Disable to use the previous visuals."},
  }},
  {id="visualLayers",tab="Style",kind="section",name="Visible layers",rows={
   {type="checks",items={{id="landing",label="Landing marker",value=true},{id="flight",label="Flight path",value=true}}},
   {type="checks",items={{id="route",label="Positioning route",value=true},{id="coverage",label="Coverage indicator",value=true}}},
   {type="checks",items={{id="pass",label="Pass markers and lanes",value=true},{id="recovery",label="Recovery marker",value=true}}},
   {type="checks",items={{id="screen",label="Screen warning",value=true},{id="intercept",label="Meeting-point marker",value=true}}},
  }},
  {id="visualLabels",tab="Details",kind="section",name="HUD cards",rows={
   {type="segment",id="corner",label="Corner",options={"Bottom right","Bottom left","Top right"},value="Bottom right"},
   {type="checks",items={{id="passPanel",label="Extra pass-target corner card",value=false}}},
  }},
  {id="visualColor",tab="Details",kind="section",name="Custom accent",rows={
   {type="checks",items={{id="enabled",label="Use custom RGB accent",value=false}}},
   {type="slider",id="red",label="Red",min=0,max=255,step=1,value=64,format="%.0f"},
   {type="slider",id="green",label="Green",min=0,max=255,step=1,value=132,format="%.0f"},
   {type="slider",id="blue",label="Blue",min=0,max=255,step=1,value=255,format="%.0f"},
  }}
 }},
 {name="Tools",icon="wrench",subtitle="Queue and session utilities",controls={
  {id="autofarm",name="Blacktop autofarm",enabled=false,desc="1v1 grinder: guards, grabs loose balls, drives for dunks",rows={
   {type="checks",items={{id="postClamp",label="Clamp post-ups / resist backdowns",value=true},{id="crossover",label="Dribble moves past the defender",value=true},{id="shoot",label="Take open shots",value=true},{id="randomThrees",label="Attempts 3s sometimes",value=true},{id="visuals",label="Ground decision overlay",value=true},{id="rimStop",label="Block nearby dunks",value=true},{id="chatRequeue",label="Requeue on cheat-related chat",value=true}}},
   {type="note",label="Takes wide-open shots before driving or optional combos. Shot range automatically includes 3 studs beyond the detected arc. Jump shots stop movement; backsteps shoot as the move ends."},
   {type="note",label="Private gym: stand near your friend and press Test vs nearest friend. Uses the same offense and defense without queueing; stop the test when finished. Test mode is not saved."},
   {type="slider",id="stepbackAggression",label="Stepback aggression",min=0,max=100,step=5,value=70,format="%.0f",suffix="%"},
   {type="slider",id="shotRange",label="Shot range",min=12,max=45,step=1,value=32,format="%.0f",suffix=" studs"},
   {type="slider",id="dunkRange",label="Dunk range",min=6,max=24,step=1,value=12,format="%.0f",suffix=" studs"},
   {type="actions",items={{type="button",id="status",label="Farm status",icon="refresh"},{type="button",id="readyNow",label="Ready now",icon="crosshair"},{type="button",id="gymTest",label="Test vs nearest friend",icon="crosshair"},{type="button",id="stopGymTest",label="Stop gym test",icon="refresh"}}},
   {type="note",label="Saved when you turn it on. Every time the script runs with it on, a 5 second prompt lets you press No; otherwise it readies up, queues itself for the next teleport and farms."},
  }},
  {id="autoJoin",name="Auto join spot",enabled=false,rows={
   {type="actions",items={{type="button",id="lock",label="Lock nearest spot",icon="crosshair"},{type="button",id="status",label="Join status",icon="refresh"}}},
   {type="note",label="Stand beside your desired spot and lock it. Retries every 0.25 s; pauses away from that spot and stops requesting once joined."},
  }},
  {id="tools",kind="section",name="Session controls",rows={
   {type="actions",items={{type="button",id="assignment",label="Switch opponent",icon="refresh"},{type="button",id="report",label="Match report",icon="layers"}}},
   {type="actions",items={{type="button",id="check",label="Connection check",icon="wrench"},{type="button",id="unload",label="Unload",icon="close"}}},
  }}
 }},
 {name="Configs",icon="layers",subtitle="Saved profiles",custom="configs",controls={}},
 {name="Preferences",icon="gear",tabs={"Themes","Layout","Notifications"},subtitle="Interface and controls",system=true,bottom=true,controls={{id="interface",tab="Layout",kind="section",name="Interface",rows={PREF_SCALE,PREF_KEY,{type="checks",items={PREF_NOTICE}}}},{
				id = "appearance", tab = "Themes", icon = "sparkle", name = "Color themes", enabled = true,
				desc = "Colour, glow and ambience of the window",
				rows = {
					{ type = "swatch", id = "theme", label = "Accent colour", desc = "Re-tints the whole interface", options = THEME_NAMES, value = "Cobalt" },
					{ type = "slider", id = "opacity", label = "Window opacity", min = 70, max = 100, step = 1, value = 98, format = "%.0f", suffix = "%" },
					{ type = "checks", items = {
						{ id = "border", label = "Animated border", value = true },
						{ id = "ambient", label = "Ambient glow", value = true },
					} },
				},
			},{
				id = "layout", tab = "Layout", icon = "bounds", name = "Adaptive layout", enabled = true,
				desc = "Spacing and density of every list",
				rows = {
					{ type = "slider", id = "spacing", label = "Card spacing", desc = "Gap between cards", min = 4, max = 32, step = 1, value = 14, format = "%.0f", suffix = " px" },
					{ type = "segment", id = "density", label = "Density", desc = "Row height inside cards", options = { "Compact", "Comfortable", "Roomy" }, value = "Comfortable" },
					{ type = "checks", items = {
						{ id = "labels", label = "Section captions", value = true },
						{ id = "dividers", label = "Row dividers", value = true },
					} },
					{ type = "actions", items = { { type = "button", id = "resetLayout", label = "Reset layout", icon = "refresh" } } },
				},
			},{
				id = "motion", icon = "run", name = "Interface motion", enabled = true,
				desc = "Applies to the whole window",
				rows = {
					{ type = "slider", id = "speed", label = "Transition length", desc = "Base duration of animations", min = 0.1, max = 1, step = 0.05, value = 0.3, format = "%.2f", suffix = " s" },
					{ type = "segment", id = "curve", label = "Easing", options = { "Smooth", "Linear", "Spring" }, value = "Smooth" },
					{ type = "motionPreview", label = "Preview", desc = "Click Play or press the shortcut" },
					{ type = "key", id = "shortcut", label = "Preview shortcut", desc = "Plays the preview from anywhere", value = Enum.KeyCode.J },
					{ type = "checks", items = { { id = "reduce", label = "Reduce motion", value = false } } },
				},
			},{
				id = "typography", tab = "Layout", icon = "text", name = "Text preview", enabled = false,
				desc = "Try sizes and weights on sample copy",
				rows = {
					{ type = "slider", id = "size", label = "Text size", min = 12, max = 34, step = 1, value = 22, format = "%.0f", suffix = " px" },
					{ type = "segment", id = "weight", label = "Weight", options = { "Regular", "Medium", "Bold" }, value = "Medium" },
					{ type = "textPreview" },
				},
			},{
				id = "notifications", icon = "bell", name = "Notifications", enabled = true,
				desc = "Toasts for saves, profiles and previews",
				rows = {
					{ type = "slider", id = "duration", label = "Display duration", desc = "How long each toast stays up", min = 1, max = 10, step = 0.5, value = 4, format = "%.1f", suffix = " s" },
					{ type = "segment", id = "position", label = "Position", desc = "Corner of the screen", options = { "Left", "Center", "Right" }, value = "Right" },
					{ type = "checks", items = {
						{ id = "progress", label = "Countdown bar", value = true },
						{ id = "stack", label = "Stack multiple toasts", value = true },
					} },
					{ type = "actions", items = { { type = "button", id = "preview", label = "Preview notification", icon = "bell" } } },
				},
			}}},
}

local START_PAGE = "Shooting"

local PAGE_BY_NAME = {}
for _, page in ipairs(PAGES) do
	PAGE_BY_NAME[page.name] = page
end

function UI.forEachControl(fn, includeSystem)
	for _, page in ipairs(PAGES) do
		if includeSystem or not page.system then
			for _, ctrl in ipairs(page.controls) do
				fn(ctrl, page)
			end
		end
	end
end

-- every row/item that holds a value
function UI.forEachValue(ctrl, fn)
	for _, r in ipairs(ctrl.rows or {}) do
		if r.id and r.value ~= nil then
			fn(r)
		end
		for _, it in ipairs(r.items or {}) do
			if it.id and it.value ~= nil then
				fn(it)
			end
		end
	end
end

local CTRL, SETTING = {}, {} -- id -> control, control id -> { setting id -> setting }
UI.forEachControl(function(ctrl)
	ctrl.defaultEnabled = ctrl.enabled
	ctrl.expanded = ctrl.id == "appearance"
	CTRL[ctrl.id] = ctrl
	SETTING[ctrl.id] = {}
	UI.forEachValue(ctrl, function(s)
		s.default = s.value
		SETTING[ctrl.id][s.id] = s
	end)
end, true)

do
local forEachControl=UI.forEachControl
local forEachValue=UI.forEachValue
do
 local help={
  smartPossession={plan="Balanced considers open jumpers and driving lanes. Attack basket prioritizes advancing inside 16 studs. Find a three targets your marked radial arc distance and will not invent an unmeasured line.",steer="While H is held, normal movement follows the chosen available lane. WASD takes priority. This does not change WalkSpeed or teleport your player.",arc="Approximate radial distance from the assigned hoop to the arc. Stand on the actual curved three-point line and use Mark three-point distance here. Corner rules and the game's official shot classification are not inferred."},
  smartDribble={gap="Desired spacing before settling. Automatic shooting can require more space to satisfy its contest limit.",stamina="Pause smart dribbling and ball protection below this stamina value. Does not restore stamina.",cooldown="Minimum time between acknowledged move requests. Default 0.24 seconds; actual accepted timing depends on the game controller. Unacknowledged moves cause backoff.",shoot="Start one shot after a stable open window while H is held. Requires Auto time enabled, possession, an actual basket, settled movement and an acceptable estimated closeout.",shotContest="Maximum estimated pressure now and 0.3 seconds ahead. This estimate is not the game's actual contest formula.",shotRange="Do not start an automatic shot beyond this distance from the assigned basket. Does not determine your player's actual shooting range.",follow="Practice defender moves toward a delayed basket-side position and shades your ball hand. It cannot apply a real contest or steal.",testSpeed="Maximum local practice-defender speed. Acceleration and reaction delay keep it from instantly matching your movement."},
  ballProtection={range="React only when a known opponent is this close on the ball-hand side. Uses the same measured moves, stamina reserve and cooldown as Smart dribbler. This is an escape estimate, not immunity to steals."},
  passVision={speed="Assumed ball speed for pass ETA, not a change to actual pass speed.",release="Estimated time from receiving the pass to releasing a shot. A teammate is marked open only when a defender is estimated to arrive later, with a clear lane."},
  autoRelease={offset="Shifts Auto time's release threshold. Positive values delay release; negative values advance it. Start at 0 ms.",maxPing="Auto time waits rather than releases when the measured ping is above this limit. This does not reduce your ping.",freshness="Maximum age of the latest changing meter sample. A smaller limit rejects delayed reads more aggressively."},
  shotRules={standing="Allow Auto time to manage shots classified as stationary when the meter starts. Turning this off does not block your manual release.",moving="Allow Auto time to manage shots started while moving faster than 4 studs/sec. This is a movement classification, not detection of every fade animation.",distance="Skip Auto time when the known basket is farther away than this distance. 0 disables the limit. If basket data is unavailable, this filter cannot classify range.",hitch="Temporarily pause automatic release for 120 ms after an unusually slow rendered frame. This may miss a timing window; it cannot repair a frame stall.",hitchMs="A rendered frame longer than this duration is considered a hitch. Only used when the frame-hitch pause is enabled."},
  contestCeiling={limit="Your maximum acceptable ESTIMATED contest. It is a local geometry estimate, not the game's exact server percentage. When enabled, new shots above this estimate are blocked.",forecast="How far ahead to project observed movement when estimating a closeout. 0 uses current positions only. Longer forecasts become less reliable when players change direction.",release="Optional: release a held shot before estimated pressure crosses the ceiling. Only uses a fresh rising meter at 90–100% fill, also respecting Shot assist if enabled. Off by default; this may sacrifice perfect timing.",minimum="The earliest meter fill allowed for a contest-triggered release. 90 means the meter must have reached at least 90%; it is not a 90% make chance.",maximum="The latest meter fill allowed for a contest-triggered release. The permitted range is also intersected with Shot assist's range when enabled.",hold="How long estimated pressure must remain at or above the ceiling before a release is considered. Helps reject a brief noisy reading.",rising="Only permit a contest-triggered release while the meter is rising. Prevents this feature from reacting after the peak has passed.",blockStart="Optional: reject a new normal shot input if current estimated contest is already above the ceiling. Does not cancel a shot already started. Block/rebound jump inputs are excluded."},
  shootingInfo={preview="A line on the supported Bat meter showing Auto time's current calculated release threshold. It appears only with an active supported meter and usable timing data; it does not change the green window.",thickness="Visual thickness of the preview line. It changes no shot timing or hitbox.",confidence="Shows meter freshness and latency stability. Stable samples means the inputs look consistent, not that the shot is guaranteed to be Perfect.",history="Shows recent feedback actually received from the game. Perfect, Early, and Late describe release feedback; they do not confirm a basket was made.",rows="Number of recent feedback entries visible in the screen-edge panel. Up to 20 entries are retained for this session.",distance="Show the distance to the known basket when the meter started. This is measured in Roblox studs, not feet.",contest="Show the contest value supplied in the game's post-shot feedback. This is separate from the live estimated contest ceiling.",ping="Show the latest measured ping recorded when shot feedback arrived. This is not a server receipt timestamp.",clear="Clear only this session's displayed recent-shot list. Does not reset Auto time's saved timing profiles."},
  shotAssist={meterMin="If early-release holding is enabled, a release below this meter fill is deferred until the minimum is reached. Inside the permitted range, manual release stays immediate.",meterMax="Upper release limit. Below 100%, this can release a held shot at the selected fill. 100% uses the existing peak predictor where available. This is meter fill, not make chance.",holdEarly="If you let go too early, keep the shot held until the minimum fill. Turning this off preserves your early manual release.",protectFall="If the meter has passed its peak and is falling, request release before the predicted fill drops below your minimum."},
  autoGuard={dunks="While holding the guard key, close the gap to 1.8 studs when the handler is within 9 studs of their rim. Attempt one native jump per observed dunk if in range and grounded. Not a guaranteed block.",cutoff="Lead a ball handler driving toward the basket using observed velocity. Only acts while Auto guard is engaged; movement stays native.",screens="Route earlier around active screens and nearby opponents obstructing the guard path. Uses wider waypoints, side commitment and map raycasts.",coverage="Outline the selected opponent: green for estimated basket-side coverage, amber for partial coverage, red for poor position. Not the server contest formula.",standOff="Desired basket-side spacing from the handler, in studs.",response="Applies a group of reaction and movement settings. It does not alter server rules or guarantee defensive outcomes.",delay="Wait before reacting to a newly selected target.",activation="Hold this key to engage Auto guard.",fakeFilter="Use observed shot persistence to reduce reactions to brief fake-shot states. It is a heuristic and can react late."},
  passIntercept={automatic="Attempt normal steals on eligible nearby moving loose balls without pressing R. This is not a guaranteed interception.",move="Allow normal movement toward a short predicted flight intercept. WASD and typing override pursuit.",reach="Distance at which a normal steal request may be attempted. This does not increase the server catch radius.",jump="Allow the normal Space/Jump input for a nearby high flight. The server still determines whether a catch or block occurs.",shots="Include balls reporting MidShot, such as shots and rebounds. Off limits the selector to other moving loose balls."},
  autoSteal={reach="Distance at which to ATTEMPT a normal steal. Increasing it does not establish increased server steal reach.",delay="An opportunity must stay eligible this long before an attempt.",cooldown="Minimum spacing between steal attempts."},
  autoBlock={reactionDelay="Minimum delay from shot start. Auto Block also waits for observed release; ball lift alone cannot trigger it. Distance, facing and height still apply.",commit="Wait for a release flag or persistent shot state before attempting the block. Can avoid fakes but react later.",oneAttempt="Allow at most one block request for each observed shot."},
  rebound={range="Maximum distance for normal-movement pursuit of a nearby rebound.",reach="Distance within which a descending loose ball can trigger a jump attempt.",height="Maximum ball height above your root for a rebound jump attempt.",lead="How far ahead to project the loose ball's horizontal movement.",chase="Move toward a reachable loose ball or estimated landing position. Manual movement takes priority."},
  comboBuilder={sequence="Click + Z/X/C/V to add steps. Select a step and toggle Combine keys for grouped inputs. Double tap creates two separate requests, with a 0.10 s tap gap that you can edit using Wait after move.",preset="Load an example input sequence that you can edit. The game still decides which dribble animation each input group produces.",interval="Default wait after steps without an explicit @ delay.",repeats="Number of full sequence repetitions. Manual Z/X/C/V or Cancel stops the sequence.",sprint="Send the sprint modifier with dribble groups. Does not restore stamina.",activation="Press this key to play the enabled builder while you have possession."},
  landingTools={enabled="Estimate a moving loose ball's path and landing point. Does not model undisclosed server physics or future collisions.",trail="Short visual trail of the ball's observed recent movement, rather than a prediction.",beam="Display the estimated future flight curve.",zone="Display the estimated landing region. A larger region represents uncertainty, not a guaranteed catch area.",width="Visual width of the predicted flight beam.",scale="Visual size multiplier for the landing region.",range="Maximum nearby distance at which to track the moving ball."}
 }
 for key,value in pairs(help.smartPossession)do help.smartDribble[key]=value end
 for _,page in ipairs(PAGES)do for _,ctrl in ipairs(page.controls or {})do
  local group=help[ctrl.id]
  if group then for _,row in ipairs(ctrl.rows or {})do
   if row.id then row.help=group[row.id]end
   for _,item in ipairs(row.items or {})do item.help=group[item.id]end
  end end
 end end
end

end
function UI.pageCounts(page)
	local on, total = 0, 0
	for _, ctrl in ipairs(page.controls) do
		if ctrl.kind ~= "section" then
			total += 1
			if ctrl.enabled then
				on += 1
			end
		end
	end
	return on, total
end

function UI.featureCounts()
	local on, total = 0, 0
	for _, page in ipairs(PAGES) do
		if not page.system then
			local o, t = UI.pageCounts(page)
			on += o
			total += t
		end
	end
	return on, total
end

-- layout settings that need a re-render when they change
local DENSITY = { Compact = 0.84, Comfortable = 1, Roomy = 1.16 }
local EASINGS = { Smooth = QUINT, Linear = LINEAR, Spring = BACK }
local layoutState = { density = 1, captions = true, dividers = true }

-- forward declarations
local closePopup, openPage, renderPage, rerender, setStatus, toast
local applySettings, repaintAll, refreshMini, refreshNav, playMotionPreview, setMode
local statsListener

-------------------------------------------------------------------- root
local TITLE_H, SIDE_W, FOOT_H = 64, 220, 68
local CONTENT_X = SIDE_W + 32
-- Start at a normal fixed window size; fitting may shrink it for smaller screens.
local W,H=1000,680
local MIN_W, MIN_H = 960, 520
local PAD = 12 -- room around the window for its shadow
local winW, winH = W, H
local targetW, targetH = W, H

local previousUI = playerGui:FindFirstChild("PracticalUI")
if previousUI then previousUI:Destroy() end
local gui = UI.make("ScreenGui", {
	Name = "PracticalUI",
	ResetOnSpawn = false,
	IgnoreGuiInset = true,
	ZIndexBehavior = Enum.ZIndexBehavior.Sibling,
	DisplayOrder = 50,
})
-- public hooks: ControlChanged(ctrlId, settingId, value), TestControls()
local controlChanged = UI.make("BindableEvent", { Name = "ControlChanged", Parent = gui })
local testControls = UI.make("BindableEvent", { Name = "TestControls", Parent = gui })

local root = UI.make("Frame", {
	Name = "Root",
	BackgroundTransparency = 1,
	Size = UDim2.fromOffset(W + PAD * 2, H + PAD * 2),
	Visible = false,
	Parent = gui,
})
local fitScale = UI.make("UIScale", { Scale = DEFAULT_SCALE, Parent = root })

-- scaled on open/close and faded with groupFade
local group = UI.make("Frame", {
	BackgroundTransparency = 1,
	AnchorPoint = Vector2.new(0.5, 0.5),
	Position = UDim2.fromScale(0.5, 0.5),
	Size = UDim2.fromScale(1, 1),
	Parent = root,
})
local popScale = UI.make("UIScale", { Scale = 0.94, Parent = group })

local shadowHost = UI.make("Frame", {
	Name = "Shadow",
	BackgroundTransparency = 1,
	Position = UDim2.fromOffset(PAD, PAD),
	Size = UDim2.fromOffset(W, H),
	Parent = group,
})
UI.softShadow(shadowHost, 16, 7, 4, 0.055, 10)

local winStroke = UI.stroke(C.stroke, 1.2)
local borderGrad = UI.bindTheme(UI.make("UIGradient", { Enabled = false, Parent = winStroke }), "Color", "border")

local win = UI.make("Frame", {
	Name = "Window",
	BackgroundColor3 = C.bg,
	BorderSizePixel = 0,
	ClipsDescendants = true,
	Position = UDim2.fromOffset(PAD, PAD),
	Size = UDim2.fromOffset(W, H),
	ZIndex = 2,
	Parent = group,
}, {
	UI.corner(16),
	winStroke,
	UI.make("UIGradient", { Rotation = 90, Color = ColorSequence.new(C.white, Color3.fromRGB(205, 210, 224)) }),
})

-- Ambient glow: slow-drifting accent light behind everything. A CanvasGroup so the
-- rounded window corners clip it properly.
local ambient = UI.make("CanvasGroup", {
	Name = "Ambient",
	BackgroundTransparency = 1,
	Size = UDim2.fromScale(1, 1),
	ZIndex = 0,
	Parent = win,
}, { UI.corner(16) })
local ambientOn = true
local blobs = {}
for i, spec in ipairs({
	{ key = "a", size = 620, x = 0.82, y = 0.08 },
	{ key = "b", size = 520, x = 0.42, y = 1.0 },
	{ key = "a", size = 420, x = 0.06, y = 0.45 },
}) do
	local blob = UI.make("Frame", {
		BackgroundTransparency = 1,
		AnchorPoint = Vector2.new(0.5, 0.5),
		Position = UDim2.fromScale(spec.x, spec.y),
		Size = UDim2.fromOffset(spec.size, spec.size),
		Parent = ambient,
	})
	-- concentric discs stack into a soft radial falloff
	for r = 1, 6 do
		local k = r / 6
		UI.bindTheme(UI.make("Frame", {
			BackgroundTransparency = 0.978,
			BorderSizePixel = 0,
			AnchorPoint = Vector2.new(0.5, 0.5),
			Position = UDim2.fromScale(0.5, 0.5),
			Size = UDim2.fromScale(k, k),
			Parent = blob,
		}, { UI.round() }), "BackgroundColor3", spec.key)
	end
	blobs[i] = { frame = blob, x = spec.x, y = spec.y, phase = i * 2.1 }
end

function UI.applySize()
	win.Size = UDim2.fromOffset(winW, winH)
	shadowHost.Size = win.Size
	root.Size = UDim2.fromOffset(winW + PAD * 2, winH + PAD * 2)
end

-- a GuiObject's on-screen box in window design units (for popups / tooltips)
function UI.localBox(obj)
	local s = win.AbsoluteSize.X / winW
	local d = obj.AbsolutePosition - win.AbsolutePosition
	return d.X / s, d.Y / s, obj.AbsoluteSize.X / s, obj.AbsoluteSize.Y / s
end

-------------------------------------------------------------------- popups + tooltips
local overlay = UI.make("TextButton", {
	Text = "",
	AutoButtonColor = false,
	BackgroundTransparency = 1,
	Size = UDim2.fromScale(1, 1),
	Visible = false,
	ZIndex = 60,
	Parent = win,
})
local activePopup
closePopup = function()
	if activePopup then
		local close = activePopup
		activePopup = nil
		overlay.Visible = false
		close()
	end
end
overlay.MouseButton1Click:Connect(function()
	closePopup()
end)

-- Hover tooltip above (or below, near the top edge) `target`.
function UI.tooltip(target, str)
	if not target:IsA("GuiButton") then
		return
	end
	local tip
	local token = 0
	target.MouseEnter:Connect(function()
		token += 1
		if tip then
			tip:Destroy()
			tip = nil
		end
		local bx, by, bw, bh = UI.localBox(target)
		local tw = math.min(360, UI.textWidth(str, 15) + 28)
		local th = 32
		local tx = math.clamp(bx + bw / 2 - tw / 2, 8, math.max(8, winW - tw - 8))
		local below = by - th - 12 < 8
		local ty = below and by + bh + 10 or by - th - 10
		tip = UI.make("Frame", {
			BackgroundTransparency = 1,
			Position = UDim2.fromOffset(tx, ty + (below and -5 or 5)),
			Size = UDim2.fromOffset(tw, th),
			ZIndex = 55,
			Parent = win,
		})
		local body = UI.make("Frame", {
			BackgroundColor3 = C.raised,
			Size = UDim2.fromScale(1, 1),
			Parent = tip,
		}, { UI.corner(8), UI.stroke(C.strokeHi, 1) })
		UI.make("TextLabel", {
			BackgroundTransparency = 1,
			Size = UDim2.fromScale(1, 1),
			Text = str,
			FontFace = UI.font(MEDIUM),
			TextSize = 15,
			TextColor3 = C.text,
			Parent = body,
		})
		UI.groupFade(tip, 0, 0.2, 0.35)
		UI.tween(tip, 0.3, { Position = UDim2.fromOffset(tx, ty) }, QUINT, OUT, 0.35)
	end)
	local function hide()
		token += 1
		if tip then
			local old = tip
			tip = nil
			UI.groupFade(old, 1, 0.12).Completed:Connect(function()
				old:Destroy()
			end)
		end
	end
	target.Destroying:Connect(function()
		if tip then
			tip:Destroy()
			tip = nil
		end
	end)
	target.MouseLeave:Connect(hide)
	target.MouseButton1Down:Connect(hide)
end

-- Select box. `pos` = left edge / vertical centre. `options` is a list or a function returning one.
-- `extra` = optional { label, icon, onClick } shown under a divider at the bottom of the menu.
local function dropdown(parent, pos, w, h, options, value, onChange, extra)
	local st = UI.stroke(C.stroke, 1)
	local btn = UI.make("TextButton", {
		Text = "",
		AutoButtonColor = false,
		BackgroundColor3 = C.raised,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = pos,
		Size = UDim2.fromOffset(w, h),
		Parent = parent,
	}, { UI.corner(10), st })
	UI.bindTheme(UI.make("Frame", {
		BorderSizePixel = 0,
		AnchorPoint = Vector2.new(0.5, 0.5),
		Position = UDim2.fromOffset(20, h / 2),
		Size = UDim2.fromOffset(8, 8),
		Parent = btn,
	}, { UI.round() }), "BackgroundColor3", "a")
	local label = UI.clip(text(btn, value, 34, h / 2, 17, { weight = MEDIUM }), w - 70)
	local chev = UI.make("Frame", {
		BackgroundTransparency = 1,
		AnchorPoint = Vector2.new(0.5, 0.5),
		Position = UDim2.fromOffset(w - 22, h / 2),
		Size = UDim2.fromOffset(16, 16),
		Parent = btn,
	})
	UI.spinIcon(chev, "chevronRight", C.sub, 2, 90, 0)
	UI.pressable(btn, 0.98)

	local isOpen = false
	local current = value
	btn.MouseEnter:Connect(function()
		if not isOpen then
			UI.tween(st, 0.15, { Color = C.strokeHi })
			UI.tween(btn, 0.15, { BackgroundColor3 = C.hover })
		end
	end)
	btn.MouseLeave:Connect(function()
		if not isOpen then
			UI.tween(st, 0.25, { Color = C.stroke })
			UI.tween(btn, 0.25, { BackgroundColor3 = C.raised })
		end
	end)

	local function set(v, t)
		current = v
		label.Text = v
		if t and t > 0 then
			label.TextTransparency = 1
			UI.tween(label, t, { TextTransparency = 0 })
		end
	end

	btn.MouseButton1Click:Connect(function()
		if isOpen then
			closePopup()
			return
		end
		closePopup()
		isOpen = true
		local opts = typeof(options) == "function" and options() or options
		local bx, by, bw, bh = UI.localBox(btn)
		local itemH = 40
		local menuH = #opts * itemH + 10 + (extra and itemH + 9 or 0)
		local mx, my = bx, by + bh + 6
		if my + menuH > winH - 10 then
			my = by - menuH - 6 -- open upward near the bottom edge
		end
		local M = 12 -- margin so the shadow survives the fade's CanvasGroup
		local menu = UI.make("Frame", {
			BackgroundTransparency = 1,
			Position = UDim2.fromOffset(mx - M, my - M - 8),
			Size = UDim2.fromOffset(bw + M * 2, menuH + M * 2),
			ZIndex = 61,
			Parent = win,
		})
		local shade = UI.make("Frame", {
			BackgroundTransparency = 1,
			Position = UDim2.fromOffset(M, M),
			Size = UDim2.fromOffset(bw, menuH),
			Parent = menu,
		})
		UI.softShadow(shade, 12, 4, 3, 0.07, 5)
		local body = UI.make("Frame", {
			BackgroundColor3 = C.raised,
			Position = UDim2.fromOffset(M, M),
			Size = UDim2.fromOffset(bw, menuH),
			ZIndex = 2,
			Parent = menu,
		}, { UI.corner(12), UI.stroke(C.strokeHi, 1) })

		local function item(i, labelText, isSelected, iconName, onClick)
			local y0 = 5 + (i - 1) * itemH + (i > #opts and 9 or 0)
			local it = UI.make("TextButton", {
				Text = "",
				AutoButtonColor = false,
				BackgroundColor3 = isSelected and A.dim or C.hover,
				BackgroundTransparency = isSelected and 0 or 1,
				Position = UDim2.fromOffset(5, y0),
				Size = UDim2.fromOffset(bw - 10, itemH - 2),
				Parent = body,
			}, { UI.corner(8) })
			local l = UI.clip(text(it, labelText, iconName and not isSelected and 42 or 14, (itemH - 2) / 2, 16, {
				color = isSelected and C.white or C.text2,
				weight = isSelected and SEMIBOLD or nil,
			}), bw - 70)
			if isSelected then
				UI.icon(it, "check", UDim2.new(1, -20, 0.5, 0), nil, 16, A.hi, 2.2)
			elseif iconName then
				UI.icon(it, iconName, 24, (itemH - 2) / 2, 16, C.sub, 2)
			end
			if not isSelected then
				it.MouseEnter:Connect(function()
					UI.tween(it, 0.12, { BackgroundTransparency = 0 })
					UI.tween(l, 0.12, { TextColor3 = C.white })
				end)
				it.MouseLeave:Connect(function()
					UI.tween(it, 0.2, { BackgroundTransparency = 1 })
					UI.tween(l, 0.2, { TextColor3 = C.text2 })
				end)
			end
			it.MouseButton1Click:Connect(onClick)
		end
		for i, opt in ipairs(opts) do
			item(i, opt, opt == current, nil, function()
				closePopup()
				if opt ~= current then
					set(opt, 0.25)
					onChange(opt)
				end
			end)
		end
		if extra then
			UI.strip(body, UDim2.fromOffset(12, 5 + #opts * itemH + 4), UDim2.new(1, -24, 0, 1), C.line)
			item(#opts + 1, extra.label, false, extra.icon, function()
				closePopup()
				extra.onClick()
			end)
		end

		overlay.Visible = true
		UI.groupFade(menu, 0, 0.25)
		UI.tween(menu, 0.32, { Position = UDim2.fromOffset(mx - M, my - M) })
		UI.spinIcon(chev, "chevronRight", C.sub, 2, -90, 0.3)
		UI.tween(st, 0.2, { Color = A.a })
		activePopup = function()
			isOpen = false
			UI.spinIcon(chev, "chevronRight", C.sub, 2, 90, 0.3)
			UI.tween(st, 0.25, { Color = C.stroke })
			UI.tween(btn, 0.25, { BackgroundColor3 = C.raised })
			UI.tween(menu, 0.15, { Position = UDim2.fromOffset(mx - M, my - M - 4) })
			UI.groupFade(menu, 1, 0.15).Completed:Connect(function()
				menu:Destroy()
			end)
		end
	end)

	return { button = btn, set = set }
end

-------------------------------------------------------------------- buttons
-- Icon-only button. `pos` is its centre.
function UI.iconButton(parent, pos, size, iconName, opts)
	opts = opts or {}
	local st = opts.solid and UI.stroke(C.stroke, 1) or nil
	local b = UI.make("TextButton", {
		Text = "",
		AutoButtonColor = false,
		BackgroundColor3 = opts.solid and C.raised or (opts.hoverColor or C.hover),
		BackgroundTransparency = opts.solid and 0 or 1,
		AnchorPoint = Vector2.new(0.5, 0.5),
		Position = pos,
		Size = UDim2.fromOffset(size, size),
		ZIndex = opts.z or 1,
		Parent = parent,
	}, { UI.corner(opts.radius or 10), st })
	local idle = opts.color or C.text2
	local ic = UI.icon(b, iconName, size / 2, size / 2, opts.iconSize or 18, idle, opts.thickness or 1.8)
	b.MouseEnter:Connect(function()
		if opts.solid then
			UI.tween(b, 0.15, { BackgroundColor3 = opts.hoverColor or C.hover })
			UI.tween(st, 0.15, { Color = opts.hoverColor and opts.hoverColor or C.strokeHi })
		else
			UI.tween(b, 0.15, { BackgroundTransparency = 0 })
		end
		UI.tintIcon(ic, opts.hot or C.white, 0.15)
	end)
	b.MouseLeave:Connect(function()
		if opts.solid then
			UI.tween(b, 0.25, { BackgroundColor3 = C.raised })
			UI.tween(st, 0.25, { Color = C.stroke })
		else
			UI.tween(b, 0.25, { BackgroundTransparency = 1 })
		end
		UI.tintIcon(ic, idle, 0.25)
	end)
	UI.pressable(b, 0.88)
	if opts.tip then
		UI.tooltip(b, opts.tip)
	end
	return b, ic
end

-- Quiet outlined button with an optional leading icon. Width fits the label.
function UI.ghostButton(parent, label, iconName, h)
	h = h or 40
	local w = math.round((iconName and 42 or 18) + UI.textWidth(label, 16) + 18)
	local st = UI.stroke(C.stroke, 1)
	local b = UI.make("TextButton", {
		Text = "",
		AutoButtonColor = false,
		BackgroundColor3 = C.raised,
		Size = UDim2.fromOffset(w, h),
		Parent = parent,
	}, { UI.corner(10), st })
	local ic = iconName and UI.icon(b, iconName, 23, h / 2, 17, C.text2, 1.8) or nil
	if ic then
		ic.ZIndex = 2
	end
	local l = text(b, label, iconName and 42 or 18, h / 2, 16, { weight = MEDIUM, color = C.text2, z = 2 })
	b.MouseEnter:Connect(function()
		UI.tween(b, 0.15, { BackgroundColor3 = C.hover })
		UI.tween(st, 0.15, { Color = C.strokeHi })
		UI.tween(l, 0.15, { TextColor3 = C.white })
		if ic then
			UI.tintIcon(ic, C.white, 0.15)
		end
	end)
	b.MouseLeave:Connect(function()
		UI.tween(b, 0.25, { BackgroundColor3 = C.raised })
		UI.tween(st, 0.25, { Color = C.stroke })
		UI.tween(l, 0.25, { TextColor3 = C.text2 })
		if ic then
			UI.tintIcon(ic, C.text2, 0.25)
		end
	end)
	UI.pressable(b)
	UI.ripple(b)
	return b, l, ic
end

-- Gradient call-to-action with a hover sheen and a glow ring.
function UI.primaryButton(parent, label, iconName, h)
	h = h or 42
	local w = math.round((iconName and 44 or 22) + UI.textWidth(label, 17) + 22)
	local glow = UI.bindTheme(UI.stroke(A.hi, 2, 1), "Color", "hi")
	local idle = Color3.fromRGB(226, 229, 240)
	local b = UI.make("TextButton", {
		Text = "",
		AutoButtonColor = false,
		BackgroundColor3 = idle,
		Size = UDim2.fromOffset(w, h),
		Parent = parent,
	}, { UI.corner(10), glow, UI.accentGradient(20) })
	local ic = iconName and UI.icon(b, iconName, 24, h / 2, 18, C.white, 1.9) or nil
	if ic then
		ic.ZIndex = 2
	end
	local l = text(b, label, iconName and 44 or 22, h / 2, 17, { weight = SEMIBOLD, color = C.white, z = 2 })
	b.MouseEnter:Connect(function()
		UI.tween(b, 0.2, { BackgroundColor3 = C.white })
		UI.tween(glow, 0.25, { Transparency = 0.45 })
	end)
	b.MouseLeave:Connect(function()
		UI.tween(b, 0.3, { BackgroundColor3 = idle })
		UI.tween(glow, 0.3, { Transparency = 1 })
	end)
	UI.shimmer(b)
	UI.ripple(b)
	UI.pressable(b)
	return b, l, ic
end

-- Rounded square with the brand gradient and logo glyph.
function UI.logoMark(parent, x, y, size)
	local r = math.floor(size * 0.3)
	UI.bindTheme(UI.make("Frame", {
		BackgroundTransparency = 0.9,
		BorderSizePixel = 0,
		AnchorPoint = Vector2.new(0.5, 0.5),
		Position = UDim2.fromOffset(x, y),
		Size = UDim2.fromOffset(size + 16, size + 16),
		Parent = parent,
	}, { UI.corner(r + 8) }), "BackgroundColor3", "a")
	local mark = UI.make("Frame", {
		BackgroundColor3 = C.white,
		AnchorPoint = Vector2.new(0.5, 0.5),
		Position = UDim2.fromOffset(x, y),
		Size = UDim2.fromOffset(size, size),
		Parent = parent,
	}, { UI.corner(r), UI.accentGradient(45) })
	UI.icon(mark, "logo", size / 2, size / 2, math.floor(size * 0.56), C.white, 1.8)
	return mark
end

-------------------------------------------------------------------- title bar
local titleBar = UI.make("Frame", {
	BackgroundTransparency = 1,
	Size = UDim2.new(1, 0, 0, TITLE_H),
	Active = true,
	Parent = win,
})
UI.strip(win, UDim2.fromOffset(0, TITLE_H), UDim2.new(1, 0, 0, 1), C.line)

UI.logoMark(titleBar, 38, TITLE_H / 2, 34)
text(titleBar, "practical", 68, TITLE_H / 2, 21, { weight = SEMIBOLD })
do
	local x = 68 + UI.textWidth("practical", 21) + 12
	local chip = UI.make("Frame", {
		AnchorPoint = Vector2.new(0, 0.5),
		Position = UDim2.fromOffset(x, TITLE_H / 2 + 1),
		Size = UDim2.fromOffset(UI.textWidth(VERSION, 13) + 16, 20),
		Parent = titleBar,
	}, { UI.corner(6) })
	UI.bindTheme(chip, "BackgroundColor3", "dim")
	UI.bindTheme(UI.stroke(A.soft, 1), "Color", "soft").Parent = chip
	UI.bindTheme(UI.make("TextLabel", {
		BackgroundTransparency = 1,
		Size = UDim2.fromScale(1, 1),
		Text = VERSION,
		FontFace = UI.font(SEMIBOLD),
		TextSize = 13,
		Parent = chip,
	}), "TextColor3", "hi")
end

-- search: lives in the title bar, centred over the content column
local searchStroke = UI.stroke(C.line, 1)
local search = UI.make("Frame", {
	BackgroundColor3 = C.field,
	BackgroundTransparency = 0.25,
	AnchorPoint = Vector2.new(0.5, 0.5),
	Position = UDim2.new(0.5, SIDE_W / 2 - 60, 0, TITLE_H / 2),
	Size = UDim2.fromOffset(420, 40),
	ZIndex = 3,
	Parent = win,
}, { UI.corner(12), searchStroke })
local searchIcon = UI.icon(search, "search", 22, 20, 17, C.sub, 1.9)
local searchBox = UI.make("TextBox", {
	BackgroundTransparency = 1,
	Position = UDim2.fromOffset(42, 0),
	Size = UDim2.new(1, -124, 1, 0),
	Text = "",
	PlaceholderText = "Search settings",
	PlaceholderColor3 = C.muted,
	TextColor3 = C.text,
	FontFace = UI.font(),
	TextSize = 16,
	TextXAlignment = Enum.TextXAlignment.Left,
	TextTruncate = Enum.TextTruncate.AtEnd,
	ClearTextOnFocus = false,
	Parent = search,
})
local kbd = UI.make("Frame", {
	BackgroundColor3 = C.raised,
	AnchorPoint = Vector2.new(1, 0.5),
	Position = UDim2.new(1, -10, 0.5, 0),
	Size = UDim2.fromOffset(58, 24),
	Parent = search,
}, { UI.corner(6), UI.stroke(C.stroke, 1) })
UI.make("TextLabel", {
	BackgroundTransparency = 1,
	Size = UDim2.fromScale(1, 1),
	Text = "Ctrl K",
	FontFace = UI.font(SEMIBOLD),
	TextSize = 13,
	TextColor3 = C.sub,
	Parent = kbd,
})
local clearSearch = UI.iconButton(search, UDim2.new(1, -24, 0.5, 0), 28, "close", { iconSize = 13, radius = 8 })
clearSearch.Visible = false
clearSearch.MouseButton1Click:Connect(function()
	searchBox.Text = ""
end)
searchBox.Focused:Connect(function()
	UI.tween(searchStroke, 0.2, { Color = A.a })
	UI.tween(search, 0.2, { BackgroundTransparency = 0 })
	UI.tintIcon(searchIcon, A.hi, 0.2)
end)
searchBox.FocusLost:Connect(function()
	UI.tween(searchStroke, 0.3, { Color = C.line })
	UI.tween(search, 0.3, { BackgroundTransparency = 0.25 })
	UI.tintIcon(searchIcon, C.sub, 0.3)
end)

local clock = text(win, os.date("%H:%M"), UDim2.new(1, -112, 0, TITLE_H / 2), 0, 15, { color = C.sub, ax = 1, weight = MEDIUM })
local minBtn = UI.iconButton(win, UDim2.new(1, -78, 0, TITLE_H / 2), 36, "minus", { tip = "Minimize" })
local closeBtn = UI.iconButton(win, UDim2.new(1, -36, 0, TITLE_H / 2), 36, "close", { hoverColor = C.dangerFill, tip = "Close" })
task.spawn(function()
	while alive do
		clock.Text = os.date("%H:%M")
		task.wait(10)
	end
end)

-------------------------------------------------------------------- sidebar
local rail = UI.make("Frame", {
	Name = "Sidebar",
	BackgroundColor3 = C.rail,
	BorderSizePixel = 0,
	Position = UDim2.fromOffset(0, TITLE_H + 1),
	Size = UDim2.new(0, SIDE_W, 1, -TITLE_H - 1),
	Parent = win,
}, { UI.corner(16) })
-- square off every corner except bottom-left, which follows the window's rounding
UI.strip(rail, UDim2.fromOffset(0, 0), UDim2.new(1, 0, 0, 20), C.rail)
UI.strip(rail, UDim2.new(1, -20, 0, 0), UDim2.new(0, 20, 1, 0), C.rail)
local railFillers = {}
for _, child in ipairs(rail:GetChildren()) do
	if child:IsA("Frame") then
		child.ZIndex = 0 -- under the nav highlight
		table.insert(railFillers, child)
	end
end
UI.strip(win, UDim2.fromOffset(SIDE_W, TITLE_H + 1), UDim2.new(0, 1, 1, -TITLE_H - 1), C.line)

local NAV_X, NAV_W, NAV_H, NAV_STEP = 14, SIDE_W - 28, 46, 50
UI.tracked(rail, "MENU", 26, 30, 12, C.muted, 1.8)

local navHighlight = UI.make("Frame", {
	BackgroundColor3 = C.white,
	BackgroundTransparency = 0.84,
	BorderSizePixel = 0,
	Position = UDim2.fromOffset(NAV_X, 52),
	Size = UDim2.fromOffset(NAV_W, NAV_H),
	Parent = rail,
}, {
	UI.corner(10),
	UI.accentGradient(0, NumberSequence.new(0, 0.8)),
	UI.bindTheme(UI.stroke(A.a, 1, 0.65), "Color", "a"),
})
UI.bindTheme(UI.make("Frame", {
	BorderSizePixel = 0,
	AnchorPoint = Vector2.new(0, 0.5),
	Position = UDim2.new(0, 0, 0.5, 0),
	Size = UDim2.fromOffset(3, 20),
	Parent = navHighlight,
}, { UI.round() }), "BackgroundColor3", "hi")

UI.navScroll=UI.make("ScrollingFrame",{
 Name="NavigationScroll",BackgroundTransparency=1,BorderSizePixel=0,
 Position=UDim2.fromOffset(0,46),Size=UDim2.new(1,0,1,-222),
 CanvasSize=UDim2.fromOffset(0,0),ScrollBarThickness=3,ScrollBarImageColor3=C.sub,
 Parent=rail,
})
local nav = {} -- page name -> { btn, icon, label, badge, badgeText, pos, page }
UI.navScroll.CanvasSize=UDim2.fromOffset(0,4+(#PAGES-1)*NAV_STEP)
local currentPage
local renderedQuery = ""

do
	local y = 4
	for _, page in ipairs(PAGES) do
		local pos
		if page.bottom then
			pos = UDim2.new(0, NAV_X, 1, -94 - NAV_H)
			UI.tracked(rail, "SYSTEM", UDim2.new(0, 26, 1, -94 - NAV_H - 18), nil, 12, C.muted, 1.8)
		else
			pos = UDim2.fromOffset(NAV_X, y)
			y += NAV_STEP
		end
		local name = page.name
		local btn = UI.make("TextButton", {
			Text = "",
			AutoButtonColor = false,
			BackgroundColor3 = C.hover,
			BackgroundTransparency = 1,
			Position = pos,
			Size = UDim2.fromOffset(NAV_W, NAV_H),
			ZIndex = 2,
			Parent = page.bottom and rail or UI.navScroll,
		}, { UI.corner(10) })
		local ic = UI.icon(btn, page.icon, 26, NAV_H / 2, 19, C.sub, 1.7)
		local label = text(btn, name, 50, NAV_H / 2, 17, { color = C.text2, weight = MEDIUM })
		local badge = UI.make("Frame", {
			BackgroundColor3 = C.field,
			AnchorPoint = Vector2.new(1, 0.5),
			Position = UDim2.new(1, -12, 0.5, 0),
			Size = UDim2.fromOffset(24, 20),
			Visible = false,
			Parent = btn,
		}, { UI.round() })
		local badgeText = UI.make("TextLabel", {
			BackgroundTransparency = 1,
			Size = UDim2.fromScale(1, 1),
			Text = "0",
			FontFace = UI.font(SEMIBOLD),
			TextSize = 13,
			TextColor3 = C.sub,
			Parent = badge,
		})
		nav[name] = { btn = btn, icon = ic, label = label, badge = badge, badgeText = badgeText, pos = pos, page = page }
		btn.MouseEnter:Connect(function()
			if currentPage ~= name then
				UI.tween(btn, 0.15, { BackgroundTransparency = 0.4 })
				UI.tween(label, 0.15, { TextColor3 = C.white })
			end
		end)
		btn.MouseLeave:Connect(function()
			UI.tween(btn, 0.25, { BackgroundTransparency = 1 })
			if currentPage ~= name then
				UI.tween(label, 0.25, { TextColor3 = C.text2 })
			end
		end)
		UI.pressable(btn, 0.97)
		btn.MouseButton1Click:Connect(function()
			openPage(name)
		end)
	end
end

refreshNav = function(t)
	t = t or 0.25
	for name, it in pairs(nav) do
		local sel = name == currentPage
		UI.tween(it.label, t, { TextColor3 = sel and C.white or C.text2 })
		UI.tintIcon(it.icon, sel and C.white or C.sub, t)
		local on, total = UI.pageCounts(it.page)
		it.badge.Visible = total > 0 and on > 0
		it.badgeText.Text = tostring(on)
		UI.tween(it.badge, t, { BackgroundColor3 = sel and A.dim or C.field })
		UI.tween(it.badgeText, t, { TextColor3 = sel and A.hi or C.sub })
	end
end

-- who's here
do
	local card = UI.make("TextButton", {
		Text = "",
		AutoButtonColor = false,
		BackgroundColor3 = C.surface,
		Position = UDim2.new(0, NAV_X, 1, -80),
		Size = UDim2.fromOffset(NAV_W, 64),
		Parent = rail,
	}, { UI.corner(12), UI.stroke(C.line, 1) })
	local avatarRing = UI.bindTheme(UI.stroke(A.a, 2, 0.2), "Color", "a")
	local avatar = UI.make("ImageLabel", {
		BackgroundColor3 = C.field,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = UDim2.new(0, 12, 0.5, 0),
		Size = UDim2.fromOffset(40, 40),
		Image = "",
		Parent = card,
	}, { UI.round(), avatarRing })
	-- initial letter until (unless) the headshot loads
	local initial = UI.make("TextLabel", {
		BackgroundTransparency = 1,
		Size = UDim2.fromScale(1, 1),
		Text = string.upper(string.sub(player.DisplayName, 1, 1)),
		FontFace = UI.font(BOLD),
		TextSize = 18,
		TextColor3 = C.text2,
		Parent = avatar,
	})
	local online = UI.make("Frame", {
		BackgroundColor3 = C.green,
		AnchorPoint = Vector2.new(0.5, 0.5),
		Position = UDim2.new(1, -4, 1, -4),
		Size = UDim2.fromOffset(11, 11),
		ZIndex = 2,
		Parent = avatar,
	}, { UI.round(), UI.stroke(C.surface, 2) })
	online.ZIndex = 3
	UI.clip(text(card, player.DisplayName, 64, 24, 16, { weight = SEMIBOLD }), NAV_W - 80)
	UI.clip(text(card, "@" .. player.Name, 64, 43, 13, { color = C.sub }), NAV_W - 80)
	card.MouseEnter:Connect(function()
		UI.tween(card, 0.15, { BackgroundColor3 = C.raised })
	end)
	card.MouseLeave:Connect(function()
		UI.tween(card, 0.25, { BackgroundColor3 = C.surface })
	end)
	UI.pressable(card, 0.98)
	card.MouseButton1Click:Connect(function()
		openPage("Preferences")
	end)
	task.spawn(function()
		local ok, img = pcall(function()
			return Players:GetUserThumbnailAsync(player.UserId, Enum.ThumbnailType.HeadShot, Enum.ThumbnailSize.Size100x100)
		end)
		if ok and alive and img then
			avatar.Image = img
			initial.Visible = false
		end
	end)
end

-------------------------------------------------------------------- page header + tabs
local LIST_TOP, LIST_TOP_TABS = TITLE_H + 138, TITLE_H + 176

local crumb = text(win, "", CONTENT_X, TITLE_H + 32, 14, { color = C.sub, rich = true, weight = MEDIUM })
local pageTitle = text(win, "", CONTENT_X, TITLE_H + 64, 30, { weight = BOLD })
local pageSub = text(win, "", CONTENT_X, TITLE_H + 96, 16, { color = C.sub })

text(win, "PROFILE", UDim2.new(1, -32 - 230, 0, TITLE_H + 32), 0, 12, { color = C.muted, weight = SEMIBOLD })

local tabBar = UI.make("Frame", {
	BackgroundTransparency = 1,
	Position = UDim2.fromOffset(CONTENT_X, TITLE_H + 120),
	Size = UDim2.new(1, -CONTENT_X - 32, 0, 42),
	Visible = false,
	Parent = win,
})
local tabUnderline = UI.make("Frame", {
	BackgroundColor3 = C.white,
	BorderSizePixel = 0,
	AnchorPoint = Vector2.new(0, 1),
	Position = UDim2.new(0, 0, 1, 0),
	Size = UDim2.fromOffset(40, 2),
	ZIndex = 2,
	Parent = tabBar,
}, { UI.round(), UI.accentGradient(0) })
local headerLine = UI.strip(win, UDim2.fromOffset(CONTENT_X, TITLE_H + 124), UDim2.new(1, -CONTENT_X - 32, 0, 1), C.line)

local tabButtons = {}
local tabsFor -- page whose tabs are built
local currentTab = {} -- page name -> selected tab

function UI.riseIn(label, delay)
	local p = label.Position
	label.TextTransparency = 1
	label.Position = p + UDim2.fromOffset(0, 10)
	UI.tween(label, 0.5, { TextTransparency = 0, Position = p }, QUINT, OUT, delay)
end

function UI.paintTabs(t)
	local page = PAGE_BY_NAME[tabsFor]
	if not page then
		return
	end
	local sel = currentTab[page.name]
	for name, b in pairs(tabButtons) do
		local on = name == sel
		UI.tween(b, t, { TextColor3 = on and C.white or C.sub })
		if on then
			UI.tween(tabUnderline, t, {
				Position = UDim2.new(0, b.Position.X.Offset, 1, 0),
				Size = UDim2.fromOffset(b.Size.X.Offset, 2),
			})
		end
	end
end

function UI.buildTabs(page)
	for _, b in pairs(tabButtons) do
		b:Destroy()
	end
	table.clear(tabButtons)
	tabsFor = page and page.tabs and page.name or nil
	tabBar.Visible = tabsFor ~= nil
	headerLine.Position = UDim2.fromOffset(CONTENT_X, TITLE_H + (tabsFor and 162 or 124))
	if not tabsFor then
		return
	end
	currentTab[page.name] = currentTab[page.name] or page.tabs[1]
	local x = 0
	for _, name in ipairs(page.tabs) do
		local w = math.ceil(UI.textWidth(name, 16))
		local b = UI.make("TextButton", {
			AutoButtonColor = false,
			BackgroundTransparency = 1,
			Text = name,
			FontFace = UI.font(SEMIBOLD),
			TextSize = 16,
			TextColor3 = C.sub,
			Position = UDim2.fromOffset(x, 0),
			Size = UDim2.fromOffset(w, 40),
			Parent = tabBar,
		})
		tabButtons[name] = b
		b.MouseEnter:Connect(function()
			if currentTab[page.name] ~= name then
				UI.tween(b, 0.15, { TextColor3 = C.text2 })
			end
		end)
		b.MouseLeave:Connect(function()
			if currentTab[page.name] ~= name then
				UI.tween(b, 0.2, { TextColor3 = C.sub })
			end
		end)
		b.MouseButton1Click:Connect(function()
			if currentTab[page.name] ~= name or renderedQuery ~= "" then
				currentTab[page.name] = name
				UI.paintTabs(0.35)
				openPage(page.name, true)
			end
		end)
		x += w + 30
	end
	UI.paintTabs(0)
end

-------------------------------------------------------------------- list
local list = UI.make("ScrollingFrame", {
	BackgroundTransparency = 1,
	BorderSizePixel = 0,
	Position = UDim2.fromOffset(CONTENT_X - 6, LIST_TOP),
	Size = UDim2.new(1, -(CONTENT_X - 6) - 18, 1, -LIST_TOP - FOOT_H - 4),
	CanvasSize = UDim2.new(),
	AutomaticCanvasSize = Enum.AutomaticSize.Y,
	ScrollBarThickness = 4,
	ScrollBarImageColor3 = C.strokeHi,
	ScrollingDirection = Enum.ScrollingDirection.Y,
	Parent = win,
}, { UI.padding(4, 14, 4, 18) })
local listLayout = UI.make("UIListLayout", { SortOrder = Enum.SortOrder.LayoutOrder, Padding = UDim.new(0, 14), Parent = list })
list:GetPropertyChangedSignal("CanvasPosition"):Connect(function()
	closePopup()
end)

function UI.setListTop(top, t)
	UI.tween(list, t, {
		Position = UDim2.fromOffset(CONTENT_X - 6, top),
		Size = UDim2.new(1, -(CONTENT_X - 6) - 18, 1, -top - FOOT_H - 4),
	})
end

local emptyLabel = text(win, "", CONTENT_X, LIST_TOP + 34, 17, { color = C.sub })
emptyLabel.Visible = false

-- every list entry is a slot (sized by the list) holding one "Body" frame (animated)
function UI.newSlot(h, order, name)
	local slot = UI.make("Frame", {
		Name = name or ("slot" .. order),
		BackgroundTransparency = 1,
		Size = UDim2.new(1, 0, 0, h + 4),
		LayoutOrder = order,
		Parent = list,
	})
	local body = UI.make("Frame", {
		Name = "Body",
		BackgroundTransparency = 1,
		Size = UDim2.fromScale(1, 1),
		Parent = slot,
	})
	return slot, body
end

-------------------------------------------------------------------- footer
UI.strip(win, UDim2.new(0, SIDE_W + 1, 1, -FOOT_H), UDim2.new(1, -SIDE_W - 1, 0, 1), C.line)

local statusDot = UI.make("Frame", {
	BackgroundColor3 = C.green,
	BorderSizePixel = 0,
	AnchorPoint = Vector2.new(0.5, 0.5),
	Position = UDim2.new(0, CONTENT_X + 5, 1, -FOOT_H / 2),
	Size = UDim2.fromOffset(9, 9),
	Parent = win,
}, { UI.round() })
local statusHalo = UI.pulse(statusDot, C.green)

local saveBtn, saveLabel = UI.primaryButton(win, "Save profile", "save", 42)
saveBtn.AnchorPoint = Vector2.new(1, 0.5)
saveBtn.Position = UDim2.new(1, -28, 1, -FOOT_H / 2)
local footX = 28 + saveBtn.Size.X.Offset + 10
local testBtn = UI.ghostButton(win, "Test controls", "check", 42)
testBtn.AnchorPoint = Vector2.new(1, 0.5)
testBtn.Position = UDim2.new(1, -footX, 1, -FOOT_H / 2)
footX += testBtn.Size.X.Offset + 10
local revertBtn, revertIcon = UI.iconButton(win, UDim2.new(1, -footX - 21, 1, -FOOT_H / 2), 42, "refresh", { solid = true, tip = "Revert to saved profile" })
footX += 42 + 18
UI.strip(win, UDim2.new(1, -footX, 1, -FOOT_H / 2 - 12), UDim2.fromOffset(1, 24), C.stroke)
footX += 18
local resetBtn = text(win, "Reset defaults", UDim2.new(1, -footX, 1, -FOOT_H / 2), 0, 15, { color = C.sub, ax = 1, button = true, weight = MEDIUM })
footX += UI.textWidth("Reset defaults", 15) + 24
local statusLabel = text(win, "Ready", UDim2.new(0, CONTENT_X + 20, 1, -FOOT_H / 2), 0, 15, { color = C.text2 })
UI.clip(statusLabel, UDim.new(1, -(CONTENT_X + 20) - footX))
resetBtn.MouseEnter:Connect(function()
	UI.tween(resetBtn, 0.15, { TextColor3 = C.white })
end)
resetBtn.MouseLeave:Connect(function()
	UI.tween(resetBtn, 0.25, { TextColor3 = C.sub })
end)
UI.tooltip(testBtn, "Run a check of every control")
UI.tooltip(saveBtn, "Save current settings to the active profile")

-- resize grip
local grip = UI.make("TextButton", {
	Text = "",
	AutoButtonColor = false,
	BackgroundTransparency = 1,
	AnchorPoint = Vector2.new(1, 1),
	Position = UDim2.new(1, -3, 1, -3),
	Size = UDim2.fromOffset(20, 20),
	ZIndex = 20,
	Parent = win,
})
local gripIcon = UI.icon(grip, "grip", 10, 10, 18, C.muted, 1.8)
grip.MouseEnter:Connect(function()
	UI.tintIcon(gripIcon, C.white, 0.15)
end)
grip.MouseLeave:Connect(function()
	UI.tintIcon(gripIcon, C.muted, 0.25)
end)

-------------------------------------------------------------------- status line
local statusToken = 0
function UI.baseStatus()
	local on, total = UI.featureCounts()
	if on == 0 then
		return "All features off", C.muted
	end
	return string.format("Ready  ·  %d of %d features on", on, total), C.green
end

function UI.showStatus(str, color)
	statusLabel.Text = str
	statusLabel.TextTransparency = 1
	statusLabel.Position = UDim2.new(0, CONTENT_X + 20, 1, -FOOT_H / 2 + 6)
	UI.tween(statusLabel, 0.3, { TextTransparency = 0, Position = UDim2.new(0, CONTENT_X + 20, 1, -FOOT_H / 2) })
	UI.tween(statusDot, 0.3, { BackgroundColor3 = color })
	statusHalo.BackgroundColor3 = color
end

setStatus = function(str)
	statusToken += 1
	local mine = statusToken
	UI.showStatus(str, A.hi)
	if refreshMini then
		refreshMini()
	end
	UI.after(2.6, function()
		if statusToken == mine then
			UI.showStatus(UI.baseStatus())
		end
	end)
end

function UI.refreshStatus()
	statusToken += 1
	UI.showStatus(UI.baseStatus())
end

-------------------------------------------------------------------- toasts
local TOAST_W, TOAST_H, TM = 420, 96, 16
local toastHost = UI.make("Frame", {
	Name = "Toasts",
	BackgroundTransparency = 1,
	AnchorPoint = Vector2.new(1, 1),
	Position = UDim2.new(1, -16, 1, -16),
	Size = UDim2.fromOffset(TOAST_W + TM * 2, 600),
	Parent = gui,
})
local toastScale = UI.make("UIScale", { Scale = DEFAULT_SCALE, Parent = toastHost })
UI.make("UIListLayout", {
	SortOrder = Enum.SortOrder.LayoutOrder,
	VerticalAlignment = Enum.VerticalAlignment.Bottom,
	Parent = toastHost,
})
local toastOrder = 0
local liveToasts = {}

function UI.placeToasts()
	local where = SETTING.notifications.position.value
	if where == "Left" then
		toastHost.AnchorPoint = Vector2.new(0, 1)
		toastHost.Position = UDim2.new(0, 16, 1, -16)
	elseif where == "Center" then
		toastHost.AnchorPoint = Vector2.new(0.5, 1)
		toastHost.Position = UDim2.new(0.5, 0, 1, -16)
	else
		toastHost.AnchorPoint = Vector2.new(1, 1)
		toastHost.Position = UDim2.new(1, -16, 1, -16)
	end
end

function UI.dismissToast(entry)
	if entry.gone then
		return
	end
	entry.gone = true
	local i = table.find(liveToasts, entry)
	if i then
		table.remove(liveToasts, i)
	end
	UI.groupFade(entry.wrap, 1, 0.2)
	UI.tween(entry.wrap, 0.25, { Position = UDim2.fromOffset(entry.dx * 0.6, entry.dx == 0 and 10 or 0) })
	UI.tween(entry.slot, 0.3, { Size = UDim2.new(1, 0, 0, 0) }, QUINT, OUT, 0.1).Completed:Connect(function()
		entry.slot:Destroy()
	end)
end

-- `force` shows it even when the Notifications feature is off (e.g. its own Preview button).
toast = function(title, message, iconName, force)
 local lines=math.clamp(math.ceil(UI.textWidth(message or "",14)/(TOAST_W-96))+1,2,6)
 local toastH=math.max(96,50+lines*18)
	if not force and not CTRL.notifications.enabled then
		return
	end
	UI.placeToasts()
	if not SETTING.notifications.stack.value then
		for _, e in ipairs(table.clone(liveToasts)) do
			UI.dismissToast(e)
		end
	elseif #liveToasts >= 3 then
		UI.dismissToast(liveToasts[1])
	end
	toastOrder += 1
	local where = SETTING.notifications.position.value
	local dx = where == "Left" and -56 or where == "Right" and 56 or 0

	local slot = UI.make("Frame", {
		BackgroundTransparency = 1,
		Size = UDim2.new(1, 0, 0, toastH + TM),
		LayoutOrder = toastOrder,
		Parent = toastHost,
	})
	local wrap = UI.make("Frame", { BackgroundTransparency = 1, Size = UDim2.fromScale(1, 1), Parent = slot })
	local shade = UI.make("Frame", {
		BackgroundTransparency = 1,
		Position = UDim2.fromOffset(TM, TM / 2),
		Size = UDim2.fromOffset(TOAST_W, toastH),
		Parent = wrap,
	})
	UI.softShadow(shade, 14, 3, 3, 0.09, 4)
 for layer=1,4 do
  local spread=layer*3
  local glow=UI.make("Frame",{
   Name="NotificationHalo",BackgroundTransparency=1,
   Position=UDim2.fromOffset(TM-spread,TM/2-spread),
   Size=UDim2.fromOffset(TOAST_W+spread*2,toastH+spread*2),Parent=wrap,
  },{UI.corner(16+spread)})
  UI.bindTheme(UI.stroke(A.a,2,.76+layer*.045),"Color","a").Parent=glow
 end
	local body = UI.make("TextButton", {
		Text = "",
		AutoButtonColor = false,
		BackgroundColor3 = C.raised,
		Position = UDim2.fromOffset(TM, TM / 2),
		Size = UDim2.fromOffset(TOAST_W, toastH),
		ZIndex = 2,
		Parent = wrap,
	}, { UI.corner(14), UI.stroke(C.strokeHi, 1) })
	local tile = UI.make("Frame", {
		BackgroundColor3 = C.white,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = UDim2.fromOffset(16, toastH / 2),
		Size = UDim2.fromOffset(38, 38),
		Parent = body,
	}, { UI.corner(11), UI.accentGradient(45) })
	UI.icon(tile, iconName or "bell", 19, 19, 19, C.white, 1.8)
	UI.clip(text(body, title, 68, 25, 17, { weight = SEMIBOLD }), TOAST_W - 112)
	UI.make("TextLabel",{
  Name="NotificationMessage",BackgroundTransparency=1,Position=UDim2.fromOffset(68,43),
  Size=UDim2.fromOffset(TOAST_W-90,toastH-53),Text=message or "",TextWrapped=true,
  TextXAlignment=Enum.TextXAlignment.Left,TextYAlignment=Enum.TextYAlignment.Top,
  TextSize=14,TextColor3=C.sub,FontFace=UI.font(),Parent=body,
 })
	local closeIc = UI.icon(body, "close", TOAST_W - 22, 20, 12, C.muted, 1.8)

	local entry = { slot = slot, wrap = wrap, dx = dx }
	table.insert(liveToasts, entry)
	local dur = SETTING.notifications.duration.value
	if SETTING.notifications.progress.value then
		local bar = UI.make("Frame", {
			BackgroundColor3 = C.white,
			BorderSizePixel = 0,
			Position = UDim2.new(0, 16, 1, -3),
			Size = UDim2.new(1, -32, 0, 2),
			Parent = body,
		}, { UI.round(), UI.accentGradient(0) })
		-- real time, not scaled by the motion settings
		TweenService:Create(bar, TweenInfo.new(dur, LINEAR), { Size = UDim2.new(0, 0, 0, 2) }):Play()
	end
	body.MouseEnter:Connect(function()
		UI.tintIcon(closeIc, C.white, 0.15)
	end)
	body.MouseLeave:Connect(function()
		UI.tintIcon(closeIc, C.muted, 0.2)
	end)
	body.MouseButton1Click:Connect(function()
		UI.dismissToast(entry)
	end)

	wrap.Position = UDim2.fromOffset(dx, dx == 0 and 20 or 0)
	UI.groupFade(wrap, 0, 0.3)
	UI.tween(wrap, 0.55, { Position = UDim2.fromOffset(0, 0) }, BACK)
	UI.after(dur, function()
		UI.dismissToast(entry)
	end)
end

-------------------------------------------------------------------- widgets
local rowConns = {} -- service connections owned by the current page
local rowAppliers = {} -- ctrl -> { apply(t) } repaint functions for the current page
local function bindingLabel(key)
 if key==Enum.KeyCode.F13 then return "Mouse 4"elseif key==Enum.KeyCode.F14 then return "Mouse 5"end
 return key.Name
end
local rebinding -- { setting, ctrl, stop } while waiting for a new key

-- Pill switch. `pos` = right edge / vertical centre. Returns paint(on, t).
function UI.toggleSwitch(parent, pos, on, onChange)
	local TW, TH, K = 50, 28, 20
	local st = UI.stroke(C.strokeHi, 1)
	local track = UI.make("TextButton", {
		Text = "",
		AutoButtonColor = false,
		BackgroundColor3 = C.field,
		AnchorPoint = Vector2.new(1, 0.5),
		Position = pos,
		Size = UDim2.fromOffset(TW, TH),
		ZIndex = 6,
		Parent = parent,
	}, { UI.round(), st })
	local fill = UI.make("Frame", {
		BackgroundColor3 = C.white,
		BackgroundTransparency = on and 0 or 1,
		Size = UDim2.fromScale(1, 1),
		ZIndex = 6,
		Parent = track,
	}, { UI.round(), UI.accentGradient(0) })
	local knob = UI.make("Frame", {
		BackgroundColor3 = on and C.white or C.text2,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = UDim2.new(0, on and (TW - 4 - K) or 4, 0.5, 0),
		Size = UDim2.fromOffset(K, K),
		ZIndex = 7,
		Parent = track,
	}, { UI.round() })
	local state, pressed = on, false

	local function knobGoal(width)
		return { Size = UDim2.fromOffset(width, K), Position = UDim2.new(0, state and (TW - 4 - width) or 4, 0.5, 0) }
	end
	local function paint(v, t)
		state = v
		UI.tween(fill, t, { BackgroundTransparency = v and 0 or 1 })
		UI.tween(st, t, { Color = v and A.a or C.strokeHi, Transparency = v and 0.35 or 0 })
		local goal = knobGoal(K)
		goal.BackgroundColor3 = v and C.white or C.text2
		UI.tween(knob, t, goal)
	end
	paint(on, 0)
	track.MouseButton1Down:Connect(function()
		pressed = true
		UI.tween(knob, 0.18, knobGoal(K + 6))
	end)
	track.MouseLeave:Connect(function()
		if pressed then
			pressed = false
			UI.tween(knob, 0.25, knobGoal(K))
		end
	end)
	track.MouseButton1Click:Connect(function()
		pressed = false
		paint(not state, 0.28)
		onChange(state)
	end)
	return paint
end

-- Slider. The knob glides after the pointer each frame (critically damped follow) so dragging
-- feels continuous while the stored value still snaps to `step`. A bubble shows the value.
function UI.slider(holder, s, ctrl)
	local stepper = s.stepper
	local trackW = stepper and 196 or 232
	local boxW = stepper and 132 or 88
	holder.Size = UDim2.fromOffset(trackW + 20 + boxW, 40)

	local track = UI.make("Frame", {
		BackgroundColor3 = C.field,
		BorderSizePixel = 0,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = UDim2.new(0, 0, 0.5, 0),
		Size = UDim2.fromOffset(trackW, 6),
		Parent = holder,
	}, { UI.round() })
	local fill = UI.make("Frame", {
		BackgroundColor3 = C.white,
		BorderSizePixel = 0,
		Size = UDim2.fromScale(0, 1),
		Parent = track,
	}, { UI.round(), UI.accentGradient(0) })
	local halo = UI.bindTheme(UI.make("Frame", {
		BackgroundTransparency = 1,
		BorderSizePixel = 0,
		AnchorPoint = Vector2.new(0.5, 0.5),
		Position = UDim2.fromScale(0, 0.5),
		Size = UDim2.fromOffset(18, 18),
		ZIndex = 2,
		Parent = track,
	}, { UI.round() }), "BackgroundColor3", "a")
	local knob = UI.make("Frame", {
		BackgroundColor3 = C.white,
		AnchorPoint = Vector2.new(0.5, 0.5),
		Position = UDim2.fromScale(0, 0.5),
		Size = UDim2.fromOffset(16, 16),
		ZIndex = 3,
		Parent = track,
	}, { UI.round(), UI.bindTheme(UI.stroke(A.a, 3), "Color", "a") })
	local bubbleStroke = UI.stroke(C.strokeHi, 1, 1)
	local bubble = UI.make("Frame", {
		BackgroundColor3 = C.raised,
		BackgroundTransparency = 1,
		AnchorPoint = Vector2.new(0.5, 1),
		Position = UDim2.new(0, 0, 0, -12),
		Size = UDim2.fromOffset(60, 24),
		ZIndex = 4,
		Parent = track,
	}, { UI.corner(7), bubbleStroke })
	local bubbleText = UI.make("TextLabel", {
		BackgroundTransparency = 1,
		Size = UDim2.fromScale(1, 1),
		Text = "",
		TextTransparency = 1,
		FontFace = UI.font(SEMIBOLD),
		TextSize = 14,
		TextColor3 = C.text,
		ZIndex = 4,
		Parent = bubble,
	})
	local grab = UI.make("TextButton", {
		Text = "",
		AutoButtonColor = false,
		BackgroundTransparency = 1,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = UDim2.new(0, -10, 0.5, 0),
		Size = UDim2.fromOffset(trackW + 20, 34),
		ZIndex = 5,
		Parent = holder,
	})

	-- value readout: stepper (− value +) or a plain box
	local boxStroke = UI.stroke(C.stroke, 1)
	local box = UI.make("Frame", {
		BackgroundColor3 = C.field,
		AnchorPoint = Vector2.new(1, 0.5),
		Position = UDim2.new(1, 0, 0.5, 0),
		Size = UDim2.fromOffset(boxW, 36),
		Parent = holder,
	}, { UI.corner(9), boxStroke })
	local input
	local set -- forward
	local function display(v)
		return string.format(s.format or "%.1f", v)
	end
	if stepper then
		UI.strip(box, UDim2.fromOffset(36, 7), UDim2.fromOffset(1, 22), C.stroke)
		UI.strip(box, UDim2.new(1, -37, 0, 7), UDim2.fromOffset(1, 22), C.stroke)
		local function stepBtn(pos, iconName, dir)
			local b = UI.make("TextButton", {
				Text = "",
				AutoButtonColor = false,
				BackgroundColor3 = C.hover,
				BackgroundTransparency = 1,
				Position = pos,
				Size = UDim2.fromOffset(36, 36),
				Parent = box,
			}, { UI.corner(9) })
			local ic = UI.icon(b, iconName, 18, 18, 15, C.text2, 1.9)
			b.MouseEnter:Connect(function()
				UI.tween(b, 0.15, { BackgroundTransparency = 0 })
				UI.tintIcon(ic, C.white, 0.15)
			end)
			b.MouseLeave:Connect(function()
				UI.tween(b, 0.2, { BackgroundTransparency = 1 })
				UI.tintIcon(ic, C.text2, 0.2)
			end)
			UI.pressable(b, 0.85)
			UI.repeatButton(b, function()
				set(s.value + dir * (s.nudge or s.step), true)
			end)
		end
		stepBtn(UDim2.fromOffset(0, 0), "minus", -1)
		stepBtn(UDim2.new(1, -36, 0, 0), "plus", 1)
		input = UI.make("TextBox", {
			BackgroundTransparency = 1,
			Position = UDim2.fromOffset(37, 0),
			Size = UDim2.new(1, -74, 1, 0),
			Text = "",
			TextColor3 = C.text,
			FontFace = UI.font(SEMIBOLD),
			TextSize = 16,
			ClearTextOnFocus = false,
			Parent = box,
		})
	else
		input = UI.make("TextBox", {
			BackgroundTransparency = 1,
			Size = UDim2.fromScale(1, 1),
			Text = "",
			TextColor3 = C.text,
			FontFace = UI.font(SEMIBOLD),
			TextSize = 16,
			ClearTextOnFocus = false,
			Parent = box,
		})
	end

	local function toP(v)
		return (v - s.min) / (s.max - s.min)
	end
	local visP = toP(s.value) -- where the knob is drawn
	local targetP = visP -- where it is heading
	local dragging, hovering, bubbleOn = false, false, false
	local function draw()
		fill.Size = UDim2.fromScale(visP, 1)
		knob.Position = UDim2.fromScale(visP, 0.5)
		halo.Position = knob.Position
		bubble.Position = UDim2.new(visP, 0, 0, -12)
	end
	draw()

	-- `raw` (0..1) lets the knob follow the pointer exactly while the value snaps
	set = function(v, fire, raw)
		v = math.clamp(math.round(v / s.step) * s.step, s.min, s.max)
		local changed = v ~= s.value
		s.value = v
		targetP = raw or toP(v)
		bubbleText.Text = display(v) .. (s.suffix or "")
		if not input:IsFocused() then
			input.Text = display(v) .. (stepper and "" or (s.suffix or ""))
		end
		if fire and changed then
			if s.onSet then
				s.onSet(v)
			end
			controlChanged:Fire(ctrl.id, s.id, v)
		end
	end
	set(s.value, false)

	table.insert(rowConns, RunService.RenderStepped:Connect(function(dt)
		if math.abs(visP - targetP) > 0.0005 then
			visP += (targetP - visP) * (1 - math.exp(-dt * (dragging and 30 or 16)))
			draw()
		elseif visP ~= targetP then
			visP = targetP
			draw()
		end
	end))

	local function refreshKnob()
		local d = dragging and 20 or hovering and 18 or 16
		UI.tween(knob, 0.2, { Size = UDim2.fromOffset(d, d) })
		UI.tween(halo, 0.25, {
			Size = UDim2.fromOffset(dragging and 38 or 18, dragging and 38 or 18),
			BackgroundTransparency = dragging and 0.8 or 1,
		})
		local show = dragging or hovering
		if show ~= bubbleOn then
			bubbleOn = show
			UI.tween(bubble, 0.18, { BackgroundTransparency = show and 0 or 1 })
			UI.tween(bubbleText, 0.18, { TextTransparency = show and 0 or 1 })
			UI.tween(bubbleStroke, 0.18, { Transparency = show and 0 or 1 })
		end
	end
	local function fromX(px)
		local p = math.clamp((px - track.AbsolutePosition.X) / track.AbsoluteSize.X, 0, 1)
		set(s.min + p * (s.max - s.min), true, p)
	end
	grab.MouseEnter:Connect(function()
		hovering = true
		refreshKnob()
	end)
	grab.MouseLeave:Connect(function()
		hovering = false
		refreshKnob()
	end)
	grab.InputBegan:Connect(function(io)
		if io.UserInputType == Enum.UserInputType.MouseButton1 or io.UserInputType == Enum.UserInputType.Touch then
			dragging = true
			refreshKnob()
			fromX(io.Position.X)
		end
	end)
	table.insert(rowConns, UserInputService.InputChanged:Connect(function(io)
		if dragging and (io.UserInputType == Enum.UserInputType.MouseMovement or io.UserInputType == Enum.UserInputType.Touch) then
			fromX(io.Position.X)
		end
	end))
	table.insert(rowConns, UserInputService.InputEnded:Connect(function(io)
		if dragging and (io.UserInputType == Enum.UserInputType.MouseButton1 or io.UserInputType == Enum.UserInputType.Touch) then
			dragging = false
			targetP = toP(s.value) -- settle onto the snapped value
			refreshKnob()
		end
	end))
	input.Focused:Connect(function()
		UI.tween(boxStroke, 0.2, { Color = A.a })
		input.Text = display(s.value)
	end)
	input.FocusLost:Connect(function()
		UI.tween(boxStroke, 0.3, { Color = C.stroke })
		local n = tonumber(string.match(input.Text, "-?[%d%.]+") or "")
		set(n or s.value, n ~= nil)
		input.Text = display(s.value) .. (stepper and "" or (s.suffix or ""))
	end)

	return function(t)
		set(s.value, false)
		if t <= 0 then
			visP = targetP
			draw()
		end
	end
end

-- Segmented control with a gliding pill.
function UI.segmented(holder, s, ctrl)
	local n = #s.options
	local widest = 0
	for _, o in ipairs(s.options) do
		widest = math.max(widest, UI.textWidth(o, 16))
	end
	local segW = math.max(76, math.ceil(widest) + 30)
	local h = 38
	holder.Size = UDim2.fromOffset(segW * n + 8, h)
	holder.BackgroundColor3 = C.field
	holder.BackgroundTransparency = 0
	UI.corner(10).Parent = holder
	UI.stroke(C.line, 1).Parent = holder
	local pill = UI.make("Frame", {
		BackgroundColor3 = C.white,
		BackgroundTransparency = 0.8,
		Position = UDim2.fromOffset(4, 4),
		Size = UDim2.fromOffset(segW, h - 8),
		Parent = holder,
	}, { UI.corner(7), UI.accentGradient(0), UI.bindTheme(UI.stroke(A.a, 1, 0.3), "Color", "a") })
	local buttons = {}
	local function select(i, fire, t)
		s.value = s.options[i]
		UI.tween(pill, t, { Position = UDim2.fromOffset(4 + (i - 1) * segW, 4) })
		for j, b in ipairs(buttons) do
			UI.tween(b, t * 0.8, { TextColor3 = j == i and C.white or C.sub })
		end
		if fire then
			controlChanged:Fire(ctrl.id, s.id, s.value)
		end
	end
	for i, opt in ipairs(s.options) do
		local b = UI.make("TextButton", {
			AutoButtonColor = false,
			BackgroundTransparency = 1,
			Text = opt,
			FontFace = UI.font(MEDIUM),
			TextSize = 16,
			TextColor3 = C.sub,
			Position = UDim2.fromOffset(4 + (i - 1) * segW, 4),
			Size = UDim2.fromOffset(segW, h - 8),
			ZIndex = 2,
			Parent = holder,
		})
		buttons[i] = b
		b.MouseEnter:Connect(function()
			if s.value ~= opt then
				UI.tween(b, 0.15, { TextColor3 = C.text2 })
			end
		end)
		b.MouseLeave:Connect(function()
			if s.value ~= opt then
				UI.tween(b, 0.2, { TextColor3 = C.sub })
			end
		end)
		b.MouseButton1Click:Connect(function()
			if s.value ~= opt then
				select(i, true, 0.35)
			end
		end)
	end
	select(table.find(s.options, s.value) or 1, false, 0)
	return function(t)
		select(table.find(s.options, s.value) or 1, false, t)
	end
end

-- Keycap that listens for the next key press.
function UI.keybind(holder, s, ctrl)
	holder.AutomaticSize = Enum.AutomaticSize.X
	holder.Size = UDim2.fromOffset(0, 38)
	local capStroke = UI.stroke(C.strokeHi, 1)
	local cap = UI.make("TextButton", {
		AutoButtonColor = false,
		BackgroundColor3 = C.raised,
		AutomaticSize = Enum.AutomaticSize.X,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = UDim2.new(0, 0, 0.5, 0),
		Size = UDim2.fromOffset(64, 38),
		Text = bindingLabel(s.value),
		FontFace = UI.font(SEMIBOLD),
		TextSize = 16,
		TextColor3 = C.text,
		Parent = holder,
	}, {
		UI.corner(9),
		capStroke,
		UI.padding(16, 16, 0, 0),
	})
	local capScale = UI.pressable(cap, 0.92)
	cap.MouseEnter:Connect(function()
		UI.tween(cap, 0.15, { BackgroundColor3 = C.hover })
		if not (rebinding and rebinding.setting == s) then
			UI.tween(capStroke, 0.15, { Color = C.sub })
		end
	end)
	cap.MouseLeave:Connect(function()
		UI.tween(cap, 0.25, { BackgroundColor3 = C.raised })
		if not (rebinding and rebinding.setting == s) then
			UI.tween(capStroke, 0.25, { Color = C.strokeHi })
		end
	end)
	cap.MouseButton1Click:Connect(function()
		if rebinding then
			rebinding.stop()
		end
		local beat = TweenService:Create(capStroke, TweenInfo.new(0.55, SINE, INOUT, -1, true), { Color = A.hi })
		capStroke.Color = A.soft
		beat:Play()
		cap.Text = "Press key / mouse…"
		UI.tween(cap, 0.2, { TextColor3 = A.hi })
		rebinding = {
			setting = s,
			ctrl = ctrl,
			stop = function()
				beat:Cancel()
				UI.tween(capStroke, 0.25, { Color = C.strokeHi })
				UI.tween(cap, 0.25, { TextColor3 = C.text })
				cap.Text = bindingLabel(s.value)
				capScale.Scale = 1.12
				UI.tween(capScale, 0.45, { Scale = 1 }, BACK)
			end,
		}
	end)
	return function()
		cap.Text = bindingLabel(s.value)
	end
end

-- Gradient colour dots; the selected one gets a ring and a check.
function UI.swatches(holder, s, ctrl)
	local D, GAP = 30, 12
	local n = #s.options
	holder.Size = UDim2.fromOffset(n * D + (n - 1) * GAP + 8, 40)
	local items = {}
	local function paint(t, pop)
		for _, it in ipairs(items) do
			local sel = it.name == s.value
			UI.tween(it.ring, t, { Transparency = sel and 0.1 or 1 })
			it.mark.Visible = sel
			if sel and pop then
				it.markScale.Scale = 0.3
				UI.tween(it.markScale, 0.45, { Scale = 1 }, BACK)
			end
		end
	end
	for i, name in ipairs(s.options) do
		local th = THEME_BY_NAME[name]
		local b = UI.make("TextButton", {
			Text = "",
			AutoButtonColor = false,
			BackgroundColor3 = C.white,
			AnchorPoint = Vector2.new(0.5, 0.5),
			Position = UDim2.new(0, 4 + D / 2 + (i - 1) * (D + GAP), 0.5, 0),
			Size = UDim2.fromOffset(D, D),
			Parent = holder,
		}, { UI.round(), UI.make("UIGradient", { Rotation = 45, Color = ColorSequence.new(th.a, th.b) }) })
		local ring = UI.stroke(C.white, 2, 1)
		UI.make("Frame", {
			BackgroundTransparency = 1,
			AnchorPoint = Vector2.new(0.5, 0.5),
			Position = UDim2.fromScale(0.5, 0.5),
			Size = UDim2.fromOffset(D + 8, D + 8),
			Parent = b,
		}, { UI.round(), ring })
		local mark = UI.icon(b, "check", D / 2, D / 2, 14, C.white, 2.2)
		local markScale = UI.make("UIScale", { Parent = mark })
		UI.pressable(b, 0.86)
		UI.tooltip(b, name)
		b.MouseButton1Click:Connect(function()
			if s.value ~= name then
				s.value = name
				paint(0.25, true)
				controlChanged:Fire(ctrl.id, s.id, name)
			end
		end)
		items[i] = { name = name, ring = ring, mark = mark, markScale = markScale }
	end
	paint(0, false)
	return function(t)
		paint(t, false)
	end
end

-- Lane with a dot that travels using the current motion settings.
function UI.motionPreview(holder, s, ctrl)
	holder.Size = UDim2.fromOffset(352, 38)
	local lane = UI.make("Frame", {
		BackgroundColor3 = C.field,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = UDim2.new(0, 0, 0.5, 0),
		Size = UDim2.fromOffset(250, 34),
		Parent = holder,
	}, { UI.round(), UI.stroke(C.line, 1) })
	UI.strip(lane, UDim2.new(0, 18, 0.5, 0), UDim2.new(1, -36, 0, 1), C.stroke)
	local dot = UI.make("Frame", {
		BackgroundColor3 = C.white,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = UDim2.new(0, 5, 0.5, 0),
		Size = UDim2.fromOffset(24, 24),
		ZIndex = 2,
		Parent = lane,
	}, { UI.round(), UI.accentGradient(45) })
	local at = 0
	local function play()
		at = 1 - at
		UI.tween(dot, 0.6, { Position = UDim2.new(at, at == 1 and -29 or 5, 0.5, 0) })
	end
	local b = UI.ghostButton(holder, "Play", "play", 36)
	b.AnchorPoint = Vector2.new(1, 0.5)
	b.Position = UDim2.new(1, 0, 0.5, 0)
	b.MouseButton1Click:Connect(play)
	playMotionPreview = play
	return function() end
end

function UI.checkbox(panel, o, x, y, ctrl)
	local boxStroke = UI.stroke(C.strokeHi, 1.5)
	local box = UI.make("TextButton", {
		Text = "",
		AutoButtonColor = false,
		BackgroundColor3 = C.white,
		BackgroundTransparency = 1,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = UDim2.fromOffset(x, y),
		Size = UDim2.fromOffset(22, 22),
		Parent = panel,
	}, { UI.corner(6), boxStroke, UI.accentGradient(45) })
	local mark = UI.icon(box, "check", 11, 11, 15, C.white, 2.4)
	local markScale = UI.make("UIScale", { Parent = mark })
	local label = text(panel, o.label, x + 34, y, 16, { color = C.text2, button = true, weight = MEDIUM })
	UI.pressable(box, 0.85)

	local function paint(t, pop)
		local on = o.value
		UI.tween(box, t, { BackgroundTransparency = on and 0 or 1 })
		UI.tween(boxStroke, t, { Color = on and A.a or C.strokeHi, Transparency = on and 1 or 0 })
		UI.tween(label, t, { TextColor3 = on and C.text or C.text2 })
		if on then
			mark.Visible = true
			if pop then
				markScale.Scale = 0.3
				UI.tween(markScale, 0.4, { Scale = 1 }, BACK)
			else
				markScale.Scale = 1
			end
		elseif t > 0 then
			UI.tween(markScale, 0.12, { Scale = 0.3 }, QUINT, IN).Completed:Connect(function(state)
				if state == Enum.PlaybackState.Completed and not o.value then
					mark.Visible = false
				end
			end)
		else
			mark.Visible = false
		end
	end
	local function flip()
		o.value = not o.value
		paint(0.2, true)
		controlChanged:Fire(ctrl.id, o.id, o.value)
	end
	box.MouseButton1Click:Connect(flip)
	label.MouseButton1Click:Connect(flip)
	local function hot(v)
		if not o.value then
			UI.tween(boxStroke, 0.15, { Color = v and C.sub or C.strokeHi })
		end
	end
	box.MouseEnter:Connect(function()
		hot(true)
	end)
	box.MouseLeave:Connect(function()
		hot(false)
	end)
	label.MouseEnter:Connect(function()
		hot(true)
	end)
	label.MouseLeave:Connect(function()
		hot(false)
	end)
	paint(0, false)
	return function(t)
		paint(t, t > 0)
	end, 34 + UI.textWidth(o.label, 16)
end

-- Outlined action; fires ControlChanged(ctrl.id, it.id, true). Returns its width.
function UI.actionButton(panel, it, x, y, ctrl)
	local b = UI.ghostButton(panel, it.label, it.icon, 40)
	b.AnchorPoint = Vector2.new(0, 0.5)
	b.Position = UDim2.fromOffset(x, y)
	if it.help then
		UI.tooltip(b, it.help)
	end
	b.MouseButton1Click:Connect(function()
		controlChanged:Fire(ctrl.id, it.id, true)
	end)
	return b.Size.X.Offset
end

-- Live sample text driven by the card's `size` and `weight` settings.
function UI.textPreview(panel, ctrl, y, rh)
	local values = SETTING[ctrl.id]
	local box = UI.make("Frame", {
		BackgroundColor3 = C.bg,
		BackgroundTransparency = 0.3,
		ClipsDescendants = true,
		Position = UDim2.fromOffset(24, y + 10),
		Size = UDim2.new(1, -48, 0, rh - 20),
		Parent = panel,
	}, { UI.corner(10), UI.stroke(C.line, 1) })
	local caption = text(box, "", UDim2.new(1, -14, 0, 14), 0, 12, { color = C.muted, ax = 1, weight = SEMIBOLD })
	local sample = UI.make("TextLabel", {
		BackgroundTransparency = 1,
		Position = UDim2.fromOffset(18, 6),
		Size = UDim2.new(1, -36, 1, -6),
		Text = "The quick brown fox jumps over the lazy dog",
		TextXAlignment = Enum.TextXAlignment.Left,
		TextTruncate = Enum.TextTruncate.AtEnd,
		TextColor3 = C.text,
		FontFace = UI.font(MEDIUM),
		TextSize = 22,
		Parent = box,
	})
	local weights = { Regular = Enum.FontWeight.Regular, Medium = MEDIUM, Bold = BOLD }
	local function apply(t)
		local size = values.size.value
		local w = values.weight.value
		sample.FontFace = UI.font(weights[w] or MEDIUM)
		UI.tween(sample, t or 0, { TextSize = size })
		caption.Text = string.upper(string.format("%d px  ·  %s", size, w))
	end
	apply(0)
	table.insert(rowConns, controlChanged.Event:Connect(function(id)
		if id == ctrl.id then
			apply(0.15)
		end
	end))
	return apply
end

local SETTING_BUILDERS = {
	slider = UI.slider,
	segment = UI.segmented,
	key = UI.keybind,
	swatch = UI.swatches,
	motionPreview = UI.motionPreview,
}

-------------------------------------------------------------------- cards + sections
SETTING_BUILDERS.combo=function(panel,s,x,y,ctrl)
 local holder=UI.make("Frame",{BackgroundTransparency=1,Position=UDim2.fromOffset(32,y-155),Size=UDim2.new(1,-64,0,310),Parent=panel})
 local steps,selected,lastSource={},1,nil
 local chipLinks,keyButtons={},{}
 local hint=text(holder,"1. ADD MOVES",0,10,15,{color=C.sub})
 local selectionLabel=text(holder,"2. CLICK A STEP TO EDIT",0,74,15,{color=C.sub})
 local tray=UI.make("Frame",{BackgroundTransparency=1,Position=UDim2.fromOffset(0,88),Size=UDim2.new(1,0,0,96),Parent=holder})
 local delaySetting={id="sequenceDelay",min=.08,max=1.5,step=.01,value=.25,format="%.2f",suffix=" s"}
 local delayPaint,paint
 local function parse(value)
  local result={}
  for keys,delay in tostring(value):gmatch("([ZXCVWAIT]+)@([%d%.]+)")do
   if #result<12 then result[#result+1]={keys=keys,delay=math.clamp(tonumber(delay)or .25,.08,1.5)}end
  end
  return result
 end
 local function commit()
  local values={}
  for _,step in ipairs(steps)do values[#values+1]=step.keys.."@"..string.format("%.2f",step.delay)end
  s.value=table.concat(values," > ");lastSource=s.value
  controlChanged:Fire(ctrl.id,s.id,s.value);paint()
 end
 local function button(parent,label,pos,size,fn)
  local b=UI.make("TextButton",{Text=label,AutoButtonColor=false,BackgroundColor3=C.hover,TextColor3=C.text,FontFace=UI.font(),TextSize=17,Position=pos,Size=size,Parent=parent},{UI.corner(7),UI.stroke(C.field,1)})
  table.insert(rowConns,b.MouseButton1Click:Connect(fn));return b
 end
 for i,keys in ipairs({"Z","X","C","V","WAIT"})do
  button(holder,keys=="WAIT"and "+ Pause"or "+ "..keys,UDim2.new((i-1)/5,0,0,28),UDim2.new(1/5,-6,0,32),function()
   if #steps>=12 then hint.Text="Sequence full · remove a step first";return end
   table.insert(steps,{keys=keys,delay=delaySetting.value});selected=#steps;commit()
  end)
 end
 text(holder,"Combine keys:",0,206,17,{color=C.text2})
 for i,key in ipairs({"Z","X","C","V"})do
  keyButtons[key]=button(holder,key,UDim2.fromOffset(150+(i-1)*57,190),UDim2.fromOffset(49,32),function()
   local step=steps[selected]
   if not step or step.keys=="WAIT"then selectionLabel.Text="Select a move step first";return end
   if step.keys:find(key,1,true)then
    local remaining=step.keys:gsub(key,"")
    if #remaining==0 then selectionLabel.Text="Keep one key, or Remove the step";return end
    step.keys=remaining
   elseif #step.keys<4 then step.keys=step.keys..key end
   commit()
  end)
 end
 button(holder,"Remove step",UDim2.new(1,-148,0,190),UDim2.fromOffset(148,32),function()if steps[selected]then table.remove(steps,selected);commit()end end)
 local actions={
  {"Double tap",function()
   local step=steps[selected]
   if not step or step.keys=="WAIT"then selectionLabel.Text="Select a move to double tap";return end
   if #steps>=12 then selectionLabel.Text="Remove a step to make room for the second tap";return end
   local after=step.delay;step.delay=.10
   table.insert(steps,selected+1,{keys=step.keys,delay=after});selected=selected+1;commit()
  end},
  {"Move left",function()if selected>1 then steps[selected],steps[selected-1]=steps[selected-1],steps[selected];selected=selected-1;commit()end end},
  {"Move right",function()if selected<#steps then steps[selected],steps[selected+1]=steps[selected+1],steps[selected];selected=selected+1;commit()end end},
  {"Clear sequence",function()steps={};selected=1;commit()end},
 }
 for i,a in ipairs(actions)do button(holder,a[1],UDim2.new((i-1)/#actions,0,0,234),UDim2.new(1/#actions,-6,0,30),a[2])end
 text(holder,"Wait after move",0,282,18,{color=C.text2})
 local delayHolder=UI.make("Frame",{BackgroundTransparency=1,AnchorPoint=Vector2.new(1,.5),Position=UDim2.new(1,0,0,282),Parent=holder})
 delayPaint=UI.slider(delayHolder,delaySetting,ctrl)
 paint=function()
  for _,c in ipairs(chipLinks)do c:Disconnect()end;chipLinks={}
  for _,child in ipairs(tray:GetChildren())do child:Destroy()end
  selected=math.clamp(selected,1,math.max(1,#steps))
  hint.Text="1. ADD MOVES · "..#steps.." / 12"
  selectionLabel.Text=steps[selected]and "2. EDIT STEP "..selected.." · choose keys to combine below"or "Click + Z, + X, + C or + V to start"
  if #steps==0 then text(tray,"Or choose a preset above and press Play combo.",0,30,18,{color=C.sub})end
  for i,step in ipairs(steps)do
   local label=step.keys=="WAIT"and "Pause"or step.keys:gsub(".","%0+"):sub(1,-2)
   local b=UI.make("TextButton",{Text=i..". "..label.."\n"..string.format("%.2f s",step.delay),AutoButtonColor=false,TextColor3=C.text,BackgroundColor3=i==selected and A.dim or C.hover,FontFace=UI.font(),TextSize=16,Position=UDim2.new(((i-1)%6)/6,0,0,math.floor((i-1)/6)*47),Size=UDim2.new(1/6,-6,0,41),Parent=tray},{UI.corner(7),UI.stroke(i==selected and A.a or C.field,1)})
   chipLinks[#chipLinks+1]=b.MouseButton1Click:Connect(function()selected=i;paint()end)
  end
  local step=steps[selected]
  for key,b in pairs(keyButtons)do
   local on=step and step.keys~="WAIT"and step.keys:find(key,1,true)
   b.BackgroundColor3=on and A.dim or C.hover;b.TextColor3=on and C.white or C.sub
  end
  delaySetting.value=step and step.delay or .25
  if delayPaint then delayPaint(0)end
 end
 table.insert(rowConns,controlChanged.Event:Connect(function(id,field,value)
  if id==ctrl.id and field=="sequenceDelay"and steps[selected]then steps[selected].delay=math.clamp(value,.08,1.5);commit()end
 end))
 table.insert(rowConns,{Disconnect=function()for _,c in ipairs(chipLinks)do c:Disconnect()end end})
 steps=parse(s.value);lastSource=s.value;paint()
 return function()if s.value~=lastSource then steps=parse(s.value);lastSource=s.value;selected=1 end;paint()end
end

SETTING_BUILDERS.library=function(panel,s,x,y,ctrl)
 local holder=UI.make("Frame",{BackgroundTransparency=1,Position=UDim2.fromOffset(24,y-105),Size=UDim2.new(1,-48,0,210),Parent=panel})
 local selected
 local keySetting={id="savedComboHotkey",value=Enum.KeyCode.K}
 local function names()
  local list={}for _,e in ipairs(api.GetNamedCombos())do list[#list+1]=e.name end
  return #list>0 and list or {"No saved combos"}
 end
 text(holder,"Saved combos",0,20,16,{color=C.sub})
 local box=UI.make("TextBox",{Name="ComboName",Text="My combo",PlaceholderText="Name this combo",ClearTextOnFocus=false,TextColor3=C.text,BackgroundColor3=C.field,FontFace=UI.font(),TextSize=16,Position=UDim2.fromOffset(150,52),Size=UDim2.fromOffset(360,38),Parent=holder},{UI.corner(8)})
 text(holder,"Name",0,71,16,{color=C.sub})
 text(holder,"Hotkey",0,120,16,{color=C.sub})
 local keyHolder=UI.make("Frame",{BackgroundTransparency=1,Position=UDim2.fromOffset(150,101),Parent=holder})
 local paintKey=UI.keybind(keyHolder,keySetting,ctrl)
 local picker=dropdown(holder,UDim2.fromOffset(150,20),360,38,names,"Choose saved combo",function(name)
  local ok,e=api.LoadNamedCombo(name)
  if not ok then setStatus(e);return end
  selected=name;box.Text=name;keySetting.value=e.key;paintKey()
  SETTING.comboBuilder.sequence.value=e.sequence;SETTING.comboBuilder.repeats.value=e.repeats;SETTING.comboBuilder.sprint.value=e.sprint
  SETTING.comboBuilder.preset.value="Custom";repaintAll();setStatus("Loaded "..name)
 end)
 local function action(label,offset,fn)
  local b=UI.ghostButton(holder,label,"folder",38);b.Position=UDim2.fromOffset(offset,155)
  b.MouseButton1Click:Connect(fn)
 end
 action("Save combo + key",0,function()
  if keySetting.value==PREF_KEY.value then setStatus("Choose a different key from the menu shortcut");return end
  local ok,message=api.SaveNamedCombo(box.Text,keySetting.value)
  if ok then
   selected=box.Text:match("^%s*(.-)%s*$");picker.set(selected,0)
   CTRL.comboBuilder.enabled=true;api.SetOption("comboBuilder",true);repaintAll()
  end
  setStatus(message)
 end)
 action("Delete saved",235,function()
  local ok,message=api.DeleteNamedCombo(selected)
  if ok then selected=nil;picker.set("Choose saved combo",0)end
  setStatus(message)
 end)
 return function()end
end

local HEADER_H = 74
local ROW_H = { combo=330, library=230, slider = 66, segment = 66, key = 66, swatch = 66, motionPreview = 66, checks = 56, actions = 66, note = 50, divider = 18, textPreview = 108 }

function UI.rowHeight(r)
	local base = ROW_H[r.type] or 62
	if r.type == "divider" or r.type == "textPreview" or r.type == "combo" or r.type == "library" then
		return base
	end
	return math.floor(base * layoutState.density + 0.5)
end

function UI.panelHeight(rows)
	local h = 12
	for _, r in ipairs(rows) do
		h += UI.rowHeight(r)
	end
	return h
end

function UI.buildPanel(panel, ctrl, appliers)
	local y = 6
	for i, r in ipairs(ctrl.rows) do
		local rh = UI.rowHeight(r)
		local cy = y + rh / 2
		local t = r.type
		if i > 1 and layoutState.dividers and t ~= "divider" then
			UI.strip(panel, UDim2.fromOffset(24, y), UDim2.new(1, -48, 0, 1), C.line)
		end
		if t == "divider" then
			UI.strip(panel, UDim2.fromOffset(24, cy), UDim2.new(1, -48, 0, 1), C.line)
		elseif t == "note" then
			UI.icon(panel, "info", 34, cy, 17, C.sub, 1.6)
			UI.make("TextLabel", {
				BackgroundTransparency = 1,
				AnchorPoint = Vector2.new(0, 0.5),
				Position = UDim2.fromOffset(54, cy),
				Size = UDim2.new(1, -78, 0, rh - 8),
				Text = r.label,
				TextWrapped = true,
				TextXAlignment = Enum.TextXAlignment.Left,
				FontFace = UI.font(),
				TextSize = 15,
				TextColor3 = C.sub,
				Parent = panel,
			})
		elseif t == "checks" then
			local cx = 24
			for _, it in ipairs(r.items) do
				local apply, w = UI.checkbox(panel, it, cx, cy, ctrl)
				table.insert(appliers, apply)
				cx += w + 36
			end
		elseif t == "actions" then
			local cx = 24
			for _, it in ipairs(r.items) do
				cx += UI.actionButton(panel, it, cx, cy, ctrl) + 12
			end
		elseif t == "library" then
   table.insert(appliers,SETTING_BUILDERS.library(panel,r,0,cy,ctrl))
  elseif t == "combo" then
   table.insert(appliers,SETTING_BUILDERS.combo(panel,r,0,cy,ctrl))
  elseif t == "textPreview" then
			table.insert(appliers, UI.textPreview(panel, ctrl, y, rh))
		else
			-- label (+ optional description) on the left, control right-aligned
			local hasDesc = r.desc ~= nil
			local label = text(panel, r.label, 24, hasDesc and cy - 10 or cy, 17, { weight = MEDIUM, button = r.help ~= nil })
			if r.help then
				UI.tooltip(label, r.help)
			end
			if hasDesc then
				text(panel, r.desc, 24, cy + 12, 14, { color = C.sub })
			end
			local holder = UI.make("Frame", {
				BackgroundTransparency = 1,
				AnchorPoint = Vector2.new(1, 0.5),
				Position = UDim2.new(1, -24, 0, cy),
				Size = UDim2.fromOffset(0, 40),
				Parent = panel,
			})
			local build = SETTING_BUILDERS[t]
			if build then
				table.insert(appliers, build(holder, r, ctrl))
			end
		end
		y += rh
	end
end

-- Always-visible group of settings with a caption (no switch).
function UI.buildSection(ctrl, order)
	local appliers = {}
	rowAppliers[ctrl] = appliers
	local ph = UI.panelHeight(ctrl.rows)
	local capH = layoutState.captions and 30 or 0
	local slot, body = UI.newSlot(capH + ph, order, ctrl.id)
	if capH > 0 then
		UI.tracked(body, string.upper(ctrl.name), 8, 14, 12, C.muted, 1.6)
	end
	local panel = UI.make("Frame", {
		BackgroundColor3 = C.surface,
		Position = UDim2.fromOffset(2, capH + 2),
		Size = UDim2.new(1, -4, 0, ph),
		Parent = body,
	}, { UI.corner(14), UI.stroke(C.stroke, 1) })
	UI.buildPanel(panel, ctrl, appliers)
	return slot, body
end

-- Collapsible feature card with an ON/OFF switch. ON opens its settings, OFF closes them;
-- the header still opens/closes them by hand either way.
function UI.buildCard(ctrl, order)
	local rows = ctrl.rows or {}
	local hasPanel = #rows > 0
	local panelH = hasPanel and UI.panelHeight(rows) or 0
	local openH = HEADER_H + panelH
	local appliers = {}
	rowAppliers[ctrl] = appliers

	local slot, body = UI.newSlot(HEADER_H, order, ctrl.id)
	local cardStroke = UI.stroke(C.stroke, 1)
	local card = UI.make("Frame", {
		BackgroundColor3 = C.surface,
		ClipsDescendants = true,
		Position = UDim2.fromOffset(2, 2),
		Size = UDim2.new(1, -4, 0, HEADER_H),
		Parent = body,
	}, { UI.corner(14), cardStroke })
	-- accent hairline along the top edge while the feature is on
	local sheen = UI.make("Frame", {
		BackgroundColor3 = C.white,
		BackgroundTransparency = 1,
		BorderSizePixel = 0,
		Position = UDim2.fromOffset(28, 0),
		Size = UDim2.new(1, -56, 0, 1),
		Parent = card,
	}, { UI.accentGradient(0, EDGE_FADE) })

	local header = UI.make("TextButton", {
		Text = "",
		AutoButtonColor = false,
		BackgroundColor3 = C.white,
		BackgroundTransparency = 1,
		Size = UDim2.new(1, 0, 0, HEADER_H),
		ZIndex = 5,
		Parent = card,
	})

	local tile = UI.make("Frame", {
		BackgroundColor3 = C.field,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = UDim2.fromOffset(18, HEADER_H / 2),
		Size = UDim2.fromOffset(42, 42),
		Parent = card,
	}, { UI.corner(12) })
	local tileFill = UI.make("Frame", {
		BackgroundColor3 = C.white,
		BackgroundTransparency = 1,
		Size = UDim2.fromScale(1, 1),
		Parent = tile,
	}, { UI.corner(12), UI.accentGradient(45) })
	local tileIcon = UI.icon(tile, ctrl.icon or "layers", 21, 21, 21, C.sub, 1.8)
	tileIcon.ZIndex = 2

	local hasDesc = ctrl.desc ~= nil
	local title = text(card, ctrl.name, 76, hasDesc and HEADER_H / 2 - 10 or HEADER_H / 2, 19, { weight = SEMIBOLD })
	if hasDesc then
		text(card, ctrl.desc, 76, HEADER_H / 2 + 13, 14, { color = C.sub })
	end

	local chevron
	if hasPanel then
		chevron = UI.make("Frame", {
			BackgroundTransparency = 1,
			AnchorPoint = Vector2.new(0.5, 0.5),
			Position = UDim2.new(1, -32, 0, HEADER_H / 2),
			Size = UDim2.fromOffset(16, 16),
			Parent = card,
		})
	end

	local panel
	if hasPanel then
		panel = UI.make("Frame", {
			BackgroundTransparency = 1,
			Position = UDim2.fromOffset(0, HEADER_H),
			Size = UDim2.new(1, 0, 0, panelH),
			Parent = card,
		})
		UI.strip(panel, UDim2.fromOffset(0, 0), UDim2.new(1, 0, 0, 1), C.line)
		UI.buildPanel(panel, ctrl, appliers)
	end

	local hovered = false
	local function paintCard(t)
		local on = ctrl.enabled == true
		UI.tween(cardStroke, t, { Color = hovered and C.strokeHi or (on and A.soft or C.stroke) })
		UI.tween(tileFill, t, { BackgroundTransparency = on and 0 or 1 })
		UI.tintIcon(tileIcon, on and C.white or C.sub, t)
		UI.tween(sheen, t, { BackgroundTransparency = on and 0.15 or 1 })
	end

	local function setExpanded(v, t)
		if not hasPanel then
			v = false
		end
		ctrl.expanded = v
		local h = v and openH or HEADER_H
		-- slot and card resize together, so cards below glide instead of jumping
		UI.tween(slot, t, { Size = UDim2.new(1, 0, 0, h + 4) })
		UI.tween(card, t, { Size = UDim2.new(1, -4, 0, h) })
		if chevron then
			UI.spinIcon(chevron, "chevronRight", C.sub, 2, v and 90 or 0, t)
		end
		if panel then
			if t > 0 then
				if v then
					panel.Visible = true
					panel.Position = UDim2.fromOffset(0, HEADER_H - 12)
					UI.groupFade(panel, 0, 0.26, 0.04)
					UI.tween(panel, 0.32, { Position = UDim2.fromOffset(0, HEADER_H) }, QUINT, OUT, 0.04)
				else
					UI.groupFade(panel, 1, 0.16)
					UI.tween(panel, 0.16, { Position = UDim2.fromOffset(0, HEADER_H - 8) })
				end
			else
				panel.Visible = v
				if v and fades[panel] then
					UI.groupFade(panel, 0, 0.01) -- unwrap a panel left faded out by an earlier collapse
				end
			end
		end
	end
	setExpanded(ctrl.expanded == true, 0)
	paintCard(0)

	local paintToggle = UI.toggleSwitch(card, UDim2.new(1, hasPanel and -56 or -22, 0, HEADER_H / 2), ctrl.enabled == true, function(on)
		ctrl.enabled = on
		paintCard(0.25)
		controlChanged:Fire(ctrl.id, "enabled", on)
		if ctrl.expanded ~= on then
			closePopup()
			setExpanded(on, 0.34)
		end
	end)
	table.insert(appliers, function(t)
		paintToggle(ctrl.enabled == true, t)
		paintCard(t)
		
	end)

	header.MouseEnter:Connect(function()
		hovered = true
		UI.tween(header, 0.2, { BackgroundTransparency = 0.985 })
		paintCard(0.2)
	end)
	header.MouseLeave:Connect(function()
		hovered = false
		UI.tween(header, 0.3, { BackgroundTransparency = 1 })
		paintCard(0.3)
	end)
	header.MouseButton1Click:Connect(function()
		if hasPanel then
			closePopup()
			setExpanded(not ctrl.expanded, 0.34)
		end
	end)

	return slot, body
end

function UI.buildControl(ctrl, order)
	if ctrl.kind == "section" then
		return UI.buildSection(ctrl, order)
	end
	return UI.buildCard(ctrl, order)
end

-------------------------------------------------------------------- profiles
function UI.snapshot()
	local snap = {}
	UI.forEachControl(function(ctrl)
		local values = {}
		UI.forEachValue(ctrl, function(s)
			values[s.id] = s.value
		end)
		snap[ctrl.id] = { enabled = ctrl.enabled, values = values }
	end, true)
	return snap
end

repaintAll = function(t)
	for _, appliers in pairs(rowAppliers) do
		for _, apply in ipairs(appliers) do
			apply(t or 0.4)
		end
	end
end

function UI.restore(snap)
	UI.forEachControl(function(ctrl)
		local entry = snap[ctrl.id]
		if entry then
			ctrl.enabled = entry.enabled
			UI.forEachValue(ctrl, function(s)
				if entry.values[s.id] ~= nil then
					s.value = entry.values[s.id]
     if s.onSet then s.onSet(s.value)end
				end
			end)
		end
	end, true)
	applySettings()
	repaintAll()
	refreshNav()
	if refreshMini then
		refreshMini()
	end
end

-- three starting profiles, each a variation on the defaults
function UI.preset(name, edits)
	local snap = UI.snapshot()
	for ctrlId, e in pairs(edits) do
		for k, v in pairs(e) do
			if k == "enabled" then
				snap[ctrlId].enabled = v
			else
				snap[ctrlId].values[k] = v
			end
		end
	end
	return { name = name, snap = snap, savedAt = os.time() }
end
local PROFILES = {
	UI.preset("Default", {}),
	UI.preset("Custom", {}),
	UI.preset("Practice", {}),
}
local currentProfile = PROFILES[1]
-- Keep the object reference so renaming preserves the startup choice.
UI.autoLoadProfile = nil

function UI.profileNames()
	local names = {}
	for i, p in ipairs(PROFILES) do
		names[i] = p.name
	end
	return names
end

function UI.findProfile(name)
	for _, p in ipairs(PROFILES) do
		if string.lower(p.name) == string.lower(name) then
			return p
		end
	end
	return nil
end

function UI.uniqueName(name, except)
	local base, n, candidate = name, 2, name
	while true do
		local found = UI.findProfile(candidate)
		if not found or found == except then
			return candidate
		end
		candidate = base .. " (" .. n .. ")"
		n += 1
	end
end

function UI.ago(ts)
	local d = os.time() - (ts or os.time())
	if d < 10 then
		return "just now"
	elseif d < 60 then
		return d .. "s ago"
	elseif d < 3600 then
		return math.floor(d / 60) .. "m ago"
	end
	return math.floor(d / 3600) .. "h ago"
end

function UI.profileMeta(p)
	local n = 0
	for id, e in pairs(p.snap) do
		local ctrl = CTRL[id]
		if ctrl and ctrl.kind ~= "section" and e.enabled then
			n += 1
		end
	end
	local theme = p.snap.appearance and p.snap.appearance.values.theme or THEMES[1].name
	return string.format("%s accent  ·  %d feature%s on  ·  saved %s", theme, n, n == 1 and "" or "s", UI.ago(p.savedAt))
end

local configNameBox -- the "new profile" text box while the Configs page is open
local profileDD -- header dropdown, created below

function UI.loadProfile(p)
	currentProfile = p
	profileDD.set(p.name, 0.25)
	UI.restore(p.snap)
	setStatus(p.name .. " loaded")
	toast("Profile loaded", '"' .. p.name .. '" is now active', "folder")
	controlChanged:Fire("profile", "selected", p.name)
end

function UI.createConfig(raw)
	local name = UI.trim(raw or "")
	if name == "" then
		if configNameBox then
			UI.shake(configNameBox.Parent)
		end
		return
	end
	local p = { name = UI.uniqueName(name), snap = UI.snapshot(), savedAt = os.time() }
	table.insert(PROFILES, p)
	currentProfile = p
	profileDD.set(p.name, 0.25)
	setStatus('Profile "' .. p.name .. '" created')
	toast("Profile created", '"' .. p.name .. '" saved from current settings', "plus")
	controlChanged:Fire("profile", "created", p.name)
	renderPage("Configs")
end

local function caption(order, str)
	local _, body = UI.newSlot(24, order)
	UI.tracked(body, str, 8, 14, 12, C.muted, 1.6)
end

function UI.renderConfigs()
	local order = 0
	local function nextOrder()
		order += 1
		return order
	end

	-- Startup preference is separate from the profile currently being edited.
	local _, startupBody = UI.newSlot(108, nextOrder(), "startupProfile")
	local startupPanel = UI.make("Frame", {
		BackgroundColor3 = C.surface,
		Position = UDim2.fromOffset(2, 2),
		Size = UDim2.new(1, -4, 0, 104),
		Parent = startupBody,
	}, { UI.corner(14), UI.stroke(C.stroke, 1) })
	text(startupPanel, "Auto-load on execution", 18, 20, 16)
	local function startupLabel()
		return UI.autoLoadProfile and ("Config: " .. UI.autoLoadProfile.name) or "Last selected config"
	end
	local startupDD
	startupDD = dropdown(startupPanel, UDim2.fromOffset(18, 68), 340, 40, function()
		local options = { "Last selected config" }
		for _, p in ipairs(PROFILES) do options[#options + 1] = "Config: " .. p.name end
		return options
	end, startupLabel(), function(choice)
		local previous = UI.autoLoadProfile
		UI.autoLoadProfile = nil
		for _, p in ipairs(PROFILES) do
			if choice == "Config: " .. p.name then UI.autoLoadProfile = p; break end
		end
		if not UI.persistProfiles() then
			UI.autoLoadProfile = previous
			startupDD.set(startupLabel(), 0)
			return
		end
		setStatus(UI.autoLoadProfile and (UI.autoLoadProfile.name .. " will auto-load on execution") or "Startup will use the last selected config")
	end)

	-- new profile
	if layoutState.captions then
		caption(nextOrder(), "NEW PROFILE")
	end
	local _, newBody = UI.newSlot(78, nextOrder(), "newProfile")
	local newPanel = UI.make("Frame", {
		BackgroundColor3 = C.surface,
		Position = UDim2.fromOffset(2, 2),
		Size = UDim2.new(1, -4, 0, 78),
		Parent = newBody,
	}, { UI.corner(14), UI.stroke(C.stroke, 1) })
	local create = UI.primaryButton(newPanel, "Create from current", "plus", 44)
	create.AnchorPoint = Vector2.new(1, 0.5)
	create.Position = UDim2.new(1, -18, 0.5, 0)
	local fieldStroke = UI.stroke(C.stroke, 1)
	local field = UI.make("Frame", {
		BackgroundColor3 = C.field,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = UDim2.new(0, 18, 0.5, 0),
		Size = UDim2.new(1, -18 - 12 - 18 - create.Size.X.Offset, 0, 44),
		Parent = newPanel,
	}, { UI.corner(10), fieldStroke })
	local fieldIcon = UI.icon(field, "pencil", 22, 22, 16, C.sub, 1.7)
	local box = UI.make("TextBox", {
		BackgroundTransparency = 1,
		Position = UDim2.fromOffset(42, 0),
		Size = UDim2.new(1, -56, 1, 0),
		Text = "",
		PlaceholderText = "Name a new profile",
		PlaceholderColor3 = C.muted,
		TextColor3 = C.text,
		FontFace = UI.font(),
		TextSize = 16,
		TextXAlignment = Enum.TextXAlignment.Left,
		ClearTextOnFocus = false,
		Parent = field,
	})
	configNameBox = box
	box.Focused:Connect(function()
		UI.tween(fieldStroke, 0.2, { Color = A.a })
		UI.tintIcon(fieldIcon, A.hi, 0.2)
	end)
	box.FocusLost:Connect(function(enter)
		UI.tween(fieldStroke, 0.3, { Color = C.stroke })
		UI.tintIcon(fieldIcon, C.sub, 0.3)
		if enter then
			UI.createConfig(box.Text)
		end
	end)
	create.MouseButton1Click:Connect(function()
		UI.createConfig(box.Text)
	end)

	if layoutState.captions then
		caption(nextOrder(), "SAVED PROFILES  ·  " .. #PROFILES)
	end

	for _, p in ipairs(PROFILES) do
		local isCurrent = p == currentProfile
		local slot, body = UI.newSlot(80, nextOrder(), "profile")
		local row = UI.make("Frame", {
			BackgroundColor3 = C.surface,
			Position = UDim2.fromOffset(2, 2),
			Size = UDim2.new(1, -4, 0, 80),
			Parent = body,
		}, { UI.corner(14), UI.stroke(isCurrent and A.soft or C.stroke, 1) })
		if isCurrent then
			UI.make("Frame", {
				BackgroundColor3 = C.white,
				BackgroundTransparency = 0.15,
				BorderSizePixel = 0,
				Position = UDim2.fromOffset(28, 0),
				Size = UDim2.new(1, -56, 0, 1),
				Parent = row,
			}, { UI.accentGradient(0, EDGE_FADE) })
		end

		-- initial tile
		local tile = UI.make("Frame", {
			BackgroundColor3 = isCurrent and C.white or C.field,
			AnchorPoint = Vector2.new(0, 0.5),
			Position = UDim2.new(0, 18, 0.5, 0),
			Size = UDim2.fromOffset(44, 44),
			Parent = row,
		}, { UI.corner(12) })
		if isCurrent then
			UI.accentGradient(45).Parent = tile
		end
		UI.make("TextLabel", {
			BackgroundTransparency = 1,
			Size = UDim2.fromScale(1, 1),
			Text = string.upper(string.sub(p.name, 1, 1)),
			FontFace = UI.font(BOLD),
			TextSize = 19,
			TextColor3 = isCurrent and C.white or C.text2,
			Parent = tile,
		})

		-- name is an editable box styled like a label; the pencil focuses it
		local nameStroke = UI.stroke(C.stroke, 1, 1)
		local nameFrame = UI.make("Frame", {
			BackgroundColor3 = C.field,
			BackgroundTransparency = 1,
			AnchorPoint = Vector2.new(0, 0.5),
			Position = UDim2.fromOffset(68, 28),
			Size = UDim2.fromOffset(280, 32),
			Parent = row,
		}, { UI.corner(8), nameStroke })
		local nameBox = UI.make("TextBox", {
			BackgroundTransparency = 1,
			Position = UDim2.fromOffset(8, 0),
			Size = UDim2.new(1, -16, 1, 0),
			Text = p.name,
			TextColor3 = C.text,
			FontFace = UI.font(SEMIBOLD),
			TextSize = 18,
			TextXAlignment = Enum.TextXAlignment.Left,
			TextTruncate = Enum.TextTruncate.AtEnd,
			ClearTextOnFocus = false,
			Parent = nameFrame,
		})
		UI.clip(text(row, UI.profileMeta(p), 76, 54, 14, { color = C.sub }), UDim.new(1, -400))
		if isCurrent then
			local bx = 76 + math.min(UI.textWidth(p.name, 18), 264) + 12
			local badge = UI.make("Frame", {
				AnchorPoint = Vector2.new(0, 0.5),
				Position = UDim2.fromOffset(bx, 28),
				Size = UDim2.fromOffset(62, 22),
				Parent = row,
			}, { UI.round() })
			UI.bindTheme(badge, "BackgroundColor3", "dim")
			UI.bindTheme(UI.stroke(A.soft, 1), "Color", "soft").Parent = badge
			UI.bindTheme(UI.make("TextLabel", {
				BackgroundTransparency = 1,
				Size = UDim2.fromScale(1, 1),
				Text = "ACTIVE",
				FontFace = UI.font(BOLD),
				TextSize = 11,
				Parent = badge,
			}), "TextColor3", "hi")
		end
		nameBox.Focused:Connect(function()
			UI.tween(nameStroke, 0.2, { Transparency = 0, Color = A.a })
			UI.tween(nameFrame, 0.2, { BackgroundTransparency = 0 })
		end)
		nameBox.FocusLost:Connect(function()
			UI.tween(nameStroke, 0.25, { Transparency = 1 })
			UI.tween(nameFrame, 0.25, { BackgroundTransparency = 1 })
			local new = UI.trim(nameBox.Text)
			if new == "" or new == p.name then
				nameBox.Text = p.name
				return
			end
			local old = p.name
			p.name = UI.uniqueName(new, p)
			nameBox.Text = p.name
			if p == currentProfile then
				profileDD.set(p.name, 0.25)
			end
			setStatus('Renamed to "' .. p.name .. '"')
			controlChanged:Fire("profile", "renamed", old .. " -> " .. p.name)
			renderPage("Configs", false)
		end)

		-- actions
		local actions = UI.make("Frame", {
			BackgroundTransparency = 1,
			AutomaticSize = Enum.AutomaticSize.X,
			AnchorPoint = Vector2.new(1, 0.5),
			Position = UDim2.new(1, -18, 0.5, 0),
			Size = UDim2.fromOffset(0, 40),
			Parent = row,
		})
		UI.make("UIListLayout", {
			FillDirection = Enum.FillDirection.Horizontal,
			Padding = UDim.new(0, 8),
			SortOrder = Enum.SortOrder.LayoutOrder,
			VerticalAlignment = Enum.VerticalAlignment.Center,
			Parent = actions,
		})
		local loadB, loadLabel = UI.ghostButton(actions, isCurrent and "Loaded" or "Load", isCurrent and "check" or "play", 40)
		loadB.LayoutOrder = 1
		if isCurrent then
			loadLabel.TextColor3 = C.sub
		end
		local function small(order2, iconName, tip, danger)
			local b = UI.iconButton(actions, UDim2.new(), 40, iconName, {
				solid = true,
				tip = tip,
				hoverColor = danger and C.dangerFill or nil,
				color = danger and C.danger or nil,
				iconSize = 17,
			})
			b.AnchorPoint = Vector2.zero
			b.LayoutOrder = order2
			return b
		end
		local saveB = small(2, "save", "Save current settings here")
		local renameB = small(3, "pencil", "Rename")
		local deleteB = small(4, "trash", #PROFILES > 1 and "Delete" or "Can't delete the last profile", true)

		loadB.MouseButton1Click:Connect(function()
			if p ~= currentProfile then
				UI.loadProfile(p)
				renderPage("Configs", false)
			end
		end)
		saveB.MouseButton1Click:Connect(function()
			p.snap = UI.snapshot()
			p.savedAt = os.time()
			setStatus('Saved current settings to "' .. p.name .. '"')
			toast("Profile saved", '"' .. p.name .. '" updated', "save")
			controlChanged:Fire("profile", "saved", p.name)
			renderPage("Configs", false)
		end)
		renameB.MouseButton1Click:Connect(function()
			nameBox:CaptureFocus()
			nameBox.SelectionStart = 1
			nameBox.CursorPosition = #nameBox.Text + 1
		end)
		deleteB.MouseButton1Click:Connect(function()
			if #PROFILES <= 1 then
				UI.shake(row)
				return
			end
			local idx = table.find(PROFILES, p)
			if idx then
				table.remove(PROFILES, idx)
                if UI.autoLoadProfile == p then UI.autoLoadProfile = nil end
                if currentProfile == p then
                    currentProfile = PROFILES[1]
                    profileDD.set(currentProfile.name, 0)
                    UI.restore(currentProfile.snap)
                end
			end
			controlChanged:Fire("profile", "deleted", p.name)
			setStatus('Deleted "' .. p.name .. '"')
			toast("Profile deleted", '"' .. p.name .. '" was removed', "trash")
			-- collapse the row away, then redraw
			UI.groupFade(body, 1, 0.2)
			UI.tween(slot, 0.3, { Size = UDim2.new(1, 0, 0, 0) }).Completed:Connect(function()

				if currentPage == "Configs" and renderedQuery == "" then
					renderPage("Configs", false)
				end
			end)
		end)
	end
end

-------------------------------------------------------------------- live stats (overview)
local stats = { fps = 60, ping = 0, fpsHist = {}, pingHist = {}, started = os.clock() }
do
	local frames, acc = 0, 0
	UI.connect(RunService.RenderStepped, function(dt)
		frames += 1
		acc += dt
		if acc >= 0.5 then
			stats.fps = frames / acc
			frames, acc = 0, 0
			local ok, p = pcall(function()
				return player:GetNetworkPing()
			end)
			stats.ping = ok and (p or 0) * 1000 or 0
			table.insert(stats.fpsHist, stats.fps)
			table.insert(stats.pingHist, stats.ping)
			if #stats.fpsHist > 30 then
				table.remove(stats.fpsHist, 1)
				table.remove(stats.pingHist, 1)
			end
			if statsListener then
				statsListener()
			end
		end
	end)
end

function UI.maxOf(t, floor)
	local m = floor
	for _, v in ipairs(t) do
		m = math.max(m, v)
	end
	return m
end

function UI.statTile(parent, label, iconName, order)
	local tile = UI.make("Frame", {
		BackgroundColor3 = C.surface,
		Size = UDim2.new(0.25, -10.5, 1, -4),
		LayoutOrder = order,
		Parent = parent,
	}, { UI.corner(14), UI.stroke(C.stroke, 1) })
	text(tile, string.upper(label), 18, 24, 12, { weight = BOLD, color = C.muted })
	UI.icon(tile, iconName, UDim2.new(1, -26, 0, 24), nil, 17, C.muted, 1.7)
	local value = text(tile, "–", 18, 58, 30, { weight = BOLD, rich = true })
	return tile, value
end

-- bottom-aligned bar chart; older bars fade out
function UI.sparkline(parent, n)
	local area = UI.make("Frame", {
		BackgroundTransparency = 1,
		AnchorPoint = Vector2.new(0, 1),
		Position = UDim2.new(0, 18, 1, -16),
		Size = UDim2.new(1, -36, 0, 28),
		Parent = parent,
	})
	local bars = {}
	for i = 1, n do
		bars[i] = UI.make("Frame", {
			BackgroundColor3 = A.a,
			BackgroundTransparency = 0.1 + 0.7 * (1 - i / n),
			BorderSizePixel = 0,
			AnchorPoint = Vector2.new(0, 1),
			Position = UDim2.new((i - 1) / n, 0, 1, 0),
			Size = UDim2.new(1 / n, -3, 0, 2),
			Parent = area,
		}, { UI.corner(2) })
	end
	return function(hist, hi)
		for i = 1, n do
			local v = hist[#hist - n + i]
			local k = v and math.clamp(v / math.max(hi, 1e-3), 0.06, 1) or 0.06
			UI.tween(bars[i], 0.35, { Size = UDim2.new(1 / n, -3, 0, math.max(2, k * 28)), BackgroundColor3 = A.a })
		end
	end
end

function UI.greeting()
	local hour = os.date("*t").hour
	if hour < 12 then
		return "Good morning"
	elseif hour < 18 then
		return "Good afternoon"
	end
	return "Good evening"
end

function UI.renderOverview(page)
	local order = 0
	local function nextOrder()
		order += 1
		return order
	end

	-- hero banner
	local _, heroBody = UI.newSlot(176, nextOrder(), "hero")
	local hero = UI.make("Frame", {
		BackgroundColor3 = C.surface,
		Position = UDim2.fromOffset(2, 2),
		Size = UDim2.new(1, -4, 1, -4),
		Parent = heroBody,
	}, { UI.corner(16), UI.bindTheme(UI.stroke(A.soft, 1), "Color", "soft") })
	local art = UI.make("CanvasGroup", {
		BackgroundTransparency = 1,
		Size = UDim2.fromScale(1, 1),
		Parent = hero,
	}, { UI.corner(16) })
	UI.make("Frame", {
		BackgroundColor3 = C.white,
		BorderSizePixel = 0,
		Size = UDim2.fromScale(1, 1),
		Parent = art,
	}, {
		UI.accentGradient(0, NumberSequence.new({
			NumberSequenceKeypoint.new(0, 0.68),
			NumberSequenceKeypoint.new(0.55, 0.9),
			NumberSequenceKeypoint.new(1, 0.78),
		})),
	})
	for i, d in ipairs({ 150, 250, 350, 450 }) do
		local ring = UI.make("Frame", {
			BackgroundTransparency = 1,
			AnchorPoint = Vector2.new(0.5, 0.5),
			Position = UDim2.new(1, -110, 0.5, 0),
			Size = UDim2.fromOffset(d, d),
			Parent = art,
		}, { UI.round() })
		UI.bindTheme(UI.stroke(A.hi, 1, 0.5 + i * 0.1), "Color", "hi").Parent = ring
	end
	local orbiters = {}
	for i, spec in ipairs({ { r = 125, size = 10, speed = 0.55 }, { r = 175, size = 7, speed = -0.35 }, { r = 75, size = 6, speed = 0.9 } }) do
		local dot = UI.make("Frame", {
			BackgroundColor3 = C.white,
			AnchorPoint = Vector2.new(0.5, 0.5),
			Size = UDim2.fromOffset(spec.size, spec.size),
			Parent = art,
		}, { UI.round() })
		orbiters[i] = { dot = dot, r = spec.r, speed = spec.speed, phase = i * 2.2 }
	end
	local t0 = os.clock()
	table.insert(rowConns, RunService.RenderStepped:Connect(function()
		local t = os.clock() - t0
		for _, o in ipairs(orbiters) do
			local a = o.phase + t * o.speed
			o.dot.Position = UDim2.new(1, -110 + math.cos(a) * o.r, 0.5, math.sin(a) * o.r)
		end
	end))
	-- hero logo in the middle of the rings
	local heroMark = UI.make("Frame", {
		BackgroundColor3 = C.white,
		AnchorPoint = Vector2.new(0.5, 0.5),
		Position = UDim2.new(1, -110, 0.5, 0),
		Size = UDim2.fromOffset(52, 52),
		Parent = hero,
	}, { UI.corner(16), UI.accentGradient(45), UI.bindTheme(UI.stroke(A.hi, 1, 0.3), "Color", "hi") })
	UI.icon(heroMark, "logo", 26, 26, 28, C.white, 2)

	text(hero, UI.greeting() .. ", " .. player.DisplayName, 30, 46, 30, { weight = BOLD })
	UI.make("TextLabel", {
		BackgroundTransparency = 1,
		Position = UDim2.fromOffset(30, 66),
		Size = UDim2.new(1, -320, 0, 44),
		Text = "Every control in this window changes the window itself. Try a new accent, a motion curve or a denser layout.",
		TextWrapped = true,
		TextXAlignment = Enum.TextXAlignment.Left,
		TextYAlignment = Enum.TextYAlignment.Top,
		FontFace = UI.font(),
		TextSize = 16,
		TextColor3 = C.text2,
		Parent = hero,
	})
	local tryBtn = UI.primaryButton(hero, "Preview notification", "bell", 40)
	tryBtn.AnchorPoint = Vector2.new(0, 0.5)
	tryBtn.Position = UDim2.fromOffset(30, 136)
	tryBtn.MouseButton1Click:Connect(function()
		toast("Hello there", "This is how a notification looks.", "bell", true)
	end)
	local themeBtn = UI.ghostButton(hero, "Customize theme", "sparkle", 40)
	themeBtn.AnchorPoint = Vector2.new(0, 0.5)
	themeBtn.Position = UDim2.fromOffset(30 + tryBtn.Size.X.Offset + 10, 136)
	themeBtn.MouseButton1Click:Connect(function()
		currentTab.Workspace = "Appearance"
		openPage("Workspace", true)
	end)

	-- live stat tiles
	if layoutState.captions then
		caption(nextOrder(), "LIVE")
	end
	local _, statsBody = UI.newSlot(128, nextOrder(), "stats")
	local row = UI.make("Frame", { BackgroundTransparency = 1, Position = UDim2.fromOffset(2, 2), Size = UDim2.new(1, -4, 1, 0), Parent = statsBody })
	UI.make("UIListLayout", {
		FillDirection = Enum.FillDirection.Horizontal,
		Padding = UDim.new(0, 14),
		SortOrder = Enum.SortOrder.LayoutOrder,
		Parent = row,
	})
	local fpsTile, fpsValue = UI.statTile(row, "Frame rate", "bolt", 1)
	local fpsSpark = UI.sparkline(fpsTile, 22)
	local pingTile, pingValue = UI.statTile(row, "Latency", "signal", 2)
	local pingSpark = UI.sparkline(pingTile, 22)
	local upTile, upValue = UI.statTile(row, "Session", "clock", 3)
	text(upTile, "since this window opened", 18, 96, 14, { color = C.sub })
	local featTile, featValue = UI.statTile(row, "Features", "layers", 4)
	local featTrack = UI.make("Frame", {
		BackgroundColor3 = C.field,
		BorderSizePixel = 0,
		Position = UDim2.new(0, 18, 1, -26),
		Size = UDim2.new(1, -36, 0, 6),
		Parent = featTile,
	}, { UI.round() })
	local featFill = UI.make("Frame", {
		BackgroundColor3 = C.white,
		BorderSizePixel = 0,
		Size = UDim2.fromScale(0, 1),
		Parent = featTrack,
	}, { UI.round(), UI.accentGradient(0) })

	local unit = '<font size="15" color="#848DA2"> %s</font>'
	statsListener = function()
		fpsValue.Text = math.round(stats.fps) .. string.format(unit, "fps")
		pingValue.Text = math.round(stats.ping) .. string.format(unit, "ms")
		local up = math.floor(os.clock() - stats.started)
		upValue.Text = string.format("%d:%02d", math.floor(up / 60), up % 60)
		local on, total = UI.featureCounts()
		featValue.Text = on .. string.format(unit, "of " .. total)
		UI.tween(featFill, 0.4, { Size = UDim2.fromScale(total > 0 and on / total or 0, 1) })
		fpsSpark(stats.fpsHist, UI.maxOf(stats.fpsHist, 60))
		pingSpark(stats.pingHist, UI.maxOf(stats.pingHist, 80))
	end
	statsListener()

	-- the page's own feature cards
	if layoutState.captions then
		caption(nextOrder(), "FEATURES")
	end
	for _, ctrl in ipairs(page.controls) do
		UI.buildControl(ctrl, nextOrder())
	end
end

-------------------------------------------------------------------- pages + search
function UI.clearList()
	closePopup()
	for _, conn in ipairs(rowConns) do
		conn:Disconnect()
	end
	table.clear(rowConns)
	table.clear(rowAppliers)
	if rebinding then
		rebinding = nil
	end
	configNameBox = nil
	statsListener = nil
	playMotionPreview = nil
	for _, child in ipairs(list:GetChildren()) do
		if child:IsA("GuiObject") then
			child:Destroy()
		end
	end
end

function UI.animateEntrance()
	local kids = {}
	for _, c in ipairs(list:GetChildren()) do
		if c:IsA("GuiObject") then
			table.insert(kids, c)
		end
	end
	table.sort(kids, function(a, b)
		return a.LayoutOrder < b.LayoutOrder
	end)
	for i, slot in ipairs(kids) do
		local body = slot:FindFirstChild("Body")
		if body then
			local d = math.min((i - 1) * 0.045, 0.27)
			body.Position = UDim2.fromOffset(0, 16)
			UI.groupFade(body, 0, 0.3, d)
			UI.tween(body, 0.45, { Position = UDim2.fromOffset(0, 0) }, QUINT, OUT, d)
		end
	end
end

function UI.setHeader(crumbText, title, subtitle, animate)
	crumb.Text = crumbText
	pageTitle.Text = title
	pageSub.Text = subtitle
	if animate then
		crumb.Position = UDim2.fromOffset(CONTENT_X, TITLE_H + 32)
		pageTitle.Position = UDim2.fromOffset(CONTENT_X, TITLE_H + 64)
		pageSub.Position = UDim2.fromOffset(CONTENT_X, TITLE_H + 96)
		UI.riseIn(crumb, 0)
		UI.riseIn(pageTitle, 0.03)
		UI.riseIn(pageSub, 0.06)
	end
end

local CRUMB_SEP = '  <font color="#586074">/</font>  '

renderPage = function(name, animate)
	if animate == nil then
		animate = true
	end
	local page = PAGE_BY_NAME[name]
	local scroll = list.CanvasPosition
	UI.clearList()
	renderedQuery = ""
	if tabsFor ~= (page.tabs and name or nil) then
		UI.buildTabs(page)
	end
	UI.setListTop(page.tabs and LIST_TOP_TABS or LIST_TOP, animate and 0.3 or 0)

	local tab = page.tabs and currentTab[name]
	UI.setHeader("practical" .. CRUMB_SEP .. name .. (tab and (CRUMB_SEP .. tab) or ""), name, page.subtitle or "", animate)

	local count = 0
	if page.custom == "configs" then
		UI.renderConfigs()
		count = 1
	elseif page.custom == "overview" then
		UI.renderOverview(page)
		count = 1
	else
		local order = 0
		for _, ctrl in ipairs(page.controls) do
			if not page.tabs or ctrl.tab == tab then
				order += 1
				UI.buildControl(ctrl, order)
			end
		end
		count = order
	end
	emptyLabel.Visible = count == 0
	emptyLabel.Text = "Nothing here yet"
	if animate then
		list.CanvasPosition = Vector2.zero
		UI.animateEntrance()
	else
		list.CanvasPosition = scroll
	end
end

-- text a control can be found by: its name, page, tab, and every label / option
function UI.searchText(ctrl, page)
	local parts = { ctrl.name, ctrl.desc or "", page.name, ctrl.tab or "" }
	for _, r in ipairs(ctrl.rows or {}) do
		table.insert(parts, r.label or "")
		table.insert(parts, r.desc or "")
		for _, it in ipairs(r.items or {}) do
			table.insert(parts, it.label or "")
		end
		for _, opt in ipairs(r.options or {}) do
			table.insert(parts, opt)
		end
	end
	return string.lower(table.concat(parts, " "))
end

function UI.renderSearch(q, animate)
	local scroll = list.CanvasPosition
	UI.clearList()
	renderedQuery = q
	UI.buildTabs(nil)
	UI.setListTop(LIST_TOP, 0.25)
	local order, count = 0, 0
	for _, page in ipairs(PAGES) do
		if page.custom ~= "configs" then
			local found = {}
			for _, ctrl in ipairs(page.controls) do
				if string.find(UI.searchText(ctrl, page), q, 1, true) then
					table.insert(found, ctrl)
				end
			end
			if #found > 0 then
				order += 1
				caption(order, string.upper(page.name))
				for _, ctrl in ipairs(found) do
					order += 1
					count += 1
					UI.buildControl(ctrl, order)
				end
			end
		end
	end
	UI.setHeader(
		"practical" .. CRUMB_SEP .. "Search",
		"Search",
		count .. (count == 1 and " result" or " results") .. " for “" .. q .. "”",
		animate
	)
	emptyLabel.Visible = count == 0
	emptyLabel.Text = "No settings match that search"
	if animate then
		UI.animateEntrance()
	else
		list.CanvasPosition = scroll
	end
end

-- redraw in place (keeps scroll), e.g. after density changes
local rerenderToken = 0
rerender = function(delay)
	rerenderToken += 1
	local mine = rerenderToken
	UI.after(delay or 0, function()
		if mine ~= rerenderToken or not currentPage then
			return
		end
		if renderedQuery ~= "" then
			UI.renderSearch(renderedQuery, false)
		else
			renderPage(currentPage, false)
		end
	end)
end

local searchToken = 0
searchBox:GetPropertyChangedSignal("Text"):Connect(function()
	local q = string.lower(UI.trim(searchBox.Text))
	local has = searchBox.Text ~= ""
	clearSearch.Visible = has
	kbd.Visible = not has
	searchToken += 1
	local mine = searchToken
	UI.after(0.14, function()
		if mine ~= searchToken or q == renderedQuery then
			return
		end
		if q == "" then
			renderPage(currentPage)
		else
			UI.renderSearch(q, true)
		end
	end)
end)

openPage = function(name, force)
	local page = PAGE_BY_NAME[name]
	if not page or (name == currentPage and renderedQuery == "" and not force) then
		return
	end
	currentPage = name
	if searchBox.Text ~= "" then
		renderedQuery = "" -- the cleared box won't trigger a second render
		searchBox.Text = ""
	end
	navHighlight.Parent=nav[name].btn.Parent
	UI.tween(navHighlight, 0.45, { Position = nav[name].pos })
	refreshNav(0.3)
	renderPage(name)
end

-------------------------------------------------------------------- settings → interface
local borderOn = false
function UI.setBorder(on)
	borderOn = on
	borderGrad.Enabled = on
	winStroke.Color = on and C.white or C.stroke
	winStroke.Thickness = on and 1.5 or 1.2
end

function UI.setAmbient(on)
	if on == ambientOn then
		return
	end
	ambientOn = on
	UI.tween(ambient, 0.5, { GroupTransparency = on and 0 or 1 })
end

function UI.setTheme(name, t)
	UI.deriveTheme(THEME_BY_NAME[name] or THEMES[1])
	UI.applyThemeRegistry(t or 0.45)
	repaintAll(0.45)
	refreshNav(0.3)
	if refreshMini then
		refreshMini()
	end
end

applySettings = function(t)
	t = t or 0.35
	local motion = CTRL.motion
	local m = motion.enabled == true
	MOTION.factor = m and SETTING.motion.speed.value / 0.3 or 1
	MOTION.style = m and EASINGS[SETTING.motion.curve.value] or QUINT
	MOTION.reduce = m and SETTING.motion.reduce.value or false

	local ap = CTRL.appearance.enabled == true
	local themeName = ap and SETTING.appearance.theme.value or THEMES[1].name
	if themeName ~= A.name then
		UI.setTheme(themeName, t)
	end
	local alpha = 1 - (ap and SETTING.appearance.opacity.value / 100 or 1)
	UI.tween(win, t, { BackgroundTransparency = alpha })
	UI.tween(rail, t, { BackgroundTransparency = alpha * 0.6 })
	for _, f in ipairs(railFillers) do
		UI.tween(f, t, { BackgroundTransparency = alpha * 0.6 })
	end
	UI.setBorder(ap and SETTING.appearance.border.value or false)
	UI.setAmbient(ap and SETTING.appearance.ambient.value or false)

	local lay = CTRL.layout.enabled == true
	UI.tween(listLayout, t, { Padding = UDim.new(0, lay and SETTING.layout.spacing.value or 14) })
	local density = lay and DENSITY[SETTING.layout.density.value] or 1
	local captions = not lay or SETTING.layout.labels.value
	local dividers = not lay or SETTING.layout.dividers.value
	local relayout = density ~= layoutState.density or captions ~= layoutState.captions or dividers ~= layoutState.dividers
	layoutState.density, layoutState.captions, layoutState.dividers = density, captions, dividers
	if relayout then
		rerender(0.32) -- let the clicked control finish its own animation first
	end
end

function UI.describe(ctrlId, id, value)
	local ctrl = CTRL[ctrlId]
	if not ctrl then
		return nil
	end
	if id == "enabled" then
		return ctrl.name .. (value and " on" or " off")
	end
	local s = SETTING[ctrlId][id]
	if not s then
		return nil
	end
	local v
	if typeof(value) == "number" then
		v = string.format(s.format or "%.1f", value) .. (s.suffix or "")
	elseif typeof(value) == "boolean" then
		v = value and "on" or "off"
	elseif typeof(value) == "EnumItem" then
		v = value.Name
	else
		v = tostring(value)
	end
	return (s.label or id) .. "  ·  " .. v
end

local fit, centerWindow -- forward (window placement)

local ACTIONS = {
	preview = function()
		toast("Notifications", "This is how a notification looks.", "bell", true)
	end,
	resetLayout = function()
		local ctrl = CTRL.layout
		UI.forEachValue(ctrl, function(s)
			s.value = s.default
		end)
		applySettings()
		repaintAll()
		setStatus("Layout reset")
		toast("Layout reset", "Spacing and density are back to default", "refresh")
	end,
	refresh = function()
		renderPage(currentPage)
		setStatus("Page refreshed")
	end,
	burst = function()
		local msgs = {
			{ "Heads up", "Toasts stack from the corner you pick", "bell" },
			{ "Nice", "Click any toast to dismiss it early", "check" },
			{ "One more", "Turn off stacking to keep just one", "layers" },
		}
		for i, m in ipairs(msgs) do
			UI.after((i - 1) * 0.22, function()
				toast(m[1], m[2], m[3], true)
			end)
		end
	end,
	center = function()
		centerWindow()
		setStatus("Window centered")
	end,
}

do (function()
 local path="PracticalUI2Profiles-v1.json"
 local http=game:GetService("HttpService")
 local function encode(v)
  if typeof(v)=="EnumItem"then return {keyCode=v.Name,mouse=v.EnumType==Enum.UserInputType}end
  if type(v)=="table"then local r={}for k,x in pairs(v)do r[k]=encode(x)end return r end
  return v
 end
 local function validated(raw)
  if type(raw)~="table"then return nil end
  local result=UI.snapshot()
  UI.forEachControl(function(ctrl)
   local e=raw[ctrl.id];if type(e)~="table"then return end
   if type(e.enabled)=="boolean"then result[ctrl.id].enabled=e.enabled end
   if type(e.values)~="table"then return end
   UI.forEachValue(ctrl,function(s)
    local v=e.values[s.id]
    if typeof(s.value)=="EnumItem"then
     if type(v)=="table"and type(v.keyCode)=="string"then
      local ok,key=pcall(function()return (v.mouse and Enum.UserInputType or Enum.KeyCode)[v.keyCode]end)
      local mouse=ok and key and(key==Enum.UserInputType.MouseButton1 or key==Enum.UserInputType.MouseButton2 or key==Enum.UserInputType.MouseButton3)
      if ok and key and key~=Enum.KeyCode.Unknown and(not v.mouse or mouse)then result[ctrl.id].values[s.id]=key end
     end
    elseif type(s.value)=="number"then
     if type(v)=="number"and v==v and math.abs(v)<math.huge then result[ctrl.id].values[s.id]=math.clamp(v,s.min or -math.huge,s.max or math.huge)end
    elseif type(v)==type(s.value)and (not s.options or table.find(s.options,v))and (type(v)~="string"or #v<=2048)then result[ctrl.id].values[s.id]=v end
   end)
  end,true)
  return result
 end
 UI.persistProfiles=function()
  if type(writefile)~="function"then setStatus("File saving unavailable in this environment");return false end
  local ok,err=pcall(function()writefile(path,http:JSONEncode({version=1,uiSizingVersion=7,selected=currentProfile.name,autoLoad=UI.autoLoadProfile and UI.autoLoadProfile.name or false,profiles=encode(PROFILES)}))end)
  if not ok then setStatus("Config could not be written");warn("Practical config save failed: "..tostring(err))end
  return ok
 end
 UI.loadSavedProfiles=function()
  if type(readfile)~="function"then return false end
  local ok,raw=pcall(function()return http:JSONDecode(readfile(path))end)
  if not ok or type(raw)~="table"or raw.version~=1 or type(raw.profiles)~="table"then return false end
  local profiles,seen={},{}
  for _,e in ipairs(raw.profiles)do
   if #profiles>=100 then break end
   if type(e)=="table"and type(e.name)=="string"and #e.name>0 and #e.name<=80 and not seen[string.lower(e.name)]then
    local snap=validated(e.snap)
    -- Every execution starts at 1.0, including the selected auto-load config.
    if snap and snap.interface and snap.interface.values then
     snap.interface.values.uiScale=DEFAULT_SCALE
    end
    if snap then table.insert(profiles,{name=e.name,snap=snap,savedAt=type(e.savedAt)=="number"and e.savedAt or os.time()});seen[string.lower(e.name)]=true end
   end
  end
  if #profiles==0 then return false end
  PROFILES=profiles
  UI.autoLoadProfile=type(raw.autoLoad)=="string" and UI.findProfile(raw.autoLoad) or nil
  currentProfile=UI.autoLoadProfile or UI.findProfile(type(raw.selected)=="string"and raw.selected or "")or profiles[1]
  return true
 end
 if UI.loadSavedProfiles() then UI.persistProfiles() end
end)()end

-------------------------------------------------------------------- header profile picker + footer actions
profileDD = dropdown(win, UDim2.new(1, -32 - 230, 0, TITLE_H + 64), 230, 44, UI.profileNames, currentProfile.name, function(name)
	local p = UI.findProfile(name)
	if p then
		UI.loadProfile(p)
		if currentPage == "Configs" and renderedQuery == "" then
			renderPage("Configs", false)
		end
	end
end, {
	label = "New profile…",
	icon = "plus",
	onClick = function()
		openPage("Configs")
		UI.after(0.1, function()
			if configNameBox then
				configNameBox:CaptureFocus()
			end
		end)
	end,
})

local saveToken = 0
saveBtn.MouseButton1Click:Connect(function()
	currentProfile.snap = UI.snapshot()
	currentProfile.savedAt = os.time()
	controlChanged:Fire("profile", "saved", currentProfile.name)
	setStatus('"' .. currentProfile.name .. '" saved')
	toast("Profile saved", '"' .. currentProfile.name .. '" is up to date', "save")
	saveToken += 1
	local mine = saveToken
	saveLabel.Text = "Saved"
	UI.after(1.4, function()
		if saveToken == mine then
			saveLabel.TextTransparency = 1
			saveLabel.Text = "Save profile"
			UI.tween(saveLabel, 0.3, { TextTransparency = 0 })
		end
	end)
	if currentPage == "Configs" and renderedQuery == "" then
		renderPage("Configs", false)
	end
end)

local revertAngle = 0
revertBtn.MouseButton1Click:Connect(function()
	revertAngle += 360
	UI.spinIcon(revertIcon, "refresh", C.white, 1.8, revertAngle, 0.7)
	UI.after(0.75, function()
		UI.tintIcon(revertIcon, C.text2, 0.2)
	end)
	UI.restore(currentProfile.snap)
	controlChanged:Fire("profile", "reverted", currentProfile.name)
	setStatus("Reverted to saved")
end)

resetBtn.MouseButton1Click:Connect(function()
	UI.forEachControl(function(ctrl)
		ctrl.enabled = ctrl.defaultEnabled
		UI.forEachValue(ctrl, function(s)
			s.value = s.default
		end)
	end)
	applySettings()
	repaintAll()
	refreshNav()
	setStatus("Defaults restored")
	toast("Defaults restored", "Every feature is back to its starting value", "refresh")
	controlChanged:Fire("profile", "reset", currentProfile.name)
end)

testBtn.MouseButton1Click:Connect(function()
	testControls:Fire()
end)

-------------------------------------------------------------------- minimized bar + hidden notice
local MINI_W, MINI_H, MM = 460, 60, 16
local mini = UI.make("Frame", {
	Name = "Mini",
	BackgroundTransparency = 1,
	AnchorPoint = Vector2.new(0.5, 0),
	Position = UDim2.new(0.5, 0, 0, 8),
	Size = UDim2.fromOffset(MINI_W + MM * 2, MINI_H + MM * 2),
	Visible = false,
	Parent = gui,
})
local miniScale = UI.make("UIScale", { Scale = DEFAULT_SCALE, Parent = mini })
local miniShade = UI.make("Frame", {
	BackgroundTransparency = 1,
	Position = UDim2.fromOffset(MM, MM),
	Size = UDim2.fromOffset(MINI_W, MINI_H),
	Parent = mini,
})
UI.softShadow(miniShade, MINI_H / 2, 4, 3, 0.08, 4)
local miniBody = UI.make("TextButton", {
	Text = "",
	AutoButtonColor = false,
	BackgroundColor3 = C.bg,
	BackgroundTransparency = 0.03,
	Position = UDim2.fromOffset(MM, MM),
	Size = UDim2.fromOffset(MINI_W, MINI_H),
	ZIndex = 2,
	Parent = mini,
}, { UI.round(), UI.bindTheme(UI.stroke(A.soft, 1), "Color", "soft") })
UI.logoMark(miniBody, 32, MINI_H / 2, 30)
text(miniBody, "practical", 58, MINI_H / 2, 18, { weight = SEMIBOLD })
UI.strip(miniBody, UDim2.fromOffset(152, 18), UDim2.fromOffset(1, 24), C.stroke)
local miniDot = UI.make("Frame", {
	BackgroundColor3 = A.a,
	BorderSizePixel = 0,
	AnchorPoint = Vector2.new(0.5, 0.5),
	Position = UDim2.fromOffset(172, MINI_H / 2),
	Size = UDim2.fromOffset(9, 9),
	Parent = miniBody,
}, { UI.round() })
local miniHalo = UI.pulse(miniDot, A.a)
local miniStatus = UI.clip(text(miniBody, "", 186, MINI_H / 2, 15, { color = C.text2, weight = MEDIUM }), MINI_W - 186 - 92)
local miniRestore = UI.iconButton(miniBody, UDim2.new(1, -70, 0.5, 0), 34, "restore", { radius = 17 })
local miniClose = UI.iconButton(miniBody, UDim2.new(1, -32, 0.5, 0), 34, "close", { radius = 17, hoverColor = C.dangerFill })

refreshMini = function(override)
	local on = UI.featureCounts()
	miniStatus.Text = override or string.format("%s  ·  %d on", currentProfile and currentProfile.name or "", on)
	local color = on > 0 and A.a or C.muted
	miniDot.BackgroundColor3 = color
	miniHalo.BackgroundColor3 = color
end

-- notice shown after closing the window, so the open key isn't forgotten
local notice = UI.make("Frame", {
	BackgroundTransparency = 1,
	AnchorPoint = Vector2.new(0.5, 0),
	Position = UDim2.new(0.5, 0, 0, 8),
	Size = UDim2.fromOffset(400, 48 + MM * 2),
	Visible = false,
	Parent = gui,
})
local noticeScale = UI.make("UIScale", { Scale = DEFAULT_SCALE, Parent = notice })
local noticeShade = UI.make("Frame", {
	BackgroundTransparency = 1,
	Position = UDim2.fromOffset(MM, MM),
	Size = UDim2.new(1, -MM * 2, 1, -MM * 2),
	Parent = notice,
})
UI.softShadow(noticeShade, 24, 4, 3, 0.08, 4)
local noticeBody = UI.make("Frame", {
	BackgroundColor3 = C.bg,
	BackgroundTransparency = 0.03,
	Position = UDim2.fromOffset(MM, MM),
	Size = UDim2.new(1, -MM * 2, 1, -MM * 2),
	ZIndex = 2,
	Parent = notice,
}, { UI.round(), UI.stroke(C.stroke, 1) })
local noticeLabel = text(noticeBody, "", 20, 24, 16, { color = C.text2, weight = MEDIUM })
local noticeKey = UI.make("Frame", {
	BackgroundColor3 = C.raised,
	AnchorPoint = Vector2.new(0, 0.5),
	Position = UDim2.fromOffset(0, 24),
	Size = UDim2.fromOffset(60, 28),
	Parent = noticeBody,
}, { UI.corner(7), UI.stroke(C.strokeHi, 1) })
local noticeKeyText = UI.make("TextLabel", {
	BackgroundTransparency = 1,
	Size = UDim2.fromScale(1, 1),
	Text = "",
	FontFace = UI.font(SEMIBOLD),
	TextSize = 14,
	TextColor3 = C.text,
	Parent = noticeKey,
})
local noticeToken = 0
function UI.showNotice()
	noticeToken += 1
	local mine = noticeToken
	local lead = "practical is hidden  ·  press"
	local keyName = PREF_KEY.value.Name
	noticeLabel.Text = lead
	noticeKeyText.Text = keyName
	local lw = UI.textWidth(lead, 16)
	local kw = UI.textWidth(keyName, 14) + 22
	noticeKey.Size = UDim2.fromOffset(kw, 28)
	noticeKey.Position = UDim2.fromOffset(20 + lw + 10, 24)
	notice.Size = UDim2.fromOffset(20 + lw + 10 + kw + 14 + MM * 2, 48 + MM * 2)
	notice.Visible = true
	notice.Position = UDim2.new(0.5, 0, 0, -4)
	UI.groupFade(notice, 0, 0.35)
	UI.tween(notice, 0.5, { Position = UDim2.new(0.5, 0, 0, 8) }, BACK)
	UI.after(3.4, function()
		if mine == noticeToken then
			UI.groupFade(notice, 1, 0.4).Completed:Connect(function(state)
				if state == Enum.PlaybackState.Completed and mine == noticeToken then
					notice.Visible = false
				end
			end)
		end
	end)
end
function UI.hideNotice()
	noticeToken += 1
	if notice.Visible then
		UI.groupFade(notice, 1, 0.15).Completed:Connect(function()
			notice.Visible = false
		end)
	end
end

-------------------------------------------------------------------- open / minimize / close
local mode = "closed" -- "open" | "mini" | "closed"

function UI.showWindow()
	root.Visible = true
	popScale.Scale = 0.94
	UI.groupFade(group, 0, 0.3)
	UI.tween(popScale, 0.55, { Scale = 1 }, BACK)
end

function UI.hideWindow()
	closePopup()
	UI.tween(popScale, 0.22, { Scale = 0.95 }, QUINT, IN)
	UI.groupFade(group, 1, 0.22, 0, QUINT, IN).Completed:Connect(function(state)
		if state == Enum.PlaybackState.Completed and mode ~= "open" then
			root.Visible = false
		end
	end)
end

function UI.showMini()
	refreshMini()
	mini.Visible = true
	mini.Position = UDim2.new(0.5, 0, 0, -14)
	UI.groupFade(mini, 0, 0.35, 0.1)
	UI.tween(mini, 0.55, { Position = UDim2.new(0.5, 0, 0, 8) }, BACK, OUT, 0.1)
end

function UI.hideMini()
	if not mini.Visible then
		return
	end
	UI.tween(mini, 0.2, { Position = UDim2.new(0.5, 0, 0, -4) })
	UI.groupFade(mini, 1, 0.2).Completed:Connect(function(state)
		if state == Enum.PlaybackState.Completed and mode ~= "mini" then
			mini.Visible = false
		end
	end)
end

setMode = function(m)
	if m == mode then
		return
	end
	local prev = mode
	mode = m
	if m == "open" then
		UI.hideMini()
		UI.hideNotice()
		UI.showWindow()
	elseif m == "mini" then
		UI.hideNotice()
		UI.hideWindow()
		UI.showMini()
	else -- closed
		if prev == "open" then
			UI.hideWindow()
		else
			UI.hideMini()
		end
		if PREF_NOTICE.value then
			UI.showNotice()
		end
	end
end

minBtn.MouseButton1Click:Connect(function()
	setMode("mini")
end)
closeBtn.MouseButton1Click:Connect(function()
	setMode("closed")
end)
miniBody.MouseButton1Click:Connect(function()
	setMode("open")
end)
miniRestore.MouseButton1Click:Connect(function()
	setMode("open")
end)
miniClose.MouseButton1Click:Connect(function()
	setMode("closed")
end)

-------------------------------------------------------------------- scale, position, drag, resize
local dragTarget -- top-left of `root` on screen (offsets)
local centered = false

centerWindow = function()
	local s = gui.AbsoluteSize
	local sc = fitScale.Scale
	dragTarget = UDim2.fromOffset(
		math.floor((s.X - (targetW + PAD * 2) * sc) / 2),
		math.floor((s.Y - (targetH + PAD * 2) * sc) / 2)
	)
end

fit = function(t)
	-- Profile restore runs before parenting; defer fitting until layout is ready.
	if not UI.sizingReady then return end
	local s = gui.AbsoluteSize
	if s.X <= 0 or s.Y <= 0 then
		return
	end
	local pref = PREF_SCALE.value
	-- Start at the default rectangle instead of expanding to fill the screen.
	if not centered then
		targetW, targetH = W, H
	end
 local availableW=math.max(MIN_W,s.X*.96/pref-PAD*2)
 local availableH=math.max(MIN_H,s.Y*.96/pref-PAD*2)
 targetW=math.min(targetW,availableW);targetH=math.min(targetH,availableH)
 -- Apply immediately when fitting, rather than temporarily scaling the old rectangle.
 winW=targetW;winH=targetH;UI.applySize()
	local sc = math.min(pref, s.X * 0.98 / (winW + PAD * 2), s.Y * 0.98 / (winH + PAD * 2))
	if UI.fitTween then UI.fitTween:Cancel(); UI.fitTween = nil end
	UI.fitTween = UI.tween(fitScale, t or 0, { Scale = sc })
	UI.tween(miniScale, t or 0, { Scale = pref })
	noticeScale.Scale = pref
	toastScale.Scale = pref
	if not centered then
		centered = true
		fitScale.Scale = sc
		centerWindow()
		root.Position = dragTarget
	end
end
gui:GetPropertyChangedSignal("AbsoluteSize"):Connect(function()
	fit(0)
end)
PREF_SCALE.onSet = function()
	fit(0.2)
end

local dragStart, dragOrigin
local resizing -- { start, w, h }
local lastTitleClick = 0
local restoreRect -- size/position before maximise

function UI.toggleMaximize()
	local s = gui.AbsoluteSize
	local sc = fitScale.Scale
	if restoreRect then
		targetW, targetH = restoreRect.w, restoreRect.h
		dragTarget = restoreRect.pos
		restoreRect = nil
		setStatus("Window restored")
	else
		restoreRect = { w = targetW, h = targetH, pos = dragTarget }
		targetW = math.max(MIN_W, (s.X - 24) / sc)
		targetH = math.max(MIN_H, (s.Y - 24) / sc)
		dragTarget = UDim2.fromOffset(12 - PAD * sc, 12 - PAD * sc)
		setStatus("Window maximized  ·  double-click the title bar to restore")
	end
end

titleBar.InputBegan:Connect(function(io)
	if io.UserInputType == Enum.UserInputType.MouseButton1 or io.UserInputType == Enum.UserInputType.Touch then
		local now = os.clock()
		if now - lastTitleClick < 0.32 then
			lastTitleClick = 0
			dragStart = nil
			UI.toggleMaximize()
			return
		end
		lastTitleClick = now
		dragStart = io.Position
		dragOrigin = dragTarget
	end
end)
grip.InputBegan:Connect(function(io)
	if io.UserInputType == Enum.UserInputType.MouseButton1 or io.UserInputType == Enum.UserInputType.Touch then
		closePopup()
		restoreRect = nil
		resizing = { start = io.Position, w = targetW, h = targetH }
	end
end)
UI.connect(UserInputService.InputChanged, function(io)
	if io.UserInputType ~= Enum.UserInputType.MouseMovement and io.UserInputType ~= Enum.UserInputType.Touch then
		return
	end
	if dragStart and dragOrigin then
		local d = io.Position - dragStart
		if restoreRect and d.Magnitude > 6 then
			restoreRect = nil -- dragging a maximised window un-maximises it in place
		end
		local s = gui.AbsoluteSize
		local sc = fitScale.Scale
		local x = math.clamp(dragOrigin.X.Offset + d.X, -(winW + PAD) * sc + 120, s.X - (PAD * sc) - 120)
		local y = math.clamp(dragOrigin.Y.Offset + d.Y, -PAD * sc, s.Y - (PAD + TITLE_H) * sc)
		dragTarget = UDim2.fromOffset(x, y)
	elseif resizing then
		local d = io.Position - resizing.start
		local sc = fitScale.Scale
		local s = gui.AbsoluteSize
		local maxW = (s.X - root.AbsolutePosition.X) / sc - PAD - 4
		local maxH = (s.Y - root.AbsolutePosition.Y) / sc - PAD - 4
		targetW = math.clamp(resizing.w + d.X / sc, MIN_W, math.max(MIN_W, maxW))
		targetH = math.clamp(resizing.h + d.Y / sc, MIN_H, math.max(MIN_H, maxH))
	end
end)

-- per-frame: window follow, animated border, drifting ambient light
UI.connect(RunService.RenderStepped, function(dt)
	local a = 1 - math.exp(-dt * 22)
	if dragTarget then
		local p = root.Position
		if math.abs(p.X.Offset - dragTarget.X.Offset) + math.abs(p.Y.Offset - dragTarget.Y.Offset) > 0.05 then
			root.Position = p:Lerp(dragTarget, a)
		end
	end
	if math.abs(winW - targetW) + math.abs(winH - targetH) > 0.1 then
		winW += (targetW - winW) * a
		winH += (targetH - winH) * a
		UI.applySize()
	end
	if mode == "open" and not MOTION.reduce then
		if borderOn then
			borderGrad.Rotation = (borderGrad.Rotation + dt * 32) % 360
		end
		if ambientOn then
			local t = os.clock()
			for _, b in ipairs(blobs) do
				b.frame.Position = UDim2.fromScale(
					b.x + math.sin(t * 0.13 + b.phase) * 0.07,
					b.y + math.cos(t * 0.1 + b.phase) * 0.09
				)
			end
		end
	end
end)

-------------------------------------------------------------------- keyboard
UI.connect(UserInputService.InputBegan, function(io, gameProcessed)
	local mouse=io.UserInputType==Enum.UserInputType.MouseButton1 or io.UserInputType==Enum.UserInputType.MouseButton2 or io.UserInputType==Enum.UserInputType.MouseButton3
	if io.UserInputType ~= Enum.UserInputType.Keyboard and not mouse then return end
	local binding=mouse and io.UserInputType or io.KeyCode
	if rebinding then
		local r = rebinding
		rebinding = nil
		if io.KeyCode ~= Enum.KeyCode.Escape then
			r.setting.value = binding
			controlChanged:Fire(r.ctrl.id, r.setting.id, binding)
		end
		r.stop()
		return
	end

	if binding == PREF_KEY.value and not gameProcessed then
		setMode(mode == "open" and "closed" or "open")
		return
	end

	local ctrlHeld = UserInputService:IsKeyDown(Enum.KeyCode.LeftControl) or UserInputService:IsKeyDown(Enum.KeyCode.RightControl)
	if io.KeyCode == Enum.KeyCode.K and ctrlHeld and mode == "open" then
		task.defer(function()
			searchBox:CaptureFocus()
		end)
		return
	end
	if io.KeyCode == Enum.KeyCode.Escape and mode == "open" then
		closePopup()
	end

	if gameProcessed then
		return
	end
	if CTRL.motion.enabled and binding == SETTING.motion.shortcut.value then
		if playMotionPreview then
			playMotionPreview()
		end
		popScale.Scale = 0.985
		UI.tween(popScale, 0.5, { Scale = 1 }, BACK)
		setStatus("Preview shortcut  ·  " .. io.KeyCode.Name)
	end
end)

UI.connect(UserInputService.InputEnded, function(io)
	if io.UserInputType == Enum.UserInputType.MouseButton1 or io.UserInputType == Enum.UserInputType.Touch then
		dragStart = nil
		resizing = nil
	end
end)

gui.Destroying:Connect(function()
	alive = false
	for _, c in ipairs(connections) do
		c:Disconnect()
	end
	for _, c in ipairs(rowConns) do
		c:Disconnect()
	end
end)

do (function()
-- Connect the supplied interface to the existing feature engine.
local booleanMap={autoRelease={latencyCompensation="latencyCompensation"},visualStyle={enabled="visualPolish",labels="visualLabels",motion="visualMotion",urgency="visualUrgency",beacon="visualBeacon",flightShadow="visualFlightShadow",comet="visualComet"},antiOop={enabled="antiOop",move="oopMove",guide="oopGuide"},passVision={enabled="passVision"},dunkStopper={enabled="dunkStopper"},autoJoin={enabled="autoJoin"},passIntercept={enabled="passIntercept",automatic="passInterceptAuto",move="passInterceptMove",jump="passInterceptJump",shots="passInterceptShots"},contestCeiling={enabled="contestCeiling",release="contestRelease"},recovery={enabled="ballRecovery"},screens={enabled="screenNavigation",visual="screenAwareness"},assignmentTools={lock="assignmentLock"},passingLanes={enabled="passingLanes"},comboBuilder={enabled="comboBuilder",sprint="comboSprint"},autoGuard={antiPush="antiPush",rimBlock="rimBlock",adaptive="adaptiveGuard",cutoff="driveCutoff",screens="screenNavigation",coverage="guardCoverage",enabled="guard",faceHandler="faceTarget",lockAssignment="assignmentLock",showIndicators="visuals",fakeFilter="fakeFilter"},
 autoSteal={enabled="steal"},autoBlock={commit="blockCommitFilter",enabled="block",oneAttempt="blockOnce"},
 rebound={enabled="rebound",chase="reboundChase"},boost={enabled="boost"},
 landingTools={trail="ballTrail",enabled="landing",beam="landingBeam",zone="landingZone"},overlayTools={contest="liveContest",guard="visuals"},
 autoRelease={liveContest="liveContest",contest="contest"},shotAssist={enabled="shotAssist",holdEarly="shotHoldEarly",protectFall="shotProtectFall"}}
local numberMap={visualStyle={scale="visualScale",intensity="visualIntensity",weight="visualWeight"},antiOop={range="oopRange"},passVision={speed="passSpeed",release="catchRelease"},dunkStopper={range="dunkRange",timing="dunkTimingMs"},passIntercept={reach="passInterceptReach"},contestCeiling={limit="contestLimit"},recovery={range="recoveryRange",window="recoveryWindow"},screens={range="screenRange"},passingLanes={range="laneRange",clearance="laneClearance"},comboBuilder={interval="comboInterval",repeats="comboRepeats"},autoGuard={standOff="guardGap",delay="guardReaction"},autoSteal={reach="stealRange",cooldown="stealCooldown",delay="stealDelay"},
 autoRelease={offset="timingOffsetMs",maxPing="timingMaxPingMs",freshness="timingFreshMs"},shotAssist={meterMin="shotMeterMin",meterMax="shotMeterMax"},
 rebound={range="reboundChaseRange",reach="reboundRange",height="reboundHeight",lead="reboundLead"},
 landingTools={width="landingBeamWidth",scale="landingZoneScale",range="landingRange"},
 autoBlock={reactionDelay="blockDelay"},boost={multiplier="boostMultiplier",tail="boostDuration"},}
booleanMap.visualLayers={landing="visualShowLanding",flight="visualShowFlight",route="visualShowRoute",coverage="visualShowCoverage",pass="visualShowPass",recovery="visualShowRecovery",screen="visualShowScreen",intercept="visualShowIntercept"}
booleanMap.visualLabels={passPanel="visualPassPanel"}
booleanMap.defenderAlerts={enabled="defenderAlerts",ballOnly="alertBallOnly"}
numberMap.defenderAlerts={range="alertRange"}
booleanMap.rebound.early="reboundEarly";numberMap.rebound.spot="reboundSpot"
booleanMap.autoGuard.idleMovement="guardIdleMovement";booleanMap.autoGuard.humanize="guardHuman";booleanMap.autoGuard.rimPress="rimPress"
numberMap.autoGuard.humanLevel="guardHumanLevel";numberMap.autoGuard.rimRange="rimPressRange";numberMap.autoGuard.rimGap="rimPressGap"
booleanMap.autofarm={postClamp="farmPostClamp",chatRequeue="farmChatRequeue",crossover="farmCrossover",shoot="farmShoot",randomThrees="farmRandomThrees",visuals="farmVisuals",rimStop="farmRimStop"};numberMap.autofarm={stepbackAggression="farmStepbackAggression",dunkRange="farmDunkRange",shotRange="farmShotRange"}
booleanMap.visualColor={enabled="visualCustomColor"}
booleanMap.denyReceiver={enabled="denyReceiver",catch="denyCatch"}
booleanMap.offBallResponse={enabled="denySoft"}
numberMap.offBallResponse={delay="denyDelay",smoothing="denySmoothing",deadzone="denyDeadzone",lead="denyLead"}
booleanMap.smartDribble={enabled="smartDribble",follow="dribbleTestFollow",shoot="smartDribbleShoot",steer="smartDribbleSteer"}
booleanMap.smartPass={enabled="smartPass",spin="smartPassSpin"}
booleanMap.catchShoot={enabled="catchShoot"}
numberMap.catchShoot={contest="catchShootContest"}
booleanMap.ballProtection={enabled="ballProtection"}
numberMap.smartDribble={gap="smartDribbleGap",stamina="smartDribbleStamina",cooldown="smartDribbleCooldown",testSpeed="dribbleTestSpeed",shotContest="smartDribbleShotContest",shotRange="smartDribbleShotRange",arc="smartDribbleThreeDistance"}
numberMap.ballProtection={range="ballProtectionRange"}
numberMap.visualStyle.smoothing="visualSmoothing";numberMap.visualStyle.clearance="visualClearance"
numberMap.visualLabels={size="visualLabelSize",opacity="visualLabelOpacity"}
numberMap.visualColor={red="visualRed",green="visualGreen",blue="visualBlue"}
numberMap.denyReceiver={range="denyRange",gap="denyGap",switch="denySwitchMargin",delay="denyDelay",smoothing="denySmoothing",deadzone="denyDeadzone"}
local firstCourtCorner
local function applyResponse(name)
 local values=name=="Legit" and {guardReaction=.24,guardPrediction=.05,guardStrength=.4,guardDeadzone=.9,sprintDistance=4,closeReaction=.15,closeGap=2,stealAngle=35,stealMaxSpeed=8,stealDelay=.2,stealCooldown=2,blockDelay=.12,jumpCooldown=1.6,reboundLead=.1}
  or name=="Blatant" and {guardReaction=0,guardPrediction=.12,guardStrength=1,guardDeadzone=.35,sprintDistance=1,closeReaction=0,closeGap=1,stealAngle=65,stealMaxSpeed=20,stealDelay=.05,stealCooldown=.8,blockDelay=.05,jumpCooldown=.8,reboundLead=.2}
  or {guardReaction=.16,guardPrediction=.08,guardStrength=1,guardDeadzone=.65,sprintDistance=2.5,closeReaction=.08,closeGap=1.5,stealAngle=45,stealMaxSpeed=10,stealDelay=.12,stealCooldown=1.6,blockDelay=.08,jumpCooldown=1.2,reboundLead=.15}
 for key,value in pairs(values) do api.SetNumber(key,value) end
end
local screenVisualChoice=true
local function routeControl(id,field,value)
 if id=="screens"and field=="enabled"then
  api.SetOption("screenNavigation",value)
  return api.SetOption("screenAwareness",value and screenVisualChoice)
 end
 if id=="screens"and field=="visual"then
  screenVisualChoice=value
  return api.SetOption("screenAwareness",value and api.GetOption("screenNavigation"))
 end
 if id=="smartDribble"and field=="activation"then return api.SetSmartDribbleKey(value)end
 if id=="smartDribble"and field=="plan"then
  return api.SetNumber("smartDribblePlan",value=="Attack basket"and 2 or value=="Find a three"and 3 or 1)
 end
 if id=="visualStyle"and field=="route"then
  return api.SetNumber("visualRouteStyle",({Dashed=1,Solid=2,Chevrons=3})[value]or 1)
 end
 if id=="visualStyle"and field=="palette"then
  return api.SetNumber("visualPalette",({Cobalt=1,Glacier=2,Mint=3,Amber=4})[value]or 1)
 end
 if id=="visualStyle"and field=="preset"then
  local preset=value=="Minimal"and {intensity=35,weight=.95,scale=.9,smoothing=70,clearance=.12,motion=false}
   or value=="Vivid"and {intensity=85,weight=1.4,scale=1.1,smoothing=45,clearance=.16,motion=true}
   or {intensity=65,weight=1.15,scale=1,smoothing=55,clearance=.12,motion=false}
  for key,settingValue in pairs(preset)do
   if key=="motion"then api.SetOption("visualMotion",settingValue)else api.SetNumber(numberMap.visualStyle[key],settingValue)end
  end
  UI.forEachControl(function(ctrl)if ctrl.id==id then UI.forEachValue(ctrl,function(setting)if preset[setting.id]~=nil then setting.value=preset[setting.id]end end)end end)
  repaintAll();return true
 end
 if id=="visualLabels" and field=="corner" then return api.SetHudCorner(value)end
 if id=="dunkStopper" and field=="activation" then return api.SetDunkKey(value)end
 if id=="comboLibrary" then return true end
 if id=="comboBuilder" and field=="sequenceDelay" then return true end
 if id=="comboBuilder" and field=="sequence" then return api.SetComboSequence(value) end
 if id=="comboBuilder" and field=="preset" then
  local presets={Crossovers="ZC@0.18 > CZ@0.18 > ZX@0.25",Stepback="Z@0.18 > X@0.28 > WAIT@0.15 > C@0.20",Mixed="ZX@0.20 > CV@0.25 > WAIT@0.10 > ZXC@0.30"}
  if presets[value] then
   UI.forEachControl(function(ctrl)if ctrl.id==id then UI.forEachValue(ctrl,function(setting)if setting.id=="sequence" then setting.value=presets[value] end end)end end)
   repaintAll();return api.SetComboSequence(presets[value])
  end
  return true
 end
 if id=="comboBuilder" and field:match("^step[1-4]$") then return api.SetComboSlot(tonumber(field:sub(-1)),value) end
 if id=="comboBuilder" and field=="activation" then return api.SetComboKey(value) end
 if field=="engaged" then return true end -- actual bound key state is read by the engine
 -- The farm toggle is user-driven only (see the controlChanged handler); startup/profile syncs must not flip it.
 if id=="autofarm" and field=="enabled" then return true end
 if id=="autoRelease" and field=="enabled" then return api.SetAutoTime(value) end
 local key=booleanMap[id] and booleanMap[id][field]
 if key then
  local ok=api.SetOption(key,value)
  return ok
 end
 key=numberMap[id] and numberMap[id][field]
 if key then
  local ok=api.SetNumber(key,value)
  if id=="shotAssist" then UI.forEachControl(function(ctrl)
   if ctrl.id==id then UI.forEachValue(ctrl,function(setting)
    local mapped=numberMap[id][setting.id];if mapped then setting.value=api.GetOption(mapped) end
   end) end
  end);repaintAll() end
  return ok
 end
 if id=="autoGuard" and field=="activation" then return api.SetGuardKey(value) end
 if id=="autoGuard" and field=="response" then
  applyResponse(value)
  UI.forEachControl(function(ctrl) UI.forEachValue(ctrl,function(setting)
   local mapped=numberMap[ctrl.id] and numberMap[ctrl.id][setting.id]
   if mapped then setting.value=api.GetOption(mapped) end
  end) end)
  repaintAll();return true
 end
 if id=="interface" or id=="appearance" or id=="layout" or id=="motion" or id=="typography" or id=="notifications" or id=="comboLibrary" then return true end
 return false,"Unsupported control: "..tostring(id).." / "..tostring(field)
end
local function applyAll()
 -- Presets first; explicit sliders then override their corresponding values.
 UI.forEachControl(function(ctrl) UI.forEachValue(ctrl,function(setting)
  if ctrl.id=="autoGuard" and setting.id=="response" then applyResponse(setting.value) end
 end) end)
 UI.forEachControl(function(ctrl)
  if ctrl.kind~="section" then routeControl(ctrl.id,"enabled",ctrl.enabled==true) end
  UI.forEachValue(ctrl,function(setting)
   if setting.id~="response" and setting.type~="button"and not(ctrl.id=="visualStyle"and setting.id=="preset")then routeControl(ctrl.id,setting.id,setting.value)end
  end)
 end)
end

local appearanceApply=applySettings
applySettings=function(t)appearanceApply(t);applyAll()end
UI.connect(controlChanged.Event,function(id,field,value)
 if id=="catchShoot"and field=="status"then
  local state=api.GetCatchShootState()
  local message=string.format("%s · Early E requests: %d · Last lead: %s · %s",state.status,state.earlyRequests or 0,state.lastLeadMs and string.format("%.0f ms",state.lastLeadMs)or "not recorded",state.bufferReason or "unknown")
  setStatus(message);toast("Early-shot buffer",message,"info",true);return
 end
 if id=="smartPass"and field=="status"then
  local passState=api.GetSmartPassState();local names={}
  for key,name in pairs(passState.slots or {})do names[#names+1]=key..": "..name end
  table.sort(names)
  setStatus(passState.status.." � receivers: "..(#names>0 and table.concat(names,", ")or "not identified yet"));return
 end
 if id=="smartDribble"and field=="pickBasket"then local _,message=api.PickDribbleBasket();setStatus(message);return end
 if id=="smartDribble"and field=="markThree"then
  local ok,message=api.MarkDribbleThreePointLine()
  if ok then
   UI.forEachControl(function(ctrl)if ctrl.id==id then UI.forEachValue(ctrl,function(setting)if setting.id=="arc"then setting.value=api.GetOption("smartDribbleThreeDistance")end end)end end)
   repaintAll()
  end
  setStatus(message);return
 end
 if id=="smartDribble"and(field=="spawnTest"or field=="stopTest")then
  local _,message
  if field=="spawnTest"then _,message=api.StartDribblePractice()else _,message=api.StopDribblePractice()end
  setStatus(message);return
 end
 if id=="smartDribble"and field=="status"then
  local state=api.GetSmartDribbleState()
  local reason=state.held and state.status or state.lastHeldStatus and("Last hold: "..state.lastHeldStatus)or state.status
  local message=string.format("%s · Key %s (%s) · Action %s · Stamina %s · CanDribble %s · %d opponents · Basket: %s · Arc: %.1f",reason,state.key or "?",state.held and "held"or "released",tostring(state.action or "unknown"),tostring(state.stamina or "unknown"),tostring(state.canDribble),state.defenders or 0,state.basketSource or "not checked",state.threeDistance or 0)
  setStatus(message);toast("Smart dribbler status",message,"info",true);return
 end
 if id=="profile" then
  applyAll()
  UI.persistProfiles();return
 end
 if id=="denyReceiver"and(field=="select"or field=="clear"or field=="next")then
  local _,message
  if field=="select"then
   _,message=api.BeginReceiverSelection();toast("Choose a matchup",message,"crosshair",true)
  elseif field=="next"then _,message=api.CycleReceiverMatchup()
  else
   _,message=api.ClearReceiverMatchup()
   UI.forEachControl(function(ctrl)if ctrl.id=="assignmentTools"then UI.forEachValue(ctrl,function(setting)if setting.id=="lock"then setting.value=false end end)end end)
   repaintAll()
  end
  setStatus(message);return
 end
 if id=="denyReceiver"and field=="status"then
  local state=api.GetReceiverDenialState()
  setStatus(state.status..(state.target and " · "..state.target or "")..(state.handler and " · passer "..state.handler or ""));return
 end
 if id=="antiOop" and field=="status" then
  local state=api.GetAntiOopState();setStatus(state.status..(state.gap and string.format(" · gap %.1f / ETA %.2fs",state.gap,state.time)or "").." · last jump: "..(state.lastJump or "none"));return
 end
 if id=="passIntercept" and field=="status" then setStatus(api.GetDefenseNavigationState().intercept);return end
 if id=="appearance" or id=="motion" or id=="layout" or id=="typography" or id=="notifications" or id=="interface" then
  applySettings();refreshNav();refreshMini()
  if field=="preview" then ACTIONS.preview() end
  if field=="resetLayout" then ACTIONS.resetLayout() end
  return
 end
 if id=="autofarm" and field=="enabled" then local _,message=api.SetFarm(value==true);setStatus(message);toast("Blacktop autofarm",message,"info",true);return end
 if id=="autofarm" and(field=="gymTest"or field=="stopGymTest")then local _,message=(field=="gymTest"and api.StartFarmGymTest or api.StopFarmGymTest)();setStatus(message);toast("Gym test",message,"info",true);return end
 if id=="autofarm" and field=="readyNow" then local _,message=api.FarmReadyNow();setStatus(message);return end
 if id=="autofarm" and field=="status" then
  local s=api.GetFarmState()
  local message=string.format("%s · %s · %s · readies %d · dunks %d · loose balls %d · crossovers %d · %s",s.running and "ON" or "OFF",s.status,s.seen or "-",s.ready,s.dunks,s.loose,s.crosses,s.queue)
  setStatus(message);toast("Blacktop autofarm",message,"info",true);return
 end
 if id=="autoJoin" and field=="lock" then local _,message=api.LockJoinSpot();setStatus(message);return end
 if id=="autoJoin" and field=="status" then local state=api.GetAutoJoinState();setStatus(state.status.." · "..state.attempts.." requests");return end
 if id=="comboBuilder" and field=="play" then
  UI.forEachControl(function(ctrl)if ctrl.id==id then ctrl.enabled=true end end)
  api.SetOption("comboBuilder",true);repaintAll()
  local _,message=api.PlayCombo();setStatus(message);return end
 if id=="comboBuilder" and field=="cancel" then api.CancelCombo();setStatus("Combo cancelled");return end
 if id=="assignmentTools" and field=="next" then setStatus(api.SwitchAssignment() and "Matchup selected" or "No opponent in range");return end
 if id=="tools" then
  if field=="report" then local _,message=api.ExportReport();setStatus(message)
  elseif field=="check" then local _,message=api.Check();setStatus(message)
  elseif field=="assignment" then setStatus(api.SwitchAssignment() and "Opponent selected" or "No opponent in range")
  elseif field=="solo" then api.StartSolo();setStatus("Local rehearsal only; no server opponent")
  elseif field=="stopSolo" then api.StopSolo();setStatus("Rehearsal stopped")
  elseif field=="unload" then api.Unload() end
  return
 end
 if id=="rebound" and field=="setCourtBounds" then
  local folder=workspace:FindFirstChild("Characters");local own=folder and folder:FindFirstChild(player.Name) or player.Character
  local r=own and own:FindFirstChild("HumanoidRootPart")
  if not r then setStatus("Character unavailable");return end
  if not firstCourtCorner then firstCourtCorner=r.Position;setStatus("First corner marked. Walk to opposite corner and click again.")
  elseif api.SetCourtBounds(firstCourtCorner,r.Position) then firstCourtCorner=nil;setStatus("Court bounds set")
  else setStatus("Corners must span at least 6 studs on both axes") end
  return
 elseif id=="rebound" and field=="clearBounds" then firstCourtCorner=nil;api.ClearCourtBounds();setStatus("Court bounds cleared");return end
 local ok,message=routeControl(id,field,value)
 if ok and id=="denyReceiver"and field=="enabled"and value and api.GetReceiverDenialState and api.BeginReceiverSelection then
  if not api.GetReceiverDenialState().selected then
   local _,notice=api.BeginReceiverSelection();toast("Choose a matchup",notice,"crosshair",true);setStatus(notice)
  end
 end
 if not ok and message then setStatus(message) end
end)
local lastMatchup,matchupNoticeAt=nil,0
UI.connect(RunService.Heartbeat,function()
 if os.clock()-matchupNoticeAt<.25 then return end
 matchupNoticeAt=os.clock()
 -- The engine owns the autofarm state (saved config, the 5 s prompt); keep the toggle matching it.
 local farmOn=api.GetOption("farm")==true
 UI.forEachControl(function(ctrl)if ctrl.id=="autofarm" and ctrl.enabled~=farmOn then ctrl.enabled=farmOn;repaintAll()end end)
 if not api.GetReceiverDenialState then return end
 local state=api.GetReceiverDenialState()
 if state.selected~=lastMatchup then
  if state.selected then
   toast("Matchup selected",state.selected.." · on and off ball","shield",true)
   UI.forEachControl(function(ctrl)if ctrl.id=="assignmentTools"then UI.forEachValue(ctrl,function(setting)if setting.id=="lock"then setting.value=false end end)end end)
   repaintAll()
  elseif lastMatchup then toast("Following ball handler","Your selected matchup was cleared or is no longer available","shield",true)end
  lastMatchup=state.selected
 end
end)
UI.connect(testControls.Event,function() local _,message=api.Check();setStatus(message) end)

-- Notification and non-interactive cursor visuals; no full-screen modal.
do
 local shownRevision,lastPoll=0,0
 local cursor,hoverOutline,nameLabel
 local function make(class,props)
  local object=Instance.new(class)
  for key,value in pairs(props)do object[key]=value end
  return object
 end
 local function hide()
  if cursor then cursor.Visible=false end
  if hoverOutline then hoverOutline.Enabled=false;hoverOutline.Adornee=nil end
 end
 local function ensure()
  if cursor then return end
  cursor=make("Frame",{Name="MatchupSelectionCursor",Size=UDim2.fromOffset(40,40),BackgroundTransparency=1,BorderSizePixel=0,Visible=false,Active=false,ZIndex=100,Parent=gui})
  local ring=make("Frame",{Size=UDim2.fromOffset(28,28),Position=UDim2.fromOffset(6,6),BackgroundTransparency=1,ZIndex=101,Parent=cursor})
  make("UICorner",{CornerRadius=UDim.new(1,0),Parent=ring})
  make("UIStroke",{Color=Color3.fromRGB(80,167,255),Thickness=1.5,Transparency=.12,Parent=ring})
  for _,line in ipairs({{19,0,2,7},{19,33,2,7},{0,19,7,2},{33,19,7,2}})do
   make("Frame",{Position=UDim2.fromOffset(line[1],line[2]),Size=UDim2.fromOffset(line[3],line[4]),BackgroundColor3=Color3.fromRGB(109,191,255),BorderSizePixel=0,ZIndex=101,Parent=cursor})
  end
  local pill=make("Frame",{Position=UDim2.fromOffset(45,8),Size=UDim2.fromOffset(220,44),BackgroundColor3=Color3.fromRGB(17,20,29),BackgroundTransparency=.05,BorderSizePixel=0,ZIndex=101,Parent=cursor})
  make("UICorner",{CornerRadius=UDim.new(0,12),Parent=pill})
  make("UIStroke",{Color=Color3.fromRGB(40,47,64),Thickness=1,Parent=pill})
  nameLabel=make("TextLabel",{Size=UDim2.new(1,-20,1,0),Position=UDim2.fromOffset(10,0),BackgroundTransparency=1,Text="",TextColor3=Color3.fromRGB(227,240,255),Font=Enum.Font.GothamMedium,TextSize=12,TextWrapped=true,ZIndex=102,Parent=pill})
  hoverOutline=make("Highlight",{Name="MatchupSelectionHover",Enabled=false,FillColor=Color3.fromRGB(49,135,255),FillTransparency=.87,OutlineColor=Color3.fromRGB(106,202,255),OutlineTransparency=.05,DepthMode=Enum.HighlightDepthMode.Occluded,Parent=workspace.CurrentCamera or workspace})
 end
 UI.connect(RunService.Heartbeat,function()
  if not api.GetMatchupPickerState or os.clock()-lastPoll<.25 then return end
  lastPoll=os.clock()
  local state=api.GetMatchupPickerState()
  if not state.active or state.revision<=shownRevision then return end
  shownRevision=state.revision
  api.BeginReceiverSelection()
  toast("Choose your matchup","Hover an opponent, then click to select. Right-click or Escape cancels.","crosshair",true)
 end)
 UI.connect(RunService.RenderStepped,function()
  if not api.GetReceiverDenialState or not api.GetReceiverDenialState().picking then hide();return end
  ensure()
  local position=UserInputService:GetMouseLocation()
  if not gui.IgnoreGuiInset then position=position-game:GetService("GuiService"):GetGuiInset()end
  cursor.Position=UDim2.fromOffset(position.X-20,position.Y-20);cursor.Visible=true
  local character,adornee
  if api.GetReceiverSelectionHover then character,adornee=api.GetReceiverSelectionHover()end
  hoverOutline.Adornee=adornee;hoverOutline.Enabled=adornee~=nil
  nameLabel.Text=character and("CLICK TO GUARD\n"..character.Name)or "SELECT MATCHUP\nHover an opposing player"
  hoverOutline.FillTransparency=.87+math.sin(os.clock()*3)*.025
 end)
 api.RegisterUICleanup(function()
  if hoverOutline then hoverOutline:Destroy()end
  if cursor then cursor:Destroy()end
  if api.CancelReceiverSelection then api.CancelReceiverSelection()end
 end)
end

api.IsUICapturingInput=function()return rebinding~=nil end
api.IsUIReservedKey=function(key)return key==PREF_KEY.value end
api.RegisterUICleanup(function()if alive then gui:Destroy()end end)
api.SetPanelVisible(false)
UI.restore(currentProfile.snap)
end)() end

-------------------------------------------------------------------- go
-- Enforce startup scale after the engine bridge has restored the selected profile.
PREF_SCALE.value = DEFAULT_SCALE
applySettings(0)
openPage(START_PAGE)
navHighlight.Parent=nav[START_PAGE].btn.Parent
navHighlight.Position = nav[START_PAGE].pos
refreshNav(0)
UI.refreshStatus()
refreshMini()
gui.Parent = playerGui
task.defer(function()
	-- Let Roblox establish the mounted ScreenGui bounds before the first fit.
	RunService.RenderStepped:Wait()
	if not alive then return end
	UI.sizingReady = true
	fit(0)
	setMode("open")
end)

]====],"PracticalUI2")
assert(run,err)
local ok,message=xpcall(run,debug.traceback)
if not ok then warn("Practical UI 2 startup failed: "..tostring(message))end
end)()end
