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
 if sr and settings._contestModel then
  local percent,detail=settings._contestModel.evaluate(sr.Position,settings._contestModel.collect(shooter),0)
  highest=percent/100;nearest=detail.gap
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

