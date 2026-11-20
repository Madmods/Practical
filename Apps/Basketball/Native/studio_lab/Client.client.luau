local Players=game:GetService("Players")
local Run=game:GetService("RunService")
if not Run:IsStudio() then return end
local Http=game:GetService("HttpService")
local Input=game:GetService("UserInputService")
local RS=game:GetService("ReplicatedStorage")
local player=Players.LocalPlayer
local mouse=player:GetMouse()
local Lab=RS:WaitForChild("VantaQBLab")
local P=require(Lab.Profile)
local Connection=require(Lab:WaitForChild("Connection"))
local Event=Lab.Event
local state,visuals=nil,{}
local colors={cyan=Color3.fromRGB(73,225,236),purple=Color3.fromRGB(174,145,255),red=Color3.fromRGB(255,101,110)}
local visualFolder=Instance.new("Folder");visualFolder.Name="VantaLabVisuals";visualFolder.Parent=workspace
local function part(name,size,color,parent)
 local p=Instance.new("Part");p.Name=name;p.Size=size;p.Color=color;p.Anchored=true;p.CanCollide=false;p.CanTouch=false;p.CanQuery=false;p.Material=Enum.Material.SmoothPlastic;p.Parent=parent or visualFolder;return p
end
local football=part("Flying football",Vector3.new(1.2,1.2,2),Color3.fromRGB(127,65,33));football.Transparency=1
local mesh=Instance.new("SpecialMesh");mesh.MeshType=Enum.MeshType.Sphere;mesh.Parent=football
local function ballPoint(o,v,t,g) return o+v*t-Vector3.new(0,g*t*t/2,0) end
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

-- All QB controls, selection, settings and overlays belong to the Vanta app.
-- These keys only operate the test world's bots.
local scenarios={"Two defenders","Overlap","Missed catch","Crossing"};local scenario=1
Input.InputBegan:Connect(function(input,used)
 if used then return end
 if input.UserInputType==Enum.UserInputType.MouseButton1 then
  if state and state.held then Event:FireServer("throw",mouse.Hit.Position) end
 elseif input.KeyCode==Enum.KeyCode.F6 then scenario=scenario%#scenarios+1;Event:FireServer("scenario",scenarios[scenario])
 elseif input.KeyCode==Enum.KeyCode.F7 then Event:FireServer("run")
 elseif input.KeyCode==Enum.KeyCode.F8 or input.KeyCode==Enum.KeyCode.F9 then Event:FireServer("add",{role=input.KeyCode==Enum.KeyCode.F8 and "DB" or "WR",p=mouse.Hit.Position})
 elseif input.KeyCode==Enum.KeyCode.F10 then Event:FireServer("reset")
 elseif input.KeyCode==Enum.KeyCode.J then Event:FireServer("jump")
 elseif input.KeyCode==Enum.KeyCode.G then Event:FireServer("dive")
 elseif input.KeyCode==Enum.KeyCode.M then Event:FireServer("miss") end
end)
Event.OnClientEvent:Connect(function(kind,data)
 if kind~="state" then return end
 state=data
 local alive={};for _,b in ipairs(data.bots) do alive[b.id]=true;if not visuals[b.id] then visuals[b.id]=robot(b) end end
 for id,v in pairs(visuals) do if not alive[id] then v.model:Destroy();visuals[id]=nil end end
end)
local socket,connected=nil,false
local function A(v) return {v.X,v.Y,v.Z} end
local function connect()
 if socket then pcall(function() socket:Close() end) end
 local ok,ws=pcall(function() return Http:CreateWebStreamClient(Enum.WebStreamClientType.WebSocket,{
  Url="ws://127.0.0.1:"..Connection.port.."/studio/view",Headers={["X-Vanta-Lab"]=Connection.token}}) end)
 if not ok then script:SetAttribute("LastBridgeError",tostring(ws));return end
 socket=ws
 ws.Opened:Connect(function() if socket==ws then connected=true;script:SetAttribute("ViewConnected",true);script:SetAttribute("LastBridgeError",nil) end end)
 ws.Closed:Connect(function() if socket==ws then connected=false;script:SetAttribute("ViewConnected",false) end end)
 ws.Error:Connect(function(code,message) if socket==ws then connected=false;script:SetAttribute("LastBridgeError",tostring(code).." "..tostring(message)) end end)
end
task.spawn(function() while script.Parent do if not connected then connect() end;task.wait(2) end end)
local frames=0
Run.RenderStepped:Connect(function(dt)
 local camera=workspace.CurrentCamera
 if connected and camera then
  local cf=camera.CFrame;local size=camera.ViewportSize
  local data={protocol=2,kind="camera",session=Lab:GetAttribute("Session"),position=A(cf.Position),right=A(cf.RightVector),up=A(cf.UpVector),look=A(cf.LookVector),
   fov=camera.FieldOfView,viewport={size.X,size.Y},textFocus=Input:GetFocusedTextBox()~=nil}
  local ok=pcall(function() socket:Send(Http:JSONEncode(data)) end);if not ok then connected=false end
  frames+=1;if frames%60==0 then script:SetAttribute("CameraFrames",frames) end
 end
 if not state then return end
 local age=math.clamp(workspace:GetServerTimeNow()-state.at,0,0.12)
 for _,b in ipairs(state.bots) do
  local v=visuals[b.id];if v then
   local target=b.p+b.v*age;v.p=v.p:Lerp(target,1-math.exp(-dt*30))
   local forward=Vector3.new(b.v.X,0,b.v.Z);if forward.Magnitude<0.2 then forward=Vector3.new(0,0,-1) end
   local cf=CFrame.lookAt(v.p,v.p+forward)*(b.dive and CFrame.Angles(math.rad(-55),0,0) or CFrame.identity)
   for _,item in ipairs(v.pieces) do item[1].CFrame=cf*CFrame.new(item[2]) end
  end
 end
 if state.ball then
  local t=math.max(0,workspace:GetServerTimeNow()-state.ball.at);local p=ballPoint(state.ball.o,state.ball.v,t,P.BallGravity);local velocity=state.ball.v-Vector3.new(0,P.BallGravity*t,0)
  football.Transparency=0;football.CFrame=CFrame.lookAt(p,p+velocity)*CFrame.Angles(0,0,t*13)
 else football.Transparency=1 end
end)
