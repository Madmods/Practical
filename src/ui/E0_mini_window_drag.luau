-------------------------------------------------------------------- minimized bar + hidden notice
local MINI_W, MINI_H, MM = 460, 60, 16
local mini = UI.make("Frame", {
	Name = "Mini",
	BackgroundTransparency = 1,
	AnchorPoint = Vector2.new(0.5, 0),
	Position = UDim2.new(0.5, 0, 0, 8),
	Size = UDim2.fromOffset(MINI_W + MM * 2, MINI_H + MM * 2),
	Visible = false,
	Parent = gui,
})
local miniScale = UI.make("UIScale", { Scale = DEFAULT_SCALE, Parent = mini })
local miniShade = UI.make("Frame", {
	BackgroundTransparency = 1,
	Position = UDim2.fromOffset(MM, MM),
	Size = UDim2.fromOffset(MINI_W, MINI_H),
	Parent = mini,
})
UI.softShadow(miniShade, MINI_H / 2, 4, 3, 0.08, 4)
local miniBody = UI.make("TextButton", {
	Text = "",
	AutoButtonColor = false,
	BackgroundColor3 = C.bg,
	BackgroundTransparency = 0.03,
	Position = UDim2.fromOffset(MM, MM),
	Size = UDim2.fromOffset(MINI_W, MINI_H),
	ZIndex = 2,
	Parent = mini,
}, { UI.round(), UI.bindTheme(UI.stroke(A.soft, 1), "Color", "soft") })
UI.logoMark(miniBody, 32, MINI_H / 2, 30)
text(miniBody, "practical", 58, MINI_H / 2, 18, { weight = SEMIBOLD })
UI.strip(miniBody, UDim2.fromOffset(152, 18), UDim2.fromOffset(1, 24), C.stroke)
local miniDot = UI.make("Frame", {
	BackgroundColor3 = A.a,
	BorderSizePixel = 0,
	AnchorPoint = Vector2.new(0.5, 0.5),
	Position = UDim2.fromOffset(172, MINI_H / 2),
	Size = UDim2.fromOffset(9, 9),
	Parent = miniBody,
}, { UI.round() })
local miniHalo = UI.pulse(miniDot, A.a)
local miniStatus = UI.clip(text(miniBody, "", 186, MINI_H / 2, 15, { color = C.text2, weight = MEDIUM }), MINI_W - 186 - 92)
local miniRestore = UI.iconButton(miniBody, UDim2.new(1, -70, 0.5, 0), 34, "restore", { radius = 17 })
local miniClose = UI.iconButton(miniBody, UDim2.new(1, -32, 0.5, 0), 34, "close", { radius = 17, hoverColor = C.dangerFill })

refreshMini = function(override)
	local on = UI.featureCounts()
	miniStatus.Text = override or string.format("%s  ·  %d on", currentProfile and currentProfile.name or "", on)
	local color = on > 0 and A.a or C.muted
	miniDot.BackgroundColor3 = color
	miniHalo.BackgroundColor3 = color
end

-- notice shown after closing the window, so the open key isn't forgotten
local notice = UI.make("Frame", {
	BackgroundTransparency = 1,
	AnchorPoint = Vector2.new(0.5, 0),
	Position = UDim2.new(0.5, 0, 0, 8),
	Size = UDim2.fromOffset(400, 48 + MM * 2),
	Visible = false,
	Parent = gui,
})
local noticeScale = UI.make("UIScale", { Scale = DEFAULT_SCALE, Parent = notice })
local noticeShade = UI.make("Frame", {
	BackgroundTransparency = 1,
	Position = UDim2.fromOffset(MM, MM),
	Size = UDim2.new(1, -MM * 2, 1, -MM * 2),
	Parent = notice,
})
UI.softShadow(noticeShade, 24, 4, 3, 0.08, 4)
local noticeBody = UI.make("Frame", {
	BackgroundColor3 = C.bg,
	BackgroundTransparency = 0.03,
	Position = UDim2.fromOffset(MM, MM),
	Size = UDim2.new(1, -MM * 2, 1, -MM * 2),
	ZIndex = 2,
	Parent = notice,
}, { UI.round(), UI.stroke(C.stroke, 1) })
local noticeLabel = text(noticeBody, "", 20, 24, 16, { color = C.text2, weight = MEDIUM })
local noticeKey = UI.make("Frame", {
	BackgroundColor3 = C.raised,
	AnchorPoint = Vector2.new(0, 0.5),
	Position = UDim2.fromOffset(0, 24),
	Size = UDim2.fromOffset(60, 28),
	Parent = noticeBody,
}, { UI.corner(7), UI.stroke(C.strokeHi, 1) })
local noticeKeyText = UI.make("TextLabel", {
	BackgroundTransparency = 1,
	Size = UDim2.fromScale(1, 1),
	Text = "",
	FontFace = UI.font(SEMIBOLD),
	TextSize = 14,
	TextColor3 = C.text,
	Parent = noticeKey,
})
local noticeToken = 0
function UI.showNotice()
	noticeToken += 1
	local mine = noticeToken
	local lead = "practical is hidden  ·  press"
	local keyName = PREF_KEY.value.Name
	noticeLabel.Text = lead
	noticeKeyText.Text = keyName
	local lw = UI.textWidth(lead, 16)
	local kw = UI.textWidth(keyName, 14) + 22
	noticeKey.Size = UDim2.fromOffset(kw, 28)
	noticeKey.Position = UDim2.fromOffset(20 + lw + 10, 24)
	notice.Size = UDim2.fromOffset(20 + lw + 10 + kw + 14 + MM * 2, 48 + MM * 2)
	notice.Visible = true
	notice.Position = UDim2.new(0.5, 0, 0, -4)
	UI.groupFade(notice, 0, 0.35)
	UI.tween(notice, 0.5, { Position = UDim2.new(0.5, 0, 0, 8) }, BACK)
	UI.after(3.4, function()
		if mine == noticeToken then
			UI.groupFade(notice, 1, 0.4).Completed:Connect(function(state)
				if state == Enum.PlaybackState.Completed and mine == noticeToken then
					notice.Visible = false
				end
			end)
		end
	end)
end
function UI.hideNotice()
	noticeToken += 1
	if notice.Visible then
		UI.groupFade(notice, 1, 0.15).Completed:Connect(function()
			notice.Visible = false
		end)
	end
end

-------------------------------------------------------------------- open / minimize / close
local mode = "closed" -- "open" | "mini" | "closed"

function UI.showWindow()
	root.Visible = true
	popScale.Scale = 0.94
	UI.groupFade(group, 0, 0.3)
	UI.tween(popScale, 0.55, { Scale = 1 }, BACK)
end

function UI.hideWindow()
	closePopup()
	UI.tween(popScale, 0.22, { Scale = 0.95 }, QUINT, IN)
	UI.groupFade(group, 1, 0.22, 0, QUINT, IN).Completed:Connect(function(state)
		if state == Enum.PlaybackState.Completed and mode ~= "open" then
			root.Visible = false
		end
	end)
end

function UI.showMini()
	refreshMini()
	mini.Visible = true
	mini.Position = UDim2.new(0.5, 0, 0, -14)
	UI.groupFade(mini, 0, 0.35, 0.1)
	UI.tween(mini, 0.55, { Position = UDim2.new(0.5, 0, 0, 8) }, BACK, OUT, 0.1)
end

function UI.hideMini()
	if not mini.Visible then
		return
	end
	UI.tween(mini, 0.2, { Position = UDim2.new(0.5, 0, 0, -4) })
	UI.groupFade(mini, 1, 0.2).Completed:Connect(function(state)
		if state == Enum.PlaybackState.Completed and mode ~= "mini" then
			mini.Visible = false
		end
	end)
end

setMode = function(m)
	if m == mode then
		return
	end
	local prev = mode
	mode = m
	if m == "open" then
		UI.hideMini()
		UI.hideNotice()
		UI.showWindow()
	elseif m == "mini" then
		UI.hideNotice()
		UI.hideWindow()
		UI.showMini()
	else -- closed
		if prev == "open" then
			UI.hideWindow()
		else
			UI.hideMini()
		end
		if PREF_NOTICE.value then
			UI.showNotice()
		end
	end
end

minBtn.MouseButton1Click:Connect(function()
	setMode("mini")
end)
closeBtn.MouseButton1Click:Connect(function()
	setMode("closed")
end)
miniBody.MouseButton1Click:Connect(function()
	setMode("open")
end)
miniRestore.MouseButton1Click:Connect(function()
	setMode("open")
end)
miniClose.MouseButton1Click:Connect(function()
	setMode("closed")
end)

-------------------------------------------------------------------- scale, position, drag, resize
local dragTarget -- top-left of `root` on screen (offsets)
local centered = false

centerWindow = function()
	local s = gui.AbsoluteSize
	local sc = fitScale.Scale
	dragTarget = UDim2.fromOffset(
		math.floor((s.X - (targetW + PAD * 2) * sc) / 2),
		math.floor((s.Y - (targetH + PAD * 2) * sc) / 2)
	)
end

fit = function(t)
	local s = gui.AbsoluteSize
	if s.X == 0 then
		return
	end
	local pref = PREF_SCALE.value
 local availableW=math.max(MIN_W,s.X*.96/pref-PAD*2)
 local availableH=math.max(MIN_H,s.Y*.96/pref-PAD*2)
 targetW=math.min(targetW,availableW);targetH=math.min(targetH,availableH)
 -- Apply immediately when fitting, rather than temporarily scaling the old rectangle.
 winW=targetW;winH=targetH;UI.applySize()
	local sc = math.min(pref, s.X * 0.98 / (winW + PAD * 2), s.Y * 0.98 / (winH + PAD * 2))
	UI.tween(fitScale, t or 0, { Scale = sc })
	UI.tween(miniScale, t or 0, { Scale = pref })
	noticeScale.Scale = pref
	toastScale.Scale = pref
	if not centered then
		centered = true
		fitScale.Scale = sc
		centerWindow()
		root.Position = dragTarget
	end
end
gui:GetPropertyChangedSignal("AbsoluteSize"):Connect(function()
	fit(0)
end)
PREF_SCALE.onSet = function()
	fit(0.2)
end

local dragStart, dragOrigin
local resizing -- { start, w, h }
local lastTitleClick = 0
local restoreRect -- size/position before maximise

function UI.toggleMaximize()
	local s = gui.AbsoluteSize
	local sc = fitScale.Scale
	if restoreRect then
		targetW, targetH = restoreRect.w, restoreRect.h
		dragTarget = restoreRect.pos
		restoreRect = nil
		setStatus("Window restored")
	else
		restoreRect = { w = targetW, h = targetH, pos = dragTarget }
		targetW = math.max(MIN_W, (s.X - 24) / sc)
		targetH = math.max(MIN_H, (s.Y - 24) / sc)
		dragTarget = UDim2.fromOffset(12 - PAD * sc, 12 - PAD * sc)
		setStatus("Window maximized  ·  double-click the title bar to restore")
	end
end

titleBar.InputBegan:Connect(function(io)
	if io.UserInputType == Enum.UserInputType.MouseButton1 or io.UserInputType == Enum.UserInputType.Touch then
		local now = os.clock()
		if now - lastTitleClick < 0.32 then
			lastTitleClick = 0
			dragStart = nil
			UI.toggleMaximize()
			return
		end
		lastTitleClick = now
		dragStart = io.Position
		dragOrigin = dragTarget
	end
end)
grip.InputBegan:Connect(function(io)
	if io.UserInputType == Enum.UserInputType.MouseButton1 or io.UserInputType == Enum.UserInputType.Touch then
		closePopup()
		restoreRect = nil
		resizing = { start = io.Position, w = targetW, h = targetH }
	end
end)
UI.connect(UserInputService.InputChanged, function(io)
	if io.UserInputType ~= Enum.UserInputType.MouseMovement and io.UserInputType ~= Enum.UserInputType.Touch then
		return
	end
	if dragStart and dragOrigin then
		local d = io.Position - dragStart
		if restoreRect and d.Magnitude > 6 then
			restoreRect = nil -- dragging a maximised window un-maximises it in place
		end
		local s = gui.AbsoluteSize
		local sc = fitScale.Scale
		local x = math.clamp(dragOrigin.X.Offset + d.X, -(winW + PAD) * sc + 120, s.X - (PAD * sc) - 120)
		local y = math.clamp(dragOrigin.Y.Offset + d.Y, -PAD * sc, s.Y - (PAD + TITLE_H) * sc)
		dragTarget = UDim2.fromOffset(x, y)
	elseif resizing then
		local d = io.Position - resizing.start
		local sc = fitScale.Scale
		local s = gui.AbsoluteSize
		local maxW = (s.X - root.AbsolutePosition.X) / sc - PAD - 4
		local maxH = (s.Y - root.AbsolutePosition.Y) / sc - PAD - 4
		targetW = math.clamp(resizing.w + d.X / sc, MIN_W, math.max(MIN_W, maxW))
		targetH = math.clamp(resizing.h + d.Y / sc, MIN_H, math.max(MIN_H, maxH))
	end
end)

-- per-frame: window follow, animated border, drifting ambient light
UI.connect(RunService.RenderStepped, function(dt)
	local a = 1 - math.exp(-dt * 22)
	if dragTarget then
		local p = root.Position
		if math.abs(p.X.Offset - dragTarget.X.Offset) + math.abs(p.Y.Offset - dragTarget.Y.Offset) > 0.05 then
			root.Position = p:Lerp(dragTarget, a)
		end
	end
	if math.abs(winW - targetW) + math.abs(winH - targetH) > 0.1 then
		winW += (targetW - winW) * a
		winH += (targetH - winH) * a
		UI.applySize()
	end
	if mode == "open" and not MOTION.reduce then
		if borderOn then
			borderGrad.Rotation = (borderGrad.Rotation + dt * 32) % 360
		end
		if ambientOn then
			local t = os.clock()
			for _, b in ipairs(blobs) do
				b.frame.Position = UDim2.fromScale(
					b.x + math.sin(t * 0.13 + b.phase) * 0.07,
					b.y + math.cos(t * 0.1 + b.phase) * 0.09
				)
			end
		end
	end
end)

