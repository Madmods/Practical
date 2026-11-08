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
    local sprint=engaged and moving and settings.guardSprint~=false and guardError>=(settings.sprintDistance or 0)
    if sprint and not sprintOwned then
        local ok=pcall(function() service.Sprint:Fire(true) end)
        sprintOwned=ok
    elseif not sprint and sprintOwned then
        pcall(function() service.Sprint:Fire(Input:IsKeyDown(Enum.KeyCode.LeftShift) or Input:IsKeyDown(Enum.KeyCode.RightShift)) end)
        sprintOwned=false
    end
    -- A held guard key must not keep the native defensive stance active during a lob run/jump.
    local lobActive=settings.antiOop and (settings.oopDriving or settings._oopActiveAt and os.clock()-settings._oopActiveAt<.25)
    local guard=engaged and not lobActive and not forceRelease and not settings._guardReleased and bindingDown(guardKey)
    if guard and not guardOwned then
        local ok=pcall(function() service.HoldG:Fire({HoldingG=true}) end)
        guardOwned=ok
    elseif not guard and guardOwned then
        pcall(function() service.HoldG:Fire({HoldingG=not forceRelease and not lobActive and bindingDown(guardKey)}) end)
        guardOwned=false
    end
end
