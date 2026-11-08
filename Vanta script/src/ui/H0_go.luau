-------------------------------------------------------------------- go
applySettings(0)
openPage(START_PAGE)
navHighlight.Parent=nav[START_PAGE].btn.Parent
navHighlight.Position = nav[START_PAGE].pos
refreshNav(0)
UI.refreshStatus()
refreshMini()
gui.Parent = playerGui
fit(0)
task.defer(setMode, "open")

