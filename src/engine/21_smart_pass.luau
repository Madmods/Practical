-- Smart pass: choose a native gesture for the user's number-key receiver.
do (function()
 settings.smartPass=false;settings.smartPassSpin=false
 local styles={"Overhead","Chest","Bounce","Spin"}
 local state={status="Waiting for native pass controller",passes=0,slots={}}
 local slots={};local hook;local stopped=false
 local recentKey,recentAt=nil,0
 local emitting=false;local generation=0;local transaction=nil
 local keys={Enum.KeyCode.One,Enum.KeyCode.Two,Enum.KeyCode.Three,Enum.KeyCode.Four}
 local actionName="VantaSmartPassGesture"
 local CAS=game:GetService("ContextActionService")
 local VIM=game:GetService("VirtualInputManager")
 local function team(a,b)
  if not a or not b or a==b then return false end
  local x,y=attr(a,"Team"),attr(b,"Team")
  return x~=nil and y~=nil and x~=0 and x~=""and x==y
 end
 local function receiver(args,own)
  local found={};local seen={}
  local function visit(value,depth)
   if depth>5 then return end
   if typeof(value)=="Instance"then
    local character=value:IsA("Player")and value.Character or value
    if rootOf(character)and team(own,character)then found[character]=true end
   elseif type(value)=="string"then
    for _,character in ipairs(defenseCharacters())do if value==character.Name and team(own,character)then found[character]=true end end
   elseif type(value)=="table"and not seen[value]then
    seen[value]=true;for _,v in pairs(value)do visit(v,depth+1)end
   end
  end
  visit(args,0)
  local only
  for character in pairs(found)do if only then return nil end;only=character end
  return only
 end
 local function choose(own,target)
  local r=rootOf(own);local tr=rootOf(target)
  if not r or not tr then return nil end
  local origin=r.Position+Vector3.new(0,1,0)
  local distance=flat(tr.Position-r.Position).Magnitude
  local ground=r.Position.Y-2.5
  if api.GetVisualGroundPoint then local ok,p=pcall(api.GetVisualGroundPoint,r.Position);if ok and p then ground=p.Y end end
  local defenders={}
  local ownTeam=attr(own,"Team")
  for _,other in ipairs(defenseCharacters())do
   local dr=rootOf(other);local t=attr(other,"Team")
   local court,otherCourt=attr(own,"CourtNumber"),attr(other,"CourtNumber")
   if other~=own and dr and ownTeam~=nil and ownTeam~=0 and t~=nil and t~=0 and ownTeam~=t
    and(not court or not otherCourt or court==0 or otherCourt==0 or court==otherCourt)then
    defenders[#defenders+1]={character=other,position=dr.Position,velocity=dr.AssemblyLinearVelocity or Vector3.zero,
     high=attr(other,"HoldingG")==true or attr(other,"Action")=="Blocking",low=attr(other,"Action")=="Stealing"}
   end
  end
  local best,bestScore
  for _,name in ipairs(styles)do
   if name~="Spin"or settings.smartPassSpin then
    local speed=(settings.passSpeed or 80)*(name=="Chest"and 1.15 or name=="Bounce"and .85 or name=="Spin"and .95 or 1)
    local travel=distance/speed
    local destination=tr.Position+Vector3.new(0,1,0)+flat(tr.AssemblyLinearVelocity or Vector3.zero)*math.min(travel,.3)
    local risk=0
    for i=1,15 do
     local t=i/16;local point=origin+(destination-origin)*t
     if name=="Overhead"then point=point+Vector3.new(0,math.sin(math.pi*t)*4,0)
     elseif name=="Bounce"then
      local y=t<.5 and origin.Y+(ground+.45-origin.Y)*t*2 or ground+.45+(destination.Y-ground-.45)*(t-.5)*2
      point=Vector3.new(point.X,y,point.Z)
     end
     for _,d in ipairs(defenders)do
      local v=flat(d.velocity);if v.Magnitude>28 then v=v.Unit*28 end
      local pos=d.position+v*math.min(travel*t,.3)
      local height=point.Y-pos.Y
      if height>=(d.low and -3 or -1.8)and height<=(d.high and 6 or 3.8)then
       risk=math.max(risk,math.clamp(1-flat(point-pos).Magnitude/3,0,1))
      end
     end
    end
    local catchContest=settings._contestModel.evaluate(tr.Position,defenders,travel+(settings.catchRelease or .5),tr.AssemblyLinearVelocity)
    local score=risk*10+catchContest*.025+travel*.8+(name=="Chest"and 0 or name=="Overhead"and .08 or name=="Spin"and .2 or .14)
    if not bestScore or score<bestScore then best=name;bestScore=score end
   end
  end
  return best,bestScore
 end

 local function passGesture(style)
  return {shift=style=="Chest"or style=="Spin",taps=(style=="Bounce"or style=="Spin")and 2 or 1}
 end
 -- Run only the native input sequence; no pass payload or receiver is fabricated.
 local function emitGesture(key,style,send,wait,cancelled)
  local gesture=passGesture(style)
  local modifier=false;local down=false
  local ok,err=pcall(function()
   if cancelled()then return end
   if gesture.shift then send(true,Enum.KeyCode.LeftShift);modifier=true end
   for i=1,gesture.taps do
    if cancelled()then break end
    send(true,key);down=true;wait(.025)
    send(false,key);down=false
    if i<gesture.taps then wait(.055)end
   end
  end)
  -- Always release our synthetic keys, including cancellation and input failures.
  if down then pcall(send,false,key)end
  if modifier then pcall(send,false,Enum.KeyCode.LeftShift)end
  return ok,err
 end
 local function numberHeld()
  if transaction and os.clock()<transaction.untilAt then return transaction.key end
  for _,key in ipairs(keys)do if Input:IsKeyDown(key)then return key end end
  if recentKey and os.clock()-recentAt<.4 then return recentKey end
 end
 local function bind()
  local service=keyboard and keyboard.Services and keyboard.Services.InputService
  local signal=service and service.Pass
  if not signal or type(signal.Fire)~="function"then state.status="Native pass controller unavailable";return end
  if hook and hook.signal==signal then return end
  if hook and hook.signal.Fire==hook.wrapper then hook.signal.Fire=hook.original end
  slots={};state.slots={}
  local original=signal.Fire
  local wrapper=function(self,...)
   if not stopped then
    local args=table.pack(...)
    pcall(function()
     local own=model();local target=own and receiver(args,own);local key=numberHeld()
     if target and key then
      slots[key]=target;state.slots[key.Name]=target.Name
      if transaction and os.clock()<transaction.untilAt and key==transaction.key then
       if not transaction.recorded then state.passes+=1;transaction.recorded=true end
       state.choice=transaction.style;state.target=target.Name
       state.status=transaction.style.." input sent to "..target.Name.." (native receiver)"
       if target~=transaction.target then state.status="Receiver slot changed: native receiver preserved; mapping refreshed"end
      else state.status="Receiver "..key.Name..": "..target.Name.." ready for Smart pass"end
     elseif settings.smartPass then state.status="Receiver unresolved: native pass preserved"end
    end)
   end
   return original(self,...)
  end
  local ok,err=pcall(function()signal.Fire=wrapper end)
  if ok then hook={signal=signal,original=original,wrapper=wrapper};state.status="Native gestures ready; receiver slots resolve from normal passes"
  else state.status="Pass observer unavailable: "..tostring(err)end
 end
 local held={}
 local function onNumber(_,inputState,event)
  if emitting then return Enum.ContextActionResult.Pass end
  if inputState~=Enum.UserInputState.Begin then
   if held[event.KeyCode]then
    if inputState==Enum.UserInputState.End or inputState==Enum.UserInputState.Cancel then held[event.KeyCode]=nil end
    return Enum.ContextActionResult.Sink
   end
   return Enum.ContextActionResult.Pass
  end
  recentKey=event.KeyCode;recentAt=os.clock()
  if stopped or not settings.smartPass or Input:GetFocusedTextBox()then return Enum.ContextActionResult.Pass end
  local own=model();if not own or not holdsBall(own)or not hook then return Enum.ContextActionResult.Pass end
  -- Explicit modified passes remain manual, including sprint-held Shift.
  if Input:IsKeyDown(Enum.KeyCode.LeftShift)or Input:IsKeyDown(Enum.KeyCode.RightShift)then
   state.status="Manual Shift pass preserved";return Enum.ContextActionResult.Pass
  end
  local target=slots[event.KeyCode]
  if not target or not target.Parent or not rootOf(target)or not team(own,target)then
   state.status="Receiver slot not identified: normal pass preserved"
   return Enum.ContextActionResult.Pass
  end
  if transaction and os.clock()<transaction.untilAt then return Enum.ContextActionResult.Sink end
  local style=choose(own,target)
  if not style then return Enum.ContextActionResult.Pass end
  held[event.KeyCode]=true;generation+=1
  local token=generation
  transaction={key=event.KeyCode,style=style,target=target,untilAt=os.clock()+.7}
  state.status="Selecting "..style.." for "..target.Name
  task.defer(function()
   if stopped or token~=generation or not settings.smartPass then transaction=nil;return end
   emitting=true
   local ok,err=emitGesture(event.KeyCode,style,function(down,key)
    VIM:SendKeyEvent(down,key,false,game)
   end,task.wait,function()return stopped or token~=generation or not settings.smartPass end)
   emitting=false
   if not ok then transaction=nil;state.status="Native key input failed: "..tostring(err)end
  end)
  return Enum.ContextActionResult.Sink
 end
 CAS:BindActionAtPriority(actionName,onNumber,false,10000,table.unpack(keys))
 local input=Input.InputBegan:Connect(function(event,processed)
  if emitting or Input:GetFocusedTextBox()then return end
  if table.find(keys,event.KeyCode)then recentKey=event.KeyCode;recentAt=os.clock()end
 end)
 api.GetSmartPassState=function()
  local out=table.clone(state);out.slots=table.clone(state.slots);return out
 end
 local lastBind=-math.huge;local rosterStamp
 local connection=Run.Heartbeat:Connect(function()
  if stopped or os.clock()-lastBind<.5 then return end;lastBind=os.clock()
  local own=model();local names={}
  if own then
   for _,character in ipairs(defenseCharacters())do if team(own,character)then names[#names+1]=character.Name end end
  end
  table.sort(names)
  local stamp=tostring(own)..":"..tostring(own and attr(own,"CourtNumber"))..":"..tostring(own and attr(own,"InGame"))..":"..table.concat(names,",")
  if rosterStamp~=stamp then slots={};state.slots={};rosterStamp=stamp end
  local ok,err=pcall(bind);if not ok then state.status="Pass binding unavailable: "..tostring(err)end
 end)
 local previous=featureCleanup
 featureCleanup=function()
  stopped=true;generation+=1;CAS:UnbindAction(actionName);input:Disconnect();connection:Disconnect()
  if hook then pcall(function()if hook.signal.Fire==hook.wrapper then hook.signal.Fire=hook.original end end)end
  previous()
 end
end)()end
