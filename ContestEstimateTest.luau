-- Standalone contest estimate. Client-side geometry only; no game inputs or remotes.
-- Tune these values against actual shot feedback. The estimate is not a server value.
local CONFIG = {
    ZeroContestDistance = 12, -- Horizontal studs beyond which a defender adds no contest.
    FullContestDistance = 2, -- Horizontal studs giving maximum proximity pressure.
    CloseRangeDistance = 5, -- Close bonus begins here and ramps toward full-contest distance.
    CloseRangeMultiplier = 5.3, -- Slider strength; bonus above 1 is amplified below.
    DistanceFalloffStart = 6.5, -- Additional attenuation starts beyond this distance.
    DistanceDropPerStud = 0.93, -- Remove this fraction of remaining pressure per extra stud.
    CloseRangeBonusGain = 3, -- Each slider increase produces three times the previous bonus.
    Curve = 1.6, -- Higher makes medium-distance defenders contribute less.
    GuardMultiplier = 1.12,
    JumpMultiplier = 1.18,
    MultipleDefenderWeight = 0.25,
    IgnoreSameTeam = true,
}

local Players = game:GetService("Players")
local RunService = game:GetService("RunService")
local Input = game:GetService("UserInputService")
local adjustment = 87
local latest = {percent = 0, distance = nil, defenders = 0}
local player = Players.LocalPlayer
local guiParent = player:WaitForChild("PlayerGui")
local old = guiParent:FindFirstChild("ContestEstimateTest")
if old then old:Destroy() end

local gui = Instance.new("ScreenGui")
gui.Name = "ContestEstimateTest"
gui.ResetOnSpawn = false
gui.DisplayOrder = 100
gui.Parent = guiParent
local panel = Instance.new("Frame")
panel.Size = UDim2.fromOffset(340, 482)
panel.Position = UDim2.new(0.5, -170, 0, 70)
panel.BackgroundColor3 = Color3.fromRGB(19, 22, 30)
panel.BorderSizePixel = 0
panel.Parent = gui
local corner = Instance.new("UICorner")
corner.CornerRadius = UDim.new(0, 10)
corner.Parent = panel
local label = Instance.new("TextLabel")
label.Size = UDim2.new(1, -40, 0, 50)
label.Position = UDim2.fromOffset(14, 8)
label.BackgroundTransparency = 1
label.Font = Enum.Font.GothamBold
label.TextSize = 22
label.TextColor3 = Color3.fromRGB(117, 167, 255)
label.TextXAlignment = Enum.TextXAlignment.Left
label.Text = "Estimated contest: --"
label.Parent = panel
local detail = Instance.new("TextLabel")
detail.Size = UDim2.new(1, -28, 0, 45)
detail.Position = UDim2.fromOffset(14, 60)
detail.BackgroundTransparency = 1
detail.Font = Enum.Font.Gotham
detail.TextSize = 13
detail.TextColor3 = Color3.fromRGB(185, 192, 208)
detail.TextXAlignment = Enum.TextXAlignment.Left
detail.TextWrapped = true
detail.Text = "Compare this estimate with actual shot feedback."
detail.Parent = panel
local close = Instance.new("TextButton")
close.Size = UDim2.fromOffset(28, 28)
close.Position = UDim2.new(1, -32, 0, 4)
close.BackgroundTransparency = 1
close.Text = "×"
close.Font = Enum.Font.Gotham
close.TextSize = 23
close.TextColor3 = Color3.fromRGB(200, 205, 215)
close.Parent = panel
close.Activated:Connect(function() gui:Destroy() end)

local sliderLabel = Instance.new("TextLabel")
sliderLabel.Size = UDim2.new(1, -28, 0, 24)
sliderLabel.Position = UDim2.fromOffset(14, 112)
sliderLabel.BackgroundTransparency = 1
sliderLabel.Font = Enum.Font.Gotham
sliderLabel.TextSize = 14
sliderLabel.TextColor3 = Color3.fromRGB(220, 225, 238)
sliderLabel.TextXAlignment = Enum.TextXAlignment.Left
sliderLabel.Parent = panel
local track = Instance.new("TextButton")
track.Size = UDim2.new(1, -40, 0, 24)
track.Position = UDim2.fromOffset(20, 142)
track.BackgroundColor3 = Color3.fromRGB(40, 46, 62)
track.BorderSizePixel = 0
track.Text = ""
track.AutoButtonColor = false
track.Parent = panel
local fill = Instance.new("Frame")
fill.BackgroundColor3 = Color3.fromRGB(80, 130, 255)
fill.BorderSizePixel = 0
fill.Size = UDim2.fromScale(0.5, 1)
fill.Parent = track
local knob = Instance.new("Frame")
knob.Size = UDim2.fromOffset(8, 28)
knob.AnchorPoint = Vector2.new(0.5, 0.5)
knob.Position = UDim2.fromScale(0.5, 0.5)
knob.BackgroundColor3 = Color3.fromRGB(235, 240, 255)
knob.BorderSizePixel = 0
knob.Parent = track
local function setAdjustment(nextValue)
 adjustment = math.clamp(math.floor(nextValue + 0.5), 0, 200)
 fill.Size = UDim2.fromScale(adjustment / 200, 1)
 knob.Position = UDim2.fromScale(adjustment / 200, 0.5)
 sliderLabel.Text = string.format("Estimate strength: %d%%  (100%% = original)", adjustment)
end
setAdjustment(87)
local dragging = nil
local function dragAt(x)
 if track.AbsoluteSize.X > 0 then
  setAdjustment((x - track.AbsolutePosition.X) / track.AbsoluteSize.X * 200)
 end
end
track.InputBegan:Connect(function(event)
 if event.UserInputType == Enum.UserInputType.MouseButton1 or event.UserInputType == Enum.UserInputType.Touch then
  dragging = event
  dragAt(event.Position.X)
 end
end)
local moveConnection = Input.InputChanged:Connect(function(event)
 if dragging and (event == dragging or event.UserInputType == Enum.UserInputType.MouseMovement) then dragAt(event.Position.X) end
end)
local endConnection = Input.InputEnded:Connect(function(event)
 if event == dragging or event.UserInputType == Enum.UserInputType.MouseButton1 then dragging = nil end
end)
local extraConnections = {}
local function tuningSlider(y, caption, minimum, maximum, step, initial, changed, format)
 local text = sliderLabel:Clone()
 text.Position = UDim2.fromOffset(14, y)
 text.Parent = panel
 local bar = track:Clone()
 bar.Position = UDim2.fromOffset(20, y + 27)
 bar:ClearAllChildren()
 bar.Parent = panel
 local progress = fill:Clone()
 progress.Parent = bar
 local handle = knob:Clone()
 handle.Parent = bar
 local function set(value)
  value = math.clamp(math.floor((value - minimum) / step + .5) * step + minimum, minimum, maximum)
  local ratio = (value - minimum) / (maximum - minimum)
  progress.Size = UDim2.fromScale(ratio, 1)
  handle.Position = UDim2.fromScale(ratio, .5)
  text.Text = caption .. string.format(format, value)
  changed(value)
 end
 local active
 local function update(x)
  if bar.AbsoluteSize.X > 0 then set(minimum + (x - bar.AbsolutePosition.X) / bar.AbsoluteSize.X * (maximum - minimum)) end
 end
 bar.InputBegan:Connect(function(event)
  if event.UserInputType == Enum.UserInputType.MouseButton1 or event.UserInputType == Enum.UserInputType.Touch then active = event;update(event.Position.X) end
 end)
 table.insert(extraConnections, Input.InputChanged:Connect(function(event)
  if active and (event == active or event.UserInputType == Enum.UserInputType.MouseMovement) then update(event.Position.X) end
 end))
 table.insert(extraConnections, Input.InputEnded:Connect(function(event)
  if event == active or event.UserInputType == Enum.UserInputType.MouseButton1 then active = nil end
 end))
 set(initial)
 return set
end
local setCloseMultiplier = tuningSlider(174, "Close-range multiplier: ", 1, 10, .05, CONFIG.CloseRangeMultiplier,
 function(v) CONFIG.CloseRangeMultiplier = v end, "%.2fx")
local setCloseDistance = tuningSlider(236, "Close bonus starts within: ", 3, 10, .5, CONFIG.CloseRangeDistance,
 function(v) CONFIG.CloseRangeDistance = v end, "%.1f studs")
local setFalloffStart = tuningSlider(298, "Distance drop starts after: ", 2, 12, .5, CONFIG.DistanceFalloffStart,
 function(v) CONFIG.DistanceFalloffStart = v end, "%.1f studs")
local setDropPerStud = tuningSlider(360, "Extra reduction per stud: ", 0, 100, 1, CONFIG.DistanceDropPerStud * 100,
 function(v) CONFIG.DistanceDropPerStud = v / 100 end, "%.0f%%")
gui.Destroying:Connect(function() for _, connection in ipairs(extraConnections) do connection:Disconnect() end end)

local function button(text, x, width)
 local item = Instance.new("TextButton")
 item.Position = UDim2.fromOffset(x, 428)
 item.Size = UDim2.fromOffset(width, 32)
 item.BackgroundColor3 = Color3.fromRGB(40, 55, 87)
 item.BorderSizePixel = 0
 item.Font = Enum.Font.Gotham
 item.TextSize = 14
 item.TextColor3 = Color3.fromRGB(230, 235, 245)
 item.Text = text
 item.Parent = panel
 return item
end
local copyButton = button("Copy settings", 14, 180)
local resetButton = button("Reset", 206, 120)
resetButton.Activated:Connect(function() setAdjustment(87);setCloseMultiplier(5.3);setCloseDistance(5);setFalloffStart(6.5);setDropPerStud(93) end)
local exportBox = Instance.new("TextBox")
exportBox.Position = UDim2.fromOffset(14, 488)
exportBox.Size = UDim2.new(1, -28, 0, 110)
exportBox.BackgroundColor3 = Color3.fromRGB(28, 33, 45)
exportBox.TextColor3 = Color3.fromRGB(220, 225, 240)
exportBox.Font = Enum.Font.Code
exportBox.TextSize = 12
exportBox.TextWrapped = true
exportBox.MultiLine = true
exportBox.ClearTextOnFocus = false
exportBox.TextEditable = false
exportBox.Visible = false
exportBox.Parent = panel
copyButton.Activated:Connect(function()
 local report = game:GetService("HttpService"):JSONEncode({
  version = 1, tool = "ContestEstimateTest", strengthPercent = adjustment,
  config = CONFIG, snapshot = latest,
  note = "Snapshot is estimated; actual shot feedback must be supplied separately."
 })
 local copied = false
 if type(setclipboard) == "function" then copied = pcall(setclipboard, report) end
 if copied then copyButton.Text = "Copied settings!"
 else
  panel.Size = UDim2.fromOffset(340, 613)
  exportBox.Visible = true
  exportBox.Text = report
  exportBox:CaptureFocus()
  exportBox.SelectionStart = 1
  exportBox.CursorPosition = #report + 1
  copyButton.Text = "Select text / Ctrl+C"
 end
end)
gui.Destroying:Connect(function()
 moveConnection:Disconnect()
 endConnection:Disconnect()
end)

local function value(character, key)
    local result = character:GetAttribute(key)
    if result ~= nil then return result end
    local folder = character:FindFirstChild("Attributes")
    local object = folder and folder:FindFirstChild(key)
    if object and object:IsA("ValueBase") then return object.Value end
end
local function root(character)
    return character and character:FindFirstChild("HumanoidRootPart")
end
local function known(v)
    return v ~= nil and v ~= 0 and v ~= "" and v ~= false
end
local function ownCharacter()
    local chars = workspace:FindFirstChild("Characters")
    return (chars and chars:FindFirstChild(player.Name)) or player.Character
end
local function candidates()
    local result, seen = {}, {}
    local function add(character)
        if character and not seen[character] and root(character) then
            seen[character] = true
            table.insert(result, character)
        end
    end
    local chars = workspace:FindFirstChild("Characters")
    if chars then for _, character in ipairs(chars:GetChildren()) do add(character) end end
    for _, other in ipairs(Players:GetPlayers()) do add(other.Character) end
    return result
end

-- Bounded monotonic distance curve; intentionally exposed for tuning.
local function proximity(distance)
    local span = math.max(0.1, CONFIG.ZeroContestDistance - CONFIG.FullContestDistance)
    local closeness = math.clamp((CONFIG.ZeroContestDistance - distance) / span, 0, 1)
    local closeSpan = math.max(.1, CONFIG.CloseRangeDistance - CONFIG.FullContestDistance)
    local closeWeight = math.clamp((CONFIG.CloseRangeDistance - distance) / closeSpan, 0, 1)
    local multiplier = 1 + (CONFIG.CloseRangeMultiplier - 1) * CONFIG.CloseRangeBonusGain * closeWeight
    local extraStuds = math.max(0, distance - CONFIG.DistanceFalloffStart)
    local distanceMultiplier = extraStuds == 0 and 1 or (1 - CONFIG.DistanceDropPerStud) ^ extraStuds
    return math.clamp(closeness ^ CONFIG.Curve * multiplier * distanceMultiplier, 0, 1)
end
local function estimate(own, ownRoot)
    local ownTeam, ownCourt = value(own, "Team"), value(own, "CourtNumber")
    local scores = {}
    local nearest = math.huge
    for _, character in ipairs(candidates()) do
        if character ~= own and character ~= player.Character then
            local otherRoot = root(character)
            local otherTeam, otherCourt = value(character, "Team"), value(character, "CourtNumber")
            local sameTeam = CONFIG.IgnoreSameTeam and known(ownTeam) and known(otherTeam) and ownTeam == otherTeam
            local otherPlayer = Players:GetPlayerFromCharacter(character)
            if CONFIG.IgnoreSameTeam and player.Team and otherPlayer and not player.Neutral and not otherPlayer.Neutral and player.Team == otherPlayer.Team then sameTeam = true end
            local otherCourtKnown = known(ownCourt) and known(otherCourt) and ownCourt ~= otherCourt
            local hum = character:FindFirstChildOfClass("Humanoid")
            if not sameTeam and not otherCourtKnown and (not hum or hum.Health > 0) then
                local delta = otherRoot.Position - ownRoot.Position
                if math.abs(delta.Y) <= 9 then
                    local distance = Vector3.new(delta.X, 0, delta.Z).Magnitude
                    nearest = math.min(nearest, distance)
                    local score = proximity(distance)
                    local action = value(character, "Action")
                    if value(character, "HoldingG") == true then score *= CONFIG.GuardMultiplier end
                    if action == "Blocking" or (hum and hum.FloorMaterial == Enum.Material.Air and delta.Y > 0.5) then score *= CONFIG.JumpMultiplier end
                    if score > 0 then table.insert(scores, math.clamp(score, 0, 1)) end
                end
            end
        end
    end
    table.sort(scores, function(a, b) return a > b end)
    local combined = scores[1] or 0
    for i = 2, #scores do combined += (1 - combined) * scores[i] * CONFIG.MultipleDefenderWeight end
    return math.clamp(combined * 100, 0, 100), nearest, #scores
end

local elapsed = 0
local connection
connection = RunService.Heartbeat:Connect(function(dt)
    elapsed += dt
    if elapsed < 0.08 then return end
    elapsed = 0
    local own = ownCharacter()
    local ownRoot = root(own)
    if not ownRoot then label.Text = "Waiting for character…";return end
    local percent, nearest, count = estimate(own, ownRoot)
    percent = math.clamp(percent * adjustment / 100, 0, 100)
    latest = {percent = percent, distance = nearest < math.huge and nearest or nil, defenders = count}
    label.Text = string.format("Estimated contest: %.0f%%", percent)
    detail.Text = string.format("Nearest: %s | Nearby defenders: %d\nEstimate only — compare with shot feedback.", nearest < math.huge and string.format("%.1f studs", nearest) or "none", count)
end)
gui.Destroying:Connect(function() if connection then connection:Disconnect() end end)
