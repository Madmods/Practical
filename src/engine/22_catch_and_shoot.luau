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

