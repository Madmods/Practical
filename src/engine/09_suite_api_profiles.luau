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

