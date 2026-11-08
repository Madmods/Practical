-- Named dribble sequences and independent hotkeys, persisted separately from UI profiles.
if api then (function()
 local entries={}
 local path="PracticalSavedCombos-v1.json"
 local stopped=false
 local function validSequence(value)
  if type(value)~="string"or #value==0 or #value>300 then return false end
  local count=0
  for piece in (value..">"):gmatch("(.-)>")do
   local keys,delay=piece:match("^%s*([ZXCVWAIT]+)@([%d%.]+)%s*$")
   delay=tonumber(delay)
   if not keys or not delay or delay<.08 or delay>1.5 or not(keys=="WAIT"or #keys<=4 and keys:match("^[ZXCV]+$"))then return false end
   count=count+1
  end
  return count>0 and count<=12
 end
 local function usable(key)
  if key==(settings.smartDribbleKey or Enum.KeyCode.H)and(settings.smartDribble or settings.ballProtection)then return false end
  if not bindingValid(key) then return false end
  for _,reserved in ipairs({Enum.KeyCode.W,Enum.KeyCode.A,Enum.KeyCode.S,Enum.KeyCode.D,Enum.KeyCode.Z,Enum.KeyCode.X,Enum.KeyCode.C,Enum.KeyCode.V,Enum.KeyCode.Space,Enum.KeyCode.RightShift,Enum.KeyCode.LeftShift,Enum.KeyCode.R})do if key==reserved then return false end end
  return key~=guardKey and key~=(settings.comboPlayKey or Enum.KeyCode.J)and not(api.IsUIReservedKey and api.IsUIReservedKey(key))
 end
 local function persist()
  if type(writefile)~="function"then return false,"Saved for this session only; file saving unavailable"end
  local serialized={}
  for name,e in pairs(entries)do serialized[name]={sequence=e.sequence,repeats=e.repeats,sprint=e.sprint,key=e.key.Name}end
  local ok=pcall(function()writefile(path,Http:JSONEncode({version=1,combos=serialized}))end)
  return ok,ok and "Combo saved"or "Saved in session; could not write combo file"
 end
 api.GetNamedCombos=function()
  local list={}for name,e in pairs(entries)do local item=table.clone(e);item.name=name;list[#list+1]=item end
  table.sort(list,function(a,b)return a.name:lower()<b.name:lower()end);return list
 end
 api.SaveNamedCombo=function(name,key)
  if type(name)~="string"then return false,"Enter a combo name"end
  name=name:match("^%s*(.-)%s*$")
  if #name<1 or #name>48 then return false,"Use a name between 1 and 48 characters"end
  if not usable(key)then return false,"Choose a key other than movement, dribble, guard, R or menu keys"end
  for other,e in pairs(entries)do if other~=name and e.key==key then return false,"That key belongs to "..other end end
  local count=0;for _ in pairs(entries)do count=count+1 end
  if not entries[name]and count>=40 then return false,"Maximum 40 saved combos"end
  local value=api.GetComboSequence()
  if not validSequence(value)then return false,"Add valid moves before saving"end
  entries[name]={sequence=value,repeats=settings.comboRepeats,sprint=settings.comboSprint,key=key}
  local _,message=persist();return true,message.." · "..name.." ["..key.Name.."]"
 end
 api.DeleteNamedCombo=function(name)
  if not entries[name]then return false,"Choose a saved combo first"end
  entries[name]=nil;persist();return true,"Saved combo removed"
 end
 api.LoadNamedCombo=function(name)
  local e=entries[name];if not e then return false,"Choose a saved combo first"end
  local ok,message=api.SetComboSequence(e.sequence);if not ok then return false,message end
  settings.comboRepeats=e.repeats;settings.comboSprint=e.sprint
  return true,table.clone(e)
 end
 if type(readfile)=="function"then
  local ok,data=pcall(function()return Http:JSONDecode(readfile(path))end)
  if ok and type(data)=="table"and data.version==1 and type(data.combos)=="table"then
   local count=0
   for name,e in pairs(data.combos)do
    if count>=40 then break end
    if type(name)=="string"and #name>0 and #name<=48 and type(e)=="table"and validSequence(e.sequence)and type(e.key)=="string"then
     local keyOk,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()local ok,key=pcall(function()return Enum.KeyCode[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end);if ok and key then return key end;return Enum.UserInputType[e.key]end)
     if keyOk and usable(key)then
      local repeats=type(e.repeats)=="number"and e.repeats==e.repeats and math.clamp(math.floor(e.repeats),1,4)or 1
      entries[name]={sequence=e.sequence,repeats=repeats,sprint=e.sprint==true,key=key};count=count+1
     end
    end
   end
  end
 end
 local connection=Input.InputBegan:Connect(function(input,processed)
  if stopped or processed or Input:GetFocusedTextBox()or not settings.comboBuilder or api.IsUICapturingInput and api.IsUICapturingInput()then return end
  for name,e in pairs(entries)do
   if bindingMatches(input,e.key) and usable(e.key)then
    local ok,message=api.PlayCombo(e)
    if not ok then assistState=message else assistState="Playing "..name end
    return
   end
  end
 end)
 local previous=featureCleanup
 featureCleanup=function()stopped=true;connection:Disconnect();previous()end
end)()end



