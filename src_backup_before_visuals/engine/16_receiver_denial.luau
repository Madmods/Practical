-- Nearest-opponent receiver denial: normal movement and existing loose-pass catch input.
do (function()
 settings.denyReceiver=false;settings.denyCatch=false
 settings.denyRange=18;settings.denyGap=2.5;settings.denySwitchMargin=3
 settings.denyDelay=.2;settings.denySmoothing=8;settings.denyDeadzone=1.2
 settings.denySoft=true;settings.denyLead=.03
 numericLimits.denyLead={0,.15}
 numericLimits.denyDelay={0,.6};numericLimits.denySmoothing={2,30};numericLimits.denyDeadzone={.3,3}
 numericLimits.denyRange={6,35};numericLimits.denyGap={1,5};numericLimits.denySwitchMargin={0,8}
 local marked,carrier,carrierAt,markAt
 local selected,picking,selectedName
 local pickDeadline
 local function restoreMouse()pickDeadline=nil end
 local previousPosition,previousAt,velocity
 local observations={}
 local state={status="Disabled",mode="Off ball"}
 local function clear(reason)
  if settings._cancelReceiverIntercept then settings._cancelReceiverIntercept()end
  marked=nil;carrier=nil;carrierAt=nil;markAt=nil
  previousPosition=nil;previousAt=nil;velocity=Vector3.zero
  observations={}
  settings._denyReceiverTarget=nil;settings._denyReceiverBallHandler=nil
  settings._denyReceiverFacePosition=nil;settings._denyReceiverFaceAt=nil
  settings._denyReceiverCatchAt=nil
  state={status=reason or "Disabled",mode="Off ball"}
 end
 clear()
 local function sameCourt(a,b)
  local n,m=attr(a,"CourtNumber"),attr(b,"CourtNumber")
  if n~=nil and m~=nil and n~=0 and m~=0 and n~=m then return false end
  local x,y=attr(a,"Court"),attr(b,"Court")
  return not(x~=nil and x~=0 and x~=""and y~=nil and y~=0 and y~=""and x~=y)
 end
 local function enemy(own,other)
  if not other or other==own or not rootOf(other)or not sameCourt(own,other)then return false end
  local a,b=attr(own,"Team"),attr(other,"Team")
  if a~=nil and b~=nil and a~=0 and b~=0 and a~=""and b~=""then return a~=b end
  local players=game:GetService("Players")
  local ownPlayer=players:GetPlayerFromCharacter(own)or P
  local otherPlayer=players:GetPlayerFromCharacter(other)or players:FindFirstChild(other.Name)
  return ownPlayer and otherPlayer and ownPlayer.Team~=nil and otherPlayer.Team~=nil and ownPlayer.Team~=otherPlayer.Team or false
 end
 local function choose(own,root,now)
  local characters=defenseCharacters();local occupied={};local present={}
  local activeCarrier,carrierDistance
  for _,other in ipairs(characters)do
   present[other]=true
   local held=attr(other,"Basketball");if held then occupied[held]=true end
   if enemy(own,other)and holdsBall(other)then
    local distance=flat(rootOf(other).Position-root.Position).Magnitude
    if distance<=80 and (not carrierDistance or distance<carrierDistance or other==carrier)then
     activeCarrier=other;carrierDistance=distance
    end
   end
  end
  if activeCarrier then carrier=activeCarrier;carrierAt=now
  elseif not carrier or not present[carrier]or not enemy(own,carrier)or now-(carrierAt or 0)>.85 then carrier=nil end
  if selected and(not present[selected]or not enemy(own,selected))then selected=nil;clear("Matchup unavailable; following ball handler")end
  if selected and holdsBall(selected)then carrier=selected;carrierAt=now end
  if not carrier or not selected then return nil,occupied end
  -- An explicit game-long matchup must not switch to the ball handler merely
  -- because the selected opponent moved outside the old proximity radius.
  return selected,occupied
 end
 api.SetReceiverMatchup=function(character)
  local own=model();local present=false
  for _,candidate in ipairs(defenseCharacters())do if candidate==character then present=true;break end end
  if not own or not present or not enemy(own,character)then return false,"Select an opposing player on your court"end
  clear();selected=character;selectedName=character.Name;picking=false;restoreMouse()
  settings.assignmentLock=false;lockedAssignment=nil
  return true,"Matchup selected: "..character.Name.." · same player on and off ball"
 end
 api.BeginReceiverSelection=function()
  pickDeadline=nil
  picking=true;return true,"Click an opposing player in the game to choose your matchup"
 end
 api.CancelReceiverSelection=function()picking=false;restoreMouse();return true end
 api.GetReceiverSelectionHover=function()
  if not picking then return nil end
  local hit=P:GetMouse().Target;local own=model()
  if not hit or not own then return nil end
  for _,character in ipairs(defenseCharacters())do
   if enemy(own,character)then
    local proxy=attr(character,"ProxyCharacter")
    if hit==character or hit:IsDescendantOf(character)or typeof(proxy)=="Instance"and(hit==proxy or hit:IsDescendantOf(proxy))then
     return character,typeof(proxy)=="Instance"and proxy or character
    end
   end
  end
 end
 api.CycleReceiverMatchup=function()
  local own=model();local candidates={}
  if not own then return false,"Character unavailable"end
  for _,character in ipairs(defenseCharacters())do if enemy(own,character)then candidates[#candidates+1]=character end end
  table.sort(candidates,function(a,b)return a.Name<b.Name end)
  if #candidates==0 then return false,"No opposing players detected on this court"end
  local index=1
  for i,character in ipairs(candidates)do if character==selected then index=i%#candidates+1;break end end
  return api.SetReceiverMatchup(candidates[index])
 end
 api.ClearReceiverMatchup=function()
  selected=nil;selectedName=nil;picking=false;restoreMouse();clear("Following ball handler")
  settings.assignmentLock=false;lockedAssignment=nil;target=nil
  return true,"Matchup cleared; Auto guard follows the ball handler"
 end
 local function requestCatch(own,root,receiver,occupied,now)
  settings._denyReceiverCatchAt=nil
  if not settings.denyCatch or not settings.steal then return false end
  local ball=settings._flightSelected;local motion=settings._flightVelocity
  if not ball or not ball.Parent or not motion or motion.Magnitude<3 or occupied[ball]then return false end
  local kind=tostring(attr(ball,"LastShotType")or ""):lower()
  if attr(ball,"MidShot")==true or kind:find("lob",1,true)or attr(ball,"Lob")==true or attr(ball,"SelfLob")==true then return false end
  local position=goalPosition(ball);if not position then return false end
  local toReceiver=flat(receiver-position);local horizontal=flat(motion)
  if horizontal.Magnitude<3 or horizontal:Dot(toReceiver)<=0 then return false end
  local receiverTime=math.clamp(toReceiver:Dot(horizontal)/math.max(horizontal:Dot(horizontal),.01),0,.7)
  if flat(position+motion*receiverTime-receiver).Magnitude>6 then return false end
  local toOwn=flat(root.Position-position)
  local catchTime=math.clamp(toOwn:Dot(horizontal)/math.max(horizontal:Dot(horizontal),.01),0,.45)
  local point=position+motion*catchTime
  if flat(point-root.Position).Magnitude>6 or point.Y-root.Position.Y<-.5 or point.Y-root.Position.Y>4 then return false end
  settings._denyReceiverCatchAt=now
  return true
 end
 settings._denyReceiverReset=clear
 settings._denyReceiverDirection=function(dt)
  local now=os.clock()
  if not settings.denyReceiver then clear();return nil end
  if settings._guardReleased or not bindingDown(guardKey)or Input:GetFocusedTextBox()then clear("Hold your guard key");return nil end
  local own=model();local root=rootOf(own)
  if not own or not root then clear("Waiting for your character");return nil end
  if holdsBall(own)then clear("You have possession");return nil end
  if not selected then clear("Select a matchup, or let Auto guard follow the ball");return nil end
  local action=attr(own,"Action")
  if attr(own,"CanMove")==false or action=="Shooting"or action=="Dunking"or action=="Stealing"then
   settings._denyReceiverCatchAt=nil;state.status="Game movement locked";return nil
  end
  local chosen,occupied=choose(own,root,now)
  if chosen~=marked then
   marked=chosen;markAt=now;previousPosition=nil;previousAt=nil;velocity=Vector3.zero;steering=Vector3.zero;observations={}
  end
  local receiverRoot=rootOf(marked);local carrierRoot=rootOf(carrier)
  settings._denyReceiverTarget=marked;settings._denyReceiverBallHandler=carrier
  settings._denyReceiverCatchAt=nil
  if not receiverRoot or not carrierRoot then
   guardAim=nil;guardClosing=false;guardError=0
   settings._denyReceiverFacePosition=nil;settings._denyReceiverFaceAt=nil
   state={status=carrierRoot and "No off-ball opponent in range"or "No enemy ball handler detected",mode="Off ball"}
   guardReason=state.status
   return nil
  end
  if marked==carrier and holdsBall(marked)then
   if settings._cancelReceiverIntercept then settings._cancelReceiverIntercept()end
   settings._denyReceiverTarget=nil;settings._denyReceiverFacePosition=nil;settings._denyReceiverFaceAt=nil
   if target~=marked then target=marked;targetSince=now-(settings.guardReaction or 0)end
   state={status=settings.guard and "Selected matchup has ball; Auto guard takes over"or "Selected matchup has ball; enable Auto guard",mode="On ball",target=marked.Name,handler=carrier.Name}
   return nil
  end
  if previousPosition and now-(previousAt or now)>=.02 and now-previousAt<.3 then
   local observed=flat(receiverRoot.Position-previousPosition)/(now-previousAt)
   if observed.Magnitude<40 then velocity=velocity:Lerp(observed,.4)else velocity=Vector3.zero end
  end
  if not previousAt or now-previousAt>=.02 then previousPosition=receiverRoot.Position;previousAt=now end
  local lead=settings.denySoft and(settings.denyLead or .03)or math.clamp(settings.guardPrediction or .08,.04,.2)+math.clamp(ping or 0,0,.12)*.5
  local predicted=receiverRoot.Position+velocity*lead
  local onBall=marked==carrier and holdsBall(marked)
  local source=not onBall and goalPosition(attr(carrier,"Basketball"))or nil
  source=source or (onBall and goalPosition(attr(marked,"Goal")))or carrierRoot.Position
  local axis=flat(source-predicted)
  if axis.Magnitude<.25 then
   axis=flat(root.Position-predicted)
   if axis.Magnitude<.25 then return Vector3.zero end
  end
  local gap=onBall and (settings.guardGap or 3)or math.min(settings.denyGap,axis.Magnitude*.45)
  local desired=clampCourt(predicted+axis.Unit*gap)
  -- Follow timestamped observations rather than reacting to the latest position.
  -- Keep one sample before the cutoff; never restart a timer on every frame.
  observations[#observations+1]={at=now,point=desired,face=carrierRoot.Position}
  local cutoff=now-(settings.denySoft and(settings.denyDelay or .2)or 0)
  while #observations>1 and observations[2].at<=cutoff do table.remove(observations,1)end
  while #observations>90 do table.remove(observations,1)end
  local sample=observations[1]
  if sample.at>cutoff then
   settings._denyReceiverFacePosition=nil;settings._denyReceiverFaceAt=nil
   settings._denyReceiverCatchAt=nil;steering=Vector3.zero
   state={status="Waiting for response delay",mode="Off ball",target=marked.Name,handler=carrier.Name}
   return Vector3.zero
  end
  desired=sample.point
  guardAim=desired;guardClosing=false
  local delta=flat(desired-root.Position);guardError=delta.Magnitude
  local catching=not onBall and requestCatch(own,root,receiverRoot.Position,occupied,now)
  settings._denyReceiverFacePosition=sample.face;settings._denyReceiverFaceAt=now
  state={status=catching and "Pass in your lane"or onBall and "Receiver caught: covering ball"or "Denying receiver",mode=onBall and "On ball"or "Off ball",
   target=marked.Name,handler=carrier.Name,distance=flat(receiverRoot.Position-root.Position).Magnitude,aim=desired,catchReady=catching}
  guardReason=state.status.." · "..marked.Name
  local tolerance=settings.denySoft and(settings.denyDeadzone or 1.2)or(settings.guardDeadzone or .45)
  if delta.Magnitude<=math.max(.3,tolerance)then steering=Vector3.zero;return steering end
  local response=settings.denySoft and(settings.denySmoothing or 8)or(8+(settings.guardStrength or 1)*24)
  local blend=1-math.exp(-math.clamp(dt or 1/60,0,.1)*response)
  steering=steering:Lerp(delta.Unit,blend)
  return steering.Magnitude>.001 and steering.Unit or Vector3.zero
 end
 api.GetReceiverDenialState=function()
  local result=table.clone(state);result.selected=selected and selected.Name;result.picking=picking==true
  return result
 end
 -- Match membership must be explicit: waiting on a join spot is not a game.
 local matchKey,matchCandidate,candidateAt,missingAt,readySince
 local matchPrompted=false
 local matchRevision=0
 api.GetMatchupPickerState=function()
  local own=model();local opponents={}
  if own then for _,character in ipairs(defenseCharacters())do
   if enemy(own,character)then opponents[#opponents+1]={character=character,name=character.Name}end
  end end
  table.sort(opponents,function(a,b)return a.name<b.name end)
  return {revision=matchRevision,active=matchKey~=nil,opponents=opponents,selected=selected and selected.Name}
 end
 local function updateMatch(now)
  local own=model()
  local function value(name)
   local v=own and attr(own,name);if v==nil then v=attr(P,name)end;return v
  end
  local inGame,onCourt=value("InGame"),value("OnCourt")
  local active=inGame==true or (inGame==nil and onCourt==true)
  local court=value("CourtNumber")or value("Court")
  local key=active and tostring(court or "current")or nil
  if key then
   missingAt=nil
   if key~=matchCandidate then matchCandidate=key;candidateAt=now;readySince=nil end
   if key~=matchKey and now-(candidateAt or now)>=1.5 then
    matchKey=key;matchPrompted=false
    selected=nil;selectedName=nil;picking=false;restoreMouse();clear("New game: choose your matchup")
    settings.assignmentLock=false;lockedAssignment=nil;target=nil
   end
   -- InGame/OnCourt become true during introductions. Membership alone must
   -- not arm world selection: wait for movement control and real opponents.
   local action=tostring(value("Action")or ""):lower()
   local ready=own and rootOf(own)and value("CanMove")==true
    and value("Intro")~=true and value("InIntro")~=true and value("Cutscene")~=true
    and (action==""or action=="idle"or action=="running"or action=="guarding"or action=="dribbling")
   local opponentReady=false
   if ready then for _,character in ipairs(defenseCharacters())do
    if enemy(own,character)then opponentReady=true;break end
   end end
   if ready and opponentReady then readySince=readySince or now else readySince=nil end
   if matchKey==key and not matchPrompted and readySince and now-readySince>=1.25 then
    matchPrompted=true;matchRevision=matchRevision+1
   end
  else
   matchCandidate=nil;candidateAt=nil;readySince=nil
   missingAt=missingAt or now
   if matchKey and now-missingAt>=3 then
    matchKey=nil;matchPrompted=false;selected=nil;selectedName=nil;picking=false;restoreMouse();clear("Game ended")
    settings.assignmentLock=false;lockedAssignment=nil;target=nil
   end
  end
 end
 local selectInput=Input.InputBegan:Connect(function(input,processed)
  if picking and input.KeyCode and input.KeyCode==Enum.KeyCode.Escape then picking=false;restoreMouse();return end
  if picking and input.UserInputType and input.UserInputType==Enum.UserInputType.MouseButton2 then picking=false;restoreMouse();return end
  if not picking or processed or Input:GetFocusedTextBox()or input.UserInputType~=Enum.UserInputType.MouseButton1 then return end
  local hit=P:GetMouse().Target
  if not hit then return end
  for _,character in ipairs(defenseCharacters())do
   local proxy=attr(character,"ProxyCharacter")
   local proxyHit=typeof(proxy)=="Instance"and(hit==proxy or hit:IsDescendantOf(proxy))
   if hit==character or hit:IsDescendantOf(character)or proxyHit then
    local ok,message=api.SetReceiverMatchup(character);state.status=message
    if ok then return end
   end
  end
 end)
 local connection=Run.Heartbeat:Connect(function()
  updateMatch(os.clock())
  if not selected and selectedName then
   local own=model()
   for _,candidate in ipairs(defenseCharacters())do
    if candidate.Name==selectedName and enemy(own,candidate)then selected=candidate;break end
   end
  end
  if picking and pickDeadline and os.clock()>=pickDeadline then picking=false;restoreMouse()end
  if selected then
   local own=model();local present=false
   for _,character in ipairs(defenseCharacters())do if character==selected then present=true;break end end
   if not own or not present or not enemy(own,selected)then selected=nil;picking=false;restoreMouse();clear("Matchup unavailable; following ball handler")end
  end
  if not settings.denyReceiver and marked then clear()end
 end)
 local selectionBlur=Input.WindowFocusReleased and Input.WindowFocusReleased.Connect and Input.WindowFocusReleased:Connect(function()
  picking=false;restoreMouse()
 end)
 local previous=featureCleanup
 featureCleanup=function()
  connection:Disconnect();selectInput:Disconnect();if selectionBlur then selectionBlur:Disconnect()end;selected=nil;picking=false;restoreMouse();clear();settings._denyReceiverDirection=nil;settings._denyReceiverReset=nil;previous()
 end
end)()end

