-- Read-only court geometry report. Run while standing inside the affected court.
local player=game:GetService("Players").LocalPlayer
local characters=workspace:FindFirstChild("Characters")
local character=characters and characters:FindFirstChild(player.Name)or player.Character
local root=character and(character:FindFirstChild("HumanoidRootPart")or character.PrimaryPart)
local lines={"Vanta court boundary diagnostic"}
local function add(text)lines[#lines+1]=text end
local function attributes(object)
 for key,value in pairs(object:GetAttributes())do
  local name=key:lower()
  if name:find("court")or name:find("bound")or name:find("game")or name=="goal"then
   add("  attribute "..key.."="..tostring(value))
  end
 end
end
if character then add("Character: "..character:GetFullName());attributes(character)end
if root then add("Position: "..tostring(root.Position))end
local assets=workspace:FindFirstChild("Assets")
local functionals=assets and assets:FindFirstChild("Functionals")
local zones=functionals and functionals:FindFirstChild("Zones")
add("Zones: "..(zones and zones:GetFullName()or "NOT FOUND"))
local count=0
for _,object in ipairs(workspace:GetDescendants())do
 local name=object.Name:lower()
 local named=name:find("bound")or name:find("court")or name:find("sideline")or name:find("baseline")or name:find("threepoint")or name=="oob"
 local inZones=zones and object:IsDescendantOf(zones)
 if named or inZones then
  local nearby=not root or not object:IsA("BasePart")or(object.Position-root.Position).Magnitude<180
  if nearby and count<350 then
   count+=1
   add(object:GetFullName().." ["..object.ClassName.."]")
   if object:IsA("BasePart")then
    add("  Position="..tostring(object.Position).." Size="..tostring(object.Size))
    add("  CFrame="..tostring(object.CFrame).." CanQuery="..tostring(object.CanQuery).." CanCollide="..tostring(object.CanCollide))
   end
   attributes(object)
  end
 end
end
add("Entries: "..count..(count>=350 and " (capped)"or ""))
local report=table.concat(lines,"\n")
print(report)
local copied=false
if type(setclipboard)=="function"then copied=pcall(setclipboard,report)end
if type(writefile)=="function"then pcall(writefile,"VantaCourtBounds.txt",report)end
local gui=Instance.new("ScreenGui");gui.Name="VantaBoundsReport";gui.ResetOnSpawn=false;gui.Parent=player:WaitForChild("PlayerGui")
local message=Instance.new("TextButton");message.Size=UDim2.new(.8,0,0,100);message.AnchorPoint=Vector2.new(.5,0)
message.Position=UDim2.fromScale(.5,.1);message.BackgroundColor3=Color3.fromRGB(20,24,34)
message.TextColor3=Color3.fromRGB(230,240,255);message.TextSize=18;message.TextWrapped=true
message.Text=copied and "Court report copied. Paste it into chat.\nClick to close."or "Court report printed to F9 and saved if supported.\nClick to close."
message.Parent=gui;message.MouseButton1Click:Connect(function()gui:Destroy()end)
