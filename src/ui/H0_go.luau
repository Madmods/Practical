-------------------------------------------------------------------- go
applySettings(0)
openPage(START_PAGE)
navHighlight.Parent=nav[START_PAGE].btn.Parent
navHighlight.Position = nav[START_PAGE].pos
refreshNav(0)
UI.refreshStatus()
refreshMini()
gui.Parent = playerGui
task.defer(function()
	-- Let Roblox establish the mounted ScreenGui bounds before the first fit.
	RunService.RenderStepped:Wait()
	if not alive then return end
	UI.sizingReady = true
	fit(0)
	setMode("open")
end)

