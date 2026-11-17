local Run = game:GetService("RunService")
if not Run:IsStudio() then warn("Vanta QB Lab is local Studio only"); return end
local Players = game:GetService("Players")
local Http = game:GetService("HttpService")
local RS = game:GetService("ReplicatedStorage")
local Lab = RS:WaitForChild("VantaQBLab")
require(Lab.BuildField)()
local P = require(Lab.Profile)
local Connection = require(script.Parent.VantaLabConnection)
local Event = Lab.Event
local session = Http:GenerateGUID(false)
local bots, nextId, selected, sequence = {}, 0, nil, 0
local running, missed, autoReact = false, false, true
local settings = {beam = true, smartFit = true, height = P.BeamHeight, lead = P.BeamLead}
local preview, ball, busy, result, owner = nil, nil, false, "", nil
local pendingThrow, generation = false, 0
local bridgeStatus, lastRequest = "Waiting for Vanta", 0
local function V(a) return Vector3.new(a[1], a[2], a[3]) end
local function A(v) return {v.X, v.Y, v.Z} end
local function add(role, position)
    if #bots >= 24 then return end
    nextId += 1
    local b = {id = tostring(nextId), role = role, p = position, home = position,
        v = Vector3.zero, speed = P.WalkSpeed, route = "Still", jump = 0, dive = 0, delay = 0,
        direction = Vector3.new(0, 0, -1), travel = 0, canJump = true, canDive = true}
    table.insert(bots, b)
    if role == "WR" and not selected then selected = b.id end
    return b
end
local function reset(name)
    generation += 1; bots = {}; selected = nil; preview = nil; ball = nil; pendingThrow = false; result = ""; running = false
    if name == "Overlap" then add("WR", Vector3.new(0, 4.5, -65)); add("DB", Vector3.new(2, 4.5, -65))
    elseif name == "Missed catch" then add("WR", Vector3.new(0, 4.5, -65)); add("DB", Vector3.new(0, 4.5, -100))
    elseif name == "Crossing" then local w = add("WR", Vector3.new(-30, 4.5, -65)); w.route = "Slant"; add("DB", Vector3.new(-15, 4.5, -75)); running = true
    else add("WR", Vector3.new(0, 4.5, -65)); add("DB", Vector3.new(-24, 4.5, -72)); add("DB", Vector3.new(20, 4.5, -90)) end
end
local function find(id) for _,b in ipairs(bots) do if b.id == id then return b end end end
local function toolFor(player)
    local ch = player.Character
    if not ch or ch:FindFirstChild("Lab Football") or player.Backpack:FindFirstChild("Lab Football") then return end
    local tool = Instance.new("Tool"); tool.Name = "Lab Football"; tool.CanBeDropped = false
    local handle = Instance.new("Part"); handle.Name = "Handle"; handle.Size = Vector3.new(1.1, 1.1, 1.8); handle.Color = Color3.fromRGB(123, 64, 36); handle.CanCollide = false; handle.Massless = true
    local mesh = Instance.new("SpecialMesh"); mesh.MeshType = Enum.MeshType.Sphere; mesh.Parent = handle; handle.Parent = tool
    tool.Parent = player.Backpack
    local h = ch:FindFirstChildOfClass("Humanoid"); if h then h:EquipTool(tool) end
end
local function initPlayer(player)
    if owner then return end; owner = player
    local function character(ch)
        local h = ch:WaitForChild("Humanoid"); h.WalkSpeed = P.WalkSpeed; h.UseJumpPower = true; h.JumpPower = P.JumpPower
        task.wait(0.2); ch:PivotTo(CFrame.new(0, 4, 30)); toolFor(player)
    end
    player.CharacterAdded:Connect(character); if player.Character then task.spawn(character, player.Character) end
end
Players.PlayerAdded:Connect(initPlayer); for _,p in ipairs(Players:GetPlayers()) do initPlayer(p) end
local function held()
    return owner and owner.Character and owner.Character:FindFirstChild("Lab Football")
end
local function actor(b)
    return {position = A(b.p), velocity = A(b.v), speed = b.speed, jumpHeight = b.canJump and P.JumpPower^2/(2*P.WorldGravity) or 0,
        diveReach = b.canDive and P.DiveReach or 0, radius = P.CatchRadius, above = P.CatchAbove, below = P.CatchBelow, worldGravity = P.WorldGravity}
end
local function point(o,v,t) return o + v*t - Vector3.new(0, P.BallGravity*t*t/2, 0) end
local function requestPlan(throw)
    local wr = find(selected); local head = owner and owner.Character and owner.Character:FindFirstChild("Head")
    if not wr or not head or not held() or ball then return end
    local epoch = generation; local targetId = selected
    local others = {}; for _,b in ipairs(bots) do if b.id ~= selected then table.insert(others,actor(b)) end end
    sequence += 1
    local body = {protocol = 1, session = session, sequence = sequence, origin = A(head.Position), receiver = actor(wr), defenders = others, target = selected,
        settings = settings, gravity = P.BallGravity, ground = P.GroundY}
    local ok, response = pcall(function()
        return Http:RequestAsync({Url = "http://127.0.0.1:"..Connection.port.."/plan", Method = "POST",
            Headers = {["Content-Type"] = "application/json", ["X-Vanta-Lab"] = Connection.token}, Body = Http:JSONEncode(body)})
    end)
    if epoch ~= generation or selected ~= targetId then return end
    if not ok or not response.Success then bridgeStatus = "Vanta disconnected - start the lab launcher"; preview = nil; lastRequest=workspace:GetServerTimeNow()+1; if not ok then script:SetAttribute("LastBridgeError", tostring(response)) end; return end
    local parsed, data = pcall(function() return Http:JSONDecode(response.Body) end)
    if not parsed or data.sequence ~= sequence or data.target ~= selected then bridgeStatus = "Invalid bridge reply"; return end
    bridgeStatus = "Vanta connected"; script:SetAttribute("BridgeConnected", true); script:SetAttribute("LastBridgeError",nil); script:SetAttribute("PlanCount", sequence)
    preview = data; preview.at = workspace:GetServerTimeNow()
    if throw and data.valid and held() then
        -- Use the exact native launch parameters. No second aim solver in Luau.
        local o, v = V(data.origin), V(data.velocity)
        if v.Magnitude > 95.01 or (o-head.Position).Magnitude > 12 then result = "Stale launch - retry"; return end
        ball = {o = o, v = v, at = workspace:GetServerTimeNow(), last = o, target = targetId, plan = data, elapsed = 0}
        script:SetAttribute("LastLaunch",Http:JSONEncode(data));script:SetAttribute("ThrowCount",(script:GetAttribute("ThrowCount") or 0)+1)
        local tool = held(); if tool then tool:Destroy() end
        result = "In flight"; Event:FireAllClients("release", data)
    elseif throw then result = "No reachable arc at these settings" end
end
local function finish(outcome)
    result = outcome; script:SetAttribute("LastResult", outcome); Event:FireAllClients("result", outcome)
    ball = nil; preview = nil; task.delay(0.7,function() if owner then toolFor(owner) end end)
end
local function catchGap(b, p)
    local delta = p-b.p; local vertical = delta.Y >= 0 and P.CatchAbove or P.CatchBelow
    return (delta.X^2+delta.Z^2)/P.CatchRadius^2 + delta.Y^2/vertical^2 - 1
end
local function botStep(b, dt, now)
    local horizontal = Vector3.zero
    local aim = nil
    if ball and autoReact then
        -- React after 180ms. Bots pursue an actual future ball point, including
        -- after the WR's missed catch; this is independent of the native risk label.
        local age = now-ball.at
        if age > 0.18 then
            local best = math.huge
            for step = 1, 18 do
                local horizon = step*0.08
                local p = point(ball.o,ball.v,age+horizon)
                if p.Y >= P.GroundY+0.6 and p.Y <= b.p.Y+P.CatchAbove+6.4 then
                    local d = Vector3.new(p.X-b.p.X,0,p.Z-b.p.Z).Magnitude
                    local cost = math.abs(d-b.speed*horizon) + horizon*3
                    if cost < best then best = cost; aim = p end
                end
            end
        end
    end
    if aim then
        local delta = Vector3.new(aim.X-b.p.X,0,aim.Z-b.p.Z)
        if delta.Magnitude > 0.1 then horizontal = delta.Unit*b.speed; b.direction = delta.Unit end
        if b.canJump and b.p.Y <= 4.501 and aim.Y-b.p.Y > P.CatchAbove-0.5 and delta.Magnitude < 15 then b.jump = P.JumpPower end
        if b.canDive and b.dive <= 0 and delta.Magnitude > 8 and delta.Magnitude < 18 then b.dive = P.DiveDuration end
    elseif running then
        if b.route == "Go" then horizontal = Vector3.new(0,0,-b.speed)
        elseif b.route == "Slant" then horizontal = Vector3.new(0.8,0,-0.6)*b.speed
        elseif b.route == "Comeback" then horizontal = Vector3.new(0,0,b.travel > 30 and b.speed or -b.speed)
        elseif b.route == "Delayed" then b.delay += dt; if b.delay > 1 then horizontal = Vector3.new(0,0,-b.speed) end end
    end
    if horizontal.Magnitude > 0.1 then b.direction = horizontal.Unit end
    if b.dive > 0 then b.dive = math.max(0,b.dive-dt); horizontal = b.direction*P.DiveSpeed end
    local y = b.p.Y + b.jump*dt - P.WorldGravity*dt*dt/2
    b.jump -= P.WorldGravity*dt
    if y <= 4.5 then y = 4.5; b.jump = 0 end
    b.v = Vector3.new(horizontal.X,b.jump,horizontal.Z)
    b.p = Vector3.new(math.clamp(b.p.X+horizontal.X*dt,-77,77),y,math.clamp(b.p.Z+horizontal.Z*dt,-300,48)); b.travel += horizontal.Magnitude*dt
end
local broadcast = 0
Run.Heartbeat:Connect(function(dt)
    local now = workspace:GetServerTimeNow()
    local steps = math.max(1,math.ceil(math.min(dt,0.1)/0.008)); local sub = math.min(dt,0.1)/steps
    for i=1,steps do
        local sample = now-(steps-i)*sub
        for _,b in ipairs(bots) do botStep(b,sub,sample) end
        if ball then
            local age = math.max(0,sample-ball.at); local pos = point(ball.o,ball.v,age)
            local candidates = {}
            for _,b in ipairs(bots) do
                if not (missed and b.id == ball.target) then local gap = catchGap(b,pos); if gap <= 0 then table.insert(candidates,{b=b,gap=gap}) end end
            end
            table.sort(candidates,function(a,b) return a.gap < b.gap end)
            if #candidates > 0 then finish(candidates[1].b.id == ball.target and "Caught by WR" or "Intercepted by "..candidates[1].b.role.." "..candidates[1].b.id)
            elseif pos.Y <= P.GroundY+P.BallRadius or age > 12 then finish("Incomplete")
            else ball.last = pos; ball.elapsed = age end
        end
    end
    broadcast += dt
    if broadcast >= 0.05 then
        broadcast = 0; local actors = {}
        for _,b in ipairs(bots) do table.insert(actors,{id=b.id,role=b.role,p=b.p,v=b.v,route=b.route,dive=b.dive>0}) end
        Event:FireAllClients("state",{at=now,bots=actors,selected=selected,running=running,missed=missed,react=autoReact,settings=settings,preview=preview,
            ball=ball and {o=ball.o,v=ball.v,at=ball.at} or false,result=result,status=bridgeStatus,held=held()~=nil})
    end
    -- Five requests/second leaves headroom below Studio's HTTP quota.
    -- A click requests a fresh native plan on the next available tick.
    if not busy and now-lastRequest >= 0.2 and held() and not ball then
        busy=true; lastRequest=now; local throw=pendingThrow; pendingThrow=false
        task.spawn(function() local ok,err=pcall(requestPlan,throw); if not ok then warn("Lab planner: "..tostring(err)) end; busy=false end)
    end
end)
local lastInput = 0
Event.OnServerEvent:Connect(function(player, action, value)
    if player ~= owner or type(action) ~= "string" then return end
    local now = os.clock(); if now-lastInput < 0.025 then return end; lastInput=now
    if action == "throw" then if held() and not ball then pendingThrow=true end
    elseif action == "select" then local b=find(tostring(value)); if b and b.role=="WR" then selected=b.id; preview=nil; generation+=1 end
    elseif action == "add" and type(value)=="table" and typeof(value.p)=="Vector3" and (value.role=="WR" or value.role=="DB") then
        add(value.role,Vector3.new(math.clamp(value.p.X,-75,75),4.5,math.clamp(value.p.Z,-295,45))); generation+=1
    elseif action == "move" and type(value)=="table" and typeof(value.p)=="Vector3" then local b=find(value.id); if b then b.p=Vector3.new(math.clamp(value.p.X,-75,75),4.5,math.clamp(value.p.Z,-295,45));b.home=b.p;b.v=Vector3.zero;b.jump=0;preview=nil;generation+=1 end
    elseif action == "scenario" and type(value)=="string" then reset(value); toolFor(player)
    elseif action == "run" then running=not running
    elseif action == "miss" then missed=not missed
    elseif action == "react" then autoReact=not autoReact
    elseif action == "reset" then generation+=1;ball=nil;preview=nil;result="";pendingThrow=false; for _,b in ipairs(bots) do b.p=b.home;b.v=Vector3.zero;b.travel=0;b.delay=0;b.jump=0;b.dive=0 end;toolFor(player)
    elseif action == "route" and type(value)=="table" then local b=find(value.id);if b and table.find({"Still","Go","Slant","Comeback","Delayed"},value.route) then b.route=value.route;b.travel=0;b.delay=0 end
    elseif action == "jump" then for _,b in ipairs(bots) do if b.p.Y<=4.501 then b.jump=P.JumpPower end end
    elseif action == "dive" then for _,b in ipairs(bots) do b.dive=P.DiveDuration end
    elseif action == "settings" and type(value)=="table" then
        if type(value.beam)=="boolean" then settings.beam=value.beam end
        if type(value.smartFit)=="boolean" then settings.smartFit=value.smartFit end
        if type(value.height)=="number" and value.height==value.height then settings.height=math.clamp(value.height,-8,12) end
        if type(value.lead)=="number" and value.lead==value.lead then settings.lead=math.clamp(value.lead,-12,12) end
        generation+=1;preview=nil
    end
end)
reset("Two defenders")
script:SetAttribute("LabReady",true)
