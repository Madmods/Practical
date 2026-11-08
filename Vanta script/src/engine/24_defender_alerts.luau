-- Defender alerts: edge-of-screen badges pointing at nearby opponents you cannot see.
-- Information only: reads positions, sends no input and moves nothing.
do (function()
 settings.defenderAlerts=false;settings.alertBallOnly=false;settings.alertRange=16
 numericLimits.alertRange={6,40}
 local style=settings._hudStyle
 local palette=style and style.c or {surface=Color3.fromRGB(17,20,29),stroke=Color3.fromRGB(40,47,64),
  text=Color3.fromRGB(246,248,252),sub=Color3.fromRGB(178,186,206),amber=Color3.fromRGB(255,200,102),danger=Color3.fromRGB(255,125,138)}
 local gui=create("ScreenGui",{Name="PracticalDefenderAlerts",ResetOnSpawn=false,IgnoreGuiInset=true,Enabled=false,DisplayOrder=6},P.PlayerGui)
 local badges={}
 for i=1,4 do
  local frame=create("Frame",{Name="Alert"..i,AnchorPoint=Vector2.new(.5,.5),Size=UDim2.fromOffset(38,38),BackgroundColor3=palette.surface,BackgroundTransparency=.05,BorderSizePixel=0,Visible=false},gui)
  create("UICorner",{CornerRadius=UDim.new(1,0)},frame)
  local stroke=create("UIStroke",{Color=palette.stroke,Thickness=1.5},frame)
  -- A small diamond sits on the outer side of the badge, on the side the opponent is.
  local pointer=create("Frame",{AnchorPoint=Vector2.new(.5,.5),Size=UDim2.fromOffset(11,11),Rotation=45,BackgroundColor3=palette.text,BorderSizePixel=0},frame)
  local number=create("TextLabel",{BackgroundTransparency=1,Size=UDim2.fromScale(1,1),Font=Enum.Font.GothamBold,TextSize=14,TextColor3=palette.text,Text=""},frame)
  if style then style.skin(frame,19,false);stroke.Thickness=1.5 end
  badges[i]={frame=frame,stroke=stroke,pointer=pointer,number=number}
 end
 local snapshots=setmetatable({},{__mode="k"})
 local function enemy(own,other)
  local a,b=attr(own,"Team"),attr(other,"Team")
  if a~=nil and b~=nil and a~=0 and b~=0 and a~=""and b~=""then return a~=b end
  local players=game:GetService("Players")
  local otherPlayer=players:GetPlayerFromCharacter(other)or players:FindFirstChild(other.Name)
  return otherPlayer~=nil and P.Team~=nil and otherPlayer.Team~=nil and otherPlayer.Team~=P.Team
 end
 local function sameCourt(a,b)
  local x,y=attr(a,"CourtNumber"),attr(b,"CourtNumber")
  return not(x~=nil and y~=nil and x~=0 and y~=0 and x~=y)
 end
 local function hideAll()
  gui.Enabled=false
  for _,badge in ipairs(badges)do badge.frame.Visible=false;badge.owner=nil;badge.x=nil end
 end
 local connection=(Run.RenderStepped or Run.Heartbeat):Connect(function(dt)
  if not settings.defenderAlerts then if gui.Enabled then hideAll()end;return end
  local camera=workspace.CurrentCamera
  local own=model();local root=rootOf(own)
  if not camera or not root or(settings.alertBallOnly and not holdsBall(own))then hideAll();return end
  local now=os.clock();local viewport=camera.ViewportSize
  local found={}
  for _,other in ipairs(defenseCharacters())do
   local r=rootOf(other)
   if r and other~=own and other.Name~="InvisCharacter" and r.Position.Y>-100 and enemy(own,other)and sameCourt(own,other)then
    local delta=flat(r.Position-root.Position);local distance=delta.Magnitude
    -- Opponent speed from position samples (replicated avatars; measured rather than assumed).
    local previous=snapshots[other]
    local velocity=previous and previous.velocity or Vector3.zero
    if not previous then snapshots[other]={position=r.Position,time=now,velocity=Vector3.zero}
    elseif now-previous.time>=.06 then
     if now-previous.time<.5 then
      velocity=flat(r.Position-previous.position)/(now-previous.time)
      if velocity.Magnitude>40 then velocity=Vector3.zero end
     else velocity=Vector3.zero end
     previous.position=r.Position;previous.time=now;previous.velocity=velocity
    end
    local closing=distance>.1 and -velocity:Dot(delta.Unit)or 0
    if distance<=settings.alertRange and(closing>2 or distance<=7)then
     found[#found+1]={character=other,position=r.Position,distance=distance,closing=closing}
    end
   end
  end
  table.sort(found,function(a,b)return a.distance<b.distance end)
  -- Keep each opponent on the same badge so it does not jump between them.
  local assigned={}
  for _,badge in ipairs(badges)do
   local keep=false
   for i=1,math.min(4,#found)do if found[i].character==badge.owner then keep=true;assigned[i]=badge end end
   if not keep then badge.owner=nil end
  end
  local blend=1-math.exp(-math.clamp(dt or 1/60,0,.1)*16)
  local anyShown=false;local used={}
  for i=1,math.min(4,#found)do
   local item=found[i];local badge=assigned[i]
   if not badge then for _,candidate in ipairs(badges)do if not candidate.owner then badge=candidate;badge.owner=item.character;break end end end
   if badge then
    used[badge]=true
    local screen,onScreen=camera:WorldToViewportPoint(item.position)
    if onScreen and screen.Z>0 then badge.frame.Visible=false;badge.x=nil -- visible on screen already
    else
     -- Direction toward the opponent in camera space (behind you points down the screen).
     local relative=camera.CFrame:PointToObjectSpace(item.position)
     local dx,dy=relative.X,-relative.Y
     if math.abs(dx)+math.abs(dy)<.01 then dx,dy=0,1 end
     local length=math.sqrt(dx*dx+dy*dy);dx,dy=dx/length,dy/length
     local halfX,halfY=viewport.X/2-50,viewport.Y/2-50
     local scale=math.min(halfX/math.max(math.abs(dx),1e-3),halfY/math.max(math.abs(dy),1e-3))
     local x,y=viewport.X/2+dx*scale,viewport.Y/2+dy*scale
     if not badge.x or math.abs(badge.x-x)+math.abs(badge.y-y)>400 then badge.x=x;badge.y=y
     else badge.x=badge.x+(x-badge.x)*blend;badge.y=badge.y+(y-badge.y)*blend end
     local color=(item.distance<=6 or item.closing>8)and palette.danger or(item.distance<=11 or item.closing>4)and palette.amber or palette.sub
     badge.frame.Position=UDim2.fromOffset(badge.x,badge.y);badge.frame.Visible=true
     badge.stroke.Color=color;badge.pointer.BackgroundColor3=color
     badge.pointer.Position=UDim2.new(.5,dx*25,.5,dy*25)
     badge.number.Text=tostring(math.floor(item.distance+.5))
     anyShown=true
    end
   end
  end
  for _,badge in ipairs(badges)do if not used[badge]then badge.frame.Visible=false;badge.owner=nil;badge.x=nil end end
  gui.Enabled=anyShown
 end)
 local previous=featureCleanup
 featureCleanup=function()connection:Disconnect();gui:Destroy();previous()end
end)() end
