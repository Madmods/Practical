-- Shared HUD dock: screen cards register here and are stacked in ONE corner so they never overlap.
-- Frames keep their own visibility logic; the dock only decides where they sit.
do (function()
 -- Card style shared with the menu UI (same palette, font, radius, outline and accent line).
 local S={lines={},accent=Color3.fromRGB(72,134,255),accent2=Color3.fromRGB(134,98,255),c={
  bg=Color3.fromRGB(10,12,18),surface=Color3.fromRGB(17,20,29),raised=Color3.fromRGB(23,27,38),
  line=Color3.fromRGB(31,37,51),stroke=Color3.fromRGB(40,47,64),strokeHi=Color3.fromRGB(68,80,106),
  text=Color3.fromRGB(240,242,248),text2=Color3.fromRGB(226,230,240),sub=Color3.fromRGB(178,186,206),
  muted=Color3.fromRGB(160,170,192),green=Color3.fromRGB(104,228,176),amber=Color3.fromRGB(255,200,102),
  danger=Color3.fromRGB(255,125,138)}}
 settings._hudStyle=S
 local FAMILY="rbxasset://fonts/families/BuilderSans.json"
 function S.font(weight)return Font.new(FAMILY,weight or Enum.FontWeight.Regular)end
 local weightFor={[Enum.Font.GothamBold]=Enum.FontWeight.Bold,[Enum.Font.GothamMedium]=Enum.FontWeight.SemiBold}
 function S.fontify(root)
  for _,object in ipairs(root:GetDescendants())do
   if object:IsA("TextLabel")or object:IsA("TextButton")then
    pcall(function()object.FontFace=S.font(weightFor[object.Font]or Enum.FontWeight.Medium)end)
   end
  end
 end
 local EDGE=NumberSequence.new({NumberSequenceKeypoint.new(0,1),NumberSequenceKeypoint.new(.5,0),NumberSequenceKeypoint.new(1,1)})
 function S.skin(frame,radius,hairline)
  frame.BackgroundColor3=S.c.surface;frame.BackgroundTransparency=.05;frame.BorderSizePixel=0
  local corner=frame:FindFirstChildOfClass("UICorner")or Instance.new("UICorner")
  corner.CornerRadius=UDim.new(0,radius or 14);corner.Parent=frame
  local stroke=frame:FindFirstChildOfClass("UIStroke")or Instance.new("UIStroke")
  stroke.Color=S.c.stroke;stroke.Thickness=1;stroke.Transparency=0
  stroke.ApplyStrokeMode=Enum.ApplyStrokeMode.Border;stroke.Parent=frame
  if hairline then
   local line=Instance.new("Frame");line.Name="AccentLine";line.BackgroundColor3=Color3.new(1,1,1)
   line.BorderSizePixel=0;line.Position=UDim2.fromOffset(16,0);line.Size=UDim2.new(1,-32,0,1)
   local gradient=Instance.new("UIGradient");gradient.Color=ColorSequence.new(S.accent,S.accent2)
   gradient.Transparency=EDGE;gradient.Parent=line;line.Parent=frame
   S.lines[#S.lines+1]=gradient
  end
  S.fontify(frame)
  return stroke
 end
 -- The menu calls this whenever you change its accent theme, so cards stay matched.
 function S.setAccent(a,b)
  S.accent,S.accent2=a,b
  for i=#S.lines,1,-1 do
   local gradient=S.lines[i]
   if gradient.Parent then gradient.Color=ColorSequence.new(a,b)else table.remove(S.lines,i)end
  end
 end
 api.SetUIAccent=S.setAccent
 settings.hudCorner="Bottom right"
 local corners={["Bottom right"]=true,["Bottom left"]=true,["Top right"]=true,["Top left"]=true}
 local entries={}
 settings._hudRegister=function(frame,order)
  entries[#entries+1]={frame=frame,order=order,y=nil}
  table.sort(entries,function(a,b)return a.order<b.order end)
 end
 api.SetHudCorner=function(name)
  if not corners[name] then return false,"Unknown HUD corner" end
  settings.hudCorner=name
  for _,entry in ipairs(entries)do entry.y=nil end
  return true
 end
 api.GetHudCorner=function()return settings.hudCorner end
 settings._hudRegister(soloInfo,30)
 local connection=(Run.RenderStepped or Run.Heartbeat):Connect(function(dt)
  local name=settings.hudCorner
  local right=name:find("right",1,true)~=nil
  local bottom=name:find("Bottom",1,true)~=nil
  local margin,base,gap=20,bottom and 130 or 80,10
  local used=0
  local blend=1-math.exp(-math.clamp(dt or 1/60,0,.1)*14)
  for _,entry in ipairs(entries)do
   local frame=entry.frame
   local shown=frame.Parent~=nil and frame.Visible
   if shown then
    local gui=frame:FindFirstAncestorWhichIsA("ScreenGui")
    if gui and not gui.Enabled then shown=false end
   end
   if shown and not entry.skinned then entry.skinned=true;S.skin(frame,14,true)end
   if shown then
    -- Slide only when the stack reflows; a card that just appeared snaps into its slot.
    entry.y=entry.y and entry.y+(used-entry.y)*blend or used
    frame.AnchorPoint=Vector2.new(right and 1 or 0,bottom and 1 or 0)
    local offsetY=bottom and -(base+entry.y) or (base+entry.y)
    frame.Position=UDim2.new(right and 1 or 0,right and -margin or margin,bottom and 1 or 0,offsetY)
    used=used+frame.Size.Y.Offset+gap
   else entry.y=nil end
  end
 end)
 local previous=featureCleanup
 featureCleanup=function()
  connection:Disconnect();settings._hudRegister=nil;settings._hudStyle=nil
  api.SetHudCorner=nil;api.GetHudCorner=nil;api.SetUIAccent=nil
  previous()
 end
end)() end
