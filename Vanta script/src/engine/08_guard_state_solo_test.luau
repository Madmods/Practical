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

