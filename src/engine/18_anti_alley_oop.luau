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

