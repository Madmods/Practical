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


