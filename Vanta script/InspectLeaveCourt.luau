-- Read-only exit-button diagnostic. Does not click, leave, or invoke remotes.
local Players=game:GetService("Players")
local player=Players.LocalPlayer
local lines={"Vanta Leave Court diagnostic"}
local function add(value) lines[#lines+1]=tostring(value) end
local function describe(item)
 add(item:GetFullName().." ["..item.ClassName.."]")
 if item:IsA("GuiObject") then
  add("  Visible="..tostring(item.Visible).." Active="..tostring(item.Active).." Position="..tostring(item.AbsolutePosition).." Size="..tostring(item.AbsoluteSize))
 end
 if item:IsA("LayerCollector") then add("  Enabled="..tostring(item.Enabled)) end
 if item:IsA("ScreenGui") then add("  IgnoreGuiInset="..tostring(item.IgnoreGuiInset)) end
end
local function attributes(item)
 if not item then return end
 add("STATE "..item:GetFullName())
 for _,key in ipairs({"InGame","OnCourt","CourtNumber","Goal","Action"})do
  add("  "..key.."="..tostring(item:GetAttribute(key)))
 end
 for _,child in ipairs(item:GetChildren())do
  if child:IsA("ValueBase") and (child.Name=="InGame"or child.Name=="OnCourt"or child.Name=="Goal")then
   add("  value "..child.Name.."="..tostring(child.Value))
  end
 end
end
attributes(player)
attributes(player.Character)
local characters=workspace:FindFirstChild("Characters")
local rig=characters and characters:FindFirstChild(player.Name)
if rig~=player.Character then attributes(rig) end
local matches=0
local roots={player:WaitForChild("PlayerGui"),workspace}
for _,root in ipairs(roots)do
 for _,item in ipairs(root:GetDescendants())do
  if item:IsA("GuiObject") then
   local label=""
   if item:IsA("TextLabel")or item:IsA("TextButton")then label=item.Text end
   local search=(item.Name.." "..label):lower()
   if search:find("leave",1,true)or search:find("lobby",1,true)then
    matches+=1;add("\nMATCH "..matches.." text="..label)
    local ancestor=item
    for _=1,8 do
     if not ancestor then break end
     describe(ancestor);ancestor=ancestor.Parent
    end
    local parent=item.Parent
    if parent then
     for _,sibling in ipairs(parent:GetChildren())do
      if sibling~=item and sibling:IsA("GuiObject")then add("  SIBLING");describe(sibling) end
     end
    end
   end
  end
 end
end
add("\nMatches="..matches)
local insetTopLeft,insetBottomRight=game:GetService("GuiService"):GetGuiInset()
add("GuiInset top-left="..tostring(insetTopLeft).." bottom-right="..tostring(insetBottomRight))
local report=table.concat(lines,"\n")
print(report)
local copied=false
if type(setclipboard)=="function"then copied=pcall(setclipboard,report) end
if type(writefile)=="function"then pcall(writefile,"VantaLeaveCourtDiagnostic.txt",report)end
pcall(function()game:GetService("StarterGui"):SetCore("SendNotification",{
 Title="Leave Court diagnostic",Text=copied and "Copied report. Paste it into chat."or "Report printed to console.",Duration=10
})end)
