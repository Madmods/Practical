-- Smart pass: adapt an existing number-key pass; preserve its actual receiver.
do (function()
 settings.smartPass=false
 local styles={"Overhead","Chest","Bounce"}
 local state={status="Waiting for native pass controller",learned={},passes=0}
 local templates,patches={},{}
 local armed,hook,stopped,recentKey,recentAt
 local path="PracticalSmartPass-v1.json"
 local function copy(value,seen)
  if type(value)~="table"or typeof(value)~="table"then return value end
  seen=seen or {};if seen[value]then return seen[value]end
  local out={};seen[value]=out
  for k,v in pairs(value)do out[k]=copy(v,seen)end
  return out
 end
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
 local function get(value,path)
  for _,k in ipairs(path)do if type(value)~="table"then return nil end;value=value[k]end
  return value
 end
 local function put(value,path,replacement)
  for i=1,#path-1 do if type(value)~="table"then return false end;value=value[path[i]]end
  if type(value)~="table"then return false end
  value[path[#path]]=replacement;return true
 end
 local literal={overhead=true,normal=true,chest=true,bounce=true}
 local field={type=true,passtype=true,style=true,passstyle=true,bounce=true,chest=true,overhead=true,modifier=true}
 local function infer(base,sample)
  if base.receiver~=sample.receiver then return nil,"Teach all pass types to the same teammate"end
  local result={};local seen={}
  local function walk(a,b,p,depth)
   if a==b then return end
   if depth>5 or type(a)~=type(b)then return end
   if type(a)=="table"then
    if seen[a]then return end;seen[a]=true
    for k,v in pairs(a)do local nextPath=table.clone(p);nextPath[#nextPath+1]=k;walk(v,b[k],nextPath,depth+1)end
   elseif a~=b and(type(a)=="string"or type(a)=="number"or type(a)=="boolean")then
    local name=tostring(p[#p]):lower():gsub("_","")
    local semantic=type(a)=="string"and literal[a:lower()]and literal[b:lower()]
    if field[name]or semantic then result[#result+1]={path=p,before=a,after=b}end
   end
  end
  walk(base.args,sample.args,{},0)
  if #result==0 or #result>4 then return nil,"Pass-type encoding unresolved; original passes preserved"end
  return result
 end
 local function persist()
  if type(writefile)~="function"then return end
  pcall(function()writefile(path,Http:JSONEncode({version=1,place=game.PlaceId,placeVersion=game.PlaceVersion,patches=patches}))end)
 end
 local function rebuild()
  if not templates.Overhead then return end
  patches.Overhead={};state.learned.Overhead=true
  for _,name in ipairs({"Chest","Bounce"})do
   if templates[name]then
    local patch,problem=infer(templates.Overhead,templates[name])
    patches[name]=patch;state.learned[name]=patch~=nil
    if problem then state.status=problem end
   end
  end
  persist()
 end
 if type(readfile)=="function"then
  pcall(function()
   local data=Http:JSONDecode(readfile(path))
   if data.version~=1 or data.place~=game.PlaceId or data.placeVersion~=game.PlaceVersion then return end
   for _,name in ipairs(styles)do
    local patch=data.patches and data.patches[name]
    if type(patch)=="table"and #patch<=4 then
     local valid=true
     for _,edit in ipairs(patch)do
      if type(edit.path)~="table"or #edit.path<1 or #edit.path>5 or type(edit.before)~=type(edit.after)
       or not(type(edit.before)=="string"or type(edit.before)=="number"or type(edit.before)=="boolean")then valid=false;break end
      for _,k in ipairs(edit.path)do if type(k)~="string"and type(k)~="number"then valid=false end end
     end
     if valid then patches[name]=patch;state.learned[name]=true end
    end
   end
  end)
 end
 api.LearnSmartPass=function(name)
  if not table.find(styles,name)then return false,"Choose Overhead, Chest or Bounce"end
  armed=name;state.status="Learning "..name..": perform that pass normally to the same teammate"
  return true,state.status
 end
 api.ResetSmartPass=function()
  templates={};patches={};state.learned={};armed=nil;persist()
  state.status="Pass mappings cleared";return true,state.status
 end
 api.GetSmartPassState=function()local out=table.clone(state);out.learned=table.clone(state.learned);return out end
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
    defenders[#defenders+1]={position=dr.Position,velocity=dr.AssemblyLinearVelocity or Vector3.zero,
     high=attr(other,"HoldingG")==true or attr(other,"Action")=="Blocking",low=attr(other,"Action")=="Stealing"}
   end
  end
  local best,bestScore
  for _,name in ipairs(styles)do
   if patches[name]then
    local speed=(settings.passSpeed or 80)*(name=="Chest"and 1.15 or name=="Bounce"and .85 or 1)
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
    local score=risk*10+travel*.8+(name=="Chest"and 0 or name=="Overhead"and .08 or .14)
    if not bestScore or score<bestScore then best=name;bestScore=score end
   end
  end
  return best,bestScore
 end
 local function numberHeld()
  for _,key in ipairs({Enum.KeyCode.One,Enum.KeyCode.Two,Enum.KeyCode.Three,Enum.KeyCode.Four})do
   if Input:IsKeyDown(key)then return key end
  end
  if recentKey and os.clock()-(recentAt or 0)<.25 then return recentKey end
 end
 local function bind()
  local service=keyboard and keyboard.Services and keyboard.Services.InputService
  local signal=service and service.Pass
  if not signal or type(signal.Fire)~="function"then state.status="Native pass controller unavailable";return end
  if hook and hook.signal==signal then return end
  if hook and hook.signal.Fire==hook.wrapper then hook.signal.Fire=hook.original end
  local original=signal.Fire
  local wrapper=function(self,...)
   if stopped then return original(self,...)end
   local args=table.pack(...);local edited=args
   local ok,problem=pcall(function()
    local own=model();if not own then return end
    local target=receiver(args,own)
    if armed then
     if not target then state.status="Receiver encoding unresolved; capture the native pass arguments";return end
     local name=armed;armed=nil;templates[name]={args=copy(args),receiver=target}
     state.status="Recorded "..name;rebuild();return
    end
    if not settings.smartPass or not holdsBall(own)or Input:GetFocusedTextBox()or not numberHeld()then return end
    if Input:IsKeyDown(Enum.KeyCode.LeftShift)or Input:IsKeyDown(Enum.KeyCode.RightShift)then state.status="Manual modified pass preserved";return end
    if not target then state.status="Receiver encoding unresolved; original pass preserved";return end
    for _,name in ipairs({"Chest","Bounce"})do
     for _,edit in ipairs(patches[name]or {})do
      if get(args,edit.path)~=edit.before then state.status="Manual modified pass preserved";return end
     end
    end
    local name=choose(own,target)
    if not name then state.status="Teach the three pass types once; original pass preserved";return end
    local candidate=copy(args)
    for _,edit in ipairs(patches[name])do
     if get(candidate,edit.path)~=edit.before then state.status="Manual type or changed encoding; original pass preserved";return end
     if not put(candidate,edit.path,edit.after)then return end
    end
    if receiver(candidate,own)~=target then state.status="Receiver validation failed; original pass preserved";return end
    edited=candidate;state.passes=state.passes+1;state.choice=name;state.target=target.Name
    state.status=name.." to "..target.Name.." · estimated lane risk"
   end)
   if not ok then edited=args;state.status="Smart pass fallback: "..tostring(problem)end
   -- Exactly one original invocation. No replay, duplicate send or guessed schema.
   return original(self,table.unpack(edited,1,edited.n))
  end
  local ok,err=pcall(function()signal.Fire=wrapper end)
  if ok then hook={signal=signal,original=original,wrapper=wrapper};state.status="Connected · teach pass types once or use saved mappings"
  else state.status="Pass handler cannot be wrapped: "..tostring(err)end
 end
 local input=Input.InputBegan:Connect(function(event,processed)
  if processed or Input:GetFocusedTextBox()then return end
  for _,key in ipairs({Enum.KeyCode.One,Enum.KeyCode.Two,Enum.KeyCode.Three,Enum.KeyCode.Four})do
   if event.KeyCode==key then recentKey=key;recentAt=os.clock();break end
  end
 end)
 local lastBind=-math.huge
 local connection=Run.Heartbeat:Connect(function()
  if stopped or os.clock()-lastBind<.5 then return end;lastBind=os.clock()
  local ok,err=pcall(bind);if not ok then state.status="Pass binding unavailable: "..tostring(err)end
 end)
 local previous=featureCleanup
 featureCleanup=function()
  stopped=true;input:Disconnect();connection:Disconnect()
  if hook then pcall(function()if hook.signal.Fire==hook.wrapper then hook.signal.Fire=hook.original end end)end
  previous()
 end
end)()end

-- END SMART PASS



