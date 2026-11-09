-------------------------------------------------------------------- settings → interface
local borderOn = false
function UI.setBorder(on)
	borderOn = on
	borderGrad.Enabled = on
	winStroke.Color = on and C.white or C.stroke
	winStroke.Thickness = on and 1.5 or 1.2
end

function UI.setAmbient(on)
	if on == ambientOn then
		return
	end
	ambientOn = on
	UI.tween(ambient, 0.5, { GroupTransparency = on and 0 or 1 })
end

function UI.setTheme(name, t)
	UI.deriveTheme(THEME_BY_NAME[name] or THEMES[1])
	UI.applyThemeRegistry(t or 0.45)
	repaintAll(0.45)
	refreshNav(0.3)
	if refreshMini then
		refreshMini()
	end
end

applySettings = function(t)
	t = t or 0.35
	local motion = CTRL.motion
	local m = motion.enabled == true
	MOTION.factor = m and SETTING.motion.speed.value / 0.3 or 1
	MOTION.style = m and EASINGS[SETTING.motion.curve.value] or QUINT
	MOTION.reduce = m and SETTING.motion.reduce.value or false

	local ap = CTRL.appearance.enabled == true
	local themeName = ap and SETTING.appearance.theme.value or THEMES[1].name
	if themeName ~= A.name then
		UI.setTheme(themeName, t)
	end
	local alpha = 1 - (ap and SETTING.appearance.opacity.value / 100 or 1)
	UI.tween(win, t, { BackgroundTransparency = alpha })
	UI.tween(rail, t, { BackgroundTransparency = alpha * 0.6 })
	for _, f in ipairs(railFillers) do
		UI.tween(f, t, { BackgroundTransparency = alpha * 0.6 })
	end
	UI.setBorder(ap and SETTING.appearance.border.value or false)
	UI.setAmbient(ap and SETTING.appearance.ambient.value or false)

	local lay = CTRL.layout.enabled == true
	UI.tween(listLayout, t, { Padding = UDim.new(0, lay and SETTING.layout.spacing.value or 14) })
	local density = lay and DENSITY[SETTING.layout.density.value] or 1
	local captions = not lay or SETTING.layout.labels.value
	local dividers = not lay or SETTING.layout.dividers.value
	local relayout = density ~= layoutState.density or captions ~= layoutState.captions or dividers ~= layoutState.dividers
	layoutState.density, layoutState.captions, layoutState.dividers = density, captions, dividers
	if relayout then
		rerender(0.32) -- let the clicked control finish its own animation first
	end
end

function UI.describe(ctrlId, id, value)
	local ctrl = CTRL[ctrlId]
	if not ctrl then
		return nil
	end
	if id == "enabled" then
		return ctrl.name .. (value and " on" or " off")
	end
	local s = SETTING[ctrlId][id]
	if not s then
		return nil
	end
	local v
	if typeof(value) == "number" then
		v = string.format(s.format or "%.1f", value) .. (s.suffix or "")
	elseif typeof(value) == "boolean" then
		v = value and "on" or "off"
	elseif typeof(value) == "EnumItem" then
		v = value.Name
	else
		v = tostring(value)
	end
	return (s.label or id) .. "  ·  " .. v
end

local fit, centerWindow -- forward (window placement)

local ACTIONS = {
	preview = function()
		toast("Notifications", "This is how a notification looks.", "bell", true)
	end,
	resetLayout = function()
		local ctrl = CTRL.layout
		UI.forEachValue(ctrl, function(s)
			s.value = s.default
		end)
		applySettings()
		repaintAll()
		setStatus("Layout reset")
		toast("Layout reset", "Spacing and density are back to default", "refresh")
	end,
	refresh = function()
		renderPage(currentPage)
		setStatus("Page refreshed")
	end,
	burst = function()
		local msgs = {
			{ "Heads up", "Toasts stack from the corner you pick", "bell" },
			{ "Nice", "Click any toast to dismiss it early", "check" },
			{ "One more", "Turn off stacking to keep just one", "layers" },
		}
		for i, m in ipairs(msgs) do
			UI.after((i - 1) * 0.22, function()
				toast(m[1], m[2], m[3], true)
			end)
		end
	end,
	center = function()
		centerWindow()
		setStatus("Window centered")
	end,
}

do (function()
 local path="PracticalUI2Profiles-v1.json"
 local http=game:GetService("HttpService")
 local function encode(v)
  if typeof(v)=="EnumItem"then return {keyCode=v.Name,mouse=v.EnumType==Enum.UserInputType}end
  if type(v)=="table"then local r={}for k,x in pairs(v)do r[k]=encode(x)end return r end
  return v
 end
 local function validated(raw)
  if type(raw)~="table"then return nil end
  local result=UI.snapshot()
  UI.forEachControl(function(ctrl)
   local e=raw[ctrl.id];if type(e)~="table"then return end
   if type(e.enabled)=="boolean"then result[ctrl.id].enabled=e.enabled end
   if type(e.values)~="table"then return end
   UI.forEachValue(ctrl,function(s)
    local v=e.values[s.id]
    if typeof(s.value)=="EnumItem"then
     if type(v)=="table"and type(v.keyCode)=="string"then
      local ok,key=pcall(function()return (v.mouse and Enum.UserInputType or Enum.KeyCode)[v.keyCode]end)
      local mouse=ok and key and(key==Enum.UserInputType.MouseButton1 or key==Enum.UserInputType.MouseButton2 or key==Enum.UserInputType.MouseButton3)
      if ok and key and key~=Enum.KeyCode.Unknown and(not v.mouse or mouse)then result[ctrl.id].values[s.id]=key end
     end
    elseif type(s.value)=="number"then
     if type(v)=="number"and v==v and math.abs(v)<math.huge then result[ctrl.id].values[s.id]=math.clamp(v,s.min or -math.huge,s.max or math.huge)end
    elseif type(v)==type(s.value)and (not s.options or table.find(s.options,v))and (type(v)~="string"or #v<=2048)then result[ctrl.id].values[s.id]=v end
   end)
  end,true)
  return result
 end
 UI.persistProfiles=function()
  if type(writefile)~="function"then setStatus("File saving unavailable in this environment");return false end
  local ok,err=pcall(function()writefile(path,http:JSONEncode({version=1,uiSizingVersion=7,selected=currentProfile.name,autoLoad=UI.autoLoadProfile and UI.autoLoadProfile.name or false,profiles=encode(PROFILES)}))end)
  if not ok then setStatus("Config could not be written");warn("Practical config save failed: "..tostring(err))end
  return ok
 end
 UI.loadSavedProfiles=function()
  if type(readfile)~="function"then return false end
  local ok,raw=pcall(function()return http:JSONDecode(readfile(path))end)
  if not ok or type(raw)~="table"or raw.version~=1 or type(raw.profiles)~="table"then return false end
  local profiles,seen={},{}
  for _,e in ipairs(raw.profiles)do
   if #profiles>=100 then break end
   if type(e)=="table"and type(e.name)=="string"and #e.name>0 and #e.name<=80 and not seen[string.lower(e.name)]then
    local snap=validated(e.snap)
    -- Every execution starts at 1.0, including the selected auto-load config.
    if snap and snap.interface and snap.interface.values then
     snap.interface.values.uiScale=DEFAULT_SCALE
    end
    if snap then table.insert(profiles,{name=e.name,snap=snap,savedAt=type(e.savedAt)=="number"and e.savedAt or os.time()});seen[string.lower(e.name)]=true end
   end
  end
  if #profiles==0 then return false end
  PROFILES=profiles
  UI.autoLoadProfile=type(raw.autoLoad)=="string" and UI.findProfile(raw.autoLoad) or nil
  currentProfile=UI.autoLoadProfile or UI.findProfile(type(raw.selected)=="string"and raw.selected or "")or profiles[1]
  return true
 end
 if UI.loadSavedProfiles() then UI.persistProfiles() end
end)()end

-------------------------------------------------------------------- header profile picker + footer actions
profileDD = dropdown(win, UDim2.new(1, -32 - 230, 0, TITLE_H + 64), 230, 44, UI.profileNames, currentProfile.name, function(name)
	local p = UI.findProfile(name)
	if p then
		UI.loadProfile(p)
		if currentPage == "Configs" and renderedQuery == "" then
			renderPage("Configs", false)
		end
	end
end, {
	label = "New profile…",
	icon = "plus",
	onClick = function()
		openPage("Configs")
		UI.after(0.1, function()
			if configNameBox then
				configNameBox:CaptureFocus()
			end
		end)
	end,
})

local saveToken = 0
saveBtn.MouseButton1Click:Connect(function()
	currentProfile.snap = UI.snapshot()
	currentProfile.savedAt = os.time()
	controlChanged:Fire("profile", "saved", currentProfile.name)
	setStatus('"' .. currentProfile.name .. '" saved')
	toast("Profile saved", '"' .. currentProfile.name .. '" is up to date', "save")
	saveToken += 1
	local mine = saveToken
	saveLabel.Text = "Saved"
	UI.after(1.4, function()
		if saveToken == mine then
			saveLabel.TextTransparency = 1
			saveLabel.Text = "Save profile"
			UI.tween(saveLabel, 0.3, { TextTransparency = 0 })
		end
	end)
	if currentPage == "Configs" and renderedQuery == "" then
		renderPage("Configs", false)
	end
end)

local revertAngle = 0
revertBtn.MouseButton1Click:Connect(function()
	revertAngle += 360
	UI.spinIcon(revertIcon, "refresh", C.white, 1.8, revertAngle, 0.7)
	UI.after(0.75, function()
		UI.tintIcon(revertIcon, C.text2, 0.2)
	end)
	UI.restore(currentProfile.snap)
	controlChanged:Fire("profile", "reverted", currentProfile.name)
	setStatus("Reverted to saved")
end)

resetBtn.MouseButton1Click:Connect(function()
	UI.forEachControl(function(ctrl)
		ctrl.enabled = ctrl.defaultEnabled
		UI.forEachValue(ctrl, function(s)
			s.value = s.default
		end)
	end)
	applySettings()
	repaintAll()
	refreshNav()
	setStatus("Defaults restored")
	toast("Defaults restored", "Every feature is back to its starting value", "refresh")
	controlChanged:Fire("profile", "reset", currentProfile.name)
end)

testBtn.MouseButton1Click:Connect(function()
	testControls:Fire()
end)

