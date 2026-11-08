-- One-shot test. Run while the native Leave Court control is available.
local storage=game:GetService("ReplicatedStorage")
local aero=storage:FindFirstChild("Aero")
local services=aero and aero:FindFirstChild("AeroRemoteServices")
local interface=services and services:FindFirstChild("InterfaceService")
local remote=interface and interface:FindFirstChild("LeaveRec")
if not remote or not remote:IsA("RemoteFunction")then
 warn("[LeaveRec test] RemoteFunction not found.")
 return
end
print("[LeaveRec test] Invoking once with no arguments...")
local ok,result=pcall(function()return remote:InvokeServer()end)
if ok then
 print("[LeaveRec test] Returned:",result)
else
 warn("[LeaveRec test] Failed:",result)
end
