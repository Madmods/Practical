local Players = game:GetService("Players")
local Run = game:GetService("RunService")
local Input = game:GetService("UserInputService")
local GuiService = game:GetService("GuiService")
local RS = game:GetService("ReplicatedStorage")
local player = Players.LocalPlayer
local Lab = RS:WaitForChild("VantaQBLab")
local P = require(Lab.Profile)
local Event = Lab.Event
local mouse = player:GetMouse()
local state, visuals, editId, placement, locked = nil, {}, nil, nil, false
local lastSequence, arcData, released = -1, nil, false
local resultUntil,resultText=0,""
local colors = {cyan=Color3.fromRGB(73,225,236),purple=Color3.fromRGB(174,145,255),red=Color3.fromRGB(255,101,110),gold=Color3.fromRGB(255,208,113)}
local visualFolder = Instance.new("Folder"); visualFolder.Name="VantaLabVisuals";visualFolder.Parent=workspace
local function part(name,size,color,parent)
    local p=Instance.new("Part");p.Name=name;p.Size=size;p.Color=color;p.Anchored=true;p.CanCollide=false;p.CanTouch=false;p.CanQuery=false;p.Material=Enum.Material.SmoothPlastic;p.Parent=parent or visualFolder;return p
end
local anchor=part("ArcAnchor",Vector3.one,colors.cyan);anchor.Transparency=1;anchor.Position=Vector3.zero
local arcs={}
for i=1,49 do local a=Instance.new("Attachment");a.Name="Arc"..i;a.Parent=anchor;arcs[i]=a end
local segments={}
for i=1,48 do
    local b=Instance.new("Beam");b.Attachment0=arcs[i];b.Attachment1=arcs[i+1];b.Width0=0.13;b.Width1=0.13;b.FaceCamera=true;b.LightEmission=1;b.Segments=1;b.Transparency=NumberSequence.new(0.12);b.Color=ColorSequence.new(colors.cyan);b.Enabled=false;b.Parent=anchor;segments[i]=b
end
local function ring(name,color,radius)
    local m=Instance.new("Model");m.Name=name;m.Parent=visualFolder
    local parts={};for i=1,48 do parts[i]=part("line",Vector3.new(0.1,0.05,1),color,m);parts[i].Material=Enum.Material.Neon end
    return {model=m,parts=parts,radius=radius}
end
local function drawRing(r,p,visible)
    for i,line in ipairs(r.parts) do
        line.Transparency=visible and 0.12 or 1
        if visible then
            local a=2*math.pi*(i-1)/#r.parts;local b=2*math.pi*i/#r.parts
            local x=p+Vector3.new(math.cos(a)*r.radius,0,math.sin(a)*r.radius)
            local y=p+Vector3.new(math.cos(b)*r.radius,0,math.sin(b)*r.radius)
            line.Size=Vector3.new(0.11,0.07,(y-x).Magnitude);line.CFrame=CFrame.lookAt((x+y)/2,y)
        end
    end
end
local landing=ring("Landing",colors.cyan,2.6)
local catchRing=ring("Catch footprint",colors.gold,1.8)
local selectedRing=ring("Locked WR",colors.purple,3.2)
local threat=part("Defender reach limit",Vector3.new(1.2,1.2,1.2),colors.red);threat.Shape=Enum.PartType.Ball;threat.Material=Enum.Material.Neon;threat.Transparency=1
local football=part("Flying football",Vector3.new(1.2,1.2,2),Color3.fromRGB(127,65,33));football.Transparency=1
local mesh=Instance.new("SpecialMesh");mesh.MeshType=Enum.MeshType.Sphere;mesh.Parent=football
local catchDot=part("Intended catch",Vector3.new(0.5,0.5,0.5),colors.gold);catchDot.Shape=Enum.PartType.Ball;catchDot.Material=Enum.Material.Neon;catchDot.Transparency=1
local function vec(a) return Vector3.new(a[1],a[2],a[3]) end
local function ballPoint(o,v,t,g) return o+v*t-Vector3.new(0,g*t*t/2,0) end
local function renderArc(plan)
    local valid=plan and plan.valid
    for _,b in ipairs(segments) do b.Enabled=valid==true end
    catchDot.Transparency=valid and 0 or 1
    if not valid then drawRing(landing,Vector3.zero,false);drawRing(catchRing,Vector3.zero,false);threat.Transparency=1;return end
    local o,v=vec(plan.origin),vec(plan.velocity)
    for i,a in ipairs(arcs) do a.WorldPosition=ballPoint(o,v,plan.groundTime*(i-1)/48,plan.gravity) end
    local color=plan.intable and colors.red or plan.possible and colors.gold or colors.cyan
    for _,b in ipairs(segments) do b.Color=ColorSequence.new(color) end
    local ground=vec(plan.groundPoint);drawRing(landing,Vector3.new(ground.X,0.08,ground.Z),true)
    local catch=vec(plan.catchPoint);drawRing(catchRing,Vector3.new(catch.X,0.1,catch.Z),true);catchDot.Position=catch
    threat.Transparency=plan.threatIndex>0 and 0.25 or 1;threat.Position=vec(plan.threatPoint)
end
local function robot(b)
    local m=Instance.new("Model");m.Name=b.role.." "..b.id;m.Parent=visualFolder
    local color=b.role=="WR" and colors.cyan or colors.red
    local root=part("HumanoidRootPart",Vector3.new(2,2,1),color,m);root.Transparency=1;m.PrimaryPart=root
    local body=part("Torso",Vector3.new(2,2,1),color,m)
    local head=part("Head",Vector3.new(2,1,1),Color3.fromRGB(220,192,156),m)
    local left=part("Left Arm",Vector3.new(1,2,1),color,m);local right=part("Right Arm",Vector3.new(1,2,1),color,m)
    local ll=part("Left Leg",Vector3.new(1,2,1),Color3.fromRGB(28,38,53),m);local rl=part("Right Leg",Vector3.new(1,2,1),Color3.fromRGB(28,38,53),m)
    local pieces={{root,Vector3.zero},{body,Vector3.new(0,-1.5,0)},{head,Vector3.zero},{left,Vector3.new(-1.5,-1.5,0)},{right,Vector3.new(1.5,-1.5,0)},{ll,Vector3.new(-0.5,-3.5,0)},{rl,Vector3.new(0.5,-3.5,0)}}
    local glow=Instance.new("Highlight");glow.FillTransparency=1;glow.OutlineColor=colors.purple;glow.OutlineTransparency=0;glow.Enabled=false;glow.Parent=m
    local name=Instance.new("BillboardGui");name.Size=UDim2.fromOffset(100,24);name.StudsOffset=Vector3.new(0,2,0);name.AlwaysOnTop=true;name.Parent=head
    local label=Instance.new("TextLabel");label.BackgroundTransparency=1;label.Size=UDim2.fromScale(1,1);label.Font=Enum.Font.GothamBold;label.TextSize=13;label.TextColor3=color;label.Text=b.role.." "..b.id;label.Parent=name
    return {model=m,pieces=pieces,glow=glow,p=b.p}
end
local gui=Instance.new("ScreenGui");gui.Name="VantaLabHUD";gui.ResetOnSpawn=false;gui.Parent=player:WaitForChild("PlayerGui")
local panel=Instance.new("Frame");panel.Name="Test controls";panel.Position=UDim2.fromOffset(18,42);panel.Size=UDim2.fromOffset(250,570);panel.BackgroundColor3=Color3.fromRGB(17,23,33);panel.BackgroundTransparency=0.07;panel.BorderSizePixel=0;panel.Parent=gui
local corner=Instance.new("UICorner");corner.CornerRadius=UDim.new(0,14);corner.Parent=panel
local y=12
local function text(label,height,size)
    local t=Instance.new("TextLabel");t.BackgroundTransparency=1;t.Position=UDim2.fromOffset(14,y);t.Size=UDim2.new(1,-28,0,height);t.TextXAlignment=Enum.TextXAlignment.Left;t.TextColor3=Color3.fromRGB(224,232,241);t.Font=Enum.Font.Gotham;t.TextSize=size or 12;t.TextWrapped=true;t.Text=label;t.Parent=panel;y+=height+6;return t
end
local function button(label,fn)
    local b=Instance.new("TextButton");b.Position=UDim2.fromOffset(12,y);b.Size=UDim2.new(1,-24,0,29);b.BackgroundColor3=Color3.fromRGB(35,46,62);b.TextColor3=Color3.fromRGB(233,240,248);b.Font=Enum.Font.GothamMedium;b.TextSize=12;b.Text=label;b.BorderSizePixel=0;b.Parent=panel
    local c=Instance.new("UICorner");c.CornerRadius=UDim.new(0,6);c.Parent=b;b.Activated:Connect(fn);y+=35;return b
end
text("VANTA / QB LAB",24,17)
local status=text("Waiting for external",26)
local mode=button("Beam | native planner",function() if state then Event:FireServer("settings",{beam=not state.settings.beam,height=state.settings.beam and 0 or 8,lead=state.settings.beam and 0 or 3}) end end)
local spawn=button("+ Defender   |   click field",function() placement="DB" end)
button("+ Receiver   |   click field",function() placement="WR" end)
local scenarios={"Two defenders","Overlap","Missed catch","Crossing"};local si=1
local scenario
scenario=button("Scenario: Two defenders",function() si=si%#scenarios+1;scenario.Text="Scenario: "..scenarios[si];Event:FireServer("scenario",scenarios[si]);locked=false end)
local run=button("Run routes",function() Event:FireServer("run") end)
local routes={"Still","Go","Slant","Comeback","Delayed"};local ri=1
local route=button("Route: Still",function() if state then ri=ri%#routes+1;Event:FireServer("route",{id=editId or state.selected,route=routes[ri]}) end end)
button("Move selected bot   |   click field",function() placement="move" end)
local miss=button("WR misses: off",function() Event:FireServer("miss") end)
local react=button("Bots react: on",function() Event:FireServer("react") end)
local function adjustment(label,key,min,max)
    local row=Instance.new("Frame");row.BackgroundTransparency=1;row.Position=UDim2.fromOffset(12,y);row.Size=UDim2.new(1,-24,0,28);row.Parent=panel
    local t=Instance.new("TextLabel");t.Size=UDim2.new(1,-62,1,0);t.BackgroundTransparency=1;t.Font=Enum.Font.Gotham;t.TextSize=12;t.TextColor3=Color3.fromRGB(220,229,240);t.TextXAlignment=Enum.TextXAlignment.Left;t.Text=label;t.Parent=row
    for i,delta in ipairs({-1,1}) do local b=Instance.new("TextButton");b.Position=UDim2.new(1,-64+(i-1)*32,0,0);b.Size=UDim2.fromOffset(28,26);b.Text=delta<0 and "-" or "+";b.Font=Enum.Font.Gotham;b.TextSize=18;b.TextColor3=Color3.new(1,1,1);b.BackgroundColor3=Color3.fromRGB(45,58,76);b.BorderSizePixel=0;b.Parent=row;b.Activated:Connect(function() if state then Event:FireServer("settings",{[key]=math.clamp(state.settings[key]+delta,min,max)}) end end) end
    y+=34;return t
end
local height=adjustment("Catch height", "height",-8,12)
local lead=adjustment("Extra lead", "lead",-12,12)
button("Reset positions / ball",function() Event:FireServer("reset") end)
text("Q select WR | E select bot | J jump | G dive\nClick throw | Tab hide controls",34,11)
panel.Size=UDim2.fromOffset(250,y+6)
local bottom=Instance.new("TextLabel");bottom.Name="Throw status";bottom.AnchorPoint=Vector2.new(0.5,1);bottom.Position=UDim2.new(0.5,0,1,-25);bottom.Size=UDim2.fromOffset(540,44);bottom.BackgroundColor3=Color3.fromRGB(17,23,33);bottom.BackgroundTransparency=0.1;bottom.BorderSizePixel=0;bottom.Font=Enum.Font.GothamMedium;bottom.TextSize=14;bottom.TextColor3=Color3.fromRGB(235,242,250);bottom.Text="Equip Lab Football | Q to select a receiver";bottom.Parent=gui
local bc=Instance.new("UICorner");bc.CornerRadius=UDim.new(0,12);bc.Parent=bottom
local function nearest(wrOnly)
    if not state then return nil end
    local inset=GuiService:GetGuiInset()
    local cursor=Input:GetMouseLocation()-inset;local best,dist=nil,45
    for _,b in ipairs(state.bots) do if not wrOnly or b.role=="WR" then
        local p,visible=workspace.CurrentCamera:WorldToViewportPoint(b.p)
        local d=(Vector2.new(p.X,p.Y)-cursor).Magnitude
        if visible and d<dist then dist=d;best=b.id end
    end end
    return best
end
Input.InputBegan:Connect(function(input,used)
    if used then return end
    if input.KeyCode==Enum.KeyCode.Q then local id=nearest(true);if id then Event:FireServer("select",id);editId=id;locked=true end
    elseif input.KeyCode==Enum.KeyCode.E then editId=nearest(false)
    elseif input.KeyCode==Enum.KeyCode.J then Event:FireServer("jump")
    elseif input.KeyCode==Enum.KeyCode.G then Event:FireServer("dive")
    elseif input.KeyCode==Enum.KeyCode.Tab then panel.Visible=not panel.Visible
    elseif input.KeyCode==Enum.KeyCode.Escape then placement=nil
    elseif input.UserInputType==Enum.UserInputType.MouseButton1 then
        if placement then
            local ray=workspace.CurrentCamera:ViewportPointToRay(mouse.X,mouse.Y)
            if math.abs(ray.Direction.Y)>0.001 then
                local t=-ray.Origin.Y/ray.Direction.Y
                if t>0 and t<2000 then
                    local p=ray.Origin+ray.Direction*t
                    if placement=="move" then Event:FireServer("move",{id=editId or (state and state.selected),p=p}) else Event:FireServer("add",{role=placement,p=p}) end
                end
            end
            placement=nil
        elseif state and state.held then Event:FireServer("throw") end
    end
end)
Event.OnClientEvent:Connect(function(kind,data)
    if kind=="release" then released=true;arcData=data;renderArc(data)
    elseif kind=="result" then released=false;arcData=nil;renderArc(nil);resultText=data;resultUntil=os.clock()+2.5
    elseif kind=="state" then
        state=data
        local alive={};for _,b in ipairs(data.bots) do alive[b.id]=true;if not visuals[b.id] then visuals[b.id]=robot(b) end end
        for id,v in pairs(visuals) do if not alive[id] then v.model:Destroy();visuals[id]=nil end end
        if not data.ball then released=false end
        if not released then local p=data.held and data.preview or nil;if p and p.sequence~=lastSequence then lastSequence=p.sequence;arcData=p;renderArc(p) elseif not p then arcData=nil;renderArc(nil) end end
        status.Text=data.status;status.TextColor3=data.status=="Vanta connected" and colors.cyan or colors.gold
        mode.Text=(data.settings.beam and "Beam" or "Regular").." | native planner"
        run.Text=data.running and "Pause routes" or "Run routes";miss.Text="WR misses: "..(data.missed and "ON" or "off");react.Text="Bots react: "..(data.react and "on" or "off")
        height.Text="Catch height  "..data.settings.height.." st";lead.Text="Extra lead  "..data.settings.lead.." st"
        for _,b in ipairs(data.bots) do if b.id==(editId or data.selected) then route.Text=b.role.." "..b.id.." | "..b.route end end
        if placement then bottom.Text="Click the field to "..(placement=="move" and "move this bot" or "place a "..placement)
        elseif data.ball then bottom.Text="In flight | "..(data.missed and "WR catch disabled" or "catch test")
        elseif os.clock()<resultUntil then bottom.Text=resultText
        elseif data.preview and data.preview.valid then
            local p=data.preview;bottom.Text=string.format("WR %s   |   %.1f P   /   %.1f deg   |   %s",data.selected,p.power,p.angle,p.intable and "Projected interception" or p.possible and "Inside potential reach" or "Clear estimate")
        elseif data.result~="" then bottom.Text=data.result else bottom.Text=data.preview and data.preview.reason or "Equip Lab Football | Q to select a receiver" end
    end
end)
Run.RenderStepped:Connect(function(dt)
    if not state then return end
    local age=math.clamp(workspace:GetServerTimeNow()-state.at,0,0.12)
    for _,b in ipairs(state.bots) do
        local v=visuals[b.id];if v then
            local target=b.p+b.v*age;v.p=v.p:Lerp(target,1-math.exp(-dt*30))
            local forward=Vector3.new(b.v.X,0,b.v.Z);if forward.Magnitude<0.2 then forward=Vector3.new(0,0,-1) end
            local cf=CFrame.lookAt(v.p,v.p+forward)*(b.dive and CFrame.Angles(math.rad(-55),0,0) or CFrame.identity)
            for _,item in ipairs(v.pieces) do item[1].CFrame=cf*CFrame.new(item[2]) end
            v.glow.Enabled=b.id==state.selected
            if b.id==state.selected then drawRing(selectedRing,Vector3.new(v.p.X,0.12,v.p.Z),true) end
        end
    end
    if state.ball then
        local t=math.max(0,workspace:GetServerTimeNow()-state.ball.at);local p=ballPoint(state.ball.o,state.ball.v,t,P.BallGravity);local velocity=state.ball.v-Vector3.new(0,P.BallGravity*t,0)
        football.Transparency=0;football.CFrame=CFrame.lookAt(p,p+velocity)*CFrame.Angles(0,0,t*13)
    else football.Transparency=1 end
end)
