-- Drive cutoff, screen routing, and predicted pass interception.
do (function()
 settings.driveCutoff=false;settings.screenNavigation=false;settings.passIntercept=false
 settings.passInterceptAuto=false;settings.passInterceptMove=true;settings.passInterceptJump=false;settings.passInterceptShots=false
 settings.passInterceptReach=4;numericLimits.passInterceptReach={2,6}
 local state={intercept="idle",screen=nil,cutoff=false}
 api.GetDefenseNavigationState=function()return state end
 local activeIntent
 local requestUntil=0
 local currentFlight
 local lastNative=-math.huge
 local releaseJumpAt
 local screenSide,screenHeldUntil,screenObstacle
 local service
 local originalSteal
 local stealWrapper
 local function ballHeld(ball)
  for _,m in ipairs(defenseCharacters())do if attr(m,"Basketball")==ball then return true end end
  return false
 end
 local function flightCandidate(now)
  local own=model();local r=rootOf(own)
  if not r or holdsBall(own) or not settings._flightSelected or not settings._flightVelocity then return nil end
  local ball=settings._flightSelected
  local kind=tostring(attr(ball,"LastShotType")or ""):lower()
  if settings.antiOop and (kind:find("lob",1,true) or attr(ball,"Lob")==true or attr(ball,"SelfLob")==true or settings._oopOwnsBall and settings._oopOwnsBall(ball))then return nil end
  if not ball.Parent or ballHeld(ball) or (not settings.passInterceptShots and ball:GetAttribute("MidShot")==true)then return nil end
  local p=goalPosition(ball);local v=settings._flightVelocity
  if not p or v.Magnitude<3 or flat(p-r.Position).Magnitude>18 then return nil end
  local best,bestTime,bestDistance
  -- Short linear extrapolation is conservative for passes; no server trajectory is claimed.
  for i=0,15 do
   local t=i*.04;local point=p+v*t
   local horizontal=flat(point-r.Position).Magnitude
   local height=point.Y-r.Position.Y
   local maxHeight=settings.passInterceptJump and 8 or 4
   if height>-.5 and height<maxHeight then
    local required=math.max(0,horizontal-settings.passInterceptReach)
    local reachable=required<=14*t+.75
    if reachable and (not bestDistance or horizontal<bestDistance)then best=point;bestTime=t;bestDistance=horizontal end
   end
  end
  if not best then return nil end
  return {ball=ball,point=best,time=bestTime,distance=bestDistance,position=p,velocity=v,at=now}
 end
 local function arm(now,denial)
  local candidate=flightCandidate(now)
  if not candidate then state.intercept="No reachable moving loose ball";return false end
  activeIntent={ball=candidate.ball,untilTime=now+1.1,denial=denial==true};state.intercept="Tracking intercept";return true
 end
 settings._cancelReceiverIntercept=function()
  if activeIntent and activeIntent.denial then activeIntent=nil;settings.interceptDriving=false end
 end
 local function bindService()
  local nextService=keyboard and keyboard.Services and keyboard.Services.InputService
  if not nextService or not nextService.Steal or type(nextService.Steal.Fire)~="function"then return false end
  if service==nextService and service.Steal.Fire==stealWrapper then return true end
  if service and service.Steal.Fire==stealWrapper then service.Steal.Fire=originalSteal end
  service=nextService;originalSteal=service.Steal.Fire
  local native=originalSteal
  settings._requestLooseCatch=function()
   if settings.steal or Input:IsKeyDown(Enum.KeyCode.R)then return native(nextService.Steal)end
  end
  stealWrapper=function(self,...)
   if settings.passIntercept and Input:IsKeyDown(Enum.KeyCode.R)and not Input:GetFocusedTextBox()then
    requestUntil=os.clock()+1.2;arm(os.clock());return
   end
   lastNative=os.clock();return native(self,...)
  end
  service.Steal.Fire=stealWrapper;return true
 end
 bindService()
 on(Input.InputBegan,function(input,processed)
  if not processed and input.KeyCode==Enum.KeyCode.R and settings.passIntercept and not Input:GetFocusedTextBox()then
   requestUntil=os.clock()+1.2;arm(os.clock())
  end
 end)
 settings._interceptDirection=function()
  settings.interceptDriving=false
  if activeIntent and not settings.steal and not Input:IsKeyDown(Enum.KeyCode.R)and os.clock()>=requestUntil then activeIntent=nil;settings.interceptDriving=false;return nil end
  if activeIntent and activeIntent.denial and (not settings.steal or not settings.denyReceiver or not settings.denyCatch or settings._guardReleased or not bindingDown(guardKey))then activeIntent=nil;return nil end
  local denial=activeIntent and activeIntent.denial
  if not(settings.passIntercept or denial)or not(denial or settings.passInterceptMove)or not activeIntent or not currentFlight or currentFlight.ball~=activeIntent.ball or os.clock()-currentFlight.at>.15 or manualTakeover(os.clock()) or Input:GetFocusedTextBox()then return nil end
  local own=model();local r=rootOf(own)
  if not r or holdsBall(own)or attr(own,"CanMove")==false then return nil end
  local action=attr(own,"Action");if action=="Shooting"or action=="Dunking"or action=="Stealing"then return nil end
  local delta=flat(currentFlight.point-r.Position)
  guardError=delta.Magnitude;settings.interceptDriving=true
  return delta.Magnitude>1 and delta.Unit or Vector3.zero
 end
 settings._defenseRoute=function(own,other,desired)
  state.cutoff=false;state.screen=nil
  local r,dr=rootOf(own),rootOf(other)
  if not r or not dr then return desired end
  if settings.driveCutoff and holdsBall(other) and not guardClosing then
   local goal=goalPosition(attr(other,"Goal"));local direction=goal and flat(goal-dr.Position)
   local velocity=flat(targetVelocity or Vector3.zero)
   if direction and direction.Magnitude>.1 and velocity:Dot(direction.Unit)>1 then
    local prediction=dr.Position+velocity*.18
    local toGoal=flat(goal-prediction)
    if toGoal.Magnitude>.1 then
     local point=prediction+toGoal.Unit*settings.guardGap
     local adjustment=flat(point-desired)
     if adjustment.Magnitude>2.5 then adjustment=adjustment.Unit*2.5 end
     desired=clampCourt(desired+adjustment);state.cutoff=true
    end
   end
  end
  if not settings.screenNavigation then return desired end
  local path=flat(desired-r.Position)
  if path.Magnitude<1 then return desired end
  local meTeam=attr(own,"Team")
  for _,m in ipairs(defenseCharacters())do
   local sr=rootOf(m);local team=attr(m,"Team")
   local same=meTeam~=nil and meTeam~=0 and team==meTeam
   if m~=own and m~=other and sr and not same and (attr(m,"Screening")==true or attr(m,"Action")=="Screening" or attr(m,"Action")=="Screen" or flat(sr.Position-r.Position).Magnitude<7)then
    local t=math.clamp(flat(sr.Position-r.Position):Dot(path)/math.max(path:Dot(path),.01),0,1)
    local separation=flat(sr.Position-(r.Position+path*t)).Magnitude
    if t>=0 and t<=1 and separation<3.8 and flat(sr.Position-r.Position).Magnitude<13 then
     local perpendicular=Vector3.new(-path.Unit.Z,0,path.Unit.X)
     local left=sr.Position+perpendicular*4.6+path.Unit*1.8
     local right=sr.Position-perpendicular*4.6+path.Unit*1.8
     local lcost=flat(left-r.Position).Magnitude+flat(desired-left).Magnitude
     local rcost=flat(right-r.Position).Magnitude+flat(desired-right).Magnitude
     local params=RaycastParams.new();params.FilterType=Enum.RaycastFilterType.Exclude
     local characters=workspace:FindFirstChild("Characters")
     local excluded={visualFolder};if characters then table.insert(excluded,characters)end
     local proxy=workspace:FindFirstChild("ProxyCharacter");if proxy then table.insert(excluded,proxy)end
     params.FilterDescendantsInstances=excluded
     local leftBlocked=workspace:Raycast(r.Position,left-r.Position,params)~=nil
     local rightBlocked=workspace:Raycast(r.Position,right-r.Position,params)~=nil
     if leftBlocked and rightBlocked then return desired end
     if leftBlocked then screenSide=-1;screenHeldUntil=os.clock()+.65
     elseif rightBlocked then screenSide=1;screenHeldUntil=os.clock()+.65
     elseif screenObstacle~=m or not screenSide or os.clock()>(screenHeldUntil or 0)then screenSide=lcost<=rcost and 1 or -1;screenHeldUntil=os.clock()+.65 end
     screenObstacle=m
     state.screen=m.Name
     return clampCourt(screenSide==1 and left or right)
    end
   end
  end
  return desired
 end
 local last=0
 on(Run.Heartbeat,function()
  local now=os.clock()
  bindService()
  if releaseJumpAt and now>=releaseJumpAt then releaseJumpAt=nil;pcall(function()service.Shoot:Fire({Shoot=false,UseSpace=true})end)end
  if now-last<.03 then return end;last=now
  local denialHeld=settings.steal and settings.denyReceiver and settings.denyCatch and not settings._guardReleased and bindingDown(guardKey)
  local denialWanted=denialHeld and settings._denyReceiverCatchAt and now-settings._denyReceiverCatchAt<.2
  if activeIntent and activeIntent.denial and not denialHeld then activeIntent=nil;settings.interceptDriving=false end
  local interceptEnabled=settings.passIntercept or denialWanted or denialHeld and activeIntent and activeIntent.denial
  currentFlight=interceptEnabled and flightCandidate(now)or nil
  if not service or not originalSteal then state.intercept="Native input unavailable";return end
  if not interceptEnabled then activeIntent=nil;requestUntil=0;state.intercept="disabled";return end
  if Input:GetFocusedTextBox()then activeIntent=nil;state.intercept="Paused while typing";return end
  local manualRequested=Input:IsKeyDown(Enum.KeyCode.R)or now<requestUntil
  local requested=settings.passInterceptAuto and settings.steal or manualRequested or denialWanted
  if activeIntent and not settings.steal and not manualRequested then activeIntent=nil;settings.interceptDriving=false;state.intercept="Automatic steals off · manual R remains available"end
  if not activeIntent and requested and now-math.max(lastNative,lastSteal)>1.4 and currentFlight then arm(now,denialWanted and not settings.passInterceptAuto and not Input:IsKeyDown(Enum.KeyCode.R))end
  if not activeIntent then state.intercept=requested and "Waiting for reachable loose flight"or "Hold R or enable automatic attempts"end
  if not activeIntent then return end
  if settings._oopOwnsBall and settings._oopOwnsBall(activeIntent.ball)then activeIntent=nil;state.intercept="Yielding lob to anti alley-oop";return end
  local own=model();local r=rootOf(own)
  if not r or holdsBall(own)or now>activeIntent.untilTime or ballHeld(activeIntent.ball)then activeIntent=nil;state.intercept="finished";return end
  local action=attr(own,"Action")
  if action=="Shooting"or action=="Dunking"or action=="Blocking"or action=="Rebounding"or action=="Stealing"then return end
  if not currentFlight or currentFlight.ball~=activeIntent.ball then return end
  if Input:GetFocusedTextBox()or attr(own,"CanMove")==false or now-math.max(lastNative,lastSteal)<1.4 then return end
  local delta=currentFlight.position-r.Position
  local lead=math.clamp(.1+(ping or 0),.08,.22)
  local projected=currentFlight.position+(currentFlight.velocity or Vector3.zero)*lead-r.Position
  local near=flat(delta).Magnitude<=settings.passInterceptReach or (flat(projected).Magnitude<=settings.passInterceptReach and projected.Y>-.5 and projected.Y<4)
  if near and delta.Y>-.5 and delta.Y<4 then
   lastNative=now;lastSteal=now;originalSteal(service.Steal);recordDefense("attempt_sent",{action="Pass intercept"});state.intercept="Native intercept requested · possession unconfirmed";activeIntent=nil;requestUntil=0
  elseif delta.Y>=4 then
   state.intercept="High pass: waiting for reachable catch height"
  end
 end)
 local previousCleanup=featureCleanup
 featureCleanup=function()
  settings._interceptDirection=nil;settings._defenseRoute=nil;settings._requestLooseCatch=nil;settings._cancelReceiverIntercept=nil
  if service and service.Steal.Fire==stealWrapper then service.Steal.Fire=originalSteal end
  if releaseJumpAt then pcall(function()service.Shoot:Fire({Shoot=false,UseSpace=true})end)end
  activeIntent=nil;previousCleanup()
 end
end)() end


