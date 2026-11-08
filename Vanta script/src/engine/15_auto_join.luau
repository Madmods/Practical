-- Selected native join spot; no leave requests or movement overrides.
if api then (function()
 settings.autoJoin=false
 local selected,lastAttempt,stopped=nil,-math.huge,false
 local state={status="Choose a spot",attempts=0}
 local function spots()
  local ok,value=pcall(function()return require(P.PlayerScripts.Aero.Modules.UI.Park.Spots)end)
  if ok and type(value)=="table" then return value end
 end
 local function nearest(module)
  local ok,value=pcall(function()return module:GetNearestSpotPad()end)
  if ok and type(value)=="table" and value.Court and value.SpotType and value.SpotIndex then return value end
 end
 local function same(a,b)
  return a and b and a.Court==b.Court and a.SpotType==b.SpotType and a.SpotIndex==b.SpotIndex
 end
 api.LockJoinSpot=function()
  local module=spots();local pad=module and nearest(module)
  if not pad then return false,"Stand next to the desired join spot first" end
  selected={Court=pad.Court,SpotType=pad.SpotType,SpotIndex=pad.SpotIndex}
  state.status="Locked "..tostring(pad.Court).." / "..tostring(pad.SpotType).." / "..tostring(pad.SpotIndex)
  return true,state.status
 end
 api.GetAutoJoinState=function()return {status=state.status,attempts=state.attempts,selected=selected and table.clone(selected)}end
 local connection=Run.Heartbeat:Connect(function()
  if stopped or not settings.autoJoin then return end
  local own=model()
  if not own then state.status="Waiting for character";return end
  if attr(own,"OnSpot") or attr(own,"OnCourt") or attr(own,"InGame") then state.status="Joined / in game";return end
  if not selected then state.status="Lock a spot first";return end
  if Input:GetFocusedTextBox()then state.status="Paused while typing";return end
  local now=os.clock();if now-lastAttempt<.25 then return end
  lastAttempt=now
  local module=spots()
  if not module or not same(selected,nearest(module))then state.status="Paused: return to selected spot";return end
  local ok=pcall(function()module.Services.SpotService.JoinSpot:Fire(table.clone(selected))end)
  if ok then state.attempts=state.attempts+1;state.status="Join requested" else state.status="Join service unavailable" end
 end)
 local previous=featureCleanup
 featureCleanup=function()stopped=true;settings.autoJoin=false;connection:Disconnect();previous()end
end)() end

-- Auto guard no longer owns rim blocks or jumps.
if api then
 settings.dunkStopper=false
 api.SetDunkKey=function()return false,"Auto guard rim blocking was removed"end
end

