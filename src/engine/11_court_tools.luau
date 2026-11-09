-- Native steal requests; experimental touch and resizing paths removed.
settings._stealAttempt=function(service)service.Steal:Fire()end

-- Additional court tools. Runs in an IIFE to preserve the engine's local limit.
do (function()
 settings.ballRecovery=false;settings.recoveryRange=12;settings.recoveryWindow=3
 settings.passingLanes=false;settings.laneRange=60;settings.laneClearance=3
 settings.screenAwareness=false;settings.screenRange=7
 settings.comboBuilder=false;settings.comboInterval=.25;settings.ballTrail=false
 for key,limit in pairs({recoveryRange={4,24},recoveryWindow={1,5},laneRange={10,100},laneClearance={1,6},screenRange={3,12},comboInterval={.08,1}}) do numericLimits[key]=limit end
 local state={recovery="idle",combo="idle",lanes={},screen=nil,opportunity=nil}
 local combo={"Z","X","C","V"}
 local comboKey=Enum.KeyCode.J
 local sequenceText="Z@0.20 > XX@0.25 > C@0.20"
 settings.comboRepeats=1;settings.comboSprint=true;numericLimits.comboRepeats={1,4}
 local token=0
 local alive=true
 local rootFolder=Instance.new("Folder");rootFolder.Name="PracticalCourtTools";rootFolder.Parent=workspace
 local laneParts={}
 local trails={}
 local recent={}
 local recentUntil=0
 local function teammate(a,b)
  local pa=game:GetService("Players"):FindFirstChild(a.Name)
  local pb=game:GetService("Players"):FindFirstChild(b.Name)
  if pa and pb and pa.Team~=nil and pb.Team~=nil then return pa.Team==pb.Team end
  local x,y=attr(a,"Team"),attr(b,"Team")
  return x~=nil and y~=nil and x~=0 and x~="" and x==y
 end
 local function ballPart(value)
  if typeof(value)~="Instance" then return nil end
  if value:IsA("BasePart") then return value end
  if value:IsA("Model") then return value.PrimaryPart or value:FindFirstChildWhichIsA("BasePart",true) end
 end
 local function liveCharacters()
  local list={}
  for _,m in ipairs(defenseCharacters()) do
   local r=rootOf(m)
   if m.Name~="InvisCharacter" and r and r.Position.Y>-100 then list[#list+1]=m end
  end
  return list
 end
 local function noteSteal()
  if not settings.ballRecovery then return end
  local me=model();local r=rootOf(me);if not r then return end
  recent={};recentUntil=os.clock()+settings.recoveryWindow
  for _,m in ipairs(liveCharacters()) do
   local ball=ballPart(attr(m,"Basketball"))
   if m~=me and ball and (ball.Position-r.Position).Magnitude<=settings.recoveryRange then recent[ball]=true end
  end
 end
 local service=keyboard and keyboard.Services and keyboard.Services.InputService
 local stealSignal=service and service.Steal
 local oldFire=stealSignal and stealSignal.Fire
 local wrappedFire
 if oldFire then
  wrappedFire=function(self,...) noteSteal();return oldFire(self,...) end
  stealSignal.Fire=wrappedFire
 end
 local oldAttempt=settings._stealAttempt
 settings._stealAttempt=function(...) noteSteal();return oldAttempt(...) end
 settings._recoveryDirection=function()
  state.recovery="idle"
  if not settings.ballRecovery or os.clock()>recentUntil or Input:GetFocusedTextBox() or manualTakeover(os.clock()) then return nil end
  local me=model();local r=rootOf(me)
  if not r or holdsBall(me) or attr(me,"CanMove")==false or attr(me,"Action")=="Stealing" or attr(me,"Action")=="Shooting" or attr(me,"Action")=="Dunking" then return nil end
  local occupied={}
  for _,m in ipairs(liveCharacters()) do local b=ballPart(attr(m,"Basketball"));if b then occupied[b]=true end end
  local best,distance
  for ball in pairs(recent) do
   if ball.Parent and not occupied[ball] then
    local d=flat(ball.Position-r.Position).Magnitude
    if d<=settings.recoveryRange and math.abs(ball.Position.Y-r.Position.Y)<6 and (not distance or d<distance) then best=ball;distance=d end
   end
  end
  if not best then return nil end
  local delta=flat(best.Position-r.Position)
  state.recovery="pursuing loose ball"
  guardError=delta.Magnitude
  settings.recoveryDriving=false
  return nil -- Recovery is a marker only; never pull the player toward a loose ball.
 end
 local board=Instance.new("ScreenGui");board.Name="CourtAwareness";board.ResetOnSpawn=false;board.Parent=P.PlayerGui
 local card=Instance.new("Frame");card.Name="CourtStatus";card.Size=UDim2.fromOffset(240,48);card.Position=UDim2.new(1,-256,1,-145);card.BackgroundColor3=Color3.fromRGB(18,22,30);card.BackgroundTransparency=.2;card.Parent=board
 if settings._hudRegister then settings._hudRegister(card,20)end
 local label=Instance.new("TextLabel");label.BackgroundTransparency=1;label.Size=UDim2.new(1,-16,1,0);label.Position=UDim2.fromOffset(8,0);label.TextColor3=Color3.fromRGB(226,230,240);label.TextSize=13;label.Font=Enum.Font.GothamMedium;label.TextWrapped=true;label.Parent=card
 local corner=Instance.new("UICorner");corner.CornerRadius=UDim.new(0,6);corner.Parent=card
 local screenHighlight=Instance.new("Highlight");screenHighlight.FillTransparency=.97;screenHighlight.OutlineTransparency=.15;screenHighlight.OutlineColor=Color3.fromRGB(255,185,65);screenHighlight.FillColor=screenHighlight.OutlineColor;screenHighlight.Parent=rootFolder
 local function cancelCombo()token=token+1;state.combo="idle"end
 api.CancelCombo=cancelCombo
 api.SetComboSlot=function(slot,value)
  if slot<1 or slot>4 or slot%1~=0 or type(value)~="string" or not (value=="None" or (#value<=3 and value:match("^[ZXCV]+$"))) then return false end
  combo[slot]=value;cancelCombo();return true
 end
 api.SetComboKey=function(key)
  if not bindingValid(key) then return false end
  comboKey=key;settings.comboPlayKey=key;return true
 end
 local function parseSequence(text)
  if type(text)~="string" or #text>300 then return nil,"Sequence is too long" end
  local result={}
  for rawPiece in (text..">"):gmatch("(.-)>") do
   local piece=rawPiece:match("^%s*(.-)%s*$"):upper()
   local keys,delay=piece:match("^([A-Z]+)%s*@%s*([%d%.]+)$")
   if not keys then keys=piece;delay=nil end
   if delay then delay=tonumber(delay) else delay=settings.comboInterval end
   if not delay or delay<.08 or delay>1.5 then return nil,"Step delay must be 0.08–1.50 seconds" end
   if not (keys=="WAIT" or (#keys>=1 and #keys<=4 and keys:match("^[ZXCV]+$"))) then return nil,"Use Z/X/C/V combinations or WAIT, separated by >" end
   result[#result+1]={keys=keys,delay=delay}
   if #result>12 then return nil,"Maximum 12 steps" end
  end
  if #result==0 then return nil,"Add a combo step" end
  return result
 end
 api.SetComboSequence=function(text)
  if text=="" then cancelCombo();sequenceText="";return true end
  local steps,err=parseSequence(text)
  if not steps then return false,err end
  cancelCombo();sequenceText=text;return true
 end
 api.GetComboSequence=function()return sequenceText end
 api.PlayCombo=function(preset)
  if not settings.comboBuilder then return false,"Enable combo builder first" end
  local remote=game:GetService("ReplicatedStorage")
  for _,name in ipairs({"Aero","AeroRemoteServices","InputService","Dribble"})do remote=remote and remote:FindFirstChild(name)end
  if not remote or not remote:IsA("RemoteEvent")then return false,"Dribble remote unavailable"end
  local me=model()
  if not me or not holdsBall(me) then return false,"You need possession" end
  if Input:GetFocusedTextBox() then return false,"Finish typing first" end
  local sequence,err=parseSequence(preset and preset.sequence or sequenceText)
  if not sequence then return false,err end
  cancelCombo();local mine=token;local repeats=settings.comboRepeats;local sprint=settings.comboSprint
  if preset then repeats=preset.repeats;sprint=preset.sprint end
  task.spawn(function()
   if attr(me,"Hand")=="L" then
    state.combo="Switching to right hand"
    settings._comboHandSwitch=true
    local ok,failure=pcall(function()
     local vim=game:GetService("VirtualInputManager")
     vim:SendKeyEvent(true,Enum.KeyCode.H,false,game)
     vim:SendKeyEvent(false,Enum.KeyCode.H,false,game)
    end)
    settings._comboHandSwitch=nil
    if not ok then state.combo="Hand switch failed: "..tostring(failure);return end
    task.wait(.22)
   end
   for loop=1,repeats do for index,step in ipairs(sequence) do
    if not alive or mine~=token or not settings.comboBuilder or model()~=me or not holdsBall(me) or Input:GetFocusedTextBox() then if mine==token then state.combo="idle" end;return end
    local action=attr(me,"Action")
    if action=="Shooting" or action=="Dunking" then state.combo="idle";return end
    state.combo=string.format("%d/%d · %s",index,#sequence,step.keys)
    if step.keys~="WAIT" then
     local ok,failure=pcall(function() remote:FireServer(step.keys,sprint==true,Vector3.zero)end)
     if not ok then state.combo=tostring(failure);return end
    end
    task.wait(step.delay)
   end end
   if mine==token then state.combo="idle" end
  end)
  return true,"Combo playing"
 end
 api.GetCourtToolsState=function()return state end
 on(Input.InputBegan,function(input,processed)
  if processed or Input:GetFocusedTextBox() or api.IsUICapturingInput and api.IsUICapturingInput() then return end
  if api.IsNamedComboBinding and api.IsNamedComboBinding(input) then return end
  if bindingMatches(input,comboKey) then api.PlayCombo()
  elseif input.KeyCode==Enum.KeyCode.Z or input.KeyCode==Enum.KeyCode.X or input.KeyCode==Enum.KeyCode.C or input.KeyCode==Enum.KeyCode.V then cancelCombo() end
 end)
 local last=0
 on(Run.Heartbeat,function()
  if not alive or os.clock()-last<.1 then return end;last=os.clock()
  local me=model();local r=rootOf(me)
  board.Enabled=false;local visibleScreen
  for _,part in ipairs(laneParts) do part.Transparency=1 end
  state.lanes={};state.screen=nil;state.opportunity=nil
  if not r then screenHighlight.Enabled=false;return end
  local characters=liveCharacters()
  local opponents={}
  for _,m in ipairs(characters) do if m~=me and not teammate(me,m) then opponents[#opponents+1]=m end end
  if settings.passingLanes and holdsBall(me) then
   local index=0
   for _,mate in ipairs(characters) do
    local mr=rootOf(mate)
    if mate~=me and teammate(me,mate) and (mr.Position-r.Position).Magnitude<=settings.laneRange and index<10 then
     index=index+1
     if not laneParts[index] then
      local part=Instance.new("Part");part.Name="PassingLane";part.Anchored=true;part.CanCollide=false;part.CanTouch=false;part.CanQuery=false;part.Material=Enum.Material.Neon;part.Parent=rootFolder;laneParts[index]=part
     end
     local a,b=r.Position+Vector3.new(0,.5,0),mr.Position+Vector3.new(0,.5,0)
     local delta=b-a;local blocked=false
     for _,enemy in ipairs(opponents) do
      local ep=rootOf(enemy).Position
      local t=math.clamp((ep-a):Dot(delta)/math.max(delta:Dot(delta),.001),0,1)
      if t>.05 and t<.95 and (ep-(a+delta*t)).Magnitude<settings.laneClearance then blocked=true;break end
     end
     local params=RaycastParams.new();params.FilterType=Enum.RaycastFilterType.Exclude;params.FilterDescendantsInstances={me,mate,rootFolder,ballPart(attr(me,"Basketball"))}
     if workspace:Raycast(a,delta,params) then blocked=true end
     local part=laneParts[index];ribbon(part,a,b,.055);part.Transparency=settings._visualV2 and 1 or .3;part.Color=blocked and Color3.fromRGB(255,105,85) or Color3.fromRGB(65,205,160)
     state.lanes[#state.lanes+1]={player=mate.Name,blocked=blocked}
    end
   end
  end
  local message={}
  if settings.screenAwareness then
   local nearest,dist
   for _,enemy in ipairs(opponents) do
    local er=rootOf(enemy);local d=flat(er.Position-r.Position).Magnitude
    local screening=attr(enemy,"Screening")==true or attr(enemy,"Action")=="Screening" or attr(enemy,"Action")=="Screen"
    if screening and d<=settings.screenRange and (not dist or d<dist) then nearest=enemy;dist=d end
   end
   if nearest then
    local localPos=r.CFrame:PointToObjectSpace(rootOf(nearest).Position)
    state.screen={player=nearest.Name,distance=dist,side=localPos.X<0 and "LEFT" or "RIGHT"}
    message[#message+1]="SCREEN "..state.screen.side.." · "..string.format("%.1f studs",dist)
    visibleScreen=nearest
   end
  end
  screenHighlight.Adornee=visibleScreen
  local showScreen=visibleScreen~=nil and (not settings._visualV2 or settings.visualShowScreen)
  if screenHighlight.Enabled~=showScreen then screenHighlight.Enabled=showScreen end
  label.Text=table.concat(message,"\n");board.Enabled=#message>0 and not settings._visualV2
  local occupied={}
  for _,m in ipairs(characters) do local ball=ballPart(attr(m,"Basketball"));if ball then occupied[ball]=true end end
  local folder=workspace:FindFirstChild("Basketballs")
  local valid={}
  if settings.ballTrail and folder then for _,ball in ipairs(folder:GetDescendants()) do
   if ball:IsA("BasePart") and not occupied[ball] and (ball.Position-r.Position).Magnitude<=settings.landingRange then
    valid[ball]=true
    if not trails[ball] then
     local a,b=Instance.new("Attachment"),Instance.new("Attachment");a.Position=Vector3.new(-.12,0,0);b.Position=Vector3.new(.12,0,0);a.Parent=ball;b.Parent=ball
     local trail=Instance.new("Trail");trail.Attachment0=a;trail.Attachment1=b;trail.Lifetime=.35;trail.MinLength=.08;trail.Color=ColorSequence.new(Color3.fromRGB(70,160,255));trail.Transparency=NumberSequence.new(.3,1);trail.Parent=ball;trails[ball]={a,b,trail}
    end
   end
  end end
  for ball,objects in pairs(trails) do if not valid[ball] then for _,obj in ipairs(objects) do obj:Destroy() end;trails[ball]=nil end end
 end)
 local previousCleanup=featureCleanup
 featureCleanup=function()
  alive=false;cancelCombo();settings._recoveryDirection=nil
  if stealSignal and stealSignal.Fire==wrappedFire then stealSignal.Fire=oldFire end
  for _,objects in pairs(trails) do for _,obj in ipairs(objects) do obj:Destroy() end end
  board:Destroy();rootFolder:Destroy();previousCleanup()
 end
end)() end

