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
