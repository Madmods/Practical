do (function()
-- Connect the supplied interface to the existing feature engine.
local booleanMap={visualStyle={enabled="visualPolish",labels="visualLabels",motion="visualMotion",urgency="visualUrgency",beacon="visualBeacon",flightShadow="visualFlightShadow",comet="visualComet"},antiOop={enabled="antiOop",move="oopMove",guide="oopGuide"},passVision={enabled="passVision"},dunkStopper={enabled="dunkStopper"},autoJoin={enabled="autoJoin"},passIntercept={enabled="passIntercept",automatic="passInterceptAuto",move="passInterceptMove",jump="passInterceptJump",shots="passInterceptShots"},contestCeiling={enabled="contestCeiling",release="contestRelease"},recovery={enabled="ballRecovery"},screens={enabled="screenNavigation",visual="screenAwareness"},assignmentTools={lock="assignmentLock"},passingLanes={enabled="passingLanes"},comboBuilder={enabled="comboBuilder",sprint="comboSprint"},autoGuard={antiPush="antiPush",rimBlock="rimBlock",adaptive="adaptiveGuard",cutoff="driveCutoff",screens="screenNavigation",coverage="guardCoverage",enabled="guard",faceHandler="faceTarget",lockAssignment="assignmentLock",showIndicators="visuals",fakeFilter="fakeFilter"},
 autoSteal={enabled="steal"},autoBlock={commit="blockCommitFilter",enabled="block",oneAttempt="blockOnce"},
 rebound={enabled="rebound",chase="reboundChase"},boost={enabled="boost"},
 landingTools={trail="ballTrail",enabled="landing",beam="landingBeam",zone="landingZone"},overlayTools={contest="liveContest",guard="visuals"},
 autoRelease={liveContest="liveContest",contest="contest"},shotAssist={enabled="shotAssist",holdEarly="shotHoldEarly",protectFall="shotProtectFall"}}
local numberMap={visualStyle={scale="visualScale",intensity="visualIntensity",weight="visualWeight"},antiOop={range="oopRange"},passVision={speed="passSpeed",release="catchRelease"},dunkStopper={range="dunkRange",timing="dunkTimingMs"},passIntercept={reach="passInterceptReach"},contestCeiling={limit="contestLimit"},recovery={range="recoveryRange",window="recoveryWindow"},screens={range="screenRange"},passingLanes={range="laneRange",clearance="laneClearance"},comboBuilder={interval="comboInterval",repeats="comboRepeats"},autoGuard={standOff="guardGap",delay="guardReaction"},autoSteal={reach="stealRange",cooldown="stealCooldown",delay="stealDelay"},
 autoRelease={offset="timingOffsetMs",maxPing="timingMaxPingMs",freshness="timingFreshMs"},shotAssist={meterMin="shotMeterMin",meterMax="shotMeterMax"},
 rebound={range="reboundChaseRange",reach="reboundRange",height="reboundHeight",lead="reboundLead"},
 landingTools={width="landingBeamWidth",scale="landingZoneScale",range="landingRange"},
 autoBlock={reactionDelay="blockDelay"},boost={multiplier="boostMultiplier",tail="boostDuration"},}
booleanMap.visualLayers={landing="visualShowLanding",flight="visualShowFlight",route="visualShowRoute",coverage="visualShowCoverage",pass="visualShowPass",recovery="visualShowRecovery",screen="visualShowScreen",intercept="visualShowIntercept"}
booleanMap.visualLabels={passPanel="visualPassPanel"}
booleanMap.defenderAlerts={enabled="defenderAlerts",ballOnly="alertBallOnly"}
numberMap.defenderAlerts={range="alertRange"}
booleanMap.rebound.early="reboundEarly";numberMap.rebound.spot="reboundSpot"
booleanMap.autoGuard.idleMovement="guardIdleMovement";booleanMap.autoGuard.humanize="guardHuman";booleanMap.autoGuard.rimPress="rimPress"
numberMap.autoGuard.humanLevel="guardHumanLevel";numberMap.autoGuard.rimRange="rimPressRange";numberMap.autoGuard.rimGap="rimPressGap"
booleanMap.autofarm={postClamp="farmPostClamp",chatRequeue="farmChatRequeue",crossover="farmCrossover",shoot="farmShoot",randomThrees="farmRandomThrees",visuals="farmVisuals",rimStop="farmRimStop"};numberMap.autofarm={stepbackAggression="farmStepbackAggression",dunkRange="farmDunkRange",shotRange="farmShotRange"}
booleanMap.visualColor={enabled="visualCustomColor"}
booleanMap.denyReceiver={enabled="denyReceiver",catch="denyCatch"}
booleanMap.offBallResponse={enabled="denySoft"}
numberMap.offBallResponse={delay="denyDelay",smoothing="denySmoothing",deadzone="denyDeadzone",lead="denyLead"}
booleanMap.smartDribble={enabled="smartDribble",follow="dribbleTestFollow",shoot="smartDribbleShoot",steer="smartDribbleSteer"}
booleanMap.smartPass={enabled="smartPass",spin="smartPassSpin"}
booleanMap.catchShoot={enabled="catchShoot"}
numberMap.catchShoot={contest="catchShootContest"}
booleanMap.ballProtection={enabled="ballProtection"}
numberMap.smartDribble={gap="smartDribbleGap",stamina="smartDribbleStamina",cooldown="smartDribbleCooldown",testSpeed="dribbleTestSpeed",shotContest="smartDribbleShotContest",shotRange="smartDribbleShotRange",arc="smartDribbleThreeDistance"}
numberMap.ballProtection={range="ballProtectionRange"}
numberMap.visualStyle.smoothing="visualSmoothing";numberMap.visualStyle.clearance="visualClearance"
numberMap.visualLabels={size="visualLabelSize",opacity="visualLabelOpacity"}
numberMap.visualColor={red="visualRed",green="visualGreen",blue="visualBlue"}
numberMap.denyReceiver={range="denyRange",gap="denyGap",switch="denySwitchMargin",delay="denyDelay",smoothing="denySmoothing",deadzone="denyDeadzone"}
local firstCourtCorner
local function applyResponse(name)
 local values=name=="Legit" and {guardReaction=.24,guardPrediction=.05,guardStrength=.4,guardDeadzone=.9,sprintDistance=4,closeReaction=.15,closeGap=2,stealAngle=35,stealMaxSpeed=8,stealDelay=.2,stealCooldown=2,blockDelay=.12,jumpCooldown=1.6,reboundLead=.1}
  or name=="Blatant" and {guardReaction=0,guardPrediction=.12,guardStrength=1,guardDeadzone=.35,sprintDistance=1,closeReaction=0,closeGap=1,stealAngle=65,stealMaxSpeed=20,stealDelay=.05,stealCooldown=.8,blockDelay=.05,jumpCooldown=.8,reboundLead=.2}
  or {guardReaction=.16,guardPrediction=.08,guardStrength=1,guardDeadzone=.65,sprintDistance=2.5,closeReaction=.08,closeGap=1.5,stealAngle=45,stealMaxSpeed=10,stealDelay=.12,stealCooldown=1.6,blockDelay=.08,jumpCooldown=1.2,reboundLead=.15}
 for key,value in pairs(values) do api.SetNumber(key,value) end
end
local screenVisualChoice=true
local function routeControl(id,field,value)
 if id=="screens"and field=="enabled"then
  api.SetOption("screenNavigation",value)
  return api.SetOption("screenAwareness",value and screenVisualChoice)
 end
 if id=="screens"and field=="visual"then
  screenVisualChoice=value
  return api.SetOption("screenAwareness",value and api.GetOption("screenNavigation"))
 end
 if id=="smartDribble"and field=="activation"then return api.SetSmartDribbleKey(value)end
 if id=="smartDribble"and field=="plan"then
  return api.SetNumber("smartDribblePlan",value=="Attack basket"and 2 or value=="Find a three"and 3 or 1)
 end
 if id=="visualStyle"and field=="route"then
  return api.SetNumber("visualRouteStyle",({Dashed=1,Solid=2,Chevrons=3})[value]or 1)
 end
 if id=="visualStyle"and field=="palette"then
  return api.SetNumber("visualPalette",({Cobalt=1,Glacier=2,Mint=3,Amber=4})[value]or 1)
 end
 if id=="visualStyle"and field=="preset"then
  local preset=value=="Minimal"and {intensity=35,weight=.95,scale=.9,smoothing=70,clearance=.12,motion=false}
   or value=="Vivid"and {intensity=85,weight=1.4,scale=1.1,smoothing=45,clearance=.16,motion=true}
   or {intensity=65,weight=1.15,scale=1,smoothing=55,clearance=.12,motion=false}
  for key,settingValue in pairs(preset)do
   if key=="motion"then api.SetOption("visualMotion",settingValue)else api.SetNumber(numberMap.visualStyle[key],settingValue)end
  end
  UI.forEachControl(function(ctrl)if ctrl.id==id then UI.forEachValue(ctrl,function(setting)if preset[setting.id]~=nil then setting.value=preset[setting.id]end end)end end)
  repaintAll();return true
 end
 if id=="visualLabels" and field=="corner" then return api.SetHudCorner(value)end
 if id=="dunkStopper" and field=="activation" then return api.SetDunkKey(value)end
 if id=="comboLibrary" then return true end
 if id=="comboBuilder" and field=="sequenceDelay" then return true end
 if id=="comboBuilder" and field=="sequence" then return api.SetComboSequence(value) end
 if id=="comboBuilder" and field=="preset" then
  local presets={Crossovers="ZC@0.18 > CZ@0.18 > ZX@0.25",Stepback="Z@0.18 > X@0.28 > WAIT@0.15 > C@0.20",Mixed="ZX@0.20 > CV@0.25 > WAIT@0.10 > ZXC@0.30"}
  if presets[value] then
   UI.forEachControl(function(ctrl)if ctrl.id==id then UI.forEachValue(ctrl,function(setting)if setting.id=="sequence" then setting.value=presets[value] end end)end end)
   repaintAll();return api.SetComboSequence(presets[value])
  end
  return true
 end
 if id=="comboBuilder" and field:match("^step[1-4]$") then return api.SetComboSlot(tonumber(field:sub(-1)),value) end
 if id=="comboBuilder" and field=="activation" then return api.SetComboKey(value) end
 if field=="engaged" then return true end -- actual bound key state is read by the engine
 -- The farm toggle is user-driven only (see the controlChanged handler); startup/profile syncs must not flip it.
 if id=="autofarm" and field=="enabled" then return true end
 if id=="autoRelease" and field=="enabled" then return api.SetAutoTime(value) end
 local key=booleanMap[id] and booleanMap[id][field]
 if key then
  local ok=api.SetOption(key,value)
  return ok
 end
 key=numberMap[id] and numberMap[id][field]
 if key then
  local ok=api.SetNumber(key,value)
  if id=="shotAssist" then UI.forEachControl(function(ctrl)
   if ctrl.id==id then UI.forEachValue(ctrl,function(setting)
    local mapped=numberMap[id][setting.id];if mapped then setting.value=api.GetOption(mapped) end
   end) end
  end);repaintAll() end
  return ok
 end
 if id=="autoGuard" and field=="activation" then return api.SetGuardKey(value) end
 if id=="autoGuard" and field=="response" then
  applyResponse(value)
  UI.forEachControl(function(ctrl) UI.forEachValue(ctrl,function(setting)
   local mapped=numberMap[ctrl.id] and numberMap[ctrl.id][setting.id]
   if mapped then setting.value=api.GetOption(mapped) end
  end) end)
  repaintAll();return true
 end
 if id=="interface" or id=="appearance" or id=="layout" or id=="motion" or id=="typography" or id=="notifications" or id=="comboLibrary" then return true end
 return false,"Unsupported control: "..tostring(id).." / "..tostring(field)
end
local function applyAll()
 -- Presets first; explicit sliders then override their corresponding values.
 UI.forEachControl(function(ctrl) UI.forEachValue(ctrl,function(setting)
  if ctrl.id=="autoGuard" and setting.id=="response" then applyResponse(setting.value) end
 end) end)
 UI.forEachControl(function(ctrl)
  if ctrl.kind~="section" then routeControl(ctrl.id,"enabled",ctrl.enabled==true) end
  UI.forEachValue(ctrl,function(setting)
   if setting.id~="response" and setting.type~="button"and not(ctrl.id=="visualStyle"and setting.id=="preset")then routeControl(ctrl.id,setting.id,setting.value)end
  end)
 end)
end

local appearanceApply=applySettings
applySettings=function(t)appearanceApply(t);applyAll()end
UI.connect(controlChanged.Event,function(id,field,value)
 if id=="catchShoot"and field=="status"then
  local state=api.GetCatchShootState()
  local message=string.format("%s · Early E requests: %d · Last lead: %s · %s",state.status,state.earlyRequests or 0,state.lastLeadMs and string.format("%.0f ms",state.lastLeadMs)or "not recorded",state.bufferReason or "unknown")
  setStatus(message);toast("Early-shot buffer",message,"info",true);return
 end
 if id=="smartPass"and field=="status"then
  local passState=api.GetSmartPassState();local names={}
  for key,name in pairs(passState.slots or {})do names[#names+1]=key..": "..name end
  table.sort(names)
  setStatus(passState.status.." � receivers: "..(#names>0 and table.concat(names,", ")or "not identified yet"));return
 end
 if id=="smartDribble"and field=="pickBasket"then local _,message=api.PickDribbleBasket();setStatus(message);return end
 if id=="smartDribble"and field=="markThree"then
  local ok,message=api.MarkDribbleThreePointLine()
  if ok then
   UI.forEachControl(function(ctrl)if ctrl.id==id then UI.forEachValue(ctrl,function(setting)if setting.id=="arc"then setting.value=api.GetOption("smartDribbleThreeDistance")end end)end end)
   repaintAll()
  end
  setStatus(message);return
 end
 if id=="smartDribble"and(field=="spawnTest"or field=="stopTest")then
  local _,message
  if field=="spawnTest"then _,message=api.StartDribblePractice()else _,message=api.StopDribblePractice()end
  setStatus(message);return
 end
 if id=="smartDribble"and field=="status"then
  local state=api.GetSmartDribbleState()
  local reason=state.held and state.status or state.lastHeldStatus and("Last hold: "..state.lastHeldStatus)or state.status
  local message=string.format("%s · Key %s (%s) · Action %s · Stamina %s · CanDribble %s · %d opponents · Basket: %s · Arc: %.1f",reason,state.key or "?",state.held and "held"or "released",tostring(state.action or "unknown"),tostring(state.stamina or "unknown"),tostring(state.canDribble),state.defenders or 0,state.basketSource or "not checked",state.threeDistance or 0)
  setStatus(message);toast("Smart dribbler status",message,"info",true);return
 end
 if id=="profile" then
  applyAll()
  UI.persistProfiles();return
 end
 if id=="denyReceiver"and(field=="select"or field=="clear"or field=="next")then
  local _,message
  if field=="select"then
   _,message=api.BeginReceiverSelection();toast("Choose a matchup",message,"crosshair",true)
  elseif field=="next"then _,message=api.CycleReceiverMatchup()
  else
   _,message=api.ClearReceiverMatchup()
   UI.forEachControl(function(ctrl)if ctrl.id=="assignmentTools"then UI.forEachValue(ctrl,function(setting)if setting.id=="lock"then setting.value=false end end)end end)
   repaintAll()
  end
  setStatus(message);return
 end
 if id=="denyReceiver"and field=="status"then
  local state=api.GetReceiverDenialState()
  setStatus(state.status..(state.target and " · "..state.target or "")..(state.handler and " · passer "..state.handler or ""));return
 end
 if id=="antiOop" and field=="status" then
  local state=api.GetAntiOopState();setStatus(state.status..(state.gap and string.format(" · gap %.1f / ETA %.2fs",state.gap,state.time)or "").." · last jump: "..(state.lastJump or "none"));return
 end
 if id=="passIntercept" and field=="status" then setStatus(api.GetDefenseNavigationState().intercept);return end
 if id=="appearance" or id=="motion" or id=="layout" or id=="typography" or id=="notifications" or id=="interface" then
  applySettings();refreshNav();refreshMini()
  if field=="preview" then ACTIONS.preview() end
  if field=="resetLayout" then ACTIONS.resetLayout() end
  return
 end
 if id=="autofarm" and field=="enabled" then local _,message=api.SetFarm(value==true);setStatus(message);toast("Blacktop autofarm",message,"info",true);return end
 if id=="autofarm" and(field=="gymTest"or field=="stopGymTest")then local _,message=(field=="gymTest"and api.StartFarmGymTest or api.StopFarmGymTest)();setStatus(message);toast("Gym test",message,"info",true);return end
 if id=="autofarm" and field=="readyNow" then local _,message=api.FarmReadyNow();setStatus(message);return end
 if id=="autofarm" and field=="status" then
  local s=api.GetFarmState()
  local message=string.format("%s · %s · %s · readies %d · dunks %d · loose balls %d · crossovers %d · %s",s.running and "ON" or "OFF",s.status,s.seen or "-",s.ready,s.dunks,s.loose,s.crosses,s.queue)
  setStatus(message);toast("Blacktop autofarm",message,"info",true);return
 end
 if id=="autoJoin" and field=="lock" then local _,message=api.LockJoinSpot();setStatus(message);return end
 if id=="autoJoin" and field=="status" then local state=api.GetAutoJoinState();setStatus(state.status.." · "..state.attempts.." requests");return end
 if id=="comboBuilder" and field=="play" then
  UI.forEachControl(function(ctrl)if ctrl.id==id then ctrl.enabled=true end end)
  api.SetOption("comboBuilder",true);repaintAll()
  local _,message=api.PlayCombo();setStatus(message);return end
 if id=="comboBuilder" and field=="cancel" then api.CancelCombo();setStatus("Combo cancelled");return end
 if id=="assignmentTools" and field=="next" then setStatus(api.SwitchAssignment() and "Matchup selected" or "No opponent in range");return end
 if id=="tools" then
  if field=="report" then local _,message=api.ExportReport();setStatus(message)
  elseif field=="check" then local _,message=api.Check();setStatus(message)
  elseif field=="assignment" then setStatus(api.SwitchAssignment() and "Opponent selected" or "No opponent in range")
  elseif field=="solo" then api.StartSolo();setStatus("Local rehearsal only; no server opponent")
  elseif field=="stopSolo" then api.StopSolo();setStatus("Rehearsal stopped")
  elseif field=="unload" then api.Unload() end
  return
 end
 if id=="rebound" and field=="setCourtBounds" then
  local folder=workspace:FindFirstChild("Characters");local own=folder and folder:FindFirstChild(player.Name) or player.Character
  local r=own and own:FindFirstChild("HumanoidRootPart")
  if not r then setStatus("Character unavailable");return end
  if not firstCourtCorner then firstCourtCorner=r.Position;setStatus("First corner marked. Walk to opposite corner and click again.")
  elseif api.SetCourtBounds(firstCourtCorner,r.Position) then firstCourtCorner=nil;setStatus("Court bounds set")
  else setStatus("Corners must span at least 6 studs on both axes") end
  return
 elseif id=="rebound" and field=="clearBounds" then firstCourtCorner=nil;api.ClearCourtBounds();setStatus("Court bounds cleared");return end
 local ok,message=routeControl(id,field,value)
 if ok and id=="denyReceiver"and field=="enabled"and value and api.GetReceiverDenialState and api.BeginReceiverSelection then
  if not api.GetReceiverDenialState().selected then
   local _,notice=api.BeginReceiverSelection();toast("Choose a matchup",notice,"crosshair",true);setStatus(notice)
  end
 end
 if not ok and message then setStatus(message) end
end)
local lastMatchup,matchupNoticeAt=nil,0
UI.connect(RunService.Heartbeat,function()
 if os.clock()-matchupNoticeAt<.25 then return end
 matchupNoticeAt=os.clock()
 -- The engine owns the autofarm state (saved config, the 5 s prompt); keep the toggle matching it.
 local farmOn=api.GetOption("farm")==true
 UI.forEachControl(function(ctrl)if ctrl.id=="autofarm" and ctrl.enabled~=farmOn then ctrl.enabled=farmOn;repaintAll()end end)
 if not api.GetReceiverDenialState then return end
 local state=api.GetReceiverDenialState()
 if state.selected~=lastMatchup then
  if state.selected then
   toast("Matchup selected",state.selected.." · on and off ball","shield",true)
   UI.forEachControl(function(ctrl)if ctrl.id=="assignmentTools"then UI.forEachValue(ctrl,function(setting)if setting.id=="lock"then setting.value=false end end)end end)
   repaintAll()
  elseif lastMatchup then toast("Following ball handler","Your selected matchup was cleared or is no longer available","shield",true)end
  lastMatchup=state.selected
 end
end)
UI.connect(testControls.Event,function() local _,message=api.Check();setStatus(message) end)

-- Notification and non-interactive cursor visuals; no full-screen modal.
do
 local shownRevision,lastPoll=0,0
 local cursor,hoverOutline,nameLabel
 local function make(class,props)
  local object=Instance.new(class)
  for key,value in pairs(props)do object[key]=value end
  return object
 end
 local function hide()
  if cursor then cursor.Visible=false end
  if hoverOutline then hoverOutline.Enabled=false;hoverOutline.Adornee=nil end
 end
 local function ensure()
  if cursor then return end
  cursor=make("Frame",{Name="MatchupSelectionCursor",Size=UDim2.fromOffset(40,40),BackgroundTransparency=1,BorderSizePixel=0,Visible=false,Active=false,ZIndex=100,Parent=gui})
  local ring=make("Frame",{Size=UDim2.fromOffset(28,28),Position=UDim2.fromOffset(6,6),BackgroundTransparency=1,ZIndex=101,Parent=cursor})
  make("UICorner",{CornerRadius=UDim.new(1,0),Parent=ring})
  make("UIStroke",{Color=Color3.fromRGB(80,167,255),Thickness=1.5,Transparency=.12,Parent=ring})
  for _,line in ipairs({{19,0,2,7},{19,33,2,7},{0,19,7,2},{33,19,7,2}})do
   make("Frame",{Position=UDim2.fromOffset(line[1],line[2]),Size=UDim2.fromOffset(line[3],line[4]),BackgroundColor3=Color3.fromRGB(109,191,255),BorderSizePixel=0,ZIndex=101,Parent=cursor})
  end
  local pill=make("Frame",{Position=UDim2.fromOffset(45,8),Size=UDim2.fromOffset(220,44),BackgroundColor3=Color3.fromRGB(17,20,29),BackgroundTransparency=.05,BorderSizePixel=0,ZIndex=101,Parent=cursor})
  make("UICorner",{CornerRadius=UDim.new(0,12),Parent=pill})
  make("UIStroke",{Color=Color3.fromRGB(40,47,64),Thickness=1,Parent=pill})
  nameLabel=make("TextLabel",{Size=UDim2.new(1,-20,1,0),Position=UDim2.fromOffset(10,0),BackgroundTransparency=1,Text="",TextColor3=Color3.fromRGB(227,240,255),Font=Enum.Font.GothamMedium,TextSize=12,TextWrapped=true,ZIndex=102,Parent=pill})
  hoverOutline=make("Highlight",{Name="MatchupSelectionHover",Enabled=false,FillColor=Color3.fromRGB(49,135,255),FillTransparency=.87,OutlineColor=Color3.fromRGB(106,202,255),OutlineTransparency=.05,DepthMode=Enum.HighlightDepthMode.Occluded,Parent=workspace.CurrentCamera or workspace})
 end
 UI.connect(RunService.Heartbeat,function()
  if not api.GetMatchupPickerState or os.clock()-lastPoll<.25 then return end
  lastPoll=os.clock()
  local state=api.GetMatchupPickerState()
  if not state.active or state.revision<=shownRevision then return end
  shownRevision=state.revision
  api.BeginReceiverSelection()
  toast("Choose your matchup","Hover an opponent, then click to select. Right-click or Escape cancels.","crosshair",true)
 end)
 UI.connect(RunService.RenderStepped,function()
  if not api.GetReceiverDenialState or not api.GetReceiverDenialState().picking then hide();return end
  ensure()
  local position=UserInputService:GetMouseLocation()
  if not gui.IgnoreGuiInset then position=position-game:GetService("GuiService"):GetGuiInset()end
  cursor.Position=UDim2.fromOffset(position.X-20,position.Y-20);cursor.Visible=true
  local character,adornee
  if api.GetReceiverSelectionHover then character,adornee=api.GetReceiverSelectionHover()end
  hoverOutline.Adornee=adornee;hoverOutline.Enabled=adornee~=nil
  nameLabel.Text=character and("CLICK TO GUARD\n"..character.Name)or "SELECT MATCHUP\nHover an opposing player"
  hoverOutline.FillTransparency=.87+math.sin(os.clock()*3)*.025
 end)
 api.RegisterUICleanup(function()
  if hoverOutline then hoverOutline:Destroy()end
  if cursor then cursor:Destroy()end
  if api.CancelReceiverSelection then api.CancelReceiverSelection()end
 end)
end

api.IsUICapturingInput=function()return rebinding~=nil end
api.IsUIReservedKey=function(key)return key==PREF_KEY.value end
api.RegisterUICleanup(function()if alive then gui:Destroy()end end)
api.SetPanelVisible(false)
UI.restore(currentProfile.snap)
end)() end

