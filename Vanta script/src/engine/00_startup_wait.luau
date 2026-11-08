-- VANTA_STARTUP_WAIT: readiness gate for manual execution, autoexec and teleports.
do
 if not game:IsLoaded()then game.Loaded:Wait()end
 local players=game:GetService("Players")
 while not players.LocalPlayer do task.wait(.1)end
 players.LocalPlayer:WaitForChild("PlayerGui")
 -- Allow replicated controllers and interface scripts time to initialize.
 task.wait(10)
end

