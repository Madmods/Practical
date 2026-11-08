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
 local busy={Shooting=true,Dunking=true,Stunned=true,Dead=true,Diving=true,Stealing=true,Blocking=true,Rebounding=true}
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
 local function resetOffense(service)
  retreat=nil;driveUntil=0;stepbackShotUntil=0;confirmedStepback=false
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

 -- Sprint dribble moves from the recorded profiles, by dribbling hand and the side they take you to.
 -- Shift+Z (right hand, goes left) and Shift+C (left hand, goes right) swap hands, so they chain.
 local MOVES={R={left={"Z"},right={}},L={right={"C"},left={}}} -- Only measured sprint crossovers; no behind-the-back drive inputs.
 local lastHand
 local lastDriveMove=nil
 local driveTransfer=nil
 local rng=Random.new()
 local recentSide,sideRepeats=0,0
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
 local function crossover(own,service,sideSign,direction)
  local now=os.clock()
  local hand=attr(own,"Hand")
  if hand~="R" and hand~="L" then return end
  -- The hand attribute can lag behind the move: if it still shows the hand we last used,
  -- either the move is still playing (wait) or it never updated (assume it flipped).
  if hand==lastHand then
   if now-lastCross<.18 or attr(own,"Action")=="Dribbling" then return end
   -- Keep the observed hand; do not invent a hand change after a rejected move.
  end
  local ownRoot=rootOf(own)
  if not ownRoot then return end
  local keys,sprint=nil,true
  local preferred=hand=="R"and -1 or 1
  if driveTransfer and now-driveTransfer.at>1.8 then driveTransfer=nil end
  if driveTransfer then
   if hand~=driveTransfer.hand then return end
   preferred=driveTransfer.side;driveTransfer=nil
   keys=hand=="R"and "Z"or "C"
  else
   local choices={}
   for _,sign in ipairs({-1,1})do
    local endpoint=ownRoot.Position+flat(ownRoot.CFrame.RightVector)*sign*6+flat(ownRoot.CFrame.LookVector)*2
    local gap=12
    for _,enemy in ipairs(opponents(own))do
     local er=rootOf(enemy)
     if er then gap=math.min(gap,flat(endpoint-er.Position-flat(er.AssemblyLinearVelocity)*.35).Magnitude)end
    end
    local bounded=clampCourt(endpoint)
    if gap>=3 and flat(bounded-endpoint).Magnitude<1 and safeMove(ownRoot.Position,endpoint)then
     choices[#choices+1]={side=sign,score=gap+rng:NextNumber(0,3)-(sign==recentSide and sideRepeats*2 or 0)}
    end
   end
   table.sort(choices,function(a,b)return a.score>b.score end)
   if #choices==0 then return end
   preferred=choices[1].side
   -- Follow the route planner when that side is viable; do not roll an unrelated side.
   local localSide=direction:Dot(flat(ownRoot.CFrame.RightVector))>=0 and 1 or -1
   for _,choice in ipairs(choices)do if choice.side==localSide then preferred=localSide;break end end
   keys=hand=="R"and "Z"or "C"
   if (hand=="R"and preferred==1)or(hand=="L"and preferred==-1)then
    -- Sprint crossover changes hands, then the opposite sprint crossover attacks.
    sprint=true;driveTransfer={hand=hand=="R"and "L"or "R",side=preferred,at=now}
   end
  end
  if preferred==recentSide then sideRepeats=sideRepeats+1 else recentSide=preferred;sideRepeats=1 end
  lastDriveMove=keys;state.dribbleSide=preferred<0 and "left"or "right"
  state.dribblePlan=driveTransfer and "Sprint crossover into opposite drive"or "Sprint crossover"
  lastCrossKeys=keys;lastCross=now;lastHand=hand
  if api.CancelCombo then api.CancelCombo()end
  if pcall(function()service.Dribble:Fire(keys,true,direction)end)then state.crosses=state.crosses+1 end
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
   list[#list+1]={character=enemy,position=er.Position,velocity=velocity,predicted=er.Position+velocity*.4}
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
  if #list==0 then return false,math.huge,0 end
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
 local function requestStandingShot(own,root,service,now,stepbackShot)
  local hoop=rimFor(own,root,now)
  if not hoop or not holdsBall(own)then return false,"No rim or possession"end
  local distance=flat(root.Position-hoop).Magnitude
  local assigned=goalPosition(attr(own,"Goal"))
  if assigned then distance=math.min(distance,flat(root.Position-assigned).Magnitude)end
  local finishRange=math.max(18,settings.farmDunkRange+6)
  if distance<=finishRange then return false,"Finishing zone: keep driving, no E shot"end
  if distance>settings.farmShotRange then return false,"Outside shot range"end
  local motion=root.AssemblyLinearVelocity
  if spaceHeld or attr(own,"CanShoot")==false or attr(own,"CanMove")==false
   or attr(own,"Action")=="Dribbling"or flat(motion).Magnitude>.35 or math.abs(motion.Y)>1 then
   return false,"Wait for planted feet and completed dribble"
  end
  local ping=0;pcall(function()ping=math.clamp(P:GetNetworkPing(),0,.35)end)
  if not shotWindow(root.Position,defenders(own),ping,stepbackShot or(confirmedStepback and now<stepbackShotUntil))then return false,"Defender can contest actual release position"end
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
 local function interceptTime(position,velocity,point,ping)
  local delta=flat(point-position);local distance=math.max(0,delta.Magnitude-2.1)
  if distance==0 then return 0 end
  local top=math.clamp(math.max(16,velocity.Magnitude),16,26)
  local toward=delta.Magnitude>.01 and math.clamp(velocity:Dot(delta.Unit),-top,top)or 0
  local acceleration=60;local ramp=(top-toward)/acceleration
  local rampDistance=toward*ramp+.5*acceleration*ramp*ramp
  local travel=distance<=rampDistance and(-toward+math.sqrt(toward*toward+2*acceleration*distance))/acceleration
   or ramp+(distance-rampDistance)/top
  return math.max(0,travel-math.clamp(ping or 0,0,.35))
 end
 -- END FARM ARRIVAL
 local tactics={failures={},choice=nil,choiceAt=0,progress=nil}
 local function candidatePlans(root,hoop,list,ping)
  local origin=root.Position;local delta=flat(hoop-origin)
  if delta.Magnitude<.1 then return {}end
  local forward=delta.Unit;local right=Vector3.new(-forward.Z,0,forward.X)
  local speed=math.clamp(math.max(16,flat(root.AssemblyLinearVelocity).Magnitude),16,26)
  local entries={}
  local function drive(name,point,bend)
   point=clampCourt(point);bend=bend or Vector3.zero
   local control=(origin+point)*.5+bend
   local previous=origin;local length=0;local margin=math.huge;local bounded=true
   local futureMargin=math.huge;local nearProgress=0
   local limiting,intercept,ourTime,theirTime=nil,nil,0,0
   for i=1,20 do
    local t=i/20
    local pos=origin*((1-t)^2)+control*(2*(1-t)*t)+point*(t*t)
    if not withinCourt(pos)then bounded=false end
    length=length+flat(pos-previous).Magnitude;previous=pos
    local ours=length/speed
    if ours<=.65 then nearProgress=delta.Magnitude-flat(pos-hoop).Magnitude end
    for _,d in ipairs(list)do
     local theirs=interceptTime(d.position,d.velocity,pos,ping)
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
   if tactics.failures[name] and os.clock()<tactics.failures[name] then score=score-8 end
   if not bounded then margin=-math.huge;score=-math.huge end
   local quality=margin>.15 and "good"or(margin>0 and "uncertain"or "bad")
   entries[#entries+1]={kind="drive",name=name,point=point,bend=bend,origin=origin,score=score,
    margin=margin,defender=limiting,intercept=intercept,ourTime=ourTime,theirTime=theirTime,
    quality=quality,text=string.format("%s | score %.1f\nNext 0.65s: you %.2fs / defender %.2fs | margin %+.2fs",name,score,ourTime,theirTime,margin)}
  end
  drive("DRIVE LEFT",origin+forward*8-right*6,-right*1.5)
  drive("DRIVE RIM",hoop-forward*2,Vector3.zero)
  drive("DRIVE RIGHT",origin+forward*8+right*6,right*1.5)
  drive("WIDE LEFT",origin+forward*4-right*9,-right*2)
  drive("WIDE RIGHT",origin+forward*4+right*9,right*2)
  drive("CUT LEFT",origin+forward*11-right*3,-right)
  drive("CUT RIGHT",origin+forward*11+right*3,right)
  local points={{"STEPBACK",clampCourt(origin-forward*5.6)},{"CROSS LEFT",clampCourt(origin-right*5)},{"CROSS RIGHT",clampCourt(origin+right*5)},{"SHOOT HERE",origin}}
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
   elseif range>settings.farmShotRange then allowed=false;reason=string.format("Landing %.1f studs / range limit %.1f",range,settings.farmShotRange)
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
  shotStart=nil;state.shotActionAt=nil;state.stepbackQueuedShot=nil;retreat=nil;confirmedStepback=false
  if service and service.Shoot then pcall(function()service.Shoot:Fire({Shoot=false})end)end
 end
 -- Move first; validate the actual landing before requesting a shot.
 local function stepbackAndShoot(service,now)
  local own=model();local root=rootOf(own)
  if not root or not service or not service.Dribble then return false,"Dribble controller unavailable"end
  local ok,err=pcall(function()service.Dribble:Fire("XX",true,Vector3.zero)end)
  if not ok then return false,err end
  lastStepback=now;retreat={at=now,origin=root.Position,seen=false}
  confirmedStepback=false;stepbackShotUntil=0
  plan.dir=Vector3.zero;plan.reason="Stepback: create separation";guardError=0
  state.status="Stepback: waiting for safe landing"
  return true
 end
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
 local resetScanAt=-math.huge
 local function offense(own,root,now,service)
  local hoop=rimFor(own,root,now)
  if not hoop then state.status="Have ball, no rim found";return end
  local toHoop=flat(hoop-root.Position);local distance=toHoop.Magnitude
  -- Remember which way the court is, so a reset knows where "back out" is even from under the rim.
  if distance>8 then approachDir=(-toHoop).Unit end
  local forward=distance>.1 and toHoop.Unit or(approachDir and -approachDir)or Vector3.new(0,0,-1)
  state.visualOffense=true;state.visualRim=hoop;state.distance=distance
  -- Measure progress toward the chosen waypoint; sideways routes need not approach the rim.
  if tactics.progress and plan.reason=="Autofarm: driving" and now-tactics.progress.at>=.85 then
   local p=tactics.progress
   local gain=p.gap-flat(p.point-root.Position).Magnitude
   if gain<.75 and attr(own,"Action")~="Dribbling"then
    tactics.failures[p.name]=now+2.5;tactics.choice=nil
    state.routeRead="Stalled "..p.name..": try another route"
   end
   tactics.progress=nil
  elseif plan.reason~="Autofarm: driving"then tactics.progress=nil end
  local right=Vector3.new(-forward.Z,0,forward.X)
  local list=defenders(own)
  if retreat or confirmedStepback then escapePlan=nil;resetting=false;allBlockedAt=nil end
  if escapePlan then
   local resetPing=0;pcall(function()resetPing=math.clamp(P:GetNetworkPing(),0,.35)end)
   local fresh=candidatePlans(root,hoop,list,resetPing)
   local drive=bestDrive(fresh)
   local scoreAvailable=drive~=nil
   for _,option in ipairs(fresh)do
    if(option.name=="STEPBACK"or option.name=="SHOOT HERE")and option.quality=="good"then scoreAvailable=true end
   end
   if scoreAvailable and not perimeterNeeded then escapePlan=nil;allBlockedAt=nil;state.status="Space recovered: attack scoring opportunity"end
  end
  if escapePlan then
   if escapePlan.waypoint and flat(escapePlan.waypoint-root.Position).Magnitude<2 then
    escapePlan.waypoint=nil;escapePlan.progressAt=now;escapePlan.gap=flat(escapePlan.point-root.Position).Magnitude
   end
   local delta=flat((escapePlan.waypoint or escapePlan.point)-root.Position)
   if delta.Magnitude<1.5 or now-escapePlan.at>5 then escapePlan=nil;allBlockedAt=nil;perimeterNeeded=false
   elseif now-(escapePlan.progressAt or escapePlan.at)>1.2 and (escapePlan.gap or delta.Magnitude)-delta.Magnitude<.5 then
    escapePlan=nil;allBlockedAt=nil;state.status="Reset route stalled: reassess"
   elseif now-(escapePlan.progressAt or escapePlan.at)>1.2 then
    escapePlan.progressAt=now;escapePlan.gap=delta.Magnitude
    plan.dir=delta.Unit;plan.reason="Reset to perimeter";state.visualTarget=escapePlan.point;guardError=60;return
   else
    plan.dir=delta.Unit;plan.reason="Reset to perimeter";state.visualTarget=escapePlan.point
    state.status="All options blocked: retreat and reassess";guardError=60;return
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
  local inRange=distance>settings.farmDunkRange+6 and distance<=settings.farmShotRange
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
   local elapsed=now-retreat.at
   if action=="Dribbling" then retreat.seen=true end
   local moved=flat(root.Position-retreat.origin).Magnitude
   local completed=(retreat.seen and action~="Dribbling")or(moved>2 and elapsed>.45 and action~="Dribbling")
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
   plan.dir=Vector3.zero;plan.reason="Stepback: plant and release";state.visualTarget=root.Position
   local validRange=distance>math.max(18,settings.farmDunkRange+6)and distance<=settings.farmShotRange
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
   local planted
   planted,lastShotMotion=stationaryShot(speed,root.AssemblyLinearVelocity.Y,action,attr(own,"CanMove"),now,lastShotMotion)
   if not planted or not ready then state.status="Stepback committed: waiting for planted feet / shot readiness";return end
   local ok,reason=requestStandingShot(own,root,service,now,true)
   if ok then
    confirmedStepback=false;stepbackShotUntil=0;lastShot=now;shotStart=now;shotSawAction=false
    state.shots=(state.shots or 0)+1;state.status="Stepback committed: shot requested"
   else state.status="Stepback committed: "..tostring(reason)end
   return
  end
  local finish=false -- Distance-based dunk request is handled before tactical decisions.
  local facingRim=flat(root.CFrame.LookVector):Dot(forward)>.7
  local retreatEnd=clampCourt(root.Position-forward*5.6)
  state.visualStepback=retreatEnd
  local retreatAllowed=settings.farmShoot and autoTimeOn and inRange and ready
   and now-lastStepback>(1.6-.8*math.clamp(settings.farmStepbackAggression or 70,0,100)/100) and action~="Dribbling" and attr(own,"CanDribble")~=false
   and flat(retreatEnd-hoop).Magnitude<=settings.farmShotRange and #list>0 and safeMove(root.Position,retreatEnd)
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
   plan.dir=Vector3.zero;plan.reason="Autofarm: setting feet"
   -- Never turn the settle timeout into permission to take a running shot.
   local planted
   planted,lastShotMotion=stationaryShot(speed,root.AssemblyLinearVelocity.Y,action,attr(own,"CanMove"),now,lastShotMotion)
   if not planted then state.status=wideOpen and "Wide open: stop and plant before shot"or "Shot blocked: waiting for fully planted feet";return end
   captureShot(root,list,ping,wideOpen and "wide-open shot"or "confirmed stepback")
   local ok,err=requestStandingShot(own,root,service,now)
   if ok then confirmedStepback=false;stepbackShotUntil=0;lastShot=now;shotStart=now;shotSawAction=false;state.shots=(state.shots or 0)+1;state.status="Open window: shot requested"
   else state.status="Shot input failed: "..tostring(err)end
   return
  end

  if choice=="stepback" and not perimeterNeeded then
   local ok,err=stepbackAndShoot(service,now)
   if not ok then state.status="Stepback / E input failed: "..tostring(err)end
   return
  end
  local options=evaluated
  local allRed=bestDrive(options)==nil
  if allRed or perimeterNeeded then allBlockedAt=allBlockedAt or now else allBlockedAt=nil end
  if allBlockedAt and now-allBlockedAt>.4 and now-resetScanAt>.75 and not spaceHeld and not shotStart then
   resetScanAt=now
   local outward=-forward
   local radius=settings.farmShotRange
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
         local gap=flat(pos-d.position).Magnitude
         if gap<2.8 then collision=true end
         local arrival=math.max(0,gap-2.1)/math.clamp(math.max(16,d.velocity.Magnitude),16,26)
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
    escapePlan={point=point,waypoint=resetWaypoint,at=now,gap=flat(point-root.Position).Magnitude,progressAt=now};retreat=nil;confirmedStepback=false;stepbackShotUntil=0
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
   plan.dir=space and space.direction or Vector3.zero
   state.visualTarget=space and space.point or root.Position
   plan.reason=space and "Create reset space"or "No collision-free escape"
   state.status=space and "Drive lanes blocked: increase separation, then reassess"or "No safe movement found"
   guardError=space and 60 or 0;return
  end
  -- Save crossovers for actual pressure; do not interrupt an uncontested run.
  if service and service.Dribble and settings.farmCrossover and distance>settings.farmDunkRange+2
   and currentGap<8 and not beaten and not laneClear
   and now-lastCross>.65 and action~="Dribbling" and attr(own,"CanDribble")~=false then
   crossover(own,service,dir:Dot(right)>=0 and 1 or -1,dir)
  end
  plan.dir=dir;plan.reason="Autofarm: driving"
  guardError=60


 end

 -- Optional crossover and stepback; actual landing uses the shared shot safety gate.
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
    threeRun=nil
    local ok,err=stepbackAndShoot(service,now)
    if not ok then cancelThree(service);state.status="Three combo input failed: "..tostring(err);return true end
    -- The ordinary landing controller now owns this attempt.
    return true
   end
   if shotStart then releaseShot(service)end;threeRun=nil;return true
  end
  if threeUsed>=threeBudget or now<threeNext then return false end
  state.threeWhy="Looking for an eligible three combo"
  local hoop=rimFor(own,root,now)
  if not hoop then return false end
  local distance=flat(root.Position-hoop).Magnitude
  local arc=settings.smartDribbleThreeDistance or 0
  -- Do not interrupt an immediate finish. Use a detected arc when available.
  if distance<=settings.farmDunkRange+6 or distance>settings.farmShotRange or(arc>0 and distance<arc-5)then state.threeWhy=string.format("Range blocked: %.1f studs, limit %.1f",distance,settings.farmShotRange);return false end
  local action=attr(own,"Action")
  if busy[action]or action=="Dribbling"or attr(own,"CanDribble")==false or spaceHeld or shotStart then return false end
  local retreatPoint=clampCourt(root.Position-flat(hoop-root.Position).Unit*5.6)
  local ping=0;pcall(function()ping=math.clamp(P:GetNetworkPing(),0,.35)end)
  if not safeMove(root.Position,retreatPoint)or flat(retreatPoint-hoop).Magnitude>settings.farmShotRange or not stepbackWindow(retreatPoint,defenders(own),ping)then
   state.threeWhy="No safe stepback landing window";return false
  end
  local hand=attr(own,"Hand")
  if hand~="R"and hand~="L"then return false end
  if not(api.GetAutoTime and api.GetAutoTime())then return false end
  threeNext=now+rng:NextNumber(4,8)
  -- The random timing supplies variety; do not discard an eligible attempt again.
  if api.CancelCombo then api.CancelCombo()end
  local ok=pcall(function()service.Dribble:Fire(hand=="R"and "Z"or "C",true,flat(hoop-root.Position).Unit)end)
  if not ok then state.status="Three combo: crossover input failed";return false end
  threeUsed=threeUsed+1;threeRun={stage="doubleX",next=now+.06,at=now}
  state.threeAttempts=threeUsed;state.threeBudget=threeBudget
  plan.dir=flat(hoop-root.Position).Unit;plan.reason="Autofarm: optional three crossover";guardError=60
  state.status=string.format("Three attempt %d/%d: sprint crossover",threeUsed,threeBudget)
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
 local bufferedMove=nil
 local bufferedAt=-math.huge
 local function planDuringDribble(own,root,now,service)
  local hoop=rimFor(own,root,now);if not hoop then return false end
  local list=defenders(own);local ping=0
  pcall(function()ping=math.clamp(P:GetNetworkPing(),0,.35)end)
  local options=candidatePlans(root,hoop,list,ping)
  state.planCandidates=options;state.planEvaluatedAt=now
  local best,straight,step=bestDrive(options),nil,nil
  for _,candidate in ipairs(options)do
   if candidate.name=="DRIVE RIM"then straight=candidate end
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
  state.status=bufferedMove and("Buffered "..bufferedMove.name..": still evaluating")or "Dribbling: reevaluating drive and shot opportunities"
  if bufferedMove or now-bufferedAt<.35 or not service or not service.Dribble then return true end
  local key,name
  if wantsStep and now-lastStepback>1.6 then key="XX";name="stepback"
  elseif not dunkOpen and best and straight and best.name~="DRIVE RIM"and best.score>straight.score+2 and settings.farmCrossover then
   local hand=attr(own,"Hand")
   if hand=="R"or hand=="L"then key=hand=="R"and "Z"or "C";name="sprint crossover"end
  end
  if key then
   local tag=attr(own,"lastDribble")or attr(own,"LastDribble")
   if key=="XX"then
    bufferedMove=nil;bufferedAt=now
    local ok,err=stepbackAndShoot(service,now)
    if not ok then state.status="Buffered stepback / E failed: "..tostring(err)end
    return true
   end
   local ok=pcall(function()service.Dribble:Fire(key,true,plan.dir)end)
   bufferedAt=now
   if ok then
    bufferedMove={name=name,key=key,at=now,tag=tag,origin=root.Position,sawEnd=false}
    if key=="XX"then lastStepback=now end
    state.status="Queued "..name..": awaiting native move"
   end
  end
  return true
 end
 local function think(now)
  state.visualTarget=nil;state.visualStepback=nil;state.visualDefender=nil;state.visualOffense=false;state.selectedCandidate=nil;state.planEvaluatedAt=nil;state.shotWhy=nil;state.gap=nil;state.closeout=nil
  plan.dir=nil;plan.reason=nil;settings._farmGuard=false;settings._farmAttack=false
  local own=model();local root=rootOf(own)
  -- No character is normal in a queue lobby; only treat it as the lobby once it has lasted a few seconds.
  if root then noCharacterSince=nil elseif not noCharacterSince then noCharacterSince=now end
  local noCharacter=not root and now-(noCharacterSince or now)>=3
  local matchFlag=own and attr(own,"InGame")
  local onCourt=own and attr(own,"OnCourt")
  if not testOpponent and state.chatExit then
   bufferedMove=nil
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
  if bufferedMove then
   local queued=bufferedMove
   local tag=attr(own,"lastDribble")or attr(own,"LastDribble")
   if not holdsBall(own)or now-queued.at>1.5 then bufferedMove=nil
   elseif action~="Dribbling"then queued.sawEnd=true
   elseif now-queued.at>.12 and(queued.sawEnd or(queued.tag~=nil and tag~=nil and tag~=queued.tag))then
    if queued.key=="XX"then retreat={at=now,origin=root.Position,seen=true};confirmedStepback=false end
    bufferedMove=nil
   end
  end
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
   bufferedMove=nil;escapePlan=nil;allBlockedAt=nil;perimeterNeeded=false
   openingUntil=now+.45
   cancelThree(nil);driveTransfer=nil
   -- Discard stale shot bookkeeping; never send a release into a new possession.
   shotStart=nil;shotSawAction=false
   settings._farmAttack=true
   if guarding then guarding=false;releaseGuard()end
   if service and service.Sprint then pcall(function()service.Sprint:Fire(true)end)end
  elseif not possession then openingUntil=0 end
  if shotStart and action=="Shooting"then
   if not shotSawAction then state.shotActionAt=now end
   shotSawAction=true
  end
  if shotStart and shotSawAction and action~="Shooting"then releaseShot(service)end
  if shotStart and ((shotSawAction and now-(state.shotActionAt or shotStart)>1.8)
   or(not shotSawAction and now-shotStart>(state.stepbackQueuedShot and 3 or 1.8)))then
   releaseShot(service);state.shotWhy="shot input not acknowledged or timed out"
  end
  -- Jump shots own movement until the shot action ends. Native finishes keep their approach.
  if (shotStart or action=="Shooting")and not spaceHeld and action~="Dunking"and action~="Layup"then
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
  if (settings.farmRandomThrees or threeRun)and not retreat and not confirmedStepback and not shotStart then
   settings._farmAttack=possession
   if randomThree(own,root,now,service)then return end
  end
  if possession and action=="Dribbling"and not retreat and not confirmedStepback then
   settings._farmAttack=true
   local clearing=clearDirection(own,root)
   if clearing then bufferedMove=nil;plan.dir=clearing;plan.reason="Autofarm: clearing ball";guardError=60;return end
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
  if bufferedMove and possession and action~="Dribbling"then
   settings._farmAttack=true
   local hoop=rimFor(own,root,now);local delta=hoop and flat(hoop-root.Position)
   plan.dir=bufferedMove.key=="XX"and Vector3.zero or(delta and delta.Magnitude>.1 and delta.Unit or Vector3.zero)
   plan.reason="Waiting for buffered move";guardError=60;return
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
  cancelThree(keyboard and keyboard.Services and keyboard.Services.InputService)
  running=false;testOpponent=nil;testPlayer=nil;state.testOpponent=nil;perimeterNeeded=false;escapePlan=nil;releaseSpace(nil);releaseShot(keyboard and keyboard.Services and keyboard.Services.InputService);retreat=nil;resetting=false;possessionOwner=nil;lastShotMotion=os.clock();settings.farm=false;settings._farmGuard=false;settings._farmAttack=false;plan.dir=nil
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
  if now-lastThink<.05 then return end
  lastThink=now
  local ok,err=pcall(think,now)
  if state.status~=lastPrinted and (not state.printAt or now-state.printAt>.5)then state.printAt=now; lastPrinted=state.status;print("[Autofarm] "..tostring(state.status)..(state.seen and " | "..state.seen or ""))end
  if ok then plan.at=now else plan.dir=nil;settings._farmGuard=false;settings._farmAttack=false;state.status="Autofarm error: "..tostring(err)end
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
