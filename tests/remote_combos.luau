local api={};local settings={comboBuilder=true,comboInterval=.25,comboRepeats=1,comboSprint=true}
local sequenceText="";local state={};local token=0;local alive=true
local function cancelCombo()token+=1 end
local me={};local function model()return me end
local function holdsBall()return true end
local hand="R";local function attr(_,key)return key=="Hand"and hand or "Idle"end
local Enum={KeyCode={H="H"}};local taps={}
local Input={GetFocusedTextBox=function()return nil end}
local Vector3={zero={}}
local sent={};local waits={};local stopAfterWait=false
local remote={}
function remote:FindFirstChild()return self end
function remote:IsA(class)return class=="RemoteEvent"end
function remote:FireServer(code,sprint,direction)
 assert(direction==Vector3.zero)
 sent[#sent+1]={code=code,sprint=sprint}
end
local game={GetService=function(_,name)if name=="VirtualInputManager"then return {SendKeyEvent=function(_,down,key)assert(key=="H");taps[#taps+1]=down end}end;return remote end}
local task={spawn=function(fn)fn()end,wait=function(seconds)waits[#waits+1]=seconds;if stopAfterWait then cancelCombo()end end}
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

assert(api.SetComboSequence("XZ@0.08 > XX@0.10 > WAIT@0.08 > XC@0.15"))
assert(api.PlayCombo())
assert(#sent==3 and sent[1].code=="XZ"and sent[2].code=="XX"and sent[3].code=="XC","One remote call per move; XX is not two X calls")
assert(sent[1].sprint and #waits==4,"Sprint boolean and waits preserved")
sent={};waits={};settings.comboSprint=false
assert(api.PlayCombo({sequence="CC@0.08",repeats=2,sprint=false}))
assert(#sent==2 and not sent[1].sprint,"Saved repeats and modifier preserved")
sent={};stopAfterWait=true
assert(api.PlayCombo())
assert(#sent==1,"Cancel stops subsequent requests")
assert(not api.SetComboSequence("BAD@0.08"),"Reject unsupported move symbols")
print("Remote combo playback checks passed")

stopAfterWait=false;sent={};waits={};hand="L"
assert(api.PlayCombo())
assert(#taps==2 and taps[1]==true and taps[2]==false,"Tap H once")
assert(waits[1]==.22 and #sent==3,"Wait before first move")
sent={};waits={};stopAfterWait=true
assert(api.PlayCombo())
assert(#sent==0,"Cancellation during hand switch prevents playback")
print("Hand switch and cancellation checks passed")
