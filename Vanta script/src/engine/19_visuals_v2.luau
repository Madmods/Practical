-- Live visual rendering V2: presentation only.
do (function()
local visualModules={}
visualModules.Theme=(function()
return {
 background=Color3.fromRGB(12,15,21), panel=Color3.fromRGB(17,21,29),
 elevated=Color3.fromRGB(24,30,41), hover=Color3.fromRGB(31,39,53),
 border=Color3.fromRGB(42,51,67), text=Color3.fromRGB(228,235,246),
 muted=Color3.fromRGB(137,151,175), dim=Color3.fromRGB(88,103,127),
 accent=Color3.fromRGB(64,132,255), cyan=Color3.fromRGB(94,211,241),
 green=Color3.fromRGB(103,221,175), amber=Color3.fromRGB(244,187,106),
 red=Color3.fromRGB(241,115,133),
 palettes={
  {name="Cobalt",color=Color3.fromRGB(64,132,255)},
  {name="Glacier",color=Color3.fromRGB(94,211,241)},
  {name="Mint",color=Color3.fromRGB(103,221,175)},
  {name="Amber",color=Color3.fromRGB(244,187,106)},
 },
}

end)()
visualModules.Geometry=(function()
-- Rendering primitives for the locally generated WorldModel only.
local G={}
function G.make(class,properties,parent)
 local object=Instance.new(class)
 for key,value in pairs(properties or {})do object[key]=value end
 object.Parent=parent
 return object
end
function G.part(parent,name,size,color,alpha)
 return G.make("Part",{Name=name,Size=size,Color=color,Transparency=alpha or 0,
  Anchored=true,CanCollide=false,CanTouch=false,CanQuery=false,CastShadow=false,
  Material=Enum.Material.Neon},parent)
end
function G.line(parent,name,color)
 return G.part(parent,name,Vector3.new(.06,.06,.1),color,1)
end
function G.placeLine(part,a,b,width,color,alpha)
 local length=(b-a).Magnitude
 part.Transparency=length<.001 and 1 or alpha or 0
 if length<.001 then return end
 part.Size=Vector3.new(width,width,length)
 part.CFrame=CFrame.lookAt((a+b)*.5,b)
 if color then part.Color=color end
end
function G.ring(parent,name,count,color)
 local ring={parts={},count=count}
 for i=1,count do ring.parts[i]=G.line(parent,name.." "..i,color)end
 return ring
end
function G.drawRing(ring,center,radius,width,color,alpha,startAngle,sweep)
 local start=startAngle or 0;local span=sweep or math.pi*2
 for i,part in ipairs(ring.parts)do
  local a=start+span*(i-1)/ring.count;local b=start+span*i/ring.count
  G.placeLine(part,center+Vector3.new(math.cos(a)*radius,0,math.sin(a)*radius),
   center+Vector3.new(math.cos(b)*radius,0,math.sin(b)*radius),width,color,alpha)
 end
end
function G.disk(parent,name,color)
 local disk=G.part(parent,name,Vector3.new(.025,1,1),color,.9)
 disk.Shape=Enum.PartType.Cylinder
 return disk
end
function G.placeDisk(disk,point,radius,color,alpha)
 disk.Size=Vector3.new(.025,radius*2,radius*2)
 disk.CFrame=CFrame.new(point)*CFrame.Angles(0,0,math.pi/2)
 disk.Color=color;disk.Transparency=alpha
end
function G.sphere(parent,name,radius,color,alpha)
 local p=G.part(parent,name,Vector3.new(radius*2,radius*2,radius*2),color,alpha)
 p.Shape=Enum.PartType.Ball;return p
end
-- Upright pillar standing on `base` (used for the landing beacon and the apex drop line).
function G.vertical(part,base,height,width,color,alpha)
 part.Size=Vector3.new(width,math.max(height,.05),width)
 part.CFrame=CFrame.new(base+Vector3.new(0,math.max(height,.05)*.5,0))
 part.Color=color;part.Transparency=alpha
end
function G.dashes(parent,name,count,color)
 local result={}
 for i=1,count do result[i]=G.line(parent,name.." "..i,color)end
 return result
end
function G.drawDashes(parts,a,b,width,color,alpha)
 local delta=b-a
 if G.routeStyle==2 then
  G.placeLine(parts[1],a,b,width,color,alpha)
  for i=2,#parts do parts[i].Transparency=1 end
  return
 elseif G.routeStyle==3 and delta.Magnitude>.01 then
  local unit=delta/delta.Magnitude;local side=Vector3.new(-unit.Z,0,unit.X)
  local count=math.floor(#parts/2);local length=math.clamp(delta.Magnitude/math.max(count,1)*.25,.25,.6)
  for i=1,count do
   local point=a+delta*i/(count+1)
   G.placeLine(parts[i*2-1],point-unit*length+side*length*.6,point,width,color,alpha)
   G.placeLine(parts[i*2],point-unit*length-side*length*.6,point,width,color,alpha)
  end
  if #parts%2==1 then parts[#parts].Transparency=1 end
  return
 end
 for i,p in ipairs(parts)do
  G.placeLine(p,a+delta*((i-.82)/#parts),a+delta*((i-.18)/#parts),width,color,alpha)
 end
end
function G.flat(point,y)return Vector3.new(point.X,y or .07,point.Z)end
function G.quadratic(a,b,c,t)return a*(1-t)^2+b*(2*(1-t)*t)+c*t*t end
function G.drawCurve(parts,a,b,c,width,color,alpha)
 if G.routeStyle==3 then
  local count=math.floor(#parts/2)
  for i=1,count do
   local t=i/(count+1);local point=G.quadratic(a,b,c,t)
   local tangent=G.quadratic(a,b,c,math.min(1,t+.01))-G.quadratic(a,b,c,math.max(0,t-.01))
   local unit=tangent/math.max(.001,tangent.Magnitude);local side=Vector3.new(-unit.Z,0,unit.X)
   G.placeLine(parts[i*2-1],point-unit*.42+side*.23,point,width,color,alpha)
   G.placeLine(parts[i*2],point-unit*.42-side*.23,point,width,color,alpha)
  end
  if #parts%2==1 then parts[#parts].Transparency=1 end
  return
 end
 for i,p in ipairs(parts)do
  local start=G.routeStyle==2 and (i-1)/#parts or (i-.86)/#parts
  local finish=G.routeStyle==2 and i/#parts or (i-.22)/#parts
  G.placeLine(p,G.quadratic(a,b,c,start),G.quadratic(a,b,c,finish),width,color,alpha)
 end
end
function G.outline(parent,name,color)
 return G.dashes(parent,name,12,color)
end
function G.drawOutline(parts,center,size,color,alpha)
 local h=size*.5
 local p={}
 for _,x in ipairs({-1,1})do for _,y in ipairs({-1,1})do for _,z in ipairs({-1,1})do
  p[#p+1]=center+Vector3.new(x*h.X,y*h.Y,z*h.Z)
 end end end
 local index=0
 for i=1,8 do for j=i+1,8 do
  local delta=p[i]-p[j]
  local changes=(math.abs(delta.X)>.001 and 1 or 0)+(math.abs(delta.Y)>.001 and 1 or 0)+(math.abs(delta.Z)>.001 and 1 or 0)
  if changes==1 then index=index+1;G.placeLine(parts[index],p[i],p[j],.035,color,alpha)end
 end end
end
return G

end)()
visualModules.Effect=(function()
local G=visualModules.Geometry
local Effect={}
function Effect.new(context,name)
 local self={folder=G.make("Folder",{Name=name},context.world),labels={},visible=true,context=context,alphas={}}
 function self:Label(value)
  local label={text=value,position=Vector3.zero,color=context.theme.text,visible=true}
  self.labels[#self.labels+1]=label;return label
 end
 function self:SetVisible(value)
  if self.visible==value then return end
  self.visible=value
  -- Keep render objects mounted. Reparenting every frame tears down render proxies.
  for _,part in ipairs(self.folder:GetDescendants())do
   if part:IsA("BasePart")then
    if value then part.Transparency=self.alphas[part]or 1
    else self.alphas[part]=part.Transparency;part.Transparency=1 end
   end
  end
 end
 function self:Destroy()self.folder:Destroy();self.labels={}end
 return self
end
return Effect

end)()
visualModules.LandingZone=(function()
local G=visualModules.Geometry
local Effect=visualModules.Effect
local F={id="LandingZone",title="Landing zone",section="COURT EFFECTS",subtitle="A precise center with a soft, layered perimeter.",color="cyan"}
function F.new(ctx)
 local self=Effect.new(ctx,F.id)
 -- Soft filled disc (three stacked layers), crisp edge, halo, closing timer ring, beacon pillar.
 local fills={}
 for i=1,3 do fills[i]=G.disk(self.folder,"Landing fill "..i,ctx.theme.cyan)end
 local edge=G.ring(self.folder,"Crisp perimeter",48,ctx.theme.cyan)
 local glow=G.ring(self.folder,"Soft outer halo",48,ctx.theme.cyan)
 local pulse=G.ring(self.folder,"Expanding arrival pulse",48,ctx.theme.cyan)
 local closing=G.ring(self.folder,"Closing timer ring",48,ctx.theme.cyan)
 local ticks=G.dashes(self.folder,"Cardinal tick",4,ctx.theme.cyan)
 local beaconCore=G.part(self.folder,"Landing beacon core",Vector3.new(.05,1,.05),ctx.theme.cyan,1)
 local beaconHalo=G.part(self.folder,"Landing beacon halo",Vector3.new(.2,1,.2),ctx.theme.cyan,1)
 local label=self:Label("LANDING ZONE")
 local flightLongest,lastRemaining=0,0
 function self:Update(s,t,o)
  local remaining=math.max(0,(1-s.phase)*ctx.math.duration)
  -- A jump upward in remaining time means a new flight; restart the timer ring.
  if flightLongest<=0 or remaining>lastRemaining+.25 then flightLongest=remaining else flightLongest=math.max(flightLongest,remaining)end
  lastRemaining=remaining
  local progress=flightLongest>.05 and math.clamp(1-remaining/flightLongest,0,1) or 1
  local urgency=o.urgency and math.clamp(1-remaining/1.1,0,1) or 0
  local color=o.urgency and o.color:Lerp(ctx.theme.amber,urgency) or o.color
  local center=G.flat(s.landing,.06);local r=1.55*o.scale;local a=1-o.intensity
  local radii={.35,.65,.92}
  for i,disk in ipairs(fills)do
   G.placeDisk(disk,center+Vector3.new(0,(i-1)*.004,0),r*radii[i],color,math.clamp(.9-urgency*.1+(i-1)*.025+a*.05,0,1))
  end
  G.drawRing(edge,center+Vector3.new(0,.025,0),r,.075*o.weight,color,.16+a*.65)
  G.drawRing(glow,center+Vector3.new(0,.012,0),r+.2,.21*o.weight,color,.88+a*.1)
  local cycle=(t*.5*o.motion)%1
  G.drawRing(pulse,center+Vector3.new(0,.035,0),r+.35+cycle*.6,.055*o.weight,color,o.motion<=0 and 1 or math.clamp(.8+cycle*.2+a*.15,0,1))
  -- Timer ring: starts wide and tightens onto the zone as the ball arrives.
  if o.urgency and progress<.985 then
   G.drawRing(closing,center+Vector3.new(0,.045,0),r+(1-progress)*r*1.6,.05*o.weight,color,.3+a*.4)
  else G.drawRing(closing,center,r,.05,color,1)end
  for i,part in ipairs(ticks)do
   local angle=(i-1)*math.pi*.5;local unit=Vector3.new(math.cos(angle),0,math.sin(angle))
   G.placeLine(part,center+unit*(r+.32),center+unit*(r+.61),.065*o.weight,color,.12+a*.6)
  end
  if o.beacon then
   G.vertical(beaconCore,center,6*o.scale,.05*o.weight,color,.35+a*.4)
   G.vertical(beaconHalo,center,6*o.scale,.2*o.weight,color,.86+a*.1)
  else beaconCore.Transparency=1;beaconHalo.Transparency=1 end
  label.position=center+Vector3.new(0,.15,2.4);label.color=color
  label.text=string.format("LANDING  /  %.1fs",remaining)
 end
 return self
end
return F

end)()
visualModules.BallFlight=(function()
local G=visualModules.Geometry
local Effect=visualModules.Effect
local F={id="BallFlight",title="Flight trail",section="COURT EFFECTS",subtitle="A tapered arc, fading tail and restrained ball halo.",color="cyan"}
function F.new(ctx)
 local self=Effect.new(ctx,F.id)
 local arc=G.dashes(self.folder,"Future trajectory",48,ctx.theme.cyan)
 local shadow=G.dashes(self.folder,"Floor shadow",24,ctx.theme.cyan)
 local drop=G.part(self.folder,"Apex drop line",Vector3.new(.03,1,.03),ctx.theme.cyan,1)
 local apex=G.sphere(self.folder,"Apex marker",.17,ctx.theme.cyan,1)
 local tail=G.dashes(self.folder,"Motion tail",16,ctx.theme.cyan)
 local halo=G.sphere(self.folder,"Ball halo",.59,ctx.theme.cyan,.82)
 function self:Update(s,t,o)
  -- Predicted path: tapers and strengthens toward the landing point.
  for i,part in ipairs(arc)do
   local a=(i-1)/#arc;local b=i/#arc
   G.placeLine(part,ctx.math.position(a),ctx.math.position(b),.035*(1.3-a*.6)*o.weight,o.color,math.clamp(.62-a*.3+(1-o.intensity)*.2,0,1))
  end
  -- Floor shadow of the path and the highest point, for depth perception.
  local hideApex=true
  if o.flightShadow then
   for i,part in ipairs(shadow)do
    local a=(i-1)/#shadow;local b=i/#shadow
    G.placeLine(part,G.flat(ctx.math.position(a),.02),G.flat(ctx.math.position(b),.02),.04*o.weight,o.color,.72+(1-o.intensity)*.2)
   end
   local bestY,bestIndex,bestPoint=-math.huge,0,nil
   for i=0,24 do
    local p=ctx.math.position(i/24)
    if p.Y>bestY then bestY=p.Y;bestIndex=i;bestPoint=p end
   end
   if bestPoint and bestIndex>0 and bestIndex<24 then
    local ground=G.flat(bestPoint,.02)
    local height=bestPoint.Y-ground.Y
    if height>.6 then
     hideApex=false
     G.vertical(drop,ground,height,.025*o.weight,o.color,.8)
     apex.Position=bestPoint;apex.Size=Vector3.one*(.34*o.scale);apex.Color=o.color;apex.Transparency=.45
    end
   end
  else for _,part in ipairs(shadow)do part.Transparency=1 end end
  if hideApex then drop.Transparency=1;apex.Transparency=1 end
  -- Motion trail from the ball's recent real positions (oldest fades out).
  local trail=s.trail
  for i,part in ipairs(tail)do
   local newer=o.comet and trail and trail[#trail-(i-1)]
   local older=o.comet and trail and trail[#trail-i]
   if newer and older then
    local fade=i/#tail
    G.placeLine(part,older,newer,.11*(1-fade*.75)*o.weight,o.color,math.clamp(.25+fade*.65+(1-o.intensity)*.3,0,1))
   else part.Transparency=1 end
  end
  halo.Position=s.ball;halo.Size=Vector3.one*(1.18*o.scale);halo.Color=o.color;halo.Transparency=.86+(1-o.intensity)*.12
 end
 return self
end
return F

end)()
visualModules.GuardRoute=(function()
local G=visualModules.Geometry
local Effect=visualModules.Effect
local F={id="GuardRoute",title="Positioning route",section="ROUTES",subtitle="A curved ground route with a clean destination reticle.",color="accent"}
function F.new(ctx)
 local self=Effect.new(ctx,F.id)
 local route=G.dashes(self.folder,"Curved route",22,ctx.theme.accent)
 local endRing=G.ring(self.folder,"Destination reticle",40,ctx.theme.accent)
 local assignment=G.dashes(self.folder,"Assignment guide",7,ctx.theme.accent)
 local arrow=G.dashes(self.folder,"Route arrowhead",2,ctx.theme.accent)
 local label=self:Label("DESTINATION")
 function self:Update(s,t,o)
  local a=G.flat(s.passer);local b=G.flat(s.guardPoint);local control=(a+b)*.5+Vector3.new(-3,0,0)
  G.drawCurve(route,a,control,b,.065*o.weight,o.color,.25+(1-o.intensity)*.55)
  -- Arrowhead on the reticle edge, pointing along the end of the route.
  local heading=Vector3.new(b.X-control.X,0,b.Z-control.Z)
  if heading.Magnitude>.2 then
   local u=heading.Unit;local side=Vector3.new(-u.Z,0,u.X)
   local tip=b-u*(1.05*o.scale);local wing=.55*o.scale
   local arrowAlpha=.2+(1-o.intensity)*.5
   G.placeLine(arrow[1],tip-u*wing+side*wing*.7,tip,.06*o.weight,o.color,arrowAlpha)
   G.placeLine(arrow[2],tip-u*wing-side*wing*.7,tip,.06*o.weight,o.color,arrowAlpha)
  else arrow[1].Transparency=1;arrow[2].Transparency=1 end
  G.drawRing(endRing,b,1*o.scale,.06*o.weight,o.color,.1+(1-o.intensity)*.65,t*.12*o.motion,math.pi*1.7)
  G.drawDashes(assignment,b,G.flat(s.handler),.032*o.weight,o.color,.62)
  label.position=b+Vector3.new(0,.2,1.7);label.color=o.color
 end
 return self
end
return F

end)()
visualModules.Contest=(function()
local G=visualModules.Geometry
local Effect=visualModules.Effect
local F={id="Contest",title="Coverage display",section="PLAYER MARKERS",subtitle="A floor arc and compact percentage, without overhead clutter.",color="accent"}
function F.new(ctx)
 local self=Effect.new(ctx,F.id)
 local base=G.ring(self.folder,"Coverage track",48,ctx.theme.dim)
 local value=G.ring(self.folder,"Coverage amount",32,ctx.theme.accent)
 local outline=G.outline(self.folder,"Player contour",ctx.theme.accent)
 local label=self:Label("32%  COVERAGE")
 function self:Update(s,t,o)
  local point=G.flat(s.handler);local ratio=s.contest/100;local r=1.55*o.scale
  G.drawRing(base,point,r,.035*o.weight,ctx.theme.dim,.65)
  G.drawRing(value,point+Vector3.new(0,.015,0),r,.085*o.weight,o.color,.13+(1-o.intensity)*.65,math.pi*.25,math.max(.03,ratio*math.pi*2))
  G.drawOutline(outline,s.handler+Vector3.new(0,2.5,0),Vector3.new(2.7,5.1,1.5),o.color,.65+(1-o.intensity)*.3)
  label.text=string.format("%d%%  COVERAGE",s.contest);label.position=point+Vector3.new(0,.15,2.4);label.color=o.color
 end
 return self
end
return F

end)()
visualModules.BallRecovery=(function()
local G=visualModules.Geometry
local Effect=visualModules.Effect
local F={id="BallRecovery",title="Recovery marker",section="COURT EFFECTS",subtitle="A loose-ball bracket, soft radius and ground-level approach line.",color="amber"}
function F.new(ctx)
 local self=Effect.new(ctx,F.id)
 local inner=G.ring(self.folder,"Ball bracket",32,ctx.theme.amber)
 local outer=G.ring(self.folder,"Recovery perimeter",48,ctx.theme.amber)
 local route=G.dashes(self.folder,"Recovery route",16,ctx.theme.amber)
 local label=self:Label("LOOSE BALL")
 function self:Update(s,t,o)
  local center=G.flat(s.loose)
  G.drawRing(inner,center,.75*o.scale,.065*o.weight,o.color,.15+(1-o.intensity)*.65,t*.25*o.motion,math.pi*1.6)
  G.drawRing(outer,center,2.1*o.scale,.035*o.weight,o.color,.72+(1-o.intensity)*.22)
  G.drawDashes(route,G.flat(s.passer),center,.04*o.weight,o.color,.55+(1-o.intensity)*.35)
  label.position=center+Vector3.new(0,.15,2.7);label.color=o.color
 end
 return self
end
return F

end)()
visualModules.ScreenAlert=(function()
local G=visualModules.Geometry
local Effect=visualModules.Effect
local F={id="ScreenAlert",title="Screen warning",section="PLAYER MARKERS",subtitle="An amber danger sector and a quieter bypass guide.",color="amber"}
function F.new(ctx)
 local self=Effect.new(ctx,F.id)
 local zone=G.ring(self.folder,"Warning sector",40,ctx.theme.amber)
 local outline=G.outline(self.folder,"Screener contour",ctx.theme.amber)
 local path=G.dashes(self.folder,"Bypass path",20,ctx.theme.amber)
 local label=self:Label("SCREEN  /  LEFT SIDE")
 function self:Update(s,t,o)
  local center=G.flat(s.screen)
  G.drawRing(zone,center,2.5*o.scale,.07*o.weight,o.color,.3+(1-o.intensity)*.5,0,math.pi*1.25)
  G.drawOutline(outline,s.screen+Vector3.new(0,2.5,0),Vector3.new(2.7,5.1,1.5),o.color,.65+(1-o.intensity)*.3)
  local a=G.flat(s.passer);local b=center+Vector3.new(-4,0,0);local c=G.flat(s.guardPoint)
  G.drawCurve(path,a,b,c,.035*o.weight,o.color,.6)
  label.position=center+Vector3.new(0,.2,3.2);label.color=o.color
 end
 return self
end
return F

end)()
visualModules.LobIntercept=(function()
local G=visualModules.Geometry
local Effect=visualModules.Effect
local F={id="LobIntercept",title="Meeting-point marker",section="ROUTES",subtitle="A split reticle and a precise crosshair at the fictional meeting point.",color="cyan"}
function F.new(ctx)
 local self=Effect.new(ctx,F.id)
 local arcs={};for i=1,4 do arcs[i]=G.ring(self.folder,"Meeting reticle "..i,12,ctx.theme.cyan)end
 local cross=G.dashes(self.folder,"Crosshair",2,ctx.theme.cyan)
 local route=G.dashes(self.folder,"Meeting route",19,ctx.theme.cyan)
 local label=self:Label("MEETING POINT")
 function self:Update(s,t,o)
  local p=G.flat(s.landing,.13);local r=.95*o.scale
  for i,arc in ipairs(arcs)do G.drawRing(arc,p,r,.065*o.weight,o.color,.15+(1-o.intensity)*.6,(i-1)*math.pi*.5+t*.08*o.motion,math.pi*.34)end
  G.placeLine(cross[1],p+Vector3.new(-.22,0,0),p+Vector3.new(.22,0,0),.035*o.weight,o.color,.25)
  G.placeLine(cross[2],p+Vector3.new(0,0,-.22),p+Vector3.new(0,0,.22),.035*o.weight,o.color,.25)
  G.drawDashes(route,G.flat(s.passer),p,.035*o.weight,o.color,.55+(1-o.intensity)*.35)
  label.position=p+Vector3.new(0,.15,-2);label.color=o.color
 end
 return self
end
return F

end)()
-- Presentation adapter: consumes existing state; sends no gameplay input.
 local function installLiveVisuals()
  local folder=visualModules.Geometry.make("Folder",{Name="PracticalVisualStyleV2"},visualFolder)
  local G=visualModules.Geometry
  local originalFlat=G.flat
  local originalLine=G.placeLine
  G.placeLine=function(part,a,b,width,color,alpha)
   local camera=workspace.CurrentCamera
   if camera and camera.CFrame then
    local distance=(camera.CFrame.Position-(a+b)*.5).Magnitude
    width=math.max(width,math.clamp(distance*.0018,.045,.14)*(settings.visualWeight or 1))
   end
   originalLine(part,a,b,width,color,alpha)
  end
  local floorY;local floorAt=-math.huge
  G.flat=function(point,y)
   local height=floorY or floorPoint(point,model()).Y
   return Vector3.new(point.X,height+(settings.visualClearance or .12)+(y or .02),point.Z)
  end
  -- Live avatar Highlights already provide exact silhouettes; preview boxes stay hidden.
  G.drawOutline=function()end
  local theme=visualModules.Theme
  local flight
  local context={world=folder,theme=theme,math={duration=0}}
  context.math.position=function(phase)
   if not flight then return Vector3.zero end
   local t=math.clamp(phase,0,1)*flight.time
   return flight.position+flight.velocity*t-Vector3.new(0,.5*(flight.gravity or workspace.Gravity)*t*t,0)
  end
  local names={"LandingZone","BallFlight","GuardRoute","Contest","BallRecovery","ScreenAlert","LobIntercept"}
  local effects={}
  for _,name in ipairs(names)do effects[name]=visualModules[name].new(context);effects[name]:SetVisible(false)end
  local labelsGui=create("ScreenGui",{Name="PracticalVisualLabelsV2",ResetOnSpawn=false},P.PlayerGui)
  local labelObjects={}
  for name,effect in pairs(effects)do
   labelObjects[name]={}
   for i in ipairs(effect.labels)do
    local board=create("BillboardGui",{Name=name.."FloorLabel",Size=UDim2.fromOffset(170,25),AlwaysOnTop=true,Enabled=false,MaxDistance=70},labelsGui)
    local text=create("TextLabel",{Size=UDim2.fromScale(1,1),BackgroundColor3=theme.background,BackgroundTransparency=.16,BorderSizePixel=0,Font=Enum.Font.GothamMedium,TextSize=10,Text=""},board)
    create("UICorner",{CornerRadius=UDim.new(0,5)},text)
    local stroke=create("UIStroke",{Color=theme.border,Transparency=.4},text)
    labelObjects[name][i]={board=board,text=text,stroke=stroke}
   end
  end
  local lanes={}
  for i=1,10 do lanes[i]=G.dashes(folder,"Passing lane "..i,12,theme.green)end
  local stopped=false;local failure;local last=0;local cache={};local dt=1/60
  settings.visualPolish=true;settings.visualLabels=true;settings.visualMotion=false
  settings.visualScale=1;settings.visualIntensity=65;settings.visualWeight=1.15
  settings.visualSmoothing=55;settings.visualClearance=.12;settings.visualPalette=1
  settings.visualRouteStyle=1;settings.visualCustomColor=false
  settings.visualRed=64;settings.visualGreen=132;settings.visualBlue=255
  settings.visualLabelSize=11;settings.visualLabelOpacity=80;settings.visualPassPanel=true
  settings.visualUrgency=true;settings.visualBeacon=true;settings.visualFlightShadow=true;settings.visualComet=true
  local trail,trailAt={},-math.huge
  -- Smooth blue -> amber -> red ramp instead of two hard colour steps.
  local function contestColor(p)
   p=math.clamp(p,0,100)
   if p<=35 then return theme.accent:Lerp(theme.amber,p/35)end
   return theme.amber:Lerp(theme.red,math.clamp((p-35)/30,0,1))
  end
  for _,key in ipairs({"Landing","Flight","Route","Coverage","Pass","Recovery","Screen","Intercept"})do settings["visualShow"..key]=true end
  numericLimits.visualScale={.6,1.6};numericLimits.visualIntensity={10,100};numericLimits.visualWeight={.5,2}
  numericLimits.visualSmoothing={0,100};numericLimits.visualClearance={.04,.35};numericLimits.visualPalette={1,4}
  numericLimits.visualRouteStyle={1,3};numericLimits.visualRed={0,255};numericLimits.visualGreen={0,255};numericLimits.visualBlue={0,255}
  numericLimits.visualLabelSize={9,16};numericLimits.visualLabelOpacity={0,95}
  local function smooth(key,value)
   local previous=cache[key]
   local amount=settings.visualSmoothing or 55
   local alpha=amount<=0 and 1 or 1-math.exp(-60/(1+amount*.04)*dt)
   local result=previous and (value-previous).Magnitude<15 and previous:Lerp(value,alpha)or value
   cache[key]=result;return result
  end
  local function hide()
   for name,effect in pairs(effects)do
    effect:SetVisible(false)
    for _,object in ipairs(labelObjects[name])do object.board.Enabled=false end
   end
   for _,path in ipairs(lanes)do for _,part in ipairs(path)do part.Transparency=1 end end
  end
  local function show(name,visible,state,now,options,root)
   local effect=effects[name];effect:SetVisible(visible==true)
   if not visible then for _,object in ipairs(labelObjects[name])do if object.board.Enabled then object.board.Enabled=false end end;return end
   effect:Update(state,now,options)
   if name=="GuardRoute"and effect.labels[1]then
    effect.labels[1].text=settings.denyReceiver and settings._denyReceiverTarget and ("DENY / "..settings._denyReceiverTarget.Name)or "DESTINATION"
   end
   for i,label in ipairs(effect.labels)do
    local object=labelObjects[name][i]
    -- Keep the contest visual at the feet; no text over the local player's head.
    -- Floor label cards were removed (landing time, DESTINATION, LOOSE BALL, ...).
    local enabled=false
    if object.board.Enabled~=(enabled==true)then object.board.Enabled=enabled==true end
    if enabled then
     object.board.Adornee=root
     object.board.StudsOffsetWorldSpace=label.position-root.Position
     object.text.Text=label.text;object.text.TextColor3=label.color
     object.text.TextSize=settings.visualLabelSize or 11
     -- Pill hugs its text and its outline follows the effect colour.
     local size=settings.visualLabelSize or 11
     object.board.Size=UDim2.fromOffset(math.clamp(#label.text*size*.62+24,80,280),size+14)
     object.stroke.Color=label.color;object.stroke.Transparency=.5
     object.text.BackgroundTransparency=1-(settings.visualLabelOpacity or 80)/100
    end
   end
  end
  local function draw(now)
   settings._visualV2=settings.visualPolish and not failure
   if not settings._visualV2 then hide();return end
   local own=model();local root=rootOf(own)
   if not root then hide();cache={};floorY=nil;return end
   if now-floorAt>.12 then
    local ground=floorPoint(root.Position,own)
    -- Exclude balls and avatars so a held/flying ball cannot become the floor.
    pcall(function()
     local params=RaycastParams.new();params.FilterType=Enum.RaycastFilterType.Exclude
     local excluded={visualFolder,own}
     for _,name in ipairs({"Characters","ProxyCharacter","Basketballs","PracticalCourtTools"})do local object=workspace:FindFirstChild(name);if object then excluded[#excluded+1]=object end end
     params.FilterDescendantsInstances=excluded
     params.RespectCanCollide=true
     local lowest
     for _,offset in ipairs({Vector3.zero,Vector3.new(2,0,0),Vector3.new(-2,0,0),Vector3.new(0,0,2),Vector3.new(0,0,-2)})do
      local hit=workspace:Raycast(root.Position+offset+Vector3.new(0,2,0),Vector3.new(0,-32,0),params)
      if hit and (not lowest or hit.Position.Y<lowest.Y)then lowest=hit.Position end
     end
     if lowest then ground=lowest+Vector3.new(0,.06,0)end
    end)
    if not floorY or math.abs(ground.Y-floorY)>.5 then floorY=ground.Y end
    floorAt=now
   end
   local predicted=landingPrediction
   if predicted then cache.prediction=table.clone(predicted);cache.predictedAt=now
   elseif settings.landing and cache.predictedAt and now-cache.predictedAt<.12 then predicted=cache.prediction end
   local actualBall=goalPosition(trackedBall)or ballPosition
   local palette=theme.palettes[math.clamp(math.floor(settings.visualPalette or 1),1,4)].color
   if settings.visualCustomColor then palette=Color3.fromRGB(settings.visualRed,settings.visualGreen,settings.visualBlue)end
   G.routeStyle=math.clamp(math.floor(settings.visualRouteStyle or 1),1,3)
   local state={phase=0,ball=actualBall or root.Position,landing=predicted and smooth("landing",predicted.position)or root.Position,
    passer=G.flat(root.Position),handler=root.Position,guardPoint=guardAim and smooth("guardPoint",guardAim)or root.Position,
    screen=root.Position,loose=root.Position,receivers={},contest=settings.hudPressure or 0}
   local options={scale=settings.visualScale,intensity=settings.visualIntensity/100,weight=settings.visualWeight,
    motion=settings.visualMotion and 1 or 0,color=palette,urgency=settings.visualUrgency~=false,beacon=settings.visualBeacon~=false,
    flightShadow=settings.visualFlightShadow~=false,comet=settings.visualComet~=false}
   flight=predicted and actualBall and ballVelocity and {position=actualBall,velocity=smooth("velocity",ballVelocity),time=predicted.time}or nil
   context.math.duration=flight and flight.time or 0
   -- Recent real ball positions for the motion trail; cleared when the ball is gone or jumps.
   if flight and actualBall then
    local newest=trail[#trail]
    if newest and (actualBall-newest).Magnitude>12 then trail={};newest=nil end
    if not newest or (actualBall-newest).Magnitude>.12 then
     trail[#trail+1]=actualBall
     if #trail>17 then table.remove(trail,1)end
    end
    trailAt=now
   elseif now-trailAt>.25 and #trail>0 then trail={} end
   state.trail=trail
   local lob=api.GetAntiOopState and api.GetAntiOopState()
   if lob and lob.target then cache.lob=table.clone(lob);cache.lobAt=now
   elseif settings.antiOop and cache.lobAt and now-cache.lobAt<.1 then lob=cache.lob end
   local lobActive=settings.antiOop and lob and lob.target~=nil
   if lobActive and lob.flight then
    flight=lob.flight;state.ball=lob.ballPosition or flight.position;context.math.duration=flight.time
   end
   options.scale=(settings.landingZoneScale or 1)*settings.visualScale
   show("LandingZone",settings.visualShowLanding and settings.landing and settings.landingZone and predicted~=nil and not lobActive,state,now,options,root)
   options.scale=settings.visualScale;options.weight=settings.visualWeight*(settings.landingBeamWidth or .065)/.065
   local flightVisible=not lobActive and settings.landing and settings.landingBeam or lobActive and settings.oopGuide
   show("BallFlight",settings.visualShowFlight and flightVisible and flight~=nil,state,now,options,root)
   options.weight=settings.visualWeight;options.color=guardClosing and theme.amber or palette
   local otherRoot=rootOf(settings.denyReceiver and settings._denyReceiverTarget or target)
   if otherRoot then state.handler=floorPoint(otherRoot.Position,own)end
   local routeVisible=settings.visuals and guardEngaged and guardAim~=nil and not lobActive
   show("GuardRoute",settings.visualShowRoute and routeVisible,state,now,options,root)
   local pressure=settings.hudPressure
   state.handler=floorPoint(root.Position,own);state.contest=pressure or 0
   options.color=pressure and contestColor(pressure) or theme.accent
   show("Contest",settings.visualShowCoverage and settings.liveContest and pressure~=nil and now-(settings.hudPressureAt or 0)<.3,state,now,options,root)
   local characters=defenseCharacters();local byName={};local occupied={}
   for _,character in ipairs(characters)do
    byName[character.Name]=character
    local held=attr(character,"Basketball");if held then occupied[held]=true end
   end
   local tools=api.GetCourtToolsState and api.GetCourtToolsState()
   local screen=tools and tools.screen
   local screenRoot=screen and rootOf(byName[screen.player])
   options.color=theme.amber
   if screenRoot then state.screen=floorPoint(screenRoot.Position,own)end
   show("ScreenAlert",settings.visualShowScreen and settings.screenAwareness and screenRoot~=nil,state,now,options,root)
   local recoveryBall;local nearest=settings.recoveryRange or 12
   if settings.ballRecovery and not holdsBall(own)then
    local balls=workspace:FindFirstChild("Basketballs")
    if balls then for _,ball in ipairs(balls:GetChildren())do
     local position=goalPosition(ball)
     if position and not occupied[ball]and math.abs(position.Y-root.Position.Y)<6 then
      local distance=flat(position-root.Position).Magnitude
      if distance<nearest then recoveryBall=position;nearest=distance end
     end
    end end
   end
   if recoveryBall then state.loose=smooth("loose",recoveryBall)end
   show("BallRecovery",settings.visualShowRecovery and recoveryBall~=nil,state,now,options,root)
   if lobActive then state.landing=smooth("interception",lob.target)end
   options.color=palette
   show("LobIntercept",settings.visualShowIntercept and lobActive and settings.oopGuide,state,now,options,root)
   local activeLanes={}
   if settings.visualShowPass and settings.passingLanes and holdsBall(own)and tools then
    for i,lane in ipairs(tools.lanes or {})do
     local receiver=rootOf(byName[lane.player])
     if lanes[i]and receiver then
      activeLanes[i]=true
      G.drawDashes(lanes[i],G.flat(root.Position),G.flat(receiver.Position),.045*settings.visualWeight,lane.blocked and theme.amber or theme.green,.45+(1-options.intensity)*.4)
     end
    end
   end
   for i,path in ipairs(lanes)do if not activeLanes[i]then for _,part in ipairs(path)do if part.Transparency~=1 then part.Transparency=1 end end end end
  end
  local connection=(Run.RenderStepped or Run.Heartbeat):Connect(function()
   if stopped or failure then return end
   local now=os.clock();dt=math.clamp(now-last,.001,.05);last=now
   local ok,message=pcall(draw,now)
   if not ok then failure=tostring(message);settings._visualV2=false;hide();warn("Practical visual V2 fallback: "..failure)end
  end)
  settings._visualV2=true
  api.GetVisualGroundPoint=function(point)return G.flat(point)end
  api.DrawVisualRoute=function(parts,a,b,width,color,alpha)G.drawDashes(parts,a,b,width,color,alpha)end
  api.GetVisualStyleState=function()
   local active={};for name,effect in pairs(effects)do active[name]=effect.visible end
   return {enabled=settings._visualV2==true,error=failure,version=3,features=active}
  end
  local previous=featureCleanup
  featureCleanup=function()
   stopped=true;connection:Disconnect();settings._visualV2=false
   for _,effect in pairs(effects)do effect:Destroy()end
   labelsGui:Destroy();folder:Destroy();api.GetVisualGroundPoint=nil;api.DrawVisualRoute=nil;G.flat=originalFlat;G.placeLine=originalLine;previous()
  end
 end
 local visualOK,visualError=pcall(installLiveVisuals)
 if not visualOK then settings._visualV2=false;warn("Practical visual V2 setup failed: "..tostring(visualError))end

end)() end

