-- Experimental peak prediction for the captured Bat meter, rotation 270.
-- No manual learning, no perfect-shot requirement, no GUI/property modification.
local env=(getgenv and getgenv()) or _G
if env.PracticalAutoTimeCleanup then pcall(env.PracticalAutoTimeCleanup) end
if env.PracticalMeterResearchStop then pcall(env.PracticalMeterResearchStop) end
if env.PracticalPeakCleanup then pcall(env.PracticalPeakCleanup) end
local P=game:GetService("Players").LocalPlayer
local Run=game:GetService("RunService")
local Input=game:GetService("UserInputService")
-- Shared keyboard / mouse binding support.
local function bindingValid(key)
 if typeof(key)~="EnumItem"then return false end
 if key.EnumType==Enum.KeyCode then return key~=Enum.KeyCode.Unknown end
 return key.EnumType==Enum.UserInputType and (key==Enum.UserInputType.MouseButton1 or key==Enum.UserInputType.MouseButton2 or key==Enum.UserInputType.MouseButton3)
end
local function bindingMatches(input,key)
 return key~=nil and (input.KeyCode==key or input.UserInputType==key)
end
local function bindingDown(key)
 if typeof(key)=="EnumItem"and key.EnumType==Enum.UserInputType then return Input:IsMouseButtonPressed(key)end
 return Input:IsKeyDown(key)
end

local Http=game:GetService("HttpService")
local alive,enabled=true,false
local connections={}
local active
local awaiting={}
local records={}
local ping,pingAt=nil,0
-- Median additional effective release delay from 10 recorded manual shots.
-- This is an empirical initial estimate, NOT a recovered server timing formula.
local extraDelay=0.0192117
local profiles={}
local pingJitter=0
local frameTime=1/60
local lastFrame
local total,perfect=0,0
local lastResult="—"
local persistence="memory only"
local function finite(v) return type(v)=="number" and v==v and math.abs(v)<math.huge end
if readfile then
    local ok,saved=pcall(function() return Http:JSONDecode(readfile("PracticalPeakProfiles-v2.json")) end)
    if ok and type(saved)=="table" and saved.version==2 and type(saved.profiles)=="table" then
        for key,profile in pairs(saved.profiles) do
            if type(key)=="string" and #key<80 and type(profile)=="table"
                and finite(profile.delay) and profile.delay>=0 and profile.delay<=0.06
                and finite(profile.confirmations) and profile.confirmations>=1 then
                profiles[key]={delay=profile.delay,confirmations=math.min(profile.confirmations,1000)}
            end
        end
    end
end
local function profileKey(velocity,currentPing)
    return "Bat/"..math.floor(velocity*4+0.5).."/"..math.floor(currentPing*1000/20+0.5)
end
local keyboard
local sequence=0
local gui=Instance.new("ScreenGui") gui.Name="PracticalSuite" gui.ResetOnSpawn=false gui.Parent=P.PlayerGui
