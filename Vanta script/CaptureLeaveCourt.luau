-- Temporary outgoing-remote diagnostic. Run, then manually click Leave Court.
-- Stops capturing after 30 seconds; never invokes or replays a remote itself.
local env=getgenv and getgenv()or _G
if env.VantaLeaveCaptureStop then pcall(env.VantaLeaveCaptureStop)end
local playerGui=game:GetService("Players").LocalPlayer:WaitForChild("PlayerGui")
local oldGui=playerGui:FindFirstChild("VantaLeaveCapturePrompt")
if oldGui then oldGui:Destroy()end
local gui=Instance.new("ScreenGui")
gui.Name="VantaLeaveCapturePrompt";gui.ResetOnSpawn=false;gui.DisplayOrder=10000;gui.Parent=playerGui
local panel=Instance.new("Frame")
panel.AnchorPoint=Vector2.new(.5,0);panel.Position=UDim2.fromScale(.5,.08)
panel.Size=UDim2.new(.88,0,0,154);panel.BackgroundColor3=Color3.fromRGB(18,22,32);panel.Parent=gui
local limit=Instance.new("UISizeConstraint");limit.MaxSize=Vector2.new(540,154);limit.Parent=panel
local corner=Instance.new("UICorner");corner.CornerRadius=UDim.new(0,12);corner.Parent=panel
local border=Instance.new("UIStroke");border.Color=Color3.fromRGB(85,120,255);border.Parent=panel
local function label(y,height,size)
 local text=Instance.new("TextLabel");text.BackgroundTransparency=1
 text.Position=UDim2.new(0,18,0,y);text.Size=UDim2.new(1,-60,0,height)
 text.Font=Enum.Font.GothamMedium;text.TextSize=size;text.TextColor3=Color3.fromRGB(235,239,255)
 text.TextXAlignment=Enum.TextXAlignment.Left;text.TextWrapped=true;text.Parent=panel
 return text
end
local heading=label(14,34,23);heading.Text="Starting capture..."
local detail=label(52,52,15);detail.Text="Wait until this says READY before leaving."
local timer=label(113,26,14);timer.Text="Preparing logger"
local close=Instance.new("TextButton");close.Text="X";close.TextSize=18;close.Font=Enum.Font.GothamBold
close.Size=UDim2.fromOffset(32,32);close.Position=UDim2.new(1,-40,0,8)
close.BackgroundTransparency=1;close.TextColor3=Color3.fromRGB(220,225,240);close.Parent=panel
close.MouseButton1Click:Connect(function()
 if env.VantaLeaveCaptureStop then env.VantaLeaveCaptureStop()end
 gui:Destroy()
end)
if type(hookmetamethod)~="function"or type(getnamecallmethod)~="function"then
 heading.Text="Capture could not start";heading.TextColor3=Color3.fromRGB(255,120,120)
 detail.Text="The required diagnostic hook is unavailable. Do not leave yet; send this message to chat."
 timer.Text="Not recording"
 return
end
local active=true
local started=os.clock()
local lines={"Vanta Leave Court capture"}
local storage=game:GetService("ReplicatedStorage")
local function formatValue(value,depth,seen)
 local kind=typeof(value)
 if kind=="string"then return string.format("%q",value:sub(1,1500))end
 if kind=="Instance"then return "Instance("..string.format("%q",value:GetFullName())..")"end
 if kind~="table"then return tostring(value)end
 if depth>=4 then return "{...}"end
 if seen[value]then return "<cycle>"end
 seen[value]=true
 local parts={}
 local count=0
 for key,item in pairs(value)do
  count+=1;if count>40 then parts[#parts+1]="...";break end
  parts[#parts+1]="["..formatValue(key,depth+1,seen).."]="..formatValue(item,depth+1,seen)
 end
 seen[value]=nil
 return "{"..table.concat(parts,", ").."}"
end
local function save()
 local report=table.concat(lines,"\n")
 if type(writefile)=="function"then pcall(writefile,"VantaLeaveCourtCapture.txt",report)end
 if type(setclipboard)=="function"then pcall(setclipboard,report)end
end
env.VantaLeaveCaptureStop=function()active=false end
local previous
local function observe(self,...)
 local method=getnamecallmethod()
 if active and os.clock()-started<=30 and(method=="FireServer"or method=="InvokeServer")then
  local args=table.pack(...)
  pcall(function()
   if typeof(self)~="Instance"or not self:IsDescendantOf(storage)then return end
   local values={}
   for index=1,args.n do values[index]=formatValue(args[index],0,{})end
   local entry=string.format("[%.3fs] %s:%s(%s)",os.clock()-started,self:GetFullName(),method,table.concat(values,", "))
   lines[#lines+1]=entry
   if #lines>150 then table.remove(lines,2)end
   print(entry)
   -- Persist before forwarding: InvokeServer may cause an immediate teleport.
   save()
  end)
 end
 return previous(self,...)
end
local hooked,result=pcall(function()
 return hookmetamethod(game,"__namecall",type(newcclosure)=="function"and newcclosure(observe)or observe)
end)
if not hooked then
 active=false;heading.Text="Capture could not start";detail.Text=tostring(result);timer.Text="Not recording";return
end
previous=result
started=os.clock()
save()
heading.Text="READY - Click Leave Court now"
heading.TextColor3=Color3.fromRGB(125,225,170)
detail.Text="Click the game's Leave Court text. After reaching the lobby, paste your clipboard into chat."
if type(setclipboard)~="function"then detail.Text="Click Leave Court. Clipboard is unavailable; send VantaLeaveCourtCapture.txt from your executor workspace afterward."end
print("[Leave capture] Click Leave Court now. Calls are printed, copied and saved for 30 seconds.")
task.spawn(function()
 while active and gui.Parent and os.clock()-started<30 do
  timer.Text=string.format("Recording: %ds remaining | %d calls",math.ceil(30-(os.clock()-started)),#lines-1)
  task.wait(.2)
 end
 active=false
 if gui.Parent then
  heading.Text="Capture finished"
  detail.Text=#lines>1 and "Paste your clipboard into chat, or send VantaLeaveCourtCapture.txt."or "No calls captured. Run this script again, then click Leave Court while READY is showing."
  timer.Text=tostring(#lines-1).." calls captured"
 end
 print("[Leave capture] Capture stopped. Paste your clipboard into chat; backup: VantaLeaveCourtCapture.txt")
end)
