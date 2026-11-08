-------------------------------------------------------------------- list
local list = UI.make("ScrollingFrame", {
	BackgroundTransparency = 1,
	BorderSizePixel = 0,
	Position = UDim2.fromOffset(CONTENT_X - 6, LIST_TOP),
	Size = UDim2.new(1, -(CONTENT_X - 6) - 18, 1, -LIST_TOP - FOOT_H - 4),
	CanvasSize = UDim2.new(),
	AutomaticCanvasSize = Enum.AutomaticSize.Y,
	ScrollBarThickness = 4,
	ScrollBarImageColor3 = C.strokeHi,
	ScrollingDirection = Enum.ScrollingDirection.Y,
	Parent = win,
}, { UI.padding(4, 14, 4, 18) })
local listLayout = UI.make("UIListLayout", { SortOrder = Enum.SortOrder.LayoutOrder, Padding = UDim.new(0, 14), Parent = list })
list:GetPropertyChangedSignal("CanvasPosition"):Connect(function()
	closePopup()
end)

function UI.setListTop(top, t)
	UI.tween(list, t, {
		Position = UDim2.fromOffset(CONTENT_X - 6, top),
		Size = UDim2.new(1, -(CONTENT_X - 6) - 18, 1, -top - FOOT_H - 4),
	})
end

local emptyLabel = text(win, "", CONTENT_X, LIST_TOP + 34, 17, { color = C.sub })
emptyLabel.Visible = false

-- every list entry is a slot (sized by the list) holding one "Body" frame (animated)
function UI.newSlot(h, order, name)
	local slot = UI.make("Frame", {
		Name = name or ("slot" .. order),
		BackgroundTransparency = 1,
		Size = UDim2.new(1, 0, 0, h + 4),
		LayoutOrder = order,
		Parent = list,
	})
	local body = UI.make("Frame", {
		Name = "Body",
		BackgroundTransparency = 1,
		Size = UDim2.fromScale(1, 1),
		Parent = slot,
	})
	return slot, body
end

-------------------------------------------------------------------- footer
UI.strip(win, UDim2.new(0, SIDE_W + 1, 1, -FOOT_H), UDim2.new(1, -SIDE_W - 1, 0, 1), C.line)

local statusDot = UI.make("Frame", {
	BackgroundColor3 = C.green,
	BorderSizePixel = 0,
	AnchorPoint = Vector2.new(0.5, 0.5),
	Position = UDim2.new(0, CONTENT_X + 5, 1, -FOOT_H / 2),
	Size = UDim2.fromOffset(9, 9),
	Parent = win,
}, { UI.round() })
local statusHalo = UI.pulse(statusDot, C.green)

local saveBtn, saveLabel = UI.primaryButton(win, "Save profile", "save", 42)
saveBtn.AnchorPoint = Vector2.new(1, 0.5)
saveBtn.Position = UDim2.new(1, -28, 1, -FOOT_H / 2)
local footX = 28 + saveBtn.Size.X.Offset + 10
local testBtn = UI.ghostButton(win, "Test controls", "check", 42)
testBtn.AnchorPoint = Vector2.new(1, 0.5)
testBtn.Position = UDim2.new(1, -footX, 1, -FOOT_H / 2)
footX += testBtn.Size.X.Offset + 10
local revertBtn, revertIcon = UI.iconButton(win, UDim2.new(1, -footX - 21, 1, -FOOT_H / 2), 42, "refresh", { solid = true, tip = "Revert to saved profile" })
footX += 42 + 18
UI.strip(win, UDim2.new(1, -footX, 1, -FOOT_H / 2 - 12), UDim2.fromOffset(1, 24), C.stroke)
footX += 18
local resetBtn = text(win, "Reset defaults", UDim2.new(1, -footX, 1, -FOOT_H / 2), 0, 15, { color = C.sub, ax = 1, button = true, weight = MEDIUM })
footX += UI.textWidth("Reset defaults", 15) + 24
local statusLabel = text(win, "Ready", UDim2.new(0, CONTENT_X + 20, 1, -FOOT_H / 2), 0, 15, { color = C.text2 })
UI.clip(statusLabel, UDim.new(1, -(CONTENT_X + 20) - footX))
resetBtn.MouseEnter:Connect(function()
	UI.tween(resetBtn, 0.15, { TextColor3 = C.white })
end)
resetBtn.MouseLeave:Connect(function()
	UI.tween(resetBtn, 0.25, { TextColor3 = C.sub })
end)
UI.tooltip(testBtn, "Run a check of every control")
UI.tooltip(saveBtn, "Save current settings to the active profile")

-- resize grip
local grip = UI.make("TextButton", {
	Text = "",
	AutoButtonColor = false,
	BackgroundTransparency = 1,
	AnchorPoint = Vector2.new(1, 1),
	Position = UDim2.new(1, -3, 1, -3),
	Size = UDim2.fromOffset(20, 20),
	ZIndex = 20,
	Parent = win,
})
local gripIcon = UI.icon(grip, "grip", 10, 10, 18, C.muted, 1.8)
grip.MouseEnter:Connect(function()
	UI.tintIcon(gripIcon, C.white, 0.15)
end)
grip.MouseLeave:Connect(function()
	UI.tintIcon(gripIcon, C.muted, 0.25)
end)

-------------------------------------------------------------------- status line
local statusToken = 0
function UI.baseStatus()
	local on, total = UI.featureCounts()
	if on == 0 then
		return "All features off", C.muted
	end
	return string.format("Ready  ·  %d of %d features on", on, total), C.green
end

function UI.showStatus(str, color)
	statusLabel.Text = str
	statusLabel.TextTransparency = 1
	statusLabel.Position = UDim2.new(0, CONTENT_X + 20, 1, -FOOT_H / 2 + 6)
	UI.tween(statusLabel, 0.3, { TextTransparency = 0, Position = UDim2.new(0, CONTENT_X + 20, 1, -FOOT_H / 2) })
	UI.tween(statusDot, 0.3, { BackgroundColor3 = color })
	statusHalo.BackgroundColor3 = color
end

setStatus = function(str)
	statusToken += 1
	local mine = statusToken
	UI.showStatus(str, A.hi)
	if refreshMini then
		refreshMini()
	end
	UI.after(2.6, function()
		if statusToken == mine then
			UI.showStatus(UI.baseStatus())
		end
	end)
end

function UI.refreshStatus()
	statusToken += 1
	UI.showStatus(UI.baseStatus())
end

-------------------------------------------------------------------- toasts
local TOAST_W, TOAST_H, TM = 420, 96, 16
local toastHost = UI.make("Frame", {
	Name = "Toasts",
	BackgroundTransparency = 1,
	AnchorPoint = Vector2.new(1, 1),
	Position = UDim2.new(1, -16, 1, -16),
	Size = UDim2.fromOffset(TOAST_W + TM * 2, 600),
	Parent = gui,
})
local toastScale = UI.make("UIScale", { Scale = DEFAULT_SCALE, Parent = toastHost })
UI.make("UIListLayout", {
	SortOrder = Enum.SortOrder.LayoutOrder,
	VerticalAlignment = Enum.VerticalAlignment.Bottom,
	Parent = toastHost,
})
local toastOrder = 0
local liveToasts = {}

function UI.placeToasts()
	local where = SETTING.notifications.position.value
	if where == "Left" then
		toastHost.AnchorPoint = Vector2.new(0, 1)
		toastHost.Position = UDim2.new(0, 16, 1, -16)
	elseif where == "Center" then
		toastHost.AnchorPoint = Vector2.new(0.5, 1)
		toastHost.Position = UDim2.new(0.5, 0, 1, -16)
	else
		toastHost.AnchorPoint = Vector2.new(1, 1)
		toastHost.Position = UDim2.new(1, -16, 1, -16)
	end
end

function UI.dismissToast(entry)
	if entry.gone then
		return
	end
	entry.gone = true
	local i = table.find(liveToasts, entry)
	if i then
		table.remove(liveToasts, i)
	end
	UI.groupFade(entry.wrap, 1, 0.2)
	UI.tween(entry.wrap, 0.25, { Position = UDim2.fromOffset(entry.dx * 0.6, entry.dx == 0 and 10 or 0) })
	UI.tween(entry.slot, 0.3, { Size = UDim2.new(1, 0, 0, 0) }, QUINT, OUT, 0.1).Completed:Connect(function()
		entry.slot:Destroy()
	end)
end

-- `force` shows it even when the Notifications feature is off (e.g. its own Preview button).
toast = function(title, message, iconName, force)
 local lines=math.clamp(math.ceil(UI.textWidth(message or "",14)/(TOAST_W-96))+1,2,6)
 local toastH=math.max(96,50+lines*18)
	if not force and not CTRL.notifications.enabled then
		return
	end
	UI.placeToasts()
	if not SETTING.notifications.stack.value then
		for _, e in ipairs(table.clone(liveToasts)) do
			UI.dismissToast(e)
		end
	elseif #liveToasts >= 3 then
		UI.dismissToast(liveToasts[1])
	end
	toastOrder += 1
	local where = SETTING.notifications.position.value
	local dx = where == "Left" and -56 or where == "Right" and 56 or 0

	local slot = UI.make("Frame", {
		BackgroundTransparency = 1,
		Size = UDim2.new(1, 0, 0, toastH + TM),
		LayoutOrder = toastOrder,
		Parent = toastHost,
	})
	local wrap = UI.make("Frame", { BackgroundTransparency = 1, Size = UDim2.fromScale(1, 1), Parent = slot })
	local shade = UI.make("Frame", {
		BackgroundTransparency = 1,
		Position = UDim2.fromOffset(TM, TM / 2),
		Size = UDim2.fromOffset(TOAST_W, toastH),
		Parent = wrap,
	})
	UI.softShadow(shade, 14, 3, 3, 0.09, 4)
 for layer=1,4 do
  local spread=layer*3
  local glow=UI.make("Frame",{
   Name="NotificationHalo",BackgroundTransparency=1,
   Position=UDim2.fromOffset(TM-spread,TM/2-spread),
   Size=UDim2.fromOffset(TOAST_W+spread*2,toastH+spread*2),Parent=wrap,
  },{UI.corner(16+spread)})
  UI.bindTheme(UI.stroke(A.a,2,.76+layer*.045),"Color","a").Parent=glow
 end
	local body = UI.make("TextButton", {
		Text = "",
		AutoButtonColor = false,
		BackgroundColor3 = C.raised,
		Position = UDim2.fromOffset(TM, TM / 2),
		Size = UDim2.fromOffset(TOAST_W, toastH),
		ZIndex = 2,
		Parent = wrap,
	}, { UI.corner(14), UI.stroke(C.strokeHi, 1) })
	local tile = UI.make("Frame", {
		BackgroundColor3 = C.white,
		AnchorPoint = Vector2.new(0, 0.5),
		Position = UDim2.fromOffset(16, toastH / 2),
		Size = UDim2.fromOffset(38, 38),
		Parent = body,
	}, { UI.corner(11), UI.accentGradient(45) })
	UI.icon(tile, iconName or "bell", 19, 19, 19, C.white, 1.8)
	UI.clip(text(body, title, 68, 25, 17, { weight = SEMIBOLD }), TOAST_W - 112)
	UI.make("TextLabel",{
  Name="NotificationMessage",BackgroundTransparency=1,Position=UDim2.fromOffset(68,43),
  Size=UDim2.fromOffset(TOAST_W-90,toastH-53),Text=message or "",TextWrapped=true,
  TextXAlignment=Enum.TextXAlignment.Left,TextYAlignment=Enum.TextYAlignment.Top,
  TextSize=14,TextColor3=C.sub,FontFace=UI.font(),Parent=body,
 })
	local closeIc = UI.icon(body, "close", TOAST_W - 22, 20, 12, C.muted, 1.8)

	local entry = { slot = slot, wrap = wrap, dx = dx }
	table.insert(liveToasts, entry)
	local dur = SETTING.notifications.duration.value
	if SETTING.notifications.progress.value then
		local bar = UI.make("Frame", {
			BackgroundColor3 = C.white,
			BorderSizePixel = 0,
			Position = UDim2.new(0, 16, 1, -3),
			Size = UDim2.new(1, -32, 0, 2),
			Parent = body,
		}, { UI.round(), UI.accentGradient(0) })
		-- real time, not scaled by the motion settings
		TweenService:Create(bar, TweenInfo.new(dur, LINEAR), { Size = UDim2.new(0, 0, 0, 2) }):Play()
	end
	body.MouseEnter:Connect(function()
		UI.tintIcon(closeIc, C.white, 0.15)
	end)
	body.MouseLeave:Connect(function()
		UI.tintIcon(closeIc, C.muted, 0.2)
	end)
	body.MouseButton1Click:Connect(function()
		UI.dismissToast(entry)
	end)

	wrap.Position = UDim2.fromOffset(dx, dx == 0 and 20 or 0)
	UI.groupFade(wrap, 0, 0.3)
	UI.tween(wrap, 0.55, { Position = UDim2.fromOffset(0, 0) }, BACK)
	UI.after(dur, function()
		UI.dismissToast(entry)
	end)
end

