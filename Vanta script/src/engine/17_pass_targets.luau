-- Catch-and-shoot spacing estimates; no automatic pass or shot inputs.
if api then (function()
 settings.passVision=false;settings.passSpeed=80;settings.catchRelease=.5
 numericLimits.passSpeed={40,140};numericLimits.catchRelease={.2,1}
 local snapshots=setmetatable({},{__mode="k"})
 local results={}
 local preferred
 local function evaluate(origin,receiver,velocity,defenders,latency)
  local distance=flat(receiver-origin).Magnitude
  local travel=distance/settings.passSpeed+math.clamp(latency or 0,0,.3)
  local predicted=receiver+velocity*math.min(travel,.8)
  travel=flat(predicted-origin).Magnitude/settings.passSpeed+math.clamp(latency or 0,0,.3)
  local nearest,arrival,lane=math.huge,math.huge,false
  local nearestDefender
  local path=flat(predicted-origin)
  for _,d in ipairs(defenders)do
   local gap=flat(d.position-predicted).Magnitude
   if gap<nearest then nearest=gap;nearestDefender=d.position end
   local speed=math.clamp(math.max(16,d.speed or 0),16,28)
   arrival=math.min(arrival,math.max(0,gap-5)/speed+.12)
   local t=math.clamp(flat(d.position-origin):Dot(path)/math.max(path:Dot(path),.01),0,1)
   if t>.05 and t<.95 then
    local across=math.max(0,flat(d.position-(origin+path*t)).Magnitude-2.5)/speed
    if across<=travel*t then lane=true end
   end
  end
  local margin=arrival-travel-settings.catchRelease
  return {distance=distance,passTime=travel,nearest=nearest,nearestDefender=nearestDefender,defenderTime=arrival,margin=margin,laneBlocked=lane,reliable=#defenders>0,position=predicted}
 end
 api.EvaluatePassTarget=evaluate
 api.GetPassTargets=function()local r={}for i,v in ipairs(results)do r[i]=table.clone(v)end return r end
 local board=Instance.new("ScreenGui");board.Name="PracticalPassTargets";board.ResetOnSpawn=false;board.Enabled=false;board.Parent=P.PlayerGui
 local panel=create("Frame",{AnchorPoint=Vector2.new(1,1),Position=UDim2.new(1,-24,1,-155),Size=UDim2.fromOffset(264,94),BackgroundColor3=Color3.fromRGB(15,20,29),BackgroundTransparency=.12,BorderSizePixel=0},board)
 if settings._hudRegister then settings._hudRegister(panel,10)end
 create("UICorner",{CornerRadius=UDim.new(0,10)},panel)
 create("UIStroke",{Color=Color3.fromRGB(54,72,98),Thickness=1},panel)
 local heading=create("TextLabel",{BackgroundTransparency=1,Position=UDim2.fromOffset(14,7),Size=UDim2.fromOffset(236,18),Font=Enum.Font.GothamBold,TextSize=12,TextXAlignment=Enum.TextXAlignment.Left,TextColor3=Color3.fromRGB(160,170,192),Text="PASS TARGETS"},panel)
 local labels={}
 for i=1,3 do labels[i]=create("TextLabel",{BackgroundTransparency=1,Position=UDim2.fromOffset(14,30+(i-1)*20),Size=UDim2.fromOffset(236,18),TextTruncate=Enum.TextTruncate.AtEnd,Font=Enum.Font.GothamMedium,TextSize=13,TextXAlignment=Enum.TextXAlignment.Left,TextColor3=Color3.fromRGB(175,184,200),Text=""},panel)end
 local outline=create("Highlight",{Name="SuggestedPassTarget",Enabled=false,FillTransparency=.9,OutlineTransparency=.05,DepthMode=Enum.HighlightDepthMode.Occluded},workspace)
 local targetVisuals={}
 local function tint(v)
  return not v.reliable and Color3.fromRGB(178,186,206)or(v.laneBlocked or v.obstructed)and Color3.fromRGB(255,125,138)or v.open and Color3.fromRGB(104,228,176)or Color3.fromRGB(255,200,102)
 end
 for i=1,3 do
  local item={ring={},halo={}}
  item.closeout=visualPart("Receiver defender closeout "..i,Color3.fromRGB(242,178,87));item.closeout.Transparency=1
  item.highlight=create("Highlight",{Name="PassOption"..i,Enabled=false,FillTransparency=1,OutlineTransparency=.25,DepthMode=Enum.HighlightDepthMode.Occluded},workspace)
  item.badge=create("BillboardGui",{Name="PassTargetBadge"..i,Enabled=false,AlwaysOnTop=true,Size=UDim2.fromOffset(176,48),StudsOffsetWorldSpace=Vector3.new(0,4.2,0),MaxDistance=110},board)
  item.cardHalo={}
  for layer=1,1 do
   local spread=layer*3
   local glow=create("Frame",{Name="PassCardHalo",Position=UDim2.fromOffset(-spread,-spread),Size=UDim2.new(1,spread*2,1,spread*2),BackgroundTransparency=1},item.badge)
   create("UICorner",{CornerRadius=UDim.new(0,11+spread)},glow)
   item.cardHalo[layer]=create("UIStroke",{Color=Color3.fromRGB(86,215,164),Transparency=.7+layer*.07,Thickness=2},glow)
  end
  item.card=create("Frame",{Size=UDim2.fromScale(1,1),BackgroundColor3=Color3.fromRGB(12,19,27),BackgroundTransparency=.12,BorderSizePixel=0},item.badge)
  create("UICorner",{CornerRadius=UDim.new(0,11)},item.card)
  item.stroke=create("UIStroke",{Color=Color3.fromRGB(86,215,164),Transparency=.35,Thickness=1.25},item.card)
  item.label=create("TextLabel",{Position=UDim2.fromOffset(10,5),Size=UDim2.new(1,-20,0,20),BackgroundTransparency=1,BorderSizePixel=0,Font=Enum.Font.GothamBold,TextSize=14,TextXAlignment=Enum.TextXAlignment.Left,TextTruncate=Enum.TextTruncate.AtEnd,TextColor3=Color3.fromRGB(220,230,245)},item.card)
  item.detail=create("TextLabel",{Position=UDim2.fromOffset(10,25),Size=UDim2.new(1,-20,0,14),BackgroundTransparency=1,Font=Enum.Font.Gotham,TextSize=11,TextXAlignment=Enum.TextXAlignment.Left,TextColor3=Color3.fromRGB(192,207,224)},item.card)
  item.track=create("Frame",{Position=UDim2.new(0,10,1,-7),Size=UDim2.new(1,-20,0,3),BackgroundColor3=Color3.fromRGB(48,62,78),BorderSizePixel=0},item.card)
  local track=item.track
  create("UICorner",{CornerRadius=UDim.new(1,0)},track)
  item.window=create("Frame",{Size=UDim2.fromScale(0,1),BackgroundColor3=Color3.fromRGB(86,215,164),BorderSizePixel=0},track)
  create("UICorner",{CornerRadius=UDim.new(1,0)},item.window)
  item.windowAmount=0
  for j=1,32 do
   item.ring[j]=visualPart("Pass target underline "..i.." "..j,Color3.fromRGB(86,215,164));item.ring[j].Transparency=1
   item.halo[j]=visualPart("Pass target halo "..i.." "..j,Color3.fromRGB(86,215,164));item.halo[j].Transparency=1
  end
  targetVisuals[i]=item
 end
 -- Screen-space chips replace the old world billboards: slim pills above each target that
 -- push each other apart on screen so they never sit on top of one another.
 local HS=settings._hudStyle
 local chipStroke=HS and HS.c.stroke or Color3.fromRGB(40,47,64)
 local chipGui=create("ScreenGui",{Name="PracticalPassChips",ResetOnSpawn=false,IgnoreGuiInset=true,Enabled=false,DisplayOrder=5},P.PlayerGui)
 local chips={}
 for i=1,3 do
  local frame=create("Frame",{Name="Chip"..i,AnchorPoint=Vector2.new(.5,1),Size=UDim2.fromOffset(122,24),BackgroundColor3=Color3.fromRGB(11,16,24),BackgroundTransparency=.08,BorderSizePixel=0,Visible=false},chipGui)
  create("UICorner",{CornerRadius=UDim.new(0,12)},frame)
  local stroke=create("UIStroke",{Color=Color3.fromRGB(86,215,164),Thickness=1.5,Transparency=.1},frame)
  local dot=create("Frame",{AnchorPoint=Vector2.new(0,.5),Position=UDim2.new(0,9,.5,0),Size=UDim2.fromOffset(8,8),BackgroundColor3=Color3.fromRGB(86,215,164),BorderSizePixel=0},frame)
  create("UICorner",{CornerRadius=UDim.new(1,0)},dot)
  local name=create("TextLabel",{BackgroundTransparency=1,Position=UDim2.fromOffset(24,0),Size=UDim2.new(1,-70,1,0),Font=Enum.Font.GothamBold,TextSize=12,TextXAlignment=Enum.TextXAlignment.Left,TextTruncate=Enum.TextTruncate.AtEnd,TextColor3=Color3.fromRGB(246,248,252)},frame)
  local value=create("TextLabel",{BackgroundTransparency=1,AnchorPoint=Vector2.new(1,0),Position=UDim2.new(1,-9,0,0),Size=UDim2.new(0,44,1,0),Font=Enum.Font.GothamBold,TextSize=12,TextXAlignment=Enum.TextXAlignment.Right,TextColor3=Color3.fromRGB(86,215,164)},frame)
  local sub=create("TextLabel",{BackgroundTransparency=1,Position=UDim2.fromOffset(24,21),Size=UDim2.new(1,-34,0,15),Font=Enum.Font.GothamMedium,TextSize=11,TextXAlignment=Enum.TextXAlignment.Left,TextColor3=Color3.fromRGB(190,198,216),Visible=false},frame)
  chips[i]={frame=frame,stroke=stroke,dot=dot,name=name,value=value,sub=sub}
  if HS then HS.skin(frame,12,false)end
 end
 local function hideChips()
  chipGui.Enabled=false
  for _,chip in ipairs(chips)do chip.frame.Visible=false;chip.x=nil end
 end
 local function layoutChips(data,dt)
  local camera=workspace.CurrentCamera
  local order={}
  if camera then
   for i=1,3 do
    local d=data[i]
    if d then
     local point,onScreen=camera:WorldToViewportPoint(d.position+Vector3.new(0,3.5,0))
     if onScreen and point.Z>0 then order[#order+1]={i=i,x=point.X,y=point.Y,d=d}end
    end
   end
  end
  if #order==0 then hideChips();return end
  table.sort(order,function(a,b)return a.y>b.y end)
  local viewport=camera.ViewportSize
  local blend=1-math.exp(-math.clamp(dt or 1/60,0,.1)*16)
  local placed,shown={},{}
  for _,o in ipairs(order)do
   local chip=chips[o.i];local d=o.d;local primary=o.i==1
   local w,h=primary and 156 or 122,primary and 38 or 24
   local x=math.clamp(o.x,w/2+8,math.max(w/2+8,viewport.X-w/2-8))
   local y=math.clamp(o.y,h+8,math.max(h+8,viewport.Y-8))
   -- Push up until clear of every chip already placed (at most three chips).
   for _=1,3 do
    for _,p in ipairs(placed)do
     if math.abs(x-p.x)<(w+p.w)/2+4 and y>p.top-5 and y-h<p.y+5 then y=p.top-6 end
    end
   end
   placed[#placed+1]={x=x,y=y,top=y-h,w=w}
   shown[o.i]=true
   if not chip.x or(Vector2.new(chip.x,chip.y)-Vector2.new(x,y)).Magnitude>320 then chip.x=x;chip.y=y
   else chip.x=chip.x+(x-chip.x)*blend;chip.y=chip.y+(y-chip.y)*blend end
   chip.frame.Size=UDim2.fromOffset(w,h)
   chip.frame.Position=UDim2.fromOffset(chip.x,chip.y)
   chip.frame.Visible=true
   -- Neutral menu-style outline; the best target's outline picks up its status colour.
   chip.stroke.Color=primary and chipStroke:Lerp(d.color,.55)or chipStroke;chip.stroke.Thickness=d.urgent and 1.6 or 1
   chip.dot.BackgroundColor3=d.color;chip.value.TextColor3=d.color
   chip.name.Text=d.name;chip.name.TextSize=primary and 14 or 12
   chip.name.Position=primary and UDim2.fromOffset(24,2)or UDim2.fromOffset(24,0)
   chip.name.Size=primary and UDim2.new(1,-74,0,19)or UDim2.new(1,-70,1,0)
   chip.value.Size=primary and UDim2.new(0,48,0,19)or UDim2.new(0,44,1,0)
   chip.value.Position=primary and UDim2.new(1,-9,0,2)or UDim2.new(1,-9,0,0)
   chip.value.Text=d.reliable and string.format("%+.2f",d.margin)or "--"
   chip.sub.Visible=primary;chip.sub.Text=d.short
  end
  for i,chip in ipairs(chips)do if not shown[i]then chip.frame.Visible=false;chip.x=nil end end
  chipGui.Enabled=true
 end
 local lane={}
 for i=1,9 do lane[i]=visualPart("Recommended pass lane "..i,Color3.fromRGB(86,215,164));lane[i].Transparency=1 end
 local ballOutline=create("Highlight",{Name="PassPossessionBall",Enabled=false,FillTransparency=1,OutlineColor=Color3.fromRGB(94,182,255),OutlineTransparency=.05,DepthMode=Enum.HighlightDepthMode.Occluded},workspace)
 local function hideCourt()
  hideChips()
  ballOutline.Enabled=false
  for _,item in ipairs(targetVisuals)do item.closeout.Transparency=1 end
  for _,item in ipairs(targetVisuals)do item.badge.Enabled=false;item.highlight.Enabled=false;for _,part in ipairs(item.ring)do part.Transparency=1 end;for _,part in ipairs(item.halo)do part.Transparency=1 end end
  for _,part in ipairs(lane)do part.Transparency=1 end
 end
 local function updateCourt(dt)
  local own=model();local root=rootOf(own)
  if not settings.passVision or not root or not holdsBall(own)or settings._visualV2 and not settings.visualShowPass then hideCourt();return end
  local ball=attr(own,"Basketball")
  local hasBall=typeof(ball)=="Instance"
  if hasBall then ballOutline.Adornee=ball end
  if ballOutline.Enabled~=hasBall then ballOutline.Enabled=hasBall end
  local function ground(point)return settings._visualV2 and api.GetVisualGroundPoint and api.GetVisualGroundPoint(point)or floorPoint(point,own)end
  local origin=ground(root.Position)
  local drawn={};local laneDrawn=false;local chipData={}
  for i=1,math.min(3,#results)do
   local v=results[i];local r=rootOf(v.character);local item=targetVisuals[i]
   if r then
    drawn[i]=true
    local proxy=attr(v.character,"ProxyCharacter")
    local renderModel=typeof(proxy)=="Instance"and proxy or v.character
    local renderRoot=typeof(proxy)=="Instance"and(proxy:IsA("BasePart")and proxy or rootOf(proxy))or nil
    renderRoot=renderRoot or r
    local color=tint(v);local center=ground(renderRoot.Position)
    local urgent=i==1 and v.wideOpen
    item.closeout.Transparency=1
    if i==1 and v.nearestDefender then
     local defender=ground(v.nearestDefender);local delta=flat(defender-center)
     local edge=center+(delta.Magnitude>.01 and delta.Unit or Vector3.new(1,0,0))*math.min(5,delta.Magnitude)
     ribbon(item.closeout,defender,edge,.09);item.closeout.Color=tint(v);item.closeout.Transparency=.35
    end
    if urgent then color=Color3.fromRGB(67,236,181)end
    item.highlight.Adornee=renderModel;item.highlight.OutlineColor=color;item.highlight.FillColor=color;item.highlight.FillTransparency=urgent and .83 or 1;item.highlight.OutlineTransparency=urgent and 0 or .3;item.highlight.Enabled=true
    item.badge.Adornee=renderRoot;item.badge.Enabled=false;item.label.TextColor3=color
    -- Compact cards: only the best target gets a detail line and timing bar.
    local primary=i==1
    local short=urgent and "PASS NOW"or not v.reliable and "NO DATA"or(v.laneBlocked or v.obstructed)and "BLOCKED"or v.open and "OPEN"or "CLOSING"
    item.label.Text=v.name.."  ·  "..short
    item.label.TextSize=primary and 14 or 12
    item.label.Position=primary and UDim2.fromOffset(10,5)or UDim2.fromOffset(10,4)
    item.detail.Visible=primary;item.track.Visible=primary
    item.detail.Text=not v.reliable and "no defender data"or string.format("%+.2fs shot window",v.margin)
    item.badge.Size=primary and(urgent and UDim2.fromOffset(196,52)or UDim2.fromOffset(176,48))or UDim2.fromOffset(150,28)
    chipData[i]={position=renderRoot.Position,color=color,name=v.name,margin=v.margin,reliable=v.reliable,urgent=urgent,
     short=urgent and "PASS NOW"or not v.reliable and "NO DATA"or(v.laneBlocked or v.obstructed)and "BLOCKED"or v.open and "OPEN LANE"or "DEFENDER CLOSING"}
    item.stroke.Color=color;item.window.BackgroundColor3=color
    for layer,stroke in ipairs(item.cardHalo)do stroke.Color=color;stroke.Transparency=urgent and(.58+layer*.09)or(.78+layer*.045)end
    local amount=v.reliable and math.clamp(v.margin/1.25,0,1)or 0
    item.windowAmount=item.windowAmount+(amount-item.windowAmount)*(1-math.exp(-math.clamp(dt or 1/60,0,.1)*12))
    item.window.Size=UDim2.fromScale(item.windowAmount,1)
    for j,part in ipairs(item.ring)do repeat
     -- Only the best target keeps a ground ring; the others are just their chip.
     if i~=1 then part.Transparency=1;item.halo[j].Transparency=1;break end
     local radius=(urgent and 2.2 or 1.25)*(settings.visualScale or 1)
     local a=math.pi*.12+(j-1)*math.pi*1.75/32;local b=math.pi*.12+j*math.pi*1.75/32
     local weight=settings._visualV2 and (settings.visualWeight or 1)or 1
     ribbon(part,center+Vector3.new(math.cos(a)*radius,0,math.sin(a)*radius),center+Vector3.new(math.cos(b)*radius,0,math.sin(b)*radius),(i==1 and .075 or .04)*weight)
     part.Color=color;part.Transparency=(i==1 and .18 or .5)+(settings._visualV2 and (1-(settings.visualIntensity or 80)/100)*.3 or 0)
     local halo=item.halo[j]
     if settings._visualV2 and i==1 then
      ribbon(halo,center+Vector3.new(math.cos(a)*(radius+.2),.025,math.sin(a)*(radius+.2)),center+Vector3.new(math.cos(b)*(radius+.2),.025,math.sin(b)*(radius+.2)),.18*weight)
      halo.Color=color;halo.Transparency=urgent and .65 or .88
     elseif halo.Transparency~=1 then halo.Transparency=1 end
    until true end
    if i==1 and v.open then
     laneDrawn=true
     local delta=center-origin
     if settings._visualV2 and api.DrawVisualRoute then
      api.DrawVisualRoute(lane,origin,center,.055*(settings.visualWeight or 1),color,.4)
     else for j,part in ipairs(lane)do
      ribbon(part,origin+delta*((j-.8)/9),origin+delta*((j-.35)/9),.085);part.Color=color;part.Transparency=.4
     end end
    end
   end
  end
  for i,item in ipairs(targetVisuals)do if not drawn[i]then
   item.closeout.Transparency=1
   if item.badge.Enabled then item.badge.Enabled=false end
   if item.highlight.Enabled then item.highlight.Enabled=false end
   for _,part in ipairs(item.ring)do if part.Transparency~=1 then part.Transparency=1 end end
   for _,part in ipairs(item.halo)do if part.Transparency~=1 then part.Transparency=1 end end
  end end
  if not laneDrawn then for _,part in ipairs(lane)do if part.Transparency~=1 then part.Transparency=1 end end end
  layoutChips(chipData,dt)
 end
 local courtConnection=(Run.RenderStepped or Run.Heartbeat):Connect(updateCourt)
 local function velocity(m,position,now)
  local previous=snapshots[m];local result=Vector3.zero
  if previous then
   local dt=now-previous.time
   if dt>.02 and dt<.6 then result=flat(position-previous.position)/dt;if result.Magnitude>28 then result=Vector3.zero end end
  end
  snapshots[m]={position=position,time=now};return result
 end
 local last=0
 local connection=Run.Heartbeat:Connect(function()
  local now=os.clock();if now-last<.12 then return end;last=now
  if not settings.passVision then board.Enabled=false;outline.Enabled=false;results={};return end
  local own=model();local root=rootOf(own)
  if not root or not holdsBall(own)then board.Enabled=false;outline.Enabled=false;results={};return end
  results={}
  board.Enabled=true;heading.Text="PASS TARGETS"
  panel.Visible=not settings._visualV2 or settings.visualPassPanel and settings.visualShowPass
  local team=attr(own,"Team")
  if team==nil or team==0 then labels[1].Text="Waiting for reliable team information";labels[2].Text="";labels[3].Text="";outline.Enabled=false;return end
  local mates,defenders={},{}
  for _,m in ipairs(defenseCharacters())do
   local r=rootOf(m)
   local ownCourt,otherCourt=attr(own,"CourtNumber")or attr(own,"Court"),attr(m,"CourtNumber")or attr(m,"Court")
   local sameCourt=not(ownCourt~=nil and otherCourt~=nil and ownCourt~=0 and otherCourt~=0 and ownCourt~=otherCourt)
   if r and m~=own and sameCourt and math.abs(r.Position.Y-root.Position.Y)<10 then
    local v=velocity(m,r.Position,now);local t=attr(m,"Team")
    if t==team then table.insert(mates,{model=m,position=r.Position,velocity=v})
    elseif t~=nil and t~=0 then table.insert(defenders,{position=r.Position,speed=v.Magnitude})end
   end
  end
  for _,mate in ipairs(mates)do
   if flat(mate.position-root.Position).Magnitude<=100 then
    local value=evaluate(root.Position,mate.position,mate.velocity,defenders,ping)
    value.name=mate.model.Name;value.character=mate.model
    local params=RaycastParams.new();params.FilterType=Enum.RaycastFilterType.Exclude
    local excluded={own,mate.model,visualFolder}
    local chars=workspace:FindFirstChild("Characters");if chars then table.insert(excluded,chars)end
    local ball=attr(own,"Basketball");if typeof(ball)=="Instance"then table.insert(excluded,ball)end
    params.FilterDescendantsInstances=excluded
    value.obstructed=workspace:Raycast(root.Position+Vector3.new(0,2,0),value.position-root.Position,params)~=nil
    value.open=value.reliable and not value.laneBlocked and not value.obstructed and value.margin>.15
    local goal=goalPosition(attr(mate.model,"Goal"))or goalPosition(attr(own,"Goal"))
    value.wideOpen=value.open and value.nearest>=10 and value.margin>.3 and goal~=nil and flat(goal-mate.position).Magnitude<=65
    value.score=(value.wideOpen and 150 or value.open and 100 or 0)+math.clamp(value.margin,-5,5)-(value.laneBlocked and 10 or 0)-(value.obstructed and 20 or 0)
    table.insert(results,value)
   end
  end
  table.sort(results,function(a,b)return a.score>b.score end)
  -- Keep similarly ranked open targets stable instead of flashing between teammates.
  if preferred and results[1]and results[1].open then
   for i=2,#results do
    if results[i].character==preferred and results[i].open and results[1].score-results[i].score<.25 then
     local retained=table.remove(results,i);table.insert(results,1,retained);break
    end
   end
  end
  preferred=results[1]and results[1].open and results[1].character or nil
  if #results==0 then labels[1].Text="No nearby teammates detected"end
  for i=1,math.min(3,#results)do
   local v=results[i]
   local verdict=not v.reliable and "NO DATA"or v.obstructed and "BLOCKED"or v.laneBlocked and "AT RISK"or v.open and "OPEN"or "CLOSING"
   labels[i].Text=v.reliable and string.format("%d  %s  ·  %s  ·  %+.2fs",i,v.name,verdict,v.margin)or string.format("%d  %s  ·  %s",i,v.name,verdict)
   labels[i].TextColor3=v.open and Color3.fromRGB(104,228,176)or Color3.fromRGB(255,200,102)
  end
  for i=math.max(2,#results+1),3 do if labels[i].Text~=""then labels[i].Text=""end end
  local showBest=results[1]~=nil and results[1].open and not settings._visualV2
  if showBest then outline.Adornee=results[1].character;outline.FillColor=Color3.fromRGB(53,186,135);outline.OutlineColor=Color3.fromRGB(86,215,164)end
  if outline.Enabled~=showBest then outline.Enabled=showBest end
 end)
 -- Persistent geometry: no per-frame instances or blinking visibility timers.
 local shotBoard=create("ScreenGui",{Name="CatchShootLiveReadout",Enabled=false,ResetOnSpawn=false,DisplayOrder=4},P.PlayerGui)
 local shotCard=create("Frame",{Size=UDim2.fromOffset(212,46),BackgroundColor3=Color3.fromRGB(12,19,27),BackgroundTransparency=.1,BorderSizePixel=0},shotBoard)
 if settings._hudRegister then settings._hudRegister(shotCard,5)end
 create("UICorner",{CornerRadius=UDim.new(0,12)},shotCard)
 local shotStroke=create("UIStroke",{Thickness=1.5,Color=Color3.fromRGB(94,182,255)},shotCard)
 local shotTitle=create("TextLabel",{BackgroundTransparency=1,Position=UDim2.fromOffset(10,5),Size=UDim2.new(1,-20,0,20),Font=Enum.Font.GothamBold,TextSize=14,TextXAlignment=Enum.TextXAlignment.Left,TextTruncate=Enum.TextTruncate.AtEnd,TextColor3=Color3.fromRGB(94,182,255),Text=""},shotCard)
 local shotDetail=create("TextLabel",{BackgroundTransparency=1,Position=UDim2.fromOffset(10,25),Size=UDim2.new(1,-20,0,16),Font=Enum.Font.Gotham,TextSize=12,TextXAlignment=Enum.TextXAlignment.Left,TextTruncate=Enum.TextTruncate.AtEnd,TextColor3=Color3.fromRGB(226,230,240),Text=""},shotCard)
 local shotRing={}
 for i=1,40 do shotRing[i]=visualPart("Estimated contest boundary "..i,Color3.fromRGB(94,182,255));shotRing[i].Transparency=1 end
 local closeoutLine=visualPart("Nearest defender to contest boundary",Color3.fromRGB(242,178,87));closeoutLine.Transparency=1
 local defenderMark=create("Highlight",{Name="CatchShootNearestDefender",Enabled=false,FillTransparency=1,OutlineTransparency=.15,OutlineColor=Color3.fromRGB(242,178,87),DepthMode=Enum.HighlightDepthMode.Occluded},workspace)
 local function hideShot()
  shotBoard.Enabled=false;defenderMark.Enabled=false;closeoutLine.Transparency=1
  for _,part in ipairs(shotRing)do part.Transparency=1 end
 end
 local shotConnection=(Run.RenderStepped or Run.Heartbeat):Connect(function()
  local own=model();local root=rootOf(own)
  if not settings.catchShoot or not root or settings._visualV2 and not settings.visualShowPass then hideShot();return end
  local state=api.GetCatchShootState and api.GetCatchShootState()or {}
  if not holdsBall(own)and not state.preholding and not state.pending then hideShot();return end
  local center=floorPoint(root.Position,own)
  local nearest,other=math.huge,nil
  local ownTeam=attr(own,"Team")
  for _,character in ipairs(defenseCharacters())do
   local r=rootOf(character);local team=attr(character,"Team")
   local court=attr(character,"CourtNumber")or attr(character,"Court")
   local ownCourt=attr(own,"CourtNumber")or attr(own,"Court")
   local function normalized(v)return tostring(v or ""):lower():gsub("^court[%s_%-]*","")end
   local sameCourt=not court or not ownCourt or normalized(court)==normalized(ownCourt)
   if r and character~=own and ownTeam and ownTeam~=0 and team and team~=0 and team~=ownTeam and sameCourt and math.abs(r.Position.Y-root.Position.Y)<10 then
    local distance=flat(r.Position-root.Position).Magnitude
    if distance<nearest then nearest=distance;other=character end
   end
  end
  local radius=10*(1-math.clamp(settings.catchShootContest or 10,0,100)/100)
  local arrival=other and math.max(0,nearest-radius)/16 or nil
  local release=(settings.catchRelease or .5)+math.min(ping or 0,.2)*.5+.06
  local margin=arrival and arrival-release or nil
  local safe=margin and margin>.15
  local color=not other and Color3.fromRGB(178,186,206)or safe and Color3.fromRGB(104,228,176)or Color3.fromRGB(255,200,102)
  shotBoard.Enabled=true;shotTitle.TextColor3=color
  shotTitle.Text=attr(own,"Action")=="Shooting"and "SHOOTING"or state.preholding and "SHOT QUEUED"or state.shooting and "SHOT SENT"or safe and "OPEN SHOT"or other and "CLOSEOUT RISK"or "NO DEFENDER DATA"
  shotDetail.Text=other and string.format("%s · %.1f studs · %+.2fs",other.Name,math.max(0,nearest-radius),margin or 0)or "no same-court defender seen"
  if state.preholding then shotDetail.Text=tostring(state.bufferReason or state.status or "waiting for catch")end
  for i,part in ipairs(shotRing)do
   local a=(i-1)*math.pi*2/40;local b=i*math.pi*2/40
   ribbon(part,center+Vector3.new(math.cos(a)*radius,0,math.sin(a)*radius),center+Vector3.new(math.cos(b)*radius,0,math.sin(b)*radius),.055)
   part.Color=color;part.Transparency=.42
  end
  defenderMark.Enabled=other~=nil;closeoutLine.Transparency=1
  if other then
   local proxy=attr(other,"ProxyCharacter");defenderMark.Adornee=typeof(proxy)=="Instance"and proxy or other
   local point=floorPoint(rootOf(other).Position,own);local delta=flat(point-center)
   local edge=center+(delta.Magnitude>.01 and delta.Unit or Vector3.new(1,0,0))*math.min(radius,delta.Magnitude)
   ribbon(closeoutLine,point,edge,.11);closeoutLine.Color=color;closeoutLine.Transparency=.22
  end
 end)
 local priorShotCleanup=featureCleanup
 featureCleanup=function()shotConnection:Disconnect();hideShot();shotBoard:Destroy();defenderMark:Destroy();closeoutLine:Destroy();for _,part in ipairs(shotRing)do part:Destroy()end;priorShotCleanup()end
 local previous=featureCleanup
 featureCleanup=function()connection:Disconnect();courtConnection:Disconnect();hideCourt();for _,item in ipairs(targetVisuals)do item.closeout:Destroy();item.highlight:Destroy();for _,part in ipairs(item.ring)do part:Destroy()end;for _,part in ipairs(item.halo)do part:Destroy()end end;for _,part in ipairs(lane)do part:Destroy()end;ballOutline:Destroy();chipGui:Destroy();board:Destroy();outline:Destroy();settings.passVision=false;previous()end
end)()end

