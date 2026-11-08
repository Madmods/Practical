-- Read-only remote inventory. No hooks, remote calls, or input simulation.
local rows={}
for _,item in ipairs(game:GetDescendants())do
 if item:IsA("RemoteEvent")or item:IsA("RemoteFunction")or item:IsA("UnreliableRemoteEvent")then
  local ok,path=pcall(function()return item:GetFullName()end)
  if ok then rows[#rows+1]="["..item.ClassName.."] "..path end
 end
end
table.sort(rows)
local report="Vanta remote inventory (names only; no calls recorded)\nCount: "..#rows.."\n"..table.concat(rows,"\n")
for _,row in ipairs(rows)do print(row)end
local copied=false
if type(setclipboard)=="function"then copied=pcall(setclipboard,report)end
local saved=false
if type(writefile)=="function"then saved=pcall(writefile,"VantaRemoteInventory.txt",report)end
local pg=game:GetService("Players").LocalPlayer:WaitForChild("PlayerGui")
local previous=pg:FindFirstChild("VantaRemoteInventory")
if previous then previous:Destroy()end
local gui=Instance.new("ScreenGui");gui.Name="VantaRemoteInventory";gui.ResetOnSpawn=false;gui.DisplayOrder=10000;gui.Parent=pg
local panel=Instance.new("TextButton");panel.AnchorPoint=Vector2.new(.5,0)
panel.Position=UDim2.fromScale(.5,.08);panel.Size=UDim2.new(.85,0,0,130)
panel.BackgroundColor3=Color3.fromRGB(20,24,34);panel.TextColor3=Color3.fromRGB(230,238,255)
panel.Font=Enum.Font.GothamMedium;panel.TextSize=18;panel.TextWrapped=true
panel.Text="Found "..#rows.." remotes\n"..(copied and "Copied to clipboard - paste into chat."or(saved and "Saved VantaRemoteInventory.txt in your executor workspace."or "Printed to F9 console; clipboard and file saving unavailable.")).."\nClick this notice to close."
panel.Parent=gui
local limit=Instance.new("UISizeConstraint");limit.MaxSize=Vector2.new(600,130);limit.Parent=panel
local corner=Instance.new("UICorner");corner.CornerRadius=UDim.new(0,12);corner.Parent=panel
panel.MouseButton1Click:Connect(function()gui:Destroy()end)
