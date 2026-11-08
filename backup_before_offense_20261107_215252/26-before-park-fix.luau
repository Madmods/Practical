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
 -- A dunk is the Space bar HELD while running at the rim with the ball. Attempt 1 holds the real key
 -- through VirtualInputManager; attempt 2 holds the same input through the game signals instead
 -- (in case the key press is not picked up); then it alternates. Released when the dunk is over.
 local spaceHeld,spaceMode,spaceStart,spaceSawDunk=false,nil,0,false
 local function releaseSpace(service)
  if not spaceHeld then return end
  spaceHeld=false
  if spaceMode=="key" then
   pcall(function()game:GetService("VirtualInputManager"):SendKeyEvent(false,Enum.KeyCode.Space,false,game)end)
  else
   service=service or(keyboard and keyboard.Services and keyboard.Services.InputService)
   if service and service.Shoot then pcall(function()service.Shoot:Fire({Shoot=false,UseSpace=true})end)end
  end
 end
 local function dunkAction(service,attempt)
  releaseSpace(service)
  -- Use the observed native Space hold only. A guessed Shoot/E fallback can become a fadeaway.
  game:GetService("VirtualInputManager"):SendKeyEvent(true,Enum.KeyCode.Space,false,game)
  spaceStart=os.clock();spaceSawDunk=false;spaceMode="key";spaceHeld=true
 end

 ----------------------------------------------------------------------------------------------

 settings.farm=false;settings.farmCrossover=true;settings.farmDunkRange=12
 settings.farmShoot=true;settings.farmShotRange=20
 numericLimits.farmDunkRange={6,24};numericLimits.farmShotRange={12,45}
 local CONFIG="VantaAutofarm-v1.json"
 local COUNTDOWN=5
 local plan={at=-math.huge}
 local busy={Shooting=true,Dunking=true,Stunned=true,Dead=true,Diving=true,Stealing=true,Blocking=true,Rebounding=true}
 local running,stopped=false,false
 local snapshot
 local lastReady,lastDunk,lastCross,lastCatch,lastThink=-math.huge,-math.huge,-math.huge,-math.huge,0
 local side,sideAt=1,-math.huge
 local rimCache,rimAt=nil,-math.huge
 local lastPrinted,noCharacterSince
 local lastCrossKeys,guarding
 local retreat,driveUntil,lastStepback=nil,0,-math.huge
 local stepbackShotUntil=0
 local possessionOwner=nil
 local function resetOffense(service)
  retreat=nil;driveUntil=0;stepbackShotUntil=0
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

 -- Auto guard, steals and rebounds are forced on while farming; your own values return afterwards.
 local function forceDefense(on)
  if on and not snapshot then
   snapshot={guard=settings.guard,steal=settings.steal,rebound=settings.rebound,reboundChase=settings.reboundChase,guardRange=settings.guardRange,antiOop=settings.antiOop,oopMove=settings.oopMove}
   settings.guard=true;settings.steal=true;settings.rebound=true;settings.reboundChase=true;settings.antiOop=true;settings.oopMove=true
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
  local list={}
  for _,character in ipairs(defenseCharacters())do
   local r=rootOf(character)
   if character~=own and character.Name~="InvisCharacter"and r and r.Position.Y>-100 then
    local a,b=attr(own,"Team"),attr(character,"Team")
    local courtA,courtB=attr(own,"CourtNumber"),attr(character,"CourtNumber")
    local otherCourt=courtA~=nil and courtB~=nil and courtA~=0 and courtB~=0 and courtA~=courtB
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
  if position.Y>root.Position.Y+1.5 and gravity>0 then
   local t=math.clamp((velocity.Y+math.sqrt(velocity.Y*velocity.Y+2*gravity*(position.Y-root.Position.Y)))/gravity,0,2.5)
   return Vector3.new(position.X+velocity.X*t,root.Position.Y,position.Z+velocity.Z*t)
  end
  local lead=flat(velocity)*.2
  return Vector3.new(position.X+lead.X,root.Position.Y,position.Z+lead.Z)
 end
 local rimCandidates,rimScanAt={},-math.huge
 local ballOwners=setmetatable({},{__mode="k"})
 local rimHits=setmetatable({},{__mode="k"})
 local rimWatches=setmetatable({},{__mode="k"})
 local clearBall,clearRim,clearTarget
 local function watchRim(part)
  if rimWatches[part]then return end
  rimWatches[part]=true
  on(part.Touched,function(hit)
   if not running then return end
   for _,ball in ipairs(ballCandidates())do
    if hit==ball or (ball.Parent and ball.Parent:IsA("Model")and hit:IsDescendantOf(ball.Parent))then
     local owner=ballOwners[ball]
     if owner and owner~=model()then rimHits[ball]={rim=part.Position,at=os.clock()}end
    end
   end
  end)
 end
 local function clearDirection(own,root)
  local ball=ballPart(attr(own,"Basketball"))
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
  local cross=hand=="R"and {keys="Z",right=-6.968,forward=.798}or{keys="C",right=8.405,forward=1.478}
  local spin=hand=="R"and {keys="CXZ",right=-5.017,forward=2.752}or{keys="ZXC",right=2.336,forward=2.974}
  local keys,bestScore
  for _,move in ipairs({cross,spin})do
   local endpoint=ownRoot.Position+flat(ownRoot.CFrame.RightVector)*move.right+flat(ownRoot.CFrame.LookVector)*move.forward
   local clearance=12
   for _,enemy in ipairs(opponents(own))do
    local er=rootOf(enemy)
    if er then clearance=math.min(clearance,flat(endpoint-er.Position-flat(er.AssemblyLinearVelocity)*.35).Magnitude)end
   end
   local score=clearance+move.forward*.3+(move==cross and 1.2 or 0)
   if move.keys==lastDriveMove then score=score-2 end
   local bounded=clampCourt(endpoint)
   if flat(bounded-endpoint).Magnitude>1 then score=score-20 end
   if not bestScore or score>bestScore then keys=move.keys;bestScore=score end
  end
  if bestScore<2 then return end
  lastDriveMove=keys
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
 -- BEGIN FARM POLICY
 local function farmChoice(c)
  if c.finish then return "finish" end
  if c.stepbackWindow and c.shotEnabled and c.inRange and c.open and c.ready and not c.dribbling then return "shoot" end
  if c.beaten or c.laneClear then return "drive" end
  if c.shotEnabled and c.inRange and c.open and c.ready and not c.dribbling then return "shoot" end
  if c.stepback then return "stepback" end
  return "create"
 end
 -- END FARM POLICY
 local function stationaryShot(speed,vertical,action,canMove,now,lastMotion)
  local moving=speed>.35 or math.abs(vertical)>1 or action=="Dribbling" or canMove==false
  if moving then return false,now end
  return true,lastMotion -- No extra timer after native movement has actually stopped.
 end
 local lastShotMotion=-math.huge
 local function releaseShot(service)
  shotStart=nil
  if service and service.Shoot then pcall(function()service.Shoot:Fire({Shoot=false})end)end
 end
 local function offense(own,root,now,service)
  local hoop=rimFor(own,root,now)
  if not hoop then state.status="Have ball, no rim found";return end
  local toHoop=flat(hoop-root.Position);local distance=toHoop.Magnitude
  -- Remember which way the court is, so a reset knows where "back out" is even from under the rim.
  if distance>8 then approachDir=(-toHoop).Unit end
  local forward=distance>.1 and toHoop.Unit or(approachDir and -approachDir)or Vector3.new(0,0,-1)
  local right=Vector3.new(-forward.Z,0,forward.X)
  local list=defenders(own)
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
  local laneClear=laneGap>=3.4
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
   if now-shotStart>1.8 or(not shotSawAction and now-shotStart>.45)then releaseShot(service)else plan.dir=Vector3.zero;plan.reason="Shooting";state.status="Shot up, waiting for release";return end
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
  local inRange=distance>math.max(8,settings.farmDunkRange) and distance<=settings.farmShotRange
  local open=currentGap>=5 and earliest>.48+ping
  local action=attr(own,"Action")
  local ready=attr(own,"CanShoot")~=false and now-lastShot>.75
  state.shotWhy=not settings.farmShoot and "shots disabled" or not autoTimeOn and "Auto time is off"
   or not inRange and "outside configured shot range" or not open and "defender can close before release"
   or not ready and "shot not ready" or "open window"
  state.closeout=earliest;state.distance=distance;state.gap=currentGap

  -- Let a measured retreat finish; do not override its glide with forward steering.
  if retreat then
   local elapsed=now-retreat.at
   if action=="Dribbling" then retreat.seen=true end
   local moved=flat(root.Position-retreat.origin).Magnitude
   local completed=(retreat.seen and action~="Dribbling")or(moved>2 and elapsed>.45 and action~="Dribbling")
   if completed or elapsed>1.15 then
    if completed then stepbackShotUntil=now+.45 end
    retreat=nil
   else
    plan.dir=Vector3.zero;plan.reason="Autofarm: stepback";state.status="Stepback: waiting for separation";return
   end
  end
  local finish=false -- Distance-based dunk request is handled before tactical decisions.
  local facingRim=flat(root.CFrame.LookVector):Dot(forward)>.7
  local retreatEnd=clampCourt(root.Position-forward*5.6)
  local retreatAllowed=settings.farmCrossover and settings.farmShoot and autoTimeOn and attr(own,"Hand")=="R"
   and facingRim and currentGap<6 and distance>=10 and flat(retreatEnd-hoop).Magnitude<=settings.farmShotRange
   and flat(retreatEnd-root.Position).Magnitude>4 and now-lastStepback>1.6 and action~="Dribbling"
  if retreatAllowed then
   for _,d in ipairs(list)do
    if segmentGap(d.position,root.Position,retreatEnd)<2 or flat(retreatEnd-d.predicted).Magnitude<5 then retreatAllowed=false end
   end
  end
  local choice=farmChoice({finish=finish,shotEnabled=settings.farmShoot and autoTimeOn,inRange=inRange,
   open=open,ready=ready,stepbackWindow=now<stepbackShotUntil,dribbling=action=="Dribbling",beaten=beaten,laneClear=laneClear,stepback=retreatAllowed})
  state.decision=choice
  if choice=="finish" and service and service.Shoot then
   plan.dir=forward;plan.reason="Autofarm: finishing";guardError=60
   dunkAttempts=dunkAttempts+1;lastDunk=now
   local ok,err=pcall(dunkAction,service,dunkAttempts)
   if ok then state.dunks=state.dunks+1;state.status="Attacking rim: dunk requested"
   else state.status="Dunk input failed: "..tostring(err)end
   return
  end
  if choice=="shoot" and service and service.Shoot then
   plan.dir=Vector3.zero;plan.reason="Autofarm: setting feet"
   -- Never turn the settle timeout into permission to take a running shot.
   local planted
   planted,lastShotMotion=stationaryShot(speed,root.AssemblyLinearVelocity.Y,action,attr(own,"CanMove"),now,lastShotMotion)
   if not planted then state.status="Shot blocked: waiting for fully planted feet";return end
   local ok,err=pcall(function()service.Shoot:Fire({Shoot=true,Input="E"})end)
   if ok then stepbackShotUntil=0;lastShot=now;shotStart=now;shotSawAction=false;state.shots=(state.shots or 0)+1;state.status="Open window: shot requested"
   else state.status="Shot input failed: "..tostring(err)end
   return
  end
  if choice=="stepback" and service and service.Dribble then
   if api.CancelCombo then api.CancelCombo()end
   local ok=pcall(function()service.Dribble:Fire("X",false,Vector3.zero)end)
   if ok then
    lastStepback=now;lastCross=now;retreat={at=now,origin=root.Position,seen=false}
    plan.dir=Vector3.zero;plan.reason="Autofarm: stepback";state.status="Pressure read: stepback for a shot";return
   end
  end

  local dir
  if laneClear then
   dir=forward;lastHeading=forward
   state.status=beaten and string.format("Defender beaten: straight to the dunk · %.0f studs out",distance) or string.format("Lane open: driving · %.0f studs out",distance)
  else
   -- Blocked. First try to get around; if the defender stays on us, back off to make room.
   local wantRim=1.8 -- Keep attacking useful lanes instead of backing away indefinitely.
   dir=chooseHeading(root,hoop,list,wantRim,runSpeed)
   state.status=string.format("Creating attacking angle (gap %.1f)",currentGap)
   local sideSign=dir:Dot(right)>=0 and 1 or -1
   if service and service.Dribble and currentGap<6 then
    -- Chain only when the previous action is finished; no blind per-frame spam.
    if settings.farmCrossover and distance>6 and now-lastCross>.18 and action~="Dribbling" and attr(own,"CanDribble")~=false then
     crossover(own,service,sideSign,dir)
    end
   end
  end
  plan.dir=dir;plan.reason="Autofarm: driving"
  guardError=60


 end

 local function think(now)
  plan.dir=nil;plan.reason=nil;settings._farmGuard=false;settings._farmAttack=false
  local own=model();local root=rootOf(own)
  -- No character is normal in a queue lobby; only treat it as the lobby once it has lasted a few seconds.
  if root then noCharacterSince=nil elseif not noCharacterSince then noCharacterSince=now end
  local noCharacter=not root and now-(noCharacterSince or now)>=3
  if not root and not noCharacter then state.phase="wait";state.status="Waiting for character";return end
  local inGame=root~=nil and(attr(own,"InGame")==true or attr(own,"OnCourt")==true or goalPosition(attr(own,"Goal"))~=nil)or env.VantaFarmForce==true
  if not inGame then
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
   if before then
    for _,rim in ipairs(allRims)do
     if before.Y>rim.Y and p.Y<=rim.Y and flat(p-rim).Magnitude<1.3 then madeUntil=now+8;state.makes=(state.makes or 0)+1 end
    end
   end
   lastBall[ball]=p
  end
  settings._guardReleased=false
  -- A profile switch in the menu must not turn the forced defence back off mid-farm.
  settings.guard=true;settings.steal=true;settings.rebound=true;settings.reboundChase=true;settings.antiOop=true
  local service=keyboard and keyboard.Services and keyboard.Services.InputService
  local action=attr(own,"Action")
  local motion= root.AssemblyLinearVelocity
  local _,motionAt=stationaryShot(flat(motion).Magnitude,motion.Y,action,attr(own,"CanMove"),now,lastShotMotion)
  lastShotMotion=motionAt
  if shotStart then plan.dir=Vector3.zero;plan.reason="Autofarm: shot movement locked" end
  local possession=holdsBall(own)
  if (possessionOwner~=own or not possession)and action~="Dunking" then
   resetOffense(service);lastHeading=nil;resetting=false;openSince=nil;settleSince=nil
   if shotStart and action~="Shooting"then releaseShot(service)end
  end
  possessionOwner=possession and own or nil
  if shotStart and action=="Shooting"then shotSawAction=true end
  if shotStart and shotSawAction and action~="Shooting"then releaseShot(service)end
  if shotStart and (now-shotStart>1.8 or(not shotSawAction and now-shotStart>.45))then releaseShot(service);state.shotWhy="shot input not acknowledged or timed out"end
  if spaceHeld then
   if action=="Dunking" then
    if not spaceSawDunk then state.dunkAcknowledged=(state.dunkAcknowledged or 0)+1 end
    spaceSawDunk=true
   end
   if not spaceSawDunk and now-spaceStart>1.0 then state.finishWhy="Space hold not acknowledged as a dunk" end
   if(spaceSawDunk and action~="Dunking")or now-spaceStart>1.1 or(not holdsBall(own)and action~="Dunking"and now-spaceStart>.6)then releaseSpace(service)end
  end
  -- Open shots are timed by Auto time, so make sure it is on while the farm runs.
  if settings.farmShoot and api.GetAutoTime and not api.GetAutoTime()and now-autoTimeTry>2 then
   autoTimeTry=now
   if api.SetAutoTime(true)then forcedAutoTime=true end
  end
  if possession and action=="Dribbling"and not retreat then
   settings._farmAttack=true
   local clearing=clearDirection(own,root)
   local hoop=rimFor(own,root,now)
   local delta=hoop and flat(hoop-root.Position)
   plan.dir=clearing or(delta and delta.Magnitude>.1 and delta.Unit)or Vector3.zero
   plan.reason="Autofarm: running through dribble";guardError=60
   if not clearing then state.status="Driving dribble: maintain sprint toward rim"end
   return
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
   -- Give up on a ball that is not getting closer (fence, out of bounds) or that takes too long.
   if best~=chaseBall then chaseBall=best;chaseSince=now;chaseGap=bestDistance;chaseGapAt=now end
   if now-chaseGapAt>2.5 then
    if chaseGap-bestDistance<2 then giveUp[best]=now+10 end
    chaseGap=bestDistance;chaseGapAt=now
   end
   if now-chaseSince>7 then giveUp[best]=now+10 end
   local aim=ballTarget(best,root);local delta=flat(aim-root.Position)
   plan.dir=delta.Magnitude>1.2 and delta.Unit or Vector3.zero;plan.reason="Autofarm: loose ball"
   -- Drop the defensive stance right away so the player runs instead of shuffling in guard.
   if guarding then guarding=false;releaseGuard()end
   guardError=60
   state.status=string.format("Chasing loose ball · %.0f studs",delta.Magnitude)
   if flat(best.Position-root.Position).Magnitude<5 and now-lastCatch>.3 and service and service.Steal then
    lastCatch=now;state.loose=state.loose+1
    pcall(function()if settings._requestLooseCatch then settings._requestLooseCatch()else service.Steal:Fire()end end)
   end
   return
  end
  chaseBall=nil
  -- Only guard while an opponent actually has the ball. A dead or out-of-bounds ball means stand down.
  local handlerHasBall=false
  for _,character in ipairs(opponents(own))do if holdsBall(character)then handlerHasBall=true;break end end
  if handlerHasBall then
   settings._farmGuard=true;guarding=true;state.status="Guarding the ball handler"
  else
   if guarding then guarding=false;releaseGuard()end
   state.status=scored and "Basket made: standing by" or #balls>0 and "Ball out of play: standing by" or "Nobody has the ball: standing by"
  end
 end

 local function disarm(persist)
  running=false;releaseSpace(nil);releaseShot(keyboard and keyboard.Services and keyboard.Services.InputService);retreat=nil;resetting=false;possessionOwner=nil;lastShotMotion=os.clock();settings.farm=false;settings._farmGuard=false;settings._farmAttack=false;plan.dir=nil
  if guarding then guarding=false;releaseGuard()end
  forceDefense(false)
  clearBall=nil;clearRim=nil;clearTarget=nil
  state.phase="off";state.status="Off"
  if persist then saveConfig(false)end
 end
 local function arm()
  if running then return end
  running=true;settings.farm=true
  forceDefense(true);saveConfig(true);queueReexec()
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
   countdownToken=countdownToken+1;closePrompt()
   arm();return true,"Autofarm on (saved)"
  end
  countdownToken=countdownToken+1;closePrompt()
  disarm(true);return true,"Autofarm off (saved)"
 end
 api.FarmReadyNow=function()
  lastReady=-math.huge
  if not running then return false,"Turn Autofarm on first"end
  return true,"Ready will be sent on the next tick if you are in the lobby"
 end
 api.GetFarmState=function()
  local result=table.clone(state);result.running=running;result.enabled=settings.farm
  return result
 end

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
  return plan.dir,plan.reason
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
  stopped=true;countdownToken=countdownToken+1;closePrompt()
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
